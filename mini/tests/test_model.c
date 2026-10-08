#include "layerkeysort_mini.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { LIMIT = 48, ORDERS = 3, STEPS = 6000, OPS = 15 };
typedef struct Record {
    size_t id;
    LksMiniHandle *handle;
    void *item;
    size_t owner;
} Record;
typedef struct Model {
    LksMiniOrder *order;
    Record *records; /* Position is the array index, never a production link. */
    size_t count;
    size_t owner;
} Model;
static uint32_t seed, state;
static size_t step, next_id, id_a, id_b;
static int operation;
static int payloads[4] = {11,22,33,44};
static const char *names[OPS] = {
    "insert_front", "insert_back", "insert_before", "insert_after", "remove",
    "move_front", "move_back", "move_before", "move_after", "first", "last",
    "next", "prev", "item", "compare"
};

static void require(int condition, const char *expr, int line)
{
    if (!condition) {
        fprintf(stderr, "seed=0x%08lx step=%zu op=%s ids=%zu,%zu line=%d failed=%s\n",
                (unsigned long)seed, step, names[operation], id_a, id_b, line, expr);
        exit(1);
    }
}
#define CHECK(expr) require(!!(expr), #expr, __LINE__)
static void status_is(LksMiniStatus actual, LksMiniStatus expected)
{
    if (actual != expected) fprintf(stderr, "actual_status=%d expected_status=%d\n", (int)actual, (int)expected);
    CHECK(actual == expected);
}
static uint32_t random_u32(void)
{
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
static void init(Model *m, size_t owner)
{
    m->records = calloc(LIMIT, sizeof *m->records); CHECK(m->records != NULL);
    m->count = 0; m->owner = owner;
    status_is(lks_mini_create(&m->order), LKS_MINI_OK);
}
static void finish(Model *m)
{
    /* Invalidate the test registry before destruction; never inspect freed handles. */
    m->count = 0;
    memset(m->records, 0, LIMIT * sizeof *m->records);
    lks_mini_destroy(m->order); m->order = NULL;
    free(m->records); m->records = NULL;
}
static void erase(Model *m, size_t index)
{
    memmove(m->records + index, m->records + index + 1,
            (m->count - index - 1) * sizeof *m->records);
    --m->count;
    memset(m->records + m->count, 0, sizeof *m->records);
}
static void insert_record(Model *m, size_t index, Record r)
{
    CHECK(m->count < LIMIT);
    memmove(m->records + index + 1, m->records + index,
            (m->count - index) * sizeof *m->records);
    m->records[index] = r; ++m->count;
}
static void compare_pair(Model *m, size_t a, size_t b)
{
    int result = 9, reverse = 9;
    int expected = a < b ? -1 : (a == b ? 0 : 1);
    status_is(lks_mini_compare(m->order, m->records[a].handle, m->records[b].handle, &result), LKS_MINI_OK);
    status_is(lks_mini_compare(m->order, m->records[b].handle, m->records[a].handle, &reverse), LKS_MINI_OK);
    if (result != expected) fprintf(stderr, "pair_ids=%zu,%zu actual_result=%d expected_result=%d\n", m->records[a].id, m->records[b].id, result, expected);
    CHECK(result == expected); CHECK(reverse == -result);
}
static void verify(Model *m, int pairs)
{
    size_t count = SIZE_MAX;
    LksMiniHandle *cursor = NULL;
    status_is(lks_mini_size(m->order, &count), LKS_MINI_OK);
    if (count != m->count) fprintf(stderr, "order=%zu actual_size=%zu expected_size=%zu\n", m->owner, count, m->count);
    CHECK(count == m->count);
    status_is(lks_mini_first(m->order, &cursor), LKS_MINI_OK);
    for (size_t i = 0; i < m->count; ++i) {
        void *item = NULL;
        if (cursor != m->records[i].handle) fprintf(stderr, "order=%zu position=%zu expected_id=%zu actual_handle=%p expected_handle=%p\n", m->owner, i, m->records[i].id, (void *)cursor, (void *)m->records[i].handle);
        CHECK(cursor == m->records[i].handle); CHECK(m->records[i].owner == m->owner);
        status_is(lks_mini_item(m->order, cursor, &item), LKS_MINI_OK); CHECK(item == m->records[i].item);
        for (size_t j = 0; j < i; ++j) {
            CHECK(m->records[j].handle != cursor); CHECK(m->records[j].id != m->records[i].id);
        }
        status_is(lks_mini_next(m->order, cursor, &cursor), LKS_MINI_OK);
    }
    CHECK(cursor == NULL);
    status_is(lks_mini_last(m->order, &cursor), LKS_MINI_OK);
    for (size_t i = m->count; i > 0; --i) {
        CHECK(cursor == m->records[i - 1].handle);
        status_is(lks_mini_prev(m->order, cursor, &cursor), LKS_MINI_OK);
    }
    CHECK(cursor == NULL);
    if (!pairs || !m->count) return;
    if (m->count <= 8) {
        for (size_t a = 0; a < m->count; ++a)
            for (size_t b = 0; b < m->count; ++b) compare_pair(m, a, b);
    } else {
        for (size_t i = 0; i < 8; ++i) compare_pair(m, (step + i * 7) % m->count, (step * 3 + i * 11) % m->count);
    }
    if (m->count >= 3) {
        int ab, bc, ac;
        size_t b = m->count / 2, c = m->count - 1;
        status_is(lks_mini_compare(m->order, m->records[0].handle, m->records[b].handle, &ab), LKS_MINI_OK);
        status_is(lks_mini_compare(m->order, m->records[b].handle, m->records[c].handle, &bc), LKS_MINI_OK);
        status_is(lks_mini_compare(m->order, m->records[0].handle, m->records[c].handle, &ac), LKS_MINI_OK);
        CHECK(ab == -1 && bc == -1 && ac == -1);
    }
}

static void execute(Model *m, int op, size_t a, size_t b, void *item)
{
    size_t n = m->count, destination = 0;
    LksMiniHandle *ha = n ? m->records[a].handle : NULL;
    LksMiniHandle *hb = n ? m->records[b].handle : NULL;
    LksMiniHandle *out = ha, *expected_handle = NULL;
    LksMiniStatus actual, expected = LKS_MINI_OK;
    void *out_item = item;
    int result = 9;
    Record moved;
    operation = op; id_a = n ? m->records[a].id : 0; id_b = n ? m->records[b].id : 0;
    if (!n && op != 0 && op != 1 && op != 9 && op != 10) expected = LKS_MINI_INVALID_ARGUMENT;
    switch (op) {
    case 0: destination = 0; actual = lks_mini_insert_front(m->order, item, &out); break;
    case 1: destination = n; actual = lks_mini_insert_back(m->order, item, &out); break;
    case 2: destination = a; actual = lks_mini_insert_before(m->order, ha, item, &out); break;
    case 3: destination = a + 1; actual = lks_mini_insert_after(m->order, ha, item, &out); break;
    case 4: actual = lks_mini_remove(m->order, ha); break;
    case 5: destination = 0; actual = lks_mini_move_front(m->order, ha); break;
    case 6: destination = n ? n - 1 : 0; actual = lks_mini_move_back(m->order, ha); break;
    case 7: destination = b - (a < b ? 1u : 0u); actual = lks_mini_move_before(m->order, ha, hb); break;
    case 8: destination = b + 1 - (a < b ? 1u : 0u); actual = lks_mini_move_after(m->order, ha, hb); break;
    case 9: expected_handle = n ? m->records[0].handle : NULL; actual = lks_mini_first(m->order, &out); break;
    case 10: expected_handle = n ? m->records[n-1].handle : NULL; actual = lks_mini_last(m->order, &out); break;
    case 11: expected_handle = n && a + 1 < n ? m->records[a+1].handle : NULL; actual = lks_mini_next(m->order, ha, &out); break;
    case 12: expected_handle = n && a > 0 ? m->records[a-1].handle : NULL; actual = lks_mini_prev(m->order, ha, &out); break;
    case 13: actual = lks_mini_item(m->order, ha, &out_item); break;
    default: actual = lks_mini_compare(m->order, ha, hb, &result); break;
    }
    status_is(actual, expected);
    if (expected != LKS_MINI_OK) {
        if (op <= 3 || op == 11 || op == 12) CHECK(out == NULL);
        if (op == 13) CHECK(out_item == NULL);
        if (op == 14) CHECK(result == 0);
    } else if (op <= 3) {
        Record r = {++next_id, out, item, m->owner};
        CHECK(out != NULL);
        for (size_t i = 0; i < n; ++i) CHECK(out != m->records[i].handle);
        insert_record(m, destination, r);
    } else if (op == 4) {
        /* Do not evaluate ha again after removal. Erase its registry entry. */
        erase(m, a);
    } else if (op <= 8) {
        if (a != b || op <= 6) {
            moved = m->records[a]; erase(m, a); insert_record(m, destination, moved);
        }
    } else if (op <= 12) CHECK(out == expected_handle);
    else if (op == 13) CHECK(out_item == m->records[a].item);
    else CHECK(result == (a < b ? -1 : (a == b ? 0 : 1)));
    verify(m, step % 31 == 0);
}

static void adversarial(void)
{
    Model m;
    seed = 0; step = 0; operation = 0; init(&m, 0);
    /* Grow with repeated head/tail and alternating insertion, then move one
     * occurrence repeatedly across the whole collection, including no-ops. */
    for (int pattern = 0; pattern < 3; ++pattern) {
        for (size_t i = 0; i < 24; ++i) { ++step; execute(&m, pattern == 0 ? 0 : (pattern == 1 ? 1 : (int)(i % 2)), 0, 0, i % 3 ? &payloads[0] : NULL); }
        for (size_t i = 0; i < 40; ++i) {
            ++step; execute(&m, 5, m.count-1, 0, NULL);
            ++step; execute(&m, 6, 0, 0, NULL);
            ++step; execute(&m, 7, m.count-1, 0, NULL);
            ++step; execute(&m, 8, 0, m.count-1, NULL);
            ++step; execute(&m, 7, 2, 2, NULL);
            ++step; execute(&m, 8, 2, 2, NULL);
            ++step; execute(&m, 7, 2, 3, NULL);
            ++step; execute(&m, 8, 3, 2, NULL);
        }
        while (m.count) { ++step; execute(&m, 4, m.count / 2, 0, NULL); }
    }
    verify(&m, 1); finish(&m);
    puts("model adversarial: PASS (head/tail/alternating growth, repeated moves, self/adjacent no-ops, drain)");
}

static void campaign(uint32_t initial)
{
    Model models[ORDERS];
    size_t counts[OPS] = {0}, max_count = 0;
    seed = initial; state = initial; step = 0; operation = 0;
    for (size_t i = 0; i < ORDERS; ++i) init(&models[i], i);
    for (step = 0; step < STEPS; ++step) {
        Model *m = &models[random_u32() % ORDERS];
        int op = (int)(step % OPS);
        size_t a = m->count ? random_u32() % m->count : 0;
        size_t b = m->count ? random_u32() % m->count : 0;
        uint32_t choice = random_u32() % 5;
        if (op < 4 && m->count == LIMIT) op = 4;
        if ((op == 7 || op == 8) && step % 3 == 0) b = a;
        execute(m, op, a, b, choice < 4 ? &payloads[choice] : NULL); ++counts[op];
        if (m->count > max_count) max_count = m->count;
        /* Verify untouched orders too; uniqueness uses only live registries. */
        for (size_t i = 0; i < ORDERS; ++i) {
            verify(&models[i], 0);
            for (size_t j = 0; j < models[i].count; ++j)
                for (size_t k = i + 1; k < ORDERS; ++k)
                    for (size_t t = 0; t < models[k].count; ++t)
                        CHECK(models[i].records[j].handle != models[k].records[t].handle);
        }
    }
    for (size_t i = 0; i < ORDERS; ++i) { verify(&models[i], 1); finish(&models[i]); }
    printf("model seed=0x%08lx operations=%d orders=%d active_range=0..%zu per_order: PASS\n", (unsigned long)seed, STEPS, ORDERS, max_count);
    for (int op = 0; op < OPS; ++op) { CHECK(counts[op] > 0); printf(" %s=%zu", names[op], counts[op]); }
    puts("");
}

int main(void)
{
    static const uint32_t seeds[] = {UINT32_C(0x12345678), UINT32_C(0x9e3779b9), UINT32_C(0xc0ffee01), UINT32_C(0xdeadbeef)};
    adversarial();
    for (size_t i = 0; i < sizeof seeds / sizeof seeds[0]; ++i) campaign(seeds[i]);
    return 0;
}
