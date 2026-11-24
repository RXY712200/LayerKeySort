"""Held-out native scale trace using the existing Full model and QPC/RSS."""
import ctypes,gzip,hashlib,json,subprocess,sys
from pathlib import Path

base,wire,out=(Path(x).resolve() for x in sys.argv[1:4])
out.mkdir()
class Memory(ctypes.Structure):
    _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','page','pageavail','virtual','virtualavail','ext')]
with gzip.open(out/'scale-invocations.jsonl.gz','wt',encoding='utf-8') as stream:
    for rep in range(4):
        order=('base','wire') if rep%2==0 else ('wire','base')
        for label,n,steps,cadence in (('retained',10000,160,0),('churn',100000,2048,512),
                                      ('distant_drag',100000,4096,0),('hotspot_drag',100000,4096,0)):
            for v in order:
                for mode in ('timed','diagnostic') if rep==0 else ('timed',):
                    b=base if v=='base' else wire
                    exe=b/('layerkeysort_v4_trace'+('_diagnostics' if mode!='timed' else '')+'.exe')
                    argv=[str(exe),label,str(n),str(steps),str(cadence),'1779033703',str(rep)]
                    memory=Memory()
                    memory.length=ctypes.sizeof(memory)
                    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):raise OSError
                    record=dict(variant=v,argv=argv,available=memory.avail,estimate=n*250,
                                sha256=hashlib.sha256(exe.read_bytes()).hexdigest())
                    if memory.avail<512*1024**2+n*250:
                        stream.write(json.dumps(dict(record,status='skipped-memory'))+'\n')
                        continue
                    r=subprocess.run(argv,capture_output=True,text=True,timeout=120)
                    stream.write(json.dumps(dict(record,code=r.returncode,stdout=r.stdout,stderr=r.stderr))+'\n')
                    stream.flush()
                    r.check_returncode()
                    data=json.loads(r.stdout.splitlines()[-1])
                    assert data['oracle'] and not data['cleanup_live_bytes']
