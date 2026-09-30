#ifndef LKS_ALLOC_INTERNAL_H
#define LKS_ALLOC_INTERNAL_H

#include <stddef.h>

typedef enum LksAllocTag {
    LKS_ALLOC_TAG_OTHER = 0,
    LKS_ALLOC_TAG_PATH_OBJECT,
    LKS_ALLOC_TAG_PATH_STEPS,
    LKS_ALLOC_TAG_TREE_OBJECT,
    LKS_ALLOC_TAG_TREE_NODE,
    LKS_ALLOC_TAG_TREE_CHILDREN, /* historical ChildBlock diagnostics only */
    LKS_ALLOC_TAG_GROUP_OBJECT,
    LKS_ALLOC_TAG_GROUP_ORDERED,
    LKS_ALLOC_TAG_BATCH_OBJECT,
    LKS_ALLOC_TAG_BATCH_GROUP_ARRAY,
    LKS_ALLOC_TAG_MERGE_SCRATCH,
    LKS_ALLOC_TAG_COUNT
} LksAllocTag;

typedef struct LksAllocTagStats {
    size_t live_bytes;
    size_t peak_live_bytes;
    size_t live_blocks;
    size_t peak_live_blocks;
    size_t alloc_calls;
    size_t realloc_calls;
    size_t free_calls;
} LksAllocTagStats;

typedef struct LksAllocStats {
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
    LksAllocTagStats tags[LKS_ALLOC_TAG_COUNT];
    size_t bytes_at_global_peak[LKS_ALLOC_TAG_COUNT];
    size_t blocks_at_global_peak[LKS_ALLOC_TAG_COUNT];
    size_t blocks_when_global_byte_peak;
} LksAllocStats;

void *lks_alloc(size_t size);
void *lks_alloc_tagged(size_t size, LksAllocTag tag);
void *lks_realloc(void *ptr, size_t size);
void lks_free(void *ptr);

/* Returns 0 on success, nonzero while LayerKeySort allocations remain live. */
int lks_alloc_stats_reset(void);
LksAllocStats lks_alloc_stats_get(void);

/* Private deterministic fault-injection controls for tests only. */
void lks_alloc_test_fail_on_attempt(size_t attempt_index);
void lks_alloc_test_disable_failure(void);
void lks_alloc_test_reset_attempt_counter(void);
size_t lks_alloc_test_get_attempt_count(void);
int lks_alloc_test_failure_triggered(void);

#endif /* LKS_ALLOC_INTERNAL_H */
