"""Run the frozen native C screen and separate structural profiles."""
import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import zipfile

PIN = '61e9290ec5ba47b9092bcb8ae7d61f13104ea839'
OLD = 'f41b7be182c2ef766ee0dec1211500f7dd762f54:benchmarks/research/full-ri-20261124/archive/'
HERE = Path(__file__).resolve().parent
VARIANTS = {'base': 64, 'transfer': 64, 'b32': 32, 'b128': 128}
GUARD = 512 * 1024**2
SUFFIX = '.exe' if os.name == 'nt' else ''
parser = argparse.ArgumentParser()
parser.add_argument('output', type=Path)
args = parser.parse_args()
out = args.output.resolve()
out.mkdir()
receipt = (out/'invocations.jsonl').open('w', encoding='utf-8')

def available():
    if os.name != 'nt':
        for line in Path('/proc/meminfo').read_text().splitlines():
            if line.startswith('MemAvailable:'):
                return int(line.split()[1])*1024
        raise RuntimeError('MemAvailable unavailable')
    import ctypes
    class Memory(ctypes.Structure):
        _fields_ = [('length', ctypes.c_ulong), ('load', ctypes.c_ulong)] + [
            (name, ctypes.c_ulonglong) for name in ('total', 'avail', 'page', 'pageavail', 'virtual', 'virtualavail', 'extended')]
    memory = Memory()
    memory.length = ctypes.sizeof(memory)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):
        raise OSError('GlobalMemoryStatusEx failed')
    return memory.avail

def execute(command, *, tag, limit=120):
    cmd = [str(x) for x in command]
    free = available()
    row = dict(tag=tag, argv=cmd, available=free)
    try:
        if free < GUARD:
            row['status'] = 'skipped-memory'
            raise RuntimeError('memory admission failed')
        result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=limit)
        row.update(status='ok' if not result.returncode else 'failed',
                   code=result.returncode, stdout=result.stdout, stderr=result.stderr)
        result.check_returncode()
        return result.stdout
    except subprocess.TimeoutExpired as exc:
        row.update(status='timeout', stdout=str(exc.stdout), stderr=str(exc.stderr))
        raise
    finally:
        receipt.write(json.dumps(row)+'\n')
        receipt.flush()

def git(*cmd):
    return subprocess.check_output(['git', *cmd])

def digest(data):
    return hashlib.sha256(data).hexdigest()

metadata = dict(platform=platform.platform(), processor=platform.processor(),
                compiler=subprocess.check_output(['gcc','--version'], text=True), pin=PIN,
                guard_bytes=GUARD, binaries={}, patched_sources={})
archive = git('archive', PIN)
metadata['source_tar_sha256'] = digest(archive)
baseline = out/'harness'
baseline.mkdir()
execute(['git','fetch','--no-tags','https://github.com/RXY712200/LayerKeySort.git', OLD.split(':')[0]],tag='reference-fetch')
for name in ('harness.c','baseline.c','baseline.h'):
    (baseline/name).write_bytes(git('show',OLD+'baseline/'+name))
if os.name != 'nt':
    path = baseline/'harness.c'
    source = path.read_text()
    assert source.count('timespec_get(&t,TIME_UTC);') == 1
    path.write_text('#define _POSIX_C_SOURCE 200809L\n'+source.replace(
        'timespec_get(&t,TIME_UTC);','if(clock_gettime(CLOCK_MONOTONIC,&t)) abort();'))
manifest = json.loads(git('show',OLD+'trace-manifest.json'))
traces = []
with zipfile.ZipFile(io.BytesIO(git('show',OLD+'traces.zip'))) as bundle:
    for entry in manifest:
        name = entry['name']
        assert Path(name).name == name
        data = bundle.read(name)
        assert digest(data)==entry['sha256']
        if entry['seed'] == 17:
            (out/name).write_bytes(data)
            traces.append(entry)
assert len(traces)==15
metadata['traces'] = traces

def build(variant):
    source = out/variant
    source.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive)) as bundle:
        bundle.extractall(source,filter='data')
    if variant == 'transfer':
        path = source/'src/order.c'
        old = path.read_text()
        start = old.index('static void v4_repair(')
        end = old.index('LksStatus lks_order_remove(',start)
        path.write_text(old[:start]+(HERE/'direct_repair.inc').read_text()+'\n'+old[end:])
    metadata['patched_sources'][variant] = digest((source/'src/order.c').read_bytes())
    b = source/'build'
    cap = f'-DLKS_ORDER_BLOCK_CAPACITY={VARIANTS[variant]}'
    execute(['cmake','-S',source,'-B',b,'-G','Ninja','-DCMAKE_C_COMPILER=gcc',
             '-DCMAKE_BUILD_TYPE=Release','-DLKS_BUILD_BENCHMARKS=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
             '-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror '+cap],tag=variant+'-configure')
    execute(['cmake','--build',b,'--parallel','3','--target','layerkeysort','layerkeysort_diagnostics',
             'layerkeysort_v4_trace','layerkeysort_v4_trace_diagnostics'],tag=variant+'-build')
    for mode in ('prod','diag'):
        diag = mode == 'diag'
        lib = b/('liblayerkeysort_diagnostics.a' if diag else 'liblayerkeysort.a')
        replay = b/(mode+SUFFIX)
        flags = ['gcc','-std=c17','-O3','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror',
                 cap,'-I'+str(source/'include'),'-I'+str(source/'src')]
        flags += ['-DDIAGNOSTIC','-DLKS_ENABLE_ALLOC_DIAGNOSTICS'] if diag else []
        libs = [lib]+(['-lpsapi'] if os.name=='nt' else [])
        execute([*flags,'-DFULL=1','-DTREE=0',baseline/'harness.c',baseline/'baseline.c',
                 *libs,'-o',replay],tag=variant+'-'+mode+'-link')
        profile = b/('structure-'+mode+SUFFIX)
        execute([*flags,HERE/'structure.c',*libs,'-o',profile],tag=variant+'-'+mode+'-structure-link')
        metadata['binaries'][str(replay.relative_to(out))] = digest(replay.read_bytes())
        metadata['binaries'][str(profile.relative_to(out))] = digest(profile.read_bytes())
    return b

def trace(b, mode, family, population, repeat):
    exe = b/('layerkeysort_v4_trace_diagnostics' if mode=='diag' else 'layerkeysort_v4_trace')
    stdout = execute([str(exe)+SUFFIX,family,population,5000,0,1779033703,repeat],
                     tag=f'{b.parent.name}-profile-{mode}-{family}-{population}-{repeat}')
    record = json.loads(stdout.splitlines()[-1])
    assert record['oracle'] and record['cleanup_live_bytes']==0

families = ('endpoint','local','large_moves','hotspot','random','churn','managed_churn')
builds = {'base': build('base')}
for population in (1000,10000,100000):
    for family in families:
        trace(builds['base'],'diag',family,population,0)
for variant in ('transfer','b32','b128'):
    builds[variant] = build(variant)
with (out/'raw.csv').open('w',newline='') as stream:
    csvout = csv.writer(stream,lineterminator='\n')
    csvout.writerow(['trace','population','scenario','repeat','variant','mode','seconds','live','peak',
                     'allocs','frees','cleanup','rss','peak_rss','assignments','max_assignments',
                     'splits','merges','redistributions','cross_moves','total_frees'])
    for i,entry in enumerate(traces):
        for repeat in range(9):
            mode = 'diag' if repeat==8 else 'prod'
            order = list(builds)
            offset = (i+repeat)%len(order)
            for variant in order[offset:]+order[:offset]:
                stdout = execute([builds[variant]/(mode+SUFFIX),out/entry['name'],int(mode=='diag')],
                                 tag=f"replay-{entry['name']}-{repeat}-{variant}")
                values = stdout.strip().split(',')
                assert len(values)==15 and int(values[5])==0
                csvout.writerow([entry['name'],entry['n'],entry['scenario'],repeat,variant,mode,*values])
for repeat in range(4):
    order = list(builds)
    for variant in order[repeat:]+order[:repeat]:
        b=builds[variant]
        for family in families:
            trace(b,'prod',family,10000,repeat)
            if repeat==0:
                trace(b,'diag',family,10000,repeat)
        for mode in ('prod','diag') if repeat==0 else ('prod',):
            execute([b/('structure-'+mode+SUFFIX)],tag=f'{variant}-structure-{mode}-{repeat}')
(out/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
receipt.close()
