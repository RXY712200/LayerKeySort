"""Reproduce the comparison tables from captured samples, without rerunning workloads."""
import csv
import json
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
RESULTS = HERE / "results"
raw = list(csv.DictReader((RESULTS / "raw.csv").open(newline="")))
errors = json.loads((RESULTS / "exceptions.json").read_text())
assert not errors["errors"] and not errors["skipped"]
assert len(raw) == 384 and sum(r["phase"] == "measured" for r in raw) == 288
groups = {}
for r in raw:
    assert r["correctness"] == "OK"
    groups.setdefault((r["scenario"], int(r["n"])), {}).setdefault(r["implementation"], []).append(r)
for pair in groups.values():
    assert set(pair) == {"full", "glib"}
    for rows in pair.values():
        assert len(rows) == 4 and {r["repetition"] for r in rows} == {"0", "1", "2", "3"}
    for key in ("trace_hash", "result_hash", "final_count"):
        assert len({r[key] for rows in pair.values() for r in rows}) == 1

scenarios = list(dict.fromkeys(r["scenario"] for r in raw))
comparisons = []
for n in (1000, 10000, 100000, 1000000):
    for scenario in scenarios:
        row = dict(scenario=scenario, n=n)
        for impl in ("full", "glib"):
            samples = [r for r in groups[scenario, n][impl] if r["phase"] == "measured"]
            times = [float(r["elapsed_ms"]) for r in samples]
            row[impl + "_median_ms"] = statistics.median(times)
            row[impl + "_min_ms"] = min(times)
            row[impl + "_max_ms"] = max(times)
            for metric in ("peak_working_set", "peak_commit", "baseline_working_set"):
                row[impl + "_" + metric + "_MiB"] = statistics.median(float(r[metric + "_bytes"]) for r in samples) / 1048576
        row["full_over_glib_time"] = row["full_median_ms"] / row["glib_median_ms"]
        comparisons.append(row)
with (RESULTS / "comparisons.csv").open("w", newline="") as f:
    writer = csv.DictWriter(f, list(comparisons[0]))
    writer.writeheader()
    writer.writerows(comparisons)

text = """# Native GLib / Full v4.2.0 comparison

## Scope and environment

One native Windows 11 x64 campaign on an AMD Ryzen 9 9955HX, using actual **GLib 2.90.1** (`mingw-w64-ucrt-x86_64-glib2` 2.90.1-1) and released **Full v4.2.0**, source commit `f751e1ee2afd682874281d6dc495969f9eca49eb`. The research branch starts at `9a7f11eb204bc3cedcf9cc1904af6af0a075987a`; its production source, public headers and canonical source manifest are byte-identical to that release. This is an initial native-platform comparison, with no algorithm modifications or root-cause investigation.

GCC 16.2.0 compiles Full and both adapters together using `-std=c17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`, without LTO. GLib is the native MSYS2 release DLL, not a replacement implementation. Its archived build metadata identifies GCC 16.2.0-4, `!debug` and `!lto`. The exact matching packaging recipe selects Meson `--buildtype=release` and disables `glib_debug`. Thus both sides use release optimization; the packaged GLib compiler command line is not overridden by the harness. Full is linked into the executable and GLib is dynamically linked, as recorded in the build command. Both modes load the same executable and DLLs. Exact package, DLL, executable and harness SHA-256 values are in [environment.json](results/environment.json). The GLib recipe provenance and native dependency hashes are in [dependency-provenance.json](results/dependency-provenance.json).

## Protocol and semantic matching

Each implementation/scenario/size cell has one untimed-for-summary warmup run followed by three measured repetitions, each in a fresh process. Warmup warms the executable/OS rather than preserving one container or allocator. Implementation order alternates by repetition (Full then GLib for warmup and repetition 2; GLib then Full for repetitions 1 and 3). A deterministic xorshift64 seed `0x91e10da5c79e7b1d` generates the same trace and payloads in both modes. There is one trace seed per condition, not three independent workload seeds. The process runs serially with normal scheduling, without CPU affinity or machine isolation.

Timing uses `QueryPerformanceCounter` around the API loop only. Trace generation, oracle construction, initial container population (except insertion workloads), correctness validation and destruction are outside that interval. Payloads are identical 16-byte `{uint64_t key, id}` records borrowed through pointers. Trace lookup costs, comparator calls, native API adaptation, Full success-status checks and result writes needed by the operation are included. No comparator counters or allocation instrumentation are enabled.

| Scenario | Timed work at size N | Native APIs / matching behavior |
| --- | --- | --- |
| insert | N insertions into an initially empty sequence; insert before a uniformly selected existing occurrence | `lks_order_insert_before/back`; `g_sequence_insert_before` |
| remove | Random-permutation removal of all N initial occurrences | `lks_order_remove`; `g_sequence_get` plus `g_sequence_remove`, returning/checking the removed item in both adapters |
| move | N random source/anchor moves within N live occurrences | `lks_order_move_before`; `g_sequence_move`; identical before-anchor semantics, self-moves allowed |
| compare | N random pairs from the initial ordered sequence | `lks_order_compare`; `g_sequence_iter_compare`; signs checked against original ID order |
| mixed | N trace steps: 25% remove-and-append replacement, 50% random move-before, 25% precedence | Fixed live population N; a replacement step makes two mutation calls, so this is 1.25N logical API operations. Precedence pairs are current adjacent occurrences (or self at the end), unlike the standalone random-pair workload |
| managed_insert | N shuffled unique even-key insertions | `lks_managed_order_insert`; `g_sequence_insert_sorted` |
| managed_lookup | N random keys in 0..2N+1, approximately half present | `lks_managed_order_locate`; `g_sequence_search` plus iterator adaptation to return predecessor/equal/successor. Unique keys make the three-result semantics equivalent |
| managed_remove | Random-permutation removal of all N sorted occurrences | Managed removal vs iterator removal, both checking the borrowed removed item |
| sort_random | One stable pointer-array sort of N random 64-bit keys | `lks_sort`; native `g_sort_array` |
| sort_reverse | One sort of strictly descending keys | Same stable array APIs |
| sort_nearly | Initially ascending, then N/100 random pair swaps (positions can repeat) | Same stable array APIs |
| sort_duplicates | One sort of uniformly generated keys in 0..15 | Same stable array APIs; original ID order must survive within ties |

The sequence oracle is an independent flat ID-linked model built before timing, not another ordered-tree implementation. Every final occurrence pointer and traversal endpoint is checked. Removed-item results and every precedence/lookup answer are checked. Sort output is compared pointer-for-pointer against C `qsort` using the total `(key, original ID)` ordering, independently encoding stable ties. Raw trace/result fingerprints match across implementations and repetitions. This validates the exercised successful operations, not allocation-failure equivalence.

The APIs have broader differences: Full supplies failure statuses and failure-atomic contracts; GLib allocation failure commonly terminates the process. This campaign stays within successful memory admission. It does not compare those failure policies, Full snapshots/history/persistence, GLib rank/index APIs, or duplicate-equal managed lookup semantics. No snapshots or OrdWeave are involved. Stable occurrence handles/iterators and borrowed payloads are retained consistently across the selected operations; removed occurrences are never reused.

## Timing results

All **384 runs completed**, including **288 measured samples** and 96 warmups. No failed oracle, timeout or memory-admission skip occurred. Each number is median milliseconds, with the observed minimum–maximum of the three measured repetitions. Ratio is Full / GLib; below 1 favors Full. Small samples have substantial timer/scheduler sensitivity. No significance or asymptotic claim is inferred from three repetitions.

| N | Workload | Full ms [min–max] | GLib ms [min–max] | Full / GLib |
| ---: | --- | ---: | ---: | ---: |
"""
for r in comparisons:
    text += f"| {r['n']:,} | {r['scenario']} | {r['full_median_ms']:.3f} [{r['full_min_ms']:.3f}–{r['full_max_ms']:.3f}] | {r['glib_median_ms']:.3f} [{r['glib_min_ms']:.3f}–{r['glib_max_ms']:.3f}] | {r['full_over_glib_time']:.3f} |\n"
text += """
## Comparable process-memory results

`GetProcessMemoryInfo` records **PeakWorkingSetSize** (peak resident working set) and **PeakPagefileUsage** (peak private process commit charge, not observed paging I/O). The baseline working set is sampled after common trace/payload/oracle preparation and before container creation. Peaks are lifetime process high-water marks sampled after validation, including container setup and validation. They cannot isolate the timed operation or establish requested allocation bytes. The common executable/DLLs, payloads, handles, trace, answers and independent oracle arrays are included; this deliberately uses an identical harness in both modes rather than presenting unequal allocator counters as comparable memory. Common harness storage is large and can dominate array-sort measurements. Subtracting baseline working set from a process high-water mark does not recover exact component allocation.

All memory values below are medians of three runs, in MiB (1,048,576 bytes), for N=1,000,000. Raw bytes and every other scale are retained in [raw.csv](results/raw.csv), [summary.csv](results/summary.csv) and [comparisons.csv](results/comparisons.csv).

| Workload | Full peak resident MiB | GLib peak resident MiB | Full peak commit MiB | GLib peak commit MiB | Full baseline resident MiB | GLib baseline resident MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
"""
for r in comparisons:
    if r["n"] == 1000000:
        text += "| " + r["scenario"] + " | " + " | ".join(f"{r[impl + '_' + metric + '_MiB']:.2f}" for metric in ("peak_working_set", "peak_commit", "baseline_working_set") for impl in ("full", "glib")) + " |\n"
text += """
## Findings and limits

At one million elements Full has lower median elapsed time in all five explicit-order scenarios and all three managed-order scenarios. The measured random-pair precedence loop takes 32.354 ms for Full and 903.995 ms for GLib. These are one million public comparisons on the stated initial sequence, not a throughput claim for arbitrary application activity.

Sorting gives a mixed result. At one million elements Full takes 121.663 ms versus GLib's 138.624 ms on random input, but GLib is faster on reverse (22.563 vs 26.695 ms), nearly sorted (30.093 vs 36.989 ms), and duplicate-heavy input (55.712 vs 61.057 ms). All smaller-scale results, including unfavorable cells, remain in the table. The report does not diagnose causes or propose algorithm changes.

Process memory does not show a universal winner: for the million-item move/compare cases Full's resident peak is about 209.8 MiB versus GLib's 206.7 MiB, while insertion has a slightly lower Full peak. Array-sort resident peaks are nearly identical because the common harness dominates that metric; Full's peak process commit is about 177.5 MiB versus GLib's 169.9 MiB. These process totals must not be relabeled as per-container or requested-byte measurements.

This bounded experiment covers native Windows, one machine, one seed, unique-key managed operations and successful allocations. It supplies reproducible evidence for these workload definitions, rather than a general ranking. A 96 MiB + 256*N estimate had to fit within half available physical memory before each child run; none was skipped. Each child had a 180-second timeout. Normal background scheduling remains an uncontrolled influence, visible in the reported ranges. Full production, public contracts, version metadata, main and releases are unchanged. Technical Debt Impact: additional comparative evidence only; no existing debt is claimed resolved.

## Evidence and reproduction

See [README.md](README.md) for commands, [benchmark.c](benchmark.c) for the native adapters/oracles, [run.py](run.py) for acquisition/build/admission/execution and [analyze.py](analyze.py) for the paired-data checks and tables. [exceptions.json](results/exceptions.json) preserves the empty failure/skip lists. `results/glib.BUILDINFO.txt` and `results/glib.PKGINFO.txt` preserve the upstream package's compiler, dependencies and LGPL-2.1-or-later declaration. GLib binaries are not redistributed in this branch.
"""
(HERE / "REPORT.md").write_text(text, encoding="utf-8")
print("Validated 384 rows, 48 paired cells; generated comparisons.csv and REPORT.md")
