# Full Live-Order Step 1 — implementation and evidence report

2026-10-24. **IMPLEMENTATION READY FOR INDEPENDENT REVIEW**. This is a compatible
private candidate, not final project acceptance, adoption, merge or a release.

## 1. Immutable baselines and scope

- Main: `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`.
- Full Stable: `80004ae732daf696f7e9192e417de0edb0b2aafa`; annotated tag object
  `83d67d3ef5dacaf6117db320d9bbea68b199e23e`.
- [Million-scale study](https://github.com/RXY712200/LayerKeySort/tree/d9d3b370fb8daf134eadc26c64e3b30aae12ec75/benchmarks/research/full-scale-cost):
  `d9d3b370fb8daf134eadc26c64e3b30aae12ec75`.
- [Independent consumer evaluation](https://github.com/RXY712200/LayerKeySort/tree/bf27502f18de5988cd2d1c797ddc3e0ff48d0b03/examples/full-consumer-evaluation):
  `bf27502f18de5988cd2d1c797ddc3e0ff48d0b03`.
- Candidate: enclosing commit on `optimization/full-live-core-step1`; exact SHA
  is delivered after commit/push. Source checksums are in `evidence/`.

Fetched main and reviewed Full source, frozen compatibility, tests, historical
V4 decisions, both studies and Issues3/4. The newest engineering-first Issue 3
roadmap governs. Stable, main, Mini, prior evidence, Issues, governance, API,
version and wire fixtures are unchanged. No PR/tag/release/merge is authorized
by this delivery. Snapshot/history redesign and final integration remain out
of scope.

## 2. Root cause and selected representation

One proven avoidable cost is duplicate owner metadata: every resident stores
an owner already determined by its containing block. All residents in a block
belong to one core, and legal moves never transfer a resident to a different
core. Share this immutable owner pointer at block granularity.

On this x64 build, a record falls **32 ->24 bytes**, a block grows **584 ->592
bytes**, and the production core remains 64 bytes. With N residents in K blocks,
the requested resident/block saving is **8(N-K) bytes**. Individual records still
have stable addresses and are freed promptly on removal. B64/min32, AVL, threaded
blocks, split/merge/redistribution and operation algorithms are retained.

Actual production edits are limited to three files: private layout,
owner initialization in ordinary/bulk construction, owner lookup and diagnostic
validation. No extra allocation, callback or fallible stage is introduced.
Prepare/commit boundaries, allocation count/sequence and resolve-first restoration
are preserved. During mutation, callback-free location updates temporarily move
records between same-owner blocks before the public operation returns.

This does **not** prove the entire random-insert slowdown was allocator overhead.
Comparator/payload reads, traversal, allocation size classes, cache/TLB effects,
fragmentation and scheduling remain partly unattributed. Exact comparator and
structural work counts agree in 63 paired diagnostic phases; timing differences
cannot be credited to fewer comparisons or a different algorithm.

## 3. Bounded alternatives

1. **Compact separately allocated residents: selected.** Full implementation,
   unchanged allocation failure points, direct reclamation, broad validated memory
   reduction, mostly favorable timing and low maintenance cost.
2. **Stable-address slabs: rejected by an actual reclamation prototype screen.**
   Records are24 bytes; empty slabs are immediately freed. At one million
   initial records, after99% shuffled deletion, retained record/slab bytes are:

   | Slots/slab | Allocations on growth | Bytes at full size | Bytes at 1% live |
   |---|---:|---:|---:|
   |16|62,500|27,500,000|4,094,640|
   |64|15,625|24,875,000|11,812,640|
   |256|3,907|24,223,400|22,369,600|

   Individually allocated compact **records alone** require240,000 requested
   bytes at 10,000 live residents. The slab screen includes its headers but not
   Full blocks; it exposes pinned sparse-page retention. Reclamation reaches
   zero after all residents die. This is deliberately **not** a complete pooled
   Full core, a pool throughput result, or pool OOM validation. It rejects these
   fixed-capacity policies before introducing their production complexity; it
   does not prove that every possible adaptive pool is inferior.
3. **Block/index replacement and B64 retuning: not implemented.** Measured
   structural work remains bounded and the narrow metadata change already
   improves the shared storage parent. Evidence does not isolate an AVL or
   occupancy replacement that would justify greater complexity. No speculative
   architecture or subsequent tuning campaign was started.

## 4. Environment and measurement controls

Windows 11 build 26300 x64, Ryzen 9 9955HX (16C/32T),16,304,992,256 bytes physical
RAM; GCC 16.2.0 UCRT64, C17 Release `-O3 -DNDEBUG`, strict driver warnings;
CMake 4.4.3 and Python 3.13.14. Public regression builds use
`-Wall -Wextra -Wpedantic -Werror` for production as well.

Both variants use the same copied immutable driver, oracle, deterministic seed17,
operation sequences, sample strata and compiler configuration. Baseline production
sources/headers/manifest match main and Stable byte-for-byte. Production,
allocation-wrapper and private diagnostic processes are separate. Application
items/maps/oracle/trace are excluded from Full requests and included in process
working set. Wrapper/alignment headers and diagnostic core size perturb memory;
those timings are not production comparisons.

An exploratory matrix overlapped compilation and is retained only as exploratory
evidence. The **confirmation** campaign runs after builds/tests finish, with no
concurrent benchmark children, alternating baseline/candidate order. Ordinary
Windows background activity is not controlled; this is not a confidence-interval
study or a proven cache-miss attribution. The post-build long campaign and a
single bounded follow-up set are separately labeled.

Confirmation:148 completed fresh processes,44 memory-admission skips; two
production repeats per workload at 100k/1m/3m, plus dedicated allocation/structural
runs. All planned10m runs were safely skipped: available RAM did not satisfy
both the envelope and 3GiB reserve. No unrelated application was terminated and
no threshold was relaxed. The previous baseline-only10m results are **not**
substituted for a fresh candidate comparison. History is capped at 1m.

All comparison rows, observed quantiles, allocation traffic, working set,
occupancy and structural counters are delivered, including unfavorable rows.
Phase wall time includes driver/status/sampling overhead, excludes expected-model
preparation. A churn pair sample brackets two calls. Timer quantization can yield
zero short samples. No observed maximum is a worst-case guarantee.

## 5. Fresh growth timing

Seconds, **baseline / candidate**, median of two fresh processes. Definitions
are in [README](README.md); duplicates use 64 keys. These tables use confirmation
only and include every growth workload.

| Workload |100k|1m|3m|
|---|---:|---:|---:|
|append|0.006997 / 0.005635|0.064126 / 0.058705|0.195649 / 0.187498|
|prepend|0.006263 / 0.006099|0.066725 / 0.060425|0.197886 / 0.184642|
|anchor|0.007123 / 0.006518|0.072827 / 0.067056|0.216807 / 0.212152|
|local|0.008751 / 0.008841|0.088333 / 0.088121|0.264763 / 0.247009|
|ascending|0.007722 / 0.007096|0.090134 / 0.080119|0.288873 / 0.243268|
|descending|0.008925 / 0.008320|0.099350 / 0.096359|0.290802 / 0.294421|
|random|0.030209 / 0.028052|0.904699 / 0.763983|4.849096 / 4.511197|
|duplicates|0.016833 / 0.015529|0.193230 / 0.176523|0.596401 / 0.566106|
|equal|0.008194 / 0.006884|0.088261 / 0.083049|0.267698 / 0.245187|

Random insertion improves 7.1%,15.6%,7.0% in these respective samples. It still
slows substantially with scale; this is partial improvement, not elimination
of the search/locality cost. Three-million descending growth regresses 1.2%.
Recent-arrival local insertion at 100k regresses 1.0%; all rows remain visible.

## 6. Moves, turnover, restoration and destruction

Append-origin phase seconds, baseline / candidate; edits use up to 200k calls
(or pairs, as indicated by raw `operations`). Local/hotspot and broad workloads
remain distinct; no convenient endpoint-only substitution is made.

| Phase |100k|1m|3m|
|---|---:|---:|---:|
|local_move|0.002034 / 0.002148|0.026357 / 0.026533|0.036735 / 0.037447|
|hotspot_move|0.010597 / 0.010488|0.099082 / 0.085919|0.147554 / 0.136973|
|front_back_move|0.012264 / 0.011056|0.094746 / 0.084213|0.126937 / 0.113843|
|uniform_move|0.012578 / 0.011906|0.140171 / 0.125202|0.181301 / 0.172004|
|uniform_churn_1|0.020480 / 0.019991|0.208620 / 0.180414|0.256164 / 0.244272|
|hotspot_churn|0.014481 / 0.015454|0.035962 / 0.035076|0.038572 / 0.040089|
|half_delete|0.003546 / 0.003533|0.106696 / 0.081355|0.247951 / 0.211909|
|regrow|0.001873 / 0.001849|0.033058 / 0.023999|0.088907 / 0.075069|
|library_destroy|0.001850 / 0.001518|0.053076 / 0.044448|0.170735 / 0.138844|

All prepend/anchor/local-origin edit rows are also in `results/confirmation/`.
At 1m, uniform moves improve10.7%, hotspot moves13.3%, local moves regress0.7%.
At 3m, uniform moves improve5.1%, local moves regress1.9%. These do not remove
rank-index or local pointer-update work.

Fresh restore at 1m: **0.043269 ->0.036353 seconds** (16.0% lower). Allocation
calls remain **1,015,627**; requested allocation traffic is
**49,125,064 ->41,250,064 bytes**. Resolve-first scratch remains 8MB. The measured
multi-state peak is **172,152,032 ->156,527,024 bytes** because source and restored
resident records shrink; snapshot/wire representation is unchanged.

The supplemental diagnostic shrink campaign includes 32 complete forward/reverse
scans over 1m residents. Median scan time: **1.081549 ->1.074439 seconds** across
two fresh processes each. This is essentially unchanged (0.7% lower), not a
production-timing claim: both sides use the same diagnostic library configuration.
Teardown remains linear; every instrumented matrix/long/shrink process reaches
zero tracked Full allocations after destruction.

## 7. Memory, allocation and occupancy

Full requested live bytes from allocation-only runs; allocation counts are equal
between variants, not improvements. Production working set includes application
state and runtime.

|Workload|Residents|Baseline bytes|Candidate bytes|Growth allocations, either variant|
|---|---:|---:|---:|---:|
|append|100,000|5,024,480|4,249,472|103,125|
|append|1,000,000|50,249,480|42,499,472|1,031,250|
|append|3,000,000|150,749,480|127,499,472|3,093,750|
|random|100,000|4,519,936|3,738,016|102,262|
|random|1,000,000|45,109,728|37,289,312|1,022,450|
|random|3,000,000|135,323,152|111,861,824|3,067,336|

Append requests fall15.4%; random requests fall17.3% at 1m. Every corresponding
allocation/free count and comparison sequence remains unchanged. Requested
allocation traffic falls because each resident request is smaller, not because
calls disappear. There is no pool utilization metric for the selected design.

Median production append working set at 3m is 301,938,688 ->255,309,824 bytes;
random is 286,185,472 ->239,069,184. These are whole-process observations, not
Full-only allocator sizes. App/model memory is unchanged. Occupancy, block
count/height, rotations, splits, merges and location assignments match63 paired
structural phases exactly. Ordinary append still trends toward 32/64 occupancy;
this cost was not separately tuned.

## 8. Long churn and severe shrink

Two production processes per variant at each100k/1m size, plus allocation and
structural processes:16 confirmation processes,100 cycles each,100k pairs/cycle,
**10 million remove/reinsert pairs per process**. Exact full-order checkpoints
initially, at cycles 1/11/.../91 and 100; no per-mutation proof is claimed.

At 1m, pooled median cycle wall time over 200 production cycle rows:
**108.450 ->93.374 ms** (13.9% lower); worst observed complete cycle
**131.783 ->114.926 ms**. Cycles within a process are correlated, so200 rows
are not200 independent fresh-process samples.

Diagnostic requested bytes at cycles 1/50/100:

|Variant|Cycle1|Cycle50|Cycle100|
|---|---:|---:|---:|
|Baseline|45,915,224|45,976,544|45,973,624|
|Candidate|38,105,784|38,167,944|38,164,984|

No accumulating requested-memory leak is observed in this finite campaign.
The structural-process working set is steady at 130,084,864 versus 114,585,600
bytes. This does not isolate long-term heap fragmentation.

Separate1m growth ->90% deletion ->99% deletion ->regrowth ->destruction runs
(two fresh processes each) preserve all surviving handle identities and exact
order. Diagnostic requested bytes at 1% live are **464,504 ->386,424**. Blocks
are240 on both sides. Core diagnostic metadata accounts for 4,280 bytes over
production. No source-sized record pool is retained. During shrink, process
working set remains near its high water mark on **both** variants; malloc/free
requests reaching the allocator do not promise immediate OS page return.

## 9. Regressions and uncertainty inventory

The candidate is not universally faster. Confirmation includes:

- 100k second retained snapshot:0.790 ->1.184ms (+49.9%). Five-process follow-up:
 **0.779 ->0.994ms (+27.5%)**. This small but repeated adverse phase remains
 reported; it is not explained away or optimized as Step 2 work.
- 1m duplicate-heavy locate:23.957 ->25.747ms (+7.5%). Five-process follow-up:
 **23.989 ->25.206ms (+5.1%)**. This is an active throughput tradeoff.
- 100k prepend-origin local moves:1.914 ->2.561ms (+33.8%). Five-process
 follow-up:1.881 ->1.754ms (-6.8%); the initial slowdown is not repeatable evidence
 of a33.8% architectural penalty. It remains in the inclusive primary table.
- 100k append local moves:+5.6%;3m prepend hotspot churn:+8.4%;1m deserialize:
 +6.6%. Two-repeat results cannot establish universal effect sizes.
- The largest primary sampled individual/pair duration is 253.9us on baseline
 (3m duplicate growth) versus 318.3us on candidate (3m local-origin uniform move).
 These are different phases and scheduling-sensitive observations, not paired
 worst-case proofs or an isolated causal regression.

Extra block-owner indirection is a real design cost, even when compactness offsets
it. No claim is made that all timing variance is noise. The measured memory gain,
better random insertion/turnover and low-complexity implementation justify
**independent review**, not automatic adoption despite these regressions.

## 10. Correctness and compatibility

Confirmation matrix:748 exact forward/reverse occurrence/handle checkpoints;
confirmation long:192; focused runs:130. Exploratory campaigns add805;
all1,875 checkpoint fingerprints agree within their corresponding workload/seed
across variants, modes and repeats. Four severe-shrink processes add16 exact
checkpoints. The independent oracle is an ID-linked model with its own Python
list self-test, not Full's block order. Managed expected ordering is independently
sorted by key/arrival identity. All tracked final allocations return to zero.

GCC C17 strict Debug/Release **39/39 each**; MSVC x64 Debug/Release **39/39 each**;
MSVC x64 ASan RelWithDebInfo **39/39**. Research harness GCC and MSVC ASan
**12/12 each**. Seven public examples run. Frozen manifest **64 functions** and
LS1/LKS4SNP1/LK1 architecture-independent golden corpus pass. Source, FetchContent,
installed C17/C++ consumers and twice-generated deterministic amalgamation pass.

The added regression covers **133 failing bulk allocation points**, then success,
with no escaped partial order or live-allocation increase; stable moves/merges,
foreign live handle rejection, and survivor ownership are checked. Existing
insertion split, cross-block move preparation, Managed insert, snapshot/restore,
callback/reentry and migration OOM suites pass unchanged. No new allocation
path needs a newly invented allocator guarantee.

The initial ordinary-shell ASan run failed to load its runtime DLL; the VS
runtime-environment rerun passed. GCC ASan/UBSan cannot link due missing runtimes;
local Clang and installed WSL are unavailable. See [VALIDATION](VALIDATION.md)
for commands/logs. Remote Linux/Clang/macOS/Win32 CI, if completed after push,
is separate exact-commit evidence in the delivery report, not a local result.

Ownership/lifetimes, valid live handle identity, invalidation, cursors/revisions,
stable equal items, callbacks, source markers, Group/Batch and wire bytes remain
unchanged. Headers/manifests/versions/fixtures are byte-identical to baseline.
Record ownership now requires one extra block read; arbitrary/expired pointers
remain outside the documented valid-input contract, with no stale registry.

## 11. Parent/subordinate disposition

|Cost family|Disposition after compact records|
|---|---|
|Repeated resident owner metadata|Resolved in this candidate|
|Live requested memory and allocation traffic|Partially improved|
|Managed random scaling|Partially improved; strong size dependence remains|
|Broad movement/turnover|Partially improved; same logical/index work|
|Allocation frequency|Unaffected: still one allocation per resident|
|Block slack/occupancy|Unaffected; extra 8 bytes/block offsets a small saving|
|Restore resident requests|Partially improved incidentally, counts unchanged|
|Restore scratch / snapshot copies / wire bytes|Unaffected; Step 2 remains separate|
|Duplicate-heavy locate / small retained capture|Observed regressions retained|
|Long-churn accumulation|No leak observed before or after; not a newly solved bug|
|Pool utilization / pool release bookkeeping|Not applicable to selected candidate|

No unbounded stale pointer registry, new data structure or public transaction
was introduced. Failure atomicity remains mandatory. Complexity contracts are
unchanged and no universal latency/memory/allocator theorem is claimed.

## 12. Delivery, evidence and remaining limits

[README](README.md) provides exact workload definitions and reproduction commands;
[VALIDATION](VALIDATION.md) gives build/package gates. `evidence/` includes raw
compressed stdout/stderr with exact commands/admission state, shrink traces,
slab screen and validation logs. `results/` preserves all phase/quantile and
comparison rows, including regressions. `FILES.txt` is the complete changed-file
inventory and `SHA256SUMS.txt` protects delivered assets and changed code.

Missing/inconclusive: fresh10m candidate measurements,10m sanitizers, a fully
integrated pooled-core throughput/OOM comparison (rejected at the reclamation
screen), hardware performance counters/allocator-size-class attribution,
statistical significance, arbitrary-duration fragmentation and real user traces.
The candidate is not validated on every possible allocator/platform. No step
beyond this assignment is needed to finish an ordinary known bug; none remains
unresolved in this candidate. Upper-scale and attribution gaps are explicit.

This is a bounded compatible optimization with honest regressions and review
material, not a claim that Step 1 eliminates all live-core costs. Do not merge it
by default. Snapshot/history changes, main integration, release decisions and
roadmap edits require their separately assigned scope.
