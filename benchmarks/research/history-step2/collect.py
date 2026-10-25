"""Fixed, paired, sequential experiments. Never relax admission or overwrite raw runs."""
import argparse
import csv
import ctypes
import gzip
import io
import json
import platform
import sys
import subprocess
import time
from pathlib import Path

GIB = 1024**3


class Memory(ctypes.Structure):
    _fields_ = [('length', ctypes.c_uint32), ('load', ctypes.c_uint32)] + [
        (x, ctypes.c_uint64) for x in ('total', 'available', 'page_total', 'page_available', 'virtual_total', 'virtual_available', 'extended')]


def memory():
    if sys.platform != "win32":
        values = {line.split(":")[0]: int(line.split()[1])*1024 for line in Path("/proc/meminfo").read_text().splitlines() if line.startswith(("MemTotal:", "MemAvailable:"))}
        return dict(total=values["MemTotal"], available=values["MemAvailable"])
    m = Memory(); m.length = ctypes.sizeof(m)
    assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return dict(total=m.total, available=m.available, load=m.load)


def cases(n):
    if n == 100000:
        return [(2, 'none', 1), (2, 'empty', 1), (8, 'one', 4096), (16, 'eight', 16),
                (16, 'variable', 16), (8, 'fixed256', 16), (4, 'wide', 4096)]
    if n == 1000000:
        return [(2, 'none', 1), (2, 'empty', 1), (4, 'one', 4096), (16, 'eight', 16),
                (8, 'variable', 16), (4, 'fixed256', 4096)]
    return [(4, 'eight', 16), (2, 'variable', 4096)]


def envelope(n, kind, keep, mode):
    # Upper mean payload budgets: actual cumulative useful lengths are deterministic.
    avg = dict(none=0, empty=0, one=1, eight=8, variable=65, fixed256=256, wide=1025).get(kind, 8)
    # Baseline row metadata + geometric association capacity (<2A), loaded
    # snapshot + caller wire, two live orders/CRT metadata, fixtures and scratch.
    if kind in ('legacy', 'append', 'random', 'duplicates', 'long', 'shrink'):
        return n*(320 if kind == 'legacy' else 200)+64*1024**2
    return int(n*(keep*(16+2*avg)+2*(16+avg)+180)+96*1024**2)


def main():
    p = argparse.ArgumentParser()
    for name in ('a', 'b', 'c', 'output'): p.add_argument(name, type=Path)
    p.add_argument('--suite', choices=['history', 'guard', 'long', 'shrink'], default='history')
    p.add_argument('--scales', default='100000,1000000,3000000')
    args = p.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    log = args.output/'runs.jsonl'
    prior = [json.loads(x) for x in log.read_text().splitlines()] if log.exists() else []
    done = {r['id'] for r in prior}
    plan = []
    for n in map(int, args.scales.split(',')):
        if args.suite == 'history':
            for keep, kind, ns in cases(n):
                for mode in ('production', 'allocation', 'structural'):
                    reps = (3 if n <= 1000000 else 2) if mode == 'production' else 1
                    variants = ['b', 'c'] + (['a'] if kind == 'eight' and n <= 1000000 else [])
                    for repeat in range(1, reps+1): plan.append((n, keep, kind, ns, mode, repeat, variants))
            if n <= 1000000:
                for mode in ('production', 'allocation', 'structural'):
                    for repeat in range(1, (5 if n == 100000 else 3)+1 if mode == 'production' else 2):
                        plan.append((n, 2, 'legacy', 2, mode, repeat, ['a', 'b', 'c']))
        elif args.suite == 'guard':
            for kind in ('append', 'random', 'duplicates'):
                for repeat in (1, 2): plan.append((n, 0, kind, 0, 'production', repeat, ['b', 'c']))
                plan.append((n, 0, kind, 0, 'structural', 1, ['b', 'c']))
        elif args.suite == 'long':
            for mode in ('production', 'allocation', 'structural'):
                for repeat in ((1, 2) if mode == 'production' else (1,)):
                    plan.append((n, 0, 'long', 0, mode, repeat, ['b', 'c']))
        else:
            assert n == 1000000
            for repeat in (1, 2): plan.append((n, 0, 'shrink', 0, 'structural', repeat, ['b', 'c']))
    for index, (n, keep, kind, ns, mode, repeat, variants) in enumerate(plan):
        variants = variants[index % len(variants):]+variants[:index % len(variants)]
        if repeat % 2 == 0: variants = list(reversed(variants))
        for variant in variants:
            label = f'{variant}-{mode}-{kind}-{n}-k{keep}-ns{ns}-r{repeat}'
            if label in done: continue
            m = memory(); estimated = envelope(n, kind, keep, mode)
            row = dict(id=label, variant=variant, mode=mode, kind=kind, residents=n, retention=keep,
                       namespace=ns, repeat=repeat, suite=args.suite, memory_before=m,
                       estimated_envelope=estimated, reserve=3*GIB,
                       utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
            if estimated > min(3*GIB, .4*m['available']) or m['available']-estimated < 3*GIB:
                row.update(status='safely_skipped', reason='conservative simultaneous-memory admission')
            else:
                suffix = dict(production='', allocation='_alloc', structural='_structural')[mode]
                binary = 'full_history'+suffix+'.exe'; arguments = [str(n), str(keep), kind, str(ns)]
                if kind in ('legacy', 'append', 'random', 'duplicates'):
                    binary = 'full_scale'+suffix+'.exe'; arguments = ['history' if kind == 'legacy' else kind, str(n), '17']
                if kind == 'long': binary = 'full_long'+suffix+'.exe'; arguments = [str(n), '100']
                if kind == 'shrink': binary = 'full_shrink.exe'; arguments = []
                if sys.platform != "win32": binary = binary.removesuffix(".exe")
                command = [str((getattr(args, variant)/binary).resolve())]+arguments
                row['command'] = command; start = time.perf_counter()
                try:
                    run = subprocess.run(command, capture_output=True, timeout=360)
                    output, error = run.stdout.decode('utf-8'), run.stderr.decode('utf-8')
                    raw = dict(id=label, stdout=output, stderr=error)
                    (args.output/(label+'.json.gz')).write_bytes(gzip.compress(json.dumps(raw).encode(), mtime=0))
                    parsed = list(csv.DictReader(io.StringIO(output)))
                    valid = run.returncode == 0 and parsed and 'COMPLETE' in error and all(None not in r for r in parsed)
                    row.update(status='completed' if valid else 'budget_aborted' if run.returncode == 77 else 'failed',
                               returncode=run.returncode, phase_rows=len(parsed))
                except subprocess.TimeoutExpired:
                    row.update(status='budget_aborted', reason='360-second timeout; child terminated')
                row['process_seconds'] = time.perf_counter()-start
            row['memory_after'] = memory()
            with log.open('a', encoding='utf-8') as f: f.write(json.dumps(row)+'\n')
            print(label, row['status'], round(row.get('process_seconds', 0), 3), flush=True)
            if row['status'] in ('failed', 'budget_aborted'): raise RuntimeError('Inspect preserved unsuccessful process: '+label)
    (args.output/'environment.json').write_text(json.dumps(dict(platform=platform.platform(), memory=memory(),
        baselines=dict(a='043872eb5560e867aa7f8c094e0e9dbf2e8a7b95',b='19ca51bc616a1b6a2d28530bc0932dc7c4a255ce'),
        flags='GCC C17 -O3 -DNDEBUG, driver -Wall -Wextra -Wpedantic -Werror', synthetic=True), indent=2))


if __name__ == '__main__': main()
