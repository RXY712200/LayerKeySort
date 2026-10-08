# Preview.1 public contract

Version: **v1.0.0-preview.1**. The authoritative declarations are in
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
order/handle/item outputs to NULL and size to 0. Output storage must be writable
and correctly typed; it must not alias private Mini state. Ordinary caller-local
in-place traversal (`lks_mini_next(order, cursor, &cursor)`) is supported.
Outputs are reset on failure, so applications should not pass their sole saved
order/handle variable as an output unless losing its previous value is intended.

Detectable invalid arguments, wrong ownership, capacity failure and allocation
failure do not mutate any order. Failed insertions return a NULL output handle.
No allocation occurs on removal or destroy. No error is promised for unsupported
invalid pointer use.

## Complexity and concurrency

Creation, queries, insertion and removal take O(1) time, excluding allocator cost.
Destroy takes O(n); storage is O(n). There are no persistence, business IDs,
automatic sorting, random access, snapshot formats or built-in thread locks.
Applications must synchronize shared order access, especially with mutation or
destruction. Independently owned orders do not share library mutable state.

Only these 13 functions are available. The planned stable API has 18; four move
functions and comparison are reserved for Preview.2 and are not declared here.
