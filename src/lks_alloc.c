#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "lks_alloc_internal.h"

#if defined(_MSC_VER)
/* MSVC's C17 headers do not expose max_align_t; use fundamental C types. */
typedef union LksMaxAlignmentFallback {
    long double long_double_value;
    long long long_long_value;
    void *pointer_value;
} LksMaxAlignmentFallback;
#define LKS_ALLOC_MAX_ALIGNMENT LksMaxAlignmentFallback
#else
#define LKS_ALLOC_MAX_ALIGNMENT max_align_t
#endif

typedef union LksAllocHeader {
    struct {
        size_t requested_size;
        LksAllocTag tag;
    } data;
    LKS_ALLOC_MAX_ALIGNMENT alignment;
} LksAllocHeader;

static LksAllocStats lks_alloc_stats;
static size_t lks_alloc_test_attempt_count;
static size_t lks_alloc_test_fail_index;
static int lks_alloc_test_triggered;

void lks_alloc_test_fail_on_attempt(size_t attempt_index)
{
    if (attempt_index == 0) {
        lks_alloc_test_disable_failure();
        return;
    }
    lks_alloc_test_attempt_count = 0;
    lks_alloc_test_fail_index = attempt_index;
    lks_alloc_test_triggered = 0;
}

void lks_alloc_test_disable_failure(void)
{
    lks_alloc_test_attempt_count = 0;
    lks_alloc_test_fail_index = 0;
    lks_alloc_test_triggered = 0;
}

void lks_alloc_test_reset_attempt_counter(void)
{
    lks_alloc_test_attempt_count = 0;
}

size_t lks_alloc_test_get_attempt_count(void)
{
    return lks_alloc_test_attempt_count;
}

int lks_alloc_test_failure_triggered(void)
{
    return lks_alloc_test_triggered;
}

static int lks_alloc_test_should_fail(void)
{
    if (lks_alloc_test_attempt_count != (size_t)-1)
        ++lks_alloc_test_attempt_count;
    if (lks_alloc_test_fail_index == 0 || lks_alloc_test_triggered)
        return 0;
    if (lks_alloc_test_attempt_count == lks_alloc_test_fail_index) {
        lks_alloc_test_triggered = 1;
        return 1;
    }
    return 0;
}

static LksAllocTag lks_alloc_normalize_tag(LksAllocTag tag)
{
    return tag >= LKS_ALLOC_TAG_OTHER && tag < LKS_ALLOC_TAG_COUNT ?
        tag : LKS_ALLOC_TAG_OTHER;
}

static void lks_alloc_counter_add(size_t *counter, size_t amount)
{
    if (*counter > (size_t)-1 - amount) {
        *counter = (size_t)-1;
        lks_alloc_stats.counter_overflowed = 1;
    } else {
        *counter += amount;
    }
}

static void lks_alloc_failed(LksAllocTag tag)
{
    lks_alloc_counter_add(&lks_alloc_stats.failed_calls, 1);
    (void)tag;
}

static void lks_alloc_capture_global_peak(void)
{
    size_t tag;
    if (lks_alloc_stats.live_bytes <= lks_alloc_stats.peak_live_bytes) return;
    lks_alloc_stats.peak_live_bytes = lks_alloc_stats.live_bytes;
    lks_alloc_stats.peak_live_blocks = lks_alloc_stats.live_blocks;
    lks_alloc_stats.blocks_when_global_byte_peak = lks_alloc_stats.live_blocks;
    for (tag = 0; tag < LKS_ALLOC_TAG_COUNT; ++tag) {
        lks_alloc_stats.bytes_at_global_peak[tag] =
            lks_alloc_stats.tags[tag].live_bytes;
        lks_alloc_stats.blocks_at_global_peak[tag] =
            lks_alloc_stats.tags[tag].live_blocks;
    }
}

static void lks_alloc_live_add(LksAllocTag tag, size_t bytes, size_t blocks)
{
    LksAllocTagStats *tag_stats = &lks_alloc_stats.tags[tag];
    lks_alloc_counter_add(&lks_alloc_stats.live_bytes, bytes);
    lks_alloc_counter_add(&lks_alloc_stats.live_blocks, blocks);
    lks_alloc_counter_add(&tag_stats->live_bytes, bytes);
    lks_alloc_counter_add(&tag_stats->live_blocks, blocks);
    if (tag_stats->live_bytes > tag_stats->peak_live_bytes)
        tag_stats->peak_live_bytes = tag_stats->live_bytes;
    if (tag_stats->live_blocks > tag_stats->peak_live_blocks)
        tag_stats->peak_live_blocks = tag_stats->live_blocks;
    if (lks_alloc_stats.live_blocks > lks_alloc_stats.max_live_blocks)
        lks_alloc_stats.max_live_blocks = lks_alloc_stats.live_blocks;
    lks_alloc_capture_global_peak();
}

static void lks_alloc_live_resize(LksAllocTag tag, size_t old_size,
    size_t new_size)
{
    LksAllocTagStats *tag_stats = &lks_alloc_stats.tags[tag];
    if (new_size > old_size) {
        size_t increase = new_size - old_size;
        lks_alloc_counter_add(&lks_alloc_stats.live_bytes, increase);
        lks_alloc_counter_add(&tag_stats->live_bytes, increase);
        if (tag_stats->live_bytes > tag_stats->peak_live_bytes)
            tag_stats->peak_live_bytes = tag_stats->live_bytes;
        lks_alloc_capture_global_peak();
    } else {
        size_t decrease = old_size - new_size;
        if (lks_alloc_stats.live_bytes >= decrease)
            lks_alloc_stats.live_bytes -= decrease;
        else {
            lks_alloc_stats.live_bytes = 0;
            lks_alloc_stats.counter_overflowed = 1;
        }
        if (tag_stats->live_bytes >= decrease)
            tag_stats->live_bytes -= decrease;
        else {
            tag_stats->live_bytes = 0;
            lks_alloc_stats.counter_overflowed = 1;
        }
    }
}

static void *lks_alloc_new(size_t size, int count_alloc_call, LksAllocTag tag,
    int count_attempt)
{
    LksAllocHeader *header;
    size_t total_size;
    LksAllocTagStats *tag_stats;

    tag = lks_alloc_normalize_tag(tag);
    tag_stats = &lks_alloc_stats.tags[tag];
    if (count_alloc_call) {
        lks_alloc_counter_add(&lks_alloc_stats.alloc_calls, 1);
        lks_alloc_counter_add(&tag_stats->alloc_calls, 1);
    }
    if (count_attempt && lks_alloc_test_should_fail()) {
        lks_alloc_failed(tag);
        return NULL;
    }
    if (size > (size_t)-1 - sizeof(*header)) {
        lks_alloc_failed(tag);
        return NULL;
    }
    total_size = sizeof(*header) + size;
    header = (LksAllocHeader *)malloc(total_size);
    if (header == NULL) {
        lks_alloc_failed(tag);
        return NULL;
    }
    header->data.requested_size = size;
    header->data.tag = tag;
    lks_alloc_live_add(tag, size, 1);
    lks_alloc_counter_add(&lks_alloc_stats.total_successful_requested_bytes,
        size);
    return (void *)(header + 1);
}

static void lks_alloc_release(void *ptr, int count_free_call)
{
    LksAllocHeader *header;
    LksAllocTag tag;
    LksAllocTagStats *tag_stats;
    size_t old_size;

    if (ptr == NULL) return;
    header = ((LksAllocHeader *)ptr) - 1;
    old_size = header->data.requested_size;
    tag = lks_alloc_normalize_tag(header->data.tag);
    tag_stats = &lks_alloc_stats.tags[tag];
    if (lks_alloc_stats.live_bytes >= old_size)
        lks_alloc_stats.live_bytes -= old_size;
    else {
        lks_alloc_stats.live_bytes = 0;
        lks_alloc_stats.counter_overflowed = 1;
    }
    if (tag_stats->live_bytes >= old_size)
        tag_stats->live_bytes -= old_size;
    else {
        tag_stats->live_bytes = 0;
        lks_alloc_stats.counter_overflowed = 1;
    }
    if (lks_alloc_stats.live_blocks > 0) --lks_alloc_stats.live_blocks;
    else lks_alloc_stats.counter_overflowed = 1;
    if (tag_stats->live_blocks > 0) --tag_stats->live_blocks;
    else lks_alloc_stats.counter_overflowed = 1;
    if (count_free_call) {
        lks_alloc_counter_add(&lks_alloc_stats.free_calls, 1);
        lks_alloc_counter_add(&tag_stats->free_calls, 1);
    }
    free(header);
}

void *lks_alloc(size_t size)
{
    return lks_alloc_tagged(size, LKS_ALLOC_TAG_OTHER);
}

void *lks_alloc_tagged(size_t size, LksAllocTag tag)
{
    return lks_alloc_new(size, 1, lks_alloc_normalize_tag(tag), 1);
}

void *lks_realloc(void *ptr, size_t size)
{
    LksAllocHeader *old_header;
    LksAllocHeader *new_header;
    LksAllocTag tag;
    LksAllocTagStats *tag_stats;
    size_t old_size;
    size_t new_total_size;

    if (ptr == NULL) {
        lks_alloc_counter_add(&lks_alloc_stats.realloc_calls, 1);
        lks_alloc_counter_add(&lks_alloc_stats.tags[LKS_ALLOC_TAG_OTHER].realloc_calls, 1);
        return lks_alloc_new(size, 0, LKS_ALLOC_TAG_OTHER, size > 0);
    }
    old_header = ((LksAllocHeader *)ptr) - 1;
    tag = lks_alloc_normalize_tag(old_header->data.tag);
    tag_stats = &lks_alloc_stats.tags[tag];
    lks_alloc_counter_add(&lks_alloc_stats.realloc_calls, 1);
    lks_alloc_counter_add(&tag_stats->realloc_calls, 1);
    if (size == 0) {
        lks_alloc_release(ptr, 0);
        return NULL;
    }
    if (lks_alloc_test_should_fail()) {
        lks_alloc_failed(tag);
        return NULL;
    }
    if (size > (size_t)-1 - sizeof(*old_header)) {
        lks_alloc_failed(tag);
        return NULL;
    }
    old_size = old_header->data.requested_size;
    new_total_size = sizeof(*old_header) + size;
    new_header = (LksAllocHeader *)realloc(old_header, new_total_size);
    if (new_header == NULL) {
        lks_alloc_failed(tag);
        return NULL;
    }
    new_header->data.requested_size = size;
    new_header->data.tag = tag;
    lks_alloc_live_resize(tag, old_size, size);
    lks_alloc_counter_add(&lks_alloc_stats.total_successful_requested_bytes,
        size);
    return (void *)(new_header + 1);
}

void lks_free(void *ptr)
{
    lks_alloc_release(ptr, 1);
}

int lks_alloc_stats_reset(void)
{
    if (lks_alloc_stats.live_blocks != 0 || lks_alloc_stats.live_bytes != 0)
        return 1;
    lks_alloc_stats = (LksAllocStats){ 0 };
    return 0;
}

LksAllocStats lks_alloc_stats_get(void)
{
    return lks_alloc_stats;
}
