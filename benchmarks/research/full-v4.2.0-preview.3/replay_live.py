"""Run the pinned Preview.3 comparison from the Full repository root."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('workspace', type=Path)
parser.add_argument('--c-smoke',action='store_true',help='build and check C semantic gates only')
args = parser.parse_args()
work = args.workspace.resolve()
work.mkdir()
here = Path(__file__).resolve().parent.parent/'full-v4.2.0-preview.1.2/replay'
pins = {
    'stable40':'80004ae732daf696f7e9192e417de0edb0b2aafa',
    'stable41':'70d7f0ef5857b54381b863d292a213909d87459f',
    'preview1':'c977b0fd14a501d769b9783bfdcc61266bd1b5ae',
    'candidate':'fe00536e7bd7f4e5128432ea4a29e54158074903',
}
archive = 'f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/'
def run(*args):
    subprocess.run(list(map(str,args)),check=True)
def read(path):
    return subprocess.check_output(['git','show',archive+path])
run('git','fetch','--no-tags','https://github.com/RXY712200/LayerKeySort.git',archive.split(':')[0])
checks = json.loads(read('trace-manifest.json'))
traces = work/'traces'
traces.mkdir()
with zipfile.ZipFile(io.BytesIO(read('traces.zip'))) as bundle:
    assert sorted(bundle.namelist()) == sorted(row['name'] for row in checks)
    for row in checks:
        name = row['name']
        assert name == Path(name).name
        data = bundle.read(name)
        assert hashlib.sha256(data).hexdigest() == row['sha256']
        (traces/name).write_bytes(data)
(traces/'manifest.json').write_text(json.dumps(checks),encoding='utf-8')
(traces/'semantic-gate.trace').write_bytes((here/'semantic-gate.trace').read_bytes())
baseline = work/'baseline'
baseline.mkdir()
for name in ('harness.c','baseline.c','baseline.h','third_party/NOTICE.md','third_party/PCG-LICENSE.txt'):
    target = baseline/name
    target.parent.mkdir(exist_ok=True)
    target.write_bytes(read('baseline/'+name))
if sys.platform.startswith('linux'):
    harness = baseline/'harness.c'
    text = harness.read_text()
    assert text.count('timespec_get(&t,TIME_UTC);') == 1
    text = '#define _POSIX_C_SOURCE 200809L\n' + text.replace(
        'timespec_get(&t,TIME_UTC);', 'if(clock_gettime(CLOCK_MONOTONIC,&t)) abort();')
    harness.write_text(text,encoding='utf-8',newline='\n')
om = work/'om'
for name in ('Cargo.toml','Cargo.lock','src/main.rs'):
    target = om/name
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(read('tooling/om/'+name))
(om/'upstream-source.tar.gz').write_bytes(read('tooling/vendor/order-maintenance-ac4b902d.tar.gz'))
for name,pin in pins.items():
    src=work/(name+'-src')
    run('git','init',src)
    run('git','-C',src,'fetch','--depth=1','--no-tags','https://github.com/RXY712200/LayerKeySort.git',pin)
    run('git','-C',src,'checkout','--detach','FETCH_HEAD')
    build=work/(name+'-build')
    run('cmake','-S',here,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_COMPILER=gcc','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
        '-DFULL_SOURCE='+src.as_posix(),'-DEXPECTED_FULL='+pin,'-DBASELINE_SOURCE='+baseline.as_posix())
    run('cmake','--build',build,'--parallel','3')
if args.c_smoke:
    ext = '.exe' if os.name=='nt' else ''
    for build,kind in [('stable40','full'),('stable41','full'),('preview1','full'),('candidate','full'),
                       ('candidate','list'),('candidate','tree')]:
        run(work/(build+'-build')/(kind+'_diag'+ext),traces/'semantic-gate.trace','1')
    sys.exit(0)
for mode,feature in [('prod',[]),('diag',['--features','diagnostic'])]:
    run('cargo','build','--locked','--release',*feature,'--manifest-path',om/'Cargo.toml',
        '--target-dir',work/('om-'+mode))
exe='ist-v05-om'+('.exe' if os.name=='nt' else '')
run(sys.executable,'-B',here/'collect.py','--suite','components','--build',work/'candidate-build',
    '--preview1',work/'preview1-build','--stable40',work/'stable40-build','--stable41',work/'stable41-build','--traces',traces,
    '--om-prod',work/'om-prod'/'release'/exe,'--om-diag',work/'om-diag'/'release'/exe,
    '--output',work/'capture')
