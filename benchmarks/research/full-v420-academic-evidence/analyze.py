"""Integrate preserved prior observations with new native campaign data; no experiments."""
import csv
import hashlib
import json
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
RESULTS = HERE / "results"
SEED = "0x91e10da5c79e7b1d"
SOURCES = [("Windows", "prior", "windows-prior", 384),
           ("Windows", "new", "windows-new", 512), ("Linux", "new", "linux-new", 896)]
all_rows = []
environments = {}
for host, vintage, folder, expected in SOURCES:
    path = RESULTS / folder
    environments[host, vintage] = json.loads((path / "environment.json").read_text())
    rows = list(csv.DictReader((path / "raw.csv").open(newline="")))
    assert len(rows) == expected, (folder, len(rows), expected)
    if vintage == "new":
        exceptions = json.loads((path / "exceptions.json").read_text())
        assert not exceptions["errors"] and not exceptions["skipped"], exceptions
    for row in rows:
        assert row["correctness"] == "OK"
        if vintage == "prior":
            row["seed"] = SEED
            for key in ("create_ms", "populate_ms", "destroy_ms"):
                row[key] = ""
            for old, new in (("baseline_working_set_bytes", "baseline_resident_bytes"),
                             ("peak_working_set_bytes", "peak_resident_bytes"), ("peak_commit_bytes", "peak_private_commit_bytes")):
                row[new] = row[old]
        row.update(host=host, vintage=vintage)
        if host == "Linux":
            # Raw zero is the sampler's unavailable sentinel, not a measurement.
            assert row["peak_private_commit_bytes"] == "0"
            row["peak_private_commit_bytes"] = ""
        all_rows.append(row)

# Eight observations per seed cell: two implementations, one warmup and three repeats.
paired = {}
for row in all_rows:
    key = row["host"], row["scenario"], int(row["n"]), row["seed"]
    paired.setdefault(key, []).append(row)
for key, rows in paired.items():
    assert len(rows) == 8, (key, len(rows))
    assert {(r["implementation"], r["repetition"]) for r in rows} == {(i, str(j)) for i in ("full", "glib") for j in range(4)}
    for field in ("trace_hash", "result_hash", "final_count"):
        assert len({r[field] for r in rows}) == 1, (key, field)
# Cross-host trace and answer identity, independent of machine timing and memory.
cross = {}
for row in all_rows:
    key = row["scenario"], row["n"], row["seed"]
    cross.setdefault(key, []).append(row)
for key, rows in cross.items():
    assert {r["host"] for r in rows} == {"Windows", "Linux"}, key
    assert len({(r["trace_hash"], r["result_hash"], r["final_count"]) for r in rows}) == 1, key

# First median within a seed's three measured repetitions, then median across seeds.
# Preserve seed-level ranges as well as the host/scenario summary; no pooled significance test.
seed_summary = []
for (host, scenario, n, seed), rows in paired.items():
    for impl in ("full", "glib"):
        samples = [r for r in rows if r["implementation"] == impl and r["phase"] == "measured"]
        assert len(samples) == 3
        times = [float(r["elapsed_ms"]) for r in samples]
        row = dict(host=host, scenario=scenario, n=n, seed=seed, implementation=impl,
                   median_ms=statistics.median(times), min_ms=min(times), max_ms=max(times))
        for field in ("create_ms", "populate_ms", "destroy_ms", "peak_resident_bytes", "peak_private_commit_bytes", "baseline_resident_bytes"):
            vals = [float(r[field]) for r in samples if r[field] != ""]
            row[field] = statistics.median(vals) if vals else ""
        row["lifecycle_sum_ms"] = (statistics.median(sum(float(r[k]) for k in ("create_ms", "populate_ms", "elapsed_ms", "destroy_ms")) for r in samples)
                                   if samples[0]["create_ms"] != "" else "")
        seed_summary.append(row)


def write_csv(path, rows):
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


write_csv(RESULTS / "seed-summary.csv", seed_summary)
summary = []
for host in ("Windows", "Linux"):
    for n in (1000, 10000, 100000, 1000000):
        for scenario in dict.fromkeys(r["scenario"] for r in all_rows if r["host"] == host):
            row = dict(host=host, scenario=scenario, n=n)
            for impl in ("full", "glib"):
                group = [r for r in seed_summary if r["host"] == host and r["scenario"] == scenario and r["n"] == n and r["implementation"] == impl]
                assert group
                row[impl + "_seeds"] = len(group)
                row[impl + "_ms"] = statistics.median(r["median_ms"] for r in group)
                row[impl + "_min_ms"] = min(r["min_ms"] for r in group)
                row[impl + "_max_ms"] = max(r["max_ms"] for r in group)
                for field in ("create_ms", "populate_ms", "destroy_ms", "lifecycle_sum_ms", "peak_resident_bytes", "peak_private_commit_bytes", "baseline_resident_bytes"):
                    vals = [r[field] for r in group if r[field] != ""]
                    row[impl + "_" + field] = statistics.median(vals) if vals else ""
            # Lifecycle-only cells have an intentionally empty operation loop.
            row["full_over_glib"] = (row["full_ms"] / row["glib_ms"]
                                     if "lifecycle" not in scenario and row["glib_ms"] > 0 else "")
            summary.append(row)
write_csv(RESULTS / "summary.csv", summary)

new_mixed = [r for r in summary if r["scenario"].startswith("mix_")]
legacy = [r for r in summary if not r["scenario"].startswith("mix_") and "lifecycle" not in r["scenario"]]
life = [r for r in summary if "lifecycle" in r["scenario"]]
lines = ["# Overall native GLib versus Full v4.2.0", "",
         "This report executes the [pinned plan](https://github.com/RXY712200/LayerKeySort/blob/2949501e9523643d5ff5831d74dc124ce17fc5ad/benchmarks/research/full-v420-academic-evidence/PLAN.md). Production baseline: released Full v4.2.0, `f751e1ee2afd682874281d6dc495969f9eca49eb`. No product changes or performance targets are part of this experiment.", "",
         "## Hosts, versions and provenance", ""]
for host, vintage, folder, count in SOURCES:
    env = environments[host, vintage]
    lines.append(f"- **{host}, {vintage}**: GLib **{env.get('glib_runtime')}**, {count} fresh-process samples. Exact compiler, hardware, build commands and source/binary identities: [{folder}/environment.json](results/{folder}/environment.json).")
lines += ["", "Windows runs use a Ryzen 9 9955HX and GCC 16.2.0; GLib is the actual native MSYS2 UCRT64 2.90.1-1 release DLL. Linux runs use a native Ubuntu 24.04 GitHub-hosted runner; its CPU/compiler are recorded in the Linux environment file. GLib 2.90.1 is built from the SHA-256-pinned GNOME source archive, in Meson release mode with `b_ndebug=true`, `glib_debug=disabled`, no LTO. Full and the adapters use C17 `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror` on each host. The MSYS2 package has downstream packaging patches; Linux uses upstream source. Both are actual native GLib 2.90.1, not emulations. Linux build options and Meson log are archived. Absolute speed comparisons between these different machines would be misleading; all ratios below compare implementations within one host.", "",
         "The original Windows data are preserved byte-for-byte from the Git blobs at [`2980792`](https://github.com/RXY712200/LayerKeySort/tree/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420). They were not rerun for this report. New Windows measurements cover only the added mixed traces and lifecycle cells. Linux executes the original twelve scenarios and the new cells. [Prior-data provenance](results/windows-prior/provenance.json) records source paths and hashes.", "",
         "## Protocol and matching", "",
         "All sizes are 1k, 10k, 100k and 1m. Each implementation/size/scenario/seed has one warmup and three measured repetitions in separate processes, with alternating implementation order. The four new stochastic mixed workloads and shuffled managed lifecycle population each use three fixed seeds (`0x91e10da5c79e7b1d`, `0x243f6a8885a308d3`, `0x13198a2e03707344`). Deterministic order lifecycle and the retained legacy scenarios use the original single seed. Tables first take each seed's median of three repetitions, then the median across seeds. Their ranges span all measured observations; [seed-summary.csv](results/seed-summary.csv) retains per-seed medians and ranges. The seeds are generated traces, not collected human interactions. Repetitions are not treated as independent application workloads or used to claim statistical significance.", "",
         "| Added workload | Nominal precedence / move / replace mix | Locality |", "| --- | --- | --- |",
         "| mix_query | 90% / 5% / 5% | Global random live handles |",
         "| mix_balanced | 50% / 25% / 25% | Global random live handles |",
         "| mix_mutation | 10% / 45% / 45% | Global random live handles |",
         "| mix_hotspot | 50% / 25% / 25% | Mutations select from 64 evolving hot occurrences; queries remain global |", "",
         "Each trace has N steps at fixed live population N. A replacement removes one occurrence and appends a new borrowed payload, so it makes two mutation calls. Moves retain occurrence identity and use before-anchor semantics. Every query selects arbitrary currently live pairs after preceding mutations, rather than inferring precedence from initial IDs. Payloads are identical 16-byte key/ID records. An untimed array-based implicit-treap rank oracle supplies expected query signs; independent ID-linked reference updates and complete final traversal validate its resulting sequence. Every actual comparison/removal answer and final occurrence pointer is checked. The oracle is research correctness machinery, not another performance control. Trace hashes and answer/final-order fingerprints match across implementations and hosts.", "",
         "The legacy scenarios retain the original definitions: random-position insertion into empty order, random-permutation removal, random move-before, random-pair precedence on initially unmutated order, the original adjacent-query mixed trace, shuffled unique-key sorted insertion, predecessor/equal/successor lookup with approximately half absent keys, sorted-container removal, and four stable pointer-array sorts. GLib lookup adapts native `g_sequence_search` to the same three-result semantics. Managed keys are unique: duplicate-equal lookup policy is not forced into equivalence. Stable sort checks use independent `(key, original occurrence ID)` total ordering. Sort distributions are random 64-bit keys, strictly reverse keys, ascending keys with N/100 random swaps, and 16 duplicate values. [Original definitions](https://github.com/RXY712200/LayerKeySort/blob/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420/REPORT.md) remain available.", "",
         "Timing covers the public operation loop, including adapters, result writes and Full success-status checks. Generation, payload/oracle preparation, correctness checking and destruction are excluded from operation time. New measurements separately time container creation, initial population and destruction. Lifecycle-only cells time construction/population/cleanup without a second operation campaign. `lifecycle_order` appends N payloads; `managed_lifecycle` populates N shuffled unique keys. Payload allocation and reference-model work remain outside those costs. Sorting has no comparable container lifecycle. Original Windows lifecycle costs were not captured and remain unavailable; unchanged successful-operation observations are reused rather than recreated.", "",
         "## Added mixed workload results", "", "Median ms; brackets are observed min–max across measured repetitions/seeds. F/G is Full time divided by GLib time; below 1 favors Full.", "",
         "| Host | N | Workload | Full ms [range] | GLib ms [range] | F/G |", "| --- | ---: | --- | ---: | ---: | ---: |"]


def time_row(r):
    return f"| {r['host']} | {r['n']:,} | {r['scenario']} | {r['full_ms']:.3f} [{r['full_min_ms']:.3f}–{r['full_max_ms']:.3f}] | {r['glib_ms']:.3f} [{r['glib_min_ms']:.3f}–{r['glib_max_ms']:.3f}] | {r['full_over_glib']:.3f} |"


lines += [time_row(r) for r in new_mixed]
lines += ["", "## Original operations and secondary stable sorting", "",
          "Windows rows below reuse the original observations; Linux rows are new. All four scales and ranges are in [summary.csv](results/summary.csv). The million-element rows are shown here to keep this report concise.", "",
          "| Host | N | Workload | Full ms [range] | GLib ms [range] | F/G |", "| --- | ---: | --- | ---: | ---: | ---: |"]
lines += [time_row(r) for r in legacy if r["n"] == 1000000]
lines += ["", "## Create, populate and destroy", "",
          "Dedicated million-element lifecycle cells below report medians (ms). Full/GLib cells use the same borrowed payloads and no item destructor. Complete phase observations for every new cell/size/seed are in raw CSVs. The lifecycle sum in summary.csv is a median of per-run phase sums (create + populate + operation loop + destroy); it is not whole-process wall time and excludes reference preparation and validation.", "",
          "| Host | Container | Full create | GLib create | Full populate | GLib populate | Full destroy | GLib destroy |", "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
for r in life:
    if r["n"] == 1000000:
        lines.append("| " + r["host"] + " | " + r["scenario"] + " | " + " | ".join(f"{r[i+'_'+m]:.4f}" for m in ("create_ms", "populate_ms", "destroy_ms") for i in ("full", "glib")) + " |")
lines += ["", "## Memory", "",
          "Resident memory is process peak working set on Windows and `getrusage(RUSAGE_SELF).ru_maxrss * 1024` on Linux. Windows also records peak private process commit (`PeakPagefileUsage`); this is not paging I/O. Linux private commit is unavailable and encoded as 0, never interpreted as zero memory consumption. Baseline resident memory is sampled after trace/reference preparation but before container creation. High-water marks include setup and validation, and are sampled before destruction. They include shared libraries, common payload/trace/answer arrays and the oracle. In particular, the new mixed oracle adds common reference memory; do not compare its peaks to the smaller legacy harness as isolated component storage. These are comparable implementation totals within each cell, not requested allocation bytes, portable allocator counters, or exact container-only footprints. A same-harness comparison avoids mixing Full diagnostic allocation bytes with unrelated GLib process metrics.", "",
          "Million-element representative peak resident values (MiB, 1,048,576 bytes); all cells and baselines remain in summary.csv.", "",
          "| Host | Workload | Full peak resident MiB | GLib peak resident MiB |", "| --- | --- | ---: | ---: |"]
for r in summary:
    if r["n"] == 1000000:
        lines.append(f"| {r['host']} | {r['scenario']} | {r['full_peak_resident_bytes']/1048576:.2f} | {r['glib_peak_resident_bytes']/1048576:.2f} |")
lines += ["", "## Observations", ""]
for host in ("Windows", "Linux"):
    mixed = [r for r in new_mixed if r["host"] == host]
    wins = sum(r["full_over_glib"] < 1 for r in mixed)
    lines.append(f"- {host}: Full has a lower median operation-loop time in {wins}/{len(mixed)} added mixed scenario/size cells. At one million elements, F/G ratios are " + ", ".join(f"{r['scenario']} {r['full_over_glib']:.3f}" for r in mixed if r["n"] == 1000000) + ".")
    sort = [r for r in legacy if r["host"] == host and r["scenario"].startswith("sort_")]
    losses = [r for r in sort if r["full_over_glib"] >= 1]
    lines.append(f"- {host}: GLib has the lower median stable-sort time in {len(losses)}/{len(sort)} distribution/size cells. All outcomes are preserved, rather than only favorable Full cells.")
lines += ["", "These observations describe the declared public-API traces on each host. They do not attribute a cause, establish a complexity bound, or support a universal component ranking. Near-equal values and small-N creation timings should be read with their observed ranges, not as robust wins.", "",
          "## Completion, limitations and reproduction", "",
          "The integrated dataset contains 1,792 successful fresh-process samples: 384 retained Windows samples, 512 new Windows samples and 896 new Linux samples. Of these, 1,344 are measured and 448 are warmups. There are no skipped cells, timeouts or failed correctness checks. The analyzer verifies every paired cell and every common trace/result identity across hosts. An additional 224 successful small-scale Windows smoke runs are archived separately and excluded from the comparison. The [execution record](results/execution.json) gives the actual Linux workflow source revision and run/artifact evidence.", "",
          "This is a bounded comparison, not a stress campaign: one physical Windows host and one native Linux hosted-runner environment, three workload seeds for additions, three measured repeats after warmup, successful allocations only, ordinary OS scheduling and no CPU isolation. The two machines differ. Full is linked into the test executable; GLib is dynamically linked. Windows uses the previously verified release package; Linux builds the pinned upstream release. Managed duplicate policy and failure semantics differ (Full returns statuses/failure-atomic contracts; GLib allocation failure can terminate). Snapshots, history, Group/Batch, other controls and downstream applications are outside the equivalent-semantics scope. No per-operation latency distribution or isolated container allocator instrumentation was added: the reused harness measures aggregate loops, and GLib lacks a directly matching public per-container allocation counter. Previous Full diagnostics retain their original scope and are not presented as GLib allocation measurements.", "",
          "See [README.md](README.md) for exact commands and provenance. Production code, APIs, formats, versions, main and releases are unchanged. Technical Debt Impact: **No change**; this adds comparative evidence and does not claim an algorithmic repair or independent acceptance."]
(HERE / "REPORT.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"Validated {len(all_rows)} rows, {len(paired)} host/seed cells, {len(cross)} shared cross-host traces; regenerated report and summaries")
