# Full 4.2.0-rc.1 internal structures and execution paths

[简体中文](IMPLEMENTATION_INTERNALS.zh-CN.md) · [Source audit](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md)

Validation tree `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08` contains both Preview.3 regressions. Operational behavior comes from `fe00536e7bd7f4e5128432ea4a29e54158074903`. Published Stable is 4.1.0 at `70d7f0ef5857b54381b863d292a213909d87459f`. The restored mixed-tag test temporarily introduces zero tags, checks both routes, and restores the original tags. [Stage verification](V4_2_0_PREVIEW3.md)

## 1. Production source inventory

| File | Duties |
| --- | --- |
| `lks_alloc.c` | Allocation wrappers; diagnostics add aligned headers, tag/accounting and deterministic failure injection. |
| `lks_base.c` | Frozen static status strings without allocation. |
| `order.c` | `v4_refresh/replace/rotate_left/rotate_right/balance` maintain AVL; `v4_link_after/detach` maintain list and tree; `v4_place/insert/repair/move/locations/shifted_locations` implement live order. |
| `managed_order.c` | `v4_managed_compare_item` guards the callback; `v4_managed_bound` searches AVL then the block, reusing Explicit order. |
| `snapshot.c` | Atomic marker lifetime, `lks_snapshot_new`, `snap_append`, `lks_snapshot_capture_sequence` and `snap_live_next`. |
| `snapshot_key.c` | `snap_nibble`, `snap_key_parse`, LS1 format, validation and same-domain comparison. |
| `snapshot_wire.c` | `snap_be_read/write` and canonical binary serialization/deserialization. |
| `snapshot_restore.c` | Ordered Resolver restoration and strict V3 import with `snap_legacy_compare`. |
| `order_bulk.c` | Uniform block fill, fresh Handles, `snap_bulk_index` from the threaded list. |
| `immutable_group.c` | `ig_new/ig_next`, copied Groups, Batch merge and snapshot management. |
| `sort.c` | Stable merges with `lks_stable_sort_copy`, `stable_sort_range`, `lks_merge_sorted_pointers`. |
| `path.c` | V3 Path representation, aligned slot and level storage, growth, copy and release. |
| `path_text.c` | Historical display text length, formatting and parsing. It is in the production source list, while LK1 import uses a separate parser. |
| `path_order_key.c` | Canonical byte-order LK1 format and parser; import uses the parser. |
| `slot_codec.c` | Three-character radix-54 slot display; LK1 uses distinct hexadecimal fields. |
| `path_compare.c` | Legacy direction, level, slot and prefix-depth ordering for migration sort and duplicate checks. |

`gap.c`, `tree.c`, `group.c`, and `bulk.c` enter historical targets through `LKS_LEGACY_REGRESSION_SOURCES`. Internal headers are `lks_order_internal.h`, `lks_snapshot_internal.h`, `lks_immutable_group_internal.h`, `lks_sort_internal.h`, `lks_alloc_internal.h`, `lks_legacy_internal.h`, `lks_path_internal.h` and `lks_slot_codec_internal.h`. Consumers use `layerkeysort.h`.

## 2. Three layers of live order

A heap-allocated `LksOrderHandle` has `block`, `local`, and borrowed `item`. Separate occurrences can refer to the same item, and a live Handle retains its address. A Block has `owner`, AVL child/parent pointers, `previous/next`, `subtree_blocks`, `height`, `count`, a private uint64_t tag (zero selects fallback), and `records[CAPACITY+1]`. The spare array slot is temporary; committed capacity is 64. With multiple blocks, each has at least 32 entries. A sole block may be smaller. Order owns root, first/last, count/blocks, revision, callback_active and source_marker.

~~~text
Item A (caller owned) ← HA { block=Block1, local=0, item=&A }
Item B (caller owned) ← HB { block=Block1, local=1, item=&B }
Item C (caller owned) ← HC { block=Block2, local=0, item=&C }

Block1.records = [HA*, HB*]
Block2.records = [HC*, ...]
threaded: Block1 <--> Block2

AVL: indexes Block1/Block2 by implicit in-order sequence
Order: root + first/last + logical count + revision
~~~

AVL in-order sequence matches the block links and each block's Handle order. `subtree_blocks` counts blocks, remains validated by diagnostics, and is unused by precedence comparison. The public API has no nth-item operation.

## 3. AVL and bounded tag maintenance

Commit 22544cf2 removed the Stable `v4_rank` route; 61e9290e added the tag fast path. `v4_tag_attach` starts at the new block with width 1. Present exterior neighbors must carry nonzero tags. Absent neighbors supply lower 0 or upper UINT64_MAX. Accept only an increasing bound and integer stride `(upper - lower) / (width + 1)` of at least 1 for width 1, or at least width otherwise. Cap a single block at an open right end at 2^40. On success write successive lower-plus-stride tags by following block links. This leaves an upper gap and allocates nothing.

After failure, stop at width 64 or when neither exterior neighbor exists, leaving the new block's zero tag. Otherwise expand left if a left neighbor exists and the right is absent, lower is zero, or width is even. In all other cases expand right. Grow by one, with no writes before a valid window is found. Successful repair can recover old zero tags. Recovery has no guaranteed deadline.

Diagnostics count successful stores in `tag_writes`, successful repairs wider than one block in `tag_repairs`, exits leaving a zero tag in `tag_fallbacks`, and peak written width in `max_tag_writes`. These are separate from the structure counters reset by `v4_begin`. In-order `v4_check_node` checks the threaded links and insists each nonzero tag exceeds `last_nonzero_tag`; zeros leave the previous tag in effect.

Within one block compare local indices, allowing zero. With both tags nonzero use their order. Otherwise ascend the lower-height side, selecting the right input on ties. If the left input's parent is the right input, return −1 only if the left input is its left child. If the right input's parent is the left input, return −1 only if the right input is its right child. Once the branches share a parent, return −1 only when the left branch is that parent's left child. The other outcomes return +1.

| Helper | Work |
| --- | --- |
| `v4_height`, `v4_subtree` | Zero for NULL, otherwise stored height or block count. |
| `v4_refresh` | Recompute height and subtree block count; instrument the work in diagnostics. |
| `v4_replace` | Update the old node's parent link or root; install replacement's parent. |
| `v4_rotate_left/right` | Pivot across the right/left child, preserve in-order sequence and refresh both roots. |
| `v4_balance` | Walk parents, refresh and choose single or double rotations. |
| `v4_minimum` | Follow left links to minimum block. |
| `v4_new_block` | Allocate, zero, set owner and initialize height and subtree count to one. |
| `v4_link_after` | Link threaded successor and implicit AVL successor, or initialize root/first/last. Increment blocks, call v4_tag_attach, then v4_balance on the parent. |
| `v4_tag_attach` | Probe and assign a bounded tag window or leave zero, without allocation. |
| `v4_detach` | Remove from list and AVL. With two children physically transplant the successor block, preserving the association between blocks and residents. Rebalance affected paths. |
| `v4_locations` | Update all residents' block/local and clear unused array slots. |
| `v4_shifted_locations` | Update only the shifted suffix on ordinary edits. |
| `v4_mark_index/v4_begin` | Diagnostic block pointer deduplication in fixed storage, overflow flag if full, and reset per-operation counters. No dynamic allocation or production action. |

Rotations preserve logical order; the AVL indexes sequence positions independently of users’ comparator keys.

## 4. Insertion transaction

Front/back/before/after call `v4_insert`; relative entry first verifies the live same-Order anchor in `v4_insert_relative`. Check order, item, output, anchor, reentry and count/revision limits. Allocate a Handle, then a spare Block if no target block exists or it is full. From `v4_place` onward, shift pointers, install the new pointer, split up to 65 pointers between old and spare if needed, and repair locations. An ordinary shift sets the new Handle's block and updates the suffix, while a split uses `v4_locations`. Link and balance as needed. Commit count and revision, then output the Handle. Preparation failure preserves the old structure, Handles and revision. The item stays borrowed and duplicate insertion produces separate occurrences.

## 5. Removal and underfull repair

Use the live Handle's block/local to remove the exact pointer, shift left and decrease both counts. A sole empty block is detached and freed. With several blocks, an underfull block and a neighbor are combined through a stack pointer buffer: merge left and detach right if total fits in 64, otherwise redistribute evenly. Without underfill, clear the vacated tail and update shifted locals. Increment revision, free only the removed Handle, and optionally return its item pointer. Removal allocates nothing. A removed Handle cannot be passed back for a liveness test.

## 6. Movement

Self, adjacent and already-at-end moves return OK without allocation, revision or cursor invalidation. Same-block movement adjusts the target index when the source precedes it, shifts the affected interval and updates local indices without allocating. Across blocks, prepare a spare if the destination is full, unlink the source occurrence, insert the same Handle with any required destination split, then repair an underfull source. No second Handle is allocated. Actual movement advances revision. Structural cost depends on local blocks and AVL paths, not on rewriting intermediate coordinates over the move distance.

## 7. Managed order

`LksManagedOrder` contains the Explicit core, comparator and active flag. `v4_managed_compare_item` sets `core->callback_active`, and the façade guards reentry. `v4_managed_bound` uses each block's last item to find the leftmost candidate block via AVL, then binary searches that block. Lower is first >= query; upper is first > query. Insert at upper for stable equality. Locate searches both; predecessor is before lower (last if lower absent), equal is lower only on equality, successor is upper.

All occurrences of a changed comparator-visible item in affected ManagedOrders must be removed first. Other ordered structures, including immutable Groups that need rebuilding or replacement, must be handled too. Full does not track cross-container shared items. Remove/edit/reinsert is a sequence of operations. Successful removal expires the old Handle and later insertion may fail; application recovery must preserve objects and state and account for possible recovery errors. The façade exposes neither arbitrary moves nor a mutable core. For `[2a,2b,3,7a,7b,9]`, locate 7 returns 3, 7a and 9; insertion places 7c after 7b.

## 8. Cursor revision and snapshot provenance

A cursor records owner, next Handle, revision and direction. Next checks revision before touching a cached Handle and returns INVALIDATED on mutation. The cursor is transient and must be destroyed before its source. Snapshot currentness requires both matching SourceMarker identity and revision. A deserialized snapshot has no live provenance. Atomic marker lifetime does not synchronize arbitrary Order or Group mutation.

## 9. Snapshot allocation and capture

`LksSnapshotRow { size_t end; }` stores cumulative association ends, including repeated ends for empty rows. `lks_snapshot_new` validates nonempty namespace, sizes and overflow and coallocates header, rows and raw namespace; association capacity grows separately.

`lks_snapshot_capture_sequence` validates busy state, options and output, then copies the options descriptor. Source-specific next yields one item per occurrence, setting and clearing the source busy guard around each Association callback. `snap_append` geometrically grows storage, copies bytes and records the end with overflow checks. Final shrink can fail and leave no published snapshot. Only after all rows and shrink succeed does `snap_marker_new` or `snap_marker_retain` record provenance and revision for publication. Live traversal is local index plus threaded next, and Group traversal is `ig_next` by array index.

`lks_snapshot_row_offset/size` derive start and length from the previous and current ends. `snap_marker_new` starts with two references, source and first snapshot. Atomic retain/release free at zero and capacity overflow is rejected. A snapshot can outlive its source. A nonempty callback span must have a non-NULL pointer and remain valid through the immediate copy after return; automatic callback-local arrays have already expired. Application callback side effects remain outside Full's failure atomicity.

## 10. LS1 and LKS4SNP1

### 10.1 LS1 historical row ordinal

`snap_nibble` recognizes digits and lowercase a..f. `snap_key_parse` checks prefix `LS1.`, nonempty even-length namespace hex, dot separator, exactly 16 ordinal hex digits and no trailing characters. Key length is `21+2*namespace_size` without NUL. Formatting writes namespace hex and fixed-width ordinal. Comparison validates each operand first, returns DOMAIN_MISMATCH for different namespace bytes, and compares ordinals in the same domain. ASCII bytewise collation preserves same-domain order. Locale or case-folding collation is outside that guarantee, and lexically ordering different namespaces does not give business precedence. LS1 represents a historical ordinal, distinct from durable record identity and V3 LK1 coordinate order.

### 10.2 Wire encoding

`snap_be_read/write` handle four- and eight-byte fields with unsigned shifts and byte access, independent of host endianness and alignment. The layout has no trailing bytes.

~~~text
magic = ASCII LKS4SNP1 (8)
flags = u32 BE 0
namespace_size = u32 BE, >= 1
row_count = u64 BE
namespace bytes
for each row: association_length u64 BE + raw bytes
end-of-input, with no trailing bytes
~~~

Serialization checks capacity before encoding into caller memory. Deserialization scans every row and checks bounds before allocations and copies in a second pass. Failure publishes no partial snapshot; loaded snapshots have no live SourceMarker. Wire encoding does not perform disk I/O, checksum, WAL or commit and power-loss recovery.

## 11. Resolve-first restore and bulk build

For nonempty snapshots, allocate N temporary item pointers and call Resolver in order. Stop at the first error or OK with NULL, and do not call subsequent rows. Only after all rows resolve does construction begin in an unpublished Order. Empty restore permits a NULL Resolver, calls no Resolver and still allocates a new Order, so it can fail with OOM. On failure release Full temporaries and output NULL. Caller-owned items and callback effects are not undone. Release temporary pointers after `lks_order_bulk_build`.

Bulk build derives block count and evenly distributes residents. It allocates each block and a fresh Handle for every occurrence, building the linked list left to right. `snap_bulk_index` recursively builds the left half, consumes the midpoint from the list, then builds the right half, filling parent links, heights and subtree counts. Zero-based block i gets `(i + 1) * (UINT64_MAX / (blocks + 1))` in uint64_t. Publication follows complete construction; partial state is cleaned up. Neither old topology nor Handle addresses are restored. The N-pointer scratch is an independent peak cost. Mixed-tag regression checks order and fallback after restoration using a deliberately constructed valid mixed state.

## 12. Sort, Group and Batch

`lks_stable_sort_copy` allocates sorted and scratch arrays, copies input, and invokes recursive `stable_sort_range`. It returns only the sorted allocation. `lks_sort` writes back only after success. Stable two-way `lks_merge_sorted_pointers` chooses left on comparison <=0. The recursion divides ranges, merges into scratch and copies back.

`LksGroup { items,count,marker,busy }` owns the sorted pointer array and borrows items. `ig_new` allocates the header. Build uses stable_sort_copy; merge allocates a new group and target array and performs one merge, preserving old groups. Self-merge treats inputs as two borrowed sequences.

`LksGroupBatch { groups,count,group_count,group_size,busy }` owns chunks. Build calls group_build for each and releases completed chunks on failure. `batch_group_at` returns a borrowed Group that cannot be separately destroyed. Merge-all copies chunks to a new array and allocates additional scratch only with group_count >1. It starts at group_size, doubles widths, alternates buffers and takes earlier-chunk equals first. The resulting Group is independent. Group immutability covers structure and pointer order; changing comparator-visible caller data can make the sequence unsuitable for ordered operations.

## 13. Private legacy V3 codecs

- `path.c` stores direction, depth, capacity and compact steps. `path_storage_layout` guards offsets and alignment for uint16_t slots and size_t levels. Reserve typically doubles capacity and may memmove levels to the new aligned offset. Slots/levels are views; create, clone, append and destroy manage lifetimes.
- `slot_codec.c` uses 54 specific characters and three positions to encode a uint16 slot. Decode checks each digit and range <=65535. This belongs to display syntax.
- `path_text.c` measures and formats direction, increasing levels expressed as differences or prefixes, slashes and three-character slots. Parsing is strict and separate from LK1.
- `path_order_key.c` measures encoded levels with `byte_count`, checks size overflow, writes uppercase hexadecimal bytes, and formats canonical `LK1:` direction, level and slot fields ending in `!`. Parsing rejects noncanonical or unrepresentable values and rebuilds a Path.
- `path_compare.c` orders NEGATIVE < ZERO < POSITIVE, checks consistency, and compares level, slot and prefix depth. Negative first-level slot order is reversed.
- `lks_order_import_v3_lk1` parses temporary Paths, sorts with legacy comparison, rejects duplicates as ALREADY_EXISTS, extracts borrowed items, calls bulk build and destroys temporary Paths. New V4 Order retains no old Paths or coordinates.
- `gap.c/tree.c/group.c/bulk.c` are separate historical target sources. They do not implement the current live core.

## 14. Allocation diagnostics and failure boundaries

Production uses standard malloc/realloc/free. Diagnostics add an aligned size/tag header, track requested live and peak bytes, blocks, failures and category totals, and can fail the kth allocation. Global diagnostic accounting gives no additional thread guarantee.

Insertion prepares the Handle and spare before commit; a cross-block move with a full destination also prepares a spare before changes. In snapshot capture, callback, growth, final shrink and marker retention happen before publication. Sort copies and sorts before writing back. Failure atomicity applies to the corresponding Full state. Caller items remain external. Requested bytes differ from process RSS and can return to zero while the allocator retains pages or fragmentation. B64 and the v4.1.0 smaller Resident do not remove per-occurrence allocation.

## 15. Diagnostic triage

| Symptom | Inspect first |
| --- | --- |
| Wrong order, correct count | records[], before/after offset correction and previous/next |
| Wrong cross-block neighbors | List endpoints, next/previous and counts |
| Compare sign wrong | Handle owner and local, strict nonzero-tag order, zero fallback, AVL parent/height and child direction. Subtree counts are separate structural diagnostics. |
| Corruption after removal | v4_repair merge/redistribute and v4_detach physical successor transplant |
| OOM changed old order | v4_insert/v4_move prepare boundary |
| Managed duplicate locate | Strictness of lower/upper and consistent comparator semantics |
| Cursor touched removed cache | Revision check before cached Handle, source lifetime |
| Snapshot unexpectedly current | Same SourceMarker and revision |
| LS1 or wire differs from golden | Big-endian lengths, namespace encoding, cumulative ends, slices and trailing-byte validation |
| Wrong legacy LK1 import | Direction/level/slot rules, canonical parse and duplicate coordinates |
| High peak memory | Per-resident allocation, block slack, association growth and retained snapshots |

These are source-derived investigation points. See the [validation map](IMPLEMENTATION_VALIDATION.zh-CN.md) for tests, commands and limits. Private mechanics are revision-specific. External consumers depend only on `include/layerkeysort.h`.
