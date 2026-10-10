/* Test executable only: GNU-compatible linker's symbol wrapping. Neither
   consumer nor Mini source/API is replaced or extended. No private headers. */
#include "workbench.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
static int fail_next;
static int tracking;
static size_t live_allocations, failures;
static void *tracked[WB_LIVE + 2u];
static void remember(void *pointer)
{
    if (pointer && tracking) {
        size_t i;
        for (i = 0; i < WB_LIVE + 2u; ++i) if (!tracked[i]) break;
        if (i == WB_LIVE + 2u) abort();
        tracked[i] = pointer;
        ++live_allocations;
    }
}
void *__wrap_malloc(size_t size)
{
    void *pointer;
    if (fail_next) { fail_next = 0; ++failures; return NULL; }
    pointer = __real_malloc(size);
    remember(pointer);
    return pointer;
}
void *__wrap_calloc(size_t count, size_t size)
{
    void *pointer;
    if (fail_next) { fail_next = 0; ++failures; return NULL; }
    pointer = __real_calloc(count, size);
    remember(pointer);
    return pointer;
}
void __wrap_free(void *pointer)
{
    size_t i;
    if (pointer) for (i = 0; i < WB_LIVE + 2u; ++i) {
        if (tracked[i] == pointer) { tracked[i] = NULL; --live_allocations; break; }
    }
    __real_free(pointer);
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "OOM CHECK line=%d: %s\n", __LINE__, #x); failed = 1; goto done; } } while (0)
static int execute(Workbench *wb, const char *text, const char *expected)
{
    Command c;
    Result r;
    const char *error = command_parse(text, &c);
    int ok;
    ++wb->sequence;
    tracking = 1;
    ok = wb_execute(wb, &c, error, &r) && !strcmp(r.status, expected);
    tracking = 0;
    return ok;
}
int main(void)
{
    const char *commands[] = {"insert-front 1", "insert-back NULL", "insert-before 1 1", "insert-after NULL 1"};
    Workbench *wb = malloc(sizeof(*wb));
    int failed = 0;
    size_t i;
    CHECK(wb);
    tracking = 1;
    fail_next = 1;
    CHECK(!wb_init(wb) && !wb->order && !fail_next && failures == 1);
    CHECK(wb_init(wb));
    tracking = 0;
    CHECK(execute(wb, "create Shared", "OK"));
    CHECK(execute(wb, "insert-back 1", "OK"));
    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        Oracle before = wb->model;
        size_t issued = wb->issued, refs = wb->objects[0].references, allocations = live_allocations;
        LksMiniHandle *anchor = wb->occurrences[0].handle;
        fail_next = 1;
        CHECK(execute(wb, commands[i], "MINI_OUT_OF_MEMORY"));
        CHECK(!fail_next && live_allocations == allocations && wb->issued == issued);
        CHECK(wb->objects[0].references == refs && wb->occurrences[0].handle == anchor);
        CHECK(wb->model.count == before.count &&
              !memcmp(wb->model.entries, before.entries, before.count * sizeof(ModelEntry)));
        CHECK(execute(wb, commands[i], "OK"));
        CHECK(wb->issued == issued + 1u && wb_verify(wb));
    }
    CHECK(failures == 5);
done:
    fail_next = 0; tracking = 0;
    if (wb) { wb_destroy(wb); free(wb); }
    if (live_allocations) { fprintf(stderr, "OOM test live allocations=%zu\n", live_allocations); failed = 1; }
    if (!failed) puts("Consumer OOM: creation + four insertion failures/recovery PASS; zero tracked Mini allocations");
    return failed;
}
