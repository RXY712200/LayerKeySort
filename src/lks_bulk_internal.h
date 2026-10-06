#ifndef LKS_BULK_INTERNAL_H
#define LKS_BULK_INTERNAL_H

#include "lks_legacy_internal.h"

/* Input is already sorted. The returned Tree owns Paths, not items. */
LksStatus lks_bulk_build_tree(void *const *sorted, size_t count,
    LksTree **out_tree);
/* Fill COUNT initially NULL entries with ordered, sparse owned Paths.
 * On failure caller destroys every non-NULL entry. */
LksStatus lks_bulk_generate_paths(LksPath **paths, size_t count);

#endif
