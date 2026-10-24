"""Fresh paired Windows processes. Raw output is compressed, never overwritten."""
import argparse
import csv
import ctypes
import gzip
import io
import json
import platform
import subprocess
import time
from pathlib import Path

GIB = 1024**3
PROFILES = 'append prepend anchor local ascending descending random duplicates equal'.split()


class Memory(ctypes.Structure):
    _fields_ = [('length', ctypes.c_uint32), ('load', ctypes.c_uint32)] + [
        (x, ctypes.c_uint64) for x in ('total', 'available', 'page_total', 'page_available', 'virtual_total', 'virtual_available', 'extended')]


def memory():
    m = Memory(); m.length = ctypes.sizeof(m)
    assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return dict(total=m.total, available=m.available, load=m.load)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('baseline', type=Path); p.add_argument('candidate', type=Path)
    p.add_argument('output', type=Path); p.add_argument('--scales', default='100000,1000000,3000000,10000000')
    p.add_argument('--profiles', default=','.join(PROFILES))
    p.add_argument('--repeats', type=int, default=2)
    p.add_argument('--production-only', action='store_true')
    p.add_argument('--long', action='store_true'); args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True); log = args.output/'runs.jsonl'
    prior = [json.loads(x) for x in log.read_text().splitlines()] if log.exists() else []
    done = {r['id'] for r in prior}
    for n in map(int, args.scales.split(',')):
        plan = []
        if args.long:
            for repeat in (1, 2): plan.append(('production', 'long', repeat))
            plan += [(mode, 'long', 1) for mode in ('allocation', 'structural')]
        else:
            profiles = args.profiles.split(',')
            if args.profiles == ','.join(PROFILES) and n <= 1000000: profiles += ['history']
            for repeat in range(1, args.repeats+1):
                plan += [('production', profile, repeat) for profile in profiles]
            if not args.production_only:
                plan += [(mode, profile, 1) for mode in ('allocation', 'structural')
                         for profile in ['append', 'random'] + (['history'] if n <= 1000000 else [])]
        for index, (mode, profile, repeat) in enumerate(plan):
            order = ['baseline', 'candidate'] if (index+repeat) % 2 else ['candidate', 'baseline']
            for variant in order:
                label = f'{variant}-{mode}-{profile}-{n}-{repeat}'
                if label in done: continue
                m = memory(); estimate = n*(300 if profile == 'history' else 160 if mode == 'production' else 200)+64*1024**2
                row = dict(id=label, variant=variant, mode=mode, profile=profile, scale=n, repeat=repeat,
                           seed=17, utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), memory_before=m,
                           envelope=estimate, reserve=3*GIB, timeout=270)
                if estimate > min(3*GIB, .4*m['available']) or m['available']-estimate < 3*GIB:
                    row.update(status='safely_skipped', reason='memory admission reserve')
                else:
                    stem = 'full_long' if args.long else 'full_scale'
                    suffix = dict(production='', allocation='_alloc', structural='_structural')[mode]
                    command = [str((getattr(args, variant)/(stem+suffix+'.exe')).resolve())]
                    command += [str(n), '100'] if args.long else [profile, str(n), '17']
                    row['command'] = command; start = time.perf_counter()
                    try:
                        r = subprocess.run(command, capture_output=True, timeout=270)
                        output, error = r.stdout.decode('utf-8'), r.stderr.decode('utf-8')
                        with gzip.open(args.output/(label+'.json.gz'), 'wt', encoding='utf-8') as f:
                            json.dump(dict(stdout=output, stderr=error), f)
                        parsed = list(csv.DictReader(io.StringIO(output)))
                        valid = r.returncode == 0 and parsed and 'COMPLETE' in error
                        if not args.long: valid = valid and parsed[-1]['phase'] == 'library_destroy'
                        row.update(status='completed' if valid else 'failed', returncode=r.returncode, rows=len(parsed))
                    except subprocess.TimeoutExpired:
                        row.update(status='timeout')
                    row['process_seconds'] = time.perf_counter()-start
                row['memory_after'] = memory()
                with log.open('a', encoding='utf-8') as f: f.write(json.dumps(row)+'\n')
                print(label, row['status'], round(row.get('process_seconds', 0), 3), flush=True)
                if row['status'] in ('failed', 'timeout'): raise RuntimeError(label)
    metadata = dict(platform=platform.platform(), memory=memory(), synthetic=True,
                    baseline='043872eb5560e867aa7f8c094e0e9dbf2e8a7b95', seed=17,
                    inherited_harness='d9d3b370fb8daf134eadc26c64e3b30aae12ec75',
                    flags='GCC C17 -O3 -DNDEBUG; driver -Wall -Wextra -Wpedantic -Werror')
    (args.output/'environment.json').write_text(json.dumps(metadata, indent=2))


if __name__ == '__main__': main()
