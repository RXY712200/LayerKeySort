"""Render paired numeric tables from committed evidence, not hard-coded timings."""
import argparse,csv,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('evidence',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
lines=['# Fresh paired measurement tables','','All timing cells are A / B / C milliseconds. A=original main/Stable; B=Step 1; C=combined candidate. Hosts are never pooled.','']
def table(head,rows):
 lines.append('|'+ '|'.join(head)+'|');lines.append('|'+ '|'.join('---' for _ in head)+'|')
 for row in rows:lines.append('|'+ '|'.join(map(str,row))+'|')
 lines.append('')
def read(host,suite,name='integrated-comparison.csv'):
 return list(csv.DictReader((a.evidence/host/('analysis-'+suite)/name).open(newline='',encoding='utf-8')))
def triple(rows,kind,n,phase,index='0'):
 matches=[r for r in rows if r['kind']==kind and r['n']==str(n) and r['phase']==phase and r['index']==index]
 if not matches:return 'not measured'
 r=matches[0];return ' / '.join(f'{float(r[v+"_median"])*1000:.3f}' if r.get(v+'_median') else 'NA' for v in 'abc')
adverse=[]
for host in ('windows','linux'):
 lines+=['## '+host.title(),'','### Live growth and managed lookup','']
 live=read(host,'live');rows=[]
 for n in (100000,1000000,3000000):
  for kind in ('append','prepend','anchor','local','ascending','descending','random','duplicates','equal'):
   rows.append([n,kind,triple(live,kind,n,'growth'),triple(live,kind,n,'locate') if kind in ('ascending','descending','random','duplicates','equal') else 'not a managed workload'])
 table(['Rows','Trace','Growth A/B/C ms','Locate A/B/C ms'],rows)
 lines+=['### Append-origin interaction phases','','Movement/churn phases use min(N, 200k) operations; churn is remove/reinsert pairs. Grow/shrink retain their actual row counts.','']
 table(['Rows','Phase','A/B/C ms'],[[n,phase,triple(live,'append',n,phase)] for n in (100000,1000000,3000000) for phase in ('local_move','hotspot_move','uniform_move','hotspot_churn','half_delete','regrow','library_destroy')])
 lines+=['### Snapshot / History','','Capture is first snapshot only; all repeated captures and remaining restore cycles are in raw phase tables. Historical-key timings include the independent expected-byte checks.','']
 hist=read(host,'history');keys=sorted({(r['kind'],int(r['n']),r['keep'],r['ns']) for r in hist},key=lambda k:(k[1],k[0]))
 table(['Rows','Payload / retained / namespace bytes','Capture A/B/C ms','Serialize A/B/C ms','Load A/B/C ms','Restore #0 A/B/C ms'],[[n,f'{kind} / {keep} / {ns}',triple(hist,kind,n,'capture'),triple(hist,kind,n,'serialize'),triple(hist,kind,n,'deserialize'),triple(hist,kind,n,'restore')] for kind,n,keep,ns in keys])
 lines+=['### Long churn','','Pooled 200 cycle observations from two fresh production processes/source; each process runs 100 cycles of 100k pairs. These are not 200 independent processes. Each cell is median / worst observed cycle ms.','']
 long=read(host,'long','phases.csv');rows=[]
 for n in (100000,1000000):
  row=[n]
  for v in 'abc':
   values=[1000*float(r['wall_seconds']) for r in long if r['variant']==v and r['n']==str(n) and r['measured_mode']=='production']
   row.append(f'{statistics.median(values):.3f} / {max(values):.3f}')
  rows.append(row)
 table(['Resident count','A median/max','B median/max','C median/max'],rows)
 lines+=['### Severe shrink and regrowth','','Diagnostic timings; two processes/source. These are not production timing substitutes. Requested bytes are native requested storage, not RSS.','']
 shrink=read(host,'shrink','phases.csv');rows=[]
 for phase in ('growth','scan32','shrink90','shrink99','regrow','destroy'):
  row=[phase]
  for v in 'abc':
   values=[r for r in shrink if r['variant']==v and r['phase']==phase]
   row.append(f'{statistics.median(float(r["wall_seconds"])*1000 for r in values):.3f} ms; {int(statistics.median(int(r["requested_bytes"]) for r in values)):,} bytes')
  rows.append(row)
 table(['Phase','A','B','C'],rows)
 lines+=['### Admitted 10m workloads','','Two fresh production processes/source; unadmitted rows are not extrapolated.','']
 upper=read(host,'upper');rows=[]
 for kind in ('append','eight'):
  for phase in (('growth','local_move','hotspot_move','uniform_move','hotspot_churn','library_destroy') if kind=='append' else ('capture','serialize','deserialize','restore','source_destroy','loaded_destroy')):
   if any(r['kind']==kind for r in upper):rows.append([kind,phase,triple(upper,kind,10000000,phase)])
 table(['Trace','Phase','A/B/C ms'],rows)
 for suite in ('live','history','upper'):
  for r in read(host,suite):
   # Listing threshold is for readability only, not an adoption gate.
   if r.get('c_over_a') and float(r['c_over_a'])>1.05 and r['phase']!='callback_replay':
    delta=(float(r['c_median'])-float(r['a_median']))*1000
    adverse.append(dict(host=host,suite=suite,**r,delta_ms=delta))
lines+=['## Interpretation limits','','No confidence interval, universal speedup, arbitrary-duration reliability or cache/fragmentation attribution is claimed. See REPORT.md for engineering classification. `ADVERSE.csv` includes every listed workload phase whose C/A median exceeds 1.05 (excluding callback-only calibration), without deleting the original rows. The listing threshold is not a correctness/adoption threshold.','']
a.output.write_text('\n'.join(lines),encoding='utf-8')
if adverse:
 fields=list(dict.fromkeys(k for r in adverse for k in r))
 with a.output.with_name('ADVERSE.csv').open('w',newline='',encoding='utf-8') as f:
  w=csv.DictWriter(f,fields);w.writeheader();w.writerows(adverse)
print('Table lines',len(lines),'adverse phase rows',len(adverse))
