"""Derive compact, auditable results without selecting favorable repetitions."""
import csv
import hashlib
import json
import re
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

capture, long_capture, target = map(Path, sys.argv[1:4])
target.mkdir(parents=True, exist_ok=True)
evidence = target / 'evidence'; evidence.mkdir(exist_ok=True)
runs = [json.loads(line) for line in (capture/'runs.jsonl').read_text().splitlines()]
rows, checkpoints, final_process = [], [], []
for run in runs:
    if run['status'] != 'completed':
        continue
    run_id = run['id']
    parsed = list(csv.DictReader((capture/f'{run_id}.csv').open()))
    assert parsed and parsed[-1]['phase'] == 'library_destroy'
    for row in parsed:
        assert None not in row and all(v is not None for v in row.values())
        row = {'run_id': run_id, 'repeat': run['repeat'], 'seed': run['seed'], 'warmup': run['warmup'], **row}
        row['live_residents_derived'] = '0' if row['phase'] == 'library_destroy' else row['residents']
        rows.append(row)
    if run['mode'] != 'production':
        assert parsed[-1]['live_requested_bytes'] == '0' and parsed[-1]['live_allocations'] == '0'
    err = (capture/f'{run_id}.stderr.txt').read_text()
    assert 'COMPLETE ' in err and 'RESEARCH_FAILURE' not in err
    completed=re.search(r'COMPLETE mode=(\w+) profile=(\w+) n=(\d+) cleanup_requested_zero=(\w+) seconds=([0-9.]+) working_set_after_fixture_free=(\d+)',err)
    assert completed
    final_process.append(dict(run_id=run_id,mode=completed[1],profile=completed[2],scale=completed[3],
                              tracked_cleanup=completed[4],child_internal_seconds=completed[5],working_set_after_fixture_free=completed[6]))
    for match in re.finditer(r'CHECKPOINT (\w+) count=(\d+) exact_forward_reverse_identity=true digest=([0-9a-f]+) oracle_seconds=([0-9.]+)', err):
        checkpoints.append(dict(run_id=run_id, checkpoint=len([c for c in checkpoints if c['run_id']==run_id]),
                                profile=match[1], count=match[2], digest=match[3], verification_seconds=match[4]))


def write_csv(name, data):
    with (evidence/name).open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(data[0]));writer.writeheader();writer.writerows(data)


write_csv('observations.csv', rows)
write_csv('checkpoints.csv', checkpoints)
write_csv('process-final.csv',final_process)
export_runs=[]
for r in runs:
    r=dict(r)
    if r.get('retry_of'):
        r['inherited_fields_note']='reason and time_utc describe original admission deferral; precise retry UTC start was not recorded. Actual retry memory, duration, exit status and command are recorded.'
    export_runs.append(r)
(evidence/'runs.json').write_text(json.dumps(export_runs, indent=2), encoding='utf-8')
(evidence/'environment.json').write_bytes((capture/'environment.json').read_bytes())
# Exact checkpoint digests must match across all modes/repetitions for same seed.
groups = defaultdict(list)
run_map = {r['id']: r for r in runs}
for c in checkpoints:
    r = run_map[c['run_id']]
    groups[(r['scale'],r['profile'],r['seed'],c['checkpoint'])].append((c['count'],c['digest']))
assert all(len(set(values))==1 for values in groups.values())
primary = [r for r in rows if r['mode']=='production' and not r['warmup'] and int(r['seed'])==17]
aggregates = []
for key in sorted({(int(r['scale']),r['profile'],r['phase']) for r in primary}):
    n, profile, phase = key
    subset = [r for r in primary if (int(r['scale']),r['profile'],r['phase'])==key]
    times = [float(r['wall_seconds']) for r in subset]
    aggregates.append(dict(scale=n,profile=profile,phase=phase,repetitions=len(subset),operations=int(subset[0]['operations']),
                           minimum_seconds=min(times),median_seconds=statistics.median(times),maximum_seconds=max(times),
                           median_ns_per_api_operation=statistics.median(times)*1e9/int(subset[0]['operations']),
                           largest_sample_seconds=max(float(r['max_sample_seconds']) for r in subset),
                           sample_count_min=min(int(r['latency_samples']) for r in subset)))
write_csv('summary.csv', aggregates)
long_runs = json.loads((long_capture/'runs.json').read_text())
long_rows, long_checks = [], []
for r in long_runs:
    assert r['status']=='completed'
    label=f"{r['mode']}-{r['scale']}-{r['repeat']}"
    actual=list(csv.DictReader((long_capture/f'{label}.csv').open()))
    assert len(actual)==100 and 'LONG_COMPLETE' in (long_capture/f'{label}.stderr.txt').read_text()
    long_rows.extend({'run_id':label,'repeat':r['repeat'],**x} for x in actual)
    for i,m in enumerate(re.finditer(r'LONG_CHECK count=(\d+) digest=([0-9a-f]+) exact_forward_reverse=true',(long_capture/f'{label}.stderr.txt').read_text())):
        long_checks.append(dict(run_id=label,scale=r['scale'],checkpoint=i,count=m[1],digest=m[2]))
write_csv('long-churn.csv',long_rows)
write_csv('long-checkpoints.csv',long_checks)
for n in (100000,1000000):
    ids={x['run_id'] for x in long_checks if x['scale']==n}
    sequences=[[(x['count'],x['digest']) for x in long_checks if x['run_id']==i] for i in sorted(ids)]
    assert all(s==sequences[0] for s in sequences)
(evidence/'long-runs.json').write_text(json.dumps(long_runs,indent=2),encoding='utf-8')

lines=['# Fresh measured results\n\nAll tables derive from committed raw observations; no old timing rows are included.\n\n## Completed/limited scale matrix\n\n| Scale | Production processes | Allocation processes | Structural processes | Policy skips |\n|---|---:|---:|---:|---|\n']
for n in (100000,1000000,3000000,10000000):
    counts=Counter(r['mode'] for r in runs if r.get('scale')==n and r['status']=='completed' and not r.get('warmup'))
    skip=('history capped at 1m' if n==3000000 else 'history and sanitizer omitted') if n>1000000 else 'none in planned scale matrix'
    lines.append(f"| {n:,} | {counts['production']} | {counts['allocation']} | {counts['structural']} | {skip} |\n")
lines+=['\n10k warmups are separate and excluded. Primary seed 17; 1m anchor/random holdouts use seed 712200 and remain separate from timing summaries. At 10m append/random/duplicates have two production repeats; other families have one. Two snapshots plus loaded state and restored live order are capped at 1m.\n\n## Growth throughput and scaling\n\nTotal phase seconds; median [minimum, maximum]. Parentheses give number of fresh processes. Each growth phase contains N inserts.\n\n| Profile | 100k | 1m | 3m | 10m | 10m ns/insert |\n|---|---:|---:|---:|---:|---:|\n']
for profile in ['append','prepend','anchor','local','ascending','descending','random','duplicates','equal']:
    cells=[]
    for n in (100000,1000000,3000000,10000000):
        a=next(x for x in aggregates if x['scale']==n and x['profile']==profile and x['phase']=='growth')
        cells.append(f"{a['median_seconds']:.6f} [{a['minimum_seconds']:.6f}, {a['maximum_seconds']:.6f}] ({a['repetitions']})")
    lines.append(f"| {profile} | {' | '.join(cells)} | {a['median_ns_per_api_operation']:.1f} |\n")
lines+=['\n## Append-origin editing phases\n\n100k or 200k moves per phase depending on scale; churn rows contain twice that many API calls. Values are **ns per API operation**, including replay bookkeeping, sampling and periodic safety checks. Sampled churn latency brackets encompass a remove+insert pair and must not be compared as single-call quantiles.\n\n| Phase | 100k | 1m | 3m | 10m | 10m largest sampled bracket (us) |\n|---|---:|---:|---:|---:|---:|\n']
for phase in ['local_move','hotspot_move','front_back_move','uniform_move','uniform_churn_1','hotspot_churn','uniform_churn_2','uniform_churn_3','half_delete','regrow']:
    vals=[]
    for n in (100000,1000000,3000000,10000000):
        a=next(x for x in aggregates if x['scale']==n and x['profile']=='append' and x['phase']==phase)
        vals.append(f"{a['median_ns_per_api_operation']:.1f}")
    lines.append(f"| {phase} | {' | '.join(vals)} | {a['largest_sample_seconds']*1e6:.1f} |\n")
lines+=['\n## Allocation and occupancy at growth checkpoints\n\nRequested bytes exclude allocator metadata. Wrapper RSS contains its tracking headers; production RSS is a separate observation. Structural objects contain extra diagnostic state.\n\n| Profile / scale | Allocation calls | Requested bytes | Requested B/resident | Production working set B | Blocks | Mean occupancy | Height |\n|---|---:|---:|---:|---:|---:|---:|---:|\n']
for n in (100000,1000000,3000000,10000000):
    for profile in ('append','anchor','random','duplicates'):
        a=next((x for x in rows if x['mode']=='allocation' and int(x['scale'])==n and x['profile']==profile and x['phase']=='growth'),None)
        if not a:continue
        p=next(x for x in primary if int(x['scale'])==n and x['profile']==profile and x['phase']=='growth')
        s=next((x for x in rows if x['mode']=='structural' and int(x['scale'])==n and x['profile']==profile and x['phase']=='growth'),None)
        blocks=s['blocks'] if s else 'unmeasured';occupancy=f"{n/int(s['blocks']):.2f}" if s else 'unmeasured';height=s['height'] if s else 'unmeasured'
        lines.append(f"| {profile} / {n:,} | {int(a['allocations']):,} | {int(a['live_requested_bytes']):,} | {int(a['live_requested_bytes'])/n:.3f} | {int(p['working_set_bytes']):,} | {blocks} | {occupancy} | {height} |\n")
lines+=['\n## Selected historical operations\n\nMedians in milliseconds, one operation per phase per process (no per-call percentile). Restore includes resolver callbacks and scratch cleanup. Requested peak is the process-lifetime Full high-water mark, not a phase-only peak.\n\n| Phase | 100k ms | 1m ms | 1m phase allocations / reallocations | 1m requested live B | 1m cumulative requested peak B |\n|---|---:|---:|---:|---:|---:|\n']
for phase in ['capture','retained_two','serialize','deserialize','restore','restored_destroy','history_destroy','library_destroy']:
    vals=[]
    for n in (100000,1000000):
        a=next(x for x in aggregates if x['scale']==n and x['profile']=='history' and x['phase']==phase);vals.append(f"{a['median_seconds']*1000:.3f}")
    m=next(x for x in rows if x['mode']=='allocation' and int(x['scale'])==1000000 and x['profile']=='history' and x['phase']==phase)
    lines.append(f"| {phase} | {' | '.join(vals)} | {m['allocations']} / {m['reallocations']} | {int(m['live_requested_bytes']):,} | {int(m['peak_requested_bytes']):,} |\n")
lines+=['\n## Long-lifetime churn: 100 cycles\n\nEach process performs **10 million remove/reinsert pairs / 20 million API calls**, no resident growth. Every tenth cycle plus first/final has full forward/reverse identity verification. Two production processes and one allocation/structural process per scale. Cycle times are sequential observations, not independent trials.\n\n| Scale | Production median cycle ms | Production max cycle ms | Allocation live B cycle 1 / 50 / 100 | Allocation working set B cycle 1 / 50 / 100 |\n|---|---:|---:|---|---|\n']
for n in (100000,1000000):
    prod=[x for x in long_rows if x['mode']=='production' and int(x['scale'])==n]
    alloc=[x for x in long_rows if x['mode']=='allocation' and int(x['scale'])==n]
    times=[float(x['wall_seconds'])*1000 for x in prod]
    live=' / '.join(f"{int(alloc[i]['live_requested_bytes']):,}" for i in (0,49,99))
    rss=' / '.join(f"{int(alloc[i]['working_set_bytes']):,}" for i in (0,49,99))
    lines.append(f"| {n:,} | {statistics.median(times):.3f} | {max(times):.3f} | {live} | {rss} |\n")
lines+=['\n## Correctness and safety summary\n\n']
lines.append(f"- Completed matrix processes: {sum(r['status']=='completed' for r in runs)} including {sum(r.get('warmup',False) for r in runs)} warmups. Eight skipped-attempt records retain six initial memory admission deferrals (all subsequently completed on retry) plus two history-policy omissions.\n")
lines.append(f"- Exact matrix checkpoints: {len(checkpoints)}. All counts/sequence digests agree across modes and repetitions for identical scale/profile/seed/checkpoint.\n")
lines.append(f"- Long-churn processes: {len(long_runs)}, exact matching checkpoints: {len(long_checks)}; every completed process reported zero tracked cleanup where instrumentation exists.\n")
lines.append(f"- Smallest phase-observed available physical memory: {min(int(x['available_physical_bytes']) for x in rows)/1024**3:.3f} GiB. This is not a continuously measured system minimum.\n")
lines.append(f"- Maximum observed location assignments per diagnostic API operation: {max(int(x['max_operation_assignments']) for x in rows if x['mode']=='structural')}; maximum distinct index blocks per operation: {max(int(x['max_operation_index_blocks']) for x in rows if x['mode']=='structural')}. These are observations, not latency guarantees.\n")
assert not any(r['status'] in ('failed','budget_aborted') for r in runs)
lines+=['- Recorded final statuses contain zero failed executions and zero budget aborts. Completed is an observed process status, not an independent acceptance decision.\n',
         '- Raw `residents` is the model cardinality; the library_destroy row retains the former source cardinality. `live_residents_derived` explicitly becomes zero at destruction. Empty latency samples mean unmeasured, not zero latency.\n',
         '- No 3m/10m history campaign or 10m sanitizer campaign was executed. The upper live-order target was actually executed, not inferred.\n']
(target/'RESULTS.md').write_text(''.join(lines),encoding='utf-8',newline='\n')
print('Wrote',len(rows),'raw phase rows,',len(checkpoints),'checkpoints,',len(long_rows),'long-churn cycle rows')
