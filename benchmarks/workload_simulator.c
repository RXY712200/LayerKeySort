#include "lks_legacy_internal.h"
/* Synthetic application-like evidence, not user traces. Production and
 * diagnostic builds execute the same trace; only the latter times instrumented
 * library calls. The independent flat model and profiling stay outside timers. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "layerkeysort.h"
#include "lks_tree_internal.h"
#include "lks_path_internal.h"
#ifdef LKS_WORKLOAD_DIAGNOSTICS
#include "lks_alloc_internal.h"
#endif

#define KEY_GAP INT64_C(1048576)
#define SAMPLE_COUNT 25u
#define FIRST_FULL_COUNT 5u

typedef struct Item {
    int64_t key;
    size_t id, dense_index;
    uint64_t sequence;
    int active;
} Item;
typedef struct Event {
    size_t operation, resident, relabelled, attempted, attempts, expansions;
    size_t generated, search_steps, rotations, depth;
    uint64_t comparisons;
    double ms;
    int full, category, sample;
} Event;
typedef struct Simulation {
    const char *workload;
    uint32_t seed, random;
    size_t initial, operations, checkpoint, capacity, count, next_id, operation;
    size_t insertions, removals, updates;
    uint64_t sequence, digest, comparisons;
    int64_t low_water, high_water;
    Item *items, **active, **expected;
    const LksTreeNode **nodes;
    Event *events;
    LksOrderedTree *tree;
    int trace;
} Simulation;

static void fail(const Simulation *s, const char *message)
{
    fprintf(stderr, "FAIL workload=%s seed=%" PRIu32 " initial=%zu operations=%zu"
        " checkpoint=%zu operation=%zu: %s\n", s->workload, s->seed,
        s->initial, s->operations, s->checkpoint, s->operation, message);
    exit(1);
}
static void require(const Simulation *s, int condition, const char *message)
{ if (!condition) fail(s, message); }
static uint32_t next_random(Simulation *s)
{
    uint32_t x = s->random;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    s->random = x; return x;
}
static uint64_t hash_word(uint64_t hash, uint64_t word)
{
    unsigned int i;
    for (i = 0; i < 8; ++i) {
        hash ^= word & 255u; hash *= UINT64_C(1099511628211); word >>= 8;
    }
    return hash;
}
static int compare_item(const void *a, const void *b, void *context)
{
    const Item *x = (const Item *)a, *y = (const Item *)b;
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    Simulation *s = (Simulation *)context;
    ++s->comparisons;
#else
    (void)context;
#endif
    return (x->key > y->key) - (x->key < y->key);
}
static int reference_order(const void *a, const void *b)
{
    const Item *x = *(Item *const *)a, *y = *(Item *const *)b;
    if (x->key != y->key) return (x->key > y->key) - (x->key < y->key);
    return (x->sequence > y->sequence) - (x->sequence < y->sequence);
}
static int size_order(const void *a, const void *b)
{
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}
static int time_order(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static int event_latency_order(const void *a, const void *b)
{
    const Event *x = *(Event *const *)a, *y = *(Event *const *)b;
    if (x->ms != y->ms) return (x->ms > y->ms) - (x->ms < y->ms);
    return (x->operation > y->operation) - (x->operation < y->operation);
}
static int event_region_order(const void *a, const void *b)
{
    const Event *x = *(Event *const *)a, *y = *(Event *const *)b;
    if (x->relabelled != y->relabelled)
        return (x->relabelled > y->relabelled) - (x->relabelled < y->relabelled);
    return event_latency_order(a, b);
}
/* Same wall-clock method as current_benchmark.c. Negative intervals are an
 * invalid measurement, not zero latency; report the exact reproducible run. */
static double now_ms(void)
{
    struct timespec t;
    if (timespec_get(&t, TIME_UTC) != TIME_UTC) abort();
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}
static size_t quantile_index(size_t n, size_t permille)
{
    size_t rank = (n / 1000) * permille + ((n % 1000) * permille + 999) / 1000;
    return rank == 0 ? 0 : rank - 1;
}
static void *array(const Simulation *s, size_t count, size_t width)
{
    void *p;
    require(s, count != 0 && count <= SIZE_MAX / width, "array size overflow");
    p = calloc(count, width); require(s, p != NULL, "simulator allocation failed");
    return p;
}
static int64_t shifted(const Simulation *s, int64_t value, int64_t delta)
{
    require(s, !(delta > 0 && value > INT64_MAX - delta) &&
        !(delta < 0 && value < INT64_MIN - delta), "synthetic key overflow");
    return value + delta;
}
static void track_key(Simulation *s, int64_t key)
{
    if (key < s->low_water) s->low_water = key;
    if (key > s->high_water) s->high_water = key;
}
static int diagnostic_build(void)
{
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    return 1;
#else
    return 0;
#endif
}
static int control(const char *name)
{
    return !strcmp(name, "ascending") || !strcmp(name, "descending") ||
        !strcmp(name, "random") || !strcmp(name, "equal") ||
        !strcmp(name, "duplicates") || !strcmp(name, "alternating") ||
        !strcmp(name, "hotspot");
}

/* All borrowed observations are reacquired within each operation. An equal-key
 * query intentionally selects the oldest resident member of that key bucket;
 * the workload is key-query based, not an application ID lookup facade. */
static Item *remove_query(Simulation *s, int64_t key)
{
    Item query = {0}, *item, *moved;
    const LksTreeNode *left, *equal, *right;
    void *removed = NULL;
    size_t index;
    query.key = key;
    require(s, lks_ordered_tree_locate(s->tree, &query, &left, &equal, &right)
        == LKS_STATUS_OK && equal != NULL, "remove query not found");
    item = (Item *)lks_tree_node_item(equal);
    require(s, item->active && item->key == key &&
        item->id < s->next_id && item == &s->items[item->id], "remove identity");
    require(s, lks_ordered_tree_remove_path(s->tree, lks_tree_node_path(equal),
        &removed) == LKS_STATUS_OK && removed == item, "exact removal failed");
    index = item->dense_index;
    moved = s->active[--s->count];
    s->active[index] = moved; moved->dense_index = index;
    item->active = 0;
    s->digest = hash_word(s->digest, 2);
    s->digest = hash_word(s->digest, item->id);
    return item;
}
static void insert_item(Simulation *s, Item *item, int64_t key, int measured)
{
    const LksTreeNode *node = NULL;
    LksStatus status;
    double begin = 0.0, elapsed = 0.0;
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    LksTreeRepairStats before = lks_tree_repair_stats_get(), after;
    uint64_t calls = s->comparisons;
#endif
    require(s, !item->active, "inserted resident item");
    item->key = key; item->sequence = ++s->sequence;
    if (measured) begin = now_ms();
    status = lks_ordered_tree_insert(s->tree, item, &node);
    if (measured) elapsed = now_ms() - begin;
    require(s, status == LKS_STATUS_OK && node != NULL, "managed insertion failed");
    require(s, elapsed >= 0.0, "wall clock went backwards");
    item->active = 1; item->dense_index = s->count;
    s->active[s->count++] = item; track_key(s, key);
    s->digest = hash_word(s->digest, 1);
    s->digest = hash_word(s->digest, item->id);
    s->digest = hash_word(s->digest, (uint64_t)key);
    if (measured) {
        Event *e = &s->events[s->insertions++];
        e->operation = s->operation; e->resident = s->count - 1; e->ms = elapsed;
        e->depth = lks_path_depth(lks_tree_node_path(node));
#ifdef LKS_WORKLOAD_DIAGNOSTICS
        after = lks_tree_repair_stats_get();
        e->relabelled = after.nodes_relabelled - before.nodes_relabelled;
        e->attempted = after.attempted_region_nodes - before.attempted_region_nodes;
        e->attempts = after.attempts - before.attempts;
        e->expansions = after.region_expansions - before.region_expansions;
        e->generated = after.generated_relabel_paths - before.generated_relabel_paths;
        e->search_steps = after.comparator_search_steps - before.comparator_search_steps;
        e->rotations = after.rotations - before.rotations;
        e->comparisons = s->comparisons - calls;
        e->full = after.full_range_relabels != before.full_range_relabels;
        e->category = e->full ? 3 : e->relabelled > 128 ? 2 : e->relabelled ? 1 : 0;
        require(s, (after.direct_inserts - before.direct_inserts) +
            (after.successes - before.successes) == 1, "insert diagnostic accounting");
#else
        e->category = -1; /* No private work counts in production timing. */
#endif
    }
}
static Item *new_item(Simulation *s)
{
    Item *item;
    require(s, s->next_id < s->capacity, "item capacity exhausted");
    item = &s->items[s->next_id]; item->id = s->next_id++;
    return item;
}

/* A flat pointer array independently sorts keys and fresh insertion sequence.
 * The Tree's physical in-order fill is checked for strict Path order as well
 * as exact identities; no cached nodes/Paths survive a mutation. */
static int validate(Simulation *s)
{
    size_t i, probes, active_count = 0;
    if (lks_ordered_tree_size(s->tree) != s->count) return 0;
    for (i = 0; i < s->next_id; ++i) {
        if (s->items[i].active) {
            ++active_count;
            if (s->items[i].dense_index >= s->count ||
                s->active[s->items[i].dense_index] != &s->items[i]) return 0;
        }
    }
    if (active_count != s->count) return 0;
    memcpy(s->expected, s->active, s->count * sizeof(*s->expected));
    qsort(s->expected, s->count, sizeof(*s->expected), reference_order);
    if (lks_tree_internal_fill_ordered(lks_ordered_tree_internal_index(s->tree),
        s->nodes, s->count) != LKS_STATUS_OK) return 0;
    for (i = 0; i < s->count; ++i) {
        int cmp;
        Item *item = s->expected[i];
        if (!item->active || item->dense_index >= s->count ||
            s->active[item->dense_index] != item ||
            lks_tree_node_item(s->nodes[i]) != item) return 0;
        if (i && (lks_path_compare(lks_tree_node_path(s->nodes[i - 1]),
            lks_tree_node_path(s->nodes[i]), &cmp) != LKS_STATUS_OK || cmp >= 0))
            return 0;
    }
    probes = s->count < 32 ? s->count : 32;
    for (i = 0; i < probes; ++i) {
        size_t index = (s->count / probes) * i, first = index;
        const LksTreeNode *left, *equal, *right, *found = NULL;
        while (first && s->expected[first - 1]->key == s->expected[index]->key)
            --first;
        if (lks_ordered_tree_locate(s->tree, s->expected[index], &left, &equal,
            &right) != LKS_STATUS_OK || equal != s->nodes[first]) return 0;
        if (lks_ordered_tree_find_path(s->tree, lks_tree_node_path(s->nodes[index]),
            &found) != LKS_STATUS_OK || found != s->nodes[index]) return 0;
    }
    return 1;
}
static void growth(Simulation *s)
{
    size_t i, depth_sum = 0, display_sum = 0, key_sum = 0, bytes = 0;
    size_t display_max = 0, key_max = 0, *depths;
    uint64_t path_digest = UINT64_C(14695981039346656037);
    require(s, validate(s), "flat-model/Path-order/identity/lookup oracle");
    depths = (size_t *)array(s, s->count ? s->count : 1, sizeof(*depths));
    for (i = 0; i < s->count; ++i) {
        const LksPath *path = lks_tree_node_path(s->nodes[i]);
        size_t d = lks_path_depth(path), display = lks_path_text_length(path);
        size_t key = lks_path_order_key_length(path), storage = lks_path_internal_storage_bytes(path);
        size_t j;
        require(s, storage <= SIZE_MAX - lks_path_internal_sizeof_path(), "Path byte overflow");
        storage += lks_path_internal_sizeof_path();
        require(s, display && key && depth_sum <= SIZE_MAX - d &&
            display_sum <= SIZE_MAX - display && key_sum <= SIZE_MAX - key &&
            bytes <= SIZE_MAX - storage, "Path profile overflow");
        depths[i] = d; depth_sum += d; display_sum += display; key_sum += key;
        bytes += storage;
        if (display > display_max) display_max = display;
        if (key > key_max) key_max = key;
        path_digest = hash_word(path_digest, s->expected[i]->id);
        path_digest = hash_word(path_digest, (uint64_t)lks_path_direction(path));
        for (j = 0; j < d; ++j) {
            size_t level; unsigned int slot;
            require(s, lks_path_get_level(path, j, &level) == LKS_STATUS_OK &&
                lks_path_get_slot(path, j, &slot) == LKS_STATUS_OK,
                "inspect Path step");
            path_digest = hash_word(hash_word(path_digest, level), slot);
        }
    }
    qsort(depths, s->count, sizeof(*depths), size_order);
    printf("{\"kind\":\"growth\",\"operation\":%zu,\"resident\":%zu,"
        "\"depth_mean\":%.6f,\"depth_p95\":%zu,\"depth_p99\":%zu,\"depth_max\":%zu,"
        "\"display_mean\":%.6f,\"display_max\":%zu,\"lk1_mean\":%.6f,\"lk1_max\":%zu,"
        "\"resident_path_bytes\":%zu,\"lk1_payload_bytes\":%zu,"
        "\"path_digest\":\"%016" PRIx64 "\"",
        s->operation, s->count, s->count ? (double)depth_sum / s->count : 0.0,
        s->count ? depths[quantile_index(s->count, 950)] : 0,
        s->count ? depths[quantile_index(s->count, 990)] : 0,
        s->count ? depths[s->count - 1] : 0, s->count ? (double)display_sum / s->count : 0.0,
        display_max, s->count ? (double)key_sum / s->count : 0.0, key_max, bytes, key_sum,
        path_digest);
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    {
        LksAllocStats a = lks_alloc_stats_get();
        require(s, !a.counter_overflowed, "allocation counter overflow");
        printf(",\"alloc_calls\":%zu,\"requested_bytes\":%zu,\"live_bytes\":%zu,"
            "\"peak_live_bytes\":%zu,\"path_object_bytes\":%zu,"
            "\"path_step_bytes\":%zu,\"tree_node_bytes\":%zu",
            a.alloc_calls, a.total_successful_requested_bytes, a.live_bytes,
            a.peak_live_bytes, a.tags[LKS_ALLOC_TAG_PATH_OBJECT].live_bytes,
            a.tags[LKS_ALLOC_TAG_PATH_STEPS].live_bytes, a.tags[LKS_ALLOC_TAG_TREE_NODE].live_bytes);
        require(s, bytes == a.tags[LKS_ALLOC_TAG_PATH_OBJECT].live_bytes +
            a.tags[LKS_ALLOC_TAG_PATH_STEPS].live_bytes, "resident Path byte accounting");
    }
#endif
    puts("}"); free(depths);
}

static void initialize(Simulation *s)
{
    LksComparator cmp = { compare_item, s };
    size_t i;
    require(s, s->initial <= SIZE_MAX - s->operations, "population overflow");
    s->capacity = s->initial + s->operations;
    require(s, (uint64_t)s->capacity <= (uint64_t)INT64_MAX / (16 * KEY_GAP),
        "synthetic domain too large");
    s->items = (Item *)array(s, s->capacity, sizeof(*s->items));
    s->active = (Item **)array(s, s->capacity, sizeof(*s->active));
    s->expected = (Item **)array(s, s->capacity, sizeof(*s->expected));
    s->nodes = (const LksTreeNode **)array(s, s->capacity, sizeof(*s->nodes));
    s->events = (Event *)array(s, s->operations, sizeof(*s->events));
    s->digest = UINT64_C(14695981039346656037);
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    lks_alloc_stats_reset();
#endif
    lks_tree_repair_stats_reset();
    s->tree = lks_ordered_tree_create(&cmp);
    require(s, s->tree != NULL, "Tree allocation failed");
    if (control(s->workload)) {
        /* Historical definitions, with the first initial elements untimed.
         * No shuffle algorithm, comparator, or historical file is changed. */
        for (i = 0; i < s->capacity; ++i) {
            int64_t key = (int64_t)i;
            if (!strcmp(s->workload, "descending")) key = (int64_t)(s->capacity - i);
            else if (!strcmp(s->workload, "equal")) key = 0;
            else if (!strcmp(s->workload, "duplicates")) key = next_random(s) % 32;
            else if (!strcmp(s->workload, "alternating"))
                key = (int64_t)((i & 1) ? s->capacity - 1 - i / 2 : i / 2);
            else if (!strcmp(s->workload, "hotspot")) key = i ? (int64_t)(s->capacity - i) : 0;
            s->items[i].key = key;
        }
        if (!strcmp(s->workload, "random")) {
            for (i = s->capacity; i > 1; --i) {
                size_t j = next_random(s) % i;
                int64_t tmp = s->items[i - 1].key;
                s->items[i - 1].key = s->items[j].key; s->items[j].key = tmp;
            }
        }
    }
    for (i = 0; i < s->initial; ++i) {
        Item *item = new_item(s);
        int64_t key = control(s->workload) ? item->key :
            !strcmp(s->workload, "priority") ? (int64_t)(next_random(s) % 32) :
            (int64_t)i * (!strcmp(s->workload, "timeline") ? 1024 : KEY_GAP);
        insert_item(s, item, key, 0);
    }
}
static int64_t uniform_key(Simulation *s, int64_t maximum)
{
    /* Exact integer arithmetic; positive domain fits the checked scenario size.
     * A 32-bit PRNG bounds resolution even on a wider key domain. */
    uint64_t span = (uint64_t)maximum, r = next_random(s);
    return (int64_t)((span / UINT32_MAX) * r + (span % UINT32_MAX) * r / UINT32_MAX);
}
static int64_t local_delta(Simulation *s, int64_t unit)
{
    int64_t delta = (int64_t)(1 + next_random(s) % 8) * unit;
    return next_random(s) & 1 ? delta : -delta;
}
static void mutate(Simulation *s)
{
    unsigned int choice;
    Item *item;
    int64_t key;
    int action; /* 0 new, 1 remove-only, 2 remove/change/reinsert */
    if (control(s->workload)) {
        item = new_item(s); key = item->key;
        insert_item(s, item, key, 1); return;
    }
    choice = next_random(s) % 100;
    if (!strcmp(s->workload, "timeline")) {
        action = choice < 90 ? 0 : 1;
        key = choice < 70 ? shifted(s, s->high_water, 1 + next_random(s) % 1024) :
            choice < 85 ? shifted(s, s->high_water, -(int64_t)(1 + next_random(s) % 65536)) :
            uniform_key(s, s->high_water);
    } else if (!strcmp(s->workload, "priority")) {
        action = choice < 35 ? 0 : choice < 60 ? 1 : 2;
        key = next_random(s) % 32;
    } else if (!strcmp(s->workload, "local")) {
        action = choice < 10 ? 0 : choice < 20 ? 1 : 2;
        key = shifted(s, s->active[next_random(s) % s->count]->key, local_delta(s, KEY_GAP));
    } else {
        size_t band = s->initial / 20;
        action = choice < 30 ? 0 : choice < 60 ? 1 : 2;
        if (action == 0 && s->count >= s->initial + band) action = 1;
        if (action == 1 && s->count <= s->initial - band) action = 0;
        key = uniform_key(s, (int64_t)s->initial * KEY_GAP);
        if (next_random(s) % 100 < (action == 2 ? 10u : 5u)) {
            key = next_random(s) & 1 ? shifted(s, s->high_water, KEY_GAP) :
                shifted(s, s->low_water, -KEY_GAP);
        }
    }
    if (s->count < 2) action = 0;
    if (action == 0) insert_item(s, new_item(s), key, 1);
    else {
        item = remove_query(s, s->active[next_random(s) % s->count]->key);
        if (action == 1) ++s->removals;
        else {
            if (!strcmp(s->workload, "local")) {
                key = next_random(s) % 100 < 90 ? shifted(s, item->key, local_delta(s, KEY_GAP)) :
                    shifted(s, s->active[next_random(s) % s->count]->key,
                        local_delta(s, 128));
            }
            ++s->updates;
            insert_item(s, item, key, 1);
        }
    }
}
static const char *category_name(int category)
{
    static const char *names[] = { "direct", "local", "large", "full" };
    return category < 0 ? "unavailable" : names[category];
}
static void emit_event(const Event *e)
{
    printf("{\"kind\":\"event\",\"operation\":%zu,\"resident\":%zu,\"ms\":%.9f,"
        "\"category\":\"%s\",\"relabelled\":%zu,\"attempted_nodes_sum\":%zu,"
        "\"attempts\":%zu,\"expansions\":%zu,\"generated\":%zu,\"search_steps\":%zu,"
        "\"rotations\":%zu,\"comparisons\":%" PRIu64 ",\"inserted_depth\":%zu,"
        "\"full\":%d,\"sample_flags\":%d}\n", e->operation, e->resident, e->ms,
        category_name(e->category), e->relabelled, e->attempted, e->attempts,
        e->expansions, e->generated, e->search_steps, e->rotations,
        e->comparisons, e->depth, e->full, e->sample);
}
static void latency(const char *kind, const char *category, double *times, size_t n)
{
    size_t i;
    double sum = 0.0;
    if (!n) return;
    for (i = 0; i < n; ++i) sum += times[i];
    qsort(times, n, sizeof(*times), time_order);
    printf("{\"kind\":\"%s\",\"category\":\"%s\",\"count\":%zu,\"mean_ms\":%.9f,"
        "\"p50_ms\":%.9f,\"p95_ms\":%.9f,\"p99_ms\":%.9f,\"p999_ms\":",
        kind, category, n, sum / n, times[quantile_index(n, 500)],
        times[quantile_index(n, 950)], times[quantile_index(n, 990)]);
    if (n >= 10000) printf("%.9f", times[quantile_index(n, 999)]); else printf("null");
    printf(",\"max_ms\":%.9f}\n", times[n - 1]);
}
static void summarize(Simulation *s)
{
    size_t i, j, bins[8] = {0}, total = 0, maximum = 0, attempts = 0, attempted = 0;
    size_t full_nodes = 0, first_full = 0;
    double *times = (double *)array(s, s->insertions, sizeof(*times));
    Event **rank = (Event **)array(s, s->insertions, sizeof(*rank));
    for (i = 0; i < s->insertions; ++i) {
        Event *e = &s->events[i];
        size_t bin = e->full ? 7 : !e->relabelled ? 0 : e->relabelled <= 8 ? 1 :
            e->relabelled <= 32 ? 2 : e->relabelled <= 128 ? 3 :
            e->relabelled <= 512 ? 4 : e->relabelled <= 4096 ? 5 : 6;
        times[i] = e->ms; rank[i] = e; ++bins[bin];
        total += e->relabelled; attempts += e->attempts; attempted += e->attempted;
        if (e->relabelled > maximum) maximum = e->relabelled;
        if (e->full) {
            full_nodes += e->relabelled;
            if (first_full++ < FIRST_FULL_COUNT) e->sample |= 4;
        }
    }
    latency("latency", "all", times, s->insertions);
    if (diagnostic_build()) {
        for (j = 0; j < 4; ++j) {
            size_t n = 0;
            for (i = 0; i < s->insertions; ++i)
                if (s->events[i].category == (int)j) times[n++] = s->events[i].ms;
            latency("latency", category_name((int)j), times, n);
        }
    }
    qsort(rank, s->insertions, sizeof(*rank), event_latency_order);
    for (i = 0; i < s->insertions && i < SAMPLE_COUNT; ++i)
        rank[s->insertions - 1 - i]->sample |= 1;
    if (diagnostic_build()) {
        qsort(rank, s->insertions, sizeof(*rank), event_region_order);
        for (i = 0; i < s->insertions && i < SAMPLE_COUNT; ++i) {
            Event *e = rank[s->insertions - 1 - i];
            if (e->relabelled) e->sample |= 2;
        }
    }
    for (i = 0; i < s->insertions; ++i)
        if (s->trace || s->events[i].sample) emit_event(&s->events[i]);
    printf("{\"kind\":\"summary\",\"workload\":\"%s\",\"seed\":%" PRIu32 ","
        "\"initial\":%zu,\"operations\":%zu,\"checkpoint\":%zu,\"diagnostic\":%d,"
        "\"insertions\":%zu,\"new_items\":%zu,\"remove_only\":%zu,\"updates\":%zu,"
        "\"final_resident\":%zu,\"trace_digest\":\"%016" PRIx64 "\",\"verified\":true,"
        "\"relabelled_total\":%zu,\"max_region\":%zu,\"attempts\":%zu,"
        "\"attempted_nodes_sum\":%zu,\"full_nodes\":%zu,\"histogram\":[",
        s->workload, s->seed, s->initial, s->operations, s->checkpoint,
        diagnostic_build(), s->insertions, s->next_id - s->initial, s->removals,
        s->updates, s->count, s->digest, total, maximum, attempts, attempted, full_nodes);
    for (i = 0; i < 8; ++i) printf("%s%zu", i ? "," : "", bins[i]);
    puts("]}");
    free(times); free(rank);
}
static void cleanup(Simulation *s)
{
    lks_ordered_tree_destroy(s->tree);
#ifdef LKS_WORKLOAD_DIAGNOSTICS
    require(s, lks_alloc_stats_get().live_bytes == 0, "library allocation leak after destroy");
#endif
    free(s->items); free(s->active); free(s->expected); free(s->nodes); free(s->events);
}
static size_t decimal(const char *text)
{
    size_t value = 0;
    if (!*text) { fprintf(stderr, "empty integer\n"); exit(2); }
    for (; *text; ++text) {
        unsigned int digit = (unsigned int)(*text - '0');
        if (digit > 9 || value > (SIZE_MAX - digit) / 10) {
            fprintf(stderr, "invalid/overflowing integer\n"); exit(2);
        }
        value = value * 10 + digit;
    }
    return value;
}
static int oracle_self_test(void)
{
    Simulation s = {0};
    s.workload = "priority"; s.seed = s.random = 7;
    s.initial = 2; s.operations = 1; s.checkpoint = 1;
    initialize(&s);
    /* Force equal keys through remove/change/reinsert, never mutate a resident. */
    {
        Item *item = remove_query(&s, s.active[0]->key);
        insert_item(&s, item, s.active[0]->key, 0);
    }
    require(&s, validate(&s), "self-test valid reference");
    {
        uint64_t tmp = s.active[0]->sequence;
        s.active[0]->sequence = s.active[1]->sequence;
        s.active[1]->sequence = tmp;
        require(&s, !validate(&s), "oracle missed corrupted equality order");
        tmp = s.active[0]->sequence;
        s.active[0]->sequence = s.active[1]->sequence;
        s.active[1]->sequence = tmp;
        s.active[0]->active = 0;
        require(&s, !validate(&s), "oracle missed corrupted membership");
        s.active[0]->active = 1;
    }
    require(&s, validate(&s), "self-test restored reference"); cleanup(&s);
    puts("{\"kind\":\"oracle_self_test\",\"passed\":true}"); return 0;
}
int main(int argc, char **argv)
{
    Simulation s = {0};
    size_t seed;
    if (argc == 2 && !strcmp(argv[1], "--oracle-self-test")) return oracle_self_test();
    if (argc < 6 || argc > 7 || (argc == 7 && strcmp(argv[6], "--trace"))) {
        fprintf(stderr, "usage: %s WORKLOAD SEED INITIAL OPERATIONS CHECKPOINT [--trace]\n", argv[0]);
        return 2;
    }
    s.workload = argv[1]; seed = decimal(argv[2]);
    s.initial = decimal(argv[3]); s.operations = decimal(argv[4]); s.checkpoint = decimal(argv[5]);
    s.trace = argc == 7;
    if ((!control(s.workload) && strcmp(s.workload, "timeline") &&
        strcmp(s.workload, "priority") && strcmp(s.workload, "local") && strcmp(s.workload, "churn")) ||
        !seed || seed > UINT32_MAX || s.initial < 2 || !s.operations || !s.checkpoint) {
        fprintf(stderr, "invalid scenario arguments\n"); return 2;
    }
    s.seed = s.random = (uint32_t)seed;
    initialize(&s); growth(&s);
    for (s.operation = 1; s.operation <= s.operations; ++s.operation) {
        mutate(&s);
        if (s.operation % s.checkpoint == 0 || s.operation == s.operations) growth(&s);
    }
    --s.operation;
    summarize(&s); cleanup(&s);
    printf("{\"kind\":\"cleanup\",\"after_destroy_live_bytes\":%s,\"diagnostic\":%d}\n",
        diagnostic_build() ? "0" : "null", diagnostic_build());
    return 0;
}
