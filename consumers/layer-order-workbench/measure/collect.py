"""Standard-library evidence collection; each repeat is a fresh process."""
import argparse,csv,hashlib,io,json,os,platform,statistics,subprocess,time
from pathlib import Path

CASES=['sequential-insert','head-tail-insert','adjacent-move','distant-move',
       'valid-noop','remove-reinsert','forward-traverse','reverse-traverse',
       'self-compare','near-compare','far-compare','mixed-editor']
SIZES=[16,128,2048]
METRICS=['init_ns','api_ns','map_ns','oracle_ns','parse_trace_output_ns','execute_ns','destroy_ns','wall_ns']
ROOT=Path(__file__).resolve().parents[1]

def run(args,**kwargs):
    return subprocess.run([str(x) for x in args],check=True,**kwargs)

def git(*args):
    return run(['git','-C',ROOT,*args],capture_output=True,text=True).stdout.strip()

def canonical_digest(data):
    # Git checkouts can use CRLF on Windows and LF on Linux for identical source.
    return hashlib.sha256(data.replace(b'\r\n',b'\n')).hexdigest()

def quantiles(values):
    med=statistics.median(values)
    q=statistics.quantiles(values,n=4,method='inclusive')
    return {'minimum':min(values),'median':med,'maximum':max(values),
            'mad':statistics.median(abs(v-med) for v in values),'iqr':q[2]-q[0]}

def summarize(rows,out):
    groups={}
    for row in rows:
        if row['phase']=='measured': groups.setdefault((row['case'],row['size'],row['mode']),[]).append(row)
    data=[]
    for (case,size,mode),items in groups.items():
        for metric in METRICS:
            values=[int(x[metric]) for x in items if x.get(metric) not in [None,'','NA']]
            if values: data.append({'case':case,'size':size,'mode':mode,'metric':metric,'count':len(values),**quantiles(values)})
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=['case','size','mode','metric','count','minimum','median','maximum','mad','iqr'])
        writer.writeheader();writer.writerows(data)
    return data

def collect(args):
    if args.repeats<15: raise ValueError('At least 15 independent repeats are required')
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    (out/'workloads').mkdir();(out/'traces').mkdir()
    measure=args.measure.resolve();memory=args.memory.resolve();workbench=args.workbench.resolve()
    run([measure,'--verify-all'],stdout=(out/'correctness.log').open('w',encoding='utf-8'),stderr=subprocess.STDOUT)
    files={};actual_files={}
    for base in [ROOT/'src',ROOT/'measure']:
        for f in sorted(base.iterdir()):
            if f.suffix in ['.c','.h','.py']:
                key=f.relative_to(ROOT).as_posix()
                files[key]=canonical_digest(f.read_bytes());actual_files[key]=hashlib.sha256(f.read_bytes()).hexdigest()
    files['CMakeLists.txt']=canonical_digest((ROOT/'CMakeLists.txt').read_bytes())
    mini=args.mini.resolve()
    mini_files={f.relative_to(mini).as_posix():canonical_digest(f.read_bytes())
                for base in [mini/'include',mini/'src'] for f in sorted(base.iterdir()) if f.is_file()}
    expected_mini={'include/layerkeysort_mini.h':'afb6cd39e6dc5cc787e596869bb9d71120aa32637933197f1cbcc871baac5c2f',
                   'src/lks_mini.c':'206b8a590b8f011c1f7d9d206169f03e7e24ad63db106f5247ba2846eec9eb33'}
    if mini_files!=expected_mini:raise ValueError('Mini public/production files differ from pinned v1.0.0 release')
    cache=(args.build.resolve()/'CMakeCache.txt').read_text(encoding='utf-8')
    flags=[line for line in cache.splitlines() if line.startswith(('CMAKE_C_FLAGS','CMAKE_C_COMPILER:','CMAKE_BUILD_TYPE:','WORKBENCH_SANITIZERS:'))]
    cpu=platform.processor()
    if Path('/proc/cpuinfo').exists():
        cpu=next((s.split(':',1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines() if s.startswith('model name')),cpu)
    meta={'schema':'LLOW-measurement-1','utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
          'workbench_measurement_commit':git('rev-parse','HEAD'),'git_dirty':bool(git('status','--porcelain')),
          'mini_version':'1.0.0','mini_release_sha':'3f52798b83b6580aa4b89846b6403b0bd7b15f0a',
          'mini_public_production_sha256':mini_files,'measurement_source_sha256':files,
          'os':platform.platform(),'architecture':platform.machine(),'cpu':cpu,'logical_cpus':os.cpu_count(),
          'python':platform.python_version(),'cmake_cache':flags,'repeats':args.repeats,
          'provenance':'generated engineering experiments; no human sessions',
          'warmup':'One fresh order run before each measured run in the same fresh process; CLI warmup uses a separate process',
          'end_to_end':'Unmodified Phase 1 executable subprocess wall time, including startup/setup/quit/destruction, local-file stdout and trace flushes',
          'comparison_distribution':'near: adjacent positions i%(n-1), alternating direction; far: endpoints alternating direction; self: identical ID',
          'affinity':'not pinned; frequency/governor and concurrent host load uncontrolled'}
    meta['measurement_source_dirty']=bool(git('status','--porcelain','--',str(ROOT/'src'),str(ROOT/'measure'),str(ROOT/'CMakeLists.txt')))
    meta['strict_target_flags']='C17; GCC/Clang -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror; MSVC /W4 /WX'
    meta['source_digest_encoding']='Canonical LF for cross-checkout identity; actual checkout digests separately retained'
    meta['measurement_actual_checkout_sha256']=actual_files
    meta['mini_actual_checkout_sha256']={f.relative_to(mini).as_posix():hashlib.sha256(f.read_bytes()).hexdigest()
                                       for base in [mini/'include',mini/'src'] for f in sorted(base.iterdir()) if f.is_file()}
    (out/'metadata.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
    rows=[];memory_rows=[]
    for size in SIZES:
        for case in CASES:
            commands=run([measure,case,size,'emit'],capture_output=True).stdout
            script=out/'workloads'/f'{case}-{size}.commands';script.write_bytes(commands)
            for repeat in range(1,args.repeats+1):
                for mode in ['isolated','pipeline']:
                    result=run([measure,case,size,mode],capture_output=True,text=True)
                    batch=list(csv.DictReader(io.StringIO(result.stdout)))
                    if len(batch)!=2 or {r['phase'] for r in batch}!={'warmup','measured'}:raise ValueError('Incomplete timing rows')
                    for row in batch:
                        if row['sanitized']!='0':raise ValueError('Sanitizer timing cannot enter ordinary evidence')
                        row['repeat']=repeat;row['wall_ns']='NA';rows.append(row)
                for phase in ['warmup','measured']:
                    trace=out/'traces'/f'{case}-{size}-{repeat}-{phase}.trace'
                    log=out/'traces'/f'{case}-{size}-{repeat}-{phase}.stdout'
                    with log.open('wb') as f:
                        start=time.perf_counter_ns()
                        result=run([workbench,'--commands',script,'--source','generated','--trace',trace],stdout=f,stderr=subprocess.PIPE)
                        elapsed=time.perf_counter_ns()-start
                    if result.stderr:raise ValueError(result.stderr.decode(errors='replace'))
                    expected=len(commands.splitlines())
                    if trace.read_bytes().splitlines()[-1]!=f'LLOW-END\t{expected}'.encode():raise ValueError('Incomplete CLI run')
                    if repeat==1 and phase=='measured':
                        run([workbench,'--replay',trace],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
                    prototype=rows[-1]
                    row={k:'NA' for k in prototype}
                    row.update(case=case,size=str(size),mode='phase1-end-to-end',phase=phase,repeat=repeat,
                               commands=expected,wall_ns=elapsed,compiler=prototype['compiler'],build=prototype['build'],sanitized='0')
                    rows.append(row)
                    # Keep one full semantic trace/output per group; raw timing rows are all retained.
                    if repeat!=1:trace.unlink();log.unlink()
            print(f'Collected {case} n={size}: {args.repeats} independent repeats',flush=True)
        for repeat in range(1,args.repeats+1):
            result=run([memory,size],capture_output=True,text=True)
            batch=list(csv.DictReader(io.StringIO(result.stdout)))
            if len(batch)!=9:raise ValueError('Incomplete memory lifecycle')
            for row in batch:row['repeat']=repeat;memory_rows.append(row)
    for filename,data in [('timing-raw.csv',rows),('memory-raw.csv',memory_rows)]:
        with (out/filename).open('w',newline='',encoding='utf-8') as f:
            writer=csv.DictWriter(f,fieldnames=list(data[0]));writer.writeheader();writer.writerows(data)
    stats=summarize(rows,out)
    report=['# Generated engineering measurement summary','',
            'All times are nanoseconds. 15+ independent process repeats; warmup excluded from summaries.',
            'No human workload or Full comparison. Source/host/flags in metadata.json; all samples in timing-raw.csv.',
            '', '| Case | n | Isolated API median ns | min | max | MAD | Pipeline execute median ns | Phase 1 wall median ns |',
            '|---|---:|---:|---:|---:|---:|---:|---:|']
    def find(case,n,mode,metric):return next(x for x in stats if (x['case'],x['size'],x['mode'],x['metric'])==(case,str(n),mode,metric))
    for n in SIZES:
        for case in CASES:
            a=find(case,n,'isolated','api_ns');p=find(case,n,'pipeline','execute_ns');e=find(case,n,'phase1-end-to-end','wall_ns')
            report.append(f"| {case} | {n} | {a['median']:g} | {a['minimum']} | {a['maximum']} | {a['mad']:g} | {p['median']:g} | {e['median']:g} |")
    report+=['','Initialization, mapping, oracle, parse/trace/output, execution and destruction distributions are separately available in summary.csv.',
             'API intervals include dispatch/status bookkeeping and timer quantization. Empty-pair baseline is reported, never subtracted.',
             'Pipeline oracle includes additional Mini read/comparison calls. Components are coupled by caches and instrumentation; they are not independent causal contributions.',
             'RSS is whole-process memory. Requested/usable allocation bytes do not measure allocator metadata or fragmentation. Released tracked blocks and retained RSS are different facts.']
    (out/'SUMMARY.md').write_text('\n'.join(report)+'\n',encoding='utf-8')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--measure',type=Path,required=True);p.add_argument('--memory',type=Path,required=True)
    p.add_argument('--workbench',type=Path,required=True);p.add_argument('--mini',type=Path,required=True)
    p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--repeats',type=int,default=15);collect(p.parse_args())
