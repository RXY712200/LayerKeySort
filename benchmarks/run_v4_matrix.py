"""Finite Preview.4 evidence collection; sequential child processes, no tuning.

Oracle failures abort before recording a row. JSONL retains per-run data, CSVs
provide focused views. Timing builds exclude allocator/work instrumentation.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def run_matrix(build, output, smoke=False):
    output.mkdir(parents=True, exist_ok=True)
    rows, details, group_rows = [], [], []
    suffix = ".exe" if os.name == "nt" else ""
    raw = output / "runs.jsonl"
    if raw.exists():
        raise RuntimeError("Refusing to overwrite captured evidence")

    def invoke(target, arguments, collection):
        command = [str(build / (target + suffix)), *map(str, arguments)]
        result = subprocess.run(command, text=True, capture_output=True, check=True)
        decoded = [json.loads(line) for line in result.stdout.splitlines()]
        if not decoded or any(row.get("oracle") is False for row in decoded):
            raise RuntimeError("No valid oracle evidence")
        with raw.open("a", encoding="utf-8", newline="\n") as stream:
            for row in decoded:
                row["target"] = target
                stream.write(json.dumps(row, sort_keys=True) + "\n")
                (details if row.get("phase_details") or row.get("reference_details") else collection).append(row)
        print(target, *arguments, "PASS", flush=True)

    families = ["ascending", "descending", "random", "equal", "duplicates", "alternating",
                "hotspot", "timeline", "priority", "local", "churn", "large_timeline",
                "local_drag", "front_back", "distant_drag", "hotspot_drag", "large_moves",
                "delete_reinsert", "managed_churn"]
    insert_only = set(families[:7] + ["priority"])
    scales = [512] if smoke else [1024, 10000, 100000]
    repetitions = 1 if smoke else 3
    # Warmups are discarded; each measured replay itself must pass the oracle.
    if not smoke:
        for family in families:
            subprocess.run([str(build / ("layerkeysort_v4_trace" + suffix)), family,
                            "128", "1024", "0", "7", "-1"], check=True, stdout=subprocess.DEVNULL)
    for n in scales:
        for family in families:
            initial = 0 if family in insert_only else n
            for repeat in range(repetitions):
                invoke("layerkeysort_v4_trace", [family, initial, n, 0, 7, repeat], rows)
            invoke("layerkeysort_v4_trace_diagnostics", [family, initial, n, 0, 7, 0], rows)
    if not smoke:
        for family, initial, steps in [("endpoint", 0, 1000000), ("equal", 0, 1000000),
                                       ("long_churn", 10000, 1000000),
                                       ("distant_drag", 100001, 100000), ("retained", 100000, 10000)]:
            for repeat in range(repetitions):
                invoke("layerkeysort_v4_trace", [family, initial, steps, 0, 19, repeat], rows)
            invoke("layerkeysort_v4_trace_diagnostics", [family, initial, steps, 0, 19, 0], rows)
    for family in ["timeline", "churn", "distant_drag"]:
        for cadence in [0, 10000, 1000, 100]:
            n = 512 if smoke else 10000
            for repeat in range(repetitions):
                invoke("layerkeysort_v4_trace", [family, n, n, cadence, 7, repeat], rows)
            invoke("layerkeysort_v4_trace_diagnostics", [family, n, n, cadence, 7, 0], rows)
    # Fair comparisons: identical key sequence, stable upper bound, identical
    # first-equal deletion in managed churn. No synthetic V3 handle moves.
    for family in ["ascending", "descending", "random", "equal", "duplicates", "alternating", "priority", "managed_churn"]:
        for n in ([512] if smoke else [10000, 100000]):
            for repeat in range(repetitions):
                invoke("layerkeysort_v4_trace", ["V3_" + family, n if family == "managed_churn" else 0, n, 0, 7, repeat], rows)
            invoke("layerkeysort_v4_trace_diagnostics", ["V3_" + family, n if family == "managed_churn" else 0, n, 0, 7, 0], rows)
    for capacity in [64, 128, 256]:
        for family in ["endpoint", "random", "local_drag", "distant_drag", "churn", "equal", "duplicates", "timeline"]:
            n = 512 if smoke else 10000
            initial = 0 if family in ["endpoint", "random", "equal", "duplicates"] else n
            for repeat in range(repetitions):
                invoke(f"lks_capacity_{capacity}_timed_trace", [family, initial, n, 0, 7, repeat], rows)
            invoke(f"lks_capacity_{capacity}_diagnostic_trace", [family, initial, n, 0, 7, 0], rows)
    for n in ([512] if smoke else [1024, 10000, 100000]):
        for equal in [0, 1]:
            for repeat in range(repetitions):
                invoke("layerkeysort_v4_groups", [n, equal, repeat], group_rows)
            invoke("layerkeysort_v4_groups_diagnostics", [n, equal, 0], group_rows)

    def write_csv(name, source, keys=None):
        if not source:
            return
        keys = keys or sorted({key for row in source for key in row})
        with (output / name).open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, keys, extrasaction="ignore")
            writer.writeheader()
            writer.writerows(source)

    write_csv("workloads.csv", rows)
    write_csv("phases-and-reference.csv", details)
    write_csv("groups.csv", group_rows)
    identity = ["family", "initial", "steps", "seed", "run", "capacity", "diagnostic", "cadence", "target"]
    write_csv("latency.csv", rows, identity + ["mutation_ms", "p50_us", "p95_us", "p99_us", "p999_us", "max_us", "query_ms", "cleanup_ms"])
    write_csv("maintenance.csv", [r for r in rows if r["diagnostic"]], identity + ["assignments", "local_arrays", "index_writes", "splits", "source_repairs", "merges", "rotations", "noops", "max_assignments", "max_arrays", "max_index", "max_splits", "max_repairs", "comparisons"])
    write_csv("allocations.csv", [r for r in rows if r["diagnostic"]], identity + ["allocations", "allocation_bytes", "frees", "peak_requested", "built_requested", "churn_requested", "small_requested", "empty_container_requested", "cleanup_live_bytes", "cleanup_live_blocks"])
    write_csv("storage.csv", rows, identity + ["rss_start", "rss_built", "rss_churn", "rss_small", "rss_empty", "rss_retained", "rss_released", "rss_final", "snapshot_peak", "snapshot_row_bytes", "association_capacity_bytes"])
    write_csv("exports.csv", [r for r in rows if r["cadence"] or r["family"] == "retained"], identity + ["mutation_ms", "capture_ms", "format_ms", "serialize_ms", "load_ms", "restore_ms", "snapshot_count", "snapshot_peak", "wire_bytes", "key_bytes"])
    write_csv("capacity-screen.csv", [r for r in rows if r["target"].startswith("lks_capacity_")])
    source_files = [*sorted((ROOT / "src").glob("*.[ch]")), ROOT / "include/layerkeysort.h", Path(__file__), ROOT / "benchmarks/v4_trace.c", ROOT / "benchmarks/v4_groups.c"]
    metadata = {"base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "measured_state": "working tree; exact SHA-256 file digests below (publication commit also contains documentation/evidence)",
                "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files},
                "os": platform.platform(), "machine": platform.machine(), "processor": platform.processor(),
                "compiler": subprocess.check_output(["D:/msys64/ucrt64/bin/gcc.exe", "--version"], text=True).splitlines()[0] if os.name == "nt" else os.environ.get("CC", "cc"),
                "build": str(build), "flags": "Release -O3 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror",
                "timer": "QueryPerformanceCounter" if os.name == "nt" else "clock_gettime(CLOCK_MONOTONIC)",
                "runs": repetitions, "seeds": [7, 19], "association_bytes": 8,
                "aggregation": "unaggregated per-run rows; summarize production timing with median of three runs; diagnostics are separate",
                "collection_time": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "tuning": "untuned baseline"}
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    (output / "validation.json").write_text(json.dumps({"oracle": True, "rows": len(rows), "group_rows": len(group_rows), "reference_and_phase_rows": len(details), "cleanup_zero": all(r["cleanup_live_bytes"] == r["cleanup_live_blocks"] == 0 for r in rows)}, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    run_matrix(args.build.resolve(), args.output.resolve(), args.smoke)
