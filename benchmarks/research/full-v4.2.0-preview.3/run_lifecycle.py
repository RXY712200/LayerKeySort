"""Stage exact source, capture endpoint and relative lifecycle receipts."""
import csv,gzip,hashlib,io,json,os,platform,subprocess,sys,tarfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
out=Path(sys.argv[1]).resolve()
out.mkdir()
pins={'final':'fe00536e7bd7f4e5128432ea4a29e54158074903','preview12':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae','stable41':'70d7f0ef5857b54381b863d292a213909d87459f','stable40':'80004ae732daf696f7e9192e417de0edb0b2aafa'}
arc='f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/baseline/'
def available():
    if os.name!='nt':
        return next(int(s.split()[1])*1024 for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith('MemAvailable:'))
    import ctypes
    class Mem(ctypes.Structure):
        _fields_=[('len',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','a','b','c','d','e')]
    m=Mem()
    m.len=ctypes.sizeof(m)
    assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return m.avail
with gzip.open(out/'receipts.jsonl.gz','wt',encoding='utf-8') as f:
    def run(tag,args,estimate=128*1024**2):
        args=list(map(str,args))
        mem=available()
        row=dict(tag=tag,args=args,available=mem,estimate=estimate)
        try:
            if mem<estimate+512*1024**2:
                row['status']='skipped-memory'
                return None
            r=subprocess.run(args,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=120)
            row.update(code=r.returncode,stdout=r.stdout,stderr=r.stderr)
            r.check_returncode()
            return r.stdout
        except subprocess.TimeoutExpired:
            row['status']='timeout'
            raise
        finally:
            f.write(json.dumps(row)+'\n')
            f.flush()
    for name in ('baseline.h','baseline.c','third_party/PCG-LICENSE.txt'):
        dest=out/name
        dest.parent.mkdir(exist_ok=True)
        dest.write_bytes(subprocess.check_output(['git','show',arc+name]))
    src=out/'lifecycle.c'
    run('generate',[sys.executable,HERE/'generate_lifecycle.py',src])
    exes={}
    for v,pin in pins.items():
        root=out/v
        root.mkdir()
        z=subprocess.check_output(['git','archive',pin])
        with tarfile.open(fileobj=io.BytesIO(z)) as t:t.extractall(root,filter='data')
        b=root/'build'
        run('configure-'+v,['cmake','-S',root,'-B',b,'-G','Ninja','-DCMAKE_C_COMPILER=gcc','-DCMAKE_BUILD_TYPE=Release','-DLKS_BUILD_BENCHMARKS=ON','-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror'])
        run('build-'+v,['cmake','--build',b,'--parallel','3','--target','layerkeysort','layerkeysort_diagnostics'])
        for mix in (0,1):
            for name in (['final','flat'] if v=='final' and not mix else ['final','tree'] if v=='final' else [v]):
                for diag in (0,1):
                    exe=out/(f'{name}-{mix}-{diag}'+('.exe' if os.name=='nt' else ''))
                    flags=['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(out),f'-DMIXED={mix}',f'-DFLAT={int(name in ("flat","tree"))}',f'-DINDEXED={int(name=="tree")}']
                    extra=[out/'baseline.c','-DTREE=1'] if name=='tree' else []
                    if diag:flags+=['-DLKS_ENABLE_ALLOC_DIAGNOSTICS']
                    run(exe.name+'-link',[*flags,src,*extra,b/('liblayerkeysort_diagnostics.a' if diag else 'liblayerkeysort.a'),'-o',exe])
                    exes[name,mix,diag]=exe
    cells=[(1000,2,8,8),(10000,2,8,8),(100000,2,8,8),(10000,0,8,8),(10000,1,8,8),
           (10000,8,8,8),(10000,16,8,8),(10000,2,0,8),(10000,2,32,8),(10000,2,256,8),
           (10000,2,4096,8),(10000,2,8,4096),(100000,8,32,8)]
    meta=dict(pins=pins,platform=platform.platform(),compiler=subprocess.check_output(['gcc','--version'],text=True),
              binaries={x.name:hashlib.sha256(x.read_bytes()).hexdigest() for x in exes.values()})
    (out/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    with (out/'raw.csv').open('w',newline='') as stream:
        w=csv.writer(stream,lineterminator='\n')
        w.writerow(['variant','mix','repeat','n','retention','association','namespace','build','edit','capture','key','encode','decode','restore','cleanup','wire_bytes','requested_retained','requested_peak','allocation_calls'])
        for mix,conditions in enumerate((cells,[cells[i] for i in (0,1,2,5,12)])):
            variants=list(pins)+(['tree'] if mix else ['flat'])
            for i,cell in enumerate(conditions):
                for rep in range(9):
                    shift=(rep+i)%len(variants)
                    for v in variants[shift:]+variants[:shift]:
                        data=run(f'life-{mix}-{i}-{rep}-{v}',[exes[v,mix,int(rep==8)],*cell],cell[0]*(256+cell[1]*(32+cell[2]))*2)
                        if data is not None:
                            w.writerow([v,mix,rep,*data.strip().split(',')])
                            stream.flush()
