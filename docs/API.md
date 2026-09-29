# API Reference

## Header

`include/layerkeysort.h` is the source of truth for the public API.

## Version macros

- `LKS_VERSION_MAJOR` is `2`.
- `LKS_VERSION_MINOR` is `0`.
- `LKS_VERSION_PATCH` is `0`.
- `LKS_VERSION_PRERELEASE` is `"preview.2"`.
- `LKS_VERSION_STRING` is `"2.0.0-preview.2"`.

Path values are ordering coordinates, not persistent application IDs. Exact
generated Path strings can change with the allocation policy; serializing them
for later reuse is not supported in this preview.

## Simple stable sort

`lks_sort(items, count, compare, context)` sorts the caller's pointer array in
place. It borrows the pointed-to objects and preserves the array on allocation
failure. Comparator-equal pointers keep their input order. A zero or one item
array needs no allocation. See the public header for the full argument contract.

```c
LksStatus lks_sort(void **items, size_t count,
    LksCompareFn compare, void *context);
```
## Path slot bounds

- `LKS_PATH_SLOT_MIN` is `0u`.
- `LKS_PATH_SLOT_MAX` is `259u`, covering slots A0 through Z9.

## Types

- **`LksStatus`** — Operation result enumeration returned by status-bearing functions.
- **`LksCompareFn`** — Callback type used to compare two borrowed items with caller context.
- **`LksComparator`** — Pair of comparison callback and opaque caller context.
- **`LksDirection`** — Direction of a Path relative to the zero Path.
- **`LksPath`** — Opaque Path value; use public Path functions to create, inspect, and destroy it.
- **`LksTree`** — Opaque Tree that owns its structural nodes and Path copies.
- **`LksTreeNode`** — Opaque, read-only Tree node view returned by Tree navigation functions.
- **`LksGroup`** — Opaque ordered Group that owns structural storage and borrows item pointers.
- **`LksGroupBatch`** — Opaque collection of Groups built from consecutive input chunks.

## Status values

| Value | Meaning |
|---|---|
| `LKS_STATUS_OK` | Operation succeeded. |
| `LKS_STATUS_INVALID_ARGUMENT` | An argument is invalid. |
| `LKS_STATUS_OUT_OF_MEMORY` | Allocation failed. |
| `LKS_STATUS_BUFFER_TOO_SMALL` | Formatting storage is insufficient. |
| `LKS_STATUS_LEVEL_LIMIT` | A Path level limit was reached. |
| `LKS_STATUS_NOT_FOUND` | Requested entry was not found. |
| `LKS_STATUS_NOT_IMPLEMENTED` | Operation is not implemented. |
| `LKS_STATUS_INTERNAL_ERROR` | An internal error occurred. |
| `LKS_STATUS_ALREADY_EXISTS` | The explicit Path already exists. |

## Direction values

- `LKS_DIRECTION_ZERO` (`0`): the zero Path.
- `LKS_DIRECTION_POSITIVE` (`1`): a positive Path.
- `LKS_DIRECTION_NEGATIVE` (`-1`): a negative Path.

## Path API

### `lks_path_create_zero`

Allocate a zero Path.

```c
LksPath *lks_path_create_zero(void);
```

### `lks_path_create`

Allocate a one-step Path at level zero.

```c
LksPath *lks_path_create(
    LksDirection direction,
    unsigned int first_slot
);
```

### `lks_path_create_at_level`

Allocate a one-step Path at an explicit level.

```c
LksPath *lks_path_create_at_level(
    LksDirection direction,
    unsigned int first_slot,
    size_t level
);
```

### `lks_path_clone`

Create an independent deep copy of a Path.

```c
LksPath *lks_path_clone(
    const LksPath *source
);
```

### `lks_path_append`

Append a slot at the next level.

```c
LksStatus lks_path_append(
    LksPath *path,
    unsigned int slot
);
```

### `lks_path_append_at_level`

Append a slot at a caller-specified greater level.

```c
LksStatus lks_path_append_at_level(
    LksPath *path,
    unsigned int slot,
    size_t level
);
```

### `lks_path_direction`

Read a Path direction; NULL is treated as zero.

```c
LksDirection lks_path_direction(
    const LksPath *path
);
```

### `lks_path_depth`

Read a Path step count; NULL is treated as empty.

```c
size_t lks_path_depth(
    const LksPath *path
);
```

### `lks_path_get_slot`

Read one slot into caller storage.

```c
LksStatus lks_path_get_slot(
    const LksPath *path,
    size_t index,
    unsigned int *out_slot
);
```

### `lks_path_get_level`

Read one level into caller storage.

```c
LksStatus lks_path_get_level(
    const LksPath *path,
    size_t index,
    size_t *out_level
);
```

### `lks_path_destroy`

Destroy a caller-owned Path; NULL is accepted.

```c
void lks_path_destroy(
    LksPath *path
);
```

### `lks_path_text_length`

Return formatted text length excluding the terminator.

```c
size_t lks_path_text_length(
    const LksPath *path
);
```

### `lks_path_format`

Format a Path into caller-owned character storage.

```c
LksStatus lks_path_format(
    const LksPath *path,
    char *buffer,
    size_t buffer_size
);
```

### `lks_path_compare`

Compare two Paths and write the result on success.

```c
LksStatus lks_path_compare(
    const LksPath *left,
    const LksPath *right,
    int *out_result
);
```

### `lks_path_before`

Generate a valid Path ordered before another Path.

```c
LksStatus lks_path_before(
    const LksPath *right,
    LksPath **out_path
);
```

### `lks_path_after`

Generate a valid Path ordered after another Path.

```c
LksStatus lks_path_after(
    const LksPath *left,
    LksPath **out_path
);
```

### `lks_path_between`

Allocate a Path strictly between two ordered Paths.

```c
LksStatus lks_path_between(
    const LksPath *left,
    const LksPath *right,
    LksPath **out_path
);
```

## Tree API

### `lks_tree_create`

Allocate an empty Tree.

```c
LksTree *lks_tree_create(void);
```

### `lks_tree_destroy`

Destroy a caller-owned Tree and its nodes and Paths.

```c
void lks_tree_destroy(LksTree *tree);
```

### `lks_tree_size`

Return the number of Tree nodes; NULL is treated as empty.

```c
size_t lks_tree_size(const LksTree *tree);
```

### `lks_tree_insert`

Insert an item at a unique explicit Path, which the Tree copies.

```c
LksStatus lks_tree_insert(
    LksTree *tree,
    const LksPath *path,
    void *item,
    const LksTreeNode **out_node
);
```

### `lks_tree_insert_item`

Insert an item at a position selected by the comparator.

```c
LksStatus lks_tree_insert_item(
    LksTree *tree,
    void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_node
);
```

### `lks_tree_locate_item`

Locate neighboring and equal nodes for an item under a comparator.

```c
LksStatus lks_tree_locate_item(
    const LksTree *tree,
    const void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_left,
    const LksTreeNode **out_equal,
    const LksTreeNode **out_right
);
```

### `lks_tree_find_path`

Find a Tree node at a Path.

```c
LksStatus lks_tree_find_path(
    const LksTree *tree,
    const LksPath *path,
    const LksTreeNode **out_node
);
```

### `lks_tree_node_path`

Borrow a node Path until Tree mutation or destruction.

```c
const LksPath *lks_tree_node_path(const LksTreeNode *node);
```

### `lks_tree_node_item`

Read a node’s borrowed item pointer.

```c
void *lks_tree_node_item(const LksTreeNode *node);
```

### `lks_tree_node_parent`

Read a node’s borrowed parent; the virtual root is NULL.

```c
const LksTreeNode *lks_tree_node_parent(const LksTreeNode *node);
```

### `lks_tree_node_child_count`

Return a node’s child count.

```c
size_t lks_tree_node_child_count(const LksTreeNode *node);
```

### `lks_tree_node_child_at`

Borrow a child node by index.

```c
const LksTreeNode *lks_tree_node_child_at(
    const LksTreeNode *node,
    size_t index
);
```

### `lks_tree_root_child_count`

Return the number of virtual-root children.

```c
size_t lks_tree_root_child_count(const LksTree *tree);
```

### `lks_tree_root_child_at`

Borrow a virtual-root child node by index.

```c
const LksTreeNode *lks_tree_root_child_at(
    const LksTree *tree,
    size_t index
);
```

## Group API

### `lks_group_build`

Stably sort borrowed item pointers and bulk-assign Paths to a new immutable Group.

```c
LksStatus lks_group_build(
    void *const *items,
    size_t count,
    const LksComparator *comparator,
    LksGroup **out_group
);
```

### `lks_group_merge`

Merge two sorted Groups into a new immutable Group, with equal Base items first.
Both inputs stay unchanged. The result receives a fresh Path layout, including
possibly different Paths for items originating in Base.

```c
LksStatus lks_group_merge(
    const LksGroup *base,
    const LksGroup *incoming,
    const LksComparator *comparator,
    LksGroup **out_group
);
```

### `lks_group_destroy`

Destroy a Group’s structure without freeing its items.

```c
void lks_group_destroy(LksGroup *group);
```

### `lks_group_size`

Return the item count; NULL is treated as empty.

```c
size_t lks_group_size(const LksGroup *group);
```

### `lks_group_item_at`

Borrow an item pointer at sorted index.

```c
void *lks_group_item_at(const LksGroup *group, size_t index);
```

### `lks_group_path_at`

Borrow a Group-owned Path until Group destruction.

```c
const LksPath *lks_group_path_at(const LksGroup *group, size_t index);
```

## GroupBatch and merge API

### `lks_group_batch_build`

Split inputs into consecutive chunks and sort each chunk.

```c
LksStatus lks_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const LksComparator *comparator,
    LksGroupBatch **out_batch
);
```

### `lks_group_batch_destroy`

Destroy a Batch and its Groups, but not the input items.

```c
void lks_group_batch_destroy(LksGroupBatch *batch);
```

### `lks_group_batch_total_size`

Return the total item count; NULL is treated as empty.

```c
size_t lks_group_batch_total_size(const LksGroupBatch *batch);
```

### `lks_group_batch_group_count`

Return the number of constituent Groups; NULL is treated as empty.

```c
size_t lks_group_batch_group_count(const LksGroupBatch *batch);
```

### `lks_group_batch_group_size`

Return the configured chunk target; NULL is treated as zero.

```c
size_t lks_group_batch_group_size(const LksGroupBatch *batch);
```

### `lks_group_batch_group_at`

Borrow a constituent Group until Batch destruction.

```c
const LksGroup *lks_group_batch_group_at(
    const LksGroupBatch *batch,
    size_t index
);
```

### `lks_group_batch_merge_all`

Stably merge the Batch's ordered Groups in source order and assign Paths once
to a new independent Group. Source Groups stay unchanged.

```c
LksStatus lks_group_batch_merge_all(
    const LksGroupBatch *batch,
    const LksComparator *comparator,
    LksGroup **out_group
);
```

## Status API

### `lks_status_string`

Return a static text description for a public status value.

```c
const char *lks_status_string(LksStatus status);
```

## Comparator semantics

`LksCompareFn` returns a negative value when the left item sorts first, zero when the values compare equal, and a positive value when the left item sorts after the right. The `context` pointer is passed through unchanged and is not interpreted by LayerKeySort.

Linear Group and Batch merges require every input Group to have been built
under ordering semantics compatible with the merge comparator and context.
Comparator identity is not checked at runtime. A successful Tree mutation may
replace all internal nodes: every borrowed Tree node, Path, and navigation
result must be reacquired afterward. Explicit-Path `lks_tree_insert` preserves
the supplied coordinate; comparator-driven `lks_tree_insert_item` may rebuild
and re-encode a bounded subtree or, as a final fallback, the entire Tree.

The v2.0.0-preview.2 public header contains **9 types** and **46 functions**,
including `lks_sort`. Private allocator, profile, benchmark, and test entry
points are not part of this reference.
