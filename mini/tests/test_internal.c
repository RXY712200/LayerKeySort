#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define OK(expr) CHECK((expr) == LKS_MINI_OK)
static size_t live_allocations;
static size_t allocation_calls;
static size_t free_calls;
static int fail_next;
static size_t fail_at;
static void *allocated[256]; /* Test-only live allocation ledger. */
static void *test_malloc(size_t size)
{
    void *p;
    ++allocation_calls;
    if (fail_next || allocation_calls == fail_at) { fail_next = 0; return NULL; }
    p = malloc(size);
    if (p) {
        size_t i;
        for (i = 0; i < 256 && allocated[i]; ++i) {}
        CHECK(i < 256); allocated[i] = p; ++live_allocations;
    }
    return p;
}
static void test_free(void *p)
{
    if (p) {
        size_t i;
        for (i = 0; i < 256 && allocated[i] != p; ++i) {}
        CHECK(i < 256); /* Reject freeing application memory or double-free. */
        allocated[i] = NULL;
        CHECK(live_allocations > 0); --live_allocations; ++free_calls;
    }
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
    for (size_t i = 0; i < count; ++i)
        for (size_t j = 0; j < i; ++j) CHECK(expected[i] != expected[j]);
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
    while (p) {
        CHECK(n > 0); CHECK(p == expected[--n]);
        if (p->prev) CHECK(p->prev->next == p);
        p = p->prev;
    }
    CHECK(n == 0);
}

static void movement_checks(void)
{
    LksMiniOrder *order = NULL, *other = NULL;
    LksMiniHandle *a = NULL, *b = NULL, *c = NULL, *foreign = NULL;
    LksMiniHandle *identities[3];
    void *items[3];
    int value = 5, result = 9;
    size_t calls, frees;
    OK(lks_mini_create(&order)); OK(lks_mini_create(&other));
    OK(lks_mini_insert_back(order, &value, &a));
    calls = allocation_calls; frees = free_calls;
    OK(lks_mini_move_front(order, a)); OK(lks_mini_move_back(order, a));
    OK(lks_mini_move_before(order, a, a)); OK(lks_mini_move_after(order, a, a));
    invariant(order, (LksMiniHandle *[]){a}, 1);
    CHECK(calls == allocation_calls && frees == free_calls);
    OK(lks_mini_insert_back(order, &value, &b)); OK(lks_mini_insert_back(order, NULL, &c));
    OK(lks_mini_insert_back(other, NULL, &foreign));
    identities[0] = a; identities[1] = b; identities[2] = c;
    for (size_t i = 0; i < 3; ++i) items[i] = identities[i]->item;
    calls = allocation_calls; frees = free_calls;
#define MOVED(expr, ...) do { OK(expr); invariant(order, (LksMiniHandle *[]){__VA_ARGS__}, 3); for (size_t i = 0; i < 3; ++i) { CHECK(identities[i]->owner == order); CHECK(identities[i]->item == items[i]); } CHECK(calls == allocation_calls && frees == free_calls); } while (0)
    MOVED(lks_mini_move_front(order, c), c,a,b);
    MOVED(lks_mini_move_back(order, c), a,b,c);
    MOVED(lks_mini_move_front(order, b), b,a,c);
    MOVED(lks_mini_move_back(order, a), b,c,a);
    MOVED(lks_mini_move_before(order, a, b), a,b,c);
    MOVED(lks_mini_move_after(order, a, c), b,c,a);
    MOVED(lks_mini_move_before(order, c, b), c,b,a);
    MOVED(lks_mini_move_after(order, c, b), b,c,a);
    MOVED(lks_mini_move_front(order, b), b,c,a);
    MOVED(lks_mini_move_back(order, a), b,c,a);
    MOVED(lks_mini_move_before(order, c, c), b,c,a);
    MOVED(lks_mini_move_after(order, c, c), b,c,a);
    MOVED(lks_mini_move_before(order, b, c), b,c,a);
    MOVED(lks_mini_move_after(order, a, c), b,c,a);
    OK(lks_mini_compare(order, b, a, &result)); CHECK(result == -1);
    OK(lks_mini_compare(order, a, b, &result)); CHECK(result == 1);
    OK(lks_mini_compare(order, c, c, &result)); CHECK(result == 0);
    CHECK(lks_mini_move_front(order, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_back(order, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_before(order, b, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_after(order, foreign, b) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_before(order, foreign, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_after(order, foreign, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_move_front(NULL, b) == LKS_MINI_INVALID_ARGUMENT);
    CHECK(lks_mini_move_back(order, NULL) == LKS_MINI_INVALID_ARGUMENT);
    CHECK(lks_mini_move_before(order, b, NULL) == LKS_MINI_INVALID_ARGUMENT);
    CHECK(lks_mini_move_after(order, NULL, b) == LKS_MINI_INVALID_ARGUMENT);
    result = 9; CHECK(lks_mini_compare(order, b, foreign, &result) == LKS_MINI_WRONG_ORDER); CHECK(result == 0);
    result = 9; CHECK(lks_mini_compare(order, foreign, b, &result) == LKS_MINI_WRONG_ORDER); CHECK(result == 0);
    result = 9; CHECK(lks_mini_compare(order, NULL, b, &result) == LKS_MINI_INVALID_ARGUMENT); CHECK(result == 0);
    CHECK(lks_mini_compare(order, b, a, NULL) == LKS_MINI_INVALID_ARGUMENT);
    invariant(order, (LksMiniHandle *[]){b,c,a}, 3);
    invariant(other, (LksMiniHandle *[]){foreign}, 1);
    for (size_t i = 0; i < 3; ++i) { CHECK(identities[i]->owner == order); CHECK(identities[i]->item == items[i]); }
    CHECK(calls == allocation_calls && frees == free_calls);
    lks_mini_destroy(order); lks_mini_destroy(other); CHECK(live_allocations == 0);
    puts("internal movement/comparison invariants and zero allocation/free deltas: PASS");
}

typedef struct Snapshot {
    LksMiniOrder *order;
    LksMiniHandle *head, *tail, *handles[4], *prev[4], *next[4];
    void *items[4];
    size_t size;
} Snapshot;

static Snapshot snapshot(LksMiniOrder *order)
{
    Snapshot s = {0};
    LksMiniHandle *p = order->head;
    s.order = order; s.head = order->head; s.tail = order->tail; s.size = order->size;
    CHECK(s.size <= 4);
    for (size_t i = 0; i < s.size; ++i) {
        CHECK(p != NULL); s.handles[i] = p; s.prev[i] = p->prev;
        s.next[i] = p->next; s.items[i] = p->item; p = p->next;
    }
    CHECK(p == NULL); return s;
}
static void unchanged(const Snapshot *s)
{
    CHECK(s->order->head == s->head && s->order->tail == s->tail && s->order->size == s->size);
    invariant(s->order, (LksMiniHandle **)s->handles, s->size);
    for (size_t i = 0; i < s->size; ++i) {
        CHECK(s->handles[i]->prev == s->prev[i]); CHECK(s->handles[i]->next == s->next[i]);
        CHECK(s->handles[i]->item == s->items[i]); CHECK(s->handles[i]->owner == s->order);
    }
}

static LksMiniStatus insert_variant(int variant, LksMiniOrder *order,
                                    LksMiniHandle *anchor, void *item, LksMiniHandle **out)
{
    if (variant == 0) return lks_mini_insert_front(order, item, out);
    if (variant == 1) return lks_mini_insert_back(order, item, out);
    if (variant == 2) return lks_mini_insert_before(order, anchor, item, out);
    return lks_mini_insert_after(order, anchor, item, out);
}

static void complex_structure(void)
{
    LksMiniOrder *order = NULL;
    LksMiniHandle *h[6];
    int value = 77;
    OK(lks_mini_create(&order));
    for (size_t i = 0; i < 6; ++i) OK(lks_mini_insert_back(order, i % 2 ? NULL : &value, &h[i]));
    size_t calls = allocation_calls, frees = free_calls;
#define STRUCT_MOVE(expr, ...) do { OK(expr); invariant(order, (LksMiniHandle *[]){__VA_ARGS__}, 6); for (size_t i = 0; i < 6; ++i) { CHECK(h[i]->owner == order); CHECK(h[i]->item == (i % 2 ? NULL : &value)); } CHECK(allocation_calls == calls && free_calls == frees); } while (0)
    STRUCT_MOVE(lks_mini_move_after(order, h[0], h[5]), h[1],h[2],h[3],h[4],h[5],h[0]);
    STRUCT_MOVE(lks_mini_move_before(order, h[4], h[1]), h[4],h[1],h[2],h[3],h[5],h[0]);
    STRUCT_MOVE(lks_mini_move_after(order, h[1], h[3]), h[4],h[2],h[3],h[1],h[5],h[0]);
    STRUCT_MOVE(lks_mini_move_before(order, h[5], h[2]), h[4],h[5],h[2],h[3],h[1],h[0]);
    STRUCT_MOVE(lks_mini_move_front(order, h[3]), h[3],h[4],h[5],h[2],h[1],h[0]);
    STRUCT_MOVE(lks_mini_move_back(order, h[5]), h[3],h[4],h[2],h[1],h[0],h[5]);
    STRUCT_MOVE(lks_mini_move_before(order, h[4], h[2]), h[3],h[4],h[2],h[1],h[0],h[5]);
    STRUCT_MOVE(lks_mini_move_after(order, h[1], h[2]), h[3],h[4],h[2],h[1],h[0],h[5]);
    lks_mini_destroy(order); CHECK(live_allocations == 0);
    puts("private six-node complex movement patterns: PASS");
}

static void fault_campaign(void)
{
    size_t cases = 0;
    int *payload = malloc(sizeof *payload); CHECK(payload != NULL); *payload = 901;
    for (size_t n = 0; n <= 3; ++n) {
        LksMiniOrder *order = NULL, *out_order;
        OK(lks_mini_create(&order));
        for (size_t i = 0; i < n; ++i) {
            LksMiniHandle *h; OK(lks_mini_insert_back(order, i % 2 ? NULL : payload, &h));
        }
        {
            Snapshot s = snapshot(order);
            size_t live = live_allocations, calls = allocation_calls, frees = free_calls;
            out_order = order; fail_at = calls + 1;
            CHECK(lks_mini_create(&out_order) == LKS_MINI_OUT_OF_MEMORY);
            CHECK(out_order == NULL); CHECK(allocation_calls == calls + 1);
            CHECK(live_allocations == live && free_calls == frees); unchanged(&s);
            fail_at = 0; OK(lks_mini_create(&out_order)); lks_mini_destroy(out_order);
            unchanged(&s); CHECK(live_allocations == live); ++cases;
        }
        for (int v = 0; v < 4; ++v) {
            size_t anchors = v < 2 ? 1 : n;
            for (size_t index = 0; index < anchors; ++index) {
                Snapshot s = snapshot(order);
                LksMiniHandle *anchor = n ? s.handles[index] : NULL, *out = anchor;
                size_t live = live_allocations, calls = allocation_calls, frees = free_calls;
                fail_at = calls + 1;
                CHECK(insert_variant(v, order, anchor, payload, &out) == LKS_MINI_OUT_OF_MEMORY);
                CHECK(out == NULL && allocation_calls == calls + 1);
                CHECK(live_allocations == live && free_calls == frees); unchanged(&s);
                fail_at = 0; OK(insert_variant(v, order, anchor, payload, &out));
                CHECK(out != NULL && out->item == payload && out->owner == order);
                CHECK(order->size == n + 1 && live_allocations == live + 1);
                {
                    LksMiniHandle *expected[4];
                    size_t position = v == 0 ? 0 : (v == 1 ? n : index + (v == 3 ? 1u : 0u));
                    for (size_t i = 0, j = 0; i < n + 1; ++i) expected[i] = i == position ? out : s.handles[j++];
                    invariant(order, expected, n + 1);
                }
                calls = allocation_calls; OK(lks_mini_remove(order, out));
                CHECK(allocation_calls == calls); unchanged(&s); CHECK(live_allocations == live); ++cases;
            }
        }
        lks_mini_destroy(order); CHECK(live_allocations == 0); CHECK(*payload == 901);
    }
    free(payload);
    printf("systematic single-allocation failure/recovery: PASS (%zu cases; empty and 1..3 nodes, every anchor)\n", cases);
}

static void error_matrix(void)
{
    LksMiniOrder *order = NULL, *other = NULL;
    LksMiniHandle *a = NULL, *b = NULL, *f = NULL;
    int payload = 37;
    size_t cases = 0;
    OK(lks_mini_create(&order)); OK(lks_mini_create(&other));
    OK(lks_mini_insert_back(order, &payload, &a)); OK(lks_mini_insert_back(order, NULL, &b));
    OK(lks_mini_insert_back(other, &payload, &f));
    Snapshot s = snapshot(order), t = snapshot(other);
    size_t calls = allocation_calls, frees = free_calls;
    CHECK(lks_mini_create(NULL) == LKS_MINI_INVALID_ARGUMENT); ++cases;
    lks_mini_destroy(NULL); ++cases;
    for (int container = 0; container < 2; ++container) {
        LksMiniOrder *o = container ? order : NULL;
        for (int output = 0; output < 2; ++output) {
            size_t size = 99;
            LksMiniHandle *out = a;
            LksMiniStatus want = container && output ? LKS_MINI_OK : LKS_MINI_INVALID_ARGUMENT;
            CHECK(lks_mini_size(o, output ? &size : NULL) == want); CHECK(!output || size == (want == LKS_MINI_OK ? 2u : 0u)); ++cases;
            CHECK(lks_mini_first(o, output ? &out : NULL) == want); CHECK(!output || out == (want == LKS_MINI_OK ? a : NULL)); ++cases;
            out = a; CHECK(lks_mini_last(o, output ? &out : NULL) == want); CHECK(!output || out == (want == LKS_MINI_OK ? b : NULL)); ++cases;
            for (int operand = 0; operand < 3; ++operand) {
                LksMiniHandle *h = operand == 0 ? NULL : (operand == 1 ? a : f);
                want = !container || !output || !h ? LKS_MINI_INVALID_ARGUMENT : (operand == 2 ? LKS_MINI_WRONG_ORDER : LKS_MINI_OK);
                out = a; CHECK(lks_mini_next(o, h, output ? &out : NULL) == want); CHECK(!output || out == (want == LKS_MINI_OK ? b : NULL)); ++cases;
                out = a; CHECK(lks_mini_prev(o, h, output ? &out : NULL) == want); CHECK(!output || out == NULL); ++cases;
                void *item = &payload;
                CHECK(lks_mini_item(o, h, output ? &item : NULL) == want); CHECK(!output || item == (want == LKS_MINI_OK ? &payload : NULL)); ++cases;
                for (int v = 0; v < 4; ++v) {
                    want = !container || !output || (v >= 2 && !h) ? LKS_MINI_INVALID_ARGUMENT : (v >= 2 && operand == 2 ? LKS_MINI_WRONG_ORDER : LKS_MINI_OK);
                    if (want != LKS_MINI_OK) {
                        out = a; CHECK(insert_variant(v, o, h, NULL, output ? &out : NULL) == want); CHECK(!output || out == NULL); ++cases;
                    }
                }
                for (int second = 0; second < 3; ++second) {
                    LksMiniHandle *anchor = second == 0 ? NULL : (second == 1 ? a : f);
                    int result = 99;
                    want = !container || !output || !h || !anchor ? LKS_MINI_INVALID_ARGUMENT : (operand == 2 || second == 2 ? LKS_MINI_WRONG_ORDER : LKS_MINI_OK);
                    CHECK(lks_mini_compare(o, h, anchor, output ? &result : NULL) == want); CHECK(!output || result == 0); ++cases;
                    unchanged(&s); unchanged(&t);
                }
                unchanged(&s); unchanged(&t);
            }
        }
        for (int operand = 0; operand < 3; ++operand) {
            LksMiniHandle *h = operand == 0 ? NULL : (operand == 1 ? a : f);
            LksMiniStatus want = !o || !h ? LKS_MINI_INVALID_ARGUMENT : (operand == 2 ? LKS_MINI_WRONG_ORDER : LKS_MINI_OK);
            if (want != LKS_MINI_OK) { CHECK(lks_mini_remove(o, h) == want); ++cases; }
            CHECK(lks_mini_move_front(o, h) == want); ++cases;
            if (want != LKS_MINI_OK) { CHECK(lks_mini_move_back(o, h) == want); ++cases; }
            for (int second = 0; second < 3; ++second) {
                LksMiniHandle *anchor = second == 0 ? NULL : (second == 1 ? a : f);
                want = !o || !h || !anchor ? LKS_MINI_INVALID_ARGUMENT : (operand == 2 || second == 2 ? LKS_MINI_WRONG_ORDER : LKS_MINI_OK);
                CHECK(lks_mini_move_before(o, h, anchor) == want); ++cases;
                CHECK(lks_mini_move_after(o, h, anchor) == want); ++cases;
                unchanged(&s); unchanged(&t);
            }
            unchanged(&s); unchanged(&t);
        }
    }
    /* Synthetic capacity: missing/foreign arguments must beat capacity/OOM.
     * Restore the real count before each structural snapshot check. */
    for (int v = 0; v < 4; ++v) {
        LksMiniHandle *out = a;
        order->size = SIZE_MAX; fail_at = allocation_calls + 1;
        CHECK(insert_variant(v, order, a, NULL, &out) == LKS_MINI_CAPACITY_LIMIT); CHECK(out == NULL); ++cases;
        CHECK(insert_variant(v, order, a, NULL, NULL) == LKS_MINI_INVALID_ARGUMENT); ++cases;
        if (v >= 2) {
            out = a; CHECK(insert_variant(v, order, f, NULL, &out) == LKS_MINI_WRONG_ORDER); CHECK(out == NULL); ++cases;
            out = a; CHECK(insert_variant(v, order, NULL, NULL, &out) == LKS_MINI_INVALID_ARGUMENT); CHECK(out == NULL); ++cases;
        }
        CHECK(allocation_calls == calls && free_calls == frees);
        order->size = 2; fail_at = 0; unchanged(&s); unchanged(&t);
    }
    CHECK(allocation_calls == calls && free_calls == frees);
    lks_mini_destroy(order); lks_mini_destroy(other); CHECK(live_allocations == 0);
    printf("systematic legal-input error/default/precedence matrix: PASS (%zu cases, no allocation/free)\n", cases);
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
    movement_checks();
    complex_structure(); fault_campaign(); error_matrix();
    return 0;
}
