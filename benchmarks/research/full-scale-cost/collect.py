"""Sequential, budgeted Windows collection. No production mutation or CI writes."""
import argparse
import csv
import ctypes
import hashlib
import io
import json
import platform
import subprocess
import time
from pathlib import Path

GIB = 1024**3
PROFILES = ['append', 'prepend', 'anchor', 'local', 'ascending', 'descending', 'random', 'duplicates', 'equal']


class Memory(ctypes.Structure):
    _fields_ = [('length', ctypes.c_uint32), ('load', ctypes.c_uint32)] + [
        (name, ctypes.c_uint64) for name in ('total', 'available', 'page_total', 'page_available', 'virtual_total', 'virtual_available', 'extended')]


def memory():
    m = Memory(); m.length = ctypes.sizeof(m)
    assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return {'total': m.total, 'available': m.available, 'load': m.load}


def estimate(n, mode, history):
    # Generous planning envelopes, not measured bytes: 32N app/model/maps,
    # about 50N Full requests, allocation headers/CRT slack, 4N sort scratch,
    # bounded trace/sample buffers, and extra simultaneous history/restore.
    per_resident = 300 if history else (200 if mode != 'production' else 160)
    return per_resident*n + 64*1024**2


def main():
    p = argparse.ArgumentParser()
    p.add_argument('build', type=Path); p.add_argument('output', type=Path)
    p.add_argument('--scales', default='100000,1000000,3000000,10000000')
    args = p.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    log = args.output / 'runs.jsonl'
    previous = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
    completed = {r['id'] for r in previous}
    def run(mode, profile, n, repeat, seed=17, warmup=False):
        label = f'{mode}-{profile}-{n}-{repeat}-{seed}'
        if label in completed:
            return
        m = memory(); planned = estimate(n, mode, profile == 'history')
        row = {'id': label, 'mode': mode, 'profile': profile, 'scale': n, 'repeat': repeat,
               'seed': seed, 'warmup': warmup, 'memory_before': m,
               'estimated_envelope_bytes': planned, 'reserve_bytes': 3*GIB,
               'process_budget_seconds': 270, 'time_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
        exe = {'production': 'full_scale.exe', 'allocation': 'full_scale_alloc.exe', 'structural': 'full_scale_structural.exe'}[mode]
        if planned > min(3*GIB, m['available']*.40) or m['available']-planned < 3*GIB:
            row.update(status='safely_skipped', reason='conservative memory envelope/reserve not satisfied')
        else:
            command = [str(args.build.resolve()/exe), profile, str(n), str(seed)]
            row['command'] = command; started = time.perf_counter()
            try:
                result = subprocess.run(command, text=True, encoding='utf-8', capture_output=True, timeout=270)
                (args.output/f'{label}.csv').write_text(result.stdout, encoding='utf-8', newline='\n')
                (args.output/f'{label}.stderr.txt').write_text(result.stderr, encoding='utf-8', newline='\n')
                row['returncode'] = result.returncode
                parsed = list(csv.DictReader(io.StringIO(result.stdout)))
                valid = result.returncode == 0 and 'COMPLETE ' in result.stderr and parsed and parsed[-1]['phase'] == 'library_destroy'
                valid = valid and all(None not in r for r in parsed)
                row.update(status='completed' if valid else ('budget_aborted' if result.returncode == 77 else 'failed'), phase_rows=len(parsed))
            except subprocess.TimeoutExpired as error:
                row.update(status='budget_aborted', reason='270-second subprocess timeout; direct child killed')
                (args.output/f'{label}.timeout.txt').write_bytes((error.stdout or b'')+(error.stderr or b''))
            row['process_wall_seconds'] = time.perf_counter()-started
        row['memory_after'] = memory()
        with log.open('a', encoding='utf-8') as f:
            f.write(json.dumps(row)+'\n')
        print(f"{label}: {row['status']} wall={row.get('process_wall_seconds',0):.2f}s free_before={m['available']/GIB:.2f}GiB", flush=True)
        if row['status'] in ('failed', 'budget_aborted'):
            raise RuntimeError(f'Inspect preserved failure before continuing: {label}')
    metadata_path = args.output / 'environment.json'
    if not metadata_path.exists():
        root = Path(__file__).resolve().parents[3]
        metadata = {'date_label': '2026-10-24', 'platform': platform.platform(), 'memory_at_start': memory(),
                    'compiler': subprocess.check_output(['gcc','--version'], text=True).splitlines()[0],
                    'base_main': subprocess.check_output(['git','rev-parse','origin/main'], cwd=root, text=True).strip(),
                    'stable': subprocess.check_output(['git','rev-parse','v4.0.0^{}'], cwd=root, text=True).strip(),
                    'previous_research': 'bf27502f18de5988cd2d1c797ddc3e0ff48d0b03',
                    'source_sha256': {x.name: hashlib.sha256(x.read_bytes()).hexdigest() for x in Path(__file__).parent.iterdir() if x.suffix in ('.c','.h','.py','.txt')},
                    'provenance': 'fresh deterministic synthetic engineering research, not human traces',
                    'flags': '-std=c17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror'}
        metadata_path.write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    for n in map(int,args.scales.split(',')):
        if n == 100000:
            for profile in PROFILES+['history']:
                run('production',profile,10000,0,warmup=True)
        # Smaller successful pilots must precede larger scale collection.
        if n > 100000:
            all_records = [json.loads(line) for line in log.read_text().splitlines()]
            pilot = 100000 if n == 1000000 else (1000000 if n == 3000000 else 3000000)
            assert any(r['scale']==pilot and r['profile']=='append' and r['status']=='completed' for r in all_records), 'missing smaller pilot'
        repeats = 3 if n <= 1000000 else (2 if n == 3000000 else 1)
        for repeat in range(1,repeats+1):
            for profile in PROFILES+(['history'] if n<=1000000 else []):
                run('production',profile,n,repeat)
        if n == 10000000:
            for profile in ('append','random','duplicates'):
                run('production',profile,n,2)
        allocation_profiles = PROFILES+['history'] if n<=1000000 else (['append','prepend','anchor','local','random','duplicates'] if n==3000000 else ['append','prepend','anchor','random','duplicates'])
        structural_profiles = PROFILES+['history'] if n<=1000000 else (['append','anchor','random'] if n==3000000 else ['append','anchor'])
        for profile in allocation_profiles:
            run('allocation',profile,n,1)
        for profile in structural_profiles:
            run('structural',profile,n,1)
        if n==1000000:
            for profile in ('anchor','random'):
                run('production',profile,n,1,seed=712200)
        if n>1000000:
            row={'id': f'history-{n}-policy-skip', 'mode':'all', 'profile':'history', 'scale':n, 'status':'safely_skipped', 'reason':'simultaneous snapshots/loaded snapshot/second live order capped at 1m; 10m sanitizer also excluded', 'memory_before':memory(), 'estimated_envelope_bytes':estimate(n,'production',True)}
            if row['id'] not in completed:
                with log.open('a',encoding='utf-8') as f:f.write(json.dumps(row)+'\n')


if __name__ == '__main__':
    main()
