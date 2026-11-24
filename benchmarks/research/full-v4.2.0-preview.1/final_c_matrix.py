"""Build and replay C controls from the frozen archive in fresh processes."""
import argparse, csv, hashlib, io, json, os, platform, subprocess, zipfile
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('output',type=Path)
parser.add_argument('stable40',type=Path)
parser.add_argument('stable41',type=Path)
parser.add_argument('candidate',type=Path)
a=parser.parse_args()
a.output.mkdir()
archive='f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/'
def blob(path): return subprocess.check_output(['git','show',archive+path])
def digest(data): return hashlib.sha256(data).hexdigest()
manifest=json.loads(blob('trace-manifest.json'))
with zipfile.ZipFile(io.BytesIO(blob('traces.zip'))) as z:
    assert sorted(z.namelist())==sorted(x['name'] for x in manifest)
    for x in manifest:
        name=x['name']
        assert Path(name).name==name
        data=z.read(name)
        assert digest(data)==x['sha256']
        (a.output/name).write_bytes(data)
for name in ('harness.c','baseline.c','baseline.h'):
    (a.output/name).write_bytes(blob('baseline/'+name))
variants={'stable40':a.stable40,'stable41':a.stable41,'candidate':a.candidate,'list':None,'tree':None}
meta={'platform':platform.platform(),'cpu':platform.processor(),'compiler':subprocess.check_output(['gcc','--version'],text=True),'binaries':{}}
for name,lib in variants.items():
    exe=(a.output/(name+('.exe' if os.name=='nt' else ''))).resolve()
    cmd=['gcc','-std=c17','-O3','-DNDEBUG','-Iinclude','-Isrc',f'-DFULL={int(lib is not None)}',f'-DTREE={int(name=="tree")}',str(a.output/'harness.c'),str(a.output/'baseline.c')]
    if lib: cmd.append(str(lib))
    if os.name=='nt':cmd.append('-lpsapi')
    subprocess.run(cmd+['-o',str(exe)],check=True)
    meta['binaries'][name]={'file':str(exe),'command':cmd+['-o',str(exe)],'sha256':digest(exe.read_bytes()),'library_sha256':digest(lib.read_bytes()) if lib else None}
(a.output/'metadata.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
names=list(variants)
with (a.output/'raw.csv').open('x',newline='') as f:
    w=csv.writer(f,lineterminator='\n')
    w.writerow(['trace','repeat','implementation','seconds','requested_live','requested_peak','alloc_calls','free_calls_before_destroy','requested_after_destroy','working_set','peak_working_set','records_reassigned','max_reassigned','splits','merges','redistributions','cross_block_moves','total_frees'])
    for i,x in enumerate(manifest):
        for repeat in range(8):
            shift=(i+repeat)%len(names)
            for name in names[shift:]+names[:shift]:
                r=subprocess.run([meta['binaries'][name]['file'],str(a.output/x['name']),'0'],capture_output=True,timeout=120)
                if r.returncode:
                    (a.output/'failure.txt').write_bytes(r.stdout+r.stderr)
                    r.check_returncode()
                values=r.stdout.decode().strip().split(',')
                assert len(values)==15 and not r.stderr
                w.writerow([x['name'],repeat,name,*values])
                f.flush()
