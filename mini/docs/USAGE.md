# Using Mini v1.0.0-preview.3

Include `layerkeysort_mini.h` and link `layerkeysort_mini`. The complete runnable
[basic example](../examples/basic.c) uses only the public header. Build commands
are in the [development guide](DEVELOPMENT.md).

## Insert an explicit order

```c
LksMiniOrder *order = NULL;
LksMiniHandle *a = NULL, *b = NULL, *c = NULL;
int values[] = {10, 20, 30};
if (lks_mini_create(&order) != LKS_MINI_OK) return 1;
if (lks_mini_insert_front(order, &values[0], &a) != LKS_MINI_OK ||
    lks_mini_insert_after(order, a, &values[2], &c) != LKS_MINI_OK ||
    lks_mini_insert_before(order, c, &values[1], &b) != LKS_MINI_OK) {
    lks_mini_destroy(order);
    return 1;
}
/* Order is a, b, c. insert_back appends independently of an anchor. */
```

These fragments belong inside a function returning int. Check each status.
NULL anchors are errors; use front/back for an empty order. Inserting the same
`&values[0]` again creates a separate handle and occurrence. Passing NULL as
the payload is valid; use `lks_mini_item`'s status to distinguish error from NULL.

## Traverse and remove

```c
LksMiniHandle *cursor = NULL;
if (lks_mini_first(order, &cursor) != LKS_MINI_OK) {
    lks_mini_destroy(order);
    return 1;
}
while (cursor) {
    LksMiniHandle *next = NULL;
    void *item = NULL;
    if (lks_mini_item(order, cursor, &item) != LKS_MINI_OK ||
        lks_mini_next(order, cursor, &next) != LKS_MINI_OK ||
        lks_mini_remove(order, cursor) != LKS_MINI_OK) {
        lks_mini_destroy(order);
        return 1;
    }
    /* Process item if needed; Mini has not freed it. */
    (void)item;
    cursor = next;
}
lks_mini_destroy(order);
```

Save the next handle before removing the current one. Never query a removed
handle. Reverse traversal starts with `lks_mini_last` and uses `lks_mini_prev`.
Removing one occurrence does not invalidate other live handles. Destroying an
order invalidates every remaining handle. Clear application-held handle variables
as appropriate; Mini cannot clear all copies.

## Own your payloads

Stack objects must outlive any application use through their handles. For heap
objects, the application frees each business object exactly once after its last
use. Duplicate payload pointers mean multiple occurrences can refer to the same
object; blindly freeing an object for every removed occurrence is incorrect.
Mini only stores pointers, so it cannot enforce business-object lifetime.

Use separate local output variables when preserving an earlier value matters:
failed operations reset outputs. Valid foreign-order handles produce WRONG_ORDER;
freed handles are unsupported. See the [contract](CONTRACT.md) for complete rules.

## Move existing occurrences and compare

After creating the a, b, c sequence above, before its traversal/removal:

```c
int relation = 0;
if (lks_mini_move_front(order, c) != LKS_MINI_OK ||
    lks_mini_move_after(order, a, b) != LKS_MINI_OK ||
    lks_mini_compare(order, c, a, &relation) != LKS_MINI_OK) {
    lks_mini_destroy(order);
    return 1;
}
/* Sequence: c, b, a; relation is exactly -1. All handles/items are unchanged. */
if (lks_mini_move_before(order, a, b) != LKS_MINI_OK ||
    lks_mini_move_back(order, c) != LKS_MINI_OK) {
    lks_mini_destroy(order);
    return 1;
}
/* Sequence: a, b, c. Size is unchanged. */
```

Known-handle moves are O(1); arbitrary comparison is O(n) worst case. Neither
allocates/frees memory. Comparison yields -1 before, 0 for the identical live
occurrence, and +1 after, even when payload pointers are duplicates or NULL.
Moving head to front, tail to back, before/after self, before the immediate next
neighbor or after the immediate previous neighbor returns OK without link writes.
Both handles must belong to the specified order, even for self-movement.
NULL required arguments are INVALID_ARGUMENT and valid foreign handles/anchors
are WRONG_ORDER. Errors preserve order/size/handles and comparison output is 0.

Preview.3 preserves the 18 APIs and these usage patterns. The independent model
and failure tests described in [Development](DEVELOPMENT.md) are test-only; no
allocator settings or diagnostic APIs are needed by applications.
