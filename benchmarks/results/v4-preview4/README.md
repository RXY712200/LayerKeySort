# V4 Preview.4 captured workload evidence

Experimental evidence; v3.1.0 remains Stable. This directory contains the full
untuned B128 collection and `tuned-b64/` contains the full final B64 collection.
Neither raw collection was replaced by a selected subset of favorable results.

## Environment and provenance

Windows build 26300, AMD Ryzen 9 9955HX (16 cores / 32 logical processors),
GCC 16.2.0, C17 strict warnings, Release -O3 -DNDEBUG, QueryPerformanceCounter.
Three sequential production runs after short warmups; a separate instrumented
run supplies allocation/maintenance evidence. Seeds 7 and 19, 8-byte immutable
occurrence associations. Rows are individual runs, not pre-aggregated results.
Metadata retains the collection-time HEAD (Preview.3), actual working-tree
SHA-256 digests, and separately identifies the subsequently committed source.
The original collection used private B128; final production default is B64.

Trace families are independently reconstructed deterministic synthetic workloads
using archived research vocabulary, not captured real application sessions or
byte-for-byte replicas of the archived research code. A flat pointer-array oracle
uses immutable application occurrence IDs and application positions, independent
of block ranks, Path text, or snapshot ordinals. It verifies exact forward/reverse
order, resident handles, stable equality and revisions. First 64 operations,
every 1,024 and final state receive full checks; structural work bounds are
asserted on **every** diagnostic mutation. Export associations are checked row by
row, with the first export also loaded/restored against the model.

## Matrix and workload definitions

Each collection has 456 workload rows, 264 Group rows and 456 phase/reference
rows. All recorded oracles passed and all final diagnostic cleanups reached zero
live bytes/blocks. Each of the following 19 families runs at 1,024, 10,000 and
100,000 operations (three timed replays plus one diagnostic replay):

- ascending/descending: monotonically ordered comparator keys;
- random: pseudorandom keys; equal: identical keys; duplicates: 16 keys;
- alternating: low/high comparator arrivals; priority: random comparator arrivals
  under a priority-family label, not a separate captured scheduler trace;
- hotspot: explicit middle insertion;
- timeline/large_timeline: explicit insert/remove mix (one-in-five removal);
  the latter names the large-scale version of the same synthetic shape;
- local: inserts within the trailing 32 positions with the same removal mix;
- churn: alternating removal/insertion; delete_reinsert reuses the application
  item/ID but obtains a fresh resident handle;
- managed_churn: comparator-managed 16-key churn, removing the first equal item;
- local_drag: moves by 1–7 neighboring positions;
- front_back: alternating front/back moves;
- distant_drag/large_moves: distant resident-handle moves; the latter is a scale
  label, not an independently captured trace;
- hotspot_drag: repeated moves into a small middle neighborhood.

Insertion-only families start empty; other regular families start with N rows
and perform N more operations. Additional cases: 1M endpoint insertions, 1M equal
insertions, 1M long-churn operations with 10k initial rows, 100,001 resident rows
with 100k distant moves, and eight retained snapshots during 10k mutations of a
100k-row source. Export cadence none / 10k / 1k / 100 is replayed identically for
timeline, churn and distant_drag with 10k initial rows and 10k operations.

Eight fair comparator families have V3 reference replays at 10k/100k. The
reference uses the same key generation, stable upper bound and first-equal
removal; V3 removal includes locate work because it has no resident-handle move
counterpart. No artificial V3 handle-move comparison is claimed. V3 LK1 strings
and V4 LS1 ordinals encode different contracts; the V3 per-row formatting probe
also includes temporary allocation and is not a direct database benchmark.

Groups: random / all-equal, N=1,024 / 10k / 100k, three production runs and one
diagnostic run, stable build / two-source merge / batch build (chunk 1,024) /
merge-all / V4 capture / cleanup. Independent key-plus-input-ID sorted oracles
check equality precedence. `groups-corrected.csv` is the canonical final Group
collection: an earlier harness cleanup row carried the preceding comparison
counter. Original `groups.csv` files are preserved; their **cleanup comparison
column** is not valid. The correction resets that reporting counter and changes
no production behavior; all 264 corrected rows were recollected and verified.

## Interpretation boundaries

Mutation timing excludes model maintenance and validation, which still influence
cache state and scheduling. Initial build operations are included in percentiles
and mutation totals; phase/reference CSV separates build and operation totals.
RSS is whole-process working set, not isolated library memory or fragmentation.
`rss_final` is taken after fixture item/model/handle arrays are freed but **before
the latency sample buffer is freed**. Allocator retention, timer samples and OS
behavior explain why RSS need not approach zero. Requested library-byte gates
are measured after container and all retained snapshots are destroyed. Empty
live objects and explicitly retained historical snapshots are not leaks.
Production zero-valued diagnostic columns mean instrumentation is unavailable,
not that no structural work occurred. Maximum pauses are observed samples,
not worst-case guarantees. Do not infer universal speed claims from one machine.

## Final B64 totals

Median of three production runs, milliseconds; includes initial build where
present. Raw rows provide tails, phases, allocations, storage and exports.

| Family | 100k operations (ms) |
|---|---:|
| ascending | 9.554 |
| descending | 17.530 |
| random | 39.681 |
| equal | 9.714 |
| duplicates | 17.545 |
| alternating | 17.021 |
| hotspot | 11.286 |
| timeline | 33.532 |
| priority | 38.204 |
| local | 20.398 |
| churn | 24.944 |
| large_timeline | 28.912 |
| local_drag | 14.320 |
| front_back | 24.398 |
| distant_drag | 28.617 |
| hotspot_drag | 26.493 |
| large_moves | 26.371 |
| delete_reinsert | 25.635 |
| managed_churn | 39.166 |

Large timeline (100k initial + 100k operations): 28.912 ms, maximum 95 local
assignments / 2 arrays / 15 index-block updates; requested resident 7,032,864
bytes. Long churn (10k initial + 1M operations): 113.451 ms, current requested
461,584 / peak 918,240 bytes, maximum 95 assignments / 16 index updates.
All final default-B64 diagnostic traces observed maxima 160 assignments,
4 local arrays and 29 index-block updates, with at most one split and one
source-pair repair per mutation. These are observed counts with separate
per-operation structural assertions, not wall-clock bounds.

Production 1M endpoint working set median: 111,550,464 bytes; requested
instrumented library peak about 50.25 MB. Diagnostic object size exceeds normal
production core by 4,280 bytes. See allocation/storage CSVs rather than equating
instrumented RSS with normal production behavior.

The 100k storage probe separates 3,200,000 record bytes, 912,792 block-index
bytes (1,563 blocks), and 4,344 instrumented container bytes. A flat immutable
Group is 800,000 pointer-array bytes + 32 object bytes. Its associated snapshot
uses 1,600,000 row bytes, 800,000 association bytes (1,048,576 capacity), 8 marker
bytes, 1,600,026 wire bytes and 2,500,000 aggregate LS1 bytes. Group/live wire is
byte-identical. Eight retained snapshots intentionally consume 25,306,928
requested bytes in this probe; every final allocation is freed. The separate
mutation-retention trace retains 13,520,000 row bytes and 8,388,608 association
capacity bytes, with requested total 27,258,552 before cleanup.

## Export cadence, final B64

Median milliseconds, 10k initial rows + 10k operations. Capture/format/serialize
are distinct costs; load/restore/query/cleanup remain separately recorded.

| Family | Cadence | Mutation | Capture | Format | Serialize |
|---|---:|---:|---:|---:|---:|
| timeline | none | 2.082 | 0.000 | 0.000 | 0.000 |
| timeline | 10000 | 2.020 | 0.205 | 0.059 | 0.070 |
| timeline | 1000 | 2.031 | 1.381 | 0.491 | 0.572 |
| timeline | 100 | 2.186 | 13.138 | 5.082 | 5.656 |
| churn | none | 1.741 | 0.000 | 0.000 | 0.000 |
| churn | 10000 | 1.871 | 0.141 | 0.037 | 0.047 |
| churn | 1000 | 1.778 | 1.168 | 0.367 | 0.429 |
| churn | 100 | 1.981 | 10.258 | 3.834 | 4.613 |
| distant_drag | none | 1.847 | 0.000 | 0.000 | 0.000 |
| distant_drag | 10000 | 1.833 | 0.144 | 0.037 | 0.048 |
| distant_drag | 1000 | 1.890 | 1.334 | 0.411 | 0.458 |
| distant_drag | 100 | 2.228 | 10.822 | 3.933 | 4.997 |

Frequent O(N+A) full export can dominate mutation; bounded local mutations do
not make full snapshots incremental or cheap at arbitrary cadence.

## Finite capacity screen and sole tuning decision

B64/B128/B256, eight identical workload shapes at 10k and 100k; the larger
screen rotates process order across three repetitions. `capacity-large.csv`
contains all 96 larger-screen rows. At 100k B64 median totals are lower in all
eight cells by roughly 7–27% against B128; local assignment maxima roughly halve,
while requested bytes/allocation traffic rise roughly 1–3%. Outcome **B**:
sole change selects B64/minimum32 and parameterizes bulk restoration by that
same policy. No second general tuning cycle, pooling or algorithm redesign.
Full untuned and full post-change collections are both kept.

## Local release gates

GCC strict C17 Release 34/34; MSVC x64 Debug 34/34; MSVC x64 Release with symbols
34/34; MSVC x64 AddressSanitizer RelWithDebInfo 34/34; MSVC Win32 Debug 34/34.
Seven examples passed on GCC Release, MSVC Release-with-symbols and MSVC ASan.
GCC and MSVC distribution validators passed C/C++ source, offline FetchContent,
installed and standalone amalgamated consumers plus reproducible ZIP checks.
Local Clang and GCC ASan/UBSan runtime libraries were unavailable; remote jobs
are independent release gates, not implied by these local results.

Default MSVC Release investigation: one existing mutation diagnostic executable
was present immediately after successful linking, still present after two
seconds, and absent on a later check without execution. No repository/generated
post-build deletion step was found. Security products were detected, but no
accessible event proves which external actor removed it. No security protection
was disabled. The affected default configuration had a Not Run test, not a
reported test assertion failure; clean Release with `/DEBUG /INCREMENTAL:NO`
passes 34/34. This environment limitation remains explicitly disclosed.

## Reproduction and remaining questions

See `benchmarks/run_v4_matrix.py`, `benchmarks/run_v4_capacity.py`, public-API
`v4_trace.c` / `v4_groups.c`, and separate storage decomposition `v4_storage.c`.
Output collectors refuse to replace existing captured runs. Source metadata
keeps original physical file digests (Windows line endings); production-source
commit identity is recorded separately to avoid self-referential evidence hashes.

Remaining costs: per-record allocations, block headers/slack, local writes,
comparator/callback execution, full export, retained history, scratch buffers,
and process allocator retention. No resident Path/LK1 growth, global coordinate
rewrite or deferred maintenance queue is introduced. Preview.5 must explicitly
review API naming/borrows, LS1/wire compatibility freeze, integration support
and evidence acceptance. No Stable/RC, rank/select, persistent handles,
incremental export, locking, database adapter or CRDT feature is added here.
