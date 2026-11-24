"""Fresh same-host lifecycle screen and guarded sparse control."""
import csv,gzip,hashlib,io,json,os,platform,subprocess,sys,tarfile
from pathlib import Path

HERE=Path(__file__).resolve().parent
OUT=Path(sys.argv[1]).resolve()
OUT.mkdir()
EXE='.exe' if os.name=='nt' else ''
PINS={'stable41':'70d7f0ef5857b54381b863d292a213909d87459f',
      'base':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae',
      'wire':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae'}
meta=dict(platform=platform.platform(),compiler=subprocess.check_output(['gcc','--version'],text=True),sources=PINS,executables={})
def free_memory():
    if os.name!='nt':
        return next(int(x.split()[1])*1024 for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:'))
    import ctypes
    class Memory(ctypes.Structure):
        _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','page','pageavail','virtual','virtualavail','ext')]
    data=Memory()
    data.length=ctypes.sizeof(data)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(data)):raise OSError('memory probe failed')
    return data.avail

with gzip.open(OUT/'invocations.jsonl.gz','wt',encoding='utf-8') as receipts:
    def run(tag,cmd,estimate=128*1024**2):
        argv=list(map(str,cmd))
        free=free_memory()
        record=dict(tag=tag,argv=argv,available=free,estimated_bytes=estimate)
        try:
            if free < estimate+512*1024**2:
                record['status']='skipped-memory'
                return None
            p=subprocess.run(argv,timeout=120,capture_output=True,text=True,encoding='utf-8',errors='replace')
            record.update(code=p.returncode,stdout=p.stdout,stderr=p.stderr)
            p.check_returncode()
            return p.stdout
        except subprocess.TimeoutExpired as e:
            record.update(status='timeout',stdout=str(e.stdout),stderr=str(e.stderr))
            raise
        finally:
            receipts.write(json.dumps(record)+'\n')
            receipts.flush()
    def link(source,b,variant,mode):
        lib=b/('liblayerkeysort_diagnostics.a' if mode=='diag' else 'liblayerkeysort.a')
        target=b/(variant+'-'+mode+EXE)
        flags=['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(source/'include'),'-I'+str(source/'src')]
        if mode=='diag': flags+=['-DLKS_ENABLE_ALLOC_DIAGNOSTICS']
        if variant=='flat': flags+=['-DFLAT=1']
        test=HERE/('sparse.c' if variant=='sparse' else 'lifecycle.c')
        if run(variant+'-'+mode+'-link',[*flags,test,lib,'-o',target]) is None:raise RuntimeError('build not admitted')
        meta['executables'][variant+'-'+mode]=hashlib.sha256(target.read_bytes()).hexdigest()
        return target
    exes={}
    for kind,pin in PINS.items():
        src=OUT/kind
        src.mkdir()
        raw=subprocess.check_output(['git','archive',pin])
        with tarfile.open(fileobj=io.BytesIO(raw)) as z:z.extractall(src,filter='data')
        if kind=='wire':
            p=src/'src/snapshot_wire.c'
            s=p.read_text()
            marker='size_t lks_snapshot_serialized_size('
            a=s.index('static uint64_t snap_be_read')
            b=s.index(marker)
            p.write_text(s[:a]+(HERE/'wire.inc').read_text()+s[b:])
        b=src/'build'
        if run(kind+'-configure',['cmake','-S',src,'-B',b,'-G','Ninja','-DCMAKE_C_COMPILER=gcc',
             '-DCMAKE_BUILD_TYPE=Release','-DLKS_BUILD_TESTS=ON','-DLKS_BUILD_BENCHMARKS=ON',
             '-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror']) is None:raise RuntimeError('build not admitted')
        if run(kind+'-build',['cmake','--build',b,'--parallel','3','--target','layerkeysort','layerkeysort_diagnostics']) is None:raise RuntimeError('build not admitted')
        for mode in ('prod','diag'):
            exes[kind,mode]=link(src,b,kind,mode)
            if kind=='base':exes['flat',mode]=link(src,b,'flat',mode)
        if kind=='base':exes['sparse','diag']=link(src,b,'sparse','diag')
    (OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
    cells=[(1000,2,8,8),(10000,2,8,8),(100000,2,8,8),(10000,0,8,8),(10000,1,8,8),
           (10000,8,8,8),(10000,16,8,8),(10000,2,0,8),(10000,2,32,8),(10000,2,256,8),
           (10000,2,4096,8),(10000,2,8,4096),(100000,8,32,8)]
    with (OUT/'raw.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n')
        w.writerow(['variant','repeat','n','retention','association','namespace','build','edit','capture','key',
                    'encode','decode','restore','cleanup','wire_bytes','requested_retained','requested_peak','allocation_calls'])
        for i,cell in enumerate(cells):
            for rep in range(9):
                order=['base','stable41','wire','flat']
                j=(i+rep)%len(order)
                mode='diag' if rep==8 else 'prod'
                for kind in order[j:]+order[:j]:
                    n,r,association,d=cell
                    estimate=n*(256+r*(32+(association if association!=4096 else 4096)))*2
                    data=run(f'life-{i}-{rep}-{kind}',[exes[kind,mode],*cell],estimate)
                    if data is not None:
                        vals=data.strip().split(',')
                        assert len(vals)==16 and tuple(map(int,vals[:4]))==cell
                        w.writerow([kind,rep,*vals])
                        f.flush()
    for n in (10000,100000,1000000):
        for scattered in (0,1):
            run(f'sparse-{n}-{scattered}',[exes['sparse','diag'],n,scattered],n*160)
