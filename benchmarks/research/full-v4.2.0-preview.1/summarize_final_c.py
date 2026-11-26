import csv,statistics as s,collections
from pathlib import Path
p=Path('D:/LayerKeySort/.local/preview1-final-c')
a=collections.defaultdict(list)
for r in csv.DictReader((p/'raw.csv').open()):
 if int(r['repeat']):a[r['trace'],r['implementation']].append(float(r['seconds']))
assert len(a)==225 and all(len(v)==7 for v in a.values())
out=Path('benchmarks/research/full-v4.2.0-preview.1/final-windows-c-cells.csv')
with out.open('w',newline='') as f:
 w=csv.writer(f,lineterminator='\n');w.writerow(['trace','implementation','median_s','min_s','max_s','candidate_ratio'])
 for (t,k),v in sorted(a.items()):w.writerow([t,k,s.median(v),min(v),max(v),s.median(a[t,'candidate'])/s.median(v)])
for k in ['stable40','stable41','list','tree']:
 vals=[(t,s.median(a[t,'candidate'])/s.median(v)) for (t,name),v in a.items() if k==name]
 print(k,len(vals),s.median(r for t,r in vals),s.median(r for t,r in vals if 'compare-heavy' in t),max(vals,key=lambda x:x[1]))
