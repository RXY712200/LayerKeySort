#ifndef LKS_BULK_INTERNAL_H
#define LKS_BULK_INTERNAL_H

#include "layerkeysort.h"

/* Input is already sorted. The returned Tree owns cloned Paths, not items. */
LksStatus lks_bulk_build_tree(void *const *sorted, size_t count,
    LksTree **out_tree);

#endif
