param(
    [Parameter(Mandatory=$true)][string]$OriginalCapture,
    [Parameter(Mandatory=$true)][string]$BuildA,
    [Parameter(Mandatory=$true)][string]$BuildB,
    [Parameter(Mandatory=$true)][string]$BuildC,
    [Parameter(Mandatory=$true)][string]$Output
)
# One bounded follow-up after the owner frees RAM. Same guard, binary and oracle;
# never overwrite an earlier skip or repeat an already completed process.
@'
import csv,gzip,io,json,subprocess,sys,time
from pathlib import Path
sys.path.insert(0,sys.argv[1])
import collect
original=Path(sys.argv[2]);builds=dict(zip('abc',map(Path,sys.argv[3:6])));output=Path(sys.argv[6]);output.mkdir(parents=True,exist_ok=True)
planned=[]
for line in (original/'runs.jsonl').read_text().splitlines():
    old=json.loads(line)
    if old['status']=='safely_skipped':
        row=dict(old)
        if 'reason' in row:row['original_skip_reason']=row.pop('reason')
        row.update(id=old['id']+'-retry1',retry_of=old['id']);planned.append(row)
for mode in ('production','allocation','structural'):
    for repeat in ((1,2) if mode=='production' else (1,)):
        for variant in (('b','c','a') if repeat%2 else ('a','c','b')):
            planned.append(dict(id=f'{variant}-{mode}-eight-10000000-k2-ns16-r{repeat}',variant=variant,mode=mode,kind='eight',residents=10000000,retention=2,namespace=16,repeat=repeat,suite='upper'))
for variant in 'bc':
    planned.append(dict(id=f'{variant}-production-variable-10000000-k2-ns4096-r1',variant=variant,mode='production',kind='variable',residents=10000000,retention=2,namespace=4096,repeat=1,suite='upper'))
log=output/'runs.jsonl';done={json.loads(x)['id'] for x in log.read_text().splitlines()} if log.exists() else set()
for old in planned:
    if old['id'] in done:continue
    row=dict(old);m=collect.memory();n=row['residents'];keep=row['retention'];kind=row['kind'];mode=row['mode'];estimated=collect.envelope(n,kind,keep,mode)
    row.update(memory_before=m,estimated_envelope=estimated,reserve=3*collect.GIB,utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()))
    if estimated>min(3*collect.GIB,.4*m['available']) or m['available']-estimated<3*collect.GIB:
        row.update(status='safely_skipped',reason='unchanged conservative admission')
    else:
        suffix=dict(production='',allocation='_alloc',structural='_structural')[mode]
        command=[str((builds[row['variant']]/('full_history'+suffix+'.exe')).resolve()),str(n),str(keep),kind,str(row['namespace'])]
        row['command']=command;start=time.perf_counter();run=subprocess.run(command,capture_output=True,timeout=360)
        raw=dict(id=row['id'],stdout=run.stdout.decode('utf-8'),stderr=run.stderr.decode('utf-8'))
        (output/(row['id']+'.json.gz')).write_bytes(gzip.compress(json.dumps(raw).encode(),mtime=0))
        parsed=list(csv.DictReader(io.StringIO(raw['stdout'])))
        valid=run.returncode==0 and parsed and 'HISTORY_COMPLETE' in raw['stderr'] and all(None not in r for r in parsed)
        row.update(status='completed' if valid else 'budget_aborted' if run.returncode==77 else 'failed',returncode=run.returncode,phase_rows=len(parsed),process_seconds=time.perf_counter()-start)
    row['memory_after']=collect.memory()
    with log.open('a',encoding='utf-8') as f:f.write(json.dumps(row)+'\n')
    print(row['id'],row['status'],round(row.get('process_seconds',0),3),flush=True)
    if row['status'] in ('failed','budget_aborted'):raise RuntimeError(row['id'])
(output/'environment.json').write_text(json.dumps(dict(memory=collect.memory(),note='One resource follow-up; source/binaries and guards unchanged; original skip records preserved.',source='7ac34f54dd7c7c544605efd0f8d97dbc4f02cd12'),indent=2))
'@ | python -B -X utf8 - $PSScriptRoot $OriginalCapture $BuildA $BuildB $BuildC $Output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
