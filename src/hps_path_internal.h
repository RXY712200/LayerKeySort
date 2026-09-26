#ifndef HPS_PATH_INTERNAL_H
#define HPS_PATH_INTERNAL_H

#include <stddef.h>
#include "hps.h"

/* Private read-only Path storage inspection for tests and benchmarks. */
size_t hps_path_internal_capacity(const HpsPath *path);
size_t hps_path_internal_sizeof_path(void);
size_t hps_path_internal_alignof_path(void);
size_t hps_path_internal_storage_bytes(const HpsPath *path);
size_t hps_path_internal_storage_bytes_for_capacity(size_t capacity);
size_t hps_path_internal_levels_offset_for_capacity(size_t capacity);

#endif /* HPS_PATH_INTERNAL_H */
