#!/usr/bin/env python3
"""Generate, but never maintain by hand, the two-file C17 distribution."""

import argparse
import re
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "cmake" / "ProductionSources.cmake"
PUBLIC = ROOT / "include" / "layerkeysort.h"
LOCAL_INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"\n]+)"[^\n]*$', re.M)
VERSION = re.compile(
    r'^#define LKS_VERSION_STRING "([0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.-]+)?)"$', re.M)


def read(path):
    if not path.is_file():
        raise ValueError(f"required source is missing: {path}")
    return path.read_text(encoding="utf-8-sig").replace("\r\n", "\n")


def sources():
    manifest = re.sub(r"#.*", "", read(MANIFEST))
    match = re.fullmatch(r"\s*set\s*\(\s*LKS_SOURCES\s+([^)]*?)\s*\)\s*", manifest, re.S)
    if not match:
        raise ValueError("production manifest must contain only set(LKS_SOURCES ...)")
    names = match.group(1).split()
    if not names or len(names) != len(set(names)) or any(
        not re.fullmatch(r"src/[A-Za-z0-9_]+\.c", name) for name in names
    ):
        raise ValueError("invalid or duplicated production source in manifest")
    actual = {p.relative_to(ROOT).as_posix() for p in (ROOT / "src").glob("*.c")}
    if set(names) != actual:
        raise ValueError(f"production manifest mismatch: missing={sorted(actual-set(names))}, "
                         f"unexpected={sorted(set(names)-actual)}")
    # The legacy Visual Studio project cannot consume the CMake manifest.
    # Fail generation/CI if its production compile sets drift from CMake.
    for project_file in ("LayerKeySort.vcxproj", "LayerKeySort.vcxproj.filters"):
        root = ElementTree.fromstring(read(ROOT / project_file))
        listed = {
            node.attrib["Include"].replace("\\", "/")
            for node in root.iter()
            if node.tag.rsplit("}", 1)[-1] == "ClCompile"
            and node.attrib.get("Include", "").replace("\\", "/").startswith("src/")
        }
        if listed != set(names):
            raise ValueError(f"{project_file} production sources differ from manifest: "
                             f"missing={sorted(set(names)-listed)}, "
                             f"unexpected={sorted(listed-set(names))}")
    return [ROOT / name for name in names]


def strip_local_includes(content, source):
    def replace(match):
        name = match.group(1)
        if name != "layerkeysort.h" and not re.fullmatch(r"lks_[A-Za-z0-9_]+\.h", name):
            raise ValueError(f"unresolved repository-local include {name!r} in {source}")
        if name != "layerkeysort.h" and not (ROOT / "src" / name).is_file():
            raise ValueError(f"missing private include {name!r} in {source}")
        return f"/* bundled include: {name} */"
    return LOCAL_INCLUDE.sub(replace, content)


def private_headers(ordered_sources):
    visited, visiting, output = set(), set(), []

    def visit(name):
        if name == "layerkeysort.h" or name in visited:
            return
        path = ROOT / "src" / name
        if not re.fullmatch(r"lks_[A-Za-z0-9_]+\.h", name):
            raise ValueError(f"unresolved local include: {name}")
        if name in visiting:
            raise ValueError(f"private include cycle: {name}")
        visiting.add(name)
        content = read(path)
        for dependency in LOCAL_INCLUDE.findall(content):
            visit(dependency)
        visiting.remove(name)
        visited.add(name)
        output.append((name, strip_local_includes(content, path)))

    for path in ordered_sources:
        for name in LOCAL_INCLUDE.findall(read(path)):
            visit(name)
    return output


def generate():
    ordered = sources()
    header = read(PUBLIC)
    if not header.endswith("\n"):
        raise ValueError("public header must end with a newline")
    match = VERSION.search(header)
    if not match:
        raise ValueError("cannot derive package version from public header")
    project = re.search(r'project\(LayerKeySort VERSION ([0-9]+\.[0-9]+\.[0-9]+)',
                        read(ROOT / "CMakeLists.txt"))
    if not project or match.group(1).split("-", 1)[0] != project.group(1):
        raise ValueError("public-header and CMake project versions differ")
    parts = ["/* Generated from LayerKeySort's canonical C17 sources. Do not edit. */\n",
             '#include "layerkeysort.h"\n']
    for name, content in private_headers(ordered):
        parts.append(f"\n/* Begin private header: src/{name} */\n")
        parts.append(content.rstrip("\n") + "\n")
        parts.append(f"/* End private header: src/{name} */\n")
    for path in ordered:
        name = path.relative_to(ROOT).as_posix()
        parts.append(f"\n/* Begin production source: {name} */\n")
        parts.append(strip_local_includes(read(path), path).rstrip("\n") + "\n")
        parts.append(f"/* End production source: {name} */\n")
    if any(LOCAL_INCLUDE.findall(part) for part in parts[2:]):
        raise ValueError("unresolved local include remains in implementation")
    return header, "".join(parts), match.group(1)


def write_files(directory, header, source):
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "layerkeysort.h").write_bytes(header.encode("utf-8"))
    (directory / "layerkeysort.c").write_bytes(source.encode("utf-8"))


def package(parent, header, source, version):
    name = f"LayerKeySort-{version}-amalgamation"
    directory = parent / name
    if directory.exists() and any(directory.iterdir()):
        raise ValueError(f"package directory is not empty: {directory}")
    write_files(directory, header, source)
    (directory / "example.c").write_bytes(read(ROOT / "examples" / "basic.c").encode("utf-8"))
    (directory / "LICENSE").write_bytes(read(ROOT / "LICENSE").encode("utf-8"))
    note = ("LayerKeySort generated C17 source package.\n"
            "layerkeysort.h: public API; layerkeysort.c: implementation.\n"
            "Also included: example.c, LICENSE, and this README.txt.\n"
            "Copy the two layerkeysort files into your project and include\n"
            "#include \"layerkeysort.h\" in your application.\n"
            "GCC/Clang: cc -std=c17 -I. layerkeysort.c example.c -o example\n"
            "MSVC Developer Command Prompt: cl /std:c17 /I. layerkeysort.c example.c\n"
            "Replace example.c with your application source. Items remain caller-owned.\n"
            "Documentation: https://github.com/RXY712200/LayerKeySort\n")
    (directory / "README.txt").write_bytes(note.encode("utf-8"))
    archive = parent / f"{name}.zip"
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in sorted(directory.iterdir()):
            info = zipfile.ZipInfo(f"{name}/{path.name}", date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return directory, archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--output-dir", type=Path, help="write only layerkeysort.h/.c")
    group.add_argument("--package-parent", type=Path, help="write package directory and ZIP here")
    args = parser.parse_args()
    try:
        header, source, version = generate()
        if args.package_parent:
            directory, archive = package(args.package_parent, header, source, version)
            print(directory)
            print(archive)
        else:
            write_files(args.output_dir, header, source)
            print(args.output_dir)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"amalgamation generation failed: {error}\n")


if __name__ == "__main__":
    main()
