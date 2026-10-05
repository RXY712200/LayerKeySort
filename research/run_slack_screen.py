#!/usr/bin/env python3
"""Pair the unchanged frozen workload driver for baseline and one prototype.

Only scenario selection and evidence packaging live here. Workload generation,
RNG, oracle, pairing and quantiles come from the frozen simulator/driver.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "benchmarks"))
import run_workloads as frozen


def screen_plan():
    return ([('timeline', s, 10000, 100000) for s in frozen.PRIMARY]
            + [('timeline', frozen.HOLDOUT, 10000, 100000), ('timeline', 17, 100000, 100000)]
            + [('priority', s, 10000, 100000) for s in frozen.PRIMARY]
            + [('priority', frozen.HOLDOUT, 10000, 100000)]
            + [(w, 17, 10000, 100000) for w in ('random', 'duplicates', 'alternating', 'local', 'churn')]
            + [('churn', s, 50000, 500000) for s in (17, frozen.HOLDOUT)])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--prototype', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--full', action='store_true')
    args = p.parse_args()
    prefix = 'theory-slack-full-' if args.full else 'theory-slack-screen-'
    args.output.mkdir(parents=True, exist_ok=True)
    frozen.check(not list(args.output.glob(prefix + '*')), 'never overwrite evidence')
    plan = frozen.campaign_plan() if args.full else screen_plan()
    tables = {name: [] for name in ('summary', 'growth', 'tail', 'latency-classes')}
    binaries = {}
    for label, directory in (('baseline', args.baseline), ('slack', args.prototype)):
        binaries[label] = {}
        for role, stem in (('timed', 'layerkeysort_workload'), ('diagnostic', 'layerkeysort_workload_diagnostics')):
            path = (directory / (stem + ('.exe' if sys.platform == 'win32' else ''))).resolve()
            binaries[label][role] = {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    metadata = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    captured_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                    environment=platform.platform(), python=sys.version, plan=plan, binaries=binaries,
                    order='sequential baseline timed/diagnostic, slack timed/diagnostic per scenario',
                    timing='frozen C17 timespec_get insertion-only, nearest-rank quantiles; one pass, no affinity/power isolation',
                    compiler='GCC 16.2.0 UCRT x64, C17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror',
                    machine='Ryzen 9 9955HX; 16 cores / 32 logical processors; 15922848 KiB RAM',
                    definitions='benchmarks/workload_simulator.c and benchmarks/run_workloads.py frozen workloads/RNG/seeds',
                    source_sha256={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                   for path in (ROOT / 'src/tree.c', ROOT / 'research/slack_family.c',
                                                ROOT / 'benchmarks/workload_simulator.c', ROOT / 'benchmarks/run_workloads.py')},
                    rewritten_bytes='diagnostic cumulative old resident Path object + allocated storage; includes initial prefix',
                    capacity_counters='diagnostic cumulative; includes initial prefix; not measured production latency')
    for i, scenario in enumerate(plan):
        logical = None
        for label in ('baseline', 'slack'):
            print(f'{i + 1}/{len(plan)} {label} {scenario}', flush=True)
            pair_args = SimpleNamespace(**{role: Path(binaries[label][role]['path']) for role in ('timed', 'diagnostic')})
            results = frozen.pair(pair_args, scenario)
            digest = results[0][0]['trace_digest']
            if logical is None:
                logical = digest
            frozen.check(logical == digest, 'cross-strategy logical trace mismatch')
            for diagnostic, (summary, growth, events, latency, wall) in enumerate(results):
                common = dict(run=i + 1, strategy=label, build='diagnostic' if diagnostic else 'timed',
                              workload=scenario[0], seed=scenario[1], initial=scenario[2], operations=scenario[3])
                row = {**common, **{k: v for k, v in summary.items() if k not in ('kind', 'histogram')},
                       **{k: v for k, v in next(x for x in latency if x['category'] == 'all').items() if k not in ('kind', 'category')},
                       'process_wall_ms': round(wall, 3)}
                for field in ('relabelled_total', 'max_region', 'attempts', 'attempted_nodes_sum', 'full_nodes'):
                    if not diagnostic:
                        row[field] = ''
                for name, count in zip(frozen.BINS, summary['histogram']):
                    row['bin_' + name] = count if diagnostic else ''
                for field, value in growth[-1].items():
                    if field not in ('kind', 'operation', 'path_digest'):
                        row['final_' + field] = value
                if diagnostic:
                    row['measured_alloc_calls'] = growth[-1]['alloc_calls'] - growth[0]['alloc_calls']
                    row['measured_requested_bytes'] = growth[-1]['requested_bytes'] - growth[0]['requested_bytes']
                tables['summary'].append(row)
                for name, records in (('growth', growth), ('tail', events), ('latency-classes', latency)):
                    tables[name].extend({**common, **{k: v for k, v in record.items() if k != 'kind'}} for record in records)
        for name, rows in tables.items():
            frozen.write_csv(args.output / (prefix + name + '.csv'), rows)
    for label in binaries.values():
        for item in label.values():
            frozen.check(hashlib.sha256(Path(item['path']).read_bytes()).hexdigest() == item['sha256'], 'binary changed during capture')
    metadata.update(completed_pairs=2 * len(plan), scenarios=len(plan), correctness_failures=0)
    (args.output / (prefix + 'metadata.json')).write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
