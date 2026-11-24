"""Independent Windows seed/rate holdouts using exact candidate libraries."""
import ctypes,gzip,hashlib,json,subprocess,sys
from pathlib import Path
here=Path(__file__).resolve().parent
build,out=(Path(x).resolve() for x in sys.argv[1:3])
out.mkdir()
diag_only='--diagnostics-only' in sys.argv[3:]
archive='f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/baseline/'
for name in ('baseline.c','baseline.h'):
    (out/name).write_bytes(subprocess.check_output(['git','show',archive+name]))
class Memory(ctypes.Structure):
    _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','a','b','c','d','e')]
with gzip.open(out/'holdouts.jsonl.gz','wt',encoding='utf-8') as f:
    def run(tag,cmd):
        m=Memory();m.length=ctypes.sizeof(m)
        assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
        argv=list(map(str,cmd))
        row=dict(tag=tag,argv=argv,available=m.avail)
        if m.avail<640*1024**2:
            f.write(json.dumps(dict(row,status='skipped-memory'))+'\n')
            return False
        p=subprocess.run(argv,capture_output=True,text=True,timeout=120)
        if Path(argv[0]).is_file():row['binary_sha256']=hashlib.sha256(Path(argv[0]).read_bytes()).hexdigest()
        f.write(json.dumps(dict(row,code=p.returncode,stdout=p.stdout,stderr=p.stderr))+'\n')
        f.flush()
        p.check_returncode()
        return True
    for seed,rate in ((17,4),(1779033703,16)):
        src=out/f'life-{seed}.c'
        if not run('generate',[sys.executable,here/'generate_lifecycle.py',src,seed,rate]):continue
        b={}
        for mode in ('diag',) if diag_only else ('prod','diag'):
            for name in ('final','tree'):
                exe=out/f'{seed}-{name}-{mode}.exe'
                flags=['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-Iinclude','-Isrc','-I'+str(out),'-DMIXED=1',f'-DFLAT={int(name=="tree")}',f'-DINDEXED={int(name=="tree")}']
                if mode=='diag':flags+=['-DLKS_ENABLE_ALLOC_DIAGNOSTICS','-DDIAGNOSTIC']
                extra=[out/'baseline.c','-DTREE=1'] if name=='tree' else []
                if run('build',[*flags,src,*extra,build/('liblayerkeysort_diagnostics.a' if mode=='diag' else 'liblayerkeysort.a'),'-o',exe]):
                    b[name,mode]=exe
        if len(b)!=(2 if diag_only else 4):continue
        for n in (1000,10000,100000):
            for rep in (8,) if diag_only else range(9):
                for v in ('final','tree') if rep%2==0 else ('tree','final'):
                    run(f'{seed}-{rate}-{n}-{rep}-{v}',[b[v,'diag' if rep==8 else 'prod'],n,2,32,8])
