#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define OK(expr) CHECK((expr) == LKS_MINI_OK)
static size_t live_allocations;
static size_t allocation_calls;
static size_t free_calls;
static int fail_next;
static void *test_malloc(size_t size)
{
    void *p;
    ++allocation_calls;
    if (fail_next) { fail_next = 0; return NULL; }
    p = malloc(size);
    if (p) ++live_allocations;
    return p;
}
static void test_free(void *p)
{
    if (p) { CHECK(live_allocations > 0); --live_allocations; ++free_calls; }
    free(p);
}
/* Test-only substitution; production has no hooks or mutable diagnostics. */
#define malloc test_malloc
#define free test_free
#include "../src/lks_mini.c"
#undef malloc
#undef free

static void invariant(const LksMiniOrder *order, LksMiniHandle **expected, size_t count)
{
    const LksMiniHandle *p = order->head, *prev = NULL;
    size_t n = 0;
    CHECK(order->size == count);
    CHECK((order->head == NULL) == (count == 0));
    CHECK((order->tail == NULL) == (count == 0));
    if (count) { CHECK(order->head->prev == NULL); CHECK(order->tail->next == NULL); }
    while (p) {
        CHECK(n < count); /* Bounds traversal and detects cycles/repeated nodes. */
        CHECK(p == expected[n]); CHECK(p->owner == order); CHECK(p->prev == prev);
        if (prev) CHECK(prev->next == p);
        if (p->next) CHECK(p->next->prev == p);
        prev = p; p = p->next; ++n;
    }
    CHECK(n == count); CHECK(prev == order->tail);
    p = order->tail; n = count;
    while (p) { CHECK(n > 0); CHECK(p == expected[--n]); p = p->prev; }
    CHECK(n == 0);
}

int main(void)
{
    LksMiniOrder *order = NULL, *other = NULL;
    LksMiniHandle *a = NULL, *b = NULL, *c = NULL, *d = NULL, *out = NULL, *foreign = NULL;
    int payload = 42;
    size_t calls, frees;
    fail_next = 1;
    CHECK(lks_mini_create(&order) == LKS_MINI_OUT_OF_MEMORY);
    CHECK(order == NULL); CHECK(live_allocations == 0);
    OK(lks_mini_create(&order)); invariant(order, NULL, 0);
    out = (LksMiniHandle *)order;
    fail_next = 1;
    CHECK(lks_mini_insert_front(order, NULL, &out) == LKS_MINI_OUT_OF_MEMORY);
    CHECK(out == NULL); invariant(order, NULL, 0); CHECK(live_allocations == 1);
    OK(lks_mini_insert_front(order, &payload, &b)); invariant(order, (LksMiniHandle *[]){b}, 1);
    OK(lks_mini_insert_back(order, &payload, &d)); invariant(order, (LksMiniHandle *[]){b,d}, 2);
    OK(lks_mini_insert_before(order, b, NULL, &a)); invariant(order, (LksMiniHandle *[]){a,b,d}, 3);
    OK(lks_mini_insert_after(order, b, NULL, &c)); invariant(order, (LksMiniHandle *[]){a,b,c,d}, 4);
    /* Basic failure at each insertion entry point; no systematic campaign. */
#define FAIL_INSERT(expr) do { out = a; fail_next = 1; CHECK((expr) == LKS_MINI_OUT_OF_MEMORY); CHECK(out == NULL); invariant(order, (LksMiniHandle *[]){a,b,c,d}, 4); CHECK(live_allocations == 5); } while (0)
    FAIL_INSERT(lks_mini_insert_front(order, NULL, &out));
    FAIL_INSERT(lks_mini_insert_back(order, NULL, &out));
    FAIL_INSERT(lks_mini_insert_before(order, c, NULL, &out));
    FAIL_INSERT(lks_mini_insert_after(order, b, NULL, &out));
    OK(lks_mini_create(&other)); OK(lks_mini_insert_back(other, NULL, &foreign));
    calls = allocation_calls;
    out = a; CHECK(lks_mini_insert_before(order, foreign, NULL, &out) == LKS_MINI_WRONG_ORDER); CHECK(out == NULL);
    out = a; CHECK(lks_mini_insert_after(order, foreign, NULL, &out) == LKS_MINI_WRONG_ORDER); CHECK(out == NULL);
    CHECK(lks_mini_remove(order, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(allocation_calls == calls);
    invariant(order, (LksMiniHandle *[]){a,b,c,d}, 4); invariant(other, (LksMiniHandle *[]){foreign}, 1);
    /* Synthetic size only to exercise unreachable-in-practice overflow guard.
     * Restore it before checking the real structural invariants. */
    order->size = SIZE_MAX;
#define CAP_INSERT(expr) do { out = a; CHECK((expr) == LKS_MINI_CAPACITY_LIMIT); CHECK(out == NULL); CHECK(order->size == SIZE_MAX); CHECK(allocation_calls == calls); } while (0)
    CAP_INSERT(lks_mini_insert_front(order, NULL, &out));
    CAP_INSERT(lks_mini_insert_back(order, NULL, &out));
    CAP_INSERT(lks_mini_insert_before(order, c, NULL, &out));
    CAP_INSERT(lks_mini_insert_after(order, b, NULL, &out));
    out = a; CHECK(lks_mini_insert_before(order, foreign, NULL, &out) == LKS_MINI_WRONG_ORDER); CHECK(out == NULL);
    order->size = 4; invariant(order, (LksMiniHandle *[]){a,b,c,d}, 4);
    frees = free_calls;
    OK(lks_mini_remove(order, a)); invariant(order, (LksMiniHandle *[]){b,c,d}, 3);
    OK(lks_mini_remove(order, c)); invariant(order, (LksMiniHandle *[]){b,d}, 2);
    OK(lks_mini_remove(order, d)); invariant(order, (LksMiniHandle *[]){b}, 1);
    OK(lks_mini_remove(order, b)); invariant(order, NULL, 0);
    CHECK(allocation_calls == calls); CHECK(free_calls == frees + 4); CHECK(payload == 42);
    lks_mini_destroy(order); lks_mini_destroy(other); lks_mini_destroy(NULL);
    CHECK(live_allocations == 0);
    /* Destroy a nonempty order with duplicate stack payloads. */
    OK(lks_mini_create(&order));
    OK(lks_mini_insert_back(order, &payload, &a));
    OK(lks_mini_insert_back(order, &payload, &b));
    invariant(order, (LksMiniHandle *[]){a,b}, 2);
    lks_mini_destroy(order); CHECK(payload == 42); CHECK(live_allocations == 0);
    printf("internal invariants, OOM, capacity and lifetime: PASS (%zu allocation calls, %zu frees)\n", allocation_calls, free_calls);
    return 0;
}
