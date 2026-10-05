#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "V4 failure line %d: %s\n", \
    __LINE__, #c); exit(1); } } while (0)

typedef struct Occurrence { size_t id; } Occurrence;
typedef struct Reference {
    const LksOrderHandle **handles;
    Occurrence *items;
    size_t *sequence;
    size_t count, created;
} Reference;
static size_t max_records, max_local, max_index, splits, merges, redistributions;
static size_t left_rotations, right_rotations, lr_rotations, rl_rotations, two_child;
static size_t operation_count, oracle_steps;
static uint32_t random_step(uint32_t *state)
{ *state = *state * UINT32_C(1664525) + UINT32_C(1013904223); return *state; }

static void check_work(const LksOrder *order, int old_height)
{
    const LksOrderWork *w = &order->work;
    /* A split fixes B+1 locations; pair repair fixes at most 3B/2-1.
     * AVL changes traverse one ancestor path, rotations touch bounded neighbors;
     * 16(h+2) is a conservative diagnostic bound, not a latency promise. */
    CHECK(!w->tracking_overflow && w->records_reassigned <= 2u * LKS_ORDER_BLOCK_CAPACITY);
    CHECK(w->local_blocks <= 2 && w->splits <= 1);
    CHECK(w->merges + w->redistributions <= 1);
    CHECK(w->index_blocks <= 16u * ((size_t)old_height + 2u));
    CHECK(w->rotations <= 2u * ((size_t)old_height + 2u));
    if (w->records_reassigned > max_records) max_records = w->records_reassigned;
    if (w->local_blocks > max_local) max_local = w->local_blocks;
    if (w->index_blocks > max_index) max_index = w->index_blocks;
    splits += w->splits; merges += w->merges; redistributions += w->redistributions;
    left_rotations += w->left_rotations; right_rotations += w->right_rotations;
    lr_rotations += w->left_right_rotations; rl_rotations += w->right_left_rotations;
    two_child += w->two_child_detaches; ++operation_count;
}
static Reference reference_create(size_t max_occurrences)
{
    Reference model;
    memset(&model, 0, sizeof(model));
    model.handles = (const LksOrderHandle **)calloc(max_occurrences, sizeof(*model.handles));
    model.items = (Occurrence *)calloc(max_occurrences, sizeof(*model.items));
    model.sequence = (size_t *)calloc(max_occurrences, sizeof(*model.sequence));
    CHECK(model.handles && model.items && model.sequence);
    return model;
}
static void reference_destroy(Reference *model)
{ free(model->handles); free(model->items); free(model->sequence); }
static void verify(const LksOrder *order, const Reference *model, uint32_t *state)
{
    const LksOrderHandle *handle = lks_order_first(order);
    size_t i;
    CHECK(lks_order_internal_valid(order) && lks_order_size(order) == model->count);
    CHECK(handle == (model->count ? model->handles[model->sequence[0]] : NULL));
    for (i = 0; i < model->count; ++i) {
        size_t id = model->sequence[i];
        CHECK(handle == model->handles[id]);
        CHECK(lks_order_item(handle) == &model->items[id] && model->items[id].id == id);
        handle = lks_order_next(handle);
    }
    CHECK(!handle);
    handle = lks_order_last(order);
    for (i = model->count; i > 0; --i) {
        CHECK(handle == model->handles[model->sequence[i-1]]);
        handle = lks_order_previous(handle);
    }
    CHECK(!handle);
    for (i = 0; i < 48 && model->count; ++i) {
        size_t a = random_step(state) % model->count, b = random_step(state) % model->count;
        int comparison = 99;
        CHECK(lks_order_compare(order, model->handles[model->sequence[a]],
            model->handles[model->sequence[b]], &comparison) == LKS_STATUS_OK);
        CHECK(comparison == (a > b) - (a < b));
    }
}
/* Only the independent flat sequence predicts order. Internal ranks/blocks are
 * inspected for invariants/counters, never used to predict semantic placement. */
static void insert_model(LksOrder *order, Reference *model, size_t at, unsigned mode)
{
    size_t id = model->created++;
    int height = order->root ? order->root->height : 0;
    const LksOrderHandle *handle = NULL;
    LksStatus status;
    model->items[id].id = id;
    if (mode == 0) { CHECK(at == 0); status = lks_order_insert_front(order, &model->items[id], &handle); }
    else if (mode == 1) { CHECK(at == model->count); status = lks_order_insert_back(order, &model->items[id], &handle); }
    else if (mode == 2) {
        CHECK(at < model->count);
        status = lks_order_insert_before(order, model->handles[model->sequence[at]], &model->items[id], &handle);
    } else {
        CHECK(at > 0 && at <= model->count);
        status = lks_order_insert_after(order, model->handles[model->sequence[at-1]], &model->items[id], &handle);
    }
    CHECK(status == LKS_STATUS_OK && handle);
    model->handles[id] = handle;
    memmove(model->sequence + at + 1, model->sequence + at,
        (model->count - at) * sizeof(*model->sequence));
    model->sequence[at] = id; ++model->count;
    check_work(order, height);
}
static void remove_model(LksOrder *order, Reference *model, size_t at)
{
    size_t id = model->sequence[at];
    void *item = NULL;
    int height = order->root ? order->root->height : 0;
    CHECK(lks_order_remove(order, model->handles[id], &item) == LKS_STATUS_OK);
    CHECK(item == &model->items[id]); model->handles[id] = NULL;
    memmove(model->sequence + at, model->sequence + at + 1,
        (model->count - at - 1) * sizeof(*model->sequence));
    --model->count;
    check_work(order, height);
}
static void random_campaign(uint32_t seed)
{
    Reference model = reference_create(40001);
    LksOrder *order = lks_order_create();
    uint32_t state = seed;
    size_t step;
    CHECK(order);
    for (step = 0; step < 40000; ++step) {
        if (!model.count || (model.count < 1024 && random_step(&state) % 100 < 57)) {
            unsigned mode = random_step(&state) % 4;
            if (!model.count) mode = 0;
            size_t at = mode == 0 ? 0 : mode == 1 ? model.count :
                mode == 2 ? random_step(&state) % model.count : 1 + random_step(&state) % model.count;
            if (!model.count) { mode = 0; at = 0; }
            insert_model(order, &model, at, mode);
        } else remove_model(order, &model, random_step(&state) % model.count);
        CHECK(lks_order_internal_valid(order));
        if (step % 17 == 0) verify(order, &model, &state);
        ++oracle_steps;
    }
    verify(order, &model, &state);
    while (model.count) {
        remove_model(order, &model, random_step(&state) % model.count);
        CHECK(lks_order_internal_valid(order));
    }
    verify(order, &model, &state);
    lks_order_destroy(order); reference_destroy(&model);
    printf("random seed=%lu steps=40000 PASS\n", (unsigned long)seed);
}
static void adversarial(unsigned pattern, size_t n)
{
    LksOrder *order = lks_order_create();
    Reference model = reference_create(n);
    uint32_t state = 17;
    size_t i;
    CHECK(order);
    insert_model(order, &model, 0, 0);
    for (i = 1; i < n; ++i) {
        if (pattern == 0) insert_model(order, &model, model.count, 1);
        else if (pattern == 1) insert_model(order, &model, 0, 0);
        else if (pattern == 2) insert_model(order, &model, model.count-1, 2); /* same original anchor */
        else if (pattern == 3) insert_model(order, &model, 1, 3); /* same original anchor */
        else if (i & 1) insert_model(order, &model, 0, 0);
        else insert_model(order, &model, model.count, 1);
        if (i % 257 == 0) verify(order, &model, &state);
    }
    verify(order, &model, &state);
    /* Random removal exercises both repair directions and retains every surviving
     * handle. For the large endpoint case use front drain to keep oracle cheap. */
    while (model.count) {
        remove_model(order, &model, n >= 100000 ? model.count-1 : random_step(&state) % model.count);
        if (model.count % 257 == 0) verify(order, &model, &state);
    }
    verify(order, &model, &state);
    lks_order_destroy(order); reference_destroy(&model);
    printf("adversarial pattern=%u N=%lu PASS\n", pattern, (unsigned long)n);
}
static void boundaries_and_checker(void)
{
    int item = 1, comparison;
    LksOrder *order = lks_order_create(), *foreign = lks_order_create();
    const LksOrderHandle *handles[193], *other, *output;
    LksOrderBlock *block;
    size_t i;
    void *removed;
    CHECK(order && foreign && lks_order_internal_valid(order));
    CHECK(!lks_order_size(NULL) && !lks_order_first(NULL) && !lks_order_last(NULL));
    CHECK(!lks_order_next(NULL) && !lks_order_previous(NULL) && !lks_order_item(NULL));
    lks_order_destroy(NULL);
    CHECK(lks_order_insert_back(foreign, &item, &other) == LKS_STATUS_OK);
    output = other;
    CHECK(lks_order_insert_before(order, other, &item, &output) == LKS_STATUS_INVALID_ARGUMENT && !output);
    CHECK(lks_order_insert_after(order, NULL, &item, &output) == LKS_STATUS_INVALID_ARGUMENT && !output);
    CHECK(lks_order_insert_front(NULL, &item, &output) == LKS_STATUS_INVALID_ARGUMENT && !output);
    CHECK(lks_order_insert_back(order, NULL, &output) == LKS_STATUS_INVALID_ARGUMENT && !output);
    CHECK(lks_order_insert_back(order, &item, NULL) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_order_compare(order, other, other, &comparison) == LKS_STATUS_INVALID_ARGUMENT && comparison == 0);
    CHECK(lks_order_compare(order, other, other, NULL) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_order_remove(order, other, &removed) == LKS_STATUS_INVALID_ARGUMENT && !removed);
    for (i = 0; i < 128; ++i)
        CHECK(lks_order_insert_back(order, &item, &handles[i]) == LKS_STATUS_OK);
    CHECK(order->blocks == 1 && order->first->count == 128);
    CHECK(lks_order_insert_back(order, &item, &handles[128]) == LKS_STATUS_OK);
    CHECK(order->blocks == 2 && order->first->count == 64 && order->last->count == 65);
    CHECK(lks_order_remove(order, handles[0], NULL) == LKS_STATUS_OK);
    CHECK(order->work.merges == 1 && order->blocks == 1 && order->first->count == 128);
    for (i = 129; i < 193; ++i)
        CHECK(lks_order_insert_back(order, &item, &handles[i]) == LKS_STATUS_OK);
    CHECK(order->first->count == 64 && order->last->count == 128);
    CHECK(lks_order_remove(order, handles[1], NULL) == LKS_STATUS_OK);
    CHECK(order->work.redistributions == 1 && order->first->count == 95 && order->last->count == 96);
    CHECK(lks_order_internal_valid(order));
    /* Prove checker catches representative non-dangling corruption. */
    block = order->root;
    { LksOrderBlock *saved = block->left; block->left = block;
      CHECK(!lks_order_internal_valid(order)); block->left = saved; }
    { size_t saved = block->subtree_blocks; ++block->subtree_blocks;
      CHECK(!lks_order_internal_valid(order)); block->subtree_blocks = saved; }
    { LksOrderHandle *saved = block->records[1]; block->records[1] = block->records[0];
      CHECK(!lks_order_internal_valid(order)); block->records[1] = saved; }
    { LksOrderBlock *saved = block->next; block->next = block;
      CHECK(!lks_order_internal_valid(order)); block->next = saved; }
    CHECK(lks_order_internal_valid(order));
    order->revision = UINT64_MAX;
    CHECK(lks_order_insert_front(order, &item, &output) == LKS_STATUS_CAPACITY_LIMIT && !output);
    CHECK(lks_order_remove(order, lks_order_first(order), &removed) == LKS_STATUS_CAPACITY_LIMIT && !removed);
    CHECK(lks_order_internal_valid(order)); order->revision = 0;
    { size_t saved = order->count; order->count = SIZE_MAX;
      CHECK(lks_order_insert_front(order, &item, &output) == LKS_STATUS_CAPACITY_LIMIT && !output);
      order->count = saved; }
    CHECK(strcmp(lks_status_string(LKS_STATUS_CAPACITY_LIMIT), "Capacity limit reached") == 0);
    lks_order_destroy(foreign); lks_order_destroy(order);
    puts("occupancy boundaries, duplicate residences, invalid arguments, capacity and checker PASS");
}
static void failpoint_insert(size_t initial, size_t expected_failures, int back)
{
    size_t attempt, i;
    int item = 1;
    for (attempt = 1; ; ++attempt) {
        LksOrder *order = lks_order_create();
        const LksOrderHandle *output = NULL, *first;
        LksOrderBlock before;
        LksAllocStats stats;
        uint64_t revision;
        CHECK(order);
        for (i = 0; i < initial; ++i)
            CHECK(lks_order_insert_back(order, &item, &output) == LKS_STATUS_OK);
        first = lks_order_first(order);
        if (order->first) before = *(back ? order->last : order->first);
        stats = lks_alloc_stats_get(); revision = order->revision;
        lks_alloc_test_fail_on_attempt(attempt);
        { LksStatus status = back ? lks_order_insert_back(order, &item, &output) :
            lks_order_insert_front(order, &item, &output);
          lks_alloc_test_disable_failure();
          if (status == LKS_STATUS_OK) {
              CHECK(attempt == expected_failures + 1 && output);
              CHECK(lks_order_internal_valid(order));
              lks_order_destroy(order); break;
          }
          CHECK(status == LKS_STATUS_OUT_OF_MEMORY && !output);
        }
        CHECK(lks_order_size(order) == initial && order->revision == revision);
        CHECK(lks_order_first(order) == first && lks_order_internal_valid(order));
        CHECK(lks_alloc_stats_get().live_bytes == stats.live_bytes &&
            lks_alloc_stats_get().live_blocks == stats.live_blocks);
        if (order->first) CHECK(memcmp(&before, back ? order->last : order->first, sizeof(before)) == 0);
        lks_order_destroy(order);
    }
    printf("insert OOM initial=%lu failing points=%lu then success PASS\n",
        (unsigned long)initial, (unsigned long)expected_failures);
}
static void oom(void)
{
    LksOrder *order;
    const LksOrderHandle *handle;
    int item = 1;
    size_t i;
    lks_alloc_test_fail_on_attempt(1);
    CHECK(!lks_order_create()); lks_alloc_test_disable_failure();
    lks_alloc_test_fail_on_attempt(2);
    order = lks_order_create(); CHECK(order); lks_alloc_test_disable_failure();
    lks_order_destroy(order);
    failpoint_insert(0, 2, 0); failpoint_insert(1, 1, 0); failpoint_insert(128, 2, 0);
    failpoint_insert(4096, 2, 1);
    order = lks_order_create(); CHECK(order);
    for (i = 0; i < 4096; ++i) CHECK(lks_order_insert_back(order, &item, &handle) == LKS_STATUS_OK);
    lks_alloc_test_fail_on_attempt(1);
    { int comparison;
      CHECK(lks_order_compare(order, lks_order_first(order), lks_order_last(order),
          &comparison) == LKS_STATUS_OK && comparison < 0); }
    while ((handle = lks_order_first(order)) != NULL) {
        CHECK(lks_order_remove(order, handle, NULL) == LKS_STATUS_OK);
        CHECK(lks_order_internal_valid(order));
    }
    CHECK(lks_alloc_test_get_attempt_count() == 0 && !lks_alloc_test_failure_triggered());
    lks_alloc_test_disable_failure(); lks_order_destroy(order);
    puts("allocation-free removal including repairs/drain PASS");
}
static void two_child_deletion(void)
{
    LksOrder *order = lks_order_create();
    int item = 1;
    const LksOrderHandle *handle, *survivor;
    size_t i;
    CHECK(order);
    for (i = 0; i < 10000; ++i) CHECK(lks_order_insert_back(order, &item, &handle) == LKS_STATUS_OK);
    CHECK(order->root->left && order->root->right && order->root->previous);
    CHECK(order->root->count == 64 && order->root->previous->count == 64);
    survivor = order->root->records[0];
    handle = order->root->previous->records[0];
    CHECK(lks_order_remove(order, handle, NULL) == LKS_STATUS_OK);
    CHECK(order->work.two_child_detaches == 1 && order->work.merges == 1);
    CHECK(lks_order_item(survivor) == &item && lks_order_internal_valid(order));
    check_work(order, order->root->height + 1);
    lks_order_destroy(order);
}
int main(void)
{
    CHECK(lks_alloc_stats_reset() == 0);
    boundaries_and_checker(); oom(); two_child_deletion();
    random_campaign(17); random_campaign(UINT32_C(0x6a09e667)); random_campaign(UINT32_C(0xbb67ae85));
    adversarial(0, 100000);
    adversarial(1, 8192); adversarial(2, 8192); adversarial(3, 8192); adversarial(4, 8192);
    CHECK(left_rotations && right_rotations && lr_rotations && rl_rotations && two_child);
    CHECK(splits && merges && redistributions);
    CHECK(lks_alloc_stats_get().live_bytes == 0 && lks_alloc_stats_get().live_blocks == 0);
    printf("V4 counters operations=%lu oracle_steps=%lu max_record_locations=%lu max_local_blocks=%lu max_index_blocks=%lu\n",
        (unsigned long)operation_count, (unsigned long)oracle_steps, (unsigned long)max_records,
        (unsigned long)max_local, (unsigned long)max_index);
    printf("splits=%lu merges=%lu redistributions=%lu left=%lu right=%lu LR=%lu RL=%lu two_child=%lu\n",
        (unsigned long)splits, (unsigned long)merges, (unsigned long)redistributions,
        (unsigned long)left_rotations, (unsigned long)right_rotations,
        (unsigned long)lr_rotations, (unsigned long)rl_rotations, (unsigned long)two_child);
    puts("V4 Preview.1 live order PASS; final live allocations=0");
    return 0;
}
