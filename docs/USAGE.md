# Using LayerKeySort

This guide describes the unreleased V3 Stable candidate header. The latest
published V3 prerelease is `v3.0.0-rc.1`; for a published Stable release,
pin `v2.0.0`. See [V3 migration](V3_MIGRATION.md).

## Requirements

- C17.
- A C17 compiler and CMake 3.21 or newer for the CMake build.

## Adding LayerKeySort to a project

The CMake target `layerkeysort` is a reusable static library and exposes the
public include directory. The repository does not provide an installed package
or package-manager recipe. For source-based integration, compile the production
C files and add `include/` to the compiler search path.

The public header is `include/layerkeysort.h`. Include it as:

```c
#include "layerkeysort.h"
```

Set the compiler include search path to the repository’s `include/` directory. The production implementation files currently are:

- `src/lks_alloc.c`
- `src/lks_base.c`
- `src/path.c`
- `src/path_text.c`
- `src/path_order_key.c`
- `src/slot_codec.c`
- `src/path_compare.c`
- `src/gap.c`
- `src/tree.c`
- `src/group.c`
- `src/sort.c`
- `src/bulk.c`

The private headers under `src/` are implementation details. The Visual Studio
project also provides a combined validation executable; use the CMake library
target for an application build.

## First use: managed Tree

[`examples/ordered_tree.c`](../examples/ordered_tree.c) is a complete,
public-header-only example. It creates a comparator-bound Tree, inserts three
caller-owned items, locates one by a query value, removes it by its borrowed
Path, locates the new neighbors, and destroys the Tree. It reacquires query
results after removal rather than using an invalidated node pointer.

```sh
cmake -S . -B build -DLKS_BUILD_TESTS=OFF
cmake --build build --target layerkeysort_ordered_example
./build/layerkeysort_ordered_example
```

For Visual Studio generators, run the executable under `build/Debug/` after a
Debug build. The Tree copies the comparator descriptor but borrows its context
and the inserted items. Keep both alive and keep comparator-relevant fields
unchanged while an item is resident. See [integration](INTEGRATION.md) when
using the library from another project.

## Sorting a pointer array

Define a comparator over your pointed-to item type, then call:

```c
LksStatus status = lks_sort(items, item_count, compare_items, NULL);
```

`lks_sort` changes the pointer array, not the pointed-to objects. It is stable
for comparator-equal items and leaves the input array unchanged if allocation
fails. [`examples/basic.c`](../examples/basic.c) is the smaller sort-only example.

To build and run it with CMake:

```sh
cmake -S . -B build
cmake --build build
./build/layerkeysort_example
```

The executable path can differ with multi-configuration generators such as
Visual Studio (for example, `build/Debug/layerkeysort_example.exe`).

## Dynamic layer-list example (V2 Preview.5 origin)

[`examples/layer_list.c`](../examples/layer_list.c) models Background, Player,
HUD, and Effects as caller-owned objects with stable application IDs. It uses
explicit Path coordinates to insert Effects between Background and Player,
rekeys HUD between Effects and Player, removes Effects, then formats and
round-trips HUD's `LK1:` coordinate. The Tree borrows item pointers; the
application owns its Path copies and destroys them after the Tree.

Build target `layerkeysort_layer_list_example` and run it after the normal
CMake build. The example orders its application-side Path associations with
`lks_path_compare()`; physical AVL navigation is not logical item order.
The simple [`basic.c`](../examples/basic.c) remains the `lks_sort()` example.
The [integration guide](INTEGRATION.md) covers consuming the library.

## Comparator

Provide an `LksComparator` whose `compare` callback follows this convention:

- A result below zero means the left item sorts before the right item.
- Zero means they are equal under the selected comparison rule.
- A result above zero means the left item sorts after the right item.

The `context` pointer may be `NULL`. LayerKeySort passes it to the callback without interpreting it, so the caller can use it for comparison configuration. Avoid subtraction-based integer comparisons when overflow is possible; compare with explicit less-than and greater-than checks.

## Building a Group

A Group sorts supplied borrowed pointers and assigns Path coordinates. This
is an advanced API; ordinary array sorting needs only `lks_sort`.

```c
LksComparator comparator = { compare_item, NULL };
LksGroup *group = NULL;
LksStatus status;

status = lks_group_build(items, item_count, &comparator, &group);
if (status != LKS_STATUS_OK) {
    /* Handle the error. */
}

/* Read the ordered Group, then release its structure. */
lks_group_destroy(group);
```

`items` is an array of pointers to caller-owned objects. The comparator follows
the same contract as `lks_sort`.

## Building and merging a GroupBatch

`lks_group_batch_build` partitions the input sequence into consecutive chunks of at most `group_size` items and sorts each chunk into a Group. It does not randomize or reorder the chunks before building them. `lks_group_batch_merge_all` merges all constituent Groups into a new independent result Group; the Batch remains readable until it is destroyed.

Groups supplied to `lks_group_merge`, and constituent Groups supplied to
`lks_group_batch_merge_all`, must have been ordered under comparison semantics
compatible with the merge comparator and its context. The library does not
infer comparator equivalence from function pointers or context addresses.

```c
LksGroupBatch *batch = NULL;
LksGroup *result = NULL;

status = lks_group_batch_build(items, item_count, group_size,
    &comparator, &batch);
if (status == LKS_STATUS_OK) {
    status = lks_group_batch_merge_all(batch, &comparator, &result);
}
/* Check status, read result, then destroy result and batch. */
```

The Batch and its Groups remain readable after a successful merge.

## Reading Path text

`lks_path_text_length(path)` returns the character count excluding the null
terminator. Supply at least one additional byte to `lks_path_format`. ZERO is
`000`; a positive Path with slot 32768 at level zero formats as `0DEq`.
Appending slot 0 at level five gives `0DEq/5222`, where `5` is the level
delta. Ordinary next-level steps omit the numeric prefix. Each slot is three
radix-54 characters from the case-sensitive ASCII alphabet documented in
[`API.md`](API.md); `/` separates steps and never counts levels. The
formatter writes canonical current output. Since V2 Preview.4,
`lks_path_parse()` accepts exactly that text and returns a caller-owned Path.
For storage and bytewise ordering, use the separate versioned
`lks_path_order_key_*()` API. **Use `lks_path_compare()` to order Paths; do not
use `strcmp` on complete formatted text.** The canonical display grammar is
part of the 2.x contract; generated coordinates may change. Do not persist
Path text as an item ID.

For example, the positive level-zero, slot-zero Path has display text `0222`
and durable key `LK1:201FF0000!`. Canonical keys of the same version sort
under `strcmp()` or a database collation preserving bytewise ASCII order.
The parsed Path belongs to the caller and must be destroyed with
`lks_path_destroy()`. Invalid or noncanonical inputs fail with NULL output;
allocator failure also leaves output NULL. The exact key grammar and
cross-platform overflow rule are in [API.md](API.md). These APIs were added in
V2 Preview.4; V2 Preview.3 did not contain them.

The key persists a Path coordinate and can order canonical same-version keys
under bytewise ASCII collation. It does not save a Tree, reconstruct its AVL
shape, save caller item payloads, or assign permanent item IDs. The application
must associate its own item identity with the stored coordinate and account
for Path changes after Tree mutation.

## Reading results

Use the Group accessors to inspect items in comparator order and their assigned Paths:

```c
size_t count = lks_group_size(result);
size_t index;
for (index = 0; index < count; ++index) {
    void *item = lks_group_item_at(result, index);
    const LksPath *path = lks_group_path_at(result, index);
    /* Use the borrowed item and Path while result remains alive. */
}
```

`lks_group_item_at` returns the item pointer at a sorted index. `lks_group_path_at`
returns a borrowed Path owned by the Group. A published Group is immutable;
its Paths remain stable until that Group is destroyed.

## Ownership

- Item pointers are borrowed.
- LayerKeySort does not clone caller business items.
- LayerKeySort does not free caller business items.
- The caller controls item lifetime and must keep each item valid for as long as it may be compared or read.
- Caller-owned Paths, Groups, Trees, and Batches must be released with their matching public destroy functions. Accessors that return `const` Path or node pointers return borrowed views; do not destroy them separately.

## Stability

Comparator-equal input items retain their input order. For the public two-Group merge, equal items from the Base Group precede equal items from the Incoming Group, while order within each source is preserved. GroupBatch merging preserves source chunk order for equal items.

## Choose the Tree model

Use `LksTree` when the caller intentionally owns coordinates. Explicit insert,
exact find/removal, and rekey accept Paths selected by the caller. A parsed LK1
coordinate can be restored through explicit insert. The Tree does not check a
comparator invariant and exposes no comparator-based insert/locate operation.

Use `LksOrderedTree` for online comparator order. Supply one comparator and a
caller-owned context at creation. Insertions are stable for equal values and
may relabel a contiguous logical range to make coordinate space. The region
can grow geometrically to the entire collection. Existing AVL nodes remain;
only Paths and the new node are prepared before an allocation-free commit.
The descriptor is copied, so its stack object may expire after creation. The
callback code and caller-owned context remain borrowed; keep them alive and
keep their ordering semantics stable for the container's lifetime. Every
resident item must remain alive, and its comparator-relevant fields must not
change while resident. To change a sort key, first clone or otherwise save
its Path, remove the item by that exact Path, update the item, then insert it
again. In-place key changes can make comparator searches miss items or choose
the wrong insertion gap. There is no arbitrary managed rekey or per-call
comparator replacement.

```c
LksComparator order = { compare_items, context };
LksOrderedTree *ordered = lks_ordered_tree_create(&order);
if (ordered == NULL) { /* handle invalid comparator or OOM */ }
LksStatus status = lks_ordered_tree_insert(ordered, item, NULL);
/* Check status, use ordered, then release it. Caller still owns item. */
lks_ordered_tree_destroy(ordered);
```

Both containers borrow items and own Paths/nodes. Reacquire borrowed nodes,
Paths, and navigation observations after an actual successful mutation.
Physical navigation exposes implementation-defined AVL links, not Path
hierarchy. Use it only as a transient index view; compare Paths using
`lks_path_compare()` for logical order. A failed operation leaves existing
borrows valid. Manual equal-Path rekey is the documented successful no-op.

A Path is an ordering coordinate rather than a stable application identity.
Canonical display text is readable; versioned LK1 keys can persist and
bytewise-sort one coordinate. Neither saves caller payloads or a whole Tree.
Exact generated Path strings are not stable application identities. Private
relabel thresholds are not public compatibility promises: a compatible
future 3.x implementation may change both while preserving documented
public ordering, ownership, failure, and LK1 contracts. Complete managed
insertion can relabel all `n` nodes; no worst-case `O(log n)` or formal
amortized bound is claimed.

## Error handling

Functions that return `LksStatus` report their outcome with values such as `LKS_STATUS_OK`, `LKS_STATUS_INVALID_ARGUMENT`, and `LKS_STATUS_OUT_OF_MEMORY`. Use `lks_status_string(status)` for a static human-readable description. Constructors returning pointers use `NULL` on failure. Consult [`API.md`](API.md) for the complete public status and function reference. There is no public allocator fault-injection API.

## Limitations

- Shared mutable objects are not guaranteed to be thread-safe; use external synchronization when sharing them.
- Paths from separate Groups are local coordinates until a merge establishes the result’s path space.
- Whole-Tree serialization, a fixed memory ceiling, and a public allocator or fault-injection API are not provided.
