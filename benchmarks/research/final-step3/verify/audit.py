"""Independent cross-source work/memory checks over delivered phase tables."""
import argparse,csv,json
from collections import defaultdict
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('root',type=Path);p.add_argument('output',type=Path);args=p.parse_args()
result={};fields=('allocations','reallocations','frees','live_allocations','comparisons','location_assignments','max_operation_assignments','index_writes','max_operation_index_blocks','splits','merges','redistributions','rotations','blocks','height','occupancy_histogram')
for suite in ('live','upper'):
 path=args.root/('analysis-'+suite)/'phases.csv'
 if not path.exists():continue
 groups=defaultdict(dict)
 for r in csv.DictReader(path.open(newline='',encoding='utf-8')):
  if r['measured_mode']=='structural' and r['kind'] not in ('eight','variable'):
   groups[(r['kind'],r['n'],r['phase'],r['index'],r['repeat'])][r['variant']]=r
 checked=0;memory=0
 for key,rows in groups.items():
  if set(rows) != set('abc'):continue
  for field in fields:
   values={rows[v][field] for v in 'abc'}
   assert len(values)==1,(key,field,values)
  checked+=1
  if rows['a']['live_requested_bytes']!='NA':
   a,b,c=(int(rows[v]['live_requested_bytes']) for v in 'abc')
   assert b==c,(key,b,c)
   # The driver retains its reference-model count in the teardown row even
   # though the actual container has been destroyed and all tracked bytes freed.
   residents=0 if key[2]=='library_destroy' else int(rows['a']['residents'])
   blocks=int(rows['a']['blocks'])
   assert a-c==8*(residents-blocks),(key,a,c,residents,blocks)
   memory+=1
 result[suite]=dict(exact_ABC_structural_phase_pairs=checked,exact_x64_memory_formula_phase_pairs=memory,fields=list(fields))
for suite in ('history','upper'):
 path=args.root/('analysis-'+suite)/'phases.csv';groups=defaultdict(dict)
 if path.exists():
  for r in csv.DictReader(path.open(newline='',encoding='utf-8')):
   # Legacy ID8 has no private Snapshot storage columns.
   if r['measured_mode']=='structural' and r['phase']=='capture' and r['kind']!='legacy':
    groups[(r['kind'],r['n'],r['keep'],r['ns'],r['index'])][r['variant']]=r
  checked=0
  for key,rows in groups.items():
   if set(rows)!=set('abc'):continue
   for field in ('useful_bytes','wire_bytes','callbacks'):
    assert len({rows[v][field] for v in 'abc'})==1,(key,field)
   assert int(rows['a']['row_bytes'])==16*int(rows['a']['n'])
   assert int(rows['b']['row_bytes'])==16*int(rows['b']['n'])
   assert int(rows['c']['row_bytes'])==8*int(rows['c']['n'])
   assert int(rows['c']['association_capacity'])==int(rows['c']['useful_bytes']),key
   checked+=1
  result[suite+'_history']=dict(exact_ABC_capture_phase_pairs=checked,exact_published_capacity_checks=checked)
path=args.root/'analysis-shrink/phases.csv';groups=defaultdict(dict)
if path.exists():
 for r in csv.DictReader(path.open(newline='',encoding='utf-8')):
  groups[(r['phase'],r['repeat'])][r['variant']]=r
 checked=0
 for key,rows in groups.items():
  if set(rows)!=set('abc'):continue
  assert len({rows[v]['residents'] for v in 'abc'})==1,key
  assert len({rows[v]['blocks'] for v in 'abc'})==1,key
  n=int(rows['a']['residents']);blocks=int(rows['a']['blocks'])
  a,b,c=(int(rows[v]['requested_bytes']) for v in 'abc')
  assert b==c and a-c==8*(n-blocks),(key,a,b,c,n,blocks)
  checked+=1
 result['shrink']=dict(exact_x64_reclamation_phase_pairs=checked)
args.output.write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result))
