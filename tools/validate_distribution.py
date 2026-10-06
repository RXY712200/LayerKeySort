#!/usr/bin/env python3
"""Exercise source, installed, and generated distribution as external projects."""

import argparse
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(*command):
    print("+", *map(str, command), flush=True)
    subprocess.run([str(part) for part in command], check=True)


def executable(build, name):
    suffix = ".exe" if sys.platform == "win32" else ""
    for path in (build / "Debug" / (name + suffix), build / (name + suffix)):
        if path.is_file():
            return path
    raise RuntimeError(f"built executable not found: {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--generator")
    parser.add_argument("--architecture", help="CMake generator platform, for example x64")
    parser.add_argument("--c-compiler")
    parser.add_argument("--cxx-compiler")
    parser.add_argument("--make-program")
    args = parser.parse_args()
    cmake = args.cmake
    configure_flags = []
    if sys.platform != "win32" or (args.generator and args.generator in
                                  {"MinGW Makefiles", "Unix Makefiles", "Ninja"}):
        configure_flags.append("-DCMAKE_BUILD_TYPE=Debug")
    if args.generator:
        configure_flags += ["-G", args.generator]
    if args.architecture:
        configure_flags += ["-A", args.architecture]
    for name, value in (("CMAKE_C_COMPILER", args.c_compiler),
                        ("CMAKE_CXX_COMPILER", args.cxx_compiler),
                        ("CMAKE_MAKE_PROGRAM", args.make_program)):
        if value:
            configure_flags.append(f"-D{name}={value}")

    scratch = ROOT / "build"
    scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="distribution-check-", dir=scratch) as temp:
        work = Path(temp)

        def configure(source, build, *extra):
            run(cmake, "-S", source, "-B", build, *configure_flags, *extra)

        def build(build_dir):
            run(cmake, "--build", build_dir, "--config", "Debug", "--parallel")

        # An external source-tree consumer uses the old target name and does
        # not receive examples/tests/install rules by default.
        source_fixture = work / "source-consumer"
        shutil.copytree(ROOT / "tests" / "distribution" / "consumer", source_fixture)
        source_build = work / "source-build"
        configure(source_fixture, source_build, f"-DLKS_SOURCE_DIR={ROOT}")
        build(source_build)
        run(executable(source_build, "consumer_c"))
        run(executable(source_build, "consumer_cpp"))

        fetch_build = work / "fetchcontent-build"
        configure(source_fixture, fetch_build,
                  f"-DLKS_FETCHCONTENT_SOURCE_DIR={ROOT}")
        build(fetch_build)
        run(executable(fetch_build, "consumer_c"))
        run(executable(fetch_build, "consumer_cpp"))
        # A fresh local static build is installed, then consumed without
        # adding the repository source tree to the consumer project.
        install_build, prefix = work / "install-build", work / "prefix"
        configure(ROOT, install_build, "-DLKS_BUILD_TESTS=OFF",
                  "-DLKS_BUILD_EXAMPLES=OFF", "-DLKS_INSTALL=ON")
        build(install_build)
        run(cmake, "--install", install_build, "--config", "Debug",
            "--prefix", prefix)
        installed_library = list(prefix.rglob("*layerkeysort.a")) + list(
            prefix.rglob("*layerkeysort.lib"))
        if (not (prefix / "include" / "layerkeysort.h").is_file() or
                not list(prefix.rglob("LayerKeySortConfig.cmake")) or
                not list(prefix.rglob("LayerKeySortTargets.cmake")) or
                not list(prefix.rglob("LICENSE")) or not installed_library):
            raise RuntimeError("installed package is missing a required component")
        if list(prefix.rglob("lks_*internal.h")):
            raise RuntimeError("private headers escaped into installed package")
        installed_fixture = work / "installed-consumer"
        shutil.copytree(ROOT / "tests" / "distribution" / "consumer", installed_fixture)
        installed_build = work / "installed-build"
        configure(installed_fixture, installed_build, f"-DCMAKE_PREFIX_PATH={prefix}")
        build(installed_build)
        run(executable(installed_build, "consumer_c"))
        run(executable(installed_build, "consumer_cpp"))

        # Two independent generations must have identical sources and ZIP
        # membership/content. The archive itself also has fixed timestamps.
        first_parent, second_parent = work / "package-a", work / "package-b"
        run(sys.executable, ROOT / "tools" / "amalgamate.py",
            "--package-parent", first_parent)
        run(sys.executable, ROOT / "tools" / "amalgamate.py",
            "--package-parent", second_parent)
        first_zip, second_zip = next(first_parent.glob("*.zip")), next(second_parent.glob("*.zip"))
        first_dir = next(path for path in first_parent.iterdir() if path.is_dir())
        second_dir = next(path for path in second_parent.iterdir() if path.is_dir())
        expected = {"layerkeysort.h", "layerkeysort.c", "example.c", "LICENSE", "README.txt"}
        if {path.name for path in first_dir.iterdir()} != expected:
            raise RuntimeError("amalgamation package membership differs from contract")
        for name in expected:
            if (first_dir / name).read_bytes() != (second_dir / name).read_bytes():
                raise RuntimeError(f"non-deterministic package member: {name}")
        if first_zip.read_bytes() != second_zip.read_bytes():
            raise RuntimeError("generated ZIP is not deterministic")
        with zipfile.ZipFile(first_zip) as archive:
            if {Path(name).name for name in archive.namelist()} != expected:
                raise RuntimeError("ZIP membership differs from package directory")
        amalgam_fixture = work / "amalgamation-consumer"
        shutil.copytree(ROOT / "tests" / "distribution" / "amalgamation", amalgam_fixture)
        shutil.copyfile(ROOT / "tests" / "distribution" / "consumer" / "snapshot_usage.h",
                        amalgam_fixture / "snapshot_usage.h")
        amalgam_build = work / "amalgamation-build"
        configure(amalgam_fixture, amalgam_build, f"-DLKS_AMALGAM_DIR={first_dir}")
        build(amalgam_build)
        run(executable(amalgam_build, "amalgamation_consumer"))
        run(executable(amalgam_build, "amalgamation_package_example"))
        print("distribution validation passed", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, StopIteration) as error:
        sys.exit(f"distribution validation failed: {error}")
