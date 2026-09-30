# Using LayerKeySort

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

## Sorting a pointer array

Define a comparator over your pointed-to item type, then call:

```c
LksStatus status = lks_sort(items, item_count, compare_items, NULL);
```

`lks_sort` changes the pointer array, not the pointed-to objects. It is stable
for comparator-equal items and leaves the input array unchanged if allocation
fails. [`examples/basic.c`](../examples/basic.c) is the complete first-use example.

To build and run it with CMake:

```sh
cmake -S . -B build
cmake --build build
./build/layerkeysort_example
```

The executable path can differ with multi-configuration generators such as
Visual Studio (for example, `build/Debug/layerkeysort_example.exe`).

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
formatter writes canonical current output. The unreleased
`lks_path_parse()` accepts exactly that text and returns a caller-owned Path.
For storage and bytewise ordering, use the separate versioned
`lks_path_order_key_*()` API. **Use `lks_path_compare()` to order Paths; do not
use `strcmp` on complete formatted text.** Text and generated coordinates may
change in later previews. Do not persist Path text as an item ID.

For example, the positive level-zero, slot-zero Path has display text `0222`
and durable key `LK1:201FF0000!`. Canonical keys of the same version sort
under `strcmp()` or a database collation preserving bytewise ASCII order.
The parsed Path belongs to the caller and must be destroyed with
`lks_path_destroy()`. Invalid or noncanonical inputs fail with NULL output;
allocator failure also leaves output NULL. The exact key grammar and
cross-platform overflow rule are in [API.md](API.md). These APIs are local
unreleased development after Preview.3.

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

## Path locality

Comparator-driven online Tree insertion first uses a direct Path when it fits.
When a Path becomes deep, it may re-encode a bounded contiguous logical-order
range. It can accept a deeper Path if local repair is unsuitable, or
rebuild the whole Tree as a final fallback. These are preview heuristics, not
stable Path identities or a promise of fixed memory use. Comparator-driven
Tree operations require existing items to be sorted compatibly with the
supplied comparator and context in Path order; explicit Path insertion does
not check item ordering.

Tree navigation exposes an implementation-defined physical index. A parent
need not be a Path prefix, children need not be logical descendants, and
physical preorder does not define logical order. The virtual root has one
physical child when nonempty. Compare Paths with `lks_path_compare()` and
reacquire borrowed navigation results after each actual mutation.

On the unreleased development branch, `lks_tree_remove_path()` removes one
exact Path without freeing its caller-owned item or compacting any other Path.
`lks_tree_rekey()` moves the same item pointer to a caller-selected unoccupied
Path. It prepares all allocations before the structural commit, so failure
leaves the old association intact. Equal old/new Paths succeed as a no-op.
That no-op preserves existing borrowed Tree nodes, Paths, and navigation views.
Physical AVL rebalancing may change parent/child links, but does not re-encode
Paths. To move an item, callers can change their own payload if appropriate,
choose a target with `lks_path_before()`, `lks_path_after()`, or
`lks_path_between()`, then rekey. Arbitrary external payload changes are not
detected: before later comparator-driven Tree operations, Path order must
remain compatible with that comparator. Published Groups remain immutable;
build a new Group if a different Group order is needed.

A Path describes order within the Group or Tree that created it, not a stable
application identity. Paths from independent Groups are local coordinates.
Merge creates a new coordinate space and may reassign every result Path, while
leaving both source Groups unchanged. The source Groups' published Paths stay
valid for their respective Group lifetimes. A successful mutable Tree operation
may locally relabel or fully rebuild; **reacquire all** borrowed Tree nodes,
Paths, and navigation results after mutation. A failed operation with the
documented strong guarantee does not commit a mutation. Do not persist or
serialize generated Paths as item identities; versioned keys may persist their
current coordinates for external ordering. Path-allocation heuristics and exact
generated strings may change before final v2.0.0.

## Error handling

Functions that return `LksStatus` report their outcome with values such as `LKS_STATUS_OK`, `LKS_STATUS_INVALID_ARGUMENT`, and `LKS_STATUS_OUT_OF_MEMORY`. Use `lks_status_string(status)` for a static human-readable description. Constructors returning pointers use `NULL` on failure. Consult [`API.md`](API.md) for the complete public status and function reference. There is no public allocator fault-injection API.

## Limitations

- Shared mutable objects are not guaranteed to be thread-safe; use external synchronization when sharing them.
- Paths from separate Groups are local coordinates until a merge establishes the result’s path space.
- Binary serialization, a fixed memory ceiling, and a public allocator or fault-injection API are not provided.
