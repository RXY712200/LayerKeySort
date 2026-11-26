# LayerKeySort Full: understanding the complete implementation (4.2.0-rc.1)

[简体中文](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [Validation/source baseline](V4_2_0_PREVIEW3.md)

Fixed-width wire reads/writes are tested against old golden bytes with offset-one buffers. Restored block labels and AVL fallback have targeted regressions.

This edition describes the Full 4.2.0-rc.1 verification tree `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`, including two Preview.3 regressions. Operational behavior comes from `fe00536e7bd7f4e5128432ea4a29e54158074903`. Published Stable remains 4.1.0. See the [source audit](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md) for lineage.

## Reading map: four connected guides

These guides progress from overview to detail without requiring knowledge of historical versions:

1. **This walkthrough:** complete architecture, typical operation paths and module relationships.
2. [Public API source mapping](IMPLEMENTATION_API_REFERENCE.md): all 64 frozen functions, ownership, invalidation and implementation files.
3. [Private algorithms and branches](IMPLEMENTATION_INTERNALS.md): blocks, AVL, moves/repair, comparator lookup, Snapshot/wire, Batch and legacy LK1, with helper calls and invariants.
4. [Build, tests, golden data, distribution and audit map](IMPLEMENTATION_VALIDATION.md): coverage, unverified assumptions, reproduction and evidence boundaries.

Scope includes all 16 production C files, public API, related private structures and key test entry points. It is not a line-by-line proof of every test, benchmark, script, historical branch or application. Source is authoritative; explanations must follow source changes.

## 0. What the library does

Full supports three separate data models rather than acting as a conventional one-shot `qsort` replacement or database:

1. **Mutable live order:** `LksOrder` maintains relative order among long-lived occurrences; `LksManagedOrder` positions them through a comparator in the same structure.
2. **Immutable historical order:** `LksSnapshot` captures logical order and application association bytes; LS1 and LKS4SNP1 provide historical coordinates and serialization.
3. **Immutable batch order:** `LksGroup`, `LksGroupBatch` and `lks_sort` provide stable sorting, chunk sorting and merging independently of live Order storage.

Across all modules:

- Applications own items and business IDs. Order/Group borrow item pointers and never destroy their contents.
- A live handle identifies one surviving insertion in its Order. Repeated insertion of the same pointer creates multiple handles.
- Live handles, permanent application IDs and historical LS1 keys are distinct. There is no automatic persistent handle.
- Live production mutation does not continually generate V3 Path/LK1 coordinates. Legacy code supports import.
- Full has no file-writing, crash-atomic commit, database transaction, SQL or collaborative-ordering module.

Read §1 structures, §2 insertion foundations, §3 mutation, §4 Managed, §5 Snapshot, §6 restore, §7 batch and §8 compatibility before exploring historical branches.

## 1. Production and historical code at the build entry point

The authoritative [production manifest](../cmake/ProductionSources.cmake) feeds the root CMake **STATIC** `layerkeysort` library, exposed as `LayerKeySort::layerkeysort`, using C17.

| Production module | Source | Responsibility |
| --- | --- | --- |
| Allocation/status | [lks_alloc.c](../src/lks_alloc.c), [lks_base.c](../src/lks_base.c) | Allocation abstraction, diagnostic injection and status descriptions |
| Live core | [order.c](../src/order.c) | Blocks, AVL, stable handles, insert/remove/move/compare/Cursor |
| Comparator facade | [managed_order.c](../src/managed_order.c) | Lower/upper bound and stable insertion on the same core |
| Snapshot capture/views | [snapshot.c](../src/snapshot.c) | Namespace/association copies, provenance marker and revision |
| Historical keys | [snapshot_key.c](../src/snapshot_key.c) | LS1 formatting, validation and same-domain comparison |
| Binary encoding | [snapshot_wire.c](../src/snapshot_wire.c) | LKS4SNP1 validation, encoding and parsing |
| Restore/import | [snapshot_restore.c](../src/snapshot_restore.c), [order_bulk.c](../src/order_bulk.c) | Resolver to fresh Order, balanced blocks/AVL, legacy LK1 import |
| Batch processing | [immutable_group.c](../src/immutable_group.c), [sort.c](../src/sort.c) | Group/Batch, stable merge and pointer sorting |
| Legacy read compatibility | [path.c](../src/path.c), [path_text.c](../src/path_text.c), [path_order_key.c](../src/path_order_key.c), [slot_codec.c](../src/slot_codec.c), [path_compare.c](../src/path_compare.c) | Private V3 Path/LK1 representation, grammar and ordering |

The root build separately defines `LKS_LEGACY_REGRESSION_SOURCES`: `src/gap.c`, `src/tree.c`, `src/group.c`, `src/bulk.c`. These V1–V3 mechanisms are linked additionally into historical regression/benchmark targets, not the ordinary production library. Sharing the src directory with order.c does not make them live V4 dependencies.

`LKS_ENABLE_ALLOC_DIAGNOSTICS` adds counters and deterministic allocation failure injection to test/measurement builds. Normal production has no such shared mutable diagnostic state. Research can screen `LKS_ORDER_BLOCK_CAPACITY`, but production defaults to **64**; that capacity is private, not frozen ABI.

## 2. Live Order structures: moving without global key rewriting

See [lks_order_internal.h](../src/lks_order_internal.h) and order.c.

### 2.1 Three object layers

`LksOrderHandle`, one live occurrence, has `block` (current block), `local` (array index), and `item` (non-NULL application pointer).

`LksOrderBlock` contains:

- `records[CAPACITY + 1]`: contiguous handle pointers with one temporary insertion slot;
- `count`: resident count;
- `previous/next`: neighbors in the full block sequence;
- `left/right/parent`, `height`, `subtree_blocks`: AVL metadata indexing blocks;
- `owner`: current Order, moved from resident to block metadata in v4.1.0;
- `tag`: private 64-bit order label; nonzero labels increase, zero selects AVL comparison fallback.

`LksOrder` holds AVL root, linked first/last blocks, occurrence count, block count, logical revision, callback_active and source_marker.

~~~text
Order
  AVL root ── B2 (block index, not user-visible Path)
              /    \
            B1      B3
  linked order: B1 <=> B2 <=> B3
  each block: [ H* H* H* ... ]  (up to 64 resident Handles)
  each H: { block, local, caller-owned item* }
~~~

This AVL is implicitly organized by block order, not keyed by business IDs or sorting strings. AVL nodes contain blocks rather than individual records. Arrays preserve local logical order and previous/next support direct cross-block traversal.

### 2.2 Invariants

Nonzero block labels strictly increase along the chain even across zero-labeled blocks. Two labeled blocks compare by labels; if either is zero, compare AVL order. Same-block comparison uses local indices. Labels are not application identity, and no label-recovery deadline is promised under arbitrary concentrated edits.

- Default committed capacity is **64**. In multi-block states each block has at least **32** records; the sole block may contain fewer and is removed when emptied.
- Each handle is separately allocated. Movement changes block/local, never its own address.
- Height/subtree_blocks must match actual AVL structure; subtree_blocks counts blocks, not records.
- AVL in-order and the doubly linked block sequence agree; records agree with logical item order.
- Ordinary insertion sets the new handle's block and updates shifted suffix local indices with v4_shifted_locations. Ordinary deletion clears the vacated tail. Split/merge/redistribution use v4_locations to update all affected locations and empty slots.

Diagnostic lks_order_internal_valid checks heights, parent links, occupancy, threading, handle locations, totals and both traversal directions. It is private and cannot validate freed dangling pointers.

### 2.3 Position comparison

Diagnostic in-order traversal also checks nonzero label monotonicity across zero blocks; the public comparison path is unchanged by that checker. See [verification](V4_2_0_PREVIEW1_2_1.md).

Two nonzero cross-block labels compare directly. Otherwise lift the lower-height AVL side along parents until ancestor/sibling directions establish order. Neither path computes ranks from subtree_blocks. Public structural bounds are same-block O(1), cross-block O(log(1+M)), with M blocks.

Next/previous access a local neighbor or the adjacent block's endpoint without an AVL search per item. Pointer numeric values never define order.

## 3. Explicit Order execution

Insertion checks arguments, callback state and capacities, then allocates its handle and any spare block. Preparation failure frees only temporaries. Once prepared blocks enter the chain/AVL, shifting, location repair, split and revision changes allocate nothing. Removal repairs blocks before freeing the removed occurrence and returning its borrowed item; other handle addresses survive. Entry points are in [order.c](../src/order.c).

### 3.1 Construction/destruction

`lks_order_create` allocates/zeros an Order without blocks or handles. `lks_order_destroy` follows first→next, frees each block's handles and then the block, releases provenance and the Order. Items remain application-owned. Cursors must no longer use the Order; callers prevent conflicting destruction/mutation and illegal callback access.

### 3.2 Insert: prepare, then non-failing commit

Front/back/before/after ultimately call `v4_insert(order, block, at, item, out_handle)`:

1. Clear output and validate Order/item/output and same-source relative anchor. Freed handles are invalid C lifetime use, not safely detectable input.
2. Check callback reentry and count/revision capacity.
3. Allocate an independent handle and a spare block if no block exists or the destination is full (64).
4. Only after all fallible allocation enter v4_place commit. OOM leaves old order/handles intact.
5. Shift pointers at `at`, insert, and if over capacity split approximately in half. v4_locations repairs both sets of handles. v4_link_after attaches the new block to list/AVL, and v4_tag_attach assigns labels or redistributes at most 64 blocks.
6. v4_balance refreshes height/subtree count along parents and performs needed single/double rotations.
7. Increment count/revision and return the new handle.

Splits move handle pointers, not items or handle objects. There is no dynamic Path rebuild. Structural target cost is O(B + log(1+M)), with private fixed capacity B. OS allocator/page-fault latency is outside this structural formula.

### 3.3 AVL maintenance preserves logical order

v4_link_after connects a block after a neighbor in the linked and implicit AVL orders; a right-subtree leftmost position supplies the attachment without a business-key search. Rotations preserve in-order order and update heights/counts/parents. v4_detach removes from list/AVL; two-child deletion physically transplants the successor block rather than copying its records, preserving handle→block relationships. v4_balance repairs along the affected parent path.

### 3.4 Remove an occurrence, not its item

Entry point: `lks_order_remove`.

1. Validate live same-owner handle, reentry and revision capacity.
2. Shift the array left, decrement block and Order counts.
3. Detach an emptied sole block.
4. If a multi-block source falls below half occupancy, v4_repair combines neighboring records in a stack array. Total≤64 merges and removes the right block; otherwise it redistributes into balanced occupancy.
5. Repair surviving block/local fields, increment revision, free the removed handle and optionally return its original item.

Removal never calls an item destructor, reallocates a surviving handle or allocates heap memory. The removed handle expires immediately.

### 3.5 Move cost does not scale with intervening distance

- Self, already-adjacent and already-at-endpoint moves return OK without allocation/revision change; existing Cursors stay valid.
- Same-block move adjusts destination for removal, shifts only the affected interval and updates its local indices without allocation.
- Cross-block move prepares a spare first if the destination is full; only then removes/places the pointer, splits if needed and repairs source underoccupancy.
- Actual movement increments revision; count, item and handle address stay unchanged.

This removes V3's live-Path global relabel burden, while retaining local array movement, AVL updates and allocator costs; it is not an O(1) bound for every operation.

### 3.6 Reads, Cursor and revision

First/last/next/previous/item/size provide borrowed reads under caller-controlled access. `lks_order_cursor_create` captures revision/next handle. `lks_order_cursor_next` validates arguments and source busy state, then checks revision **before touching cached handles**. Actual mutation returns INVALIDATED without dereferencing a potentially freed cached handle. No-op moves, failures and queries do not increment revision. Destroy the Cursor before its source.

Surviving handle identity does not freeze adjacency or comparison results. Ordinary iteration does not check revision. Save next before removing current and keep that saved occurrence alive, or use Cursor for change detection. See the [compilable API example](IMPLEMENTATION_API_REFERENCE.md#d1-handle-identity-does-not-freeze-adjacency).

## 4. ManagedOrder is a facade, not a second sorting tree

Managed insertion locates the comparator boundary, then uses shared live insertion; equality uses upper bound. Comparisons run under busy state. Before changing ordering fields, remove affected occurrences, modify and reinsert. A repeated item pointer may have several occurrences.

[managed_order.c](../src/managed_order.c) implements `{ core, comparator, active }`: a private Order, copied descriptor, borrowed context. It creates no second tree, business-key registry or handle type and exposes no mutable core.

`lks_managed_order_insert` sets active, searches AVL for the first block whose maximum is strictly greater, binary-searches that block's upper bound, then calls `lks_order_insert_before` before the found handle or at the back and clears active. New equal items follow old equals.

`lks_managed_order_locate` searches first ≥query and first >query to derive strictly lower predecessor, first equal and first strictly greater. All three output addresses must differ. Comparator equality is not occurrence identity.

During comparison, core callback_active and facade active reject same-source reentry. Independent containers are unaffected. This is not a thread lock.

Before changing comparator-visible data, handle **every occurrence** of the shared object in every relevant ManagedOrder and other sorted structures such as Groups. Removing one occurrence is insufficient; Full does not track cross-container sharing. Group has no in-place removal: rebuild/replace it.

Remove→modify→insert is not transactional. Removal expires every copy of the old handle; insertion may fail, and Full does not undo removal or restore the handle. Preserve items and enough order/identity information for application recovery; recovery insertion may also fail. ManagedOrder has no arbitrary drag-style move API.

## 5. Snapshot: history independent of live structure

Captures from the same source/revision may contain different association bytes if the application changes payloads. Every capture independently copies namespace/bytes. Currentness checks marker/revision, not payload equality.

See [snapshot.c](../src/snapshot.c), [lks_snapshot_internal.h](../src/lks_snapshot_internal.h). Snapshot stores no live handles, AVL/block layout or old item pointers. It holds `namespace_data`, the namespace byte copy (1..UINT32_MAX), occurrence count, cumulative row end offsets (equal adjacent ends mean empty association), concatenated copied association bytes, and marker/revision only for live-source currentness, not business identity.

### 5.1 Capture call chain

`lks_order_snapshot_capture`, `lks_managed_order_snapshot_capture` and `lks_group_snapshot_capture` enter common `lks_snapshot_capture_sequence`:

1. Validate options/reentry and copy the descriptor, preventing callback mutation of the original options from changing the operation.
2. lks_snapshot_new makes one allocation for header, row array and namespace, then copies namespace.
3. Iterate logical occurrences, call association at most once per row, copy its span via snap_append into a growing buffer and set cumulative end.
4. Trim association capacity to exact length; this realloc may fail, in which case nothing is published.
5. Only after all rows succeed create/retain SourceMarker and store revision, then return the complete Snapshot. OOM/callback failure destroys the temporary.
6. Full does not roll back application callback side effects.

Associations are application-chosen recovery information, often occurrence IDs; raw memory addresses are not permanent file identity. With no callback all associations are empty and a resolver may use ordinal.

Returned spans must remain valid after callback return through immediate copying; automatic local arrays expire too early. Use live item fields/caller-managed buffers. Nonzero length requires non-NULL data.

### 5.2 Historical survival

`LksSourceMarker` atomics protect marker lifetime. Snapshot namespace/associations/keys survive source Order/Group destruction. `snapshot_is_current` requires same live marker/revision; deserialized history has no original live provenance. These atomics do not provide general concurrent source mutation/capture safety.

### 5.3 LS1 historical keys

[snapshot_key.c](../src/snapshot_key.c) defines:

~~~text
LS1.<namespace lower-case hex>.<16-digit lowercase hexadecimal ordinal>
Example: LS1.64656d6f.0000000000000002
~~~

Length is `21 + 2 * namespace_size`, excluding NUL. Format encodes namespace bytes and Snapshot row as 16 lowercase hex digits. Validation rejects malformed syntax; `lks_snapshot_key_compare` requires same namespace or returns DOMAIN_MISMATCH, then compares ordinal. Different snapshots sharing namespace can assign the same key to different items, so applications separately identify snapshot/version.

Canonical same-namespace bytewise ASCII lexical order matches ordinal. Databases require byte-order-preserving collation, not linguistic/case-insensitive assumptions. Cross-domain string sorting does not define business order. LS1 is historical position, not cross-version identity, and differs from V3 LK1 Path encoding.

## 6. LKS4SNP1 binary representation

Helpers use unsigned shifts and byte access for 4/8-byte fields. Offset-one tests compare old golden bytes and alignment-independent decoding; full validation precedes allocation. [snapshot_wire.c](../src/snapshot_wire.c) writes caller buffers, not files.

### 6.1 Big-endian layout

~~~text
offset  width  contents
0       8      ASCII "LKS4SNP1"
8       4      reserved flags, must be 0
12      4      namespace byte length
16      8      occurrence count N
24      D      namespace raw bytes
...     8+Ai   row i: 64-bit association length Ai followed by Ai bytes
Repeat N times; no trailing bytes
~~~

Total is `24 + D + 8*N + sum(Ai)`. Preview.2 snap_be_write/read specialize fixed widths while retaining explicit big-endian byte access; this is not native-struct fwrite and has no native struct alignment premise.

`lks_snapshot_serialize` checks destination size, returns BUFFER_TOO_SMALL without writing on shortage, otherwise writes without heap allocation. `lks_snapshot_deserialize` scans/validates magic, flags, domain/count, row lengths, arithmetic and exact tail, then allocates/copies independent history. Disk transactions and checksum protection against silent corruption belong to an upper persistence protocol; versioning alone is not database reliability.

### 6.2 Restoring editable Order

[snapshot_restore.c](../src/snapshot_restore.c) implements restore:

1. Allocate N temporary item pointers; empty input omits that array.
2. Resolve rows in order, stopping on first error or OK/NULL item. Later rows are not called. Only complete resolution permits unpublished bulk construction. Empty input allows NULL resolver and calls none, but new empty Order allocation may still fail. Failure clears output/frees Full temporaries, never callback side effects or resolved application items. Duplicate pointers are allowed.
3. Call lks_order_bulk_build to create a fresh live Order.
4. order_bulk.c computes block count, fills evenly, allocates each fresh handle and threads blocks. snap_bulk_index recursively selects middle blocks for balanced AVL and computes heights/subtree_blocks.
5. Return fresh Order/handles, not old pointer identity. Failure output NULL; application objects/effects remain application responsibilities.

Restoration costs include a handle allocation per occurrence and N-pointer scratch; it is not O(1) mapping of an on-disk AVL.

## 7. Group, Batch and one-shot sorting

See [immutable_group.c](../src/immutable_group.c), [sort.c](../src/sort.c), [private structures](../src/lks_immutable_group_internal.h).

- Sort copies input pointers, recursively stable-merge-sorts the copy, and commits by memcpy after allocation/comparison completes. OOM leaves original input intact.
- `lks_group_build` stable-sorts copied pointers into its own array; never owns item contents.
- `lks_group_merge` requires compatible sorted sources; base/left wins equality, preserving source precedence.
- `lks_group_batch_build` partitions by group_size, builds each Group independently and owns those Groups.
- `lks_group_batch_merge_all` allocates scratch only for more than one chunk, doubles widths from group_size and returns a separately owned Group array. Empty/single chunk skips this scratch. Earlier source wins equality at every merge.
- `lks_group_snapshot_capture` uses the same Snapshot/LS1/wire pipeline. Logical revision is zero; marker tracks source Group currentness.
- Busy guards reject source reentry/destruction during comparator/association callbacks; they are not thread mutexes.

These structures have no dynamic handles/live AVL; Group is not another Mutable Order.

## 8. Why private V3 Path/LK1 remains

Current production import follows `lks_order_import_v3_lk1` → `lks_path_order_key_parse` → temporary `LksPath` → `lks_path_compare`/sort → duplicate rejection → bulk-build Path-free V4 Order. All temporary Paths are freed on success/failure.

- [path.c](../src/path.c): directions, 16-bit slots, size_t levels, append/clone/destroy, shared capacity-based step allocation.
- [slot_codec.c](../src/slot_codec.c): private radix-54, three-character display slots excluding ambiguous characters.
- [path_text.c](../src/path_text.c): historical canonical display/parser; display cannot replace a sortable LK1 key.
- [path_order_key.c](../src/path_order_key.c): strict versioned LK1 ASCII order encoding/parser for legacy import, with direction/level/slot.
- [path_compare.c](../src/path_compare.c): historical negative/zero/positive and level/slot semantics.

Gap generation, Path-keyed Tree/OrderedTree, old Group/Batch and bulk coordinates in gap.c/tree.c/group.c/bulk.c are extra links only for historical regression/benchmarks. Their presence does not reopen V3 public APIs or imply a second live production engine. Production path files exist for legacy compatibility, not live Path relabeling.

## 9. Allocation, errors, reentry, threads and lifetime

See [allocator](../src/lks_alloc.c), [private header](../src/lks_alloc_internal.h) and public comments.

### 9.1 Allocation layers

Production wrappers use standard malloc/realloc/free without global diagnostic state or SQLite/LMDB dependencies. Diagnostics track allocation calls, live/peak requested bytes, categories and deterministic failure points; some old tags are historical categories. Records, blocks, snapshots and scratch are allocated separately. v4.1.0 reduced per-resident/Snapshot metadata, but still allocates each resident independently. Strong atomicity covers specified API state on recoverable errors, not external callback effects, disk transactions or power loss.

### 9.2 Errors

`LKS_STATUS_INVALID_ARGUMENT` covers NULL/invalid/foreign handles; `LKS_STATUS_OUT_OF_MEMORY` fallible allocations; `LKS_STATUS_BUFFER_TOO_SMALL` formatting/serialization; `LKS_STATUS_CAPACITY_LIMIT` protected counts/revisions; `LKS_STATUS_INVALIDATED` changed-source Cursor; `LKS_STATUS_REENTRANT` same-source callback access; `LKS_STATUS_DOMAIN_MISMATCH` different LS1 namespaces; `LKS_STATUS_ALREADY_EXISTS` duplicate imported coordinates. Pointer getters may return NULL for invalid/busy input; this cannot make freed-handle access safe.

### 9.3 Synchronization

Completed immutable Snapshot reads can run concurrently if lifetime is externally protected. Group pure reads require no concurrent destruction, merge/capture state access or unsafe item reads. Merge/capture changes busy/marker state; logical immutability does not imply arbitrary thread safety. Caller synchronizes Order/ManagedOrder accesses involving mutation/destruction. Marker atomics and callback guards are not container locks; keep items/context alive as required.

## 10. Performance and applicability

The 4.2 candidate compares local indices within a block, nonzero labels across blocks, or AVL parent/height when either label is zero. Attachment maintains at most 64 blocks; ordinary edits update affected slots. Observed Full/Stable 4.1 median live ratios are Windows **0.629106**, Linux **0.640047**; compare-heavy **0.197718**, **0.219173**. Endpoint lifecycle versus flat history is **1.137284**, **1.136323**; live ratios do not transfer to every application. Native tag-range OM wins some conditions. See [candidate evidence](FULL_RC1_EVIDENCE.md) and [selection guide](FULL_SELECTION_GUIDE.md).

- Live structural work depends on local capacity B and AVL block count M, not rewriting every intervening external coordinate.
- Whole Snapshot processing visits N occurrences and copies A association bytes; these explicit linear costs remain even when live edits improve.
- Historical v4.1.0 evidence retains regressions in random lookup, namespace key formatting, wide-association serialization and some churn/move/destruction cases.
- Requested bytes differ from RSS and exclude application objects, allocator metadata/fragmentation and page caches.
- Database integration needs real application traces before considering incremental persistence/rank APIs. Full has neither native disk-order files nor general database transactions.

[4.1.0 changes/limits](V4_1_0.md), [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3), and [historical Issue #4](https://github.com/RXY712200/LayerKeySort/issues/4) explain tradeoffs; source determines behavior.

### 10.1 Simultaneously live memory

N is occurrence count and A total association bytes. These are coexistence/ownership relationships, not architecture-independent byte sizes or RSS bounds.

| Operation | May coexist at peak | Ownership after return |
| --- | --- | --- |
| Live Order | Handles, blocks, unused slots, application items, optional insertion/full-target move spare | Order frees structure, application frees items |
| Capture | Source, header/rows/namespace, association growth/slack; realloc overlap depends on allocator | Snapshot owns exact bytes and marker reference; source independent |
| Serialize | Snapshot and caller output buffer | Separate owners; no encoding heap allocation |
| Deserialize | Caller input and newly copied Snapshot rows/namespace/associations | Independent Snapshot, input remains caller-owned |
| Restore | Snapshot, N-pointer scratch, partially built Order, resolved application objects | Scratch freed; fresh Order borrows items; failure never frees items |
| Sort/Group build | Input, sorted/result, merge scratch; Group header for build | Sort commits/frees scratch; Group retains result array; neither owns items |
| Group/Batch merge | Sources and fresh result; merge_all scratch only for >1 chunk, none for ordinary two-source merge | Sources unchanged, result independent, scratch freed |

Allocator metadata, caches, fragmentation, item storage and extra caller buffers can increase process memory.

## 11. Editing, saving and reopening end to end

Applications persist encoded bytes and business identity. Restore resolves every row before building/publishing a fresh Order. Failed callback processing cleans Full temporaries only; external effects/disk commit remain application concerns.

~~~text
Application owns layer records A/B/C and business IDs
  lks_order_create()
    insert_back(A) -> HA; insert_back(B) -> HB; insert_back(C) -> HC
Move C before A:
  lks_order_move_before(HC, HA)
    validate/no-op check
    same-block shift OR prepare then remove/place across blocks
    maintain blocks/AVL if needed; revision++
Save:
  lks_order_snapshot_capture(...)
    iterate/copy application associations
    lks_snapshot_serialize(...) -> caller buffer
      application commits bytes through its own file protocol
Reopen from application storage:
  lks_snapshot_deserialize(bytes)
    lks_snapshot_restore_order(snapshot, resolver)
      resolve associations to live application pointers
      bulk-build balanced blocks and fresh handles
      fresh editable Order
~~~

File I/O, commit, business IDs and fault recovery belong to the caller. To restore distinct occurrences of a repeated item pointer, associations must distinguish occurrences; item ID alone may not establish a one-to-one mapping.

## 12. Source-to-test entry map

| Audit/change target | Primary source | Existing tests |
| --- | --- | --- |
| Handle/block/AVL invariants | lks_order_internal.h, order.c | tests/v4_order.c, tests/v4_mutation.c |
| Mutation failure atomicity | v4_insert/v4_place/v4_move/v4_repair | tests/v4_order.c, tests/v4_mutation.c |
| Managed order/equality | managed_order.c | tests/v4_mutation.c |
| Cursor/revision | order.c Cursor functions | tests/v4_mutation.c |
| Snapshot/marker/callback | snapshot.c, lks_snapshot_internal.h | tests/v4_snapshot.c, tests/v4_marker_threads.c |
| LS1 | snapshot_key.c | tests/v4_snapshot.c, tests/v4_rc_torture.c |
| Wire/corruption rejection | snapshot_wire.c | tests/v4_snapshot.c, tests/v4_wire_probe.c, tools/verify_v4_wire.py |
| Restore/bulk | snapshot_restore.c, order_bulk.c | tests/v4_snapshot.c |
| Group/Batch/sort | immutable_group.c, sort.c | tests/v4_immutable_group.c |
| Legacy LK1 | path_order_key.c, path_compare.c, snapshot_restore.c | tests/v4_snapshot.c, historical parser/regressions |
| API freeze | include/layerkeysort.h | tests/v4_api_compile.c, tests/v4_freeze.c, tools/verify_v4_api.py |
| Production/two-file package | cmake/ProductionSources.cmake, CMakeLists.txt, tools/amalgamate.py | [Validation](VALIDATION.md), distribution validators |

This maps inspected source to relevant test entries; it does not claim this review independently executed every assertion. Historical release configurations ran 39 CTests, but options/Python affect availability; confirm actual ctest -N and execution output.

## 13. Reproducing source review/build

Pin a revision rather than following moving main:

~~~sh
git clone https://github.com/RXY712200/LayerKeySort.git
cd LayerKeySort
git checkout d1132a26ca7b2c65bfd512fa2eb29f1b16793c08
git rev-parse HEAD
cat cmake/ProductionSources.cmake

cmake -S . -B build -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
~~~

Follow [validation](VALIDATION.md) for API manifest, distribution, C/C++ consumers and platform/sanitizer checks; select the correct compiler/generator for the host.

The reconstruction inspected all 16 production C files/major headers at fixed GitHub revisions, production manifest/root CMake, related call paths, representative V4 tests and legacy-source ownership. Initial RC strict Windows GCC Debug/Release each ran 39/39; six hosted configurations and extended sanitizer are in [initial Full PR CI](https://github.com/RXY712200/LayerKeySort/actions/runs/45518848531). Repair commits require their own exact-head checks. Initial RC validation did not individually rerun historical CSVs, entire external systems or every fault combination.

## 14. Keeping explanations aligned with source

- Update affected sections in the same PR as substantive implementation changes; pin revision, old/new call flow and observable semantics. Never rewrite old data to fabricate history.
- Consumer changes go in the [handoff ledger](FULL_CONSUMER_HANDOFF.en.md). Introducing [PR #24](https://github.com/RXY712200/LayerKeySort/pull/24) merged at 2026-11-23 06:02:54 UTC; new candidate entries remain candidate-branch content.
- This walkthrough locates the complete flow and source, rather than hiding implementation behind prose. Verify real diffs/source and appropriate tests when changing it.
- Resolve conflicts with actual functions/diffs/reproduction, not conversation memory or treating prose as evidence of runtime behavior.

Full keeps changing logical order in **stable occurrence handles + ordered block chain + bounded labels and block-level AVL fallback**, historical order in **independent Snapshot + LS1 + explicit binary wire**, and one-shot sorting in **stable copy/merge**. Application identity and physical persistence stay above these boundaries.
