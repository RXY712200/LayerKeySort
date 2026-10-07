# LayerKeySort V4 usage

Include only `<layerkeysort.h>`.

Start with [basic.c](../examples/basic.c), then
[managed_order.c](../examples/managed_order.c),
[immutable_group.c](../examples/immutable_group.c) and
[snapshot.c](../examples/snapshot.c).

## When to use LayerKeySort

LayerKeySort is designed for **long-lived collections whose order keeps changing**.

Typical fits include:

- layer stacks,
- timelines,
- editor object lists,
- node/workflow editors,
- playlists,
- local document/editor ordering.

Use it when repeated insert/remove/move behavior and stable live residence matter more
than one-shot sorting.

## When not to use it

Prefer a simpler tool when you only need:

- one-shot sorting of an array,
- a simple database `position` column,
- distributed/CRDT conflict resolution,
- a persistent business-identity registry,
- database/WAL/transaction ownership.

LayerKeySort manages ordering, not all surrounding application state.

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
remove → edit → reinsert.

### `LksGroup` / `LksGroupBatch`

Use immutable flat groups for stable batch ordering, merging and chunked build workflows.

Groups copy pointer arrays but borrow item payloads. Borrowed Batch group views expire
with the Batch; merge results are independently owned.

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

## Next evidence priority

The synthetic/application-shaped benchmark matrix is extensive. The next valuable
evidence is real editor integration and implementation-independent real workload traces.

Current roadmap: [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).

[API](API.md), [compatibility](COMPATIBILITY.md) and [freeze](V4_FREEZE.md) define exact
buffers, statuses, ownership, reentry and persistent compatibility.
[Migration](MIGRATION_V3_V4.md) covers V3 Path/Tree workflows.
