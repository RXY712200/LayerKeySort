#ifndef LKS_PATH_INTERNAL_H
#define LKS_PATH_INTERNAL_H

#include <stddef.h>
#include "lks_legacy_internal.h"

/* Private read-only Path storage inspection for tests and benchmarks. */
size_t lks_path_internal_capacity(const LksPath *path);
size_t lks_path_internal_sizeof_path(void);
size_t lks_path_internal_alignof_path(void);
size_t lks_path_internal_storage_bytes(const LksPath *path);
size_t lks_path_internal_storage_bytes_for_capacity(size_t capacity);
size_t lks_path_internal_levels_offset_for_capacity(size_t capacity);

#endif /* LKS_PATH_INTERNAL_H */
