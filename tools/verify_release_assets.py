#!/usr/bin/env python3
"""Independently verify the exact two-file future Release asset set."""

import argparse
import hashlib
import re
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION_PATTERN = r"[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.-]+)?"


def source_version():
    header = (ROOT / "include" / "layerkeysort.h").read_text(encoding="utf-8-sig")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8-sig")
    match = re.search(r'^#define LKS_VERSION_STRING "(' + VERSION_PATTERN + r')"$',
                      header, re.M)
    project = re.search(r'project\(LayerKeySort VERSION ([0-9]+\.[0-9]+\.[0-9]+)', cmake)
    if not match or not project or match.group(1).split("-", 1)[0] != project.group(1):
        raise ValueError("public-header and CMake project versions disagree")
    return match.group(1)


def verify(directory):
    if not directory.is_dir():
        raise ValueError(f"release asset directory is missing: {directory}")
    version = source_version()
    zip_name = f"LayerKeySort-{version}-amalgamation.zip"
    sums_name = f"LayerKeySort-{version}-SHA256SUMS.txt"
    expected = {zip_name, sums_name}
    actual = {path.name for path in directory.iterdir()}
    if actual != expected or any(not path.is_file() or path.is_symlink()
                                 for path in directory.iterdir()):
        raise ValueError(f"release asset membership differs: expected={sorted(expected)}, "
                         f"actual={sorted(actual)}")
    raw = (directory / sums_name).read_bytes()
    line = re.fullmatch(rb"([0-9a-f]{64})  ([A-Za-z0-9.-]+)\n", raw)
    if not line or line.group(2).decode("ascii") != zip_name:
        raise ValueError("SHA256SUMS must contain one canonical LF-terminated entry")
    digest = hashlib.sha256((directory / zip_name).read_bytes()).hexdigest()
    if line.group(1).decode("ascii") != digest:
        raise ValueError("SHA-256 checksum mismatch")
    root = f"LayerKeySort-{version}-amalgamation/"
    members = {root + name for name in
               ("layerkeysort.h", "layerkeysort.c", "example.c", "LICENSE", "README.txt")}
    with zipfile.ZipFile(directory / zip_name) as archive:
        listed = archive.namelist()
        if len(listed) != len(members) or set(listed) != members or archive.testzip():
            raise ValueError("amalgamation ZIP membership or integrity differs")
    return version, zip_name, sums_name, digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--summary", action="store_true", help="print Markdown for a CI job summary")
    args = parser.parse_args()
    try:
        version, zip_name, sums_name, digest = verify(args.directory)
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, f"release asset verification failed: {error}\n")
    if args.summary:
        print(f"Version from source: `{version}`")
        print(f"- `{zip_name}` — SHA-256 `{digest}`")
        print(f"- `{sums_name}` — checksum manifest")
    else:
        print(f"verified {zip_name} sha256={digest}")
        print(f"verified exact asset set: {zip_name}, {sums_name}")


if __name__ == "__main__":
    main()
