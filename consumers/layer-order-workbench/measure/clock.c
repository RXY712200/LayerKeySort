#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "clock.h"
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
static uint64_t frequency(void)
{
    LARGE_INTEGER f;
    if (!QueryPerformanceFrequency(&f) || f.QuadPart <= 0) abort();
    return (uint64_t)f.QuadPart;
}
uint64_t measure_now(void)
{
    LARGE_INTEGER t;
    uint64_t f = frequency(), c;
    if (!QueryPerformanceCounter(&t)) abort();
    c = (uint64_t)t.QuadPart;
    return (c / f) * UINT64_C(1000000000) + (c % f) * UINT64_C(1000000000) / f;
}
uint64_t measure_resolution(void) { return (UINT64_C(1000000000) + frequency() - 1u) / frequency(); }
const char *measure_clock_name(void) { return "QueryPerformanceCounter"; }
#else
#include <time.h>
uint64_t measure_now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) abort();
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}
uint64_t measure_resolution(void)
{
    struct timespec t;
    if (clock_getres(CLOCK_MONOTONIC, &t)) abort();
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}
const char *measure_clock_name(void) { return "CLOCK_MONOTONIC"; }
#endif
static uint64_t observed_step;
static size_t zero_pairs;
uint64_t measure_observed_step(void) { return observed_step; }
size_t measure_zero_pairs(void) { return zero_pairs; }
static int compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
uint64_t measure_empty_pair(void)
{
    uint64_t samples[1001];
    size_t i;
    observed_step = 0; zero_pairs = 0;
    for (i = 0; i < 1001; ++i) {
        uint64_t a = measure_now(); samples[i] = measure_now() - a;
        if (!samples[i]) ++zero_pairs;
        else if (!observed_step || samples[i] < observed_step) observed_step = samples[i];
    }
    qsort(samples, 1001, sizeof(samples[0]), compare_u64);
    return samples[500];
}
