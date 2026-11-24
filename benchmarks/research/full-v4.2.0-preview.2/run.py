"""Fresh same-host lifecycle screen and guarded sparse control."""
import csv,gzip,hashlib,io,json,os,platform,subprocess,sys,tarfile,zipfile
from pathlib import Path

HERE=Path(__file__).resolve().parent
OUT=Path(sys.argv[1]).resolve()
OUT.mkdir()
EXE='.exe' if os.name=='nt' else ''
PINS={'stable41':'70d7f0ef5857b54381b863d292a213909d87459f',
      'base':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae',
      'wire':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae'}
FINAL = sys.argv[2] if len(sys.argv)==3 else None
if FINAL:
    PINS['wire'] = FINAL
OLD = 'f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/'
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
        if kind=='wire' and FINAL is None:
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
        if kind=='base' or (kind=='wire' and FINAL):
            exes['sparse-'+kind,'diag']=link(src,b,'sparse','diag')
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
            for kind in ('base','wire') if FINAL else ('base',):
                run(f'sparse-{n}-{scattered}-{kind}',[exes['sparse-'+kind,'diag'],n,scattered],n*160)
    if FINAL:
        def read(name):
            return subprocess.check_output(['git','show',OLD+name])
        harness=OUT/'harness'
        harness.mkdir()
        for name in ('harness.c','baseline.c','baseline.h'):
            (harness/name).write_bytes(read('baseline/'+name))
        if os.name!='nt':
            path=harness/'harness.c'
            data=path.read_text()
            assert data.count('timespec_get(&t,TIME_UTC);')==1
            path.write_text('#define _POSIX_C_SOURCE 200809L\n'+data.replace(
                'timespec_get(&t,TIME_UTC);','if(clock_gettime(CLOCK_MONOTONIC,&t)) abort();'))
        suite=json.loads(read('trace-manifest.json'))
        traces=OUT/'traces'
        traces.mkdir()
        with zipfile.ZipFile(io.BytesIO(read('traces.zip'))) as z:
            assert sorted(z.namelist())==sorted(x['name'] for x in suite)
            for x in suite:
                name=x['name']
                content=z.read(name)
                assert Path(name).name==name and hashlib.sha256(content).hexdigest()==x['sha256']
                (traces/name).write_bytes(content)
        gate=HERE.parent/'full-v4.2.0-preview.1.2/replay/semantic-gate.trace'
        variants=('base','stable41','wire')
        for kind in variants:
            src=OUT/kind
            b=src/'build'
            for mode in ('prod','diag'):
                diag=mode=='diag'
                lib=b/('liblayerkeysort_diagnostics.a' if diag else 'liblayerkeysort.a')
                target=b/(kind+'-live-'+mode+EXE)
                flags=['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror',
                       '-DFULL=1','-DTREE=0','-I'+str(src/'include'),'-I'+str(src/'src')]
                if diag:flags+=['-DDIAGNOSTIC','-DLKS_ENABLE_ALLOC_DIAGNOSTICS']
                libs=[lib]+(['-lpsapi'] if os.name=='nt' else [])
                if run(f'{kind}-live-link-{mode}',[*flags,harness/'harness.c',harness/'baseline.c',
                       *libs,'-o',target]) is None:raise RuntimeError('live build not admitted')
                exes[kind+'-live',mode]=target
                meta['executables'][target.name]=hashlib.sha256(target.read_bytes()).hexdigest()
            assert run(kind+'-live-gate',[exes[kind+'-live','diag'],gate,1])
        with (OUT/'live.csv').open('w',newline='') as f:
            w=csv.writer(f,lineterminator='\n')
            w.writerow(['trace','sha256','variant','repeat','seconds','requested_live','requested_peak',
                       'alloc_calls','free_calls_before_destroy','requested_after_destroy','working_set',
                       'peak_working_set','records_reassigned','max_reassigned','splits','merges',
                       'redistributions','cross_block_moves','total_frees'])
            for i,x in enumerate(suite):
                for rep in range(9):
                    j=(i+rep)%len(variants)
                    mode='diag' if rep==8 else 'prod'
                    for kind in variants[j:]+variants[:j]:
                        data=run(f'live-{i}-{rep}-{kind}',[exes[kind+'-live',mode],traces/x['name'],int(mode=='diag')])
                        if data:
                            fields=data.strip().split(',')
                            assert len(fields)==15 and int(fields[5])==0
                            w.writerow([x['name'],x['sha256'],kind,rep,*fields])
                            f.flush()
        (OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
