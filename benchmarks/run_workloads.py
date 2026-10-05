#!/usr/bin/env python3
"""Deterministic evidence orchestration, using only the Python standard library.

No library tuning occurs here. --smoke validates every family and old control,
trace pairing, bounded output, a deliberately corrupted equality/membership
oracle, and output rejection. --campaign runs the frozen first campaign.
"""
import argparse
import csv
import hashlib
import json
import math
import platform
from pathlib import Path
import subprocess
import sys
import time

APPS = ("timeline", "priority", "local", "churn")
CONTROLS = ("ascending", "descending", "random", "equal", "duplicates", "alternating", "hotspot")
PRIMARY = (17, 0x12345678, 0x9E3779B9, 0x31415926, 0x00C0FFEE)
HOLDOUT = 0xD15EA5ED
BASELINE = "dfa9562b9471947cfbd4ee1d1750a59434be83e2"
BINS = ("direct", "1_8", "9_32", "33_128", "129_512", "513_4096", "4097_plus", "full_range")


def check(condition, message):
    if not condition:
        raise ValueError(message)


def execute(command):
    begin = time.perf_counter()
    result = subprocess.run([str(x) for x in command], text=True, capture_output=True)
    check(result.returncode == 0, f"command failed: {command}\n{result.stderr}\n{result.stdout[-3000:]}")
    return result.stdout, (time.perf_counter() - begin) * 1000


def parse(output, workload, seed, initial, operations, checkpoint, diagnostic):
    rows = [json.loads(line) for line in output.splitlines()]
    check(all(isinstance(row, dict) and "kind" in row for row in rows), "invalid record")
    check(all(row["kind"] in ("growth", "latency", "event", "summary", "cleanup") for row in rows), "unknown record")
    summaries = [row for row in rows if row["kind"] == "summary"]
    cleanups = [row for row in rows if row["kind"] == "cleanup"]
    growth = [row for row in rows if row["kind"] == "growth"]
    events = [row for row in rows if row["kind"] == "event"]
    latency = [row for row in rows if row["kind"] == "latency"]
    check(len(summaries) == len(cleanups) == 1, "missing/duplicated completion")
    summary = summaries[0]
    for field, expected in dict(workload=workload, seed=seed, initial=initial,
                                operations=operations, checkpoint=checkpoint, diagnostic=diagnostic).items():
        check(summary[field] == expected, f"scenario mismatch: {field}")
    check(summary["verified"] is True and
          cleanups[0]["after_destroy_live_bytes"] == (0 if diagnostic else None),
          "correctness/cleanup failure")
    check(summary["new_items"] + summary["remove_only"] + summary["updates"] == operations,
          "operation accounting mismatch")
    check(summary["insertions"] == summary["new_items"] + summary["updates"], "insertion accounting")
    check(summary["final_resident"] == initial + summary["new_items"] - summary["remove_only"], "population accounting")
    check(len(summary["histogram"]) == 8 and sum(summary["histogram"]) == summary["insertions"], "histogram accounting")
    expected_ops = [0] + list(range(checkpoint, operations + 1, checkpoint))
    if expected_ops[-1] != operations:
        expected_ops.append(operations)
    check([row["operation"] for row in growth] == expected_ops, "checkpoint sequence")
    check(growth[-1]["resident"] == summary["final_resident"], "checkpoint population")
    check(len(events) <= (55 if diagnostic else 25), "unbounded default sample")
    check(len({row["operation"] for row in events}) == len(events), "duplicated event")
    check(all(1 <= row["operation"] <= operations for row in events), "event index")
    check(len([row for row in latency if row["category"] == "all"]) == 1, "latency completion")
    all_latency = next(row for row in latency if row["category"] == "all")
    check(all_latency["count"] == summary["insertions"], "timing count")
    for row in latency:
        values = [row[field] for field in ("mean_ms", "p50_ms", "p95_ms", "p99_ms", "max_ms")]
        check(all(math.isfinite(x) and x >= 0 for x in values), "nonfinite/negative timing")
        check(row["p50_ms"] <= row["p95_ms"] <= row["p99_ms"] <= row["max_ms"], "quantile order")
        check((row["p999_ms"] is not None) == (row["count"] >= 10000), "unsupported tail quantile")
    if diagnostic:
        check(sum(row["count"] for row in latency if row["category"] != "all") == summary["insertions"], "class count")
        check(all(row["category"] != "unavailable" for row in events), "missing diagnostic event")
        for row in growth:
            check(row["resident_path_bytes"] == row["path_object_bytes"] + row["path_step_bytes"], "resident accounting")
    else:
        check(all(row["category"] == "unavailable" for row in events), "production work instrumentation")
    return summary, growth, events, latency


def pair(args, scenario):
    workload, seed, initial, operations = scenario
    checkpoint = max(1, operations // 10)
    results = []
    for diagnostic, executable in enumerate((args.timed, args.diagnostic)):
        command = [executable, workload, seed, initial, operations, checkpoint]
        output, wall = execute(command)
        parsed = parse(output, *scenario, checkpoint, diagnostic)
        results.append((*parsed, wall))
    timed, diag = results
    for field in ("trace_digest", "insertions", "new_items", "updates", "remove_only", "final_resident"):
        check(timed[0][field] == diag[0][field], f"trace pairing mismatch: {field}, {scenario}")
    for a, b in zip(timed[1], diag[1]):
        for field in ("operation", "resident", "path_digest", "resident_path_bytes", "lk1_payload_bytes"):
            check(a[field] == b[field], f"coordinate pairing mismatch: {field}, {scenario}")
    return results


def smoke(args):
    for executable in (args.timed, args.diagnostic):
        output, _ = execute([executable, "--oracle-self-test"])
        check(json.loads(output) == {"kind": "oracle_self_test", "passed": True}, "oracle self-test output")
        for bad_args in (("priority", 0, 128, 512, 128), ("unknown", 7, 128, 512, 128),
                         ("churn", 7, 0, 512, 128), ("local", 7, 128, "9999999999999999999999999", 128)):
            check(subprocess.run([str(executable), *map(str, bad_args)], capture_output=True).returncode != 0,
                  "invalid CLI accepted")
    for i, workload in enumerate(APPS + CONTROLS):
        scenario = (workload, 7 if i % 2 else 23, 128, 512)
        pair(args, scenario)
    # Parser must fail closed on truncated output and accounting corruption.
    output, _ = execute([args.timed, "priority", 7, 128, 512, 51])
    rows = [json.loads(line) for line in output.splitlines()]
    corruptions = [rows[:-1], [row for row in rows if row["kind"] != "summary"]]
    wrong = [dict(row) for row in rows]
    next(row for row in wrong if row["kind"] == "summary")["insertions"] += 1
    corruptions.append(wrong)
    for broken in corruptions:
        try:
            parse("\n".join(json.dumps(row) for row in broken), "priority", 7, 128, 512, 51, 0)
        except (ValueError, KeyError):
            continue
        raise ValueError("parser accepted corrupted output")
    print("11 families/controls paired; equality/membership oracle self-test and malformed-output rejection passed")


def campaign_plan():
    plan = [(w, seed, 1000, 10000) for w in APPS for seed in PRIMARY[:2]]
    plan += [(w, seed, 10000, 100000) for w in APPS for seed in PRIMARY]
    plan += [(w, PRIMARY[0], 100000, 100000) for w in APPS]
    plan += [("churn", PRIMARY[0], 50000, 500000)]
    plan += [(w, PRIMARY[0], 1000, 10000) for w in CONTROLS]
    plan += [(w, PRIMARY[0], 10000, 100000) for w in CONTROLS]
    # Reserved seed is executed only after the definitions and primary campaign.
    plan += [(w, HOLDOUT, 10000, 100000) for w in APPS]
    plan += [("churn", HOLDOUT, 50000, 500000)]
    return plan


def write_csv(path, rows):
    fields = list(dict.fromkeys(field for row in rows for field in row))
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def evidence(args):
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    prefix = "v3.1-workload-evidence-"
    check(not list(out.glob(prefix + "*")), "evidence output exists; never overwrite captured results")
    plan = campaign_plan()
    repo = Path(__file__).resolve().parents[1]
    source, _ = execute(["git", "-C", repo, "rev-parse", "HEAD"])
    status, _ = execute(["git", "-C", repo, "status", "--porcelain", "--untracked-files=no"])
    check(not status.strip(), "commit simulator before evidence capture")
    compiler, _ = execute([args.compiler, "--version"])
    metadata = {
        "baseline": BASELINE, "harness_commit": source.strip(), "captured_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "compiler": compiler.strip(), "build_options": args.build_options,
        "os": platform.platform(), "machine": args.machine,
        "python": sys.version, "primary_seeds": PRIMARY, "holdout_seed": HOLDOUT,
        "plan": plan, "warmup": "untimed initial prefix; no discard from measured history",
        "timing": "C17 timespec_get(TIME_UTC), epoch milliseconds as in current_benchmark.c; timer surrounds managed insert only",
        "diagnostics": "separate executable; diagnostic latency includes instrumentation and is not production performance",
        "quantiles": "nearest rank; p999 requires at least 10000 inserts; zero/submicrosecond readings may be timer resolution noise",
        "sampling": "union worst 25 latency, worst 25 relabel size, first 5 full events; optional C --trace is not captured",
        "binaries": {label: {"path": str(Path(path).resolve()), "sha256": hashlib.sha256(Path(path).read_bytes()).hexdigest()}
                     for label, path in (("timed", args.timed), ("diagnostic", args.diagnostic))},
    }
    tables = {name: [] for name in ("summary", "growth", "tail", "latency-classes")}
    for i, scenario in enumerate(plan):
        print(f"{i + 1}/{len(plan)} {scenario}", flush=True)
        for diagnostic, (summary, growth, events, latency, wall) in enumerate(pair(args, scenario)):
            common = dict(run=i + 1, build="diagnostic" if diagnostic else "timed",
                          workload=scenario[0], seed=scenario[1], initial=scenario[2], operations=scenario[3])
            row = {**common, **{k: v for k, v in summary.items() if k not in ("kind", "histogram")},
                   **{k: v for k, v in next(x for x in latency if x["category"] == "all").items() if k not in ("kind", "category")},
                   "process_wall_ms": round(wall, 3)}
            # Counts are unavailable in production, never turn stub zeros into evidence.
            for field in ("relabelled_total", "max_region", "attempts", "attempted_nodes_sum", "full_nodes"):
                if not diagnostic:
                    row[field] = ""
            for bin_name, count in zip(BINS, summary["histogram"]):
                row["bin_" + bin_name] = count if diagnostic else ""
            final, start = growth[-1], growth[0]
            for field, value in final.items():
                if field not in ("kind", "operation", "path_digest"):
                    row["final_" + field] = value
            if diagnostic:
                row["measured_alloc_calls"] = final["alloc_calls"] - start["alloc_calls"]
                row["measured_requested_bytes"] = final["requested_bytes"] - start["requested_bytes"]
            tables["summary"].append(row)
            for name, records in (("growth", growth), ("tail", events), ("latency-classes", latency)):
                tables[name].extend({**common, **{k: v for k, v in record.items() if k != "kind"}} for record in records)
        # Incremental write allows inspection/recovery without giant raw traces.
        for name, rows in tables.items():
            write_csv(out / (prefix + name + ".csv"), rows)
    metadata["completed_pairs"] = len(plan)
    metadata["correctness_failures"] = 0
    (out / (prefix + "metadata.json")).write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"Captured {len(plan)} identical-trace pairs in {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timed", required=True, type=Path)
    parser.add_argument("--diagnostic", required=True, type=Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--smoke", action="store_true")
    mode.add_argument("--campaign", action="store_true")
    parser.add_argument("--output", default="benchmarks/results")
    parser.add_argument("--compiler", default="gcc")
    parser.add_argument("--build-options", default="Release; C17; -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror; benchmarks/tests ON")
    parser.add_argument("--machine", default=platform.machine())
    args = parser.parse_args()
    args.timed = args.timed.resolve(); args.diagnostic = args.diagnostic.resolve()
    try:
        smoke(args) if args.smoke else evidence(args)
    except (ValueError, KeyError, OSError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
