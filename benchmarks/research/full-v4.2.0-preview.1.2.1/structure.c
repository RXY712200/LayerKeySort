#define _POSIX_C_SOURCE 200809L
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d\n", __LINE__); exit(1); } } while (0)

#ifdef _WIN32
static double tick_ns;
#endif
static uint64_t ticks(void)
{
#ifdef _WIN32
    LARGE_INTEGER value;
    CHECK(QueryPerformanceCounter(&value));
    return (uint64_t)value.QuadPart;
#else
    struct timespec value;
    CHECK(clock_gettime(CLOCK_MONOTONIC, &value) == 0);
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
#endif
}

static int compare_ticks(const void *p, const void *q)
{
    uint64_t a = *(const uint64_t *)p, b = *(const uint64_t *)q;
    return (a > b) - (a < b);
}

static void report(const char *name, uint64_t *times, size_t count, const LksOrder *order)
{
    size_t min = LKS_ORDER_BLOCK_CAPACITY, max = 0, zeros = 0, cluster = 0, longest = 0;
    uint64_t prior = 0;
    size_t occupancy[LKS_ORDER_BLOCK_CAPACITY + 1] = {0};
    for (const LksOrderBlock *p = order->first; p; p = p->next) {
        CHECK(p->count <= LKS_ORDER_BLOCK_CAPACITY);
        ++occupancy[p->count];
        if (p->count < min) min = p->count;
        if (p->count > max) max = p->count;
        if (p->tag) {
            CHECK(p->tag > prior);
            prior = p->tag;
            cluster = 0;
        } else {
            ++zeros;
            if (++cluster > longest) longest = cluster;
        }
    }
    printf("%s,residents=%zu,blocks=%zu,min_fill=%zu,max_fill=%zu,zero_tags=%zu,max_zero_cluster=%zu",
        name, order->count, order->blocks, min, max, zeros, longest);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(lks_order_internal_valid(order));
    printf(",tag_stores=%zu,repairs=%zu,fallbacks=%zu,max_stores=%zu,requested_live=%zu,requested_peak=%zu",
        order->tag_writes, order->tag_repairs, order->tag_fallbacks, order->max_tag_writes,
        lks_alloc_stats_get().live_bytes, lks_alloc_stats_get().peak_live_bytes);
#endif
    if (count) {
        static const size_t per_mille[] = {500, 950, 990, 999, 1000};
        static const char *names[] = {"p50", "p95", "p99", "p999", "max"};
        qsort(times, count, sizeof(*times), compare_ticks);
        for (size_t i = 0; i < sizeof(per_mille)/sizeof(*per_mille); ++i) {
            size_t at = (count * per_mille[i] + 999)/1000 - 1;
            double ns = (double)times[at];
#ifdef _WIN32
            ns *= tick_ns;
#endif
            printf(",%s_ns=%.1f", names[i], ns);
        }
    }
    for (size_t i = 0; i <= LKS_ORDER_BLOCK_CAPACITY; ++i)
        if (occupancy[i]) printf(",fill%zu=%zu", i, occupancy[i]);
    putchar('\n');
}

static void check_sequence(const LksOrder *order, const LksOrderHandle **handles, size_t n)
{
    const LksOrderHandle *cursor = lks_order_first(order);
    size_t same = 0, fast = 0, fallback = 0;
    for (size_t i = 0; i < n; ++i) {
        int cmp;
        CHECK(cursor == handles[i]);
        cursor = lks_order_next(cursor);
        size_t j = i / 2;
        CHECK(lks_order_compare(order, handles[j], handles[i], &cmp) == LKS_STATUS_OK);
        CHECK(cmp == (j == i ? 0 : -1));
        if (handles[j]->block == handles[i]->block) ++same;
        else if (handles[j]->block->tag && handles[i]->block->tag) ++fast;
        else ++fallback;
    }
    CHECK(!cursor);
    printf("sequence,n=%zu,same=%zu,fast=%zu,fallback=%zu\n", n, same, fast, fallback);
}

int main(void)
{
    enum { B = LKS_ORDER_BLOCK_CAPACITY, PREFIX = 4*B, N = 100000 };
    const LksOrderHandle **handles = calloc(N+PREFIX+2*B, sizeof(*handles));
    uint64_t *times = calloc(N, sizeof(*times));
    LksOrder *order = lks_order_create();
    int item = 1;
    CHECK(handles && times && order);
#ifdef _WIN32
    LARGE_INTEGER frequency;
    CHECK(QueryPerformanceFrequency(&frequency));
    tick_ns = 1e9 / (double)frequency.QuadPart;
#endif
    CHECK(lks_order_insert_back(order, &item, &handles[0]) == LKS_STATUS_OK);
    for (size_t i = 1; i < PREFIX; ++i)
        CHECK(lks_order_insert_back(order, &item, &handles[N+i]) == LKS_STATUS_OK);
    for (size_t i = 1; i <= N; ++i) {
        uint64_t start = ticks();
        CHECK(lks_order_insert_before(order, handles[N+1], &item, &handles[i]) == LKS_STATUS_OK);
        times[i-1] = ticks() - start;
    }
    report("focused-insert",times,N,order);
    check_sequence(order,handles,N+PREFIX);
    for (size_t i = 1; i <= N; ++i) {
        void *out = NULL;
        uint64_t start = ticks();
        CHECK(lks_order_remove(order, handles[i], &out) == LKS_STATUS_OK && out == &item);
        times[i-1] = ticks() - start;
    }
    report("shrink",times,N,order);
    memmove(handles+1,handles+N+1,(PREFIX-1)*sizeof(*handles));
    check_sequence(order,handles,PREFIX);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(order->last->previous);
    order->last->previous->tag = 0;
    order->last->tag = 0;
    report("forced-zero-cluster",times,0,order);
    check_sequence(order,handles,PREFIX);
#endif
    for (size_t i = 0; i < N; ++i) {
        uint64_t start = ticks();
        CHECK(lks_order_insert_back(order, &item, &handles[PREFIX+i]) == LKS_STATUS_OK);
        times[i] = ticks() - start;
    }
    report("regrow",times,N,order);
    check_sequence(order,handles,N+PREFIX);
    lks_order_destroy(order);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(lks_alloc_stats_get().live_bytes == 0);
#endif
    free(times);
    free(handles);
    return 0;
}
