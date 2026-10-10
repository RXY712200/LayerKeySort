"""Fetch the published, pinned Mini-only package for external integration tests."""
import argparse
import hashlib
from pathlib import Path
import urllib.request
import zipfile

ASSETS = {
    'LayerKeySort-Mini-v1.0.0-source.zip': '96b21a4d9177a98991e27712063dab7ce4d91a4adc0a76e420e8ab74263089c4',
    'LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt': '722c2d9aa5bace6ce7d15cdb5f876c270d9a0c3105b882f8e1d041db9743141a',
}
if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    for name, expected in ASSETS.items():
        url='https://github.com/RXY712200/LayerKeySort/releases/download/mini-v1.0.0/'+name
        with urllib.request.urlopen(url,timeout=60) as response:
            data=response.read()
        if hashlib.sha256(data).hexdigest()!=expected:
            raise SystemExit('Published asset digest mismatch: '+name)
        (args.output/name).write_bytes(data)
    manifest=(args.output/'LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt').read_text().split()
    if manifest != [ASSETS['LayerKeySort-Mini-v1.0.0-source.zip'],'LayerKeySort-Mini-v1.0.0-source.zip']:
        raise SystemExit('Unexpected checksum manifest')
    with zipfile.ZipFile(args.output/'LayerKeySort-Mini-v1.0.0-source.zip') as archive:
        for name in archive.namelist():
            if not name.startswith('LayerKeySort-Mini-v1.0.0/') or '..' in Path(name).parts:
                raise SystemExit('Unsafe/unexpected package entry')
        archive.extractall(args.output)
    print('Verified published Mini v1.0.0 package:',args.output/'LayerKeySort-Mini-v1.0.0')
