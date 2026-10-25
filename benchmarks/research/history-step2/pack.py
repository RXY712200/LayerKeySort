"""Compact a CI experiment's raw data without changing or discarding run records."""
import argparse
import gzip
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(); p.add_argument('output', type=Path); args = p.parse_args()
for capture in args.output.glob('capture-*'):
    packed = args.output/'delivery'/capture.name; packed.mkdir(parents=True, exist_ok=True)
    for name in ('runs.jsonl', 'environment.json'):
        if (capture/name).exists(): shutil.copyfile(capture/name, packed/name)
    records = []
    for path in sorted(capture.glob('*.json.gz')):
        records.append(gzip.decompress(path.read_bytes()).decode('utf-8'))
    (packed/'raw.jsonl.gz').write_bytes(gzip.compress(('\n'.join(records)+'\n').encode(), mtime=0))
for path in args.output.glob('analysis-*'):
    shutil.copytree(path, args.output/'delivery'/path.name, dirs_exist_ok=True)
for pattern in ('*.txt', '*.json', '*.log'):
    for path in args.output.glob(pattern): shutil.copyfile(path, args.output/'delivery'/path.name)
root = args.output/'delivery'; root.mkdir(parents=True, exist_ok=True)
manifest = []
for path in sorted(root.rglob('*')):
    if path.is_file() and path.name != 'SHA256SUMS.txt':
        manifest.append(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.relative_to(root).as_posix())
(root/'SHA256SUMS.txt').write_text('\n'.join(manifest)+'\n')
