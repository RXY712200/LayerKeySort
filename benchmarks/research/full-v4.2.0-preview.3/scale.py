"""Repeat exact sparse survival and concentrated label probes on native Windows."""
import ctypes,gzip,hashlib,json,subprocess,sys
from pathlib import Path
here=Path(__file__).resolve().parent
build,out=(Path(x).resolve() for x in sys.argv[1:3])
out.mkdir()
class Memory(ctypes.Structure):
    _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','a','b','c','d','e')]
with gzip.open(out/'scale.jsonl.gz','wt',encoding='utf-8') as f:
    def run(tag,argv,estimate=80*1024**2):
        m=Memory();m.length=ctypes.sizeof(m)
        assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
        args=list(map(str,argv));row=dict(tag=tag,argv=args,available=m.avail,estimate=estimate)
        if m.avail<estimate+512*1024**2:
            f.write(json.dumps(dict(row,status='skipped-memory'))+'\n')
            return
        p=subprocess.run(args,capture_output=True,text=True,timeout=120)
        f.write(json.dumps(dict(row,code=p.returncode,stdout=p.stdout,stderr=p.stderr))+'\n')
        f.flush()
        p.check_returncode()
    deps=['gcc','-std=c17','-O3','-Wall','-Wextra','-Wpedantic','-Werror','-DLKS_ENABLE_ALLOC_DIAGNOSTICS','-Iinclude','-Isrc']
    lib=build/'liblayerkeysort_diagnostics.a'
    for source,name in ((here.parent/'full-v4.2.0-preview.2/sparse.c','sparse'),(here.parent/'full-v4.2.0-preview.1.2.1/structure.c','structure')):
        exe=out/(name+'.exe')
        run('build-'+name,[*deps,source,lib,'-o',exe])
        if not exe.exists():continue
        if name=='sparse':
            for n in (10000,100000,1000000):
                for scattered in (0,1):run(f'{name}-{n}-{scattered}',[exe,n,scattered],n*160)
        else:run(name,[exe])
    (out/'binaries.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest()
        for p in out.glob('*.exe')},indent=2)+'\n',encoding='utf-8')
