"""100-cycle turnover campaign using the same external physical-memory gate."""
import csv
import io
import json
import subprocess
import sys
import time
from pathlib import Path
from collect import memory, estimate, GIB

build, output = map(Path, sys.argv[1:3]); output.mkdir(exist_ok=True)
records = []
for n in (100000, 1000000):
    for mode, repeats in (('production', 2), ('allocation', 1), ('structural', 1)):
        for repeat in range(1, repeats+1):
            m = memory(); planned = estimate(n, mode, False)
            row = dict(mode=mode, scale=n, repeat=repeat, memory_before=m, estimated_envelope_bytes=planned)
            name = f'{mode}-{n}-{repeat}'
            assert planned < min(3*GIB, m['available']*.40) and m['available']-planned>3*GIB
            exe = {'production': 'full_long.exe', 'allocation': 'full_long_alloc.exe', 'structural': 'full_long_structural.exe'}[mode]
            start = time.perf_counter()
            result = subprocess.run([str(build.resolve()/exe),str(n),'100'],capture_output=True,text=True,timeout=270)
            (output/f'{name}.csv').write_text(result.stdout,encoding='utf-8',newline='\n')
            (output/f'{name}.stderr.txt').write_text(result.stderr,encoding='utf-8',newline='\n')
            rows = list(csv.DictReader(io.StringIO(result.stdout)))
            row.update(returncode=result.returncode, wall_seconds=time.perf_counter()-start,
                       status='completed' if result.returncode==0 and len(rows)==100 and 'LONG_COMPLETE' in result.stderr else 'failed',
                       memory_after=memory())
            records.append(row);(output/'runs.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
            print(name,row['status'],f"wall={row['wall_seconds']:.2f}s",flush=True)
            assert row['status']=='completed'
