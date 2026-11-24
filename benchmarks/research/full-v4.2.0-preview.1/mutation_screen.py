import argparse, subprocess,csv,statistics as st
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('trace_directory',type=Path)
parser.add_argument('baseline_directory',type=Path)
parser.add_argument('stable_library',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args()
p=args.trace_directory.resolve()
root=Path(__file__).resolve().parent
base=args.baseline_directory
lib=args.stable_library
variants=['control','ancestor','mutation','combined']
for kind in variants:
 cmd=['gcc','-std=c17','-O3','-DNDEBUG','-Iinclude','-Isrc','-DFULL=1','-DTREE=0',str(base/'harness.c'),str(base/'baseline.c')]
 if kind!='control':cmd+=['-x','c',str(root/(kind+'-order.c.txt')),'-x','none']
 subprocess.run(cmd+[str(lib),'-lpsapi','-o',str(p/(kind+'.exe'))],check=True)
ratios={k:[] for k in variants[1:]}
with args.output.open('x',newline='') as f:
 w=csv.writer(f,lineterminator='\n');w.writerow(['trace','repeat','variant','seconds'])
 for j,t in enumerate(sorted(p.glob('*.trace'))):
  a={k:[] for k in variants}
  for r in range(8):
   s=(j+r)%4
   for k in variants[s:]+variants[:s]:
    q=subprocess.run([str(p/(k+'.exe')),str(t),'0'],capture_output=True,timeout=120,check=True)
    v=float(q.stdout.split(b',')[0]);w.writerow([t.name,r,k,v]);f.flush()
    if r:a[k].append(v)
  for k in ratios:ratios[k].append((t.name,st.median(a[k])/st.median(a['control'])))
for k,v in ratios.items():print(k,len(v),st.median(y for x,y in v),st.median(y for x,y in v if 'compare-heavy' in x),max(y for x,y in v))
