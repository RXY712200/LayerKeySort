#!/usr/bin/env python3
"""Branch-only A/B driver. Frozen simulator traces and historical CSVs unchanged."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time
from types import SimpleNamespace
import run_workloads as frozen

STRATEGIES = ('baseline', 'direct7', 'direct8', 'depth_bias', 'equal8', 'open_range', 'open_dense')


def executables(build, strategy):
    suffix = '.exe' if os.name == 'nt' else ''
    stem = 'layerkeysort_workload' if strategy == 'baseline' else 'lks_workload_' + strategy
    if strategy == 'baseline':
        return build / (stem + suffix), build / (stem + '_diagnostics' + suffix)
    return build / (stem + '_timed' + suffix), build / (stem + '_diagnostic' + suffix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--phase', choices=('screen', 'full'), required=True)
    parser.add_argument('--strategies', nargs='+', choices=STRATEGIES, default=list(STRATEGIES))
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.build = args.build.resolve()
    prefix = 'tail-cost-research-' + args.phase + '-'
    args.output.mkdir(parents=True, exist_ok=True)
    frozen.check(not list(args.output.glob(prefix + '*')), 'never overwrite research capture')
    frozen.check(not os.environ.get('LKS_RESEARCH_TRACE_OP'), 'disable targeted tracing during A/B timing')
    status, _ = frozen.execute(['git', 'status', '--porcelain', '--untracked-files=no'])
    if args.phase == 'full':
        frozen.check(not status.strip(), 'freeze candidate implementation before full capture')
    commit, _ = frozen.execute(['git', 'rev-parse', 'HEAD'])
    plan = frozen.campaign_plan() if args.phase == 'full' else [(w, 17, 10000, 100000) for w in (*frozen.APPS, 'alternating', 'duplicates', 'random')]
    cache = dict(line.split('=', 1) for line in (args.build / 'CMakeCache.txt').read_text(encoding='utf-8').splitlines() if '=' in line and not line.startswith(('#', '//')))
    compiler = cache['CMAKE_C_COMPILER:STRING']
    compiler_version, _ = frozen.execute([compiler, '--version'])
    metadata = dict(compiler=compiler_version.strip(),
                    build_options={k: v for k, v in cache.items() if k.startswith(('CMAKE_C_FLAGS', 'CMAKE_BUILD_TYPE', 'LKS_'))},
                    machine='Windows 11 Pro 10.0.26300; Ryzen 9 9955HX; 16 cores/32 threads; visible RAM 15922848 KiB; no affinity or power isolation',
                    baseline_main='c33afa2ac08248bffa6dde51bd590a5d799b394a',
                    stable_tag='v3.1.0', stable_commit='dfa9562b9471947cfbd4ee1d1750a59434be83e2',
                    harness_commit=commit.strip(), dirty_tracked=bool(status.strip()), phase=args.phase,
                    strategies=args.strategies, plan=plan, os=platform.platform(),
                    captured_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
                    timing='same frozen C17 clock; public insertion only; production/diagnostic separate; trace disabled',
                    order='scenario outer loop, listed strategies inner loop, production then diagnostic; not randomized',
                    comparison='historical baseline separate; research baseline rerun and candidates in this campaign',
                    binaries={name: {label: hashlib.sha256(path.read_bytes()).hexdigest()
                               for label,path in zip(('timed','diagnostic'),executables(args.build,name))} for name in args.strategies})
    tables = {name: [] for name in ('summary','growth','events','latency-classes')}
    for run, scenario in enumerate(plan, 1):
        print(f'{run}/{len(plan)} {scenario}',flush=True)
        trace = None
        for strategy in args.strategies:
            timed, diag = executables(args.build,strategy)
            for diagnostic,(summary,growth,events,latency,wall) in enumerate(frozen.pair(SimpleNamespace(timed=timed,diagnostic=diag),scenario)):
                frozen.check(summary['strategy']==STRATEGIES.index(strategy),'compile identity mismatch')
                frozen.check(trace is None or trace==summary['trace_digest'], 'logical trace changed across strategies')
                trace = summary['trace_digest']
                common=dict(strategy=strategy,run=run,build='diagnostic' if diagnostic else 'timed',workload=scenario[0],seed=scenario[1],initial=scenario[2],operations=scenario[3])
                row={**common,**{k:v for k,v in summary.items() if k not in ('kind','histogram','strategy')},
                     **{k:v for k,v in next(x for x in latency if x['category']=='all').items() if k not in ('kind','category')},'process_wall_ms':round(wall,3)}
                for field in ('relabelled_total','max_region','attempts','attempted_nodes_sum','full_nodes'):
                    if not diagnostic:row[field]=''
                for field,count in zip(frozen.BINS,summary['histogram']):row['bin_'+field]=count if diagnostic else ''
                for field,value in growth[-1].items():
                    if field not in ('kind','operation','path_digest'):row['final_'+field]=value
                if diagnostic:
                    row['measured_alloc_calls']=growth[-1]['alloc_calls']-growth[0]['alloc_calls']
                    row['measured_requested_bytes']=growth[-1]['requested_bytes']-growth[0]['requested_bytes']
                tables['summary'].append(row)
                for name,records in (('growth',growth),('events',events),('latency-classes',latency)):
                    tables[name].extend({**common,**{k:v for k,v in record.items() if k!='kind'}} for record in records)
        for name,rows in tables.items():frozen.write_csv(args.output/(prefix+name+'.csv'),rows)
    metadata['scenario_pairs']=len(plan)*len(args.strategies)
    metadata['correctness_failures']=0
    (args.output/(prefix+'metadata.json')).write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    print('completed',metadata['scenario_pairs'],'identical-trace production/diagnostic pairs')


if __name__=='__main__':
    main()
