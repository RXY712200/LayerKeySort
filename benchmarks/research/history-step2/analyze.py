"""Recompute inclusive phase summaries and verify raw cross-variant oracles."""
import argparse
import csv
import gzip
import io
import json
import re
import statistics
from collections import Counter, defaultdict
from pathlib import Path


def main():
    p = argparse.ArgumentParser(); p.add_argument('capture', type=Path); p.add_argument('output', type=Path)
    args = p.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    runs = [json.loads(x) for x in (args.capture/'runs.jsonl').read_text().splitlines()]
    bundled = {}
    if (args.capture/'raw.jsonl.gz').exists():
        with gzip.open(args.capture/'raw.jsonl.gz', 'rt', encoding='utf-8') as f:
            bundled = {r['id']: r for r in map(json.loads, f)}
    phases = []; fingerprints = {}; checkpoints = 0; groups = defaultdict(list); process_walls = defaultdict(list)
    for run in runs:
        if run['status'] != 'completed': continue
        if run['id'] in bundled: raw = bundled[run['id']]
        else:
            with gzip.open(args.capture/(run['id']+'.json.gz'), 'rt', encoding='utf-8') as f: raw = json.load(f)
        assert run['returncode'] == 0 and 'COMPLETE' in raw['stderr']
        errors = raw['stderr']; digest = re.findall(r'(?:CHECK history shift=\d+ count|CHECKPOINT \w+ count|LONG_CHECK count)=(\d+).*?digest=([0-9a-f]+)', errors)
        key = run['kind'], run['residents'], run['retention'], run['namespace']
        assert digest and (key not in fingerprints or fingerprints[key] == digest), ('oracle mismatch', run['id'])
        fingerprints[key] = digest; checkpoints += len(digest)
        rows = list(csv.DictReader(io.StringIO(raw['stdout']))); assert all(None not in r for r in rows)
        if run['mode'] != 'production' and run['kind'] != 'long':
            final = rows[-1]; assert final.get('live_requested_bytes', final.get('requested_bytes')) == '0', run['id']
        if run['mode'] != 'production' and run['kind'] == 'long':
            assert 'tracked_cleanup=zero' in errors, run['id']
        for r in rows:
            r.update(id=run['id'], variant=run['variant'], repeat=run['repeat'], kind=run['kind'],
                     n=run['residents'], keep=run['retention'], ns=run['namespace'], measured_mode=run['mode'])
            if 'phase' not in r: r['phase'] = 'long_cycle'
            if 'index' not in r: r['index'] = r.get('cycle', '0')
            phases.append(r)
            if run['mode'] == 'production':
                groups[key+(r['phase'], r['index'], run['variant'])].append(r)
        if run['mode'] == 'production':
            process_walls[key+(run['variant'],)].append(sum(float(r['wall_seconds']) for r in rows))
    def write(name, rows):
        if not rows: return
        fields = list(dict.fromkeys(k for row in rows for k in row))
        with (args.output/name).open('w', newline='', encoding='utf-8') as f:
            w = csv.DictWriter(f, fields); w.writeheader(); w.writerows(rows)
    write('phases.csv', phases)
    comparisons = []
    for key in sorted({k[:-1] for k in groups}):
        available = {variant: groups[key+(variant,)] for variant in 'abc'}
        if not available['b'] or not available['c']: continue
        row = dict(kind=key[0], residents=key[1], retention=key[2], namespace=key[3], phase=key[4], index=key[5])
        for variant, observations in available.items():
            if not observations: continue
            row[variant+'_repeats'] = len(observations)
            row[variant+'_seconds'] = statistics.median(float(r['wall_seconds']) for r in observations)
            row[variant+'_min_seconds'] = min(float(r['wall_seconds']) for r in observations)
            row[variant+'_max_seconds'] = max(float(r['wall_seconds']) for r in observations)
            row[variant+'_working_set'] = statistics.median(float(r['working_set_bytes']) for r in observations)
        row['c_over_b'] = row['c_seconds']/row['b_seconds'] if row['b_seconds'] else 0
        comparisons.append(row)
    write('comparison.csv', comparisons)
    totals = []
    for key in sorted({k[:-1] for k in process_walls}):
        row = dict(kind=key[0], residents=key[1], retention=key[2], namespace=key[3])
        for v in 'abc':
            if process_walls[key+(v,)]: row[v+'_timed_phase_sum'] = statistics.median(process_walls[key+(v,)])
        if 'b_timed_phase_sum' in row and 'c_timed_phase_sum' in row:
            row['c_over_b'] = row['c_timed_phase_sum']/row['b_timed_phase_sum']
        totals.append(row)
    write('timed_phase_totals.csv', totals)
    result = dict(process_statuses=dict(Counter(r['status'] for r in runs)), phase_rows=len(phases),
                  exact_oracle_checkpoints=checkpoints, fingerprint_groups=len(fingerprints),
                  fingerprints_equal_across_variants_modes_repeats=True,
                  limitations='Whole phase costs; callback replay is not subtracted; checkpoints are not per-mutation proof.')
    (args.output/'verification.json').write_text(json.dumps(result, indent=2));print(json.dumps(result))


if __name__ == '__main__': main()
