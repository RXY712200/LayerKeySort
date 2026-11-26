import json,subprocess,sys,hashlib
from pathlib import Path
out=Path(sys.argv[1])
stable=Path(sys.argv[2])
candidate=Path(sys.argv[3])
def capture():
    for mode in ('trace','trace_diagnostics'):
        bins=[d/('layerkeysort_v4_'+mode+'.exe') for d in (stable,candidate)]
        for family in ('endpoint','local','hotspot','large_moves','churn','managed_churn','random'):
            for rep in range(4):
                for index in (range(2) if rep%2==0 else (1,0)):
                    exe=bins[index]
                    cmd=[str(exe),family,'10000','5000','0','1779033703',str(rep)]
                    r=subprocess.run(cmd,capture_output=True,timeout=120)
                    yield dict(variant=['stable41','candidate'][index],mode=mode,command=cmd,
                               sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),code=r.returncode,
                               stdout=r.stdout.decode(),stderr=r.stderr.decode())
with out.open('x',encoding='utf-8') as f:
    for row in capture():
        f.write(json.dumps(row)+'\n')
        f.flush()
        assert row['code']==0,row
