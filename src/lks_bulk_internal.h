#ifndef LKS_BULK_INTERNAL_H
#define LKS_BULK_INTERNAL_H

#include "layerkeysort.h"

/* Input is already sorted. The returned Tree owns cloned Paths, not items. */
LksStatus lks_bulk_build_tree(void *const *sorted, size_t count,
    LksTree **out_tree);

/* Build a complete sparse branch rooted at anchor_path. Temporary prefix
 * ancestors have NULL items; the caller detaches the anchor before commit. */
LksStatus lks_bulk_build_branch(const LksPath *anchor_path,
    void *const *sorted, size_t count, LksTree **out_tree);

#endif
