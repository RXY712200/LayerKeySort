#!/usr/bin/env python3
"""Build or verify Mini-only source assets from one immutable Git commit.

This script never creates tags, GitHub Releases, or repository writes.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "CMakeLists.txt", "LICENSE", "README.md", "README.zh-CN.md",
    "include/layerkeysort_mini.h", "src/lks_mini.c",
}


def git(*args: str) -> bytes:
    return subprocess.run(
        ["git", "-C", str(ROOT), *args],
        check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    ).stdout


def snapshot(ref: str):
    commit = git("rev-parse", "--verify", f"{ref}^{{commit}}").decode("ascii").strip()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("expected a resolved 40-digit Git commit")
    cmake = git("show", f"{commit}:mini/CMakeLists.txt").decode("utf-8")
    match = re.search(
        r"^project\(LayerKeySortMini VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES C\)$",
        cmake, re.MULTILINE,
    )
    if not match:
        raise ValueError("Mini CMake version is missing or unsupported")
    version = match.group(1)
    entries = {}
    for record in git("ls-tree", "-r", "-z", f"{commit}:mini").split(b"\0"):
        if not record:
            continue
        metadata, name = record.split(b"\t", 1)
        mode, kind, blob = metadata.decode("ascii").split()
        path = name.decode("utf-8")
        parts = path.split("/")
        if (kind != "blob" or mode not in ("100644", "100755")
                or not path or any(part in ("", ".", "..") for part in parts)
                or "\\" in path or path in entries):
            raise ValueError(f"unsupported Mini archive entry: {path}")
        entries[path] = blob
    if not REQUIRED.issubset(entries):
        raise ValueError(f"required Mini files missing: {sorted(REQUIRED - entries.keys())}")
    return commit, version, entries


def names(version: str):
    stem = f"LayerKeySort-Mini-v{version}"
    return stem, f"{stem}-source.zip", f"{stem}-SHA256SUMS.txt"


def verify(directory: Path, ref: str):
    commit, version, entries = snapshot(ref)
    stem, archive_name, sums_name = names(version)
    if not directory.is_dir() or directory.is_symlink():
        raise ValueError(f"asset directory is missing or a symlink: {directory}")
    files = list(directory.iterdir())
    if ({path.name for path in files} != {archive_name, sums_name}
            or any(not path.is_file() or path.is_symlink() for path in files)):
        raise ValueError("Mini asset set must contain exactly the ZIP and checksum")
    archive_path = directory / archive_name
    checksum = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    actual_manifest = (directory / sums_name).read_bytes()
    expected_manifest = f"{checksum}  {archive_name}\n".encode("ascii")
    if actual_manifest != expected_manifest:
        raise ValueError("Mini asset checksum is missing or incorrect")
    directory_names = {stem + "/"}
    expected_files = {}
    for path, blob in entries.items():
        expected_files[f"{stem}/{path}"] = blob
        pieces = path.split("/")
        for i in range(1, len(pieces)):
            directory_names.add(f"{stem}/{'/'.join(pieces[:i])}/")
    with zipfile.ZipFile(archive_path) as archive:
        members = archive.namelist()
        if (len(members) != len(set(members))
                or set(members) != set(expected_files) | directory_names
                or archive.testzip() is not None):
            raise ValueError("Mini ZIP membership or CRC does not match source tree")
        for member, blob in expected_files.items():
            if archive.read(member) != git("cat-file", "blob", blob):
                raise ValueError(f"Mini ZIP content differs from Git: {member}")
        for member in directory_names:
            if not archive.getinfo(member).is_dir():
                raise ValueError(f"Mini ZIP directory entry is not a directory: {member}")
    return commit, version, archive_name, sums_name, checksum


def build(directory: Path, ref: str):
    commit, version, _ = snapshot(ref)
    stem, archive_name, sums_name = names(version)
    parent = directory.absolute().parent
    parent.mkdir(parents=True, exist_ok=True)
    if directory.exists() or directory.is_symlink():
        raise ValueError(f"refusing to replace existing asset directory: {directory}")
    with tempfile.TemporaryDirectory(prefix="mini-assets-", dir=parent) as temporary:
        staged = Path(temporary) / "assets"
        staged.mkdir()
        archive_path = staged / archive_name
        git("archive", "--format=zip", f"--prefix={stem}/",
            f"--output={archive_path}", f"{commit}:mini")
        digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
        (staged / sums_name).write_bytes(
            f"{digest}  {archive_name}\n".encode("ascii")
        )
        verify(staged, commit)
        if directory.exists() or directory.is_symlink():
            raise ValueError(f"asset directory appeared during build: {directory}")
        staged.rename(directory)
    return verify(directory, commit)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--ref", default="HEAD",
                        help="commit or ref to package/verify (default: HEAD)")
    parser.add_argument("--verify", action="store_true",
                        help="check an existing asset set without writing")
    args = parser.parse_args()
    try:
        result = (verify(args.output_dir, args.ref) if args.verify
                  else build(args.output_dir, args.ref))
    except (OSError, UnicodeError, ValueError, subprocess.CalledProcessError,
            zipfile.BadZipFile, KeyError) as error:
        parser.exit(1, f"Mini release asset validation failed: {error}\n")
    commit, version, archive_name, sums_name, checksum = result
    print(f"Mini v{version}; source commit {commit}")
    print(f"{archive_name}: sha256 {checksum}")
    print(f"{sums_name}: verified")
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
            output.write(f"version={version}\n")
            output.write(f"source_sha={commit}\n")


if __name__ == "__main__":
    main()
