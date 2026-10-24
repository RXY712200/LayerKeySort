"""One retry of instrumented admission deferrals, preserving the original rows.
Uses exactly the original safety inequalities; never retries policy history caps.
"""
import csv
import io
import json
import subprocess
import sys
import time
from pathlib import Path
from collect import memory, GIB

build, output = map(Path, sys.argv[1:3]); log=output/'runs.jsonl'
records=[json.loads(line) for line in log.read_text().splitlines()]
ids={r['id'] for r in records}
for old in records:
    if old['status']!='safely_skipped' or old.get('mode') not in ('allocation','structural'):
        continue
    label=old['id']+'-retry1'
    if label in ids:
        continue
    m=memory(); row={**old,'id':label,'retry_of':old['id'],'memory_before':m}
    planned=old['estimated_envelope_bytes']
    if planned>min(3*GIB,m['available']*.40) or m['available']-planned<3*GIB:
        row.update(status='safely_skipped',reason='unchanged conservative gate still not satisfied')
    else:
        exe='full_scale_alloc.exe' if old['mode']=='allocation' else 'full_scale_structural.exe'
        command=[str(build.resolve()/exe),old['profile'],str(old['scale']),str(old['seed'])]
        start=time.perf_counter();r=subprocess.run(command,capture_output=True,text=True,timeout=270)
        (output/f'{label}.csv').write_text(r.stdout,encoding='utf-8',newline='\n')
        (output/f'{label}.stderr.txt').write_text(r.stderr,encoding='utf-8',newline='\n')
        parsed=list(csv.DictReader(io.StringIO(r.stdout)))
        row.update(command=command,returncode=r.returncode,phase_rows=len(parsed),process_wall_seconds=time.perf_counter()-start,
                   status='completed' if r.returncode==0 and parsed and parsed[-1]['phase']=='library_destroy' and 'COMPLETE ' in r.stderr else 'failed')
    row['memory_after']=memory()
    with log.open('a',encoding='utf-8') as f:f.write(json.dumps(row)+'\n')
    print(label,row['status'],flush=True)
    if row['status']=='failed':
        raise RuntimeError(label)
