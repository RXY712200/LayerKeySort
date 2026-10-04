# Using LayerKeySort

English | [简体中文](USAGE.zh-CN.md)

This is the LayerKeySort 3.1.0 API/use guide. The public function set remains
compatible with stable `v3.0.0`. See [Integration](INTEGRATION.md) for obtaining
the library and [V3 migration](V3_MIGRATION.md) when upgrading from V2.

## Before you start

Use the [integration guide](INTEGRATION.md) to obtain the two-file package,
add a CMake source dependency, install a local CMake package, or build modular
sources. Application code includes only `layerkeysort.h`. This guide starts
with using the API after the library is available.

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

## Dynamic layer-list example

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

## Reading and storing Path coordinates

`lks_path_format()` produces canonical readable text, and `lks_path_parse()`
reconstructs a caller-owned Path. Use `lks_path_compare()` for logical order:
complete display strings are not lexical sort keys. The separately versioned
`lks_path_order_key_*()` API produces canonical `LK1:` keys whose bytewise
ASCII lexical order matches Path order within the same version. For example,
a positive level-zero slot-zero Path has display text `0222` and key
`LK1:201FF0000!`. Use a database collation that preserves bytewise ASCII
ordering. The exact grammars, invalid-input rules, and overflow behavior are
in [API.md](API.md).

A stored key preserves a coordinate at one point in time, not permanent item
identity, caller payloads, or a whole Tree. The application must associate
its own identity with the coordinate and account for Path changes after
managed mutation. Destroy parsed Paths with `lks_path_destroy()`.

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
may relabel a contiguous logical range to make coordinate space. Relabel may
change every resident Path; see the
[architecture](ARCHITECTURE.md) for the implementation and
[benchmarks](BENCHMARKS.md) for measured costs.
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
relabel thresholds are not public compatibility promises; see the
[3.x contract](COMPATIBILITY.md). Complete managed
insertion can relabel all `n` nodes; no worst-case `O(log n)` or formal
amortized bound is claimed.

## Error handling

Functions that return `LksStatus` report their outcome with values such as `LKS_STATUS_OK`, `LKS_STATUS_INVALID_ARGUMENT`, and `LKS_STATUS_OUT_OF_MEMORY`. Use `lks_status_string(status)` for a static human-readable description. Constructors returning pointers use `NULL` on failure. Consult [`API.md`](API.md) for the complete public status and function reference. There is no public allocator fault-injection API.

## Limitations

- Shared mutable objects are not guaranteed to be thread-safe; use external synchronization when sharing them.
- Paths from separate Groups are local coordinates until a merge establishes the result’s path space.
- Whole-Tree serialization, a fixed memory ceiling, and a public allocator or fault-injection API are not provided.
