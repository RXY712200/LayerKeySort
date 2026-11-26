"""Copy verified capture bytes and seal raw process output for Git archival."""
import csv
import gzip
import hashlib
import json
import shutil
import sys
import zipfile
from pathlib import Path

source = Path(sys.argv[1])
out = Path(__file__).resolve().parent/'final-matrix'
for host in ('Linux','Windows'):
    src = source/host
    dest = out/host
    dest.mkdir(parents=True,exist_ok=False)
    calls = [json.loads(line) for line in (src/'invocations.jsonl').read_text().splitlines()]
    with (src/'raw.csv').open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    assert len(calls) == 3245 and len(rows) == 3240
    assert all(call['status']=='ok' and call['returncode']==0 and call['available']>=512*1024**2 for call in calls)
    expected = {call['id']+suffix for call in calls for suffix in ('.stdout.gz','.stderr.gz')}
    assert expected == {p.name for p in src.glob('*.gz')}
    # Collector row order is the successful invocation order after five gates.
    for row, call in zip(rows, calls[5:], strict=True):
        output = gzip.decompress((src/(call['id']+'.stdout.gz')).read_bytes()).decode().strip().split(',')
        assert len(output)==15 and float(output[0])==float(row['seconds'])
        if row['mode']=='diag':
            assert int(output[1])==int(row['requested_live'])
            assert int(output[5])==int(row['requested_after_destroy'])==0
    with zipfile.ZipFile(dest/'raw-process-output.zip','w',zipfile.ZIP_DEFLATED) as z:
        for name in sorted(expected):
            path=src/name
            gzip.decompress(path.read_bytes())
            z.write(path,name)
    with zipfile.ZipFile(dest/'raw-process-output.zip') as z:
        assert z.testzip() is None
    for name in ('raw.csv','metadata.json','invocations.jsonl'):
        shutil.copyfile(src/name,dest/name)
    (dest/'.gitattributes').write_text('raw.csv -text whitespace=cr-at-eol\nmetadata.json -text whitespace=cr-at-eol\ninvocations.jsonl -text whitespace=cr-at-eol\n',encoding='ascii',newline='\n')
    print(host,len(rows),len(calls),hashlib.sha256((dest/'raw.csv').read_bytes()).hexdigest())
