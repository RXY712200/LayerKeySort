"""Summarize replay receipts and verify independent per-process outputs."""
import csv,gzip,hashlib,json,statistics,sys
from collections import defaultdict
from pathlib import Path

root=Path(sys.argv[1])
meta=json.loads((root/'metadata.json').read_text())
with gzip.open(root/'invocations.jsonl.gz','rt',encoding='utf-8') as stream:
    items=[json.loads(line) for line in stream]
# Early final captures used the same build-receipt label for two isolated
# sparse executables. The absolute argv and all measurement tags are distinct.
measurements=[v for v in items if '-link' not in v['tag']]
assert len({v['tag'] for v in measurements})==len(measurements)
assert len({tuple(v['argv']) for v in items if v['tag'].endswith('-link')})==sum(
    v['tag'].endswith('-link') for v in items)
skips=[v for v in items if v.get('status')=='skipped-memory']
assert all(v.get('code')==0 or v in skips for v in items)
assert all(v['available']>=512*1024**2+v['estimated_bytes'] or v in skips for v in items)
record={v['tag']:v for v in items}
phases=('build','edit','capture','key','encode','decode','restore','cleanup')
samples=defaultdict(list)
raw=(root/'raw.csv').read_bytes()
keys=[]
for r in csv.DictReader(raw.decode().splitlines()):
    key=tuple(r[k] for k in ('n','retention','association','namespace'))
    if key not in keys:keys.append(key)
    tag=f"life-{keys.index(key)}-{r['repeat']}-{r['variant']}"
    assert record[tag]['stdout'].strip().split(',')==[r[k] for k in r if k not in ('variant','repeat')]
    samples[r['variant'],key].append(r)

def stat(data):
    q1,_,q3=statistics.quantiles(data,n=4,method='inclusive')
    return dict(median=statistics.median(data),low=min(data),high=max(data),iqr=q3-q1)
cells={}
for (variant,key), group in samples.items():
    formal=[r for r in group if 0<int(r['repeat'])<8]
    diag,=[r for r in group if r['repeat']=='8']
    assert len(formal)==7 and len(group)==9
    fields={p:stat([float(r[p]) for r in formal]) for p in phases}
    fields['sum_timed_phases']=stat([sum(float(r[p]) for p in phases) for r in formal])
    fields.update(retained=int(diag['requested_retained']),peak=int(diag['requested_peak']),
                  allocation_calls=int(diag['allocation_calls']),wire_bytes=int(diag['wire_bytes']))
    cells[variant,key]=fields

result=dict(rows=sum(map(len,samples.values())),raw_sha256=hashlib.sha256(raw).hexdigest(),
            memory_skips=[v['tag'] for v in skips],ratios={},sparse={})
for variant in ('stable41','flat','wire'):
    ratios={}
    for p in (*phases,'sum_timed_phases'):
        ratios[p]=statistics.median(c[p]['median']/cells['base',k][p]['median']
                for (v,k),c in cells.items() if v==variant and cells['base',k][p]['median']>0)
    result['ratios'][variant]=ratios
for v in items:
    if v['tag'].startswith('sparse-') and v.get('code')==0:
        result['sparse'][v['tag']]=v['stdout'].splitlines()
if (root/'live.csv').exists():
    rows=list(csv.DictReader((root/'live.csv').open()))
    live=defaultdict(list)
    ordered=[]
    for r in rows:
        trace=r['trace']
        if trace not in ordered:ordered.append(trace)
        tag=f"live-{ordered.index(trace)}-{r['repeat']}-{r['variant']}"
        assert record[tag]['stdout'].strip().split(',')==[r[k] for k in r if k not in ('trace','sha256','variant','repeat')]
        assert r['requested_after_destroy']=='0'
        live[r['variant'],trace].append(r)
    assert len(live)==135 and len(rows)==1215
    med={k:stat([float(r['seconds']) for r in group if 0<int(r['repeat'])<8]) for k,group in live.items()}
    live_result={}
    for control in ('base','stable41'):
        ratios=[med['wire',trace]['median']/med[control,trace]['median'] for trace in ordered]
        live_result[control]=dict(median=statistics.median(ratios),worst=max(ratios),
            worst_trace=ordered[ratios.index(max(ratios))],wins=sum(r<1 for r in ratios))
    result['live']=dict(rows=len(rows),comparisons=live_result,
                        cells={v+':'+t:x for (v,t),x in med.items()})
(root/'analysis.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
(root/'lifecycle-cells.json').write_text(json.dumps({v+':'+','.join(k):d for (v,k),d in cells.items()},indent=2)+'\n',
                                       encoding='utf-8',newline='\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('sparse','live')},indent=2))
if 'live'in result:print(result['live']['comparisons'])
