#!/usr/bin/env python3
"""Build the two future manual Release assets; never publish them."""

import argparse
import hashlib
import os
import shutil
import sys
import tempfile
from pathlib import Path

import amalgamate
import verify_release_assets


def build_assets(output):
    output = output.absolute()
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists() or output.is_symlink():
        raise ValueError(f"refusing to replace existing asset directory: {output}")
    header, source, version = amalgamate.generate()
    with tempfile.TemporaryDirectory(prefix="release-package-", dir=output.parent) as temp:
        workspace = Path(temp)
        _, archive = amalgamate.package(workspace / "amalgamation", header, source, version)
        staged = workspace / "assets"
        staged.mkdir()
        public_zip = staged / archive.name
        shutil.copyfile(archive, public_zip)
        digest = hashlib.sha256(public_zip.read_bytes()).hexdigest()
        sums = staged / f"LayerKeySort-{version}-SHA256SUMS.txt"
        sums.write_bytes(f"{digest}  {public_zip.name}\n".encode("ascii"))
        verify_release_assets.verify(staged)
        if output.exists() or output.is_symlink():
            raise ValueError(f"asset directory appeared during generation: {output}")
        staged.rename(output)
    return version, output, public_zip.name, sums.name, digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path("release-assets"))
    args = parser.parse_args()
    try:
        version, output, zip_name, sums_name, digest = build_assets(args.output_dir)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"release asset packaging failed: {error}\n")
    print(f"source version: {version}")
    print(f"assets: {output / zip_name}, {output / sums_name}")
    print(f"sha256: {digest}")
    # Metadata is for Actions orchestration; it is never packaged as an asset.
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8", newline="\n") as stream:
            stream.write(f"version={version}\n")


if __name__ == "__main__":
    main()
