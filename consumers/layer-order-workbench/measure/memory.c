#if defined(__APPLE__)
/* Darwin's extended rusage fields are hidden by strict POSIX feature selection. */
#define _DARWIN_C_SOURCE 1
#elif !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "workload.h"
#include "trace.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif
#if MEASURE_TRACKING
#ifdef __linux__
#include <malloc.h>
#endif
typedef struct { void *pointer; size_t requested, usable; } Allocation;
static Allocation ledger[WB_LIVE + 2u];
static size_t requests, requested_total, frees, live_count, live_requested, live_usable, peak_requested;
static int tracking;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
static void remember(void *p, size_t size)
{
    size_t i, usable = 0;
    if (!p || !tracking) return;
    for (i = 0; i < WB_LIVE + 2u && ledger[i].pointer; ++i) {}
    if (i == WB_LIVE + 2u) abort();
#ifdef __linux__
    usable = malloc_usable_size(p);
#endif
    ledger[i] = (Allocation){p, size, usable};
    ++requests; ++live_count; requested_total += size; live_requested += size; live_usable += usable;
    if (live_requested > peak_requested) peak_requested = live_requested;
}
void *__wrap_malloc(size_t size) { void *p = __real_malloc(size); remember(p, size); return p; }
void *__wrap_calloc(size_t count, size_t size) { void *p = __real_calloc(count, size); remember(p, count * size); return p; }
void __wrap_free(void *p)
{
    size_t i;
    if (p) for (i = 0; i < WB_LIVE + 2u; ++i) if (ledger[i].pointer == p) {
        --live_count; ++frees; live_requested -= ledger[i].requested; live_usable -= ledger[i].usable;
        ledger[i] = (Allocation){0}; break;
    }
    __real_free(p);
}
#define TRACK(value) (tracking = (value))
#else
#define TRACK(value) ((void)(value))
#endif
static void rss(size_t *current, size_t *peak)
{
    *current = *peak = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS info;
    memset(&info, 0, sizeof(info)); info.cb = sizeof(info);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) abort();
    *current = info.WorkingSetSize; *peak = info.PeakWorkingSetSize;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) abort();
#ifdef __linux__
    FILE *file = fopen("/proc/self/statm", "r");
    size_t total, resident;
    long page = sysconf(_SC_PAGESIZE);
    if (!file || page <= 0 || fscanf(file, "%zu %zu", &total, &resident) != 2) abort();
    (void)fclose(file);
    *current = resident * (size_t)page;
    /* getrusage can retain a pre-exec Python launcher high-water mark. VmHWM
       belongs to this executable's current address space instead. */
    file = fopen("/proc/self/status", "r");
    if (!file) abort();
    { char line[256]; size_t high;
      while (fgets(line, sizeof(line), file))
          if (sscanf(line, "VmHWM: %zu kB", &high) == 1) *peak = high * 1024u; }
    if (ferror(file) || fclose(file) || !*peak) abort();
#else
    *peak = (size_t)usage.ru_maxrss; /* macOS bytes; current RSS unavailable. */
#endif
#endif
}
static void sample(const char *stage, size_t n, const Workbench *wb, size_t app_bytes)
{
    size_t current, peak;
    rss(&current, &peak);
    printf("%s,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,", stage, n, wb ? wb->model.count : 0,
        current, peak, app_bytes, sizeof(((Workbench *)0)->objects),
        sizeof(((Workbench *)0)->occurrences), sizeof(Oracle), (size_t)WB_RECORD * 2u);
#if MEASURE_TRACKING
    printf("%zu,%zu,%zu,%zu,%zu,%zu,", requests, requested_total, frees, live_count, live_requested, peak_requested);
#ifdef __linux__
    printf("%zu,", live_usable);
#else
    printf("NA,");
#endif
    printf("%zu\n", sizeof(ledger));
#else
    puts("NA,NA,NA,NA,NA,NA,NA,0");
#endif
    if (fflush(stdout)) abort();
}
static int insert(Workbench *wb, size_t payload)
{
    LksMiniHandle *handle = NULL;
    LksMiniStatus s;
    TRACK(1); s = lks_mini_insert_back(wb->order, payload ? &wb->objects[payload - 1u] : NULL, &handle); TRACK(0);
    if (s != LKS_MINI_OK || !handle || wb->issued == WB_OCCURRENCES) return 0;
    wb->occurrences[wb->issued] = (LiveOccurrence){handle, payload};
    ++wb->issued;
    if (payload) ++wb->objects[payload - 1u].references;
    oracle_insert(&wb->model, wb->model.count, (ModelEntry){wb->issued, payload});
    return 1;
}
static int remove_first(Workbench *wb)
{
    size_t id = wb->model.entries[0].id;
    LiveOccurrence *occ = &wb->occurrences[id - 1u];
    LksMiniStatus s;
    TRACK(1); s = lks_mini_remove(wb->order, occ->handle); TRACK(0);
    if (s != LKS_MINI_OK) return 0;
    if (occ->payload) --wb->objects[occ->payload - 1u].references;
    occ->handle = NULL; occ->payload = 0;
    oracle_remove(&wb->model, 0);
    return 1;
}
int main(int argc, char **argv)
{
    size_t n, i;
    Workbench *wb = NULL;
    LksMiniStatus status;
    int failed = 1;
    if (argc != 2) return 1;
    if (!strcmp(argv[1], "16")) n = 16;
    else if (!strcmp(argv[1], "128")) n = 128;
    else if (!strcmp(argv[1], "2048")) n = 2048;
    else return 1;
    puts("stage,size,active,rss_bytes,peak_rss_bytes,app_workbench_bytes,object_capacity_bytes,mapping_capacity_bytes,oracle_bytes,phase1_trace_buffer_estimate_bytes,mini_allocations,mini_requested_total_bytes,mini_frees,mini_live_allocations,mini_live_requested_bytes,mini_peak_requested_bytes,mini_live_usable_bytes,tracker_bytes");
    /* Warm the observation path before establishing baseline. */
    { size_t current, peak; rss(&current, &peak); }
#if MEASURE_TRACKING
    memset(ledger, 0, sizeof(ledger));
#endif
    sample("baseline", n, NULL, 0);
    wb = calloc(1, sizeof(*wb));
    if (!wb) goto done;
    /* Touch each application page so lazy calloc is not mistaken for absent storage. */
    { volatile unsigned char *bytes = (volatile unsigned char *)wb;
      for (i = 0; i < sizeof(*wb); i += 4096u) bytes[i] = 0; }
    sample("app-allocated", n, wb, sizeof(*wb));
    TRACK(1); status = lks_mini_create(&wb->order); TRACK(0);
    if (status != LKS_MINI_OK) goto done;
    wb->object_count = 2;
    for (i = 0; i < 2; ++i) wb->objects[i].id = i + 1u;
    sample("order-created", n, wb, sizeof(*wb));
    for (i = 0; i < n; ++i) if (!insert(wb, i % 3u)) goto done;
    if (!wb_verify(wb)) goto done;
    sample("inserted", n, wb, sizeof(*wb));
#if MEASURE_TRACKING
    if (live_count != n + 1u || requests != n + 1u) goto done;
#endif
    for (i = 0; i < n / 2u; ++i) if (!remove_first(wb)) goto done;
    if (!wb_verify(wb)) goto done;
    sample("half-removed", n, wb, sizeof(*wb));
    for (i = 0; i < n / 2u; ++i) if (!insert(wb, i % 3u)) goto done;
    if (!wb_verify(wb)) goto done;
    sample("reinserted", n, wb, sizeof(*wb));
    for (i = 0; i < 128; ++i) if (!remove_first(wb) || !insert(wb, i % 3u)) goto done;
    if (!wb_verify(wb)) goto done;
    sample("128-churn-cycles", n, wb, sizeof(*wb));
    TRACK(1); wb_destroy(wb); TRACK(0);
    sample("mini-destroyed", n, NULL, sizeof(*wb));
#if MEASURE_TRACKING
    if (live_count || live_requested || requests != frees) goto done;
#endif
    free(wb); wb = NULL;
    sample("app-freed", n, NULL, 0);
    failed = 0;
done:
    if (wb) { TRACK(1); wb_destroy(wb); TRACK(0); free(wb); }
    return failed;
}
