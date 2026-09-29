#ifndef LKS_SORT_INTERNAL_H
#define LKS_SORT_INTERNAL_H

#include "layerkeysort.h"

LksStatus lks_stable_sort_copy(void *const *items, size_t count,
    const LksComparator *comparator, void ***out_sorted);
void lks_merge_sorted_pointers(void *const *left, size_t left_count,
    void *const *right, size_t right_count, const LksComparator *comparator,
    void **output);

#endif
