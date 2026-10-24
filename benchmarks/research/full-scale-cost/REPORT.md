# LayerKeySort Full — large-scale architecture cost research

2026-10-24. Codex engineering observations; independent ChatGPT review pending.
No optimization is implemented or adopted. Full v4.0.0 Stable is unchanged.

## A. Exact baselines

- Remote main: **043872eb5560e867aa7f8c094e0e9dbf2e8a7b95**, fetched before work.
- Full Stable commit: **80004ae732daf696f7e9192e417de0edb0b2aafa**.
- Annotated v4.0.0 tag object: **83d67d3ef5dacaf6117db320d9bbea68b199e23e**.
- Previous consumer: **bf27502f18de5988cd2d1c797ddc3e0ff48d0b03**, branch
  `research/full-comprehensive-consumer-audit`. Its REPORT, PERFORMANCE,
  EVIDENCE, DEPENDENCIES, measurement and allocation source were reviewed.
- New branch: `research/full-million-scale-cost-study`, from current remote main,
  in a separate worktree. Previous branches/research artifacts were preserved.

The sixteen production modules, public header, production manifest and root
CMake match Stable. Frozen 4.x remains 64 functions/14 types, contextual stable
resident handles, copied snapshot bytes, exact LS1/LKS4SNP1 and strict V3 import.
Static source rebuild is supported, not universal cross-toolchain ABI identity.
No other product is evaluated.

## B. Hardware, compiler and memory

Windows x64 **10.0.26300**, AMD Ryzen 9 **9955HX**, 16 cores/32 logical processors,
**16,304,992,256 bytes** physical RAM. GCC **16.2.0** MSYS2 UCRT64, C17 Release
`-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`; CMake **4.4.3**, Python **3.13.14**.
MSVC **19.51.36260.0** additionally validates the research at safe ASan sizes.

Available memory was inspected before each process, not inferred from installed
RAM. Admission estimates were 160N+64 MiB for ordinary live timing,
200N+64 MiB for instrumented live runs and 300N+64 MiB for multi-state history.
These deliberately conservative envelopes include app/model/handles, Full
records/blocks, allocator/header slack, samples and temporary memory. Estimates
must fit within 40% of current available memory and 3 GiB maximum, with 3 GiB
remaining. Children also enforce an observed 2 GiB reserve and time budgets.

The final process ledger preserves actual admission data. Available memory
fell during upper-scale collection; six 10m instrumented attempts were deferred.
All six later ran under the **same** gate after memory recovered. Those original
skipped-attempt records were retained, not overwritten or counted as executions.

## C. Executed scale matrix

See [RESULTS.md](RESULTS.md) and `evidence/runs.json` for every process/status.

| Residents | Production processes | Allocation processes | Structural processes |
|---|---:|---:|---:|
| 100k | 30 | 10 | 10 |
| 1m | 32, including two holdouts | 10 | 10 |
| 3m | 18 | 6 | 3 |
| 10m | 12 | 5 | 2 |

Ten additional 10k warmup processes are excluded from primary summaries.
Thus **158 completed matrix processes**, including warmups, plus **eight**
focused lifetime-churn processes. There were no final failed executions or
time/pressure aborts. Six original admission deferrals were subsequently resolved;
two history-policy omissions remain. History/restore was tested at 100k and 1m,
not 3m/10m. No 10m sanitizer test was attempted. These omissions are not passes.

100k/1m primary profiles have three fresh processes; 3m has two. At 10m append,
random and duplicates have two, other profiles one. Seed 17 is primary;
1m anchor/random holdouts use 712200 and are not pooled into primary statistics.
One complete 10k warmup per family precedes collection; no full 10m warmup is
claimed. Larger repetitions are resource/time-limited and not statistical proof.

## D. Workloads and expected behavior

[README.md](README.md) specifies every operation distribution and exact CLI.
Growth includes append, prepend, fixed-anchor relative insertion and insertion
among recently arrived IDs. The latter is not a guaranteed rank-local insert.
Each explicit-order source then receives local, fixed-region, front/back and
uniform-resident moves; three uniform churn replays and one first-to-anchor
hotspot churn; half deletion by ID parity and regrowth.

The managed families are ascending, descending, 64-bit random, 64-bucket
duplicates and all-equal arrivals. Selected locate queries include both present
keys and absent UINT64_MAX; an independent sorted-ID oracle computes every
returned lower/first-equal/greater result.

Every long-churn process has **100 cycles x 100k pairs**, or **20m mutation API
calls**, with constant 100k or 1m residents. It uses continuous deterministic
random state. Two production repeats plus separate allocation/structural runs
per size expose longer lifetime turnover. These are synthetic engineering
workloads, not user distributions or human adoption evidence.

## E. Independent correctness

The expected explicit order is an array-linked sequence of numeric IDs, not
Full's blocks/AVL/ranks. Model transitions are completed before replay. Full
forward/reverse order, every ID/item/handle, endpoints and counts are compared
at checkpoints; selected comparisons span the collection. Managed reference
sorting explicitly breaks equal keys by arrival ID, independently of Full's
stable insertion implementation. Restore is verified against the complete
expected sequence with new handles.

**848 exact matrix checkpoints** matched. Complete sequence receipts agree
across modes/repetitions for identical scale/profile/seed/checkpoint. The long
campaign adds **96** matching exact checkpoints. Every operation status is
checked; moderate-size oracle logic separately matches 20k Python list
transitions and rejects a deliberately corrupted comparison input.

This is exact **checkpoint** verification, not a check after every million-scale
mutation or proof against transient self-repairing faults. Raw digests are
compact receipts; actual checks compare entries, not hashes alone. Removed
handles are cleared immediately and never dereferenced after expiry.

## F. Timing and distributions

The final constant-density, rotated-offset sampler avoids scale-dependent
overhead and periodic lookup aliasing found in the exploratory pilot. The whole
final matrix was recollected; pilot timing is not substituted into results.
See [METHOD.md](METHOD.md) for overhead, quantile and attribution boundaries.

Representative primary medians, total growth seconds:

| Profile | 100k | 1m | 3m | 10m |
|---|---:|---:|---:|---:|
| Append | 0.006115 | 0.058681 | 0.175323 | 0.611243 |
| Managed random | 0.026931 | 0.804755 | 4.416295 | 23.042261 |
| Managed duplicates | 0.015299 | 0.173094 | 0.543408 | 1.959025 |
| Managed equal | 0.007398 | 0.077261 | 0.243541 | 0.874671 |

All min/median/max totals, sampled p50/p95/p99/max, sample counts, throughput,
phase costs and process wall times remain in raw evidence. At 10m random growth,
the two observations were 22.386 and 23.699 seconds. That repeatable cost is a
useful internal engineering target; it is not a correctness failure.

Phase times exclude oracle preparation/verification and include replay overhead,
status/map work and sparse sampling. API brackets retain timer overhead. Churn
brackets measure a pair, not a single call. Central quantiles near the 0.1 us
clock quantum are resolution-limited. The largest sampled production bracket
in the final matrix was 267.5 us (a 1m anchor-origin churn pair), not a whole-run
worst-case bound. Unsampled rare events and scheduling/allocator causes remain
unresolved. No hard latency or formal amortized theorem follows.

## G. Resident memory, traffic and working set

Requested growth storage scales approximately linearly: append **50.245–50.250
B/resident**; random managed insertion **45.199–45.108 B/resident** over 100k–10m.
At 10m append requests **502,499,480 bytes** in **10,312,500 allocations**;
random requests **451,080,064 bytes** in **10,224,454 allocations**. The latter
uses less requested storage but has much lower insertion throughput.

Separate source inspection explains the ABI-specific append decomposition:
320 MB resident records plus about 182.5 MB blocks and a 64-byte ordinary core.
These are not public struct/layout promises. The diagnostic core adds 4,280 bytes.

10m append production working set at the growth checkpoint was **993,800,192
bytes**; random was **941,383,680**. App payload/map/model alone request 320 MB,
with trace/sampling/runtime costs in addition. The residual cannot be labelled
allocator metadata or fragmentation. Instrumented RSS has extra tracking headers
and is not mixed into those production figures.

In the long campaign, requested live storage at cycles 1/50/100 was:

- 100k: **4,590,568 / 4,598,744 / 4,594,656 bytes**;
- 1m: **45,910,944 / 45,972,264 / 45,969,344 bytes**.

Instrumented working set plateaued at the sampled cycles for each run. Small
live-byte changes track block occupancy, not accumulating expired residents.
All instrumented library teardowns returned requested counts/bytes to zero.
After freeing app fixtures, final production working sets ranged about 8.2–9.7
MB for 10m sources, rather than retaining a source-sized allocation. This is
strong finite cleanup evidence, not a theorem for every allocator or long uptime.

## H. Structural costs and scaling behavior

Append mean occupancy remains near **32/64**; random managed insertion is near
**44.5/64** where directly measured. Append block counts grow from 3,124 at 100k
to 312,499 at 10m, with growth-checkpoint AVL heights 12 and 19 respectively.
Churn reshapes occupancy and changes block count; unchanged resident count does
not imply identical requested bytes. No nonlinear live-coordinate storage exists.

Across diagnostic operations the observed maxima were **160 resident-location
assignments** and **44 distinct index blocks touched**. These are measured
counters with instrumentation, not CPU-store counts or latency bounds. Global
coordinate relabeling is absent; larger wall cost must not be mislabelled as a
return of V3 relabel cliffs.

Random managed per-insert phase cost rose from about 805 ns at 1m to 2,304 ns at
10m, while average comparator calls rose from 18.752 to 22.115. Broad movement
also deteriorated materially; append-origin uniform moves rose from about
117 ns/call at 100k to 1,039 ns at 10m. Endpoint/clustered patterns behave very
differently. Counts alone do not explain elapsed cost. Cache/TLB, payload access,
allocation and background OS load are plausible contributors, **not isolated
causes**. Memory availability changed during collection, and no host isolation
or hardware-counter attribution was performed.

## I. Snapshot and restoration

At 1m, median capture was **17.236 ms**, serialization **2.574 ms**, deserialization
**9.429 ms**, restore **34.873 ms**. A second retained capture was separately
measured; no enormous snapshot bank was retained at 10m.

One wire image has **16,000,040 bytes** for this namespace/eight-byte association
contract. Capture row storage is 16N; association capacity grows geometrically.
The loaded representation owns exact copied associations. The restore phase
makes **1,015,627 allocations**, including N records, ceil(N/64) blocks, one
order and the resolver scratch array. Its 8 MB pointer scratch is transient;
Full's cumulative requested peak in the 1m multi-state scenario was
**172,152,032 bytes**, not process RSS. All histories/restored/source state were
destroyed and counters returned to zero.

More fully packed bulk-restored blocks need less requested live storage than
append-split blocks. That does not eliminate per-resident allocations or make
restoration free. These selected-scale results support investigating shared
resident-allocation costs before treating scratch elimination as the parent fix.

## J. What is new relative to the earlier 100k assessment

The earlier consumer supplied broad workflow evidence, limited large-state
checking and separate requested-byte observations. This assignment adds actual
1m/3m/**10m** execution, exact complete checkpoint sequences at those scales,
constant-density sampling, private occupancy/work counts, lower/upper locate
oracles, longer turnover and a 1m historical-state peak.

The earlier report's production-integration conclusion is not a scalability
guarantee. The new data establish materially different throughput trends and
millions of allocation calls. They justify a stronger **internal optimization
research priority** now, without requiring external adoption first. No old timing
row was presented as newly measured, and changed traces/timer boundaries prevent
a valid simple speedup comparison to the earlier consumer.

## K. Issue relationships and superseded hypotheses

Issues #3 and #4, including all current comments, were reviewed read-only and
remain unchanged. Relationships:

| Current issue family | New disposition/evidence |
|---|---|
| #4 High-1 resident allocation | Quantified to 10m; common parent of live/restore allocation; future prototype justified |
| Medium-1 block slack/work | Direct occupancy and counter evidence; keep distinct from resident allocator costs |
| High-2 / Medium-2 export/history | Actual 1m multi-state costs; owned retained state, not a leak |
| Highest-7 / Medium-11 whole-operation cost | Throughput differs more than comparator counts; retain structural versus allocator/OS distinction |
| Highest-6 / Medium-12 application evidence | Genuine traces still missing; no longer a prerequisite to this owner's engineering-first optimization strategy |
| Medium-3 stale handles / Medium-15 synchronization | Deliberate boundaries respected; no new defense/locking demand inferred |
| Medium-4 portability | New Windows GCC/MSVC-ASan evidence only; broader platforms remain separate validation |
| Low-3 MSVC executable anomaly | Not reproduced in this task; no security changes/workaround introduced |
| V1–V3 Path growth/prefix coupling/equal scans/relabel | Superseded historical mechanisms; do not reopen as V4 defects |

Historic V3 policy/reserve strategies, pre-V4 I/S/F research and the finite
B64/B128/B256 screen are retained in [CANDIDATES.md](CANDIDATES.md), not claimed
as freshly rebenchmarked alternatives. No universal impossibility proof or
universal capacity winner follows from those historical experiments.

## L. Severity, confidence, cost and priority

These are qualitative engineering levels, not fabricated risk scores. Historical
severity remains the issue ledger's lineage, not automatic current urgency.

| Finding | Historical severity | Current impact | Evidence confidence | Implementation cost | Proposed action priority |
|---|---|---|---|---|---|
| Per-resident / restore allocation frequency | High | Medium accepted cost | High for counts; Low for exact metadata attribution | Medium-to-large private refactor | High: bounded compatible prototype |
| Random managed / broad-edit scaling | Highest lineage of cost-model problem | High workload cost | High for observation; Medium/Low for cause | Profiling first; possible Medium/Large refactor | High: causal profiling |
| Occupancy/slack/local writes | Medium | Medium accepted cost | High in measured modes | Small/Medium policy or layout work | Medium, after parent attribution |
| Full capture and retained history | High/Medium | Medium/High at frequent cadence | High at selected scales | Medium/Large depending representation | Medium |
| Restore pointer scratch | Medium restore lineage | Low/Medium memory cost | High | Medium with callback-contract risk | Low after allocation parent |
| Accumulating resident leak | No confirmed new defect | None observed | High within finite runs | No fix justified | None |
| Hard latency / whole-library formal bound | Highest historical | Unspecified application impact | Not established by finite data | Research | Low |
| Breaking V5 redesign | No new breaking defect | None required by evidence | No selected alternative measured | Architectural replacement | None now |

## M. Dependency map

See the diagram and separate parents in [CANDIDATES.md](CANDIDATES.md): stable
resident allocation jointly influences insertion/restore/destruction; blocks
govern occupancy/local writes/index traversal; snapshots govern copied history;
resolve-first semantics govern scratch/callback sequencing. No single verified
root cause subsumes every memory/latency observation.

## N. Candidate tradeoffs and compatibility

The candidate report covers stable-address pools/slabs, other resident stores,
occupancy/capacity, cache-conscious block/index alternatives, restore scratch,
and snapshot storage. For each it states benefit/evidence, joint/non-addressed
problems, introduced costs, implementation cost, stable handles, atomicity,
snapshot/wire semantics and source-compatibility consequences. No candidate is
benchmarked unfairly with required semantics removed.

Private changes could remain inside 4.x. Moving live records, changing handle
expiry, weakening prepare/commit, reinterpreting LS1/LKS4SNP1 or changing strict
V3 migration is not silently compatible. Streaming resolve/build can alter the
documented resolve-first sequencing and callback side effects; it requires
explicit contract review. No evidence currently requires a new major version.

## O. Recommended future order

1. Independently review this evidence and its oracles/limits.
2. Prototype stable resident allocation/storage, jointly measuring live growth,
   churn, restore, destruction and retained memory under identical semantics.
3. Profile large random managed/broad-move behavior; distinguish library layout,
   caller payload access, allocator and OS effects before blaming one mechanism.
4. Revisit block/index/capacity only with that attribution; avoid disposable
   child micro-optimizations while parent storage may change.
5. Investigate snapshot capacity/copy and restore scratch on their own measured
   merits, preserving callbacks, historical ownership and frozen formats.
6. Require full correctness, OOM/lifetime and compatibility validation before
   adoption. No optimization or production modification starts in this task.

## P. Changed files and provenance

Only `benchmarks/research/full-scale-cost/` is added. Its standalone CMake,
public drivers/oracle, isolated private adapter, allocation wrapper, collectors,
analysis, reports and compact evidence are listed in [PROVENANCE.md](PROVENANCE.md).
Production/header/root CMake/CI/existing tests/fixtures and other product files
are unchanged. SHA-256 manifests cover committed evidence and separately retained
local raw artifacts. No generated binary, cache or enormous raw trace is committed.

## Q. Git delivery

The final delivery response records exact local/remote research SHA and clean
status; a report cannot embed its own commit hash recursively. The branch is
committed/pushed without merging main, opening a PR, changing Issues or publishing
a tag/Release. Main and Stable refs are rechecked after delivery.

## R. Reproduction

[README.md](README.md) gives exact configure/build/test and sequential collector
commands. Use fresh output directories for changed source/binaries. All commands,
seeds, repetitions, estimates, admission data and statuses are in the compact run
ledger; source and binary provenance is recorded separately. Do not merge pilot,
instrumented or holdout timing into primary summary statistics.

## S. Missing evidence and interpretation limits

- No actual candidate optimization, alternative index comparison or V5 design
  is implemented. Expected benefits are proposals, not measured wins.
- No external adoption/human trace or independent acceptance is asserted.
- No fresh Linux/macOS/Win32 or GCC ASan/UBSan result; MSVC ASan is safe-size
  verification, not a 10m sanitizer campaign or leak theorem.
- No direct allocator metadata/fragmentation, hardware-counter causal attribution,
  continuously sampled peak working set, days-long uptime or hard latency bound.
- Rare unsampled operation tails and transient between-checkpoint faults remain
  possible. Upper repeats are limited, host activity is uncontrolled, and memory
  pressure changed during collection. Safety deferrals are preserved explicitly.
- 3m/10m multi-state history and 10m sanitizer were deliberately omitted; no
  inability of Full at those scenarios is inferred from the chosen policy.
- Existing API/wire/OOM regressions complement these measurements; future
  storage/layout changes require fresh fault-injection and compatibility review.

LayerKeySort Full large-scale cost research complete. Awaiting independent ChatGPT review and the user's optimization decisions.
