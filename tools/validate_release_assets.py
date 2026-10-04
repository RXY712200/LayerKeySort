#!/usr/bin/env python3
"""Check release assets, reproducibility, and checksum failure detection."""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import verify_release_assets

ROOT = Path(__file__).resolve().parents[1]


def expect_rejection(directory):
    try:
        verify_release_assets.verify(directory)
    except ValueError:
        return
    raise RuntimeError(f"invalid release asset set was accepted: {directory}")


def validate(candidate=None):
    scratch = ROOT / "build"
    scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="release-check-", dir=scratch) as temp:
        work = Path(temp)
        first, second = work / "first", work / "second"
        for directory in (first, second):
            subprocess.run([sys.executable, str(ROOT / "tools" / "package_release.py"),
                            "--output-dir", str(directory)], check=True)
            verify_release_assets.verify(directory)
        names = {path.name for path in first.iterdir()}
        if names != {path.name for path in second.iterdir()}:
            raise RuntimeError("repeated asset membership differs")
        for name in names:
            if (first / name).read_bytes() != (second / name).read_bytes():
                raise RuntimeError(f"repeated asset bytes differ: {name}")
        if candidate is not None:
            verify_release_assets.verify(candidate)
            if names != {path.name for path in candidate.iterdir()} or any(
                (first / name).read_bytes() != (candidate / name).read_bytes()
                for name in names
            ):
                raise RuntimeError("candidate assets differ from independent generation")

        corrupt = work / "corrupt"
        shutil.copytree(first, corrupt)
        sums = next(corrupt.glob("*-SHA256SUMS.txt"))
        raw = sums.read_bytes()
        sums.write_bytes((b"0" if raw[:1] != b"0" else b"1") + raw[1:])
        expect_rejection(corrupt)
        sums.write_bytes(raw)
        (corrupt / "unexpected.txt").write_text("not a public asset\n", encoding="ascii")
        expect_rejection(corrupt)
    print("release asset validation passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset-dir", type=Path)
    args = parser.parse_args()
    try:
        validate(args.asset_dir)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"release asset validation failed: {error}\n")


if __name__ == "__main__":
    main()
