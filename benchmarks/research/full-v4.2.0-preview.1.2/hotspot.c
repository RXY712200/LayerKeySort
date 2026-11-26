#define _POSIX_C_SOURCE 200809L
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d\n",__LINE__); exit(1); } } while (0)

static double now_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER t, f;
    CHECK(QueryPerformanceCounter(&t) && QueryPerformanceFrequency(&f));
    return (double)t.QuadPart * 1e9 / (double)f.QuadPart;
#else
    struct timespec t;
    CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);
    return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
#endif
}

int main(void)
{
    enum { PREFIX = 130, INSERTS = 10000 };
    const LksOrderHandle **expected = calloc(PREFIX + INSERTS, sizeof(*expected));
    double *durations = calloc(INSERTS, sizeof(*durations));
    LksOrder *order = lks_order_create();
    int payload = 3;
    CHECK(expected && durations && order);
    for (size_t i=0; i<PREFIX; ++i)
        CHECK(lks_order_insert_back(order,&payload,&expected[i ? INSERTS+i : 0]) == LKS_STATUS_OK);
    for (size_t i=1; i<=INSERTS; ++i) {
        double start = now_ns();
        CHECK(lks_order_insert_before(order,expected[INSERTS+1],&payload,&expected[i]) == LKS_STATUS_OK);
        durations[i-1] = now_ns()-start;
        int result;
        CHECK(lks_order_compare(order,expected[i-1],expected[i],&result) == LKS_STATUS_OK && result == -1);
        CHECK(lks_order_compare(order,expected[INSERTS+1],expected[i],&result) == LKS_STATUS_OK && result == 1);
    }
    const LksOrderHandle *cursor = lks_order_first(order);
    for (size_t i=0; i<PREFIX+INSERTS; ++i) {
        CHECK(cursor == expected[i]);
        cursor = lks_order_next(cursor);
    }
    CHECK(!cursor);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(lks_order_internal_valid(order));
#ifdef PREVIEW_TAGS
    uint64_t last = 0;
    for (LksOrderBlock *b=order->first; b; b=b->next) {
        if (b->tag) { CHECK(b->tag>last); last=b->tag; }
    }
    printf("tags,%zu,%zu,%zu,%zu\n",order->tag_writes,order->tag_repairs,
           order->tag_fallbacks,order->max_tag_writes);
#endif
    printf("memory,%zu,%zu\n",lks_alloc_stats_get().live_bytes,lks_alloc_stats_get().peak_live_bytes);
#else
    for (size_t i=0; i<INSERTS; ++i)
        printf("%zu,%.0f\n",i+1,durations[i]);
#endif
    lks_order_destroy(order);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(lks_alloc_stats_get().live_bytes == 0);
#endif
    free(durations);
    free(expected);
    return 0;
}
