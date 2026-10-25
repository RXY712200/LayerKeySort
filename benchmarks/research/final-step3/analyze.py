"""Recheck raw oracles, then add primary integrated C/A summaries to C/B."""
import csv,json,statistics,subprocess,sys
from collections import defaultdict
from pathlib import Path
capture,output=map(Path,sys.argv[1:]);old=Path(__file__).resolve().parent.parent/'history-step2/analyze.py'
subprocess.run([sys.executable,str(old),str(capture),str(output)],check=True)
rows=list(csv.DictReader((output/'phases.csv').open(newline='',encoding='utf-8')));groups=defaultdict(list)
for r in rows:
 if r['measured_mode']=='production':groups[(r['kind'],r['n'],r['keep'],r['ns'],r['phase'],r['index'],r['variant'])].append(float(r['wall_seconds']))
summary=[]
for key in sorted({k[:-1] for k in groups}):
 r=dict(zip(('kind','n','keep','ns','phase','index'),key))
 for v in 'abc':
  values=groups[key+(v,)]
  if values:r.update({v+'_median':statistics.median(values),v+'_min':min(values),v+'_max':max(values),v+'_repeats':len(values)})
 if 'c_median' in r:
  for v in 'ab':
   if r.get(v+'_median'):r['c_over_'+v]=r['c_median']/r[v+'_median']
 summary.append(r)
if summary:
 fields=list(dict.fromkeys(k for r in summary for k in r))
 with (output/'integrated-comparison.csv').open('w',newline='',encoding='utf-8') as f:
  w=csv.DictWriter(f,fields);w.writeheader();w.writerows(summary)
print('Integrated phase summaries:',len(summary))
