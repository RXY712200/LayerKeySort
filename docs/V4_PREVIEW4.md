# V4 Preview.4 — flat immutable integration and workload convergence

Experimental `v4.0.0-preview.4`; **v3.1.0 remains the recommended Stable release**.
This milestone integrates Groups with the selected contextual live-order and
historical-snapshot architecture. It does not freeze V4 API names, LS1 or wire
compatibility. Preview.5 must explicitly decide those contracts.

## Representation and ownership

`LksImmutableGroup` owns an object, a flat array of borrowed non-NULL item
pointers, count, a callback guard and an optional lazy source marker. It owns no
Path, Tree, live record/block, AVL or mutable order. On this x64 GCC build the
object is 32 bytes; a nonempty Group owns exactly N pointer slots (8N bytes).
Application payload and allocator overhead are additional. Duplicate pointers
are legal occurrences. Destruction never destroys items.

V3 `LksGroup` and `LksGroupBatch` remain unchanged, including their Paths. The
new types are separate additive provisional APIs, not aliases or conversions.
There are 121 public functions: 59 preserved V3 + 62 provisional V4. No V3
signature, status value, Path rule or published LK1 byte encoding changes here.

## Group contract

```c
LksStatus lks_immutable_group_build(void *const *items, size_t count,
    const LksComparator *comparator, LksImmutableGroup **out_group);
void lks_immutable_group_destroy(LksImmutableGroup *group);
size_t lks_immutable_group_size(const LksImmutableGroup *group);
void *lks_immutable_group_item_at(const LksImmutableGroup *group, size_t index);
LksStatus lks_immutable_group_merge(const LksImmutableGroup *base,
    const LksImmutableGroup *incoming, const LksComparator *comparator,
    LksImmutableGroup **out_group);
```

Build copies the input pointer array and uses stable O(N log N) merge sorting;
the caller array and items remain unchanged. Equality preserves input occurrence
order. Empty Group is valid (`items == NULL`, count zero). A comparator callback
and output pointer are required even for an empty build.

Merge allocates a fresh result array, takes O(Nbase + Nincoming) work and leaves
both sources unchanged. Comparator-equal Base occurrences precede Incoming;
within-source order is preserved. Merging a Group with itself is legal and
produces two occurrences of each item under that same precedence rule.

**Comparator choice:** the descriptor is copied for the duration of an operation,
not retained in the result. Callback code/context and item payload remain owned
by the application. Merge requires ordering semantics compatible with both
source sequences. Reads perform no comparator calls or hidden compatibility
scan. Changing comparator-relevant payload after construction does not reorder
the Group; subsequent compatible merge is the caller's responsibility.

Size/item reads are O(1). Invalid/NULL/busy Group reads return zero/NULL, and
out-of-range item access returns NULL. A successful item read borrows the
application pointer; its lifetime is controlled by the application. Array index
is a read position, not permanent application identity.

## Batch contract

```c
LksStatus lks_immutable_group_batch_build(void *const *items, size_t count,
    size_t group_size, const LksComparator *comparator,
    LksImmutableGroupBatch **out_batch);
void lks_immutable_group_batch_destroy(LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_size(const LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_group_count(const LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_group_size(const LksImmutableGroupBatch *batch);
const LksImmutableGroup *lks_immutable_group_batch_group_at(
    const LksImmutableGroupBatch *batch, size_t index);
LksStatus lks_immutable_group_batch_merge_all(const LksImmutableGroupBatch *batch,
    const LksComparator *comparator, LksImmutableGroup **out_group);
```

Group size must be nonzero. Consecutive input chunks are independently
stable-sorted. Batch owns its pointer table and Groups; borrowed chunk access
expires when Batch is destroyed. **Never destroy a borrowed chunk yourself.**
Total item count, configured group size and chunk count are distinct getters.
NULL/busy Batch getters return zero/NULL. Empty Batch is valid and has no chunks.

Merge-all concatenates the sorted runs, then performs stable pointer-only
pairwise passes in O(N log K), with one N-pointer scratch array for K > 1.
Earlier chunks precede later chunks for equality. No Path/index is constructed
between passes. The fresh result outlives Batch; source Groups remain unchanged.

## Shared historical export

```c
LksStatus lks_immutable_group_snapshot_capture(const LksImmutableGroup *group,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot);
int lks_immutable_group_snapshot_is_current(const LksImmutableGroup *group,
    const LksSnapshot *snapshot);
```

Capture traverses the flat array once, invokes no comparator, and uses the
same private capture transaction as live order: O(N + association bytes +
callback work). Namespace, associations, LS1 and `LKS4SNP1` are exactly the
[Preview.3 format](V4_PREVIEW3.md), not a second Group wire protocol.
Identical ordered items/namespace/association bytes produce byte-identical wire
and LS1 across live and Group sources.

Group revision is fixed at zero. First successful capture attaches a lazy
atomic-reference-counted source marker only after all rows are ready. Subsequent
captures retain it. Failed first capture leaves the marker absent; failed retain
publishes no snapshot. A captured snapshot is current against its existing
Group; loaded snapshots carry no marker and are not current. Historical rows and
copied associations remain readable after Group destruction, with no item pointer
or source address retained. Never query against an already destroyed Group.

Snapshot captures are logically read-only but modify private marker/guard state;
serialize operations on the same Group externally. There is no mutable-container
thread-safety promise. Concurrent immutable snapshot reads require externally
protected lifetime. Independent snapshots may release a shared marker concurrently.

## Failure and callback safety

All output-owning calls set a supplied output to NULL before validation/fallible
work. Invalid arguments/unrepresentable sizes return INVALID_ARGUMENT; allocation
failure returns OUT_OF_MEMORY. Marker exhaustion returns CAPACITY_LIMIT. Prepare
all new storage before publication; failures leave source Groups/Batch usable.
Allocator fault-injection sweeps cover empty/nonempty builds, sort scratch,
objects/arrays, every partial Batch chunk, merge scratch, capture row/association
storage, lazy marker and marker retain capacity. All failed attempts restore live
allocation bytes; successful final cleanup restores both bytes and blocks to zero.

Merge guards both sources during comparator callbacks; merge-all guards Batch
and every chunk. Capture guards its source during association callbacks. Same-source
capture/merge recursion returns REENTRANT. Busy reads return zero/NULL. Destroying
a busy source is ignored; destroying a Batch while one borrowed chunk is busy is
also ignored. Unrelated operations may proceed under normal ownership rules.
No global locks, exception/longjmp recovery or detection of dangling pointers is
provided. Callback output spans must remain valid for immediate copying after
return; callback-local automatic arrays are not valid association storage.

## Production evidence and methodology

[Captured evidence](../benchmarks/results/v4-preview4/README.md) contains the full
untuned matrix, separate production timings and diagnostic work/allocation rows.
Reproduce with strict Release build, `LKS_BUILD_BENCHMARKS=ON` and optional private
`LKS_BUILD_CAPACITY_SCREEN=ON`, then:

```sh
python benchmarks/run_v4_matrix.py --build build --output build/new-v4-evidence
```

The output directory must not already contain runs.jsonl. No research ancestry
is merged. Traces are newly implemented deterministic application-shaped
synthetic workloads, using the family vocabulary of the archived
[Phase 1/2 report](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ARCHITECTURE_PHASE2.md).
They are not captured real-user/editor sessions and are not byte-for-byte
reproductions of archived research traces. Definitions are in the evidence guide.

The independent flat model uses immutable occurrence IDs, predicted upper-bound
ordering or explicit application positions. It checks count, exact forward/reverse
sequence, stable equality, resident handle identity and revision behavior. Private
diagnostic invariants run after early operations, every 1,024 operations and final
state; per-operation structural bounds are checked on every mutation. Exports
verify every association row; first export also load/restores the exact sequence.
Failed oracles abort rather than record timing evidence.

Windows timing uses QueryPerformanceCounter; POSIX uses CLOCK_MONOTONIC. Three
measured runs follow short warmups. Timers exclude model maintenance/verification,
which still affect caches and scheduling; diagnostics never predict production
latency. Percentiles include initial population construction when present; separate
build/operation totals are recorded. Small operations near timer quantization are
not a basis for tiny percentage claims. All maxima are observed, not hard latency
guarantees. RSS is full-process working set, including application arrays and
allocator behavior; it does not isolate fragmentation. Final RSS still includes
the latency sample buffer; the evidence guide explains the exact measurement point.

## Sole tuning pass: B128 to B64

The predefined B64/B128/B256 screen used identical eight-workload matrices at
10k and 100k, with rotated capacity order for the larger screen. At 100k B64 had
lower median totals in all eight cells, approximately 7–27% below B128, with
maximum local assignments roughly halved. Requested live bytes and allocation
counts increased approximately 1–3% in these cells. This is the broad material
tradeoff required before changing the selected private policy.

**Outcome B — one bounded tuning applied:** select B64, with multi-block minimum
occupancy 32. No record pool, new structure, comparator/Group algorithm or export
buffer tuning was introduced. Private bulk restoration follows the same capacity
so 65 rows form 33/32; persisted LS1/wire do not encode physical blocks. The
original B128 matrix and full post-change matrix are retained independently.
Tests now express private occupancy/OOM fixtures in terms of capacity/minimum;
public semantics and wire goldens are unchanged. No second general tuning pass.

## Suitability and Preview.5 decisions

The selected live representation has no Path/LK1 or resident coordinate growth,
global coordinate rewrite, tombstone registry or deferred maintenance queue.
Each ordinary mutation changes bounded local arrays plus logarithmic block-index
metadata; comparator work, record allocation and local pointer writes remain
costs. A distant move uses resident handles, not a distance scan. These structural
facts do not promise constant wall-clock latency or an overall worst-case O(log N)
API bound independent of callbacks, allocation and platform effects.

Snapshots have explicit O(N+A) capture/export cost and retain historical rows;
frequent full exports may dominate application work. Retaining eight historical
snapshots intentionally multiplies storage. Requested library bytes exclude item
payloads and allocator overhead. Empty live objects still occupy their object and,
after capture, marker; **zero live allocations means after container destruction**,
and after explicitly retained snapshots are destroyed. Never mistake deliberate
retention or a still-live empty object for a leak.

Known costs include separate record allocations, block headers/slack, local
reindex writes, comparator calls, snapshot row metadata/capacity slack, external
LS1 strings, Group sort/merge scratch, process allocator retention and cleanup.
V3 comparisons are limited to fair stable comparator and immutable Group/Batch
counterparts. V3 has no resident-handle move counterpart. LK1 Path coordinates and
LS1 snapshot ordinals persist different contracts; encoding sizes alone are not
an end-to-end database comparison.

Preview.5 must decide provisional Group/Batch names and V3 presentation,
callback/borrow compatibility, LS1/wire freeze and migration obligations,
supported integration configurations, reproducible evidence review and any
material acceptance blockers. This milestone does not add public rank/select,
locking, owned identity, persistent handles, incremental export, DB adapters,
CRDT, transactions, automatic namespaces, Stable or RC status.

## Verified local release gates

GCC strict Release, MSVC x64 Debug, Release with symbols, AddressSanitizer, and
Win32 Debug each passed CTest 34/34. All seven examples passed on GCC Release,
MSVC Release with symbols and MSVC ASan. C/C++ source, offline FetchContent,
installed and amalgamated consumers and reproducible distribution checks passed
on GCC/MSVC. Local Clang and GCC sanitizer runtimes were unavailable.

Default MSVC Release showed delayed disappearance of one linked executable
without execution; the external actor is unconfirmed. No repository deletion
step or accessible security event identified a cause. Release with debug linking
metadata passed all tests; security protections were unchanged. This limitation
and corrected Group cleanup-counter evidence are detailed in the evidence guide.
