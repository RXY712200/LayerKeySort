/* Held-out shape built by the same bulk constructor used during restore. */
#include "lks_alloc_internal.h"
#include "lks_order_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d\n", __LINE__); exit(1); } } while (0)

static void verify(const LksOrder *order, const LksOrderHandle **expected, size_t n)
{
    const LksOrderHandle *current = lks_order_first(order);
    CHECK(lks_order_internal_valid(order));
    for (size_t i = 0; i < n; ++i) {
        CHECK(current == expected[i]);
        current = lks_order_next(current);
    }
    CHECK(!current);
}

static void report(const LksOrder *order, const char *phase, size_t same, size_t cross,
    size_t full, size_t index, size_t assignments)
{
    size_t min = LKS_ORDER_BLOCK_CAPACITY, max = 0, zeros = 0;
    for (const LksOrderBlock *b = order->first; b; b = b->next) {
        if (b->count < min) min = b->count;
        if (b->count > max) max = b->count;
        if (!b->tag) ++zeros;
    }
    printf("%s,n=%zu,blocks=%zu,min=%zu,max=%zu,zero=%zu,same=%zu,cross=%zu,full_dest=%zu,"
           "tag_stores=%zu,tag_repairs=%zu,tag_fallbacks=%zu,index_touches=%zu,position_writes=%zu,live=%zu\n",
        phase, order->count, order->blocks, min, max, zeros, same, cross, full,
        order->tag_writes, order->tag_repairs, order->tag_fallbacks, index, assignments,
        lks_alloc_stats_get().live_bytes);
}

int main(void)
{
    const size_t populations[] = {1000, 10000, 100000};
    for (size_t j = 0; j < sizeof(populations)/sizeof(*populations); ++j) {
        size_t n = populations[j];
        int payload = 0;
        void **items = malloc(n * sizeof(*items));
        const LksOrderHandle **expected = malloc(n * sizeof(*expected));
        LksOrder *order = NULL;
        CHECK(items && expected);
        for (size_t i = 0; i < n; ++i) items[i] = &payload;
        CHECK(lks_order_bulk_build(items, n, &order) == LKS_STATUS_OK);
        const LksOrderHandle *cursor = lks_order_first(order);
        for (size_t i = 0; i < n; ++i) {
            CHECK(cursor);
            expected[i] = cursor;
            cursor = lks_order_next(cursor);
        }
        verify(order, expected, n);
        report(order, "bulk-built", 0, 0, 0, 0, 0);
        size_t same = 0, cross = 0, full = 0, index = 0, assignments = 0;
        for (size_t i = 0; i < 4096; ++i) {
            size_t from = i % 3 == 0 ? n/2 + 9 : (i % 2 ? n-1 : 0);
            size_t anchor = n/2 - i%7;
            const LksOrderHandle *record = expected[from];
            const LksOrderHandle *bound = expected[anchor];
            if (record->block == bound->block) ++same;
            else { ++cross; if (bound->block->count == LKS_ORDER_BLOCK_CAPACITY) ++full; }
            CHECK(lks_order_move_before(order, record, bound) == LKS_STATUS_OK);
            if (from < anchor) {
                --anchor;
                memmove(expected+from, expected+from+1, (anchor-from)*sizeof(*expected));
            } else {
                memmove(expected+anchor+1, expected+anchor, (from-anchor)*sizeof(*expected));
            }
            expected[anchor] = record;
            index += order->work.index_blocks;
            assignments += order->work.records_reassigned;
            if (!(i%256)) verify(order, expected, n);
        }
        verify(order, expected, n);
        report(order, "mixed-move", same, cross, full, index, assignments);
        lks_order_destroy(order);
        CHECK(lks_alloc_stats_get().live_bytes == 0);
        free(expected);
        free(items);
    }
    return 0;
}
