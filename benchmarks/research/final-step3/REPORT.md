# Full Step 3 — final integrated engineering acceptance

2026-10-25. **A — READY FOR COMPATIBLE ADOPTION** is the engineering
recommendation. This is not final project PASS, permission to merge or release,
or a claim that every workload becomes faster. The owner and independent
ChatGPT review make the final acceptance decision.

## 1. Exact ancestry and scope

|Reference|Commit|
|---|---|
|Full Stable v4.0.0|`80004ae732daf696f7e9192e417de0edb0b2aafa`|
|A: original public main|`043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`|
|B: accepted Step 1|`19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`|
|C: mandatory accepted Step 2 base|`291305dca03d2d7599c88b6e6cebf2cc426e1790`|
|Fresh driver / Linux research run commit|`9baa229194d7900f9de5107ef743b5f9baaa3bde`|
|Bounded distribution-tool correction|`7f8cc0b5a9f7687286ccf2729ab02456aa5df7de`|

Each preceding baseline is an ancestor of the next. A's production files,
public header and production source manifest are identical to Stable. An actual
isolated A worktree is used locally; B's accepted checkout remains unchanged.
C includes BOTH optimization steps. Its production code remains exactly the
accepted Step 2 source throughout this assignment. The enclosing evidence
commit and exact verified remote SHA are reported in the delivery response.

Branch: `integration/full-final-step3`. Main, Stable, Step 1, Step 2, Mini,
prior research evidence, Issues and governance remain unchanged. No version,
PR, merge, tag, release or follow-on development phase is included. Stable tag
object remains `83d67d3ef5dacaf6117db320d9bbea68b199e23e`.

## 2. Combined architecture and direct source review

The complete A..C production diff is seven private files: `lks_order_internal.h`,
`order.c`, `order_bulk.c`, `lks_snapshot_internal.h`, `snapshot.c`,
`snapshot_wire.c`, `snapshot_restore.c`. Read every actual diff and adjacent
construction/movement/repair, managed lookup, marker, Group/Batch, key and
restore paths. [SOURCE_REVIEW.md](SOURCE_REVIEW.md) records the detailed proof
obligations, arithmetic, alignment and lifetime reasoning.

Step 1 removes the duplicate owner pointer from individually allocated stable
Resident records, sharing it at block level. On x64, residents are 32 ->24 bytes
and blocks 584 ->592 bytes. B64/AVL algorithms, stable addresses, per-record
reclamation and allocation count remain. Live requested saving is exactly
`8*(resident_count-block_count)` in these builds.

Step 2 uses one cumulative end word per Snapshot row (16 ->8 bytes on x64),
groups header/rows/copied namespace into one allocation, and trims association
capacity before publication. Repeated ends represent empty spans. Every row is
initialized before publication; failure cleanup never reads unfinished rows.

Normal and bulk construction initialize block owners; location updates precede
block retirement. Foreign LIVE handles are rejected through their block owner.
Expired handles are not probed. Allocation precedes structural commit. Snapshot
trim precedes marker retain/create. Failed realloc still leaves an owned buffer
for cleanup. Marker lifetime, callbacks, currentness and resolve-first ordering
are unchanged. Group/Batch adapters retain their independent immutable semantics.

No production correction is justified by this review or subsequent tests. No
new data structure, public API, allocator, coordinate scheme or persistent format
is introduced. Native layout is never persisted.

## 3. One bounded corrective change

The distribution validator, not the production library, had a confirmed Windows
implicit-single-config-generator failure. It omitted CMAKE_BUILD_TYPE but built
and installed with `--config Debug`, which can omit the imported-location export
when the implicit generator is Ninja. The original failed output is preserved.

`tools/validate_distribution.py` now always configures Debug, matching its
existing build/install intent. Multi-config generators ignore BUILD_TYPE. The
previously failing default Ninja invocation passes after the correction; explicit
MinGW and MSVC source/FetchContent/installed C/C++/amalgamation paths also pass.
Remote Windows x64/Win32, Ubuntu and macOS distribution gates pass with this fix.
It changes validation configuration only, not shipped algorithms or API behavior.

An initial ASan command-launch quoting error is separately retained; it did not
execute a test. The correctly invoked developer-environment retry passes 39/39.
Neither failure is retroactively relabeled a successful run.

## 4. Environments and measurement controls

Windows 11 x64 build26300; Ryzen 9 9955HX,16 cores/32 threads; physical memory
16,304,992,256 bytes. GCC16.2.0 UCRT64, strict C17, `-O3 -DNDEBUG` production
measurements; MSVC19.51 supplies native Debug/Release and ASan validation.
Local CMake4.4.3/Python3.13.14 versions and exact commands are archived.

Linux: Ubuntu24.04 x86_64, kernel6.17 Azure; runner presents 4 vCPUs/EPYC9V74,
16,766,414,848 physical bytes. GCC13.3.0, CMake3.31.6 and setup-python3.12.
All A/B/C binaries are built before sequential measurements on each host.
Earlier Step 1/2 timing numbers are historical observations, not pooled data.
Different Linux runner CPUs/protocols cannot establish cross-round causation.

Workloads/oracles are the unchanged Step 2 C drivers. Three-source order rotates
by repetition; it balances source position, not every possible pairwise carryover.
Production has three repeats normally, six for specified regression cases,
two for each 10m case. Allocation and private structural modes use separate
processes and matching source headers, not production timing substitutes.

Admission remains min(3 GiB,40% available), plus3 GiB reserve. Existing2 GiB
runtime guards and the360-second child timeout remain. No unrelated process was
terminated and no safeguards/correctness checks were weakened. A rejected
admission is a skip, never a pass. No unsafe10m sanitizer campaign is claimed.

All normal/native timings include callback and allocator cost where invoked;
oracle preparation and full scans are outside timed replay. Query/format phases
retain their original in-loop checks, consistently across sources. Callback-only
calibration is never subtracted. Sampled latency quantiles/maxima are observations,
not worst-case bounds; raw tables preserve them. Timed-phase sums include source
and calibration work and must not be described as pure history time.

## 5. Full correctness and compatibility matrix

|Gate|Fresh observed result|
|---|---|
|Strict GCC C17 Debug / Release|39/39 each|
|MSVC x64 Debug / Release|39/39 each|
|MSVC x64 ASan RelWithDebInfo|39/39|
|GCC research A/B/C, each source|21/21 each|
|MSVC ASan research + layout/lifetime probe|21/21|
|Linux research A/B/C|21/21 each|
|Seven published public examples|All execute successfully|
|Source/add_subdirectory, FetchContent consumers|Pass|
|Installed C17/C++ and full-header consumers|Pass|
|Twice-generated amalgamation files/ZIP and consumers|Pass|
|Frozen public API|64 functions /14 types, unchanged|
|LS1/LKS4SNP1/V3 LK1 vectors and fixture hashes|Unchanged, executable checks pass|

Production CI at9baa229: run40168805412 success. With the distribution fix,
run40170420306 success, including `full-gate`. Inspected logs from each of the
six configurations show39/39 and successful distribution validation:
Windows MSVC x64, Windows MSVC Win32, Ubuntu GCC, Ubuntu Clang, Ubuntu Clang
ASan/UBSan and macOS AppleClang. The extended Clang sanitizer campaign also
passes. `evidence/ci-inspected.json` records exact job IDs and step conclusions.
Final evidence-commit CI is verified and linked in the delivery response.

Local Clang is unavailable. A fresh Windows GCC ASan/UBSan link probe fails for
missing `-lasan`/`-lubsan`; those are not local passing configurations. Actual
Linux sanitizer coverage comes from CI. No unavailable platform is called local.

Existing suites cover explicit and managed mutation, stable equality, cursors,
capacity, AVL/block repair, movement, foreign handles, callback reentry, immutable
Groups/Batches, snapshot lifetimes/currentness, exact keys/wire, malformed bytes,
V3 import, overflow, bulk restoration and complete tracked cleanup.

## 6. OOM, publication and lifetime acceptance

Fresh sweeps include133 failing bulk-construction points before success, stable
cross-block moves/merges with live foreign handles, insertion/spare-block OOM,
managed/cursor OOM, and12 Snapshot capture failures before success. The last
capture failure occurs after all257 callbacks at final trim. Prior source,
current snapshot, revision/size, outputs and tracked bytes remain valid.

Deserialization, fresh/existing marker, restore and V3 migration failpoints remain
covered. Resolve callbacks complete before fresh resident/core construction;
failure never frees caller items. Serialization/key formatting remain allocation-
free, including under a pending allocation failure. Source-before-snapshot,
snapshot-before-source, corrupted/freed caller-wire, scratch-buffer reuse,
zero associations and protected concurrent marker releases/reads all pass.

Every completed instrumented benchmark reaches zero tracked Full allocations
at final destruction. This does not count deliberately retained immutable history
as a leak or assert arbitrary-duration safety. No intentionally expired handle
was dereferenced. Native layout/alignment probes pass for A/B/C and under ASan;
Win32 real CI additionally exercises the narrow-size_t production paths.

## 7. Fresh measurement scale

|Host|Live|History|Long|Shrink|10m completed|Safely skipped|Completed total|
|---|---:|---:|---:|---:|---:|---:|---:|
|Windows|441|285|24|6|12|24|768|
|Linux|441|285|24|6|24|12|780|

Total: 1,548 completed native processes,36 skips,11,802 exact ordering checkpoint
fingerprints and23,286 preserved phase rows. Checkpoints are not per-mutation
proof; complete in-process traversal/association/wire/restore checks supplement
those fingerprints. All compared fingerprints agree across A/B/C/modes/repeats.

Windows admits10m append/move/churn/shrink/regrow but rejects10m eight-byte history
under actual available-memory admission. Linux admits both that live workload
and10m/two-snapshot eight-byte history. Both hosts reject10m variable-payload
history because its6,440,663,296-byte envelope exceeds the unchanged cap.
No unsafe case is extrapolated or replaced with old baseline-only measurements.

Live profiles cover append, prepend, fixed anchor, local insertion, managed
ascending/descending/random/64-way duplicates/all-equal. Explicit profiles also
exercise local/hotspot/front-back/uniform movement, repeated churn,50% deletion,
regrowth and teardown. Managed locate uses4096 deterministic queries with
25% missing/high probes; query loops cap at200k operations.

History covers absent callback, empty spans,one/eight/fixed256-byte spans,
variable0..128 and wide0..2048 bytes, namespace1/16/4096, unchanged and mutated
sources,2..16 retained snapshots, partial release, load,three restores and source/
caller-buffer destruction. Each generic process verifies every association and
the entire serialized image independently. Legacy ID8 remains a separate trace.

Full numeric tables appear in the appendix and [TABLES.md](TABLES.md); all
repetitions, ranges, diagnostics and unfavorable rows remain in `evidence/`.

## 8. Requested memory acceptance

Owned eight-byte history subtracts the pre-capture source and includes its shared
marker once. Native allocation-wrapper requests give the following identical
100k..3m figures on both hosts;10m history is Linux only.

|Rows / retained|A history bytes|B history bytes|C history bytes|Whole peak A/B/C bytes|
|---|---:|---:|---:|---|
|100k /16|42,378,632|42,378,632|25,601,416|54,716,056 /53,153,552 /35,576,336|
|1m /16|390,219,144|390,219,144|256,001,416|513,593,776 /497,968,768 /355,751,040|
|3m /4|326,218,088|326,218,088|192,000,360|696,342,720 /649,467,712 /491,249,984|
|10m /2|588,435,640|588,435,640|320,000,184|1,822,185,272 /1,665,935,264 /1,317,499,808|

The combined1m case reduces whole requested peak about30.7% versus A;10m reduces
it about27.7%. Source-only1m append requests fall50,249,480 ->42,499,472 bytes;
10m source requests fall502,499,480 ->424,999,472. Allocation count is unchanged
by Step 1. Whole peaks include the actual resident source/restore orders, retained
history and library scratch, not application allocations or allocator internals.

Independent checks validate189 regular+12 upper live structural phase triples
per host: same allocation/free/comparison/repair/index/occupancy work and exact
x64 saving8*(residents-blocks), with B/C requested memory equal. Twelve severe-
shrink phase triples per host satisfy the same reclamation formula.98 generic
capture triples per host plus2 Linux 10m triples verify half-sized rows, exact
capacity, equal useful bytes/callback counts/wire sizes.

Raw rows distinguish useful bytes, capacity, temporary pointers and process WS/RSS.
Process samples include caller fixtures, CRT/runtime and observer overhead;
maximum sampled working set is not a continuous OS peak. The allocator may
retain pages after free. No fragmentation/cache/page-retention cause is invented.

## 9. Adverse timing investigation and engineering disposition

No unfavorable observation is deleted. [ADVERSE.csv](ADVERSE.csv) lists all
non-calibration production phase medians C/A >1.05, regardless of whether their
absolute delta is tiny. This is a listing aid, not a post-hoc adoption threshold.
Ranges/repetitions and per-cycle rows remain in the underlying CSVs.

|Observation|Fresh evidence|Disposition / reason|
|---|---|---|
|Prior Linux 3m random lookup +15.8% C/B|Six repeats: A/B/C217.717/227.254/240.027ms. C/B+5.6%, C/A+10.2% (+22.310ms per200k queries)|Adverse lookup timing persists, at lower C/B magnitude. Accepted measured cost, about112ns/query versus A; not erased as noise. Search/work counts are identical, ownership/layout differ A/C; exact cause unisolated.|
|Prior Windows 100k local movement +17.5%|Six repeats:1.939/1.820/1.828ms; C/B+0.45%, C/A-5.7%|Does not reproduce at old magnitude in this round. No claim that all move workloads improve.|
|Prior Windows 1m legacy serialization +5%|Six repeats:2.671/2.819/2.471ms; C/B-12.3%, C/A-7.5%|Not reproduced on this trace. Wide payloads still show different behavior.|
|Step 1 duplicate-heavy locate|Windows 100k/1m/3m C/A approximately-1.1/-1.2/-1.3%; Linux-1.0/+0.6/-1.1%|No consistent material duplicate-lookup regression in this matrix. Historical5.1% observation remains historical, not disproven for other environments.|
|Other live lookup/movement/churn|Linux descending1m locate+31.127ms (+16.2%); Windows append-origin1m hotspot churn+6.142ms (+16.9%); Linux local-origin3m hotspot churn+9.131ms (+18.6%)|Accepted phase-specific throughput costs, no new work-count/complexity cliff. About156ns/query and31/46ns per churn pair respectively; full affected traces and ranges remain.|
|Linux variable100k serialization|Six repeats:1.078/1.996/1.946ms; C/A+80.6% (+0.869ms), C/B-2.5%. Per-source ranges overlap/bimodality is preserved.|Real adverse A/C median; not a demonstrated incremental Step 2 regression in this round. Source-copy paths reviewed, cause unisolated. Accepted bounded cost; no unsupported cache attribution or speculative rewrite.|
|Large4096-byte namespace key formatting|Linux example:48.856/50.403/63.031ms for4096 checked keys; C/A+29.0%, C/B+25.1% (+14.175ms)|Repeatable specialized CPU cost (~3.46us extra/key vs A), accepted explicitly. Key algorithm/bytes unchanged; grouped namespace placement changes, but causation not isolated. Key-heavy large-domain users must assess this tradeoff.|
|Wide serialization|Windows 1m fixed256:38.239/39.192/41.616ms, C/A+8.8% (+3.377ms)|Accepted measurable256MB-copy phase cost. Actual wire bytes are unchanged; smaller history capacity is not a universal CPU win.|
|Destruction pockets|Linux 1m empty loaded destruction0.160/0.191/0.326ms; Windows 100k fixed256 partial-history destruction+0.506ms|Some percentage regressions are large; absolute costs and every affected row remain. Grouped frees and exact shrink alter allocation lifecycle. No leak/atomicity failure; accepted measured costs, not promised faster destruction.|
|Wide-span requested allocation traffic|Final trim adds an A-byte realloc request; row requests drop8N. Net traffic increase can be roughly A-8N per capture.|Accepted explicit storage-versus-request-traffic tradeoff. Request bytes are not actual allocator copied bytes. No claim that callback copies or allocation traffic universally decrease.|

Relevant source paths were reviewed: `v4_managed_bound`, predecessor/public owner
checks, `v4_move`/repair and remove/reinsert, sequential Snapshot wire loops,
namespace/key loops, trim and destruction. Windows optimized `.text` sections
for order,bulk,managed modules are identical B/C; hashes are archived. This is
not proof of identical executable placement, runtime memory state or Linux code.

The old Step 2 Linux capture/serialization observations cannot be causally compared
with this new EPYC model. Unchanged source/work counts cannot make a slower timing
nonexistent. Conversely, a timing difference alone does not identify a Snapshot
source defect. No causal CPU/cache/allocator experiment or formal timing proof is
claimed. A speculative corrective algorithm change is therefore not justified.

The acceptance judgment is overall engineering value: substantial, deterministic
resident/history storage reduction; unchanged allocation/reclamation semantics;
broadly improved growth, long-churn and restore measurements; limited private
code complexity; and no correctness/compatibility failure. The adverse costs
above remain genuine workload suitability constraints. A lookup/large-domain-key/
wide-export dominated application may prefer Stable until its own budget accepts
them. The recommendation is not universal performance approval.

## 10. Long-running stability and retained costs

Each host runs24 long processes:100k/1m residents, A/B/C, two production and one
allocation/private process per size. Each process executes10m remove/reinsert
pairs; that is240m pairs per host, plus separate ordinary interaction traces.
All model counts/identities/checkpoints and final cleanup pass. Resident requests
remain proportional to current residents and occupied blocks, not an accumulating
retired-record slab. Blocks may fluctuate during valid churn; constant N does
not imply an identical block count on every cycle.

Pooled1m cycle medians A/B/C: Windows108.535/96.145/96.019ms;
Linux137.999/116.846/116.258ms. Worst observed cycles remain in the appendix/raw
rows, not converted into formal tail guarantees. Two severe-shrink processes
per source/host delete90%, then99%, regrow to1m and perform32 full scans.
They preserve surviving handles and reclaim individual records; all finish with
zero tracked allocations. Snapshot processes independently retain/release states,
mutate sources, restore/destroy orders and validate bytes after caller/source
lifetime ends. This finite evidence does not prove arbitrary-duration reliability.

Restore still allocates N stable records,ceil(N/64) blocks,one core and an
N-pointer scratch: `N+ceil(N/64)+2` nonempty allocations. The pointer vector is
8N bytes on measured x64. Removing it would change resolve-first semantics or
add other complexity and is intentionally not attempted. Full capture remains
O(N+association bytes); K independently owned histories remain K full copies.

## 11. Remaining-cost classification and cycle endpoint

|Cost family|Classification / disposition|
|---|---|
|Duplicate per-resident owner metadata|Resolved by Step 1; block ownership preserved|
|Stable-address immediate reclamation|Preserved; no slab pinning introduced|
|Resident allocation frequency / B64 slack|Accepted remaining cost; no allocator redesign|
|Redundant Snapshot offset+size words|Resolved by Step 2; one end word remains|
|Separate fixed-field Snapshot allocations|Resolved by Step 2|
|Published geometric association slack|Resolved by Step 2 at requested-capacity level|
|History/loaded/restore-context footprint|Partially improved; full independent ownership remains|
|Callback copying and full recapture|Accepted remaining cost required by current contract|
|Resolve-first pointer scratch / resident build allocations|Accepted remaining cost|
|Wide final-trim requested traffic|Accepted remaining cost/regression, explicitly measured|
|Lookup/move/export/destruction timing pockets|Accepted remaining costs with suitability limits above|
|Unisolated allocator/OS retention or cache effects|Independent future research candidate only if a real use case requires it; no asserted defect|
|Implicit Windows single-config validation mismatch|Reproducible defect requiring action; corrected and retested within Step 3|
|Open correctness, wire or OOM defect|None confirmed by completed review/gates|
|New APIs/formats, Mini, public allocator, pooling, delta history, cross-source comparisons, real-time guarantees|Explicit non-goals|

Resolved parent costs and removed symptoms are consolidated; no fourth step or
new optimization roadmap is proposed. Existing structural B64/AVL contract is
preserved. No new allocator-inclusive/comparator-inclusive worst-case latency,
formal end-to-end amortized claim, fixed resource ceiling or universal speedup
is asserted.

## 12. Evidence, reproduction, checksums and handoff

[VALIDATION.md](VALIDATION.md) provides exact reproduction and verification
commands. Research run40168805478 succeeded for9baa229; artifact12590122413 was
downloaded and its outer SHA256 verified:
`fffb7c93d14b06336c48207a955240a0468ca2482e0c7454b174a9bbab31da45`.
Every original inner manifest hash is verified. Permanent unpacked captures,
analyses, commands, environments, compile metadata and logs are committed here.
All22 Linux analysis outputs were recomputed from raw data and matched after
normalizing only host text-newline conventions for comparison. Original capsule
bytes are preserved, not reformatted. Source/object/binary SHA records identify
the measured inputs; compiled executables are not released as assets.

The Step 3 and integrated A..final file inventories are `FILES.txt` and
`INTEGRATED_FILES.txt`. `SHA256SUMS.txt` covers canonical committed Git blobs
for the integrated changed files except itself. Native capsule bytes use local
attributes to preserve original checksums. Earlier research data are unchanged.
The completion response supplies the enclosing final SHA, final CI and verified
clean/synchronized remote state.

Remaining uncertainty: only two hosts/finite deterministic workloads; ordinary
scheduler/link-layout variance; sparse latency/process-memory samples; no causal
hardware-counter attribution, allocator-internal copy/usable-size measurement,
arbitrary-duration fragmentation test, confidence interval or real-user acceptance.
Windows 10m history, both hosts'10m variable history and10m sanitizer/high-retention
cases are not claimed. No unresolved blocker is confirmed. A different application
performance budget can reasonably weigh the disclosed costs differently.

**Final engineering recommendation: A — READY FOR COMPATIBLE ADOPTION.**
No merge/release authority is exercised. This fixed three-step optimization cycle
ends with this integrated evidence delivery and independent acceptance handoff.

# Numeric appendix — fresh paired measurement tables

All timing cells are A / B / C milliseconds. A=original main/Stable; B=Step 1; C=combined candidate. Hosts are never pooled.

## Windows

### Live growth and managed lookup

|Rows|Trace|Growth A/B/C ms|Locate A/B/C ms|
|---|---|---|---|
|100000|append|5.765 / 5.297 / 5.328|not a managed workload|
|100000|prepend|5.659 / 5.476 / 5.328|not a managed workload|
|100000|anchor|6.215 / 6.279 / 5.950|not a managed workload|
|100000|local|7.808 / 7.670 / 7.486|not a managed workload|
|100000|ascending|6.988 / 6.411 / 6.669|22.890 / 22.730 / 23.724|
|100000|descending|8.431 / 8.071 / 7.839|23.268 / 23.229 / 23.675|
|100000|random|28.786 / 27.106 / 27.454|26.611 / 26.276 / 26.115|
|100000|duplicates|14.934 / 14.411 / 14.222|9.105 / 9.266 / 9.001|
|100000|equal|7.325 / 6.325 / 6.324|4.470 / 4.458 / 4.426|
|1000000|append|62.422 / 53.703 / 54.953|not a managed workload|
|1000000|prepend|58.076 / 54.244 / 53.946|not a managed workload|
|1000000|anchor|65.972 / 62.388 / 62.254|not a managed workload|
|1000000|local|78.134 / 75.452 / 74.808|not a managed workload|
|1000000|ascending|78.913 / 72.679 / 72.902|76.395 / 77.878 / 78.529|
|1000000|descending|88.803 / 85.598 / 84.795|73.227 / 72.209 / 75.366|
|1000000|random|868.551 / 785.877 / 760.997|93.709 / 97.466 / 90.494|
|1000000|duplicates|182.920 / 171.945 / 169.923|24.387 / 24.808 / 24.090|
|1000000|equal|84.715 / 80.985 / 77.515|11.284 / 12.781 / 11.341|
|3000000|append|179.404 / 162.355 / 161.839|not a managed workload|
|3000000|prepend|180.133 / 164.761 / 163.075|not a managed workload|
|3000000|anchor|207.427 / 181.643 / 191.331|not a managed workload|
|3000000|local|245.483 / 232.435 / 225.087|not a managed workload|
|3000000|ascending|274.151 / 244.815 / 245.163|125.415 / 145.319 / 123.678|
|3000000|descending|279.920 / 274.286 / 278.469|98.419 / 96.625 / 92.704|
|3000000|random|4437.361 / 4016.894 / 4175.666|131.119 / 133.046 / 129.802|
|3000000|duplicates|539.053 / 514.124 / 513.956|26.770 / 26.227 / 26.434|
|3000000|equal|241.537 / 222.942 / 224.518|11.307 / 11.439 / 11.074|

### Append-origin interaction phases

Movement/churn phases use min(N, 200k) operations; churn is remove/reinsert pairs. Grow/shrink retain their actual row counts.

|Rows|Phase|A/B/C ms|
|---|---|---|
|100000|local_move|1.939 / 1.820 / 1.828|
|100000|hotspot_move|10.251 / 9.726 / 10.062|
|100000|uniform_move|11.951 / 11.344 / 11.694|
|100000|hotspot_churn|14.955 / 13.499 / 13.789|
|100000|half_delete|3.412 / 3.254 / 3.384|
|100000|regrow|1.802 / 1.774 / 1.786|
|100000|library_destroy|1.679 / 1.514 / 1.527|
|1000000|local_move|28.307 / 24.535 / 24.618|
|1000000|hotspot_move|106.577 / 75.248 / 84.045|
|1000000|uniform_move|143.593 / 116.795 / 129.789|
|1000000|hotspot_churn|36.262 / 36.367 / 42.404|
|1000000|half_delete|109.390 / 80.667 / 87.364|
|1000000|regrow|31.612 / 24.844 / 25.781|
|1000000|library_destroy|58.075 / 42.643 / 46.978|
|3000000|local_move|37.465 / 33.682 / 31.578|
|3000000|hotspot_move|145.924 / 120.805 / 122.358|
|3000000|uniform_move|175.278 / 156.872 / 160.306|
|3000000|hotspot_churn|37.691 / 36.389 / 37.663|
|3000000|half_delete|228.595 / 195.529 / 194.500|
|3000000|regrow|84.364 / 69.699 / 69.072|
|3000000|library_destroy|152.231 / 123.359 / 124.551|

### Snapshot / History

Capture is first snapshot only; all repeated captures and remaining restore cycles are in raw phase tables. Historical-key timings include the independent expected-byte checks.

|Rows|Payload / retained / namespace bytes|Capture A/B/C ms|Serialize A/B/C ms|Load A/B/C ms|Restore #0 A/B/C ms|
|---|---|---|---|---|---|
|100000|eight / 16 / 16|1.782 / 1.687 / 1.608|0.263 / 0.269 / 0.257|0.955 / 0.922 / 0.883|4.560 / 3.735 / 3.492|
|100000|empty / 2 / 1|0.769 / 0.725 / 0.651|0.103 / 0.100 / 0.089|0.652 / 0.645 / 0.499|3.380 / 2.983 / 2.981|
|100000|fixed256 / 8 / 16|17.451 / 17.280 / 17.258|4.136 / 4.162 / 4.061|5.235 / 5.563 / 4.847|10.825 / 9.842 / 10.969|
|100000|legacy / 2 / 2|0.938 / 0.827 / 0.711|0.259 / 0.281 / 0.262|0.924 / 0.930 / 0.845|3.391 / 3.015 / 2.903|
|100000|none / 2 / 1|0.396 / 0.345 / 0.222|0.106 / 0.105 / 0.090|0.637 / 0.645 / 0.487|3.691 / 3.118 / 3.063|
|100000|one / 8 / 4096|1.120 / 1.083 / 1.026|0.252 / 0.215 / 0.218|0.951 / 0.933 / 0.814|3.742 / 3.287 / 3.289|
|100000|variable / 16 / 16|5.592 / 5.505 / 5.270|0.939 / 0.889 / 0.930|1.624 / 1.622 / 1.463|5.787 / 5.339 / 5.157|
|100000|wide / 4 / 4096|57.011 / 57.441 / 55.462|15.322 / 15.219 / 14.778|23.699 / 23.332 / 23.156|24.939 / 24.236 / 24.577|
|1000000|eight / 16 / 16|36.416 / 30.858 / 29.720|3.589 / 3.063 / 2.779|10.911 / 10.206 / 8.936|46.849 / 40.713 / 41.015|
|1000000|empty / 2 / 1|21.468 / 14.576 / 13.145|1.057 / 1.047 / 1.163|6.147 / 6.148 / 5.223|36.039 / 32.385 / 32.314|
|1000000|fixed256 / 4 / 4096|178.858 / 157.685 / 158.231|38.239 / 39.192 / 41.616|49.929 / 51.706 / 50.158|114.608 / 107.428 / 101.789|
|1000000|legacy / 2 / 2|18.033 / 13.641 / 12.083|2.671 / 2.819 / 2.471|9.512 / 9.437 / 8.567|36.599 / 31.238 / 30.613|
|1000000|none / 2 / 1|9.212 / 7.918 / 6.784|1.035 / 1.098 / 1.085|6.247 / 6.512 / 5.096|38.189 / 33.890 / 34.771|
|1000000|one / 4 / 4096|26.899 / 20.909 / 20.211|2.463 / 2.205 / 2.228|9.107 / 8.891 / 8.608|39.399 / 34.572 / 36.592|
|1000000|variable / 8 / 16|74.928 / 65.811 / 63.115|12.540 / 13.171 / 10.363|22.423 / 23.337 / 16.325|63.547 / 58.565 / 55.067|
|3000000|eight / 4 / 16|99.314 / 87.724 / 85.422|8.276 / 8.141 / 7.589|29.718 / 29.649 / 26.908|136.317 / 119.507 / 119.648|
|3000000|variable / 2 / 4096|219.539 / 187.533 / 192.541|34.539 / 35.411 / 34.276|50.868 / 51.442 / 48.364|181.314 / 163.240 / 167.394|

### Long churn

Pooled 200 cycle observations from two fresh production processes/source; each process runs 100 cycles of 100k pairs. These are not 200 independent processes. Each cell is median / worst observed cycle ms.

|Resident count|A median/max|B median/max|C median/max|
|---|---|---|---|
|100000|18.629 / 19.818|17.512 / 18.965|17.612 / 19.899|
|1000000|108.535 / 131.884|96.145 / 132.899|96.019 / 160.999|

### Severe shrink and regrowth

Diagnostic timings; two processes/source. These are not production timing substitutes. Requested bytes are native requested storage, not RSS.

|Phase|A|B|C|
|---|---|---|---|
|growth|82.907 ms; 50,253,760 bytes|73.445 ms; 42,503,752 bytes|72.815 ms; 42,503,752 bytes|
|scan32|1013.801 ms; 50,253,760 bytes|943.643 ms; 42,503,752 bytes|946.552 ms; 42,503,752 bytes|
|shrink90|396.863 ms; 4,608,280 bytes|374.909 ms; 3,827,512 bytes|367.894 ms; 3,827,512 bytes|
|shrink99|19.983 ms; 464,504 bytes|18.087 ms; 386,424 bytes|18.136 ms; 386,424 bytes|
|regrow|92.019 ms; 50,212,296 bytes|89.010 ms; 42,461,720 bytes|79.886 ms; 42,461,720 bytes|
|destroy|52.458 ms; 0 bytes|39.981 ms; 0 bytes|40.266 ms; 0 bytes|

### Admitted 10m workloads

Two fresh production processes/source; unadmitted rows are not extrapolated.

|Trace|Phase|A/B/C ms|
|---|---|---|
|append|growth|591.601 / 535.724 / 552.781|
|append|local_move|38.762 / 38.902 / 39.012|
|append|hotspot_move|196.244 / 187.428 / 184.342|
|append|uniform_move|230.724 / 189.050 / 195.237|
|append|hotspot_churn|49.882 / 42.856 / 50.812|
|append|library_destroy|626.037 / 447.239 / 441.412|

## Linux

### Live growth and managed lookup

|Rows|Trace|Growth A/B/C ms|Locate A/B/C ms|
|---|---|---|---|
|100000|append|8.900 / 7.855 / 7.854|not a managed workload|
|100000|prepend|12.717 / 11.210 / 11.182|not a managed workload|
|100000|anchor|9.988 / 9.167 / 9.195|not a managed workload|
|100000|local|12.043 / 11.059 / 11.071|not a managed workload|
|100000|ascending|11.585 / 10.443 / 10.686|43.344 / 43.100 / 43.364|
|100000|descending|16.196 / 14.782 / 14.963|46.145 / 45.652 / 45.953|
|100000|random|52.911 / 50.315 / 50.140|48.831 / 48.243 / 48.185|
|100000|duplicates|29.212 / 27.628 / 27.629|19.265 / 19.056 / 19.072|
|100000|equal|11.395 / 10.437 / 10.471|9.398 / 10.689 / 11.262|
|1000000|append|88.098 / 80.057 / 79.489|not a managed workload|
|1000000|prepend|127.091 / 112.930 / 112.187|not a managed workload|
|1000000|anchor|101.754 / 94.103 / 92.604|not a managed workload|
|1000000|local|118.981 / 109.244 / 108.968|not a managed workload|
|1000000|ascending|129.492 / 134.198 / 134.678|229.256 / 261.359 / 246.486|
|1000000|descending|203.093 / 183.704 / 185.042|192.400 / 203.139 / 223.527|
|1000000|random|1402.229 / 1659.101 / 1273.357|178.624 / 274.706 / 171.231|
|1000000|duplicates|324.654 / 307.057 / 307.465|47.955 / 48.224 / 48.253|
|1000000|equal|129.150 / 132.385 / 133.542|29.888 / 29.759 / 29.676|
|3000000|append|261.823 / 240.266 / 237.257|not a managed workload|
|3000000|prepend|377.499 / 341.881 / 333.322|not a managed workload|
|3000000|anchor|304.272 / 279.863 / 274.905|not a managed workload|
|3000000|local|354.948 / 323.636 / 325.645|not a managed workload|
|3000000|ascending|467.518 / 454.927 / 458.142|178.686 / 175.624 / 177.072|
|3000000|descending|630.542 / 583.575 / 581.503|198.260 / 193.185 / 196.446|
|3000000|random|7266.928 / 6536.193 / 7084.953|217.717 / 227.254 / 240.027|
|3000000|duplicates|1021.520 / 969.172 / 968.569|55.835 / 55.360 / 55.235|
|3000000|equal|466.696 / 461.162 / 453.218|36.641 / 38.060 / 37.994|

### Append-origin interaction phases

Movement/churn phases use min(N, 200k) operations; churn is remove/reinsert pairs. Grow/shrink retain their actual row counts.

|Rows|Phase|A/B/C ms|
|---|---|---|
|100000|local_move|4.864 / 4.635 / 4.896|
|100000|hotspot_move|21.211 / 19.912 / 20.039|
|100000|uniform_move|25.537 / 24.669 / 23.451|
|100000|hotspot_churn|17.566 / 18.094 / 17.956|
|100000|half_delete|6.880 / 6.026 / 6.257|
|100000|regrow|2.776 / 2.623 / 2.623|
|100000|library_destroy|1.224 / 1.158 / 1.153|
|1000000|local_move|41.088 / 38.405 / 37.482|
|1000000|hotspot_move|140.379 / 122.916 / 122.982|
|1000000|uniform_move|201.324 / 179.061 / 173.626|
|1000000|hotspot_churn|41.870 / 43.011 / 42.841|
|1000000|half_delete|158.744 / 128.828 / 133.597|
|1000000|regrow|43.227 / 37.557 / 37.807|
|1000000|library_destroy|43.373 / 36.200 / 35.922|
|3000000|local_move|53.443 / 49.920 / 53.602|
|3000000|hotspot_move|191.082 / 178.414 / 178.328|
|3000000|uniform_move|231.601 / 213.297 / 212.560|
|3000000|hotspot_churn|45.412 / 46.748 / 48.356|
|3000000|half_delete|327.100 / 289.819 / 288.478|
|3000000|regrow|134.016 / 119.181 / 117.684|
|3000000|library_destroy|116.655 / 103.575 / 101.719|

### Snapshot / History

Capture is first snapshot only; all repeated captures and remaining restore cycles are in raw phase tables. Historical-key timings include the independent expected-byte checks.

|Rows|Payload / retained / namespace bytes|Capture A/B/C ms|Serialize A/B/C ms|Load A/B/C ms|Restore #0 A/B/C ms|
|---|---|---|---|---|---|
|100000|eight / 16 / 16|3.673 / 3.769 / 3.284|1.071 / 1.112 / 1.187|2.598 / 2.577 / 2.145|6.649 / 5.941 / 5.885|
|100000|empty / 2 / 1|1.781 / 1.759 / 1.305|0.416 / 0.403 / 0.386|1.662 / 1.655 / 1.224|5.394 / 4.488 / 4.547|
|100000|fixed256 / 8 / 16|13.998 / 13.482 / 12.788|2.996 / 2.971 / 2.998|5.392 / 5.238 / 5.049|16.004 / 15.274 / 15.269|
|100000|legacy / 2 / 2|2.005 / 1.977 / 1.596|1.110 / 1.126 / 1.082|2.641 / 2.614 / 2.123|5.069 / 4.142 / 4.166|
|100000|none / 2 / 1|1.138 / 1.154 / 0.632|0.410 / 0.413 / 0.367|1.679 / 1.665 / 1.227|5.706 / 4.783 / 4.896|
|100000|one / 8 / 4096|2.924 / 2.870 / 2.473|0.847 / 0.875 / 0.876|2.251 / 2.271 / 1.739|6.197 / 5.399 / 5.431|
|100000|variable / 16 / 16|7.846 / 7.536 / 7.191|1.078 / 1.996 / 1.946|3.472 / 3.520 / 2.161|9.595 / 8.700 / 8.744|
|100000|wide / 4 / 4096|38.819 / 38.790 / 38.426|11.619 / 10.767 / 10.544|22.344 / 22.956 / 21.957|44.415 / 43.814 / 43.557|
|1000000|eight / 16 / 16|32.365 / 31.526 / 31.419|5.143 / 6.463 / 6.329|17.290 / 16.377 / 16.193|64.153 / 56.557 / 57.266|
|1000000|empty / 2 / 1|12.483 / 11.370 / 11.676|1.999 / 1.980 / 1.756|10.232 / 10.263 / 10.937|52.309 / 43.286 / 44.309|
|1000000|fixed256 / 4 / 4096|93.445 / 92.872 / 93.189|25.714 / 25.806 / 26.269|41.422 / 42.182 / 40.806|154.863 / 148.138 / 147.811|
|1000000|legacy / 2 / 2|16.515 / 15.265 / 14.568|5.793 / 6.000 / 5.068|17.166 / 16.888 / 16.929|46.067 / 39.721 / 39.530|
|1000000|none / 2 / 1|10.578 / 9.054 / 8.276|1.919 / 1.944 / 1.753|10.193 / 10.187 / 10.976|54.674 / 46.273 / 47.780|
|1000000|one / 4 / 4096|23.115 / 22.422 / 23.333|6.009 / 7.096 / 5.487|15.723 / 15.740 / 15.198|59.936 / 51.299 / 51.403|
|1000000|variable / 8 / 16|52.068 / 51.829 / 51.086|8.434 / 9.250 / 9.010|20.530 / 19.822 / 20.041|92.588 / 83.876 / 84.661|
|3000000|eight / 4 / 16|90.908 / 87.945 / 83.053|13.794 / 14.907 / 14.767|46.783 / 46.584 / 45.930|192.090 / 171.078 / 171.552|
|3000000|variable / 2 / 4096|148.106 / 148.699 / 142.447|23.950 / 24.967 / 22.934|57.578 / 57.832 / 55.003|279.287 / 255.890 / 258.496|

### Long churn

Pooled 200 cycle observations from two fresh production processes/source; each process runs 100 cycles of 100k pairs. These are not 200 independent processes. Each cell is median / worst observed cycle ms.

|Resident count|A median/max|B median/max|C median/max|
|---|---|---|---|
|100000|30.684 / 31.498|29.237 / 30.296|28.976 / 30.431|
|1000000|137.999 / 152.679|116.846 / 128.829|116.258 / 138.845|

### Severe shrink and regrowth

Diagnostic timings; two processes/source. These are not production timing substitutes. Requested bytes are native requested storage, not RSS.

|Phase|A|B|C|
|---|---|---|---|
|growth|177.220 ms; 50,253,760 bytes|165.687 ms; 42,503,752 bytes|169.218 ms; 42,503,752 bytes|
|scan32|361.527 ms; 50,253,760 bytes|286.057 ms; 42,503,752 bytes|292.740 ms; 42,503,752 bytes|
|shrink90|675.313 ms; 4,608,280 bytes|583.185 ms; 3,827,512 bytes|567.484 ms; 3,827,512 bytes|
|shrink99|36.076 ms; 464,504 bytes|29.800 ms; 386,424 bytes|29.455 ms; 386,424 bytes|
|regrow|343.039 ms; 50,212,296 bytes|318.645 ms; 42,461,720 bytes|310.541 ms; 42,461,720 bytes|
|destroy|97.036 ms; 0 bytes|89.071 ms; 0 bytes|88.836 ms; 0 bytes|

### Admitted 10m workloads

Two fresh production processes/source; unadmitted rows are not extrapolated.

|Trace|Phase|A/B/C ms|
|---|---|---|
|append|growth|873.701 / 797.858 / 796.123|
|append|local_move|63.948 / 60.344 / 59.612|
|append|hotspot_move|278.545 / 253.577 / 252.000|
|append|uniform_move|263.528 / 245.778 / 248.453|
|append|hotspot_churn|47.381 / 49.005 / 49.378|
|append|library_destroy|363.944 / 313.251 / 307.400|
|eight|capture|291.765 / 280.969 / 281.291|
|eight|serialize|45.402 / 50.999 / 49.708|
|eight|deserialize|153.281 / 153.331 / 149.398|
|eight|restore|630.855 / 551.070 / 560.557|
|eight|source_destroy|104.693 / 84.463 / 83.033|
|eight|loaded_destroy|1.247 / 1.273 / 0.995|

## Interpretation limits

No confidence interval, universal speedup, arbitrary-duration reliability or cache/fragmentation attribution is claimed. See REPORT.md for engineering classification. `ADVERSE.csv` includes every listed workload phase whose C/A median exceeds 1.05 (excluding callback-only calibration), without deleting the original rows. The listing threshold is not a correctness/adoption threshold.
