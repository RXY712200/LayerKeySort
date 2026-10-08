# Preview.3 public contract

Version: **v1.0.0-preview.3**. The authoritative declarations are in
[`layerkeysort_mini.h`](../include/layerkeysort_mini.h). All required standard
types are included by that header. It supports C and C++ linkage; implementation
is C17. `LksMiniOrder` and `LksMiniHandle` are opaque struct types.

## Occurrences and ownership

Each successful insertion creates an independently allocated occurrence.
A handle identifies an occurrence, not a payload or a position number. NULL
payloads and duplicate payload pointers are supported. Removing one duplicate
does not remove the others. Mini never dereferences, frees or otherwise inspects
payloads. Applications own all referenced objects and must manage their lifetime.

Mini owns orders and occurrence nodes. A live handle remains stable across
insertions and removal of other occurrences. Removing its occurrence or destroying
its order invalidates it. Dangling, freed, forged and otherwise invalid pointers
are outside the calling contract. There is no promise of detecting them; passing
them may cause undefined behavior. A valid handle from another live order is
rejected with `LKS_MINI_WRONG_ORDER` before mutation.

## Implemented functions

Every function except destroy returns `LksMiniStatus`.

| Function | Arguments | Successful behavior |
| --- | --- | --- |
| `lks_mini_create` | `LksMiniOrder **out_order` | Allocate an empty order |
| `lks_mini_destroy` | `LksMiniOrder *order` | Return void; release all nodes and order; NULL is a no-op |
| `lks_mini_size` | `const LksMiniOrder *order, size_t *out_size` | Write live occurrence count |
| `lks_mini_first` | `const LksMiniOrder *order, LksMiniHandle **out_handle` | Write first handle, NULL if empty |
| `lks_mini_last` | `const LksMiniOrder *order, LksMiniHandle **out_handle` | Write last handle, NULL if empty |
| `lks_mini_next` | `const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle` | Write next handle, NULL at last |
| `lks_mini_prev` | `const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle` | Write previous handle, NULL at first |
| `lks_mini_item` | `const LksMiniOrder *order, const LksMiniHandle *handle, void **out_item` | Write payload, including NULL |
| `lks_mini_insert_front` | `LksMiniOrder *order, void *item, LksMiniHandle **out_handle` | Insert at head, return fresh handle |
| `lks_mini_insert_back` | `LksMiniOrder *order, void *item, LksMiniHandle **out_handle` | Insert at tail, return fresh handle |
| `lks_mini_insert_before` | `LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle` | Insert immediately before live anchor |
| `lks_mini_insert_after` | `LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle` | Insert immediately after live anchor |
| `lks_mini_remove` | `LksMiniOrder *order, LksMiniHandle *handle` | Unlink and free that occurrence |
| `lks_mini_move_front` | `LksMiniOrder *order, LksMiniHandle *handle` | Move existing occurrence to head |
| `lks_mini_move_back` | `LksMiniOrder *order, LksMiniHandle *handle` | Move existing occurrence to tail |
| `lks_mini_move_before` | `LksMiniOrder *order, LksMiniHandle *handle, const LksMiniHandle *anchor` | Move immediately before anchor |
| `lks_mini_move_after` | `LksMiniOrder *order, LksMiniHandle *handle, const LksMiniHandle *anchor` | Move immediately after anchor |
| `lks_mini_compare` | `const LksMiniOrder *order, const LksMiniHandle *a, const LksMiniHandle *b, int *out_result` | Write -1 before, 0 same occurrence, +1 after |

Empty/end queries return OK with NULL; a NULL payload query also returns OK
with NULL. The status distinguishes these successful results from errors.
Anchors must be valid live handles; NULL has no endpoint meaning.

## Errors and outputs

| Status | Meaning |
| --- | --- |
| `LKS_MINI_OK` | Operation succeeded |
| `LKS_MINI_INVALID_ARGUMENT` | Required order, handle, anchor or output pointer is NULL |
| `LKS_MINI_WRONG_ORDER` | A valid live handle belongs to another live order |
| `LKS_MINI_OUT_OF_MEMORY` | Order or node allocation failed |
| `LKS_MINI_CAPACITY_LIMIT` | An insertion would exceed SIZE_MAX occurrences |

Required NULL arguments are checked before ownership; ownership is checked before
capacity/allocation. Non-NULL outputs are initialized before normal errors:
order/handle/item outputs to NULL, size to 0 and comparison result to 0. Output storage must be writable
and correctly typed; it must not alias private Mini state. Ordinary caller-local
in-place traversal (`lks_mini_next(order, cursor, &cursor)`) is supported.
Outputs are reset on failure, so applications should not pass their sole saved
order/handle variable as an output unless losing its previous value is intended.

Detectable invalid arguments, wrong ownership, capacity failure and allocation
failure do not mutate any order. Failed insertions return a NULL output handle.
No allocation occurs on removal or destroy. No error is promised for unsupported
invalid pointer use.

## Complexity and concurrency

Creation, basic queries, insertion, removal and known-handle movement take O(1)
time, excluding allocator cost. Arbitrary order comparison takes O(n) worst case.
Destroy takes O(n); storage is O(n). There are no persistence, business IDs,
automatic sorting, random access, snapshot formats or built-in thread locks.
Applications must synchronize shared order access, especially with mutation or
destruction. Independently owned orders do not share library mutable state.

All 18 planned public functions remain implemented with unchanged signatures,
statuses and semantics. Preview.3 adds verification, not functionality. RC.1
integration and Stable validation remain separate milestones.

## Movement and comparison guarantees

Movement preserves node address, handle identity, owner, payload and size. It
cannot transfer nodes between orders and never allocates or frees memory.
All required arguments and both owners are validated before deciding on a no-op.
The following return OK without modifying links: front on the head, back on the
tail, before/after self, before the immediate next neighbor and after the
immediate previous neighbor. These rules also cover singleton/two-node orders.
A foreign self-handle remains WRONG_ORDER. NULL anchors are INVALID_ARGUMENT.

Comparison uses current occurrence order, not item values or pointer-address
ordering. Distinct occurrences with identical or NULL payloads compare by their
positions. Identical valid handles return 0. Neither comparison nor movement
allocates or frees anything. Comparison never mutates state. For missing required
arguments return INVALID_ARGUMENT; for any valid foreign operand/anchor return
WRONG_ORDER. Multiple foreign handles also return WRONG_ORDER. Missing required
arguments take precedence. Every detectable error leaves sequence, handles and
size unchanged; non-NULL comparison output is reset to zero.

The reliability evidence and its platform limits are recorded in the
[development guide](DEVELOPMENT.md). Tests never call through freed handles and
do not extend this contract to stale-pointer detection.
