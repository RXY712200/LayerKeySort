# LayerKeySort V4 usage

[简体中文](USAGE.zh-CN.md)

Full v4.2.0 supports five complementary workflows. Use one family or combine them; items and business identities remain application-owned.

| Work | API | Example |
| --- | --- | --- |
| Explicit relative editing and precedence | `LksOrder` | [basic.c](../examples/basic.c) |
| Comparator-maintained stable order and lookup | `LksManagedOrder` | [managed_order.c](../examples/managed_order.c) |
| Stable one-shot pointer-array sorting | `lks_sort` | Minimal example below |
| Immutable sorted batches and stable merge | `LksGroup`, `LksGroupBatch` | [immutable_group.c](../examples/immutable_group.c) |
| Historical associations, LS1, wire and restore | `LksSnapshot` | [snapshot.c](../examples/snapshot.c) |

Explicit order suits editors, timelines and playlists. Managed order suits incrementally maintained priorities and comparator-based collections. Groups suit sorted chunks and batch aggregation. Sorting needs no live container. A database can use these ordering components while retaining its own transactions, identity and storage. Distributed conflict resolution requires a separate mechanism.

## Stable array sorting

```c
#include <layerkeysort.h>
static int compare_int(const void *a, const void *b, void *context) {
    const int x = *(const int *)a, y = *(const int *)b;
    (void)context;
    return (x > y) - (x < y);
}
int main(void) {
    int values[] = {3, 1, 1};
    void *items[] = {&values[0], &values[1], &values[2]};
    return lks_sort(items, 3, compare_int, NULL) == LKS_STATUS_OK ? 0 : 1;
}
```

Equal pointers-to-items retain input order. `lks_sort` uses a copied pointer array and an equally sized scratch array before committing the caller's array. OOM preserves the input; fewer than two items needs no allocation, but still requires a comparator. This is a stable sorting convenience with copy/scratch costs; the live-order measurements do not compare it with `qsort` or `std::sort`.

## Choose the API family

### `LksOrder`

Use explicit relative ordering when the application decides where an occurrence belongs.

It supports endpoint/relative insertion, remove, move, traversal and same-source
contextual comparison.

Handles survive unrelated mutations and moves. Never use a handle after its own removal
or source destruction.

### `LksManagedOrder`

Use comparator-managed order when placement is derived from application comparison data.

Keep comparator context and comparator-visible resident data stable. To change a key:
Every affected occurrence in every relevant ManagedOrder must be removed or otherwise safely handled before changing comparator-visible data. Account for other sorted structures borrowing the same item; immutable Groups require application-managed reconstruction or replacement if their ordering prerequisites would be violated. Removal immediately expires that occurrence’s Handle. Reinsertion can fail: remove/edit/reinsert is not an atomic library transaction, and application recovery is the caller’s responsibility. See the [complete mutation contract](IMPLEMENTATION_API_REFERENCE.md).

### `LksGroup` / `LksGroupBatch`

Use immutable flat groups for stable batch ordering, merging and chunked build workflows.

Groups copy pointer arrays but borrow item payloads. Borrowed Batch group views expire
with the Batch; merge results are independently owned.

If shared payload mutation invalidates GroupBatch sorted chunks, the application must reconstruct or replace the Batch. Borrowed Batch-owned Group views cannot be independently replaced or destroyed and remain valid only while their Batch lives.

### `LksSnapshot`

Capture when historical or persistent order is needed.

Snapshots copy namespace and application association bytes. Serialize to caller storage,
deserialize strictly, resolve associations back to caller items, and restore a fresh live
order with fresh handles.

Loaded snapshots have no former-source provenance/currentness.

## Identity and lifetime rules

Applications own item payloads and business identity.

A live `LksOrderHandle` is not:

- a business ID,
- a persistent key,
- a serialized coordinate,
- something to reuse after removal.

An LS1 key is a historical snapshot key, not a live position.

Keep application items alive while resident. Keep comparator contexts alive for the
container lifetime. Destroy cursors before their source.

## Performance expectations

V4 avoids live Path growth and collection-wide coordinate relabel, but does not make all
work free.

Important costs include:

- resident allocation,
- bounded local pointer/block maintenance,
- comparator/callback cost,
- full snapshot/export `O(N+A)`,
- retained historical snapshot memory.

No universal hard latency or universal speedup is promised.

## Measured workload guidance

The [4.2 evidence](FULL_RC1_EVIDENCE.md) reports matched Windows/Linux live, lifecycle and generated application workloads. Use the [selection guide](FULL_SELECTION_GUIDE.md) for resource tradeoffs and contrary cases. The [4.1 record](V4_1_0.md) retains the separate 100k–10m storage study.

Current roadmap: [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).

[API](API.md), [compatibility](COMPATIBILITY.md) and [freeze](V4_FREEZE.md) define exact
buffers, statuses, ownership, reentry and persistent compatibility.
[Migration](MIGRATION_V3_V4.md) covers V3 Path/Tree workflows.
