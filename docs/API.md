# API Reference

## Header

`include/layerkeysort.h` is the source of truth for the public API.

## Version macros

- `LKS_VERSION_MAJOR` is `2`.
- `LKS_VERSION_MINOR` is `0`.
- `LKS_VERSION_PATCH` is `0`.
- `LKS_VERSION_PRERELEASE` is `"preview.3"`.
- `LKS_VERSION_STRING` is `"2.0.0-preview.3"`.

Path values are ordering coordinates, not permanent application item IDs.
Preview.3 specifies the current formatter output, but provides no public
parser, stable persistence format, binary serialization protocol, or
cross-Preview compatibility for stored Path strings. Exact coordinates and
their text can change when a Tree is re-encoded or Preview policy changes.

## Simple stable sort

`lks_sort(items, count, compare, context)` sorts the caller's pointer array in
place. It borrows the pointed-to objects and preserves the array on allocation
failure. Comparator-equal pointers keep their input order. A zero or one item
array needs no allocation. See the public header for the full argument contract.

```c
LksStatus lks_sort(void **items, size_t count,
    LksCompareFn compare, void *context);
```
## Path slots and codec

`LKS_PATH_SLOT_MIN` is `0u` and `LKS_PATH_SLOT_MAX` is `65535u`: 65,536 legal
numeric values. Public slot arguments and results use `unsigned int`; the
implementation range-checks before storing each slot as `uint16_t`. A Path
stores numeric slots and `size_t` levels, not its formatted text. The
three-character text width does **not** imply three stored bytes per slot.

The current fixed-width slot codec uses radix 54 and this exact alphabet,
listed in increasing **codec rank**:

```text
23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz
2 < 3 < 4 < 5 < 6 < 7 < 8 < 9
  < A < B < C < D < E < F < G < H < J < K < M < N < P < Q < R < S < T < U < V < W < X < Y < Z
  < a < b < c < d < e < f < g < h < j < k < m < n < p < q < r < s < t < u < v < w < x < y < z
```

`alphabet[0]` is radix digit 0 (`'2'`), `alphabet[1]` is digit 1 (`'3'`),
`'9'` is digit 7, `'A'` is digit 8, and `'z'` is digit 53. The visible
character `'2'` does not mean numeric digit 2; slot 0 is `222`, never `0`.
Encoding takes three successive remainders modulo 54, from low digit to
high, retaining leading `'2'` digits. Capacity is 54^3 = 157,464, of which
only the first 65,536 values are legal. Verified examples:

| Numeric slot | Token | Numeric slot | Token |
| ---: | :--- | ---: | :--- |
| 0 | `222` | 1 | `223` |
| 53 | `22z` | 54 | `232` |
| 130 | `24R` | 259 | `26p` |
| 32767 | `DEp` | 32768 | `DEq` |
| 65535 | `RUc` | | |

Slot 259 is shown as a codec example near the historical v1 upper bound;
it is not a Preview.3 limit.

Digits rank before uppercase, and uppercase before lowercase: `A < a` and
`Z < a`. These are ASCII byte/codepoint and codec-rank rules, not
locale-sensitive language rules. The codec emits fixed ASCII constants; it is
byte-oriented and case-sensitive (`A != a`) with no locale comparison or
Unicode normalization. The permitted alphabet is in ASCII byte order, so
numeric slot order matches bytewise lexical order **for three-character slot
tokens only**; the exhaustive test checks all 65,536 tokens. It omits
`0/O/o` and `1/I/i/L/l` to reduce visual confusion. Punctuation is
deliberately absent from slot tokens, easing copying through shells, URLs,
configuration, and logs while leaving `/` for structure. This restriction is
a format choice, not a technical impossibility.

## Path ordering

`lks_path_compare()` defines Path order. It returns negative, zero, or positive
through `out_result` for left before, equal to, or after right. Directions
sort **negative < ZERO < positive**. Within either nonzero direction, compare
steps from the root. At the first differing step, compare the explicit
`size_t` **level first**: the larger numeric level sorts **earlier**. Only
when levels match, compare numeric slots. Slots normally ascend, except at
the **negative root** (step 0), where the larger slot sorts earlier. Negative
descendant slots ascend normally. If all common steps match, the shorter Path
(parent) sorts before its descendant. Skipped levels participate through
their explicit level values; they are not inferred from `/` counts.

For example, `1223 < 1222` (negative root slots 1 and 0);
`1222/223 < 1222/224` (negative descendant slots ascend);
`0222/5223 < 0222/223` (at the first different child step, level 5 sorts
before level 1); and `0222 < 0222/223` (parent before descendant).
ZERO lies after every negative and before every positive Path. These examples
follow the comparison code and the current formatter grammar below.

**Complete formatted Path text is not an ordering key.** Direction digits,
optional decimal level metadata, separators, and omitted default deltas mean
ordinary string sorting, including `strcmp`, is not the public ordering
contract. The slot-token lexical property above does not extend to full Path
strings. Use `lks_path_compare()`.

## Current Path text grammar

For a valid Path, the formatter emits the following deterministic,
NUL-terminated ASCII text (the grammar describes **output**, not input
accepted by any parser):

```text
ZERO     = "000"
NONZERO  = DIRECTION FIRST_SEGMENT ("/" NEXT_SEGMENT)*
DIRECTION = "0" for positive, "1" for negative
FIRST_SEGMENT = [absolute_level_decimal_if_nonzero] SLOT_TOKEN
NEXT_SEGMENT  = [positive_delta_decimal_if_greater_than_one] SLOT_TOKEN
SLOT_TOKEN    = exactly three characters from the ordered slot alphabet
```

ZERO has direction `LKS_DIRECTION_ZERO` (numeric value 0) and depth 0. It
is a Path value distinct from the Tree's virtual root. Every nonzero Path
has at least one step and at least four text characters, so `000` cannot
collide with it. For a nonzero Path, the first step's absolute level is
omitted when zero; later levels must strictly increase, and their delta from
the previous level is omitted when one. Otherwise the nonzero absolute level
or delta is written as minimal unsigned decimal without leading zeroes.
The final three characters of each segment are always its slot token.
Exactly one `/` separates adjacent steps; `/` is **only** a separator, not a
unary level count. No repeated separators or whitespace are emitted.

The slot-token exclusions do not apply to other text fields: `0` and `1`
occur as direction/ZERO characters and may occur in decimal level metadata;
decimal metadata uses `0..9`, whereas slot tokens use the exact alphabet
above. Uppercase and lowercase letters occur only inside slot tokens.

Verified formatter examples:

| Path construction | Text |
| --- | --- |
| ZERO, depth 0 | `000` |
| Positive, first slot 0 at level 0 | `0222` |
| Positive, first slot 65535 at level 0 | `0RUc` |
| Positive, first slot 32768 at level 0 | `0DEq` |
| Positive `0222`, then slot 1 at level 1 | `0222/223` |
| Positive `0222`, then slot 1 at level 5, slot 53 at level 6 | `0222/5223/22z` |
| Negative, first slot 54 at level 12 | `112232` |

The table's first, minimum, maximum, skipped-level, and nonzero-first-level
cases are asserted by the focused Preview.3 tests; consecutive-step output
also follows their depth/length test and the same formatter. Formatting
retains all three slot digits, omits default level metadata, and emits no
decimal leading zeroes. Thus each valid Path has one formatter-produced
current text. There is **no public parser**, so this does not define what a
future parser might accept. It is also not a long-term persistence or wire
format, and should not be used as a permanent item ID.

`lks_path_text_length(path)` returns the character count excluding `\0`
(and returns 0 for a null or internally invalid Path). Allocate at least
that count plus one byte for `lks_path_format(path, buffer, buffer_size)`.
The formatter writes a trailing `\0` on success; an undersized buffer
returns `LKS_STATUS_BUFFER_TOO_SMALL` without writing output. Consecutive
levels starting at zero use exactly `4 * depth` characters. In general,
length is one direction character plus three per step, one separator per
additional step, and the decimal digits of any emitted level metadata.

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

Insert an item at a unique explicit Path, which the Tree copies. Any valid
unique Path is accepted without requiring its proper prefixes to be present;
this removes a restriction of the former physical prefix representation.
Duplicate Paths return `LKS_STATUS_ALREADY_EXISTS`.

```c
LksStatus lks_tree_insert(
    LksTree *tree,
    const LksPath *path,
    void *item,
    const LksTreeNode **out_node
);
```

### `lks_tree_insert_item`

Insert an item at a position selected by the comparator. Existing items must
already be ordered compatibly with that comparator and context in Path order.

```c
LksStatus lks_tree_insert_item(
    LksTree *tree,
    void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_node
);
```

### `lks_tree_locate_item`

Locate neighboring and equal nodes for an item under a comparator. Existing
items must already be ordered compatibly with that comparator and context in
Path order. Explicit-Path insertion does not check this condition.

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

Read a node’s borrowed physical parent; the virtual root is NULL.

```c
const LksTreeNode *lks_tree_node_parent(const LksTreeNode *node);
```

### `lks_tree_node_child_count`

Return a node’s physical child count (currently at most two).

```c
size_t lks_tree_node_child_count(const LksTreeNode *node);
```

### `lks_tree_node_child_at`

Borrow a physical child by index. With two children, index 0 has the lower
Path and index 1 the higher Path. With one child, it is at index 0.

```c
const LksTreeNode *lks_tree_node_child_at(
    const LksTreeNode *node,
    size_t index
);
```

### `lks_tree_root_child_count`

Return zero for an empty Tree and one for a nonempty Tree.

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

### `lks_tree_remove_path` (unreleased development branch)

Remove exactly one Path-keyed Tree node without allocating or relabeling other
Paths. The Tree destroys its node and owned Path, but never frees the borrowed
item. Optional `out_item` receives that same item pointer on success and NULL
on failure. Missing Paths return `LKS_STATUS_NOT_FOUND`; invalid inputs return
`LKS_STATUS_INVALID_ARGUMENT`. Tree size decreases by one. ZERO is an ordinary
key, distinct from the virtual root.

```c
LksStatus lks_tree_remove_path(
    LksTree *tree, const LksPath *path, void **out_item
);
```

### `lks_tree_rekey` (unreleased development branch)

Move one caller-owned item from `old_path` to an unoccupied `new_path` without
changing its item pointer or Tree size. The Tree clones the new Path. Missing
old Paths return `LKS_STATUS_NOT_FOUND`; an occupied new Path returns
`LKS_STATUS_ALREADY_EXISTS`. Equal Paths succeed without mutation. Invalid
inputs return `LKS_STATUS_INVALID_ARGUMENT`. On failure, including allocation
failure, the Tree remains unchanged and optional `out_node` is NULL. On a
changed-Path success it borrows the resulting node.

The transaction validates both positions, allocates the replacement Path and
node, then links the new node and removes the old node with no further
allocation or recoverable failure. Only the selected item's Path changes;
AVL rotations may change physical links but never re-encode unrelated Paths.
All earlier borrowed Tree nodes, Paths, and navigation results expire on a
changed-Path success. Callers selecting arbitrary Paths must keep item order
compatible with any comparator used by later comparator-driven operations;
the Tree does not scan and validate all items during rekey.

```c
LksStatus lks_tree_rekey(
    LksTree *tree, const LksPath *old_path, const LksPath *new_path,
    const LksTreeNode **out_node
);
```

The physical Tree index shape is implementation-defined and is not a stable
API contract. Physical parents need not be Path prefixes, physical children
need not be logical Path descendants, and physical preorder is not logical
Path order. Shape may change after mutation. Use `lks_path_compare()` to
compare positions; reacquire all borrowed navigation results after success.

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
and re-encode a bounded logical-order range or, as a final fallback, the entire Tree.
A failed operation with a documented strong guarantee commits no Tree
mutation. Comparator equality concerns item ordering and stable source order;
it does not mean two items share an equal Path. Public Group merge places
comparator-equal Base items before Incoming items, while Batch merge preserves
source chunk order for equals. Each successful merge result has its own Path
coordinate space, independent of unchanged source Groups.

The released v2.0.0-preview.3 public header contains **9 types** and **46 functions**,
including `lks_sort`; the unreleased development header adds the two Tree
mutation functions above. Private allocator, profile, benchmark, and test entry
points are not part of this reference.
