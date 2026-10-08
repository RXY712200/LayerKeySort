#ifndef LAYERKEYSORT_MINI_H
#define LAYERKEYSORT_MINI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* v1.0.0-preview.3. Opaque live orders and occurrence handles. */
typedef struct LksMiniOrder LksMiniOrder;
typedef struct LksMiniHandle LksMiniHandle;
typedef enum LksMiniStatus {
    LKS_MINI_OK = 0,
    LKS_MINI_INVALID_ARGUMENT,
    LKS_MINI_WRONG_ORDER,
    LKS_MINI_OUT_OF_MEMORY,
    LKS_MINI_CAPACITY_LIMIT
} LksMiniStatus;

/* Outputs default to NULL (size to 0) on normal errors.
 * Items belong to the caller; NULL and duplicate items are allowed.
 * Only valid live handles may be passed. Removal/destroy invalidates them.
 * Calls on the same order require caller-provided synchronization. */
LksMiniStatus lks_mini_create(LksMiniOrder **out_order);
void lks_mini_destroy(LksMiniOrder *order);
LksMiniStatus lks_mini_size(const LksMiniOrder *order, size_t *out_size);
LksMiniStatus lks_mini_first(const LksMiniOrder *order, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_last(const LksMiniOrder *order, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_next(const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_prev(const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_item(const LksMiniOrder *order, const LksMiniHandle *handle, void **out_item);
LksMiniStatus lks_mini_insert_front(LksMiniOrder *order, void *item, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_insert_back(LksMiniOrder *order, void *item, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_insert_before(LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_insert_after(LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle);
LksMiniStatus lks_mini_remove(LksMiniOrder *order, LksMiniHandle *handle);
/* Moves preserve occurrence identity and allocate/free nothing. */
LksMiniStatus lks_mini_move_front(LksMiniOrder *order, LksMiniHandle *handle);
LksMiniStatus lks_mini_move_back(LksMiniOrder *order, LksMiniHandle *handle);
LksMiniStatus lks_mini_move_before(LksMiniOrder *order, LksMiniHandle *handle, const LksMiniHandle *anchor);
LksMiniStatus lks_mini_move_after(LksMiniOrder *order, LksMiniHandle *handle, const LksMiniHandle *anchor);
/* O(n): -1 before, 0 same occurrence, +1 after. Error output defaults to 0. */
LksMiniStatus lks_mini_compare(const LksMiniOrder *order, const LksMiniHandle *a, const LksMiniHandle *b, int *out_result);

#ifdef __cplusplus
}
#endif
#endif
