"""Validate preserved raw processes and produce inclusive comparison tables."""
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
    groups = defaultdict(list); fingerprints = {}; checkpoints = 0; phases = []
    structural = {}; structural_pairs = 0
    structural_fields = 'comparisons location_assignments max_operation_assignments index_writes max_operation_index_blocks splits merges redistributions rotations blocks height occupancy_histogram'.split()
    for run in runs:
        if run['status'] != 'completed': continue
        if run['id'] in bundled: raw = bundled[run['id']]
        else:
            with gzip.open(args.capture/(run['id']+'.json.gz'), 'rt', encoding='utf-8') as f: raw = json.load(f)
        assert 'COMPLETE' in raw['stderr'] and run['returncode'] == 0
        digest = re.findall(r'(?:CHECKPOINT \w+ count|LONG_CHECK count)=(\d+).*?digest=([0-9a-f]+)', raw['stderr'])
        key = run['profile'], run['scale'], run['seed']
        assert digest and (key not in fingerprints or fingerprints[key] == digest), ('oracle mismatch', run['id'])
        fingerprints[key] = digest; checkpoints += len(digest)
        for row in csv.DictReader(io.StringIO(raw['stdout'])):
            assert None not in row
            row.update(id=run['id'], variant=run['variant'], repeat=run['repeat'])
            phases.append(row)
            if run['mode'] == 'structural' and run['profile'] != 'long':
                key = run['profile'], run['scale'], row['phase']
                value = tuple(row[x] for x in structural_fields)
                if key in structural:
                    assert structural[key] == value, ('structural mismatch', run['id'], key)
                    structural_pairs += 1
                structural[key] = value
            if run['mode'] == 'production':
                phase = row.get('phase', 'long_churn')
                groups[run['profile'], run['scale'], phase, run['variant']].append(row)
        if run['mode'] != 'production' and run['profile'] != 'long':
            assert phases[-1]['live_requested_bytes'] == '0' and phases[-1]['live_allocations'] == '0'
    if phases:
        fields = list(dict.fromkeys(k for r in phases for k in r))
        with (args.output/'phases.csv').open('w', newline='') as f:
            w = csv.DictWriter(f, fields); w.writeheader(); w.writerows(phases)
    comparisons = []
    for profile, n, phase in sorted({k[:3] for k in groups}):
        a = groups[profile, n, phase, 'baseline']; b = groups[profile, n, phase, 'candidate']
        if not a or not b: continue
        row = dict(profile=profile, scale=n, phase=phase, baseline_observations=len(a), candidate_observations=len(b))
        for variant, observations in [('baseline', a), ('candidate', b)]:
            row[variant+'_seconds'] = statistics.median(float(r['wall_seconds']) for r in observations)
            for metric in ('p50_seconds', 'p95_seconds', 'p99_seconds', 'max_sample_seconds', 'working_set_bytes'):
                if metric in observations[0]:
                    row[variant+'_'+metric] = (max if metric == 'max_sample_seconds' else statistics.median)(float(r[metric]) for r in observations)
        row['candidate_over_baseline'] = row['candidate_seconds']/row['baseline_seconds'] if row['baseline_seconds'] else 0
        comparisons.append(row)
    if comparisons:
        fields = list(dict.fromkeys(k for r in comparisons for k in r))
        with (args.output/'comparison.csv').open('w', newline='') as f:
            w = csv.DictWriter(f, fields); w.writeheader(); w.writerows(comparisons)
    result = dict(process_statuses=dict(Counter(r['status'] for r in runs)), exact_checkpoints=checkpoints,
                  fingerprint_groups=len(fingerprints), cross_variant_and_mode_fingerprints_equal=True,
                  identical_structural_phase_pairs=structural_pairs,
                  phase_rows=len(phases), limitations='Checkpoints are not per-mutation proof; all timings include driver overhead.')
    (args.output/'verification.json').write_text(json.dumps(result, indent=2)); print(json.dumps(result))


if __name__ == '__main__': main()
