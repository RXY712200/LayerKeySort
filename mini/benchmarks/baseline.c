#include "layerkeysort_mini.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define OK(x) do { if ((x) != LKS_MINI_OK) exit(1); } while (0)
static double now(void)
{
    struct timespec t;
    if (timespec_get(&t, TIME_UTC) != TIME_UTC) exit(1);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
static void report(size_t size, const char *workload, size_t ops, double start)
{
    printf("%zu,%s,%zu,%.6f\n", size, workload, ops,
           1000.0 * (now() - start));
}
int main(void)
{
    const size_t sizes[] = {128,2048,32768};
    puts("size,workload,operations,wall_ms");
    for (size_t s = 0; s < 3; ++s) {
        size_t n = sizes[s], repeats = 20000;
        LksMiniHandle **h = malloc(n * sizeof *h);
        LksMiniOrder *o = NULL;
        LksMiniHandle *cursor = NULL;
        double start;
        int result;
        if (!h) return 1;
        OK(lks_mini_create(&o)); start = now();
        for (size_t i = 0; i < n; ++i) OK(lks_mini_insert_back(o, NULL, &h[i]));
        report(n, "tail_insert", n, start);
        start = now();
        for (size_t i = 0; i < repeats; ++i) { OK(lks_mini_move_front(o, h[n-1])); OK(lks_mini_move_back(o, h[n-1])); }
        report(n, "known_handle_move", repeats * 2, start);
        start = now();
        for (size_t i = 0; i < 20; ++i) {
            OK(lks_mini_first(o, &cursor));
            while (cursor) OK(lks_mini_next(o, cursor, &cursor));
        }
        report(n, "traverse", n * 20, start);
        start = now();
        for (size_t i = 0; i < 200; ++i) { OK(lks_mini_compare(o, h[0], h[n-1], &result)); if (result != -1) return 1; }
        report(n, "head_tail_compare", 200, start);
        start = now();
        for (size_t i = 0; i < repeats; ++i) {
            LksMiniHandle *temporary = NULL;
            OK(lks_mini_insert_front(o, NULL, &temporary));
            OK(lks_mini_move_back(o, temporary)); OK(lks_mini_remove(o, temporary));
        }
        report(n, "mixed_insert_move_remove", repeats * 3, start);
        start = now();
        for (size_t i = 0; i < n; ++i) OK(lks_mini_remove(o, h[i]));
        report(n, "remove", n, start);
        lks_mini_destroy(o);
        OK(lks_mini_create(&o)); start = now();
        for (size_t i = 0; i < n; ++i) OK(lks_mini_insert_front(o, NULL, &h[i]));
        report(n, "head_insert", n, start);
        lks_mini_destroy(o); free(h);
    }
    return 0;
}
