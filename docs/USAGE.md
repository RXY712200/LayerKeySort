# V4 usage

Include only `<layerkeysort.h>`. Start with [basic.c](../examples/basic.c), then
[managed_order.c](../examples/managed_order.c), [immutable_group.c](../examples/immutable_group.c)
and [snapshot.c](../examples/snapshot.c). All use public headers only.

Keep application items alive while resident, comparison data/context stable.
Group copies pointer array, not payload. Remove never frees item. Destroy cursor
before source. Never use handle after own removal/source destruction; unrelated
mutation/moves preserve it. Never destroy borrowed Batch chunk. Merge results and
snapshots are independent owned objects, destroy separately. Loaded snapshot never current.

Explicit relative operations suit layers/playlists/editors. ManagedOrder maintains
comparison-defined sequence: remove/edit/reinsert to change key. Capture when
historical order needed, copying application identity associations/namespace.
Serialize to caller buffer, deserialize strict wire, resolve identities and restore
fresh live handles. No automatic ID, live key persistence or incremental export.

[API](API.md)/[freeze](V4_FREEZE.md) define buffers, errors, reentry and lifetimes.
[Migration](MIGRATION_V3_V4.md) replaces old Path workflows.
