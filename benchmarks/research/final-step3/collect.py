"""Bounded fresh A/B/C acceptance; reuse the pinned Step 2 workload/oracles."""
import argparse,csv,gzip,importlib.util,io,json,platform,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('step2',Path(__file__).resolve().parent.parent/'history-step2/collect.py')
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
GIB=base.GIB

def plans(suite):
    if suite=='live':
        for n in (100000,1000000,3000000):
            for kind in ('append','prepend','anchor','local','ascending','descending','random','duplicates','equal'):
                reps=6 if (n==3000000 and kind in ('random','duplicates')) or (n==100000 and kind in ('append','local')) else 3
                for mode in ('production','allocation','structural'):
                    for repeat in range(1,(reps if mode=='production' else 1)+1):yield n,0,kind,0,mode,repeat
    elif suite=='history':
        for n in (100000,1000000,3000000):
            cases=base.cases(n)+([(2,'legacy',2)] if n<=1000000 else [])
            for keep,kind,ns in cases:
                reps=6 if (n==1000000 and kind in ('legacy','none','fixed256')) or (n==100000 and kind=='variable') else 3 if n<=1000000 else 2
                for mode in ('production','allocation','structural'):
                    for repeat in range(1,(reps if mode=='production' else 1)+1):yield n,keep,kind,ns,mode,repeat
    elif suite=='long':
        for n in (100000,1000000):
            for mode in ('production','allocation','structural'):
                for repeat in ((1,2) if mode=='production' else (1,)):yield n,0,'long',0,mode,repeat
    elif suite=='shrink':
        for repeat in (1,2):yield 1000000,0,'shrink',0,'structural',repeat
    else:
        for kind,keep,ns in [('append',0,0),('eight',2,16),('variable',2,4096)]:
            for mode in ('production','allocation','structural'):
                for repeat in ((1,2) if mode=='production' else (1,)):yield 10000000,keep,kind,ns,mode,repeat

def main():
    p=argparse.ArgumentParser()
    for name in ('a','b','c','output'):p.add_argument(name,type=Path)
    p.add_argument('--suites',default='live,history,long,shrink,upper');args=p.parse_args()
    for suite in args.suites.split(','):
        out=args.output/('capture-'+suite);out.mkdir(parents=True,exist_ok=True);log=out/'runs.jsonl'
        done={json.loads(x)['id'] for x in log.read_text().splitlines()} if log.exists() else set()
        for n,keep,kind,ns,mode,repeat in plans(suite):
            order='abc';offset=(repeat-1)%3;order=order[offset:]+order[:offset]
            for variant in order:
                label=f'{variant}-{mode}-{kind}-{n}-k{keep}-ns{ns}-r{repeat}'
                if label in done:continue
                memory=base.memory();envelope=base.envelope(n,kind if kind in ('eight','variable','legacy','long','shrink') else 'append',keep,mode)
                row=dict(id=label,variant=variant,mode=mode,kind=kind,residents=n,retention=keep,namespace=ns,repeat=repeat,suite=suite,memory_before=memory,estimated_envelope=envelope,reserve=3*GIB,order_protocol='ABC-repeat-rotation-v1',utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()))
                if envelope>min(3*GIB,.4*memory['available']) or memory['available']-envelope<3*GIB:
                    row.update(status='safely_skipped',reason='unchanged conservative simultaneous-memory admission')
                else:
                    suffix={'production':'','allocation':'_alloc','structural':'_structural'}[mode]
                    binary='full_scale'+suffix;arguments=[kind if kind!='legacy' else 'history',str(n),'17']
                    if suite=='history' and kind!='legacy' or suite=='upper' and keep:
                        binary='full_history'+suffix;arguments=[str(n),str(keep),kind,str(ns)]
                    if kind=='long':binary='full_long'+suffix;arguments=[str(n),'100']
                    if kind=='shrink':binary='full_shrink';arguments=[]
                    if sys.platform=='win32':binary+='.exe'
                    command=[str((getattr(args,variant)/'history-harness'/binary).resolve())]+arguments
                    row['command']=command;start=time.perf_counter()
                    try:
                        run=subprocess.run(command,capture_output=True,timeout=360)
                        stdout,stderr=run.stdout.decode('utf-8'),run.stderr.decode('utf-8')
                        (out/(label+'.json.gz')).write_bytes(gzip.compress(json.dumps(dict(id=label,stdout=stdout,stderr=stderr)).encode(),mtime=0))
                        parsed=list(csv.DictReader(io.StringIO(stdout)))
                        valid=run.returncode==0 and parsed and 'COMPLETE' in stderr and all(None not in r for r in parsed)
                        row.update(status='completed' if valid else 'budget_aborted' if run.returncode==77 else 'failed',returncode=run.returncode,phase_rows=len(parsed))
                    except subprocess.TimeoutExpired:
                        row.update(status='budget_aborted',reason='360-second timeout; only benchmark child terminated')
                    row['process_seconds']=time.perf_counter()-start
                row['memory_after']=base.memory()
                with log.open('a',encoding='utf-8') as f:f.write(json.dumps(row)+'\n')
                print(suite,label,row['status'],round(row.get('process_seconds',0),3),flush=True)
                if row['status']=='failed':raise RuntimeError('Inspect failed process '+label)
        (out/'environment.json').write_text(json.dumps(dict(platform=platform.platform(),memory=base.memory(),sources=dict(a='043872eb5560e867aa7f8c094e0e9dbf2e8a7b95',b='19ca51bc616a1b6a2d28530bc0932dc7c4a255ce',c='291305dca03d2d7599c88b6e6cebf2cc426e1790'),driver_source='291305dca03d2d7599c88b6e6cebf2cc426e1790',synthetic=True,flags='GCC C17 -O3 -DNDEBUG; strict warnings'),indent=2))
if __name__=='__main__':main()
