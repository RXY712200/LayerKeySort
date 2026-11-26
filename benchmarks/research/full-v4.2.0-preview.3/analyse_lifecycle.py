"""Reconcile archived lifecycle rows with raw process outputs and compute cells."""
from pathlib import Path
import csv,gzip,hashlib,json,statistics,sys
from collections import defaultdict
root=Path(sys.argv[1])
with gzip.open(root/'receipts.jsonl.gz','rt',encoding='utf-8') as f:
 records=[json.loads(line) for line in f]
calls={r['tag']:r for r in records if r['tag'].startswith('life-')}
assert len(calls)==810 and len({r['tag'] for r in records})==len(records)
assert all(r['code']==0 and r['available']>=r['estimate']+512*1024**2 for r in records)
cells=defaultdict(list)
order=defaultdict(list)
raw=(root/'raw.csv').read_bytes()
for r in csv.DictReader(raw.decode().splitlines()):
 mix=int(r['mix'])
 cell=tuple(r[k] for k in ('n','retention','association','namespace'))
 if cell not in order[mix]:order[mix].append(cell)
 tag=f"life-{mix}-{order[mix].index(cell)}-{r['repeat']}-{r['variant']}"
 assert calls.pop(tag)['stdout'].strip().split(',')==list(r.values())[3:]
 cells[mix,cell,r['variant']].append(r)
assert not calls and len(cells)==90
phases=('build','edit','capture','key','encode','decode','restore','cleanup')
def stats(a):
 q1,_,q3=statistics.quantiles(a,n=4,method='inclusive')
 return dict(median=statistics.median(a),min=min(a),max=max(a),iqr=q3-q1)
summary={}
for (mix,cell,v),g in cells.items():
 assert len(g)==9
 formal=[r for r in g if 1<=int(r['repeat'])<=7]
 diag,=[r for r in g if int(r['repeat'])==8]
 s={p:stats([float(r[p]) for r in formal]) for p in phases}
 if 'wall' in formal[0]:s['wall']=stats([float(r['wall']) for r in formal])
 s['sum_phases']=stats([sum(float(r[p]) for p in phases) for r in formal])
 s.update({k:int(diag[k]) for k in ('wire_bytes','requested_retained','requested_peak','allocation_calls')})
 summary[mix,cell,v]=s
def matched_ratios(metric):
 return {
  f'{mix}:{v}': stats([
   summary[mix,k,'final'][metric]['median'] / summary[mix,k,v][metric]['median']
   for k in order[mix]
  ])
  for mix in (0,1)
  for v in ('stable40','stable41','preview12','tree' if mix else 'flat')
 }

result=dict(rows=810,sha256=hashlib.sha256(raw).hexdigest(),
            ratios=matched_ratios('sum_phases'),
            cells={f"{m}:{','.join(k)}:{v}":s for (m,k,v),s in summary.items()})
if all('wall' in s for s in summary.values()):
 result['wall_ratios']=matched_ratios('wall')
(root/'analysis.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('Summed phases:', result['ratios'])
if 'wall_ratios' in result:
 print('Process wall time:', result['wall_ratios'])
