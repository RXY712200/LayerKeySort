"""Run pinned C prototype screens from staged Release builds."""
import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import subprocess
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('output',type=Path)
for name in ('control','tags','no-count'):
    parser.add_argument('--'+name,type=Path,required=True)
a = parser.parse_args()
out = a.output.resolve()
out.mkdir()
archive = 'f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/'
def read(path):
    return subprocess.check_output(['git','show',archive+path])
def sha(data):
    return hashlib.sha256(data).hexdigest()
inputs = [x for x in json.loads(read('trace-manifest.json')) if x['n'] in (10000,100000) and x['seed']==17]
assert len(inputs)==10
with zipfile.ZipFile(io.BytesIO(read('traces.zip'))) as z:
    for x in inputs:
        assert Path(x['name']).name == x['name']
        data = z.read(x['name'])
        assert sha(data)==x['sha256']
        (out/x['name']).write_bytes(data)
for name in ('harness.c','baseline.c','baseline.h'):
    (out/name).write_bytes(read('baseline/'+name))
if os.name!='nt':
    p=out/'harness.c'
    s=p.read_text()
    assert s.count('timespec_get(&t,TIME_UTC);')==1
    p.write_text('#define _POSIX_C_SOURCE 200809L\n'+s.replace('timespec_get(&t,TIME_UTC);','if(clock_gettime(CLOCK_MONOTONIC,&t)) abort();'))
variants={'control':a.control,'tags':a.tags,'no-count':a.no_count}
assert all(x.is_dir() for x in variants.values())
ext='.exe' if os.name=='nt' else ''
here=Path(__file__).resolve().parent
metadata={'platform':platform.platform(),'processor':platform.processor(),'compiler':subprocess.check_output(['gcc','--version'],text=True),'inputs':inputs,'commands':{}}
for kind,source in variants.items():
    for mode in ('prod','diag'):
        common=['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(source/'include'),'-I'+str(source/'src')]
        suffix='_diagnostics' if mode=='diag' else ''
        lib=source/'build'/('liblayerkeysort'+suffix+'.a')
        if mode=='diag':common+=['-DDIAGNOSTIC','-DLKS_ENABLE_ALLOC_DIAGNOSTICS']
        builds={'replay':['-DFULL=1','-DTREE=0',str(out/'harness.c'),str(out/'baseline.c')],
                'hotspot':[str(here/'hotspot.c')]}
        if kind=='tags':builds['hotspot'].append('-DPREVIEW_TAGS')
        for role,parts in builds.items():
            exe=out/(kind+'-'+mode+'-'+role+ext)
            cmd=common+parts+[str(lib)]+(['-lpsapi'] if os.name=='nt' else [])+['-o',str(exe)]
            subprocess.run(cmd,check=True)
            metadata['commands'][exe.name]={'build':cmd,'sha256':sha(exe.read_bytes())}
(out/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
with (out/'invocations.jsonl').open('w',encoding='utf-8') as receipts:
    def execute(kind, mode, role, args):
        cmd=[str(out/(kind+'-'+mode+'-'+role+ext)),*map(str,args)]
        r=subprocess.run(cmd,capture_output=True,timeout=120,text=True)
        receipts.write(json.dumps(dict(command=cmd,status=r.returncode,stdout=r.stdout,stderr=r.stderr))+'\n')
        receipts.flush()
        r.check_returncode()
        return r.stdout
    with (out/'raw.csv').open('w',newline='') as stream:
        w=csv.writer(stream,lineterminator='\n')
        w.writerow(['trace','repeat','kind','mode','seconds','requested_live','requested_peak','allocs','frees','after_destroy','rss','peak_rss','assignments','max_assignments','splits','merges','redistributions','cross_moves','total_frees'])
        ordered=list(variants)
        for i,x in enumerate(inputs):
            for repeat in range(9):
                mode='diag' if repeat==8 else 'prod'
                offset=(i+repeat)%len(ordered)
                for k in ordered[offset:]+ordered[:offset]:
                    row=execute(k,mode,'replay',[out/x['name'],str(int(mode=='diag'))]).strip().split(',')
                    assert len(row)==15 and int(row[5])==0
                    w.writerow([x['name'],repeat,k,mode,*row])
        for k in ordered:
            for mode in ('prod','diag'):
                execute(k,mode,'hotspot',[])
