#include <string.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_sort_internal.h"

void lks_merge_sorted_pointers(void *const *left, size_t left_count,
    void *const *right, size_t right_count, const LksComparator *comparator,
    void **output)
{
    size_t l = 0, r = 0, out = 0;
    /* Taking the left item on equality preserves source order. */
    while (l < left_count && r < right_count) {
        if (comparator->compare(left[l], right[r], comparator->context) <= 0)
            output[out++] = left[l++];
        else
            output[out++] = right[r++];
    }
    while (l < left_count) output[out++] = left[l++];
    while (r < right_count) output[out++] = right[r++];
}

static void stable_sort_range(void **items, void **scratch, size_t begin,
    size_t end, const LksComparator *comparator)
{
    size_t middle;
    if (end - begin < 2) return;
    middle = begin + (end - begin) / 2;
    stable_sort_range(items, scratch, begin, middle, comparator);
    stable_sort_range(items, scratch, middle, end, comparator);
    lks_merge_sorted_pointers(items + begin, middle - begin,
        items + middle, end - middle, comparator, scratch + begin);
    memcpy(items + begin, scratch + begin, (end - begin) * sizeof(*items));
}

LksStatus lks_stable_sort_copy(void *const *items, size_t count,
    const LksComparator *comparator, void ***out_sorted)
{
    void **sorted, **scratch;
    if (out_sorted == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_sorted = NULL;
    if (comparator == NULL || comparator->compare == NULL ||
        (count != 0 && items == NULL)) return LKS_STATUS_INVALID_ARGUMENT;
    if (count == 0) return LKS_STATUS_OK;
    if (count > (size_t)-1 / sizeof(*sorted)) return LKS_STATUS_OUT_OF_MEMORY;
    sorted = (void **)lks_alloc_tagged(count * sizeof(*sorted), LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (sorted == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    scratch = (void **)lks_alloc_tagged(count * sizeof(*scratch), LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (scratch == NULL) { lks_free(sorted); return LKS_STATUS_OUT_OF_MEMORY; }
    memcpy(sorted, items, count * sizeof(*sorted));
    stable_sort_range(sorted, scratch, 0, count, comparator);
    lks_free(scratch);
    *out_sorted = sorted;
    return LKS_STATUS_OK;
}

LksStatus lks_sort(void **items, size_t count, LksCompareFn compare, void *context)
{
    LksComparator comparator;
    void **sorted = NULL;
    LksStatus status;
    if (compare == NULL || (count != 0 && items == NULL))
        return LKS_STATUS_INVALID_ARGUMENT;
    comparator.compare = compare;
    comparator.context = context;
    status = lks_stable_sort_copy(items, count, &comparator, &sorted);
    if (status != LKS_STATUS_OK) return status;
    /* Commit only after all allocations and comparisons have completed. */
    if (count != 0) memcpy(items, sorted, count * sizeof(*items));
    lks_free(sorted);
    return LKS_STATUS_OK;
}
