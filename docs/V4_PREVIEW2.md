# V4 Preview.2: live movement and comparator-managed order

`v4.0.0-preview.2` is an experimental prerelease. Prefer **stable v3.1.0** for
normal use. V4 APIs are provisional; final performance acceptance is later work.
All 59 V3 functions, Path/LK1 formats and V3 ordering algorithms remain available.
This Preview builds directly on the released Preview.1 commit
`21b2902555534d10d97e704161066197cebc13ae`, without merging research history.

## Public examples and integration

Include only `layerkeysort.h` and link `LayerKeySort::layerkeysort`.
The current branch/package exposes version `4.0.0-preview.2`; CMake numeric
package version remains 4.0.0. See [integration](INTEGRATION.md).

```sh
cmake -S . -B build -DLKS_BUILD_BENCHMARKS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/layerkeysort_order_example
./build/layerkeysort_managed_order_example
```

For Visual Studio use `--config Release`, CTest `-C Release` and executables
under `build/Release/`. [Explicit example](../examples/live_order.c) demonstrates
handle-preserving movement. [Managed example](../examples/managed_order.c)
demonstrates comparator binding, equal insertion, locate, cursor traversal and
exact removal. CI explicitly runs these and all three retained V3 examples.

## Explicit movement

```c
LksStatus lks_order_move_front(LksOrder *, const LksOrderHandle *handle);
LksStatus lks_order_move_back(LksOrder *, const LksOrderHandle *handle);
LksStatus lks_order_move_before(LksOrder *, const LksOrderHandle *handle,
                              const LksOrderHandle *anchor);
LksStatus lks_order_move_after(LksOrder *, const LksOrderHandle *handle,
                             const LksOrderHandle *anchor);
```

Both handles must be live in the same explicit container. Moves preserve their
addresses, item associations and resident count. No cross-container move exists.
Business identity remains caller-owned. Handles are not Paths, serialized keys,
persistent IDs or pointer-value order labels. Own removal/container destruction
expires a handle; dangling-handle detection is not provided.

Self moves, already-adjacent relative moves and already-at-endpoint moves return
OK with **no allocation, structural maintenance or revision increment**. They
remain OK at revision capacity. An actual move increments revision exactly once;
failed moves preserve it. Revision exhaustion returns CAPACITY_LIMIT before
mutation; no wrap is permitted.

Same-block moves shift only the required local interval and update its indices;
they allocate nothing, leave occupancy unchanged and do no AVL work. Cross-block
moves prepare one spare block only if the destination is full, before unlinking
the resident. Commit inserts relative to the stable anchor, performs at most one
destination split, then restores source occupancy using post-insertion neighbors.
At most one source pair merge/redistribution is needed. It can involve a newly
split destination. Final resident locations, thread links, heights and subtree
block counts are updated without allocation or callbacks during commit.

The existing B128 architecture is unchanged: multiple blocks contain 64..128
residents each, sole block 1..128, split 129 into 64/65. A cross-block move touches
at most four local arrays. Structural work is O(B + log(1+M)), independent of the
number of intervening residents, for M blocks. Allocator/callback/OS latency is
not bounded. No global coordinate relabel, distance scan, split cascade, global
normalization or deferred maintenance queue exists.

## Comparator-managed facade

`LksManagedOrder` communicates comparator-managed order without suggesting a
second Tree/index architecture. It is distinct from V3 `LksOrderedTree`.

| API | Purpose |
|---|---|
| `lks_managed_order_create`, `destroy`, `size` | Own facade and its private core |
| `lks_managed_order_insert` | Stable upper-bound insertion |
| `lks_managed_order_locate` | Strict predecessor, first equal, strict successor |
| `lks_managed_order_remove` | Exact residence removal |
| `lks_managed_order_first`, `last` | Logical endpoints |
| `lks_managed_order_compare` | Residence precedence, not value equality |

Next/previous/item access reuse `lks_order_next`, `lks_order_previous` and
`lks_order_item`. No duplicate record/wrapper allocation is required. Managed
handles cannot be arbitrarily moved: explicit mutation requires their owning
`LksOrder *`, and the facade never exposes that mutable private core.

The facade exclusively owns one ordinary explicit core. Creation copies the
`LksComparator` descriptor and borrows its callable callback/context. They must
remain valid with consistent total-preorder semantics for the facade lifetime.
Comparator-visible resident fields must not change. Use **remove, modify,
reinsert** for key changes; these are separate operations, not an atomic payload
transaction. Items remain borrowed and are never freed by the library.

Upper bound searches AVL block maxima for the first block strictly greater than
the query, then binary-searches inside that block. If none exists, insertion
appends. Equal-only blocks are skipped by balanced search, never successor scans.
Equal occurrences retain insertion order. Lower bound similarly finds the first
block whose maximum is greater than or equal to the query.

Locate requires three distinct output pointers, all initialized NULL. It returns
OK with first equal (or NULL if missing), the residence immediately before the
lower bound, and the first strictly greater residence. For a duplicate run these
are strictly lower/greater neighbors, not the second equal occurrence. Query may
be nonresident. Empty lookup returns OK with three NULLs.

Insertion search costs at most AVL height plus a B-sized binary search in
comparator calls. Locate uses lower and upper searches plus equality verification:
O(log(1+M) + log B) calls, multiplied by comparator cost. These bounds describe
this implemented search, not realtime or V3 complete-insertion guarantees.
Exact handle removal uses the explicit core, invokes no comparator and allocates
nothing. Inconsistent comparators/changed payloads invalidate logical ordering
guarantees; structural maintenance still uses bounded arrays, not comparator keys.

## Callback safety and status behavior

No comparator is called once structural commit begins. Same-facade callback
reentry, including reads, is rejected: status APIs return REENTRANT; pointer/size
getters return NULL/zero; destroy is ignored while busy. Shared handle getters
and cursor advancement also reject that core's comparator-time access. Independent
containers are allowed. Callbacks must return normally, without `longjmp` or
crossing the C boundary with an exception. Comparator errors/OOM are not part of
the `LksCompareFn` model, which returns only negative/zero/positive.

The guard is not a mutex. Callers must serialize access against mutation and
destruction and synchronize comparator/context/payload access appropriately.
INVALIDATED and REENTRANT statuses are appended without renumbering old values.
Insert output is required and initialized NULL; remove output is optional and
initialized NULL; compare output is required and initialized zero. Invalid
arguments use INVALID_ARGUMENT, OOM uses OUT_OF_MEMORY. NULL destruction is
harmless; NULL getter/size access returns NULL/zero. Foreign live handles are
rejected; use after handle/container destruction is a caller lifetime error.

## Revision-checked cursor

`LksOrderCursor` is opaque and allocated once. The caller owns/destroys it.
`lks_order_cursor_create(order, reverse, &cursor)` and
`lks_managed_order_cursor_create(managed, reverse, &cursor)` share one
implementation; reverse is exactly 0 or 1. Creation initializes output NULL,
captures revision and begins at first or last. Cursor allocation failure does
not mutate the container.

`lks_order_cursor_next(cursor, &handle)` advances in the selected direction and
returns OK/NULL at end. It checks revision **before inspecting its cached handle**:
actual insert/remove/move causes INVALIDATED/NULL, including removal of that
cached resident. Failure, successful no-op move and queries preserve the cursor.
Returned handles retain ordinary residence lifetime independently of the cursor.
`lks_order_cursor_destroy(NULL)` is harmless.

The cursor borrows its container and **cannot outlive it**. Revision checking is
not destroyed-container detection. Raw logical neighbors remain available when
applications control mutation explicitly (save next before removing current).

## Validation and diagnostics

[Mutation tests](../tests/v4_mutation.c) add an independent flat sequence model:
three seeds, 30000 mixed insert/remove/move steps each, private invariant checks
every step, frequent exact forward/reverse/reference association checks and
sampled pair comparisons. A 100001-resident campaign performs 2000 distant moves.
Targeted tests cover local directions, all no-op forms, adjacent full-destination
split/source merge, source redistribution, foreign handles and revision capacity.

Managed reference campaigns cover seven distributions: ascending, descending,
random, all equal, low-cardinality duplicates, alternating and hotspot values.
All-equal size is 100001; others are 4096 each. Exact reference occurrence order
is checked, not just sortedness. Another 5000 remove/change-key/reinsert cycles
exercise stable insertion and cursor invalidation. Removal is tested with next
allocation failing and comparator-call counts unchanged.

Per-search comparator assertions allow height+8 for insertion and 2*height+17 for
locate. Reentry tests invoke nested insert/locate/remove/compare/getters/cursors/
destroy and verify rejection. OOM sweeps cover spare preparation, managed facade
and core creation, empty/nonfull/split insertion and cursor creation. Failed
full-destination move preserves exact block bytes, all handles, revision, order,
live allocation totals and existing cursor. Nonallocating moves/removal consume
zero allocator attempts. Tests require zero live allocations/bytes at completion.

Private counters track location assignments, local arrays, distinct modified
index/thread blocks, splits/repairs/rotations, no-op and cross-block moves.
Counters are absent from the normal library. A move may update at most 4B
locations and four local arrays; index work follows bounded AVL ancestor paths.
No public diagnostics ABI exists.

Local acceptance passed GCC 16.2 strict C17 warnings (`-Wall -Wextra -Wpedantic
-Werror`), MSVC 19.51 x64 Release and MSVC x64 AddressSanitizer: **CTest 16/16**
in each configuration, including the unchanged Preview.1 and V3 regressions.
No ASan findings occurred. Clang and GCC ASan/UBSan runtimes were unavailable
locally; remote CI is a separate publication gate. All five public examples and
C/C++ source, offline FetchContent, installed-package and reproducible generated
amalgamation consumers passed. The public header retains all 73 Preview.1
signatures, adding 17 functions for a total of 90.

The explicit oracle measured 42186 move operations (41113 cross-block, 20 no-op),
with maxima of **319 location assignments, four local arrays and 12 index/thread
blocks**. The separate random-move screen observed 20 modified index/thread
blocks, so the combined observed maximum is 20. All operations satisfied the
asserted bounded-work envelopes. Managed campaigns observed at most 14 comparator
calls for one insertion and 37 for one locate; height-based bounds, not these
machine-specific maxima, are asserted. Final live allocations and bytes were zero.

## Preliminary screen

With benchmarks ON, `layerkeysort_v4_mutation_smoke` measures production timing
and comparator calls. `layerkeysort_v4_mutation_diagnostics` separately records
structural and allocation counts. Its timings include instrumentation and must
not be compared directly with production timings. Zero structural/allocation
columns in production output mean unavailable, not zero work.

Workloads use seed 17 and 20000 residents/operations: repeated local/distant/random
moves, alternating removal/reinsertion, and managed ascending/random/equal/
duplicate/alternating insertion. Argument `smoke` selects fixed smaller CI sizes.
Output gives summed operation time, nearest-index sample p50/p99/observed maximum,
comparator calls, assignments, local-array touches, index writes, splits/repairs,
per-operation maxima and library allocations. Clock/check overhead is included.
Windows uses QPC; C17 TIME_UTC fallback is not guaranteed monotonic. Allocations
exclude application-owned harness buffers. No comparative superiority, realtime
guarantee or final performance acceptance is inferred from one machine's run.

One Windows x64 GCC 16.2 Release capture is preserved under
[v4-preview2-smoke](../benchmarks/results/v4-preview2-smoke/). Production totals
for 20000 measured operations were 0.597 ms local moves, 1.940 ms distant moves,
3.362 ms random moves and 1.750 ms alternating remove/reinsert. Managed totals
for 20000 inserts were 2.060 ms ascending, 5.082 ms random, 2.063 ms equal,
3.072 ms duplicates and 2.291 ms alternating. The equal workload used 147351
comparator calls in total. Structural/allocation evidence is in the separate
diagnostic CSV. These are one-run observations with timer overhead; sub-resolution
samples can report zero and do not imply a zero-cost operation.

## Deferred scope

Snapshots/exported keys, domains/namespaces, persistence/save/load, source-marker
snapshot lifetimes, V3 LK1 migration, V4 Group/Batch and incremental export remain
Preview.3 or later, subject to its explicit scope. Public rank/select, internal
locking and final API/wire freeze are also absent. No such future facility is
implied by live cursor revisions or resident handles. Final performance acceptance
remains later Preview work.

See the unchanged [Preview.1 record](V4_PREVIEW1.md),
[architecture](design/V4_ARCHITECTURE.md) and [decisions](design/V4_DECISIONS.md).
The Preview.1 source record retains its original pre-publication wording; that
milestone is published as [v4.0.0-preview.1](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0-preview.1).
