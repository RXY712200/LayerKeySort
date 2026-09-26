#ifndef HPS_ALLOC_INTERNAL_H
#define HPS_ALLOC_INTERNAL_H

#include <stddef.h>

typedef enum HpsAllocTag {
    HPS_ALLOC_TAG_OTHER = 0,
    HPS_ALLOC_TAG_PATH_OBJECT,
    HPS_ALLOC_TAG_PATH_STEPS,
    HPS_ALLOC_TAG_TREE_OBJECT,
    HPS_ALLOC_TAG_TREE_NODE,
    HPS_ALLOC_TAG_TREE_CHILDREN,
    HPS_ALLOC_TAG_GROUP_OBJECT,
    HPS_ALLOC_TAG_GROUP_ORDERED,
    HPS_ALLOC_TAG_BATCH_OBJECT,
    HPS_ALLOC_TAG_BATCH_GROUP_ARRAY,
    HPS_ALLOC_TAG_MERGE_SCRATCH,
    HPS_ALLOC_TAG_COUNT
} HpsAllocTag;

typedef struct HpsAllocTagStats {
    size_t live_bytes;
    size_t peak_live_bytes;
    size_t live_blocks;
    size_t peak_live_blocks;
    size_t alloc_calls;
    size_t realloc_calls;
    size_t free_calls;
} HpsAllocTagStats;

typedef struct HpsAllocStats {
    size_t live_bytes;
    size_t peak_live_bytes;
    size_t live_blocks;
    /* Blocks at peak_live_bytes; max_live_blocks preserves the 8.1 high-water mark. */
    size_t peak_live_blocks;
    size_t max_live_blocks;
    size_t alloc_calls;
    size_t realloc_calls;
    size_t free_calls;
    size_t failed_calls;
    size_t total_successful_requested_bytes;
    int counter_overflowed;
    HpsAllocTagStats tags[HPS_ALLOC_TAG_COUNT];
    size_t bytes_at_global_peak[HPS_ALLOC_TAG_COUNT];
    size_t blocks_at_global_peak[HPS_ALLOC_TAG_COUNT];
    size_t blocks_when_global_byte_peak;
} HpsAllocStats;

void *hps_alloc(size_t size);
void *hps_alloc_tagged(size_t size, HpsAllocTag tag);
void *hps_realloc(void *ptr, size_t size);
void hps_free(void *ptr);

/* Returns 0 on success, nonzero while HPSort allocations remain live. */
int hps_alloc_stats_reset(void);
HpsAllocStats hps_alloc_stats_get(void);

/* Private deterministic fault-injection controls for tests only. */
void hps_alloc_test_fail_on_attempt(size_t attempt_index);
void hps_alloc_test_disable_failure(void);
void hps_alloc_test_reset_attempt_counter(void);
size_t hps_alloc_test_get_attempt_count(void);
int hps_alloc_test_failure_triggered(void);

#endif /* HPS_ALLOC_INTERNAL_H */
