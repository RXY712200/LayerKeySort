#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"
#include "path_external.h"
#include "../src/lks_alloc_internal.h"

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "external Path test failed: %s:%d: %s\n", \
        __FILE__, __LINE__, #expr); return 0; } } while (0)

static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *state = x;
}

static int same_path(const LksPath *a, const LksPath *b)
{
    size_t i, depth = lks_path_depth(a);
    if (lks_path_direction(a) != lks_path_direction(b) ||
        depth != lks_path_depth(b)) return 0;
    for (i = 0; i < depth; ++i) {
        size_t la, lb;
        unsigned int sa, sb;
        if (lks_path_get_level(a, i, &la) != LKS_STATUS_OK ||
            lks_path_get_level(b, i, &lb) != LKS_STATUS_OK ||
            lks_path_get_slot(a, i, &sa) != LKS_STATUS_OK ||
            lks_path_get_slot(b, i, &sb) != LKS_STATUS_OK ||
            la != lb || sa != sb) return 0;
    }
    return 1;
}

static char *format_display(const LksPath *path)
{
    size_t n = lks_path_text_length(path);
    char *s = n < SIZE_MAX ? (char *)malloc(n + 1) : NULL;
    if (s == NULL || lks_path_format(path, s, n + 1) != LKS_STATUS_OK) {
        free(s); return NULL;
    }
    return s;
}

static char *format_key(const LksPath *path)
{
    size_t n = lks_path_order_key_length(path);
    char *s = n < SIZE_MAX ? (char *)malloc(n + 1) : NULL;
    if (s == NULL || lks_path_order_key_format(path, s, n + 1) != LKS_STATUS_OK) {
        free(s); return NULL;
    }
    return s;
}

static int round_trip(const LksPath *path)
{
    char *display = format_display(path), *key = format_key(path);
    char *display_again = NULL, *key_again = NULL;
    LksPath *from_display = NULL, *from_key = NULL;
    int result = 0, cmp = 1;
    if (display != NULL && key != NULL &&
        lks_path_parse(display, &from_display) == LKS_STATUS_OK &&
        lks_path_order_key_parse(key, &from_key) == LKS_STATUS_OK &&
        same_path(path, from_display) && same_path(path, from_key) &&
        lks_path_compare(path, from_key, &cmp) == LKS_STATUS_OK && cmp == 0)
        result = 1;
    if (result) {
        display_again = format_display(from_display);
        key_again = format_key(from_key);
        result = display_again != NULL && key_again != NULL &&
            strcmp(display, display_again) == 0 && strcmp(key, key_again) == 0;
    }
    lks_path_destroy(from_display); lks_path_destroy(from_key);
    free(display); free(key); free(display_again); free(key_again);
    return result;
}

static int compare_paths(const void *a, const void *b)
{
    const LksPath *left = *(const LksPath *const *)a;
    const LksPath *right = *(const LksPath *const *)b;
    int result = 0;
    (void)lks_path_compare(left, right, &result);
    return result;
}

static int compare_keys(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int sign(int x) { return (x > 0) - (x < 0); }

static int corpus_test(void)
{
    enum { COUNT = 480 };
    static const unsigned int slots[] = {0, 1, 53, 54, 32768, 65535};
    LksPath *paths[COUNT], *ordered[COUNT];
    char *keys[COUNT], *sorted_keys[COUNT];
    uint32_t state = UINT32_C(0x91A30D47);
    size_t i, j;
    memset(paths, 0, sizeof(paths)); memset(keys, 0, sizeof(keys));
    paths[0] = lks_path_create_zero();
    CHECK(paths[0] != NULL);
    for (i = 1; i < COUNT; ++i) {
        LksDirection direction = i % 2 ? LKS_DIRECTION_NEGATIVE : LKS_DIRECTION_POSITIVE;
        size_t level, depth = 1 + (i % 5);
        unsigned int slot = i < 70 ? slots[i % 6] : next_random(&state) & 65535u;
        if (i % 17 == 0) level = SIZE_MAX - 32;
        else if (i % 11 == 0) level = 256 + i;
        else level = i % 4;
        paths[i] = lks_path_create_at_level(direction, slot, level);
        CHECK(paths[i] != NULL);
        for (j = 1; j < depth; ++j) {
            size_t delta = 1 + next_random(&state) % 4;
            slot = i < 70 ? slots[(i + j) % 6] : next_random(&state) & 65535u;
            CHECK(lks_path_append_at_level(paths[i], slot, level + delta) == LKS_STATUS_OK);
            level += delta;
        }
        CHECK(round_trip(paths[i]));
    }
    CHECK(round_trip(paths[0]));
    for (i = 0; i < COUNT; ++i) {
        keys[i] = format_key(paths[i]);
        CHECK(keys[i] != NULL);
        ordered[i] = paths[i]; sorted_keys[i] = keys[i];
    }
    for (i = 0; i < COUNT; ++i) for (j = 0; j < COUNT; ++j) {
        int path_cmp;
        CHECK(lks_path_compare(paths[i], paths[j], &path_cmp) == LKS_STATUS_OK);
        CHECK(sign(path_cmp) == sign(strcmp(keys[i], keys[j])));
    }
    qsort(ordered, COUNT, sizeof(ordered[0]), compare_paths);
    qsort(sorted_keys, COUNT, sizeof(sorted_keys[0]), compare_keys);
    for (i = 0; i < COUNT; ++i) {
        LksPath *decoded = NULL;
        CHECK(lks_path_order_key_parse(sorted_keys[i], &decoded) == LKS_STATUS_OK);
        CHECK(same_path(decoded, ordered[i]));
        lks_path_destroy(decoded);
    }
    for (i = 0; i < COUNT; ++i) { free(keys[i]); lks_path_destroy(paths[i]); }
    printf("Path external corpus: %u paths, %u ordered pairs, sorted corpus PASS\n",
        COUNT, COUNT * COUNT);
    return 1;
}

static int malformed_test(void)
{
    static const char *bad_display[] = {
        "", "0", "1", "2", "0000", "001", "022", "0/222", "0222/",
        "0222//223", "0222/1", "0222/0", "00222", "001222", "0123",
        "0 222", "0222!", "0O22", "022z/01AAA", "0222/0223",
        "0222/1223", "0" "184467440737095516160" "222",
        "0222/184467440737095516160223", "0RUd", "\t0222"
    };
    static const char *bad_key[] = {
        "", "LK", "LK2:1!", "XX1:1!", "LK1:", "LK1:3!", "LK1:1",
        "LK1:1!!", "LK1:0!", "LK1:2!", "LK1:201FF0000",
        "LK1:201FF0000!!", "LK1:201Ff0000!", "LK1:201FG0000!",
        "LK1:201FF000!", "LK1:2001FFFF0000!", "LK1:201FF000001FF0001!",
        "LK1:201FE000001FF0001!", "LK1:201FF0000x!",
        "LK1:201FF000001FF0000!", "LK1:201FF000001FF0001!"
    };
    size_t i;
    for (i = 0; i < sizeof(bad_display) / sizeof(bad_display[0]); ++i) {
        LksPath *path = (LksPath *)(uintptr_t)1;
        if (lks_path_parse(bad_display[i], &path) != LKS_STATUS_INVALID_ARGUMENT) {
            fprintf(stderr, "accepted bad display %zu: %s\n", i, bad_display[i]);
            return 0;
        }
        CHECK(path == NULL);
    }
    for (i = 0; i < sizeof(bad_key) / sizeof(bad_key[0]); ++i) {
        LksPath *path = (LksPath *)(uintptr_t)1;
        if (lks_path_order_key_parse(bad_key[i], &path) != LKS_STATUS_INVALID_ARGUMENT) {
            fprintf(stderr, "accepted bad key %zu: %s\n", i, bad_key[i]);
            return 0;
        }
        CHECK(path == NULL);
    }
    {
        char overwide[4 + 1 + sizeof(size_t) + 1 +
            2 * (sizeof(size_t) + 1) + 4 + 3];
        size_t pos = 0;
        LksPath *path = (LksPath *)(uintptr_t)1;
        memcpy(overwide, "LK1:2", 5); pos = 5;
        for (i = 0; i < sizeof(size_t) + 1; ++i) overwide[pos++] = '0';
        overwide[pos++] = '1';
        for (i = 0; i < sizeof(size_t) + 1; ++i) {
            overwide[pos++] = i == 0 ? 'F' : '0';
            overwide[pos++] = 'F';
        }
        memcpy(overwide + pos, "0000!", 5); pos += 5;
        overwide[pos] = '\0';
        CHECK(lks_path_order_key_parse(overwide, &path) ==
            LKS_STATUS_INVALID_ARGUMENT && path == NULL);
    }
    {
        /* 2^32 is a canonical five-byte level: accepted on 64-bit, rejected
         * rather than truncated on 32-bit receivers. */
        const char *cross_size = "LK1:2000001FEFFFFFFFF0000!";
        LksPath *path = (LksPath *)(uintptr_t)1;
        LksStatus status = lks_path_order_key_parse(cross_size, &path);
        if (SIZE_MAX > UINT32_MAX) {
            size_t level = 0;
            CHECK(status == LKS_STATUS_OK && path != NULL &&
                lks_path_get_level(path, 0, &level) == LKS_STATUS_OK &&
                level == UINT64_C(4294967296));
            lks_path_destroy(path);
        } else CHECK(status == LKS_STATUS_INVALID_ARGUMENT && path == NULL);
    }
    printf("Path external malformed: %zu display, %zu key PASS\n",
        sizeof(bad_display) / sizeof(bad_display[0]),
        sizeof(bad_key) / sizeof(bad_key[0]));
    return 1;
}

static int oom_test_one(const char *text,
    LksStatus (*parser)(const char *, LksPath **))
{
    size_t attempt;
    for (attempt = 1; attempt < 32; ++attempt) {
        LksAllocStats before = lks_alloc_stats_get(), after;
        LksPath *path = (LksPath *)(uintptr_t)1;
        LksStatus status;
        lks_alloc_test_reset_attempt_counter();
        lks_alloc_test_fail_on_attempt(attempt);
        status = parser(text, &path);
        size_t calls = lks_alloc_test_get_attempt_count();
        int triggered = lks_alloc_test_failure_triggered();
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            CHECK(path != NULL && !triggered && calls == attempt - 1);
            lks_path_destroy(path);
            after = lks_alloc_stats_get();
            CHECK(after.live_blocks == before.live_blocks &&
                after.live_bytes == before.live_bytes);
            printf("Path parser OOM sweep: %zu injected failures PASS\n", attempt - 1);
            return 1;
        }
        CHECK(status == LKS_STATUS_OUT_OF_MEMORY && path == NULL && triggered);
        after = lks_alloc_stats_get();
        CHECK(after.live_blocks == before.live_blocks &&
            after.live_bytes == before.live_bytes);
    }
    return 0;
}

static int deep_test(size_t depth)
{
    LksPath *path = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    char *display, *key;
    size_t i;
    CHECK(path != NULL);
    for (i = 1; i < depth; ++i)
        CHECK(lks_path_append(path, (unsigned int)(i & 65535u)) == LKS_STATUS_OK);
    display = format_display(path); key = format_key(path);
    CHECK(display != NULL && key != NULL && round_trip(path));
    if (depth == 1000) {
        LksPath *rejected = (LksPath *)(uintptr_t)1;
        LksAllocStats before, after;
        size_t n = strlen(display), k = strlen(key);
        char *expanded = (char *)realloc(display, n + 2);
        CHECK(expanded != NULL);
        display = expanded;
        expanded = (char *)realloc(key, k + 2);
        CHECK(expanded != NULL);
        key = expanded;
        display[n] = '/'; display[n + 1] = '\0';
        before = lks_alloc_stats_get();
        CHECK(lks_path_parse(display, &rejected) == LKS_STATUS_INVALID_ARGUMENT &&
            rejected == NULL);
        after = lks_alloc_stats_get();
        CHECK(after.live_blocks == before.live_blocks && after.live_bytes == before.live_bytes);
        display[n] = '\0';
        key[k - 1] = '0'; key[k] = '!'; key[k + 1] = '\0';
        rejected = (LksPath *)(uintptr_t)1;
        CHECK(lks_path_order_key_parse(key, &rejected) == LKS_STATUS_INVALID_ARGUMENT &&
            rejected == NULL);
        after = lks_alloc_stats_get();
        CHECK(after.live_blocks == before.live_blocks && after.live_bytes == before.live_bytes);
        key[k - 1] = '!'; key[k] = '\0';
    }
    printf("Path depth %zu: display %zu, key %zu bytes PASS\n",
        depth, strlen(display), strlen(key));
    free(display); free(key); lks_path_destroy(path);
    return 1;
}

static int rekey_noop_test(void)
{
    int item = 7;
    LksTree *tree = lks_tree_create();
    LksPath *path = lks_path_create(LKS_DIRECTION_POSITIVE, 42);
    LksPath *equal = NULL;
    const LksTreeNode *node = NULL, *result = NULL, *root;
    const LksPath *borrowed;
    CHECK(tree != NULL && path != NULL);
    CHECK(lks_tree_insert(tree, path, &item, &node) == LKS_STATUS_OK);
    equal = lks_path_clone(path);
    CHECK(equal != NULL);
    root = lks_tree_root_child_at(tree, 0);
    borrowed = lks_tree_node_path(node);
    CHECK(lks_tree_rekey(tree, path, equal, &result) == LKS_STATUS_OK);
    CHECK(result == node && root == lks_tree_root_child_at(tree, 0) &&
        borrowed == lks_tree_node_path(node) &&
        lks_tree_node_item(node) == &item && lks_tree_size(tree) == 1);
    lks_path_destroy(equal); lks_path_destroy(path); lks_tree_destroy(tree);
    return 1;
}

static int example_test(void)
{
    LksPath *positive = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    LksPath *negative = lks_path_create_at_level(LKS_DIRECTION_NEGATIVE, 54, 12);
    char *display, *key;
    CHECK(positive != NULL && negative != NULL);
    key = format_key(positive);
    CHECK(key != NULL && strcmp(key, "LK1:201FF0000!") == 0);
    free(key);
    CHECK(lks_path_append_at_level(positive, 1, 5) == LKS_STATUS_OK);
    display = format_display(positive); key = format_key(positive);
    CHECK(display != NULL && key != NULL &&
        strcmp(display, "0222/5223") == 0 &&
        strcmp(key, "LK1:201FF000001FA0001!") == 0);
    free(display); free(key);
    display = format_display(negative); key = format_key(negative);
    CHECK(display != NULL && key != NULL &&
        strcmp(display, "112232") == 0 &&
        strcmp(key, "LK1:001F3FFC9!") == 0);
    free(display); free(key);
    lks_path_destroy(positive); lks_path_destroy(negative);
    return 1;
}

static int golden_key_test(void)
{
    static const struct GoldenKey {
        LksDirection direction;
        size_t first_level;
        unsigned int first_slot;
        int has_child;
        size_t child_level;
        unsigned int child_slot;
        const char *key;
    } vectors[] = {
        {LKS_DIRECTION_POSITIVE, 0, 0, 0, 0, 0, "LK1:201FF0000!"},
        {LKS_DIRECTION_POSITIVE, 0, 65535, 0, 0, 0, "LK1:201FFFFFF!"},
        {LKS_DIRECTION_POSITIVE, 1, 0, 0, 0, 0, "LK1:201FE0000!"},
        {LKS_DIRECTION_POSITIVE, 254, 0, 0, 0, 0, "LK1:201010000!"},
        {LKS_DIRECTION_POSITIVE, 255, 0, 0, 0, 0, "LK1:201000000!"},
        {LKS_DIRECTION_POSITIVE, 256, 0, 0, 0, 0, "LK1:2001FEFF0000!"},
        {LKS_DIRECTION_POSITIVE, 65535, 0, 0, 0, 0, "LK1:200100000000!"},
        {LKS_DIRECTION_POSITIVE, 65536, 0, 0, 0, 0, "LK1:20001FEFFFF0000!"},
        {LKS_DIRECTION_POSITIVE, 0, 0, 1, 1, 1,
            "LK1:201FF000001FE0001!"},
        {LKS_DIRECTION_POSITIVE, 0, 0, 1, 5, 1,
            "LK1:201FF000001FA0001!"},
        {LKS_DIRECTION_NEGATIVE, 0, 0, 0, 0, 0, "LK1:001FFFFFF!"},
        {LKS_DIRECTION_NEGATIVE, 0, 65535, 0, 0, 0, "LK1:001FF0000!"},
        {LKS_DIRECTION_NEGATIVE, 0, 0, 1, 1, 0,
            "LK1:001FFFFFF01FE0000!"},
        {LKS_DIRECTION_NEGATIVE, 0, 0, 1, 1, 1,
            "LK1:001FFFFFF01FE0001!"},
        {LKS_DIRECTION_NEGATIVE, 12, 54, 0, 0, 0, "LK1:001F3FFC9!"},
        {LKS_DIRECTION_NEGATIVE, 12, 54, 1, 15, 65535,
            "LK1:001F3FFC901F0FFFF!"}
    };
    size_t i;
    LksPath *zero = lks_path_create_zero();
    char *zero_key = format_key(zero);
    CHECK(zero != NULL && zero_key != NULL && strcmp(zero_key, "LK1:1!") == 0);
    free(zero_key); lks_path_destroy(zero);
    for (i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        const struct GoldenKey *v = &vectors[i];
        LksPath *path = lks_path_create_at_level(v->direction,
            v->first_slot, v->first_level);
        LksPath *decoded = NULL;
        char *key;
        CHECK(path != NULL);
        if (v->has_child)
            CHECK(lks_path_append_at_level(path, v->child_slot,
                v->child_level) == LKS_STATUS_OK);
        key = format_key(path);
        if (key == NULL || strcmp(key, v->key) != 0) {
            fprintf(stderr, "golden key mismatch %zu: expected %s, got %s\n",
                i, v->key, key == NULL ? "(null)" : key);
            return 0;
        }
        CHECK(lks_path_order_key_parse(v->key, &decoded) == LKS_STATUS_OK &&
            same_path(path, decoded));
        free(key); lks_path_destroy(decoded); lks_path_destroy(path);
    }
    if (sizeof(size_t) == 4 || sizeof(size_t) == 8) {
        const char *max_key = sizeof(size_t) == 8 ?
            "LK1:2" "000000001" "0000000000000000" "0000!" :
            "LK1:2" "00001" "00000000" "0000!";
        const char *near_key = sizeof(size_t) == 8 ?
            "LK1:2" "000000001" "0000000000000001" "0000!" :
            "LK1:2" "00001" "00000001" "0000!";
        LksPath *maximum = lks_path_create_at_level(
            LKS_DIRECTION_POSITIVE, 0, SIZE_MAX);
        LksPath *near = lks_path_create_at_level(
            LKS_DIRECTION_POSITIVE, 0, SIZE_MAX - 1);
        char *a = format_key(maximum), *b = format_key(near);
        CHECK(maximum != NULL && near != NULL && a != NULL && b != NULL &&
            strcmp(a, max_key) == 0 && strcmp(b, near_key) == 0);
        free(a); free(b); lks_path_destroy(maximum); lks_path_destroy(near);
    }
    printf("Path key v1 golden vectors: %zu fixed plus ZERO and SIZE_MAX edges PASS\n",
        sizeof(vectors) / sizeof(vectors[0]));
    return 1;
}

static int gap_boundary_test(void)
{
    LksPath *negative = lks_path_create(LKS_DIRECTION_NEGATIVE, 65535);
    LksPath *negative_next = lks_path_create(LKS_DIRECTION_NEGATIVE, 65534);
    LksPath *zero = lks_path_create_zero();
    LksPath *positive = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    LksPath *positive_next = lks_path_create(LKS_DIRECTION_POSITIVE, 1);
    LksPath *positive_edge = lks_path_create_at_level(
        LKS_DIRECTION_POSITIVE, 0, SIZE_MAX);
    LksPath *candidate = NULL, *descendant;
    int left_order, right_order;
    CHECK(negative != NULL && negative_next != NULL && zero != NULL &&
        positive != NULL && positive_next != NULL && positive_edge != NULL);
    CHECK(lks_path_between(negative, negative_next, &candidate) == LKS_STATUS_OK &&
        lks_path_compare(negative, candidate, &left_order) == LKS_STATUS_OK &&
        lks_path_compare(candidate, negative_next, &right_order) == LKS_STATUS_OK &&
        left_order < 0 && right_order < 0);
    lks_path_destroy(candidate); candidate = NULL;
    CHECK(lks_path_between(positive, positive_next, &candidate) == LKS_STATUS_OK &&
        lks_path_compare(positive, candidate, &left_order) == LKS_STATUS_OK &&
        lks_path_compare(candidate, positive_next, &right_order) == LKS_STATUS_OK &&
        left_order < 0 && right_order < 0);
    lks_path_destroy(candidate); candidate = NULL;
    descendant = lks_path_clone(positive);
    CHECK(descendant != NULL &&
        lks_path_append(descendant, 0) == LKS_STATUS_OK &&
        lks_path_between(positive, descendant, &candidate) == LKS_STATUS_OK &&
        lks_path_compare(positive, candidate, &left_order) == LKS_STATUS_OK &&
        lks_path_compare(candidate, descendant, &right_order) == LKS_STATUS_OK &&
        left_order < 0 && right_order < 0);
    lks_path_destroy(candidate); candidate = NULL;
    CHECK(lks_path_between(negative, positive, &candidate) == LKS_STATUS_OK &&
        lks_path_direction(candidate) == LKS_DIRECTION_ZERO);
    lks_path_destroy(candidate); candidate = NULL;
    CHECK(lks_path_before(zero, &candidate) == LKS_STATUS_OK &&
        lks_path_direction(candidate) == LKS_DIRECTION_NEGATIVE);
    lks_path_destroy(candidate); candidate = NULL;
    CHECK(lks_path_after(zero, &candidate) == LKS_STATUS_OK &&
        lks_path_direction(candidate) == LKS_DIRECTION_POSITIVE);
    lks_path_destroy(candidate); candidate = NULL;
    CHECK(lks_path_between(zero, positive_edge, &candidate) ==
        LKS_STATUS_LEVEL_LIMIT && candidate == NULL);
    CHECK(lks_path_append(positive_edge, 0) == LKS_STATUS_INVALID_ARGUMENT);
    lks_path_destroy(negative); lks_path_destroy(negative_next);
    lks_path_destroy(zero); lks_path_destroy(positive);
    lks_path_destroy(positive_next); lks_path_destroy(positive_edge);
    lks_path_destroy(descendant);
    puts("Path gap boundary cases PASS");
    return 1;
}

int lks_run_path_external_tests(void)
{
    LksPath *p = NULL;
    char small[8] = "keep";
    CHECK(lks_path_parse(NULL, &p) == LKS_STATUS_INVALID_ARGUMENT && p == NULL);
    CHECK(lks_path_order_key_parse(NULL, &p) == LKS_STATUS_INVALID_ARGUMENT && p == NULL);
    CHECK(lks_path_parse("000", NULL) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_path_order_key_parse("LK1:1!", NULL) == LKS_STATUS_INVALID_ARGUMENT);
    p = lks_path_create_zero(); CHECK(p != NULL);
    CHECK(lks_path_order_key_length(p) == 6);
    CHECK(lks_path_order_key_format(p, small, 5) == LKS_STATUS_BUFFER_TOO_SMALL);
    CHECK(strcmp(small, "keep") == 0);
    CHECK(lks_path_order_key_format(p, small, sizeof(small)) == LKS_STATUS_OK);
    CHECK(strcmp(small, "LK1:1!") == 0);
    lks_alloc_test_reset_attempt_counter();
    lks_alloc_test_fail_on_attempt(1);
    CHECK(lks_path_order_key_length(p) == 6 &&
        lks_path_order_key_format(p, small, sizeof(small)) == LKS_STATUS_OK &&
        lks_alloc_test_get_attempt_count() == 0 &&
        !lks_alloc_test_failure_triggered());
    lks_alloc_test_disable_failure();
    lks_path_destroy(p);
    CHECK(example_test() && golden_key_test() && gap_boundary_test() && corpus_test() &&
        malformed_test() && rekey_noop_test());
    CHECK(oom_test_one("01222/223/2RUc", lks_path_parse));
    CHECK(oom_test_one("LK1:201FE000001FD000101FCFFFF!", lks_path_order_key_parse));
    CHECK(deep_test(1000) && deep_test(10000) && deep_test(100000));
    return 0;
}
