#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "hps_alloc_internal.h"

#if defined(_MSC_VER)
/* MSVC's C17 headers do not expose max_align_t; use fundamental C types. */
typedef union HpsMaxAlignmentFallback {
    long double long_double_value;
    long long long_long_value;
    void *pointer_value;
} HpsMaxAlignmentFallback;
#define HPS_ALLOC_MAX_ALIGNMENT HpsMaxAlignmentFallback
#else
#define HPS_ALLOC_MAX_ALIGNMENT max_align_t
#endif

typedef union HpsAllocHeader {
    struct {
        size_t requested_size;
        HpsAllocTag tag;
    } data;
    HPS_ALLOC_MAX_ALIGNMENT alignment;
} HpsAllocHeader;

static HpsAllocStats hps_alloc_stats;
static size_t hps_alloc_test_attempt_count;
static size_t hps_alloc_test_fail_index;
static int hps_alloc_test_triggered;

void hps_alloc_test_fail_on_attempt(size_t attempt_index)
{
    if (attempt_index == 0) {
        hps_alloc_test_disable_failure();
        return;
    }
    hps_alloc_test_attempt_count = 0;
    hps_alloc_test_fail_index = attempt_index;
    hps_alloc_test_triggered = 0;
}

void hps_alloc_test_disable_failure(void)
{
    hps_alloc_test_attempt_count = 0;
    hps_alloc_test_fail_index = 0;
    hps_alloc_test_triggered = 0;
}

void hps_alloc_test_reset_attempt_counter(void)
{
    hps_alloc_test_attempt_count = 0;
}

size_t hps_alloc_test_get_attempt_count(void)
{
    return hps_alloc_test_attempt_count;
}

int hps_alloc_test_failure_triggered(void)
{
    return hps_alloc_test_triggered;
}

static int hps_alloc_test_should_fail(void)
{
    if (hps_alloc_test_attempt_count != (size_t)-1)
        ++hps_alloc_test_attempt_count;
    if (hps_alloc_test_fail_index == 0 || hps_alloc_test_triggered)
        return 0;
    if (hps_alloc_test_attempt_count == hps_alloc_test_fail_index) {
        hps_alloc_test_triggered = 1;
        return 1;
    }
    return 0;
}

static HpsAllocTag hps_alloc_normalize_tag(HpsAllocTag tag)
{
    return tag >= HPS_ALLOC_TAG_OTHER && tag < HPS_ALLOC_TAG_COUNT ?
        tag : HPS_ALLOC_TAG_OTHER;
}

static void hps_alloc_counter_add(size_t *counter, size_t amount)
{
    if (*counter > (size_t)-1 - amount) {
        *counter = (size_t)-1;
        hps_alloc_stats.counter_overflowed = 1;
    } else {
        *counter += amount;
    }
}

static void hps_alloc_failed(HpsAllocTag tag)
{
    hps_alloc_counter_add(&hps_alloc_stats.failed_calls, 1);
    (void)tag;
}

static void hps_alloc_capture_global_peak(void)
{
    size_t tag;
    if (hps_alloc_stats.live_bytes <= hps_alloc_stats.peak_live_bytes) return;
    hps_alloc_stats.peak_live_bytes = hps_alloc_stats.live_bytes;
    hps_alloc_stats.peak_live_blocks = hps_alloc_stats.live_blocks;
    hps_alloc_stats.blocks_when_global_byte_peak = hps_alloc_stats.live_blocks;
    for (tag = 0; tag < HPS_ALLOC_TAG_COUNT; ++tag) {
        hps_alloc_stats.bytes_at_global_peak[tag] =
            hps_alloc_stats.tags[tag].live_bytes;
        hps_alloc_stats.blocks_at_global_peak[tag] =
            hps_alloc_stats.tags[tag].live_blocks;
    }
}

static void hps_alloc_live_add(HpsAllocTag tag, size_t bytes, size_t blocks)
{
    HpsAllocTagStats *tag_stats = &hps_alloc_stats.tags[tag];
    hps_alloc_counter_add(&hps_alloc_stats.live_bytes, bytes);
    hps_alloc_counter_add(&hps_alloc_stats.live_blocks, blocks);
    hps_alloc_counter_add(&tag_stats->live_bytes, bytes);
    hps_alloc_counter_add(&tag_stats->live_blocks, blocks);
    if (tag_stats->live_bytes > tag_stats->peak_live_bytes)
        tag_stats->peak_live_bytes = tag_stats->live_bytes;
    if (tag_stats->live_blocks > tag_stats->peak_live_blocks)
        tag_stats->peak_live_blocks = tag_stats->live_blocks;
    if (hps_alloc_stats.live_blocks > hps_alloc_stats.max_live_blocks)
        hps_alloc_stats.max_live_blocks = hps_alloc_stats.live_blocks;
    hps_alloc_capture_global_peak();
}

static void hps_alloc_live_resize(HpsAllocTag tag, size_t old_size,
    size_t new_size)
{
    HpsAllocTagStats *tag_stats = &hps_alloc_stats.tags[tag];
    if (new_size > old_size) {
        size_t increase = new_size - old_size;
        hps_alloc_counter_add(&hps_alloc_stats.live_bytes, increase);
        hps_alloc_counter_add(&tag_stats->live_bytes, increase);
        if (tag_stats->live_bytes > tag_stats->peak_live_bytes)
            tag_stats->peak_live_bytes = tag_stats->live_bytes;
        hps_alloc_capture_global_peak();
    } else {
        size_t decrease = old_size - new_size;
        if (hps_alloc_stats.live_bytes >= decrease)
            hps_alloc_stats.live_bytes -= decrease;
        else {
            hps_alloc_stats.live_bytes = 0;
            hps_alloc_stats.counter_overflowed = 1;
        }
        if (tag_stats->live_bytes >= decrease)
            tag_stats->live_bytes -= decrease;
        else {
            tag_stats->live_bytes = 0;
            hps_alloc_stats.counter_overflowed = 1;
        }
    }
}

static void *hps_alloc_new(size_t size, int count_alloc_call, HpsAllocTag tag,
    int count_attempt)
{
    HpsAllocHeader *header;
    size_t total_size;
    HpsAllocTagStats *tag_stats;

    tag = hps_alloc_normalize_tag(tag);
    tag_stats = &hps_alloc_stats.tags[tag];
    if (count_alloc_call) {
        hps_alloc_counter_add(&hps_alloc_stats.alloc_calls, 1);
        hps_alloc_counter_add(&tag_stats->alloc_calls, 1);
    }
    if (count_attempt && hps_alloc_test_should_fail()) {
        hps_alloc_failed(tag);
        return NULL;
    }
    if (size > (size_t)-1 - sizeof(*header)) {
        hps_alloc_failed(tag);
        return NULL;
    }
    total_size = sizeof(*header) + size;
    header = (HpsAllocHeader *)malloc(total_size);
    if (header == NULL) {
        hps_alloc_failed(tag);
        return NULL;
    }
    header->data.requested_size = size;
    header->data.tag = tag;
    hps_alloc_live_add(tag, size, 1);
    hps_alloc_counter_add(&hps_alloc_stats.total_successful_requested_bytes,
        size);
    return (void *)(header + 1);
}

static void hps_alloc_release(void *ptr, int count_free_call)
{
    HpsAllocHeader *header;
    HpsAllocTag tag;
    HpsAllocTagStats *tag_stats;
    size_t old_size;

    if (ptr == NULL) return;
    header = ((HpsAllocHeader *)ptr) - 1;
    old_size = header->data.requested_size;
    tag = hps_alloc_normalize_tag(header->data.tag);
    tag_stats = &hps_alloc_stats.tags[tag];
    if (hps_alloc_stats.live_bytes >= old_size)
        hps_alloc_stats.live_bytes -= old_size;
    else {
        hps_alloc_stats.live_bytes = 0;
        hps_alloc_stats.counter_overflowed = 1;
    }
    if (tag_stats->live_bytes >= old_size)
        tag_stats->live_bytes -= old_size;
    else {
        tag_stats->live_bytes = 0;
        hps_alloc_stats.counter_overflowed = 1;
    }
    if (hps_alloc_stats.live_blocks > 0) --hps_alloc_stats.live_blocks;
    else hps_alloc_stats.counter_overflowed = 1;
    if (tag_stats->live_blocks > 0) --tag_stats->live_blocks;
    else hps_alloc_stats.counter_overflowed = 1;
    if (count_free_call) {
        hps_alloc_counter_add(&hps_alloc_stats.free_calls, 1);
        hps_alloc_counter_add(&tag_stats->free_calls, 1);
    }
    free(header);
}

void *hps_alloc(size_t size)
{
    return hps_alloc_tagged(size, HPS_ALLOC_TAG_OTHER);
}

void *hps_alloc_tagged(size_t size, HpsAllocTag tag)
{
    return hps_alloc_new(size, 1, hps_alloc_normalize_tag(tag), 1);
}

void *hps_realloc(void *ptr, size_t size)
{
    HpsAllocHeader *old_header;
    HpsAllocHeader *new_header;
    HpsAllocTag tag;
    HpsAllocTagStats *tag_stats;
    size_t old_size;
    size_t new_total_size;

    if (ptr == NULL) {
        hps_alloc_counter_add(&hps_alloc_stats.realloc_calls, 1);
        hps_alloc_counter_add(&hps_alloc_stats.tags[HPS_ALLOC_TAG_OTHER].realloc_calls, 1);
        return hps_alloc_new(size, 0, HPS_ALLOC_TAG_OTHER, size > 0);
    }
    old_header = ((HpsAllocHeader *)ptr) - 1;
    tag = hps_alloc_normalize_tag(old_header->data.tag);
    tag_stats = &hps_alloc_stats.tags[tag];
    hps_alloc_counter_add(&hps_alloc_stats.realloc_calls, 1);
    hps_alloc_counter_add(&tag_stats->realloc_calls, 1);
    if (size == 0) {
        hps_alloc_release(ptr, 0);
        return NULL;
    }
    if (hps_alloc_test_should_fail()) {
        hps_alloc_failed(tag);
        return NULL;
    }
    if (size > (size_t)-1 - sizeof(*old_header)) {
        hps_alloc_failed(tag);
        return NULL;
    }
    old_size = old_header->data.requested_size;
    new_total_size = sizeof(*old_header) + size;
    new_header = (HpsAllocHeader *)realloc(old_header, new_total_size);
    if (new_header == NULL) {
        hps_alloc_failed(tag);
        return NULL;
    }
    new_header->data.requested_size = size;
    new_header->data.tag = tag;
    hps_alloc_live_resize(tag, old_size, size);
    hps_alloc_counter_add(&hps_alloc_stats.total_successful_requested_bytes,
        size);
    return (void *)(new_header + 1);
}

void hps_free(void *ptr)
{
    hps_alloc_release(ptr, 1);
}

int hps_alloc_stats_reset(void)
{
    if (hps_alloc_stats.live_blocks != 0 || hps_alloc_stats.live_bytes != 0)
        return 1;
    hps_alloc_stats = (HpsAllocStats){ 0 };
    return 0;
}

HpsAllocStats hps_alloc_stats_get(void)
{
    return hps_alloc_stats;
}
