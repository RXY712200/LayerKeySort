# Full v4.2.0: complete public API and contract guide

Released source: [`v4.2.0`](https://github.com/RXY712200/LayerKeySort/tree/v4.2.0), commit `f751e1ee2afd682874281d6dc495969f9eca49eb`. RC/Preview test pins below are retained historical verification baselines.

[简体中文](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [Exact declaration index](design/V4_2_0_RC1_API_INDEX.md) · [Source reconstruction](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md)

This edition applies to Full v4.2.0. Operational implementation and regression pins remain unchanged from RC.1; only release-version macros change to 4.2.0. See the [4.2.0 release and distribution record](V4_2_0.md).
This edition covers the root Full library. Operational source is pinned to `fe00536e7bd7f4e5128432ea4a29e54158074903`; the RC metadata/test baseline is `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`, containing the two Preview.3 regressions. The [public header](../include/layerkeysort.h) and exact declaration index specify parameter order and types. The 4.x interface remains frozen. Serialization bytes, statuses, ownership and borrowing contracts are unchanged; offset-one buffers are covered by regression tests. See [validation/source evidence](V4_2_0_PREVIEW3.md).

## A. Reading this guide

The following 64 entries correspond individually to public declarations. Error precedence follows the validation order of each entry point. For example, explicit compare clears its output, validates arguments and owners, and then checks callback reentry. An expired handle must never be passed to any API.

- An **occurrence** refers to one non-NULL application-owned `void *item`. Repeated insertion of the same item pointer creates distinct `LksOrderHandle` instances.
- Full allocates Order, ManagedOrder, Group, Batch, Snapshot and Cursor structures; their successful creators must arrange destruction. Application items, comparator contexts, association-data sources and external files remain application responsibilities.
- `LksStatus` values are frozen. Non-status APIs may return NULL, zero or no action for invalid inputs. NULL handling does not detect dangling pointers.
- Handles are borrowed views, expiring on removal of their occurrence or destruction of their Order. Destroy a Cursor before its source. Snapshot byte views expire when their Snapshot is destroyed.
- Callbacks must not reaccess the same mutable source while it is busy. Comparators must define a consistent ordering. Callbacks must return normally, without `longjmp` or C++ exceptions crossing the C call.
- Callers synchronize conflicting container access. Atomic SourceMarker reference counting does not lock the container.
- Each entry below identifies its main production file. The internals guide explains call paths and the validation guide maps tests and evidence. Consult the header for detailed output clearing and invalid-input clauses not repeated here.

## B. Complete 64-function index by use case

Each row is one actual public function. All implementation paths belong to the current production manifest.

### B1. Status and one-shot sorting (2)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_status_string` | Returns a static library-owned description; unknown values return `Unknown status`. No allocation; never free the string. | `src/lks_base.c` |
| `lks_sort` | Stable-sorts the caller's pointer array. Allocates copied input and merge scratch before replacing the original array on success. OOM preserves the array; `count<2` allocates nothing. | `src/sort.c` |

### B2. Explicit Order: construction, mutation and queries (19)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_order_create` | Creates an empty Order; NULL on allocation failure. Does not create application objects. | `src/order.c` |
| `lks_order_destroy` | Frees blocks, handles and marker references, never application items. Accepts NULL. | `src/order.c` |
| `lks_order_size` | Returns the occurrence count; zero for NULL or callback-busy input. | `src/order.c` |
| `lks_order_insert_front` | Creates an independent occurrence and handle at the front; prepares any new/split block before commit. | `src/order.c` |
| `lks_order_insert_back` | Creates an occurrence at the back with the same preparation guarantees. | `src/order.c` |
| `lks_order_insert_before` | Inserts before a live same-Order anchor; a live foreign handle is invalid. | `src/order.c` |
| `lks_order_insert_after` | Inserts after a live same-Order anchor. | `src/order.c` |
| `lks_order_remove` | Removes the exact occurrence and optionally returns its borrowed item. Only the removed handle expires. | `src/order.c` |
| `lks_order_first` | Borrows the first handle; NULL for an empty Order. | `src/order.c` |
| `lks_order_last` | Borrows the last handle; NULL for an empty Order. | `src/order.c` |
| `lks_order_next` | Advances one local slot, or to the next block's first slot; NULL past the end. | `src/order.c` |
| `lks_order_previous` | Moves one local slot backward, or to the previous block's last slot; NULL before the beginning. | `src/order.c` |
| `lks_order_item` | Returns the original borrowed item pointer from a live handle; no copy or ownership transfer. | `src/order.c` |
| `lks_order_compare` | Compares live same-Order occurrences: local slots within a block, labels across blocks, AVL parent/height fallback if either block has no label. Worst-case structural comparison is O(log(1+M)); result is negative/zero/positive. | `src/order.c` |
| `lks_order_move_front` | Moves an occurrence to the front, preserving its handle; already-first is a no-op. | `src/order.c` |
| `lks_order_move_back` | Moves an occurrence to the back; already-last is a no-op. | `src/order.c` |
| `lks_order_move_before` | Moves before a live same-Order anchor; self/already-adjacent is a no-op without revision increment. | `src/order.c` |
| `lks_order_move_after` | Moves after a live same-Order anchor; self/already-adjacent is a no-op. | `src/order.c` |
| `lks_order_snapshot_capture` | Creates independent immutable history from Explicit Order; does not write a file. | `src/snapshot.c` |

Snapshot capture is counted once, here. The Explicit API excluding Snapshot capture has 18 functions.

### B3. Cursors, source currentness and legacy import (6)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_order_cursor_create` | Allocates a revision-capturing Cursor, initially at first/last according to direction 0/1. | `src/order.c` |
| `lks_order_cursor_next` | Returns successive handles; end is `OK + NULL`; actual mutation produces `INVALIDATED + NULL` before touching cached handles. | `src/order.c` |
| `lks_order_cursor_destroy` | Frees only the Cursor; does not destroy or change its borrowed Order. | `src/order.c` |
| `lks_order_snapshot_is_current` | True only for matching source marker and revision. Loaded historical Snapshots have no live marker and return false. | `src/snapshot.c` |
| `lks_order_import_v3_lk1` | Strictly parses legacy LK1, sorts by Path, rejects duplicate coordinates, and bulk-builds a new Path-free V4 Order. | `src/snapshot_restore.c` |
| `lks_managed_order_cursor_create` | Creates a Cursor through the ManagedOrder's private Explicit core; no mutable core is exposed. | `src/managed_order.c` |

### B4. Comparator-managed order (10)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_managed_order_create` | Validates and copies the comparator descriptor, borrows callback/context and creates a private `LksOrder`. | `src/managed_order.c` |
| `lks_managed_order_destroy` | Destroys facade and core, never items or context. | `src/managed_order.c` |
| `lks_managed_order_size` | Returns occurrence count; NULL/busy returns zero. | `src/managed_order.c` |
| `lks_managed_order_insert` | Finds the upper bound and inserts before it or at the back; new comparator-equal occurrences follow old ones. | `src/managed_order.c` |
| `lks_managed_order_locate` | Finds lower/upper bounds and returns strictly lower predecessor, first equal and first strictly greater successor. Three output addresses must be distinct. | `src/managed_order.c` |
| `lks_managed_order_remove` | Removes by handle without comparing item data. Before changing shared comparator-visible data, handle all relevant occurrences. Remove/reinsert is not transactional; see E. | `src/managed_order.c` |
| `lks_managed_order_first` | Borrows the first comparator-ordered occurrence. | `src/managed_order.c` |
| `lks_managed_order_last` | Borrows the last occurrence. | `src/managed_order.c` |
| `lks_managed_order_compare` | Uses core occurrence order; does not dynamically re-sort changed fields. | `src/managed_order.c` |
| `lks_managed_order_snapshot_capture` | Captures the core after passing the ManagedOrder reentry guard. | `src/snapshot.c` |

### B5. Managed currentness (1)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_managed_order_snapshot_is_current` | Checks core source marker and revision against the Snapshot. | `src/snapshot.c` |

### B6. Snapshot, LS1, wire and restoration (12)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_snapshot_destroy` | Frees copied bytes, Snapshot and marker reference. Accepts NULL. | `src/snapshot.c` |
| `lks_snapshot_count` | Returns row count; NULL returns zero. | `src/snapshot.c` |
| `lks_snapshot_namespace` | Outputs borrowed namespace bytes/length, valid until Snapshot destruction. | `src/snapshot.c` |
| `lks_snapshot_association` | Outputs borrowed row association bytes/length; empty association is `NULL,0`. | `src/snapshot.c` |
| `lks_snapshot_key_length` | Returns LS1 character count excluding terminal NUL, for buffer sizing. | `src/snapshot_key.c` |
| `lks_snapshot_key_format` | Formats `LS1.<ns-hex>.<ordinal-hex>` for one row; insufficient space leaves destination unchanged. | `src/snapshot_key.c` |
| `lks_snapshot_key_validate` | Strictly validates canonical LS1 byte syntax without requiring a source Snapshot. | `src/snapshot_key.c` |
| `lks_snapshot_key_compare` | Validates both keys, compares ordinal in the same namespace, otherwise returns `DOMAIN_MISMATCH`. | `src/snapshot_key.c` |
| `lks_snapshot_serialized_size` | Returns the exact **LKS4SNP1** binary byte count; NULL returns zero. | `src/snapshot_wire.c` |
| `lks_snapshot_serialize` | Writes exact wire bytes into caller memory, never creates a file; insufficient space preserves destination. | `src/snapshot_wire.c` |
| `lks_snapshot_deserialize` | Reads fixed-width big-endian fields; validates magic, flags, row lengths, overflow and trailing input before allocating/copying a complete Snapshot. Error/output contracts are unchanged. | `src/snapshot_wire.c` |
| `lks_snapshot_restore_order` | Resolves non-NULL items row by row and bulk-builds a fresh Order. Fresh handles do not inherit old process addresses. | `src/snapshot_restore.c` |

### B7. Group/Batch: immutable arrays and stable grouped sorting (14)

| API | Purpose and central semantics | Implementation |
| --- | --- | --- |
| `lks_group_build` | Copies/stable-sorts pointers into an independent Group. Empty input is allowed; callable comparator is required. | `src/immutable_group.c` |
| `lks_group_destroy` | Frees pointer array, marker and structure, never items. | `src/immutable_group.c` |
| `lks_group_size` | Returns Group count; NULL/busy returns zero. | `src/immutable_group.c` |
| `lks_group_item_at` | Borrows the original item at sorted position; NULL out of range. | `src/immutable_group.c` |
| `lks_group_merge` | Produces a fresh merged Group; base precedes incoming on equality. Both inputs may be the same Group. | `src/immutable_group.c` |
| `lks_group_batch_build` | Splits input into `group_size` chunks and independently sorts each; Batch owns chunks. | `src/immutable_group.c` |
| `lks_group_batch_destroy` | Destroys Batch and all owned chunks. Never separately destroy a borrowed chunk view. | `src/immutable_group.c` |
| `lks_group_batch_size` | Returns total item count across the Batch. | `src/immutable_group.c` |
| `lks_group_batch_group_count` | Returns actual chunk count. | `src/immutable_group.c` |
| `lks_group_batch_group_size` | Returns configured target chunk size; final chunk may be smaller. | `src/immutable_group.c` |
| `lks_group_batch_group_at` | Borrows a Batch-owned Group until Batch destruction. | `src/immutable_group.c` |
| `lks_group_batch_merge_all` | Width-doubling stable merge of sorted chunks into a fresh Group; earlier chunks precede later chunks on equality. | `src/immutable_group.c` |
| `lks_group_snapshot_capture` | Captures Group order using the common Snapshot export machinery. | `src/immutable_group.c` |
| `lks_group_snapshot_is_current` | True only for the same Group marker and revision zero; does not establish unchanged application fields. | `src/immutable_group.c` |

If shared payload mutation invalidates GroupBatch sorted chunks, the application must reconstruct or replace the Batch. Borrowed Batch-owned Group views cannot be independently replaced or destroyed and remain valid only while their Batch lives.

### B8. Inputs, failure and ownership by family

This table summarizes header and implementation. Validation has an order; several simultaneous invalid conditions do not have one universal status precedence. All borrowed objects must remain live. NULL handling does not validate expired pointers.

| Family | Required/NULL/empty inputs | Outputs and principal errors | Reentry, allocation and lifetime |
| --- | --- | --- | --- |
| Status/sort | Unknown enum accepted; sort requires compare even when count=0/items=NULL | Static unknown-status text; sort INVALID_ARGUMENT, allocation/oversize OUT_OF_MEMORY; original array preserved | count<2 allocates nothing; otherwise copied/scratch arrays; borrowed items/context; caller protects comparator and array from callback interference; no container busy guard |
| Explicit construction/read | create has no arguments; destroy(NULL) harmless; size(NULL)=0, pointer getters(NULL)=NULL | create returns NULL on OOM; empty/end getters NULL; compare requires output and live same-owner handles, failure output zero | Creation allocates; read/compare do not; busy getters NULL/0, compare REENTRANT, destroy ignored; Order owns structure and borrows items |
| Explicit insert/remove/move | Insert requires Order/item/out_handle and live same-owner relative anchor; remove out_item optional; empty insertion allowed, absent occurrence cannot be removed/moved | Insert failure output NULL; remove failure item NULL; INVALID_ARGUMENT, REENTRANT, CAPACITY_LIMIT; insert/spare-requiring move may OUT_OF_MEMORY | Prepare handle/optional block; remove allocates nothing; actual changes advance revision; no-op may succeed at revision capacity; live handles stable except removed occurrence |
| Managed | Callable comparator at creation; descriptor copied/context borrowed; insert item/output required; locate requires query and three distinct outputs, query need not be resident | Constructor NULL for invalid/OOM; status APIs INVALID_ARGUMENT/REENTRANT; insert inherits core capacity/OOM; locate clears outputs and missing equality still OK; compare clears result | Bound/locate/remove/compare allocate nothing; removal does not compare; busy getters NULL/0, destroy ignored; resident ordering semantics cannot change; no public move/core |
| Cursor | Source/output required; reverse only 0/1; empty source allowed | Creation output NULL on INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY; next clears output, ends OK/NULL, mutation INVALIDATED/NULL | Creation allocates; next/destroy do not; destroy(NULL) harmless; source callback reentry REENTRANT; Cursor must be destroyed before source |
| Snapshot capture/currentness | Source/options/output required; namespace 1..UINT32_MAX valid bytes; association callback optional; empty capture allowed | Capture clears output; INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY/CAPACITY_LIMIT or callback error; NULL/busy/loaded/different-source currentness false | Allocates rows/namespace/associations/optional marker, checks capacity/arithmetic before publication; failure preserves source revision; Snapshot owns bytes; callback-span lifetime in E |
| Snapshot views/LS1/wire | Both view outputs required; association/key format requires valid row; format/serialize destination required; NULL count/length/serialized_size zero | View failure NULL/0; compare clears result; INVALID_ARGUMENT/BUFFER_TOO_SMALL; cross-domain DOMAIN_MISMATCH; deserialize output NULL, INVALID_ARGUMENT/OUT_OF_MEMORY | Views/encoding do not allocate; short buffer untouched; deserialize validates before allocation; views borrow until destruction, destroy(NULL) harmless |
| Restore/LK1 import | Snapshot/output required for restore; import output required; nonempty restore requires resolver, empty allows NULL; empty import entries may be NULL; nonempty keys/items valid and non-NULL | Clears output; INVALID_ARGUMENT/OUT_OF_MEMORY, callback error forwarded; duplicate LK1 ALREADY_EXISTS; values outside destination representation rejected, never truncated | Resolve then bulk-build; import parses/sorts first; even empty output may OOM; caller owns fresh Order and items remain borrowed; callback side effects not rolled back |
| Group/Batch | Build/merge require callable comparator/output; empty build items=NULL allowed; nonempty entries non-NULL; Batch group_size>0; merge inputs must be comparator-compatible | Clears output; INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY; NULL/busy sizes zero, out-of-range/busy views NULL | Build/merge allocate; count/array arithmetic may INVALID_ARGUMENT, actual allocation OOM, not universally CAPACITY_LIMIT; busy destroy ignored; Batch owns borrowed chunk views; merge_all scratch only for multiple chunks |

### B9. Shared handle accessors for ManagedOrder

This function can be compiled in a C file including `layerkeysort.h`. The caller keeps source/items alive and prevents conflicting changes. `visit` must not change or destroy the source.

```c
static void visit_managed(const LksManagedOrder *order, void (*visit)(void *))
{
    const LksOrderHandle *h;
    if (!order || !visit) return;
    for (h = lks_managed_order_first(order); h; h = lks_order_next(h))
        visit(lks_order_item(h));
    for (h = lks_managed_order_last(order); h; h = lks_order_previous(h))
        visit(lks_order_item(h)); /* Deliberate second visit in reverse order. */
}
```

These shared handle accessors are supported; obtaining a mutable core is neither necessary nor possible through the public API.

## C. Types, enum and callbacks

The frozen fixture [`tests/fixtures/v4_public_api.json`](../tests/fixtures/v4_public_api.json) is the inventory of **14 public types**. Important objects/callbacks are:

| Type | Purpose | Holding rule |
| --- | --- | --- |
| `LksStatus` | Explicit integer statuses | Frozen values; no reordering |
| `LksComparator` / `LksCompareFn` | Comparison and context | Managed copies descriptor, borrows context; sort/group borrow only during operation |
| `LksOrder` / `LksOrderHandle` | Live container and occurrence view | Order frees structure; removed handle expires |
| `LksManagedOrder` | Comparator-managed facade | Owns private Order core |
| `LksOrderCursor` | Revision-checked traversal | Borrows source |
| `LksSnapshotOptions` / `LksSnapshotAssociationFn` | Namespace, callback and association bytes to copy | Full immediately copies callback data |
| `LksSnapshot` | Independent immutable historical order | Owns bytes; caller destroys |
| `LksSnapshotResolveFn` | Resolves item from association/ordinal | Resolved item remains application-owned |
| `LksV3Lk1ImportEntry` | One-time legacy LK1 migration input | Key/item borrowed from caller |
| `LksGroup` / `LksGroupBatch` | Immutable sorted group/chunks | Group owns pointer array; Batch owns its Groups |

**Status values are intentionally non-contiguous:**

| Value | Name | Typical trigger |
| ---: | --- | --- |
| 0 | `OK` | Success |
| 1 | `INVALID_ARGUMENT` | NULL arguments, aliased outputs, malformed identifiers |
| 2 | `OUT_OF_MEMORY` | Allocation failure |
| 3 | `BUFFER_TOO_SMALL` | Formatting/serialization destination too short |
| 5 | `NOT_FOUND` | May be returned by application callbacks |
| 7 | `INTERNAL_ERROR` | Defensive internal failure |
| 8 | `ALREADY_EXISTS` | Duplicate legacy LK1 coordinate |
| 9 | `CAPACITY_LIMIT` | Count, revision or reference-count boundary |
| 10 | `INVALIDATED` | Cursor observes actual source mutation |
| 11 | `REENTRANT` | Forbidden same-source access during callback |
| 12 | `DOMAIN_MISMATCH` | Different LS1 namespaces |

Holes 4 and 6 are intentional. Do not change ABI or persistent formats to fill them.

## D. Ownership and invalidation in use

Two captures at the same revision still own independent historical bytes. Row/namespace views borrow their Snapshot. Restore resolves application-owned items in row order and creates a fresh handle per occurrence.

| Event | Order | Handle | Cursor | Snapshot | External item |
| --- | --- | --- | --- | --- | --- |
| Insert | Same object, revision increases | Existing addresses stable; new handle independent | Invalidated | Historical bytes readable; currentness false | Never freed by Full |
| Remove one occurrence | Container survives; count−1 | Removed expires, all others survive | Invalidated | Historical bytes readable | Never freed by Full |
| Actual move | Same container; revision +1 | All live addresses stable | Invalidated | Historical bytes readable | Never freed by Full |
| No-op move | Order/revision unchanged | All remain valid | Remains valid | May remain current | Unchanged |
| Ordinary failure such as OOM | Original state preserved where strong atomicity is promised | Existing live handles valid | Not invalidated merely by failure | Unaffected | Callback side effects separate |
| Destroy Order | Expires | All expire | Must already be destroyed | Bytes survive; source currentness false | Still application-owned |
| Deserialize/restore | Fresh different Order | All handles fresh | Create new Cursor | Loaded Snapshot has no live provenance | Resolver supplies live items |

Do not destroy an item while a handle still points to it, reuse old handles for restored records, persist raw handle pointers, treat LS1 as a cross-Snapshot primary key, mutate/traverse across threads without synchronization, or modify the source during capture callbacks.

### D1. Handle identity does not freeze adjacency

A surviving handle does not preserve previously observed next/previous relationships or comparison results. Insert/move can change these. Ordinary traversal has no automatic revision check; Cursor detects actual changes with INVALIDATED, while no-op/failure preserves validity.

Save the next live handle before removing the current occurrence. Saving does not extend lifetime: if that saved occurrence is subsequently removed, its handle expires too.

```c
static LksStatus remove_all(LksOrder *order)
{
    const LksOrderHandle *h, *next;
    if (!order) return LKS_STATUS_INVALID_ARGUMENT;
    for (h = lks_order_first(order); h; h = next) {
        LksStatus status;
        next = lks_order_next(h);
        status = lks_order_remove(order, h, NULL);
        if (status != LKS_STATUS_OK) return status;
        /* h expired; next remains valid only while its occurrence/source lives. */
    }
    return LKS_STATUS_OK;
}
```

## E. Less obvious error-handling boundaries

Serialization/deserialization accept byte buffers satisfying length requirements, including offset-one addresses. Deserialization completely checks lengths, flags and trailing bytes before publishing a Snapshot. Application callback side effects remain application responsibilities.

Before modifying comparator-visible fields, handle **every occurrence** referring to that object in every relevant ManagedOrder, and other sorted structures such as Groups. Removing only one occurrence is insufficient: Full does not track shared items across containers. Groups have no in-place removal; arrange reconstruction/replacement.

**Remove → modify → insert is several operations, not a Full transaction.** Successful removal expires all copies of the original handle immediately. A later insertion can fail, including OOM; Full does not undo prior removal or restore its handle. Keep the item and sufficient identity/order information for application-level reconstruction/recovery. Recovery insertion can itself fail and must be checked.

**Association span lifetime:** returned data must remain valid after callback return until Full finishes the immediate copy. Never return a local automatic array, whose lifetime ends at return. Use a live item field or caller-managed storage. Nonzero length requires non-NULL data.

1. Relative anchors must be live handles belonging to the same Order. A live foreign handle can be rejected; detecting a freed dangling handle is not promised.
2. Insert requires a valid `out_handle` address, cleared to NULL on failure. Remove's optional `out_item`, when supplied, is cleared first.
3. Managed locate requires three distinct output addresses: `out_predecessor`, `out_equal`, `out_successor`. NULL query results do not permit aliasing their storage.
4. `namespace_size` must be nonzero. The application selects namespace bytes; reusing a namespace does not automatically make keys from different captures unique.
5. Key length excludes `\0`, so allocate at least `length+1`. Binary serialized size is exact bytes, without a text terminator.
6. Restore calls the resolver in row order, stopping at the first non-OK or OK/NULL item; later rows are not called. Only after all required rows resolve does unpublished bulk construction begin. Empty Snapshot accepts NULL resolver, calls none, but still allocates a new empty Order and may OOM. Failure frees Full temporary storage and clears output, never rolls back callback effects or frees resolved items.
7. Group/Batch merging requires compatible comparator semantics. Changing comparator-visible fields can invalidate sorted-input prerequisites.
8. Cross-block move to a full destination prepares a spare first. OOM causes no logical order change. Same-block move allocates nothing.
9. V4 is frozen: proposed rank, at or persistent-ID functions are not existing APIs. New interfaces require a separate design and compatibility review.

### E1. Concurrent reads and LS1 sorting

Completed immutable Snapshots permit concurrent reads when the application protects their lifetime against destruction. Group pure reads require no concurrent destruction, merge/capture state access, or unsafe item reads. Merge/capture read/write busy/marker state; logical immutability does not allow arbitrary concurrency. Order/ManagedOrder access involving mutation/destruction requires caller synchronization. Reentry guards and marker reference counts are not container locks.

Canonical same-namespace LS1 keys have fixed 16-digit lowercase hexadecimal ordinals. Bytewise ASCII lexical order matches ordinal order. Database use of this property requires a byte-order-preserving collation; linguistic/case-insensitive order is not assumed. Cross-namespace key compare returns `DOMAIN_MISMATCH`; sorting those strings does not define business-record order. LS1 is a historical Snapshot coordinate, not persistent identity across versions, and differs from V3 LK1 Path encoding.

Group pure reads require protected Group lifetime and safe payload access. Synchronize Group accessors against merge, capture, destruction and Batch merge-all involving constituent Groups. Synchronize GroupBatch accessors against merge-all and destruction. Private non-atomic busy fields are read by Group size/item and Batch size/chunk accessors and written by merge and merge-all; capture accesses Group busy and marker state. Capture guards in snapshot.c and atomic SourceMarker references protect reentry/marker lifetime, not container locking. Immutable Snapshot concurrent reads still require lifetime protection throughout reading.

## F. Getting started and further reading

C/C++ consumers use only the public header and library target. Snapshot byte views last until destruction; callers synchronize Order reads/writes/destruction. A loaded Snapshot can be restored but lacks the original live marker.

Read in this order:

1. [`examples/live_order.c`](../examples/live_order.c) or [`examples/basic.c`](../examples/basic.c), using public APIs.
2. [`src/lks_order_internal.h`](../src/lks_order_internal.h): Handle, Block, Order and Cursor.
3. [`src/order.c`](../src/order.c): `v4_insert`, `v4_place`, `v4_move`, `v4_repair`, `v4_balance`.
4. [`src/managed_order.c`](../src/managed_order.c): comparator lookup using the same Order.
5. [`src/snapshot.c`](../src/snapshot.c), [`src/snapshot_wire.c`](../src/snapshot_wire.c), [`src/snapshot_restore.c`](../src/snapshot_restore.c): application identity, independent history and restoration.
6. [`src/immutable_group.c`](../src/immutable_group.c), [`src/sort.c`](../src/sort.c).
7. [Implementation internals](IMPLEMENTATION_INTERNALS.md): private functions, transformations and boundaries.
8. [Validation map](IMPLEMENTATION_VALIDATION.md): tests, evidence and reproducible commands.

This guide explains every public function's purpose, implementation entry point and principal constraints. Source inspection remains necessary for line-level behavior; the guide is not a formal proof of every caller interaction.
