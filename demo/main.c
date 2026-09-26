#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"
#include "../tests/benchmark.h"
#include "../tests/property.h"

static int compare_int(const void *left, const void *right, void *context)
{
    int left_value;
    int right_value;

    (void)context;
    left_value = *(const int *)left;
    right_value = *(const int *)right;

    if (left_value < right_value) {
        return -1;
    }
    if (left_value > right_value) {
        return 1;
    }
    return 0;
}

static LksPath *make_path(
    LksDirection direction,
    const unsigned int *slots,
    size_t count
)
{
    LksPath *path;
    size_t index;

    if (slots == NULL || count == 0) {
        return NULL;
    }

    path = lks_path_create(direction, slots[0]);
    if (path == NULL) {
        return NULL;
    }

    for (index = 1; index < count; ++index) {
        if (lks_path_append(path, slots[index]) != LKS_STATUS_OK) {
            lks_path_destroy(path);
            return NULL;
        }
    }
    return path;
}

static LksPath *make_path_at_levels(
    LksDirection direction,
    const unsigned int *slots,
    const size_t *levels,
    size_t count
)
{
    LksPath *path;
    size_t index;

    if (slots == NULL || levels == NULL || count == 0) {
        return NULL;
    }

    path = lks_path_create_at_level(direction, slots[0], levels[0]);
    if (path == NULL) {
        return NULL;
    }
    for (index = 1; index < count; ++index) {
        if (lks_path_append_at_level(path, slots[index], levels[index]) !=
            LKS_STATUS_OK) {
            lks_path_destroy(path);
            return NULL;
        }
    }
    return path;
}
static int expect_compare(
    const char *label,
    const LksPath *left,
    const LksPath *right,
    int expected
)
{
    int comparison = 99;
    LksStatus status;

    status = lks_path_compare(left, right, &comparison);
    printf("%s: %d\n", label, comparison);
    return status == LKS_STATUS_OK && comparison == expected;
}

static int run_path_compare_demo(const LksPath *zero_path)
{
    static const unsigned int negative_a1_slots[] = { 1u };
    static const unsigned int negative_a2_slots[] = { 2u };
    static const unsigned int positive_a1_slots[] = { 1u };
    static const unsigned int positive_a2_slots[] = { 2u };
    static const unsigned int positive_parent_slots[] = { 3u };
    static const unsigned int p1_slots[] = { 3u, 259u, 0u };
    static const unsigned int p2_slots[] = { 3u, 259u, 0u, 0u };
    static const unsigned int p3_slots[] = { 3u, 259u, 1u };
    static const unsigned int n2_slots[] = { 2u, 0u };
    static const unsigned int n3_slots[] = { 2u, 1u };
    LksPath *negative_a1 = NULL;
    LksPath *negative_a2 = NULL;
    LksPath *positive_a1 = NULL;
    LksPath *positive_a2 = NULL;
    LksPath *positive_parent = NULL;
    LksPath *positive_child = NULL;
    LksPath *p1 = NULL;
    LksPath *p2 = NULL;
    LksPath *p3 = NULL;
    LksPath *p1_clone = NULL;
    LksPath *n2 = NULL;
    LksPath *n3 = NULL;
    int success = 0;

    negative_a1 = make_path(
        LKS_DIRECTION_NEGATIVE, negative_a1_slots, 1
    );
    negative_a2 = make_path(
        LKS_DIRECTION_NEGATIVE, negative_a2_slots, 1
    );
    positive_a1 = make_path(
        LKS_DIRECTION_POSITIVE, positive_a1_slots, 1
    );
    positive_a2 = make_path(
        LKS_DIRECTION_POSITIVE, positive_a2_slots, 1
    );
    positive_parent = make_path(
        LKS_DIRECTION_POSITIVE, positive_parent_slots, 1
    );
    if (positive_parent != NULL) {
        positive_child = lks_path_clone(positive_parent);
    }
    if (positive_child != NULL &&
        lks_path_append(positive_child, 0u) != LKS_STATUS_OK) {
        lks_path_destroy(positive_child);
        positive_child = NULL;
    }
    p1 = make_path(LKS_DIRECTION_POSITIVE, p1_slots, 3);
    p2 = make_path(LKS_DIRECTION_POSITIVE, p2_slots, 4);
    p3 = make_path(LKS_DIRECTION_POSITIVE, p3_slots, 3);
    if (p1 != NULL) {
        p1_clone = lks_path_clone(p1);
    }
    n2 = make_path(LKS_DIRECTION_NEGATIVE, n2_slots, 2);
    n3 = make_path(LKS_DIRECTION_NEGATIVE, n3_slots, 2);

    if (negative_a1 == NULL || negative_a2 == NULL ||
        positive_a1 == NULL || positive_a2 == NULL ||
        positive_parent == NULL || positive_child == NULL ||
        p1 == NULL || p2 == NULL || p3 == NULL || p1_clone == NULL ||
        n2 == NULL || n3 == NULL) {
        goto cleanup;
    }

    printf("Path compare tests:\n");
    if (!expect_compare("negative vs zero", negative_a1, zero_path, -1) ||
        !expect_compare("zero vs positive", zero_path, positive_a1, -1) ||
        !expect_compare("negative vs positive", negative_a1, positive_a1, -1) ||
        !expect_compare("positive A1 vs A2", positive_a1, positive_a2, -1) ||
        !expect_compare("positive A2 vs A1", positive_a2, positive_a1, 1) ||
        !expect_compare("positive parent vs child", positive_parent, positive_child, -1) ||
        !expect_compare("complex P1 vs P2", p1, p2, -1) ||
        !expect_compare("complex P2 vs P3", p2, p3, -1) ||
        !expect_compare("complex P1 vs P3", p1, p3, -1) ||
        !expect_compare("negative A2 vs A1", negative_a2, negative_a1, -1) ||
        !expect_compare("negative A1 vs A2", negative_a1, negative_a2, 1) ||
        !expect_compare("negative N1 vs N2", negative_a2, n2, -1) ||
        !expect_compare("negative N2 vs N3", n2, n3, -1) ||
        !expect_compare("negative N1 vs N3", negative_a2, n3, -1) ||
        !expect_compare("negative N3 vs N4", n3, negative_a1, -1) ||
        !expect_compare("negative N4 vs zero", negative_a1, zero_path, -1) ||
        !expect_compare("equal cloned paths", p1, p1_clone, 0)) {
        goto cleanup;
    }

    success = 1;

cleanup:
    lks_path_destroy(n3);
    lks_path_destroy(n2);
    lks_path_destroy(p1_clone);
    lks_path_destroy(p3);
    lks_path_destroy(p2);
    lks_path_destroy(p1);
    lks_path_destroy(positive_child);
    lks_path_destroy(positive_parent);
    lks_path_destroy(positive_a2);
    lks_path_destroy(positive_a1);
    lks_path_destroy(negative_a2);
    lks_path_destroy(negative_a1);
    return success;
}

static int run_path_level_demo(void)
{
    static const unsigned int parent_slots[] = { 3u };
    static const size_t parent_levels[] = { 0 };
    static const unsigned int a_slots[] = { 3u, 0u };
    static const size_t a_levels[] = { 0, 1 };
    static const unsigned int b_slots[] = { 3u, 0u };
    static const size_t b_levels[] = { 0, 2 };
    static const unsigned int c_slots[] = { 3u, 0u };
    static const size_t c_levels[] = { 0, 3 };
    static const unsigned int x_slots[] = { 3u, 259u, 0u };
    static const size_t x_levels[] = { 0, 1, 2 };
    static const unsigned int z_slots[] = { 3u, 259u, 1u };
    static const size_t z_levels[] = { 0, 1, 2 };
    LksPath *parent = NULL;
    LksPath *a = NULL;
    LksPath *b = NULL;
    LksPath *c = NULL;
    LksPath *x = NULL;
    LksPath *y = NULL;
    LksPath *z = NULL;
    char text[64];
    size_t level;
    LksStatus status;
    int success = 0;

    parent = make_path_at_levels(
        LKS_DIRECTION_POSITIVE, parent_slots, parent_levels, 1
    );
    if (parent != NULL) {
        a = lks_path_clone(parent);
    }
    if (a != NULL && lks_path_append(a, 0u) != LKS_STATUS_OK) {
        lks_path_destroy(a);
        a = NULL;
    }
    b = make_path_at_levels(
        LKS_DIRECTION_POSITIVE, b_slots, b_levels, 2
    );
    c = make_path_at_levels(
        LKS_DIRECTION_POSITIVE, c_slots, c_levels, 2
    );
    x = make_path_at_levels(
        LKS_DIRECTION_POSITIVE, x_slots, x_levels, 3
    );
    if (x != NULL) {
        y = lks_path_clone(x);
    }
    if (y != NULL && lks_path_append_at_level(y, 0u, 3) != LKS_STATUS_OK) {
        lks_path_destroy(y);
        y = NULL;
    }
    z = make_path_at_levels(
        LKS_DIRECTION_POSITIVE, z_slots, z_levels, 3
    );

    if (parent == NULL || a == NULL || b == NULL || c == NULL ||
        x == NULL || y == NULL || z == NULL) {
        goto cleanup;
    }

    printf("Path level tests:\n");
    if (lks_path_get_level(a, 1, &level) != LKS_STATUS_OK || level != 1 ||
        lks_path_get_level(b, 1, &level) != LKS_STATUS_OK || level != 2 ||
        lks_path_get_level(c, 1, &level) != LKS_STATUS_OK || level != 3) {
        goto cleanup;
    }
    if (lks_path_format(a, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3/A0") != 0) {
        goto cleanup;
    }
    printf("Level A: %s\n", text);
    if (lks_path_format(b, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3//A0") != 0) {
        goto cleanup;
    }
    printf("Level B: %s\n", text);
    if (lks_path_format(c, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3///A0") != 0) {
        goto cleanup;
    }
    printf("Level C: %s\n", text);

    if (!expect_compare("Parent vs level 3", parent, c, -1) ||
        !expect_compare("Level 3 vs level 2", c, b, -1) ||
        !expect_compare("Level 2 vs level 1", b, a, -1) ||
        !expect_compare("Parent vs level 1", parent, a, -1)) {
        goto cleanup;
    }

    if (lks_path_format(x, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3/Z9//A0") != 0) {
        goto cleanup;
    }
    printf("Complex X: %s\n", text);
    if (lks_path_format(y, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3/Z9//A0///A0") != 0) {
        goto cleanup;
    }
    printf("Complex Y: %s\n", text);
    if (lks_path_format(z, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, "0A3/Z9//A1") != 0) {
        goto cleanup;
    }
    printf("Complex Z: %s\n", text);
    if (!expect_compare("Complex X vs Y", x, y, -1) ||
        !expect_compare("Complex Y vs Z", y, z, -1) ||
        !expect_compare("Complex X vs Z", x, z, -1)) {
        goto cleanup;
    }

    status = lks_path_append_at_level(b, 5u, 2);
    printf("Append duplicate level: %s\n", lks_status_string(status));
    if (status != LKS_STATUS_INVALID_ARGUMENT || lks_path_depth(b) != 2) {
        goto cleanup;
    }
    status = lks_path_append_at_level(b, 5u, 1);
    printf("Append lower level: %s\n", lks_status_string(status));
    if (status != LKS_STATUS_INVALID_ARGUMENT || lks_path_depth(b) != 2) {
        goto cleanup;
    }
    status = lks_path_append_at_level(b, 5u, 3);
    printf("Append higher level: %s\n", lks_status_string(status));
    if (status != LKS_STATUS_OK ||
        lks_path_get_level(b, 2, &level) != LKS_STATUS_OK || level != 3) {
        goto cleanup;
    }

    success = 1;

cleanup:
    lks_path_destroy(z);
    lks_path_destroy(y);
    lks_path_destroy(x);
    lks_path_destroy(c);
    lks_path_destroy(b);
    lks_path_destroy(a);
    lks_path_destroy(parent);
    return success;
}
static int run_negative_level_demo(const LksPath *zero_path)
{
    static const unsigned int root_slots[] = { 2u };
    static const size_t root_levels[] = { 0 };
    static const unsigned int level1_slots[] = { 2u, 0u };
    static const size_t level1_levels[] = { 0, 1 };
    static const unsigned int level2_slots[] = { 2u, 0u };
    static const size_t level2_levels[] = { 0, 2 };
    static const unsigned int level3_slots[] = { 2u, 0u };
    static const size_t level3_levels[] = { 0, 3 };
    static const unsigned int nearer_root_slots[] = { 1u };
    static const size_t nearer_root_levels[] = { 0 };
    LksPath *root;
    LksPath *level1;
    LksPath *level2;
    LksPath *level3;
    LksPath *nearer_root;
    int success;

    root = make_path_at_levels(
        LKS_DIRECTION_NEGATIVE, root_slots, root_levels, 1
    );
    level1 = make_path_at_levels(
        LKS_DIRECTION_NEGATIVE, level1_slots, level1_levels, 2
    );
    level2 = make_path_at_levels(
        LKS_DIRECTION_NEGATIVE, level2_slots, level2_levels, 2
    );
    level3 = make_path_at_levels(
        LKS_DIRECTION_NEGATIVE, level3_slots, level3_levels, 2
    );
    nearer_root = make_path_at_levels(
        LKS_DIRECTION_NEGATIVE, nearer_root_slots, nearer_root_levels, 1
    );

    if (root == NULL || level1 == NULL || level2 == NULL || level3 == NULL ||
        nearer_root == NULL) {
        success = 0;
    } else {
        printf("Negative jump-level tests:\n");
        success =
            expect_compare("Negative parent vs level 3", root, level3, -1) &&
            expect_compare("Negative level 3 vs level 2", level3, level2, -1) &&
            expect_compare("Negative level 2 vs level 1", level2, level1, -1) &&
            expect_compare("Negative level 1 vs nearer root", level1, nearer_root, -1) &&
            expect_compare("Negative nearer root vs zero", nearer_root, zero_path, -1);
    }

    lks_path_destroy(nearer_root);
    lks_path_destroy(level3);
    lks_path_destroy(level2);
    lks_path_destroy(level1);
    lks_path_destroy(root);
    return success;
}
static int run_root_level_demo(const LksPath *zero_path)
{
    static const unsigned int p0_slots[] = { 0u };
    static const size_t p0_levels[] = { 0 };
    static const unsigned int p1_slots[] = { 0u };
    static const size_t p1_levels[] = { 1 };
    static const unsigned int p2_slots[] = { 0u };
    static const size_t p2_levels[] = { 2 };
    static const unsigned int p3_slots[] = { 0u };
    static const size_t p3_levels[] = { 3 };
    static const unsigned int p1a1_slots[] = { 1u };
    static const size_t p1a1_levels[] = { 1 };
    static const unsigned int p1z9_slots[] = { 259u };
    static const size_t p1z9_levels[] = { 1 };
    static const unsigned int n0_slots[] = { 259u };
    static const size_t n0_levels[] = { 0 };
    static const unsigned int n1_slots[] = { 259u };
    static const size_t n1_levels[] = { 1 };
    static const unsigned int n2_slots[] = { 259u };
    static const size_t n2_levels[] = { 2 };
    static const unsigned int n3_slots[] = { 259u };
    static const size_t n3_levels[] = { 3 };
    static const unsigned int n1z8_slots[] = { 258u };
    static const size_t n1z8_levels[] = { 1 };
    static const unsigned int n1a0_slots[] = { 0u };
    static const size_t n1a0_levels[] = { 1 };
    LksPath *p0 = NULL;
    LksPath *p1 = NULL;
    LksPath *p2 = NULL;
    LksPath *p3 = NULL;
    LksPath *p1a1 = NULL;
    LksPath *p1z9 = NULL;
    LksPath *n0 = NULL;
    LksPath *n1 = NULL;
    LksPath *n2 = NULL;
    LksPath *n3 = NULL;
    LksPath *n1z8 = NULL;
    LksPath *n1a0 = NULL;
    char text[64];
    int success = 0;

    p0 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p0_slots, p0_levels, 1);
    p1 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p1_slots, p1_levels, 1);
    p2 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p2_slots, p2_levels, 1);
    p3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p3_slots, p3_levels, 1);
    p1a1 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p1a1_slots, p1a1_levels, 1);
    p1z9 = make_path_at_levels(LKS_DIRECTION_POSITIVE, p1z9_slots, p1z9_levels, 1);
    n0 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n0_slots, n0_levels, 1);
    n1 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n1_slots, n1_levels, 1);
    n2 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n2_slots, n2_levels, 1);
    n3 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n3_slots, n3_levels, 1);
    n1z8 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n1z8_slots, n1z8_levels, 1);
    n1a0 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, n1a0_slots, n1a0_levels, 1);

    if (p0 == NULL || p1 == NULL || p2 == NULL || p3 == NULL ||
        p1a1 == NULL || p1z9 == NULL || n0 == NULL || n1 == NULL ||
        n2 == NULL || n3 == NULL || n1z8 == NULL || n1a0 == NULL) {
        goto cleanup;
    }

    printf("Root-level positive paths:\n");
    if (lks_path_format(p0, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "0A0") != 0) goto cleanup;
    printf("P0: %s\n", text);
    if (lks_path_format(p1, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "000/A0") != 0) goto cleanup;
    printf("P1: %s\n", text);
    if (lks_path_format(p2, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "000//A0") != 0) goto cleanup;
    printf("P2: %s\n", text);
    if (lks_path_format(p3, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "000///A0") != 0) goto cleanup;
    printf("P3: %s\n", text);
    if (!expect_compare("zero vs P3", zero_path, p3, -1) ||
        !expect_compare("P3 vs P2", p3, p2, -1) ||
        !expect_compare("P2 vs P1", p2, p1, -1) ||
        !expect_compare("P1 vs P0", p1, p0, -1) ||
        !expect_compare("P0 vs P1", p0, p1, 1) ||
        !expect_compare("positive level 1 A0 vs A1", p1, p1a1, -1) ||
        !expect_compare("positive level 1 A1 vs Z9", p1a1, p1z9, -1) ||
        !expect_compare("positive level 1 Z9 vs level 0 A0", p1z9, p0, -1)) {
        goto cleanup;
    }

    printf("Root-level negative paths:\n");
    if (lks_path_format(n0, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "1Z9") != 0) goto cleanup;
    printf("N0: %s\n", text);
    if (lks_path_format(n1, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "1/Z9") != 0) goto cleanup;
    printf("N1: %s\n", text);
    if (lks_path_format(n2, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "1//Z9") != 0) goto cleanup;
    printf("N2: %s\n", text);
    if (lks_path_format(n3, text, sizeof(text)) != LKS_STATUS_OK || strcmp(text, "1///Z9") != 0) goto cleanup;
    printf("N3: %s\n", text);
    if (!expect_compare("N3 vs N2", n3, n2, -1) ||
        !expect_compare("N2 vs N1", n2, n1, -1) ||
        !expect_compare("N1 vs N0", n1, n0, -1) ||
        !expect_compare("N0 vs zero", n0, zero_path, -1) ||
        !expect_compare("negative level 1 Z9 vs Z8", n1, n1z8, -1) ||
        !expect_compare("negative level 1 Z8 vs A0", n1z8, n1a0, -1) ||
        !expect_compare("negative level 1 A0 vs level 0 Z9", n1a0, n0, -1)) {
        goto cleanup;
    }

    success = 1;

cleanup:
    lks_path_destroy(n1a0);
    lks_path_destroy(n1z8);
    lks_path_destroy(n3);
    lks_path_destroy(n2);
    lks_path_destroy(n1);
    lks_path_destroy(n0);
    lks_path_destroy(p1z9);
    lks_path_destroy(p1a1);
    lks_path_destroy(p3);
    lks_path_destroy(p2);
    lks_path_destroy(p1);
    lks_path_destroy(p0);
    return success;
}
static int gap_case(
    const char *label,
    const LksPath *left,
    const LksPath *right,
    const char *expected_text,
    LksStatus expected_status
)
{
    LksPath *middle = NULL;
    LksStatus status;
    int left_comparison;
    int right_comparison;
    char text[256];

    status = lks_path_between(left, right, &middle);
    if (status != expected_status) {
        printf("%s: unexpected status %s\n", label, lks_status_string(status));
        lks_path_destroy(middle);
        return 0;
    }
    if (expected_status != LKS_STATUS_OK) {
        printf("%s: %s\n", label, lks_status_string(status));
        if (middle != NULL) {
            lks_path_destroy(middle);
            return 0;
        }
        return 1;
    }

    if (middle == NULL ||
        lks_path_format(middle, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, expected_text) != 0 ||
        lks_path_compare(left, middle, &left_comparison) != LKS_STATUS_OK ||
        lks_path_compare(middle, right, &right_comparison) != LKS_STATUS_OK ||
        left_comparison != -1 || right_comparison != -1) {
        printf("%s: candidate verification failed\n", label);
        lks_path_destroy(middle);
        return 0;
    }

    printf("%s: %s\n", label, text);
    lks_path_destroy(middle);
    return 1;
}

static int invalid_gap_case(
    const char *label,
    const LksPath *left,
    const LksPath *right
)
{
    LksPath *seed;
    LksPath *out_path;
    LksStatus status;
    int valid;

    seed = lks_path_create_zero();
    if (seed == NULL) {
        return 0;
    }
    out_path = seed;
    status = lks_path_between(left, right, &out_path);
    valid = status == LKS_STATUS_INVALID_ARGUMENT && out_path == NULL;
    printf("%s: %s, output %s\n", label, lks_status_string(status),
        out_path == NULL ? "NULL" : "not NULL");
    lks_path_destroy(seed);
    return valid;
}

static int run_gap_demo(void)
{
    static const unsigned int root3_slots[] = { 3u };
    static const size_t level0[] = { 0 };
    static const unsigned int root5_slots[] = { 5u };
    static const unsigned int root4_slots[] = { 4u };
    static const unsigned int child5_slots[] = { 3u, 5u };
    static const size_t levels01[] = { 0, 1 };
    static const unsigned int child0_slots[] = { 3u, 0u };
    static const unsigned int deep_left_slots[] = { 3u, 259u, 0u };
    static const size_t levels012[] = { 0, 1, 2 };
    static const unsigned int deep_right_slots[] = { 3u, 259u, 1u };
    static const unsigned int root0_slots[] = { 0u };
    static const unsigned int root0_level3_slots[] = { 0u };
    static const size_t level3[] = { 3 };
    static const unsigned int negative5_slots[] = { 5u };
    static const unsigned int negative2_slots[] = { 2u };
    static const unsigned int negative1_slots[] = { 1u };
    static const unsigned int negative0_slots[] = { 0u };
    static const unsigned int zero_slot_child_slots[] = { 3u, 0u };
    static const size_t levels0max[] = { 0, (size_t)-1 };
    static const unsigned int root_max0_slots[] = { 0u };
    static const unsigned int root_max1_slots[] = { 1u };
    static const size_t max_level[] = { (size_t)-1 };
    LksPath *zero = NULL;
    LksPath *p3 = NULL;
    LksPath *p4 = NULL;
    LksPath *p5 = NULL;
    LksPath *child5 = NULL;
    LksPath *child0 = NULL;
    LksPath *deep_left = NULL;
    LksPath *deep_right = NULL;
    LksPath *root0 = NULL;
    LksPath *root0_level3 = NULL;
    LksPath *negative5 = NULL;
    LksPath *negative2 = NULL;
    LksPath *negative1 = NULL;
    LksPath *negative0 = NULL;
    LksPath *limit_child = NULL;
    LksPath *max_root0 = NULL;
    LksPath *max_root1 = NULL;
    int success = 0;

    zero = lks_path_create_zero();
    p3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root3_slots, level0, 1);
    p4 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root4_slots, level0, 1);
    p5 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root5_slots, level0, 1);
    child5 = make_path_at_levels(LKS_DIRECTION_POSITIVE, child5_slots, levels01, 2);
    child0 = make_path_at_levels(LKS_DIRECTION_POSITIVE, child0_slots, levels01, 2);
    deep_left = make_path_at_levels(LKS_DIRECTION_POSITIVE, deep_left_slots, levels012, 3);
    deep_right = make_path_at_levels(LKS_DIRECTION_POSITIVE, deep_right_slots, levels012, 3);
    root0 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root0_slots, level0, 1);
    root0_level3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root0_level3_slots, level3, 1);
    negative5 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, negative5_slots, level0, 1);
    negative2 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, negative2_slots, level0, 1);
    negative1 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, negative1_slots, level0, 1);
    negative0 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, negative0_slots, level0, 1);
    limit_child = make_path_at_levels(LKS_DIRECTION_POSITIVE, zero_slot_child_slots, levels0max, 2);
    max_root0 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root_max0_slots, max_level, 1);
    max_root1 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root_max1_slots, max_level, 1);

    if (zero == NULL || p3 == NULL || p4 == NULL || p5 == NULL ||
        child5 == NULL || child0 == NULL || deep_left == NULL ||
        deep_right == NULL || root0 == NULL || root0_level3 == NULL ||
        negative5 == NULL || negative2 == NULL || negative1 == NULL ||
        negative0 == NULL || limit_child == NULL || max_root0 == NULL ||
        max_root1 == NULL) {
        goto cleanup;
    }

    printf("Path between tests:\n");
    if (!gap_case("1 root slot gap", p3, p5, "0A4", LKS_STATUS_OK) ||
        !gap_case("2 adjacent root slots", p3, p4, "0A3/A0", LKS_STATUS_OK) ||
        !gap_case("3 parent before child", p3, child5, "0A3/A0", LKS_STATUS_OK) ||
        !gap_case("4 parent before first child", p3, child0, "0A3//A0", LKS_STATUS_OK) ||
        !gap_case("5 deep adjacent siblings", deep_left, deep_right,
            "0A3/Z9//A0///A0", LKS_STATUS_OK) ||
        !gap_case("6 zero to root slot 5", zero, p5, "0A0", LKS_STATUS_OK) ||
        !gap_case("7 zero to root slot 0", zero, root0, "000/A0", LKS_STATUS_OK) ||
        !gap_case("8 zero to root level 3", zero, root0_level3,
            "000////A0", LKS_STATUS_OK) ||
        !gap_case("9 negative root slot gap", negative5, negative2,
            "1A4", LKS_STATUS_OK) ||
        !gap_case("10 adjacent negative roots", negative2, negative1,
            "1A2/A0", LKS_STATUS_OK) ||
        !gap_case("11 negative to zero", negative2, zero,
            "1A1", LKS_STATUS_OK) ||
        !gap_case("12 negative A0 to zero", negative0, zero,
            "1A0/A0", LKS_STATUS_OK) ||
        !gap_case("prefix level overflow", p3, limit_child,
            NULL, LKS_STATUS_LEVEL_LIMIT) ||
        !gap_case("append level overflow", max_root0, max_root1,
            NULL, LKS_STATUS_LEVEL_LIMIT) ||
        !invalid_gap_case("equal paths", p3, p3) ||
        !invalid_gap_case("left greater than right", p5, p3) ||
        !invalid_gap_case("negative to positive", negative2, p5) ||
        !invalid_gap_case("NULL left", NULL, p5) ||
        !invalid_gap_case("NULL right", p3, NULL)) {
        goto cleanup;
    }
    {
        LksStatus status;

        status = lks_path_between(p3, p5, NULL);
        if (status != LKS_STATUS_INVALID_ARGUMENT) {
            printf("NULL output pointer: %s\n", lks_status_string(status));
            goto cleanup;
        }
        printf("NULL output pointer: %s\n", lks_status_string(status));
    }

    success = 1;

cleanup:
    lks_path_destroy(max_root1);
    lks_path_destroy(max_root0);
    lks_path_destroy(limit_child);
    lks_path_destroy(negative0);
    lks_path_destroy(negative1);
    lks_path_destroy(negative2);
    lks_path_destroy(negative5);
    lks_path_destroy(root0_level3);
    lks_path_destroy(root0);
    lks_path_destroy(deep_right);
    lks_path_destroy(deep_left);
    lks_path_destroy(child0);
    lks_path_destroy(child5);
    lks_path_destroy(p5);
    lks_path_destroy(p4);
    lks_path_destroy(p3);
    lks_path_destroy(zero);
    return success;
}
static int placement_case(
    const char *label,
    int is_before,
    const LksPath *input,
    const char *expected_text
)
{
    LksPath *candidate = NULL;
    LksPath *zero_path = NULL;
    LksStatus status;
    int comparison;
    char text[128];
    int valid;

    status = is_before ? lks_path_before(input, &candidate) :
        lks_path_after(input, &candidate);
    if (status != LKS_STATUS_OK || candidate == NULL ||
        lks_path_format(candidate, text, sizeof(text)) != LKS_STATUS_OK ||
        strcmp(text, expected_text) != 0) {
        printf("%s: %s\n", label, lks_status_string(status));
        lks_path_destroy(candidate);
        return 0;
    }

    if (is_before) {
        valid = lks_path_compare(candidate, input, &comparison) == LKS_STATUS_OK &&
            comparison == -1;
        if (valid && lks_path_direction(input) == LKS_DIRECTION_POSITIVE) {
            zero_path = lks_path_create_zero();
            valid = zero_path != NULL &&
                lks_path_compare(zero_path, candidate, &comparison) == LKS_STATUS_OK &&
                comparison == -1;
        }
    } else {
        valid = lks_path_compare(input, candidate, &comparison) == LKS_STATUS_OK &&
            comparison == -1;
        if (valid && lks_path_direction(input) == LKS_DIRECTION_NEGATIVE) {
            zero_path = lks_path_create_zero();
            valid = zero_path != NULL &&
                lks_path_compare(candidate, zero_path, &comparison) == LKS_STATUS_OK &&
                comparison == -1;
        }
    }

    printf("%s: %s%s\n", label, text, valid ? "" : " (order check failed)");
    lks_path_destroy(zero_path);
    lks_path_destroy(candidate);
    return valid;
}

static int placement_status_case(
    const char *label,
    int is_before,
    const LksPath *input,
    LksStatus expected_status
)
{
    LksPath *seed;
    LksPath *out_path;
    LksStatus status;
    int valid;

    seed = lks_path_create_zero();
    if (seed == NULL) {
        return 0;
    }
    out_path = seed;
    status = is_before ? lks_path_before(input, &out_path) :
        lks_path_after(input, &out_path);
    valid = status == expected_status && out_path == NULL;
    printf("%s: %s, output %s\n", label, lks_status_string(status),
        out_path == NULL ? "NULL" : "not NULL");
    lks_path_destroy(seed);
    return valid;
}

static int run_placement_demo(void)
{
    static const unsigned int slot0[] = { 0u };
    static const unsigned int root3_slots[] = { 3u };
    static const unsigned int slot5[] = { 5u };
    static const unsigned int slot2[] = { 2u };
    static const unsigned int slot259[] = { 259u };
    static const unsigned int deep_slots[] = { 3u, 259u };
    static const size_t level0[] = { 0 };
    static const size_t level2[] = { 2 };
    static const size_t level3[] = { 3 };
    static const size_t levels01[] = { 0, 1 };
    static const size_t max_level[] = { (size_t)-1 };
    LksPath *zero = NULL;
    LksPath *pos0 = NULL;
    LksPath *pos3 = NULL;
    LksPath *pos5 = NULL;
    LksPath *pos_level3 = NULL;
    LksPath *negative2 = NULL;
    LksPath *negative_z9 = NULL;
    LksPath *negative_z9_level2 = NULL;
    LksPath *negative0 = NULL;
    LksPath *deep = NULL;
    LksPath *after_pos3 = NULL;
    LksPath *negative_z9_max = NULL;
    LksPath *positive_max = NULL;
    int success = 0;
    LksStatus status;

    zero = lks_path_create_zero();
    pos0 = make_path_at_levels(LKS_DIRECTION_POSITIVE, slot0, level0, 1);
    pos3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root3_slots, level0, 1);
    pos5 = make_path_at_levels(LKS_DIRECTION_POSITIVE, slot5, level0, 1);
    pos_level3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, slot0, level3, 1);
    negative2 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, slot2, level0, 1);
    negative_z9 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, slot259, level0, 1);
    negative_z9_level2 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, slot259, level2, 1);
    negative0 = make_path_at_levels(LKS_DIRECTION_NEGATIVE, slot0, level0, 1);
    deep = make_path_at_levels(LKS_DIRECTION_POSITIVE, deep_slots, levels01, 2);
    after_pos3 = make_path_at_levels(LKS_DIRECTION_POSITIVE, root3_slots, level0, 1);
    negative_z9_max = make_path_at_levels(LKS_DIRECTION_NEGATIVE, slot259, max_level, 1);
    positive_max = make_path_at_levels(LKS_DIRECTION_POSITIVE, slot0, max_level, 1);

    if (zero == NULL || pos0 == NULL || pos3 == NULL || pos5 == NULL || pos_level3 == NULL ||
        negative2 == NULL || negative_z9 == NULL || negative_z9_level2 == NULL ||
        negative0 == NULL || deep == NULL || after_pos3 == NULL || negative_z9_max == NULL ||
        positive_max == NULL) {
        goto cleanup;
    }

    printf("Before/after tests:\n");
    if (!placement_case("before(000)", 1, zero, "1A0") ||
        !placement_case("before(0A5)", 1, pos5, "0A0") ||
        !placement_case("before(0A0)", 1, pos0, "000/A0") ||
        !placement_case("before(root level 3)", 1, pos_level3, "000////A0") ||
        !placement_case("before(1A2)", 1, negative2, "1A3") ||
        !placement_case("before(1Z9)", 1, negative_z9, "1/Z9") ||
        !placement_case("before(level 2 Z9)", 1, negative_z9_level2, "1///Z9") ||
        !placement_case("after(000)", 0, zero, "0A0") ||
        !placement_case("after(0A3)", 0, after_pos3, "0A3/A0") ||
        !placement_case("after(0A3/Z9)", 0, deep, "0A3/Z9//A0") ||
        !placement_case("after(1A2)", 0, negative2, "1A2/A0") ||
        !placement_case("after(1A0)", 0, negative0, "1A0/A0")) {
        goto cleanup;
    }

    if (!placement_status_case("before(NULL)", 1, NULL, LKS_STATUS_INVALID_ARGUMENT) ||
        !placement_status_case("after(NULL)", 0, NULL, LKS_STATUS_INVALID_ARGUMENT) ||
        !placement_status_case("before level limit", 1, negative_z9_max, LKS_STATUS_LEVEL_LIMIT) ||
        !placement_status_case("after level limit", 0, positive_max, LKS_STATUS_LEVEL_LIMIT)) {
        goto cleanup;
    }
    status = lks_path_before(pos0, NULL);
    printf("before(NULL out_path): %s\n", lks_status_string(status));
    if (status != LKS_STATUS_INVALID_ARGUMENT) {
        goto cleanup;
    }
    status = lks_path_after(pos0, NULL);
    printf("after(NULL out_path): %s\n", lks_status_string(status));
    if (status != LKS_STATUS_INVALID_ARGUMENT) {
        goto cleanup;
    }

    success = 1;

cleanup:
    lks_path_destroy(after_pos3);
    lks_path_destroy(positive_max);
    lks_path_destroy(negative_z9_max);
    lks_path_destroy(deep);
    lks_path_destroy(negative0);
    lks_path_destroy(negative_z9_level2);
    lks_path_destroy(negative_z9);
    lks_path_destroy(negative2);
    lks_path_destroy(pos_level3);
    lks_path_destroy(pos5);
    lks_path_destroy(pos3);
    lks_path_destroy(pos0);
    lks_path_destroy(zero);
    return success;
}
static LksPath *tree_test_path(
    LksDirection direction,
    unsigned int first_slot,
    size_t first_level,
    const unsigned int *tail_slots,
    const size_t *tail_levels,
    size_t tail_count
)
{
    LksPath *path;
    size_t index;

    if (direction == LKS_DIRECTION_ZERO) {
        return lks_path_create_zero();
    }
    path = lks_path_create_at_level(direction, first_slot, first_level);
    if (path == NULL) {
        return NULL;
    }
    for (index = 0; index < tail_count; ++index) {
        if (lks_path_append_at_level(path, tail_slots[index],
                tail_levels[index]) != LKS_STATUS_OK) {
            lks_path_destroy(path);
            return NULL;
        }
    }
    return path;
}

static int tree_path_is(const LksPath *path, const char *expected)
{
    char text[128];

    return path != NULL &&
        lks_path_format(path, text, sizeof(text)) == LKS_STATUS_OK &&
        strcmp(text, expected) == 0;
}

static int run_tree_demo(void)
{
    static const char *const root_expected[] = {
        "1A2", "1A1", "000", "000//A0", "000/A0", "0A0", "0A3"
    };
    static const unsigned int levels_tail_slots[] = { 259u, 0u, 0u };
    static const size_t levels_tail_levels[] = { 1, 2, 3 };
    LksTree *tree = NULL;
    LksTree *sparse_tree = NULL;
    LksPath *paths[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    LksPath *path = NULL;
    LksPath *parent_path = NULL;
    LksPath *child_path = NULL;
    const LksTreeNode *node = NULL;
    const LksTreeNode *parent = NULL;
    const LksTreeNode *found = NULL;
    const LksTreeNode *root_nodes[7];
    size_t index;
    int borrowed_value;
    LksStatus status;

    tree = lks_tree_create();
    if (tree == NULL || lks_tree_size(tree) != 0 ||
        lks_tree_root_child_count(tree) != 0) {
        lks_tree_destroy(tree);
        return 0;
    }
    lks_tree_destroy(tree);
    printf("Tree A empty: size 0, root children 0\n");

    tree = lks_tree_create();
    if (tree == NULL) {
        return 0;
    }
    paths[0] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    paths[1] = lks_path_create_zero();
    paths[2] = tree_test_path(LKS_DIRECTION_NEGATIVE, 1u, 0, NULL, NULL, 0);
    paths[3] = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 1, NULL, NULL, 0);
    paths[4] = tree_test_path(LKS_DIRECTION_NEGATIVE, 2u, 0, NULL, NULL, 0);
    paths[5] = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 0, NULL, NULL, 0);
    paths[6] = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 2, NULL, NULL, 0);
    for (index = 0; index < 7; ++index) {
        if (paths[index] == NULL || lks_tree_insert(tree, paths[index], NULL,
                NULL) != LKS_STATUS_OK) {
            goto fail;
        }
    }
    if (lks_tree_size(tree) != 7 || lks_tree_root_child_count(tree) != 7) {
        goto fail;
    }
    printf("Tree B root order:");
    for (index = 0; index < 7; ++index) {
        int compare_status;
        root_nodes[index] = lks_tree_root_child_at(tree, index);
        if (root_nodes[index] == NULL ||
            !tree_path_is(lks_tree_node_path(root_nodes[index]),
                root_expected[index])) {
            goto fail;
        }
        if (index > 0 && (lks_path_compare(
                lks_tree_node_path(root_nodes[index - 1]),
                lks_tree_node_path(root_nodes[index]), &compare_status) !=
                LKS_STATUS_OK || compare_status != -1)) {
            goto fail;
        }
        printf(" %s", root_expected[index]);
    }
    printf("\n");
    for (index = 0; index < 7; ++index) {
        lks_path_destroy(paths[index]);
        paths[index] = NULL;
    }
    lks_tree_destroy(tree);

    sparse_tree = lks_tree_create();
    path = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 0, NULL, NULL, 0);
    child_path = tree_test_path(LKS_DIRECTION_POSITIVE, 259u, 0, NULL, NULL, 0);
    if (sparse_tree == NULL || path == NULL || child_path == NULL ||
        lks_tree_insert(sparse_tree, path, NULL, NULL) != LKS_STATUS_OK ||
        lks_tree_insert(sparse_tree, child_path, NULL, NULL) != LKS_STATUS_OK ||
        lks_tree_size(sparse_tree) != 2 ||
        lks_tree_root_child_count(sparse_tree) != 2) {
        lks_path_destroy(path);
        lks_path_destroy(child_path);
        lks_tree_destroy(sparse_tree);
        goto fail;
    }
    printf("Tree C sparse root: 2 nodes, 2 children\n");
    lks_path_destroy(path);
    lks_path_destroy(child_path);
    lks_tree_destroy(sparse_tree);

    tree = lks_tree_create();
    parent_path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    if (tree == NULL || parent_path == NULL ||
        lks_tree_insert(tree, parent_path, NULL, &parent) != LKS_STATUS_OK) {
        lks_path_destroy(parent_path);
        lks_tree_destroy(tree);
        goto fail;
    }
    lks_path_destroy(parent_path);
    paths[0] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 0u }, (const size_t[]){ 1 }, 1);
    paths[1] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 0u }, (const size_t[]){ 2 }, 1);
    paths[2] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 259u }, (const size_t[]){ 1 }, 1);
    for (index = 0; index < 3; ++index) {
        if (paths[index] == NULL || lks_tree_insert(tree, paths[index], NULL,
                NULL) != LKS_STATUS_OK) {
            goto fail;
        }
    }
    if (lks_tree_node_child_count(parent) != 3 ||
        !tree_path_is(lks_tree_node_path(lks_tree_node_child_at(parent, 0)),
            "0A3//A0") ||
        !tree_path_is(lks_tree_node_path(lks_tree_node_child_at(parent, 1)),
            "0A3/A0") ||
        !tree_path_is(lks_tree_node_path(lks_tree_node_child_at(parent, 2)),
            "0A3/Z9")) {
        goto fail;
    }
    printf("Tree D children of 0A3: 0A3//A0, 0A3/A0, 0A3/Z9\n");
    for (index = 0; index < 3; ++index) {
        lks_path_destroy(paths[index]);
        paths[index] = NULL;
    }
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    paths[0] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    paths[1] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 259u }, (const size_t[]){ 1 }, 1);
    paths[2] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 259u, 0u }, (const size_t[]){ 1, 2 }, 2);
    paths[3] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        levels_tail_slots, levels_tail_levels, 3);
    if (tree == NULL) {
        goto fail;
    }
    for (index = 0; index < 4; ++index) {
        if (paths[index] == NULL || lks_tree_insert(tree, paths[index], NULL,
                NULL) != LKS_STATUS_OK) {
            goto fail;
        }
    }
    if (lks_tree_find_path(tree, paths[1], &node) != LKS_STATUS_OK ||
        lks_tree_find_path(tree, paths[2], &found) != LKS_STATUS_OK ||
        lks_tree_find_path(tree, paths[3], &parent) != LKS_STATUS_OK ||
        lks_tree_node_parent(found) != node ||
        lks_tree_node_parent(parent) != found ||
        lks_tree_node_parent(node) != lks_tree_root_child_at(tree, 0)) {
        goto fail;
    }
    printf("Tree E parent chain: 0A3 <- 0A3/Z9 <- 0A3/Z9//A0 <- 0A3/Z9//A0///A0\n");
    for (index = 0; index < 4; ++index) {
        lks_path_destroy(paths[index]);
        paths[index] = NULL;
    }
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = lks_path_create_zero();
    child_path = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 1, NULL, NULL, 0);
    if (tree == NULL || path == NULL || child_path == NULL ||
        lks_tree_insert(tree, path, NULL, &node) != LKS_STATUS_OK ||
        lks_tree_insert(tree, child_path, NULL, &found) != LKS_STATUS_OK ||
        lks_tree_node_parent(node) != NULL || lks_tree_node_parent(found) != NULL ||
        lks_tree_root_child_count(tree) != 2) {
        lks_path_destroy(path);
        lks_path_destroy(child_path);
        lks_tree_destroy(tree);
        goto fail;
    }
    printf("Tree F 000 and 000/A0: both virtual-root children\n");
    lks_path_destroy(path);
    lks_path_destroy(child_path);
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = tree_test_path(LKS_DIRECTION_POSITIVE, 1u, 0,
        (const unsigned int[]){ 0u }, (const size_t[]){ 1 }, 1);
    node = (const LksTreeNode *)1;
    status = lks_tree_insert(tree, path, NULL, &node);
    if (tree == NULL || path == NULL || status != LKS_STATUS_NOT_FOUND ||
        node != NULL || lks_tree_size(tree) != 0) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    printf("Tree G missing parent: Not found, size 0, output NULL\n");
    lks_path_destroy(path);
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    if (tree == NULL || path == NULL ||
        lks_tree_insert(tree, path, NULL, &node) != LKS_STATUS_OK) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    found = (const LksTreeNode *)1;
    status = lks_tree_insert(tree, path, NULL, &found);
    if (status != LKS_STATUS_ALREADY_EXISTS || found != NULL ||
        lks_tree_size(tree) != 1 ||
        strcmp(lks_status_string(status), "Already exists") != 0) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    printf("Tree H duplicate: Already exists, size 1, output NULL\n");
    lks_path_destroy(path);
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    if (tree == NULL || path == NULL ||
        lks_tree_insert(tree, path, NULL, &node) != LKS_STATUS_OK) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    lks_path_destroy(path);
    if (!tree_path_is(lks_tree_node_path(node), "0A3")) {
        lks_tree_destroy(tree);
        goto fail;
    }
    printf("Tree I cloned Path survives source destruction: 0A3\n");
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = lks_path_create_zero();
    borrowed_value = 123;
    if (tree == NULL || path == NULL ||
        lks_tree_insert(tree, path, &borrowed_value, &node) != LKS_STATUS_OK ||
        lks_tree_node_item(node) != &borrowed_value) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    lks_path_destroy(path);
    lks_tree_destroy(tree);
    if (borrowed_value != 123) {
        goto fail;
    }
    printf("Tree J borrowed item remains valid: 123\n");

    tree = lks_tree_create();
    paths[0] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
    paths[1] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 259u }, (const size_t[]){ 1 }, 1);
    paths[2] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        (const unsigned int[]){ 259u, 0u }, (const size_t[]){ 1, 2 }, 2);
    paths[3] = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
        levels_tail_slots, levels_tail_levels, 3);
    for (index = 0; index < 4; ++index) {
        if (tree == NULL || paths[index] == NULL ||
            lks_tree_insert(tree, paths[index], NULL, NULL) != LKS_STATUS_OK ||
            lks_tree_find_path(tree, paths[index], &node) != LKS_STATUS_OK ||
            !tree_path_is(lks_tree_node_path(node), index == 0 ? "0A3" :
                index == 1 ? "0A3/Z9" : index == 2 ? "0A3/Z9//A0" :
                "0A3/Z9//A0///A0")) {
            goto fail;
        }
    }
    path = tree_test_path(LKS_DIRECTION_POSITIVE, 4u, 0, NULL, NULL, 0);
    found = (const LksTreeNode *)1;
    status = lks_tree_find_path(tree, path, &found);
    if (status != LKS_STATUS_NOT_FOUND || found != NULL) {
        goto fail;
    }
    printf("Tree K exact find: 4 paths found; absent 0A4 -> Not found\n");
    for (index = 0; index < 4; ++index) {
        lks_path_destroy(paths[index]);
        paths[index] = NULL;
    }
    lks_path_destroy(path);
    lks_tree_destroy(tree);

    tree = lks_tree_create();
    path = lks_path_create_zero();
    node = (const LksTreeNode *)1;
    if (lks_tree_insert(NULL, path, NULL, &node) != LKS_STATUS_INVALID_ARGUMENT ||
        node != NULL || lks_tree_insert(tree, NULL, NULL, &node) !=
            LKS_STATUS_INVALID_ARGUMENT || node != NULL ||
        lks_tree_find_path(NULL, path, &found) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_find_path(tree, NULL, &found) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_find_path(tree, path, NULL) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_size(NULL) != 0 || lks_tree_root_child_count(NULL) != 0 ||
        lks_tree_root_child_at(NULL, 0) != NULL ||
        lks_tree_node_path(NULL) != NULL || lks_tree_node_item(NULL) != NULL ||
        lks_tree_node_parent(NULL) != NULL ||
        lks_tree_node_child_count(NULL) != 0 ||
        lks_tree_node_child_at(NULL, 0) != NULL) {
        lks_path_destroy(path);
        lks_tree_destroy(tree);
        goto fail;
    }
    lks_path_destroy(path);
    lks_tree_destroy(tree);
    lks_tree_destroy(NULL);
    printf("Tree L invalid arguments and NULL getters: passed\n");
    return 1;

fail:
    printf("Tree tests: FAILED\n");
    return 0;
}
typedef struct CompareCounter {
    size_t calls;
} CompareCounter;

static int compare_int_counted(
    const void *left,
    const void *right,
    void *context
)
{
    CompareCounter *counter;

    counter = (CompareCounter *)context;
    ++counter->calls;
    return compare_int(left, right, NULL);
}

static int item_node_matches(
    const LksTreeNode *node,
    int should_exist,
    int expected_value
)
{
    if (!should_exist) {
        return node == NULL;
    }
    return node != NULL && lks_tree_node_item(node) != NULL &&
        *(const int *)lks_tree_node_item(node) == expected_value;
}

static int expect_item_location(
    const LksTree *tree,
    const LksComparator *comparator,
    int target,
    int has_left,
    int left_value,
    int has_equal,
    int equal_value,
    int has_right,
    int right_value,
    const char *label
)
{
    const LksTreeNode *left;
    const LksTreeNode *equal;
    const LksTreeNode *right;
    LksStatus status;

    left = NULL;
    equal = NULL;
    right = NULL;
    status = lks_tree_locate_item(tree, &target, comparator,
        &left, &equal, &right);
    if (status != LKS_STATUS_OK ||
        !item_node_matches(left, has_left, left_value) ||
        !item_node_matches(equal, has_equal, equal_value) ||
        !item_node_matches(right, has_right, right_value)) {
        printf("Locate %s: FAILED (%s)\n", label, lks_status_string(status));
        return 0;
    }

    printf("Locate %s: ", label);
    if (has_left) {
        printf("%d", left_value);
    } else {
        printf("NULL");
    }
    if (has_equal) {
        printf(" == %d", target);
    } else {
        printf(" < %d < ", target);
        if (has_right) {
            printf("%d", right_value);
        } else {
            printf("NULL");
        }
    }
    printf("\n");
    return 1;
}

static int run_tree_locate_demo(void)
{
    static const unsigned int child_slots1[] = { 0u };
    static const size_t child_levels2[] = { 2 };
    static const unsigned int child_slots2[] = { 259u };
    static const size_t child_levels1[] = { 1 };
    static const unsigned int deep_slots[] = { 259u, 0u };
    static const size_t deep_levels2[] = { 1, 2 };
    static const unsigned int deepest_slots[] = { 259u, 0u, 0u };
    static const size_t deepest_levels[] = { 1, 2, 3 };
    static const int expected_values[] = {
        10, 20, 30, 40, 50, 60, 100, 110, 120, 200, 210, 215, 300
    };
    LksTree *tree;
    LksTree *empty_tree;
    LksTree *wide_tree;
    LksPath *path;
    const LksTreeNode *node;
    const LksTreeNode *left;
    const LksTreeNode *equal;
    const LksTreeNode *right;
    LksComparator comparator;
    LksComparator invalid_comparator;
    CompareCounter counter;
    int values[13];
    int wide_values[128];
    int target;
    size_t index;
    LksStatus status;
    int success;

    tree = lks_tree_create();
    if (tree == NULL) {
        return 0;
    }
    comparator.compare = compare_int;
    comparator.context = NULL;
    for (index = 0; index < 13; ++index) {
        values[index] = expected_values[index];
        switch (index) {
        case 0:
            path = tree_test_path(LKS_DIRECTION_NEGATIVE, 2u, 0, NULL, NULL, 0);
            break;
        case 1:
            path = tree_test_path(LKS_DIRECTION_NEGATIVE, 1u, 0, NULL, NULL, 0);
            break;
        case 2:
            path = lks_path_create_zero();
            break;
        case 3:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 2, NULL, NULL, 0);
            break;
        case 4:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 1, NULL, NULL, 0);
            break;
        case 5:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 0u, 0, NULL, NULL, 0);
            break;
        case 6:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0, NULL, NULL, 0);
            break;
        case 7:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
                child_slots1, child_levels2, 1);
            break;
        case 8:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
                child_slots1, child_levels1, 1);
            break;
        case 9:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
                child_slots2, child_levels1, 1);
            break;
        case 10:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
                deep_slots, deep_levels2, 2);
            break;
        case 11:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 3u, 0,
                deepest_slots, deepest_levels, 3);
            break;
        default:
            path = tree_test_path(LKS_DIRECTION_POSITIVE, 4u, 0, NULL, NULL, 0);
            break;
        }
        if (path == NULL || lks_tree_insert(tree, path, &values[index],
                &node) != LKS_STATUS_OK) {
            lks_path_destroy(path);
            lks_tree_destroy(tree);
            return 0;
        }
        lks_path_destroy(path);
    }

    success =
        expect_item_location(tree, &comparator, 5, 0, 0, 0, 0, 1, 10, "5") &&
        expect_item_location(tree, &comparator, 15, 1, 10, 0, 0, 1, 20, "15") &&
        expect_item_location(tree, &comparator, 25, 1, 20, 0, 0, 1, 30, "25") &&
        expect_item_location(tree, &comparator, 35, 1, 30, 0, 0, 1, 40, "35") &&
        expect_item_location(tree, &comparator, 55, 1, 50, 0, 0, 1, 60, "55") &&
        expect_item_location(tree, &comparator, 90, 1, 60, 0, 0, 1, 100, "90") &&
        expect_item_location(tree, &comparator, 105, 1, 100, 0, 0, 1, 110, "105") &&
        expect_item_location(tree, &comparator, 115, 1, 110, 0, 0, 1, 120, "115") &&
        expect_item_location(tree, &comparator, 150, 1, 120, 0, 0, 1, 200, "150") &&
        expect_item_location(tree, &comparator, 205, 1, 200, 0, 0, 1, 210, "205") &&
        expect_item_location(tree, &comparator, 212, 1, 210, 0, 0, 1, 215, "212") &&
        expect_item_location(tree, &comparator, 250, 1, 215, 0, 0, 1, 300, "250") &&
        expect_item_location(tree, &comparator, 350, 1, 300, 0, 0, 0, 0, "350") &&
        expect_item_location(tree, &comparator, 30, 0, 0, 1, 30, 0, 0, "equal 30") &&
        expect_item_location(tree, &comparator, 120, 0, 0, 1, 120, 0, 0, "equal 120") &&
        expect_item_location(tree, &comparator, 215, 0, 0, 1, 215, 0, 0, "equal 215") &&
        expect_item_location(tree, &comparator, 300, 0, 0, 1, 300, 0, 0, "equal 300");
    lks_tree_destroy(tree);
    if (!success) {
        return 0;
    }

    empty_tree = lks_tree_create();
    if (empty_tree == NULL) {
        return 0;
    }
    target = 100;
    left = (const LksTreeNode *)1;
    equal = (const LksTreeNode *)1;
    right = (const LksTreeNode *)1;
    status = lks_tree_locate_item(empty_tree, &target, &comparator,
        &left, &equal, &right);
    lks_tree_destroy(empty_tree);
    if (status != LKS_STATUS_OK || left != NULL || equal != NULL || right != NULL) {
        return 0;
    }
    printf("Locate empty tree: OK, NULL / NULL / NULL\n");

    empty_tree = lks_tree_create();
    if (empty_tree == NULL) {
        return 0;
    }
    invalid_comparator = comparator;
    invalid_comparator.compare = NULL;
    target = 1;
    if (lks_tree_locate_item(NULL, &target, &comparator,
            &left, &equal, &right) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_locate_item(empty_tree, &target, NULL,
            &left, &equal, &right) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_locate_item(empty_tree, &target, &invalid_comparator,
            &left, &equal, &right) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_locate_item(empty_tree, &target, &comparator,
            NULL, &equal, &right) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_locate_item(empty_tree, &target, &comparator,
            &left, NULL, &right) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_tree_locate_item(empty_tree, &target, &comparator,
            &left, &equal, NULL) != LKS_STATUS_INVALID_ARGUMENT) {
        lks_tree_destroy(empty_tree);
        return 0;
    }
    lks_tree_destroy(empty_tree);
    printf("Locate invalid arguments: all six rejected\n");

    wide_tree = lks_tree_create();
    if (wide_tree == NULL) {
        return 0;
    }
    counter.calls = 0;
    comparator.compare = compare_int_counted;
    comparator.context = &counter;
    for (index = 0; index < 128; ++index) {
        wide_values[index] = (int)(index * 2);
        path = tree_test_path(LKS_DIRECTION_POSITIVE, (unsigned int)index,
            0, NULL, NULL, 0);
        if (path == NULL || lks_tree_insert(wide_tree, path, &wide_values[index],
                NULL) != LKS_STATUS_OK) {
            lks_path_destroy(path);
            lks_tree_destroy(wide_tree);
            return 0;
        }
        lks_path_destroy(path);
    }
    target = 127;
    counter.calls = 0;
    status = lks_tree_locate_item(wide_tree, &target, &comparator,
        &left, &equal, &right);
    if (status != LKS_STATUS_OK || !item_node_matches(left, 1, 126) ||
        equal != NULL || !item_node_matches(right, 1, 128) ||
        counter.calls >= 20) {
        lks_tree_destroy(wide_tree);
        return 0;
    }
    printf("Locate 128 root children: 126 < 127 < 128; comparisons %lu\n",
        (unsigned long)counter.calls);
    lks_tree_destroy(wide_tree);
    return 1;
}
typedef struct TreeWalkCheck {
    const LksComparator *comparator;
    const LksTreeNode *previous;
    int *values;
    const LksTreeNode **nodes;
    size_t count;
    size_t capacity;
    int allow_equal_items;
    int valid;
} TreeWalkCheck;

static void tree_walk_node(const LksTreeNode *node, TreeWalkCheck *check)
{
    size_t index;
    const LksPath *path;

    if (!check->valid || node == NULL || check->count >= check->capacity) {
        check->valid = 0;
        return;
    }
    path = lks_tree_node_path(node);
    if (path == NULL || lks_tree_node_item(node) == NULL) {
        check->valid = 0;
        return;
    }
    if (check->previous != NULL) {
        int item_order;
        int path_order;

        item_order = check->comparator->compare(
            lks_tree_node_item(check->previous),
            lks_tree_node_item(node),
            check->comparator->context);
        if (item_order > 0 || (!check->allow_equal_items && item_order == 0) ||
            lks_path_compare(lks_tree_node_path(check->previous), path,
                &path_order) != LKS_STATUS_OK || path_order != -1) {
            check->valid = 0;
            return;
        }
    }

    check->values[check->count] = *(const int *)lks_tree_node_item(node);
    check->nodes[check->count] = node;
    ++check->count;
    check->previous = node;
    for (index = 0; index < lks_tree_node_child_count(node); ++index) {
        tree_walk_node(lks_tree_node_child_at(node, index), check);
        if (!check->valid) {
            return;
        }
    }
}

static int verify_tree_walk(
    const LksTree *tree,
    const LksComparator *comparator,
    const int *expected,
    size_t expected_count,
    int allow_equal_items,
    const char *label
)
{
    int actual[32];
    const LksTreeNode *nodes[32];
    TreeWalkCheck check;
    size_t index;

    check.comparator = comparator;
    check.previous = NULL;
    check.values = actual;
    check.nodes = nodes;
    check.count = 0;
    check.capacity = sizeof(actual) / sizeof(actual[0]);
    check.allow_equal_items = allow_equal_items;
    check.valid = 1;

    for (index = 0; index < lks_tree_root_child_count(tree); ++index) {
        tree_walk_node(lks_tree_root_child_at(tree, index), &check);
        if (!check.valid) {
            break;
        }
    }
    if (!check.valid || check.count != expected_count) {
        printf("%s traversal: FAILED (count %lu)\n", label,
            (unsigned long)check.count);
        return 0;
    }
    for (index = 0; index < expected_count; ++index) {
        if (actual[index] != expected[index]) {
            printf("%s traversal: FAILED at index %lu\n", label,
                (unsigned long)index);
            return 0;
        }
    }
    printf("%s traversal:", label);
    for (index = 0; index < check.count; ++index) {
        printf(" %d", actual[index]);
    }
    printf("\n");
    return 1;
}

static int insert_item_and_check_path(
    LksTree *tree,
    int *item,
    const LksComparator *comparator,
    const char *expected_path,
    int check_exact_path,
    const char *label
)
{
    const LksTreeNode *node;
    const LksPath *node_path;
    char text[128];
    LksStatus status;

    node = NULL;
    status = lks_tree_insert_item(tree, item, comparator, &node);
    if (status != LKS_STATUS_OK || node == NULL ||
        lks_tree_node_item(node) != item) {
        printf("%s insert: %s\n", label, lks_status_string(status));
        return 0;
    }
    node_path = lks_tree_node_path(node);
    if (node_path == NULL || lks_path_format(node_path, text, sizeof(text)) !=
            LKS_STATUS_OK ||
        (check_exact_path && strcmp(text, expected_path) != 0)) {
        printf("%s Path mismatch\n", label);
        return 0;
    }
    printf("%s -> %s\n", label, text);
    return 1;
}

static int run_tree_insert_item_demo(void)
{
    static const int input_a[] = { 100, 50, 150, 125, 75, 25, 175, 160, 170, 165 };
    static const char *const paths_a[] = {
        "000", "1A0", "0A0", "000/A0", "1A0/A0",
        "1A1", "0A0/A0", "0A0//A0", "0A0//A1", "0A0//A0///A0"
    };
    static const int order_a[] = { 25, 50, 75, 100, 125, 150, 160, 165, 170, 175 };
    static const int input_b[] = { 100, 125, 100, 100, 150, 125 };
    static const int order_b[] = { 100, 100, 100, 125, 125, 150 };
    static const int input_c[] = { 10, 20, 30, 40, 50 };
    static const char *const paths_c[] = {
        "000", "0A0", "0A0/A0", "0A0/A0//A0", "0A0/A0//A0///A0"
    };
    static const int order_c[] = { 10, 20, 30, 40, 50 };
    static const int input_d[] = { 50, 40, 30, 20, 10 };
    static const char *const paths_d[] = { "000", "1A0", "1A1", "1A2", "1A3" };
    static const int order_d[] = { 10, 20, 30, 40, 50 };
    static const int input_e[] = { 100, 100 };
    static const int order_e[] = { 100, 100 };
    LksComparator comparator;
    LksComparator no_function;
    LksTree *tree;
    LksTree *duplicate_tree;
    LksTree *ascending_tree;
    LksTree *descending_tree;
    LksTree *equal_max_tree;
    LksTree *null_output_tree;
    const LksTreeNode *node;
    const LksTreeNode *left;
    const LksTreeNode *equal;
    const LksTreeNode *right;
    int values_a[10];
    int values_b[6];
    int values_c[5];
    int values_d[5];
    int values_e[2];
    int borrowed;
    size_t index;
    int path_comparison;
    int valid;

    comparator.compare = compare_int;
    comparator.context = NULL;

    tree = lks_tree_create();
    if (tree == NULL) {
        return 0;
    }
    valid = 1;
    for (index = 0; index < 10; ++index) {
        values_a[index] = input_a[index];
        if (!insert_item_and_check_path(tree, &values_a[index], &comparator,
                paths_a[index], 1, "A")) {
            valid = 0;
            break;
        }
    }
    if (valid) {
        valid = verify_tree_walk(tree, &comparator, order_a, 10, 0, "A");
    }
    lks_tree_destroy(tree);
    if (!valid) {
        return 0;
    }

    duplicate_tree = lks_tree_create();
    if (duplicate_tree == NULL) {
        return 0;
    }
    valid = 1;
    for (index = 0; index < 6; ++index) {
        values_b[index] = input_b[index];
        if (!insert_item_and_check_path(duplicate_tree, &values_b[index],
                &comparator, index == 2 ? "000/A0" : NULL,
                index == 2, "B")) {
            valid = 0;
            break;
        }
    }
    if (valid) {
        valid = lks_tree_size(duplicate_tree) == 6 &&
            verify_tree_walk(duplicate_tree, &comparator, order_b, 6, 1, "B");
    }
    lks_tree_destroy(duplicate_tree);
    if (!valid) {
        return 0;
    }

    ascending_tree = lks_tree_create();
    if (ascending_tree == NULL) {
        return 0;
    }
    valid = 1;
    for (index = 0; index < 5; ++index) {
        values_c[index] = input_c[index];
        if (!insert_item_and_check_path(ascending_tree, &values_c[index],
                &comparator, paths_c[index], 1, "C")) {
            valid = 0;
            break;
        }
    }
    if (valid) {
        valid = verify_tree_walk(ascending_tree, &comparator, order_c, 5, 0, "C");
    }
    lks_tree_destroy(ascending_tree);
    if (!valid) {
        return 0;
    }

    descending_tree = lks_tree_create();
    if (descending_tree == NULL) {
        return 0;
    }
    valid = 1;
    for (index = 0; index < 5; ++index) {
        values_d[index] = input_d[index];
        if (!insert_item_and_check_path(descending_tree, &values_d[index],
                &comparator, paths_d[index], 1, "D")) {
            valid = 0;
            break;
        }
    }
    if (valid) {
        valid = verify_tree_walk(descending_tree, &comparator, order_d, 5, 0, "D");
    }
    lks_tree_destroy(descending_tree);
    if (!valid) {
        return 0;
    }

    equal_max_tree = lks_tree_create();
    if (equal_max_tree == NULL) {
        return 0;
    }
    for (index = 0; index < 2; ++index) {
        values_e[index] = input_e[index];
        if (!insert_item_and_check_path(equal_max_tree, &values_e[index],
                &comparator, index == 0 ? "000" : "0A0", 1, "E")) {
            lks_tree_destroy(equal_max_tree);
            return 0;
        }
    }
    if (!verify_tree_walk(equal_max_tree, &comparator, order_e, 2, 1, "E") ||
        lks_path_compare(lks_tree_node_path(lks_tree_root_child_at(equal_max_tree, 0)),
            lks_tree_node_path(lks_tree_root_child_at(equal_max_tree, 1)),
            &path_comparison) != LKS_STATUS_OK || path_comparison == 0) {
        lks_tree_destroy(equal_max_tree);
        return 0;
    }
    lks_tree_destroy(equal_max_tree);

    node = (const LksTreeNode *)1;
    if (lks_tree_insert_item(NULL, &values_e[0], &comparator, &node) !=
            LKS_STATUS_INVALID_ARGUMENT || node != NULL) {
        return 0;
    }
    tree = lks_tree_create();
    if (tree == NULL || lks_tree_insert_item(tree, &values_e[0], NULL,
            NULL) != LKS_STATUS_INVALID_ARGUMENT || lks_tree_size(tree) != 0) {
        lks_tree_destroy(tree);
        return 0;
    }
    no_function = comparator;
    no_function.compare = NULL;
    if (tree == NULL || lks_tree_insert_item(tree, &values_e[0], &no_function,
            NULL) != LKS_STATUS_INVALID_ARGUMENT || lks_tree_size(tree) != 0) {
        lks_tree_destroy(tree);
        return 0;
    }
    lks_tree_destroy(tree);
    printf("F invalid arguments: NULL tree/comparator/function rejected; output pointer NULL allowed\n");

    null_output_tree = lks_tree_create();
    borrowed = 42;
    if (null_output_tree == NULL || lks_tree_insert_item(null_output_tree,
            &borrowed, &comparator, NULL) != LKS_STATUS_OK ||
        lks_tree_size(null_output_tree) != 1) {
        lks_tree_destroy(null_output_tree);
        return 0;
    }
    left = NULL;
    equal = NULL;
    right = NULL;
    if (lks_tree_locate_item(null_output_tree, &borrowed, &comparator,
            &left, &equal, &right) != LKS_STATUS_OK || equal == NULL ||
        lks_tree_node_item(equal) != &borrowed) {
        lks_tree_destroy(null_output_tree);
        return 0;
    }
    lks_tree_destroy(null_output_tree);
    printf("F borrowed item: original int address retained\n");
    return 1;
}
static int group_check_order(
    const LksGroup *group,
    const LksComparator *comparator,
    const int *expected_values,
    size_t count,
    const char *const *expected_paths,
    const int *source_values,
    size_t source_count,
    const char *label,
    int verbose
)
{
    size_t index;

    if (lks_group_size(group) != count) {
        printf("%s size mismatch\n", label);
        return 0;
    }
    for (index = 0; index < count; ++index) {
        void *item;
        const LksPath *path;
        int value;
        size_t source_index;
        int found_source;

        item = lks_group_item_at(group, index);
        path = lks_group_path_at(group, index);
        if (item == NULL || path == NULL) {
            return 0;
        }
        value = *(const int *)item;
        if (expected_values != NULL && value != expected_values[index]) {
            printf("%s value mismatch at %lu\n", label, (unsigned long)index);
            return 0;
        }
        if (expected_paths != NULL) {
            char text[256];

            if (lks_path_format(path, text, sizeof(text)) != LKS_STATUS_OK ||
                strcmp(text, expected_paths[index]) != 0) {
                printf("%s Path mismatch at %lu\n", label, (unsigned long)index);
                return 0;
            }
        }
        found_source = source_values == NULL;
        for (source_index = 0; source_index < source_count; ++source_index) {
            if (item == &source_values[source_index]) {
                found_source = 1;
                break;
            }
        }
        if (!found_source) {
            printf("%s item address was not borrowed from source\n", label);
            return 0;
        }
        if (index > 0) {
            int path_order;
            int item_order;

            if (lks_path_compare(lks_group_path_at(group, index - 1), path,
                    &path_order) != LKS_STATUS_OK || path_order != -1) {
                printf("%s Path order failure at %lu\n", label,
                    (unsigned long)index);
                return 0;
            }
            item_order = comparator->compare(
                lks_group_item_at(group, index - 1), item,
                comparator->context);
            if (item_order > 0) {
                printf("%s business order failure at %lu\n", label,
                    (unsigned long)index);
                return 0;
            }
        }
        if (verbose) {
            char text[256];

            if (lks_path_format(path, text, sizeof(text)) != LKS_STATUS_OK) {
                return 0;
            }
            printf("%s[%lu] %d -> %s\n", label, (unsigned long)index,
                value, text);
        }
    }
    if (!verbose) {
        printf("%s: size %lu, ordered values and adjacent Paths verified\n",
            label, (unsigned long)count);
    }
    return 1;
}

static int group_expect_invalid(
    void *const *items,
    size_t count,
    const LksComparator *comparator,
    int expect_null_out
)
{
    LksGroup *existing;
    LksGroup *out_group;
    LksComparator valid_comparator;
    LksStatus status;

    valid_comparator.compare = compare_int;
    valid_comparator.context = NULL;
    existing = NULL;
    if (lks_group_build(NULL, 0, &valid_comparator, &existing) != LKS_STATUS_OK ||
        existing == NULL) {
        lks_group_destroy(existing);
        return 0;
    }
    out_group = existing;
    status = lks_group_build(items, count, comparator, &out_group);
    lks_group_destroy(existing);
    return status == LKS_STATUS_INVALID_ARGUMENT &&
        (!expect_null_out || out_group == NULL);
}
static int run_group_demo(void)
{
    static const int single_expected[] = { 100 };
    static const char *const single_paths[] = { "000" };
    static const int values_c[] = { 100, 50, 150, 125, 75, 25, 175, 160, 170, 165 };
    static const int sorted_c[] = { 25, 50, 75, 100, 125, 150, 160, 165, 170, 175 };
    static const char *const paths_c[] = {
        "1A1", "1A0", "1A0/A0", "000", "000/A0",
        "0A0", "0A0//A0", "0A0//A0///A0", "0A0//A1", "0A0/A0"
    };
    static const int values_d[] = { 100, 125, 100, 100, 150, 125 };
    static const int sorted_d[] = { 100, 100, 100, 125, 125, 150 };
    static const int values_e[] = { 10, 20, 30, 40, 50 };
    static const int sorted_e[] = { 10, 20, 30, 40, 50 };
    static const char *const paths_e[] = {
        "000", "0A0", "0A0/A0", "0A0/A0//A0", "0A0/A0//A0///A0"
    };
    static const int values_f[] = { 50, 40, 30, 20, 10 };
    static const int sorted_f[] = { 10, 20, 30, 40, 50 };
    static const char *const paths_f[] = { "1A3", "1A2", "1A1", "1A0", "000" };
    static const int sorted_g[] = { 10, 20, 30, 40 };
    LksComparator comparator;
    LksComparator invalid_comparator;
    LksGroup *group;
    LksGroup *empty_group;
    int single_value;
    int mutable_values[4];
    void *single_items[1];
    void *items_c[10];
    void *items_d[6];
    void *items_e[5];
    void *items_f[5];
    void *items_g[4];
    int *large_values;
    int *large_expected;
    void **large_items;
    size_t index;
    int valid;
    LksStatus status;

    comparator.compare = compare_int;
    comparator.context = NULL;

    empty_group = NULL;
    status = lks_group_build(NULL, 0, &comparator, &empty_group);
    if (status != LKS_STATUS_OK || empty_group == NULL ||
        lks_group_size(empty_group) != 0 ||
        lks_group_item_at(empty_group, 0) != NULL ||
        lks_group_path_at(empty_group, 0) != NULL) {
        lks_group_destroy(empty_group);
        return 0;
    }
    lks_group_destroy(empty_group);
    lks_group_destroy(NULL);
    printf("Group A empty: OK, size 0, getters NULL\n");

    single_value = 100;
    single_items[0] = &single_value;
    group = NULL;
    status = lks_group_build(single_items, 1, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, single_expected, 1,
            single_paths, &single_value, 1, "Group B", 1);
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    group = NULL;
    for (index = 0; index < 10; ++index) {
        items_c[index] = (void *)&values_c[index];
    }
    status = lks_group_build(items_c, 10, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, sorted_c, 10, paths_c,
            values_c, 10, "Group C", 1);
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    group = NULL;
    for (index = 0; index < 6; ++index) {
        items_d[index] = (void *)&values_d[index];
    }
    status = lks_group_build(items_d, 6, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, sorted_d, 6, NULL,
            values_d, 6, "Group D duplicates", 0);
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    group = NULL;
    for (index = 0; index < 5; ++index) {
        items_e[index] = (void *)&values_e[index];
    }
    status = lks_group_build(items_e, 5, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, sorted_e, 5, paths_e,
            values_e, 5, "Group E ascending", 0);
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    group = NULL;
    for (index = 0; index < 5; ++index) {
        items_f[index] = (void *)&values_f[index];
    }
    status = lks_group_build(items_f, 5, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, sorted_f, 5, paths_f,
            values_f, 5, "Group F descending", 0);
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    mutable_values[0] = 40;
    mutable_values[1] = 10;
    mutable_values[2] = 30;
    mutable_values[3] = 20;
    items_g[0] = &mutable_values[0];
    items_g[1] = &mutable_values[1];
    items_g[2] = &mutable_values[2];
    items_g[3] = &mutable_values[3];
    group = NULL;
    status = lks_group_build(items_g, 4, &comparator, &group);
    if (status != LKS_STATUS_OK || group == NULL) {
        lks_group_destroy(group);
        return 0;
    }
    for (index = 0; index < 4; ++index) {
        items_g[index] = NULL;
    }
    valid = group_check_order(group, &comparator, sorted_g, 4, NULL,
        mutable_values, 4, "Group G array lifetime and borrowed addresses", 0);
    if (valid) {
        printf("Group H item pointers still refer to original values[]\n");
    }
    lks_group_destroy(group);
    if (!valid) {
        return 0;
    }

    if (lks_group_size(NULL) != 0 || lks_group_item_at(NULL, 0) != NULL ||
        lks_group_path_at(NULL, 0) != NULL) {
        return 0;
    }
    group = NULL;
    status = lks_group_build(items_e, 5, &comparator, &group);
    if (status != LKS_STATUS_OK || group == NULL ||
        lks_group_item_at(group, 5) != NULL || lks_group_path_at(group, 5) != NULL) {
        lks_group_destroy(group);
        return 0;
    }
    lks_group_destroy(group);
    printf("Group I NULL and out-of-range getters: passed\n");

    invalid_comparator = comparator;
    invalid_comparator.compare = NULL;
    if (lks_group_build(NULL, 0, &comparator, NULL) !=
            LKS_STATUS_INVALID_ARGUMENT ||
        !group_expect_invalid(single_items, 1, NULL, 1) ||
        !group_expect_invalid(single_items, 1, &invalid_comparator, 1) ||
        !group_expect_invalid(NULL, 1, &comparator, 1)) {
        return 0;
    }
    printf("Group J invalid parameters: passed; failed outputs reset to NULL\n");

    large_values = (int *)malloc(1000 * sizeof(*large_values));
    large_expected = (int *)malloc(1000 * sizeof(*large_expected));
    large_items = (void **)malloc(1000 * sizeof(*large_items));
    if (large_values == NULL || large_expected == NULL || large_items == NULL) {
        free(large_items);
        free(large_expected);
        free(large_values);
        return 0;
    }
    for (index = 0; index < 1000; ++index) {
        large_values[index] = (int)((index * 37) % 1000);
        large_expected[index] = (int)index;
        large_items[index] = &large_values[index];
    }
    group = NULL;
    status = lks_group_build(large_items, 1000, &comparator, &group);
    valid = status == LKS_STATUS_OK && group != NULL &&
        group_check_order(group, &comparator, large_expected, 1000, NULL,
            NULL, 0, "Group K 1000 elements", 0);
    if (valid) {
        for (index = 1; index < 1000; ++index) {
            if (comparator.compare(lks_group_item_at(group, index - 1),
                    lks_group_item_at(group, index), comparator.context) >= 0) {
                valid = 0;
                break;
            }
        }
    }
    lks_group_destroy(group);
    free(large_items);
    free(large_expected);
    free(large_values);
    return valid;
}
static int batch_group_check(
    const LksGroup *group,
    const LksComparator *comparator,
    const int *expected_values,
    size_t expected_count,
    const int *source_values,
    size_t source_start,
    size_t source_count,
    int require_strict,
    int require_first_zero,
    const char *label
)
{
    size_t index;
    int saw_first_zero;

    if (group == NULL || lks_group_size(group) != expected_count) {
        printf("%s size mismatch\n", label);
        return 0;
    }
    saw_first_zero = !require_first_zero;
    for (index = 0; index < expected_count; ++index) {
        void *item;
        const LksPath *path;
        int value;
        int in_chunk;
        size_t source_index;

        item = lks_group_item_at(group, index);
        path = lks_group_path_at(group, index);
        if (item == NULL || path == NULL) {
            return 0;
        }
        value = *(const int *)item;
        if (expected_values != NULL && value != expected_values[index]) {
            printf("%s value mismatch at %lu\n", label, (unsigned long)index);
            return 0;
        }
        in_chunk = 0;
        for (source_index = source_start;
                source_index < source_start + source_count; ++source_index) {
            if (item == &source_values[source_index]) {
                in_chunk = 1;
                if (require_first_zero && source_index == source_start) {
                    char path_text[32];

                    saw_first_zero = lks_path_format(path, path_text,
                        sizeof(path_text)) == LKS_STATUS_OK &&
                        strcmp(path_text, "000") == 0;
                }
                break;
            }
        }
        if (!in_chunk) {
            printf("%s contains an item from outside its original chunk\n", label);
            return 0;
        }
        if (index > 0) {
            int path_order;
            int item_order;

            if (lks_path_compare(lks_group_path_at(group, index - 1), path,
                    &path_order) != LKS_STATUS_OK || path_order != -1) {
                printf("%s Path order failure\n", label);
                return 0;
            }
            item_order = comparator->compare(lks_group_item_at(group, index - 1),
                item, comparator->context);
            if (item_order > 0 || (require_strict && item_order >= 0)) {
                printf("%s business order failure\n", label);
                return 0;
            }
        }
    }
    if (!saw_first_zero) {
        printf("%s first chunk item does not have local Path 000\n", label);
        return 0;
    }
    return 1;
}

static int batch_expect_invalid(
    void *const *items,
    size_t count,
    size_t group_size,
    const LksComparator *candidate_comparator
)
{
    LksComparator valid_comparator;
    LksGroupBatch *existing;
    LksGroupBatch *out_batch;
    LksStatus status;

    valid_comparator.compare = compare_int;
    valid_comparator.context = NULL;
    existing = NULL;
    if (lks_group_batch_build(NULL, 0, 1, &valid_comparator, &existing) !=
            LKS_STATUS_OK || existing == NULL) {
        lks_group_batch_destroy(existing);
        return 0;
    }
    out_batch = existing;
    status = lks_group_batch_build(items, count, group_size,
        candidate_comparator, &out_batch);
    lks_group_batch_destroy(existing);
    return status == LKS_STATUS_INVALID_ARGUMENT && out_batch == NULL;
}

static int merge_group_expect(
    const LksGroup *group,
    void *const *expected_items,
    const char *const *expected_paths,
    size_t count,
    const LksComparator *comparator
)
{
    size_t index;

    if (group == NULL || lks_group_size(group) != count) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        char path_text[128];

        if (lks_group_item_at(group, index) != expected_items[index] ||
            (expected_paths != NULL &&
             (lks_path_format(lks_group_path_at(group, index), path_text,
                sizeof(path_text)) != LKS_STATUS_OK ||
              strcmp(path_text, expected_paths[index]) != 0))) {
            return 0;
        }
        if (index > 0) {
            int path_order;

            if (lks_path_compare(lks_group_path_at(group, index - 1),
                    lks_group_path_at(group, index), &path_order) !=
                    LKS_STATUS_OK || path_order != -1 ||
                comparator->compare(lks_group_item_at(group, index - 1),
                    lks_group_item_at(group, index), comparator->context) > 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int run_group_merge_demo(void)
{
    int a_base_values[] = { 200, 100, 300 };
    int a_in_values[] = { 50, 150, 250, 350 };
    void *a_base_items[] = { &a_base_values[0], &a_base_values[1], &a_base_values[2] };
    void *a_in_items[] = { &a_in_values[0], &a_in_values[1], &a_in_values[2], &a_in_values[3] };
    void *a_result_items[] = { &a_in_values[0], &a_base_values[1], &a_in_values[1],
        &a_base_values[0], &a_in_values[2], &a_base_values[2], &a_in_values[3] };
    const char *a_result_paths[] = { "1A1", "1A0", "1A0/A0", "000",
        "000/A0", "0A0", "0A0/A0" };
    int b_base_values[] = { 100, 200 };
    int b_in_values[] = { 110, 120, 130 };
    void *b_base_items[] = { &b_base_values[0], &b_base_values[1] };
    void *b_in_items[] = { &b_in_values[0], &b_in_values[1], &b_in_values[2] };
    void *b_result_items[] = { &b_base_values[0], &b_in_values[0], &b_in_values[1],
        &b_in_values[2], &b_base_values[1] };
    const char *b_result_paths[] = { "000", "000/A0", "000/A1", "000/A2", "0A0" };
    int c_base_values[] = { 100, 200, 200, 300 };
    int c_in_values[] = { 200, 200, 250 };
    void *c_base_items[] = { &c_base_values[0], &c_base_values[1], &c_base_values[2], &c_base_values[3] };
    void *c_in_items[] = { &c_in_values[0], &c_in_values[1], &c_in_values[2] };
    void *c_result_items[] = { &c_base_values[0], &c_base_values[1], &c_base_values[2],
        &c_in_values[0], &c_in_values[1], &c_in_values[2], &c_base_values[3] };
    int d_base_values[] = { 100, 50, 150, 125, 75, 25, 175, 160, 170, 165 };
    void *d_base_items[10];
    int e_in_values[] = { 10, 20, 30 };
    void *e_in_items[] = { &e_in_values[0], &e_in_values[1], &e_in_values[2] };
    void *e_result_items[] = { &e_in_values[0], &e_in_values[1], &e_in_values[2] };
    const char *e_result_paths[] = { "000", "0A0", "0A0/A0" };
    LksComparator comparator;
    LksComparator counted_comparator;
    LksGroup *a_base = NULL;
    LksGroup *a_incoming = NULL;
    LksGroup *a_result = NULL;
    LksGroup *base = NULL;
    LksGroup *incoming = NULL;
    LksGroup *result = NULL;
    LksGroup *empty = NULL;
    void *base_snapshot[10];
    char base_path_snapshot[10][128];
    void *incoming_snapshot[10];
    char incoming_path_snapshot[10][128];
    LksStatus status;
    size_t index;
    int valid = 0;

    comparator.compare = compare_int;
    comparator.context = NULL;
    if (lks_group_build(a_base_items, 3, &comparator, &a_base) != LKS_STATUS_OK ||
        lks_group_build(a_in_items, 4, &comparator, &a_incoming) != LKS_STATUS_OK) {
        goto cleanup;
    }
    if (lks_path_format(lks_group_path_at(a_incoming, 0),
            base_path_snapshot[0], sizeof(base_path_snapshot[0])) != LKS_STATUS_OK ||
        strcmp(base_path_snapshot[0], "000") != 0) {
        goto cleanup;
    }
    for (index = 0; index < lks_group_size(a_base); ++index) {
        base_snapshot[index] = lks_group_item_at(a_base, index);
        if (lks_path_format(lks_group_path_at(a_base, index),
                base_path_snapshot[index], sizeof(base_path_snapshot[index])) !=
                LKS_STATUS_OK) {
            goto cleanup;
        }
    }
    for (index = 0; index < lks_group_size(a_incoming); ++index) {
        incoming_snapshot[index] = lks_group_item_at(a_incoming, index);
        if (lks_path_format(lks_group_path_at(a_incoming, index),
                incoming_path_snapshot[index], sizeof(incoming_path_snapshot[index])) !=
                LKS_STATUS_OK) {
            goto cleanup;
        }
    }
    status = lks_group_merge(a_base, a_incoming, &comparator, &a_result);
    if (status != LKS_STATUS_OK || a_result == NULL ||
        !merge_group_expect(a_result, a_result_items, a_result_paths, 7, &comparator)) {
        goto cleanup;
    }
    for (index = 0; index < lks_group_size(a_base); ++index) {
        int path_order;

        if (lks_group_item_at(a_base, index) != base_snapshot[index] ||
            lks_path_compare(lks_group_path_at(a_base, index),
                lks_group_path_at(a_result, index == 0 ? 1 : index == 1 ? 3 : 5),
                &path_order) != LKS_STATUS_OK || path_order != 0) {
            goto cleanup;
        }
        if (lks_path_format(lks_group_path_at(a_base, index),
                incoming_path_snapshot[9], sizeof(incoming_path_snapshot[9])) != LKS_STATUS_OK ||
            strcmp(incoming_path_snapshot[9], base_path_snapshot[index]) != 0) {
            goto cleanup;
        }
    }
    for (index = 0; index < lks_group_size(a_incoming); ++index) {
        char path_text[128];

        if (lks_group_item_at(a_incoming, index) != incoming_snapshot[index] ||
            lks_path_format(lks_group_path_at(a_incoming, index), path_text,
                sizeof(path_text)) != LKS_STATUS_OK ||
            strcmp(path_text, incoming_path_snapshot[index]) != 0) {
            goto cleanup;
        }
    }
    printf("Merge A: 50/1A1, 100/1A0, 150/1A0/A0, 200/000, 250/000/A0, 300/0A0, 350/0A0/A0\n");
    printf("Merge A: Base Paths retained; Incoming 50 re-encoded from 000 to 1A1; inputs unchanged\n");
    lks_group_destroy(a_base);
    a_base = NULL;
    lks_group_destroy(a_incoming);
    a_incoming = NULL;
    if (!merge_group_expect(a_result, a_result_items, a_result_paths, 7, &comparator)) {
        goto cleanup;
    }
    printf("Merge H: Result remains readable after both input Groups are destroyed\n");

    if (lks_group_build(b_base_items, 2, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(b_in_items, 3, &comparator, &incoming) != LKS_STATUS_OK ||
        lks_group_merge(base, incoming, &comparator, &result) != LKS_STATUS_OK ||
        !merge_group_expect(result, b_result_items, b_result_paths, 5, &comparator)) {
        goto cleanup;
    }
    printf("Merge B: 100/000, 110/000/A0, 120/000/A1, 130/000/A2, 200/0A0\n");
    lks_group_destroy(result); result = NULL;
    lks_group_destroy(incoming); incoming = NULL;
    lks_group_destroy(base); base = NULL;

    if (lks_group_build(c_base_items, 4, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(c_in_items, 3, &comparator, &incoming) != LKS_STATUS_OK ||
        lks_group_merge(base, incoming, &comparator, &result) != LKS_STATUS_OK ||
        !merge_group_expect(result, c_result_items, NULL, 7, &comparator)) {
        goto cleanup;
    }
    printf("Merge C: equal values retain Base A/B before Incoming A/B; order stable within each input\n");
    lks_group_destroy(result); result = NULL;
    lks_group_destroy(incoming); incoming = NULL;
    lks_group_destroy(base); base = NULL;

    for (index = 0; index < 10; ++index) {
        d_base_items[index] = &d_base_values[index];
    }
    if (lks_group_build(d_base_items, 10, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(NULL, 0, &comparator, &empty) != LKS_STATUS_OK ||
        lks_group_merge(base, empty, &comparator, &result) != LKS_STATUS_OK ||
        result == base || lks_group_size(result) != 10) {
        goto cleanup;
    }
    for (index = 0; index < 10; ++index) {
        int path_order;
        size_t result_index;

        for (result_index = 0; result_index < 10; ++result_index) {
            if (lks_group_item_at(result, result_index) ==
                lks_group_item_at(base, index)) {
                break;
            }
        }
        if (result_index == 10 || lks_path_compare(lks_group_path_at(base, index),
                lks_group_path_at(result, result_index), &path_order) !=
                LKS_STATUS_OK || path_order != 0) {
            goto cleanup;
        }
    }
    printf("Merge D: empty Incoming creates an independent copy; all 10 Base Paths preserved\n");
    lks_group_destroy(result); result = NULL;
    lks_group_destroy(empty); empty = NULL;
    lks_group_destroy(base); base = NULL;

    if (lks_group_build(NULL, 0, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(e_in_items, 3, &comparator, &incoming) != LKS_STATUS_OK ||
        lks_group_merge(base, incoming, &comparator, &result) != LKS_STATUS_OK ||
        !merge_group_expect(result, e_result_items, e_result_paths, 3, &comparator)) {
        goto cleanup;
    }
    printf("Merge E: empty Base re-encodes Incoming as 000, 0A0, 0A0/A0\n");
    lks_group_destroy(result); result = NULL;
    lks_group_destroy(incoming); incoming = NULL;
    lks_group_destroy(base); base = NULL;

    if (lks_group_build(NULL, 0, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(NULL, 0, &comparator, &incoming) != LKS_STATUS_OK ||
        lks_group_merge(base, incoming, &comparator, &result) != LKS_STATUS_OK ||
        result == NULL || lks_group_size(result) != 0) {
        goto cleanup;
    }
    printf("Merge F: empty + empty returns a valid empty Group\n");
    lks_group_destroy(result); result = NULL;
    lks_group_destroy(incoming); incoming = NULL;
    lks_group_destroy(base); base = NULL;

    {
        int complex_base_values[] = { 100, 50, 150, 125, 75, 25, 175, 160, 170, 165 };
        int complex_incoming_values[] = { 30, 80, 140, 168, 180 };
        void *complex_base_items[10];
        void *complex_incoming_items[5];

        for (index = 0; index < 10; ++index) {
            complex_base_items[index] = &complex_base_values[index];
        }
        for (index = 0; index < 5; ++index) {
            complex_incoming_items[index] = &complex_incoming_values[index];
        }
        status = lks_group_build(complex_base_items, 10, &comparator, &base);
        if (status != LKS_STATUS_OK) {
            printf("Merge I Base build failed: %s\n", lks_status_string(status));
            goto cleanup;
        }
        status = lks_group_build(complex_incoming_items, 5, &comparator, &incoming);
        if (status != LKS_STATUS_OK) {
            printf("Merge I Incoming build failed: %s\n", lks_status_string(status));
            goto cleanup;
        }
        status = lks_group_merge(base, incoming, &comparator, &result);
        if (status != LKS_STATUS_OK) {
            printf("Merge I merge failed: %s\n", lks_status_string(status));
            goto cleanup;
        }
        for (index = 0; index < 10; ++index) {
            size_t result_index;
            int path_order;

            for (result_index = 0; result_index < lks_group_size(result); ++result_index) {
                if (lks_group_item_at(result, result_index) ==
                    lks_group_item_at(base, index)) {
                    break;
                }
            }
            if (result_index == lks_group_size(result) ||
                lks_path_compare(lks_group_path_at(base, index),
                    lks_group_path_at(result, result_index), &path_order) !=
                LKS_STATUS_OK || path_order != 0) {
                char old_text[128] = "<none>";
                char new_text[128] = "<none>";
                if (index < lks_group_size(base)) {
                    lks_path_format(lks_group_path_at(base, index), old_text,
                        sizeof(old_text));
                }
                if (result_index < lks_group_size(result)) {
                    lks_path_format(lks_group_path_at(result, result_index), new_text,
                        sizeof(new_text));
                }
                printf("Merge I Base Path mismatch at base index %lu, result index %lu, order %d\n",
                    (unsigned long)index, (unsigned long)result_index, path_order);
                printf("  old=%s new=%s\n", old_text, new_text);
                goto cleanup;
            }
        }
        printf("Merge I: all 10 complex Base Paths preserved\n");
        lks_group_destroy(result); result = NULL;
        lks_group_destroy(incoming); incoming = NULL;
        lks_group_destroy(base); base = NULL;
    }

    if (lks_group_build(NULL, 0, &comparator, &empty) != LKS_STATUS_OK) {
        goto cleanup;
    }
    {
        LksComparator invalid_comparator = comparator;
        LksGroup *invalid_result = NULL;

        invalid_comparator.compare = NULL;
        invalid_result = empty;
        if (lks_group_merge(NULL, empty, &comparator, &invalid_result) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_result != NULL) {
            goto cleanup;
        }
        invalid_result = empty;
        if (lks_group_merge(empty, NULL, &comparator, &invalid_result) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_result != NULL) {
            goto cleanup;
        }
        invalid_result = empty;
        if (lks_group_merge(empty, empty, NULL, &invalid_result) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_result != NULL) {
            goto cleanup;
        }
        invalid_result = empty;
        if (lks_group_merge(empty, empty, &invalid_comparator, &invalid_result) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_result != NULL ||
            lks_group_merge(empty, empty, &comparator, NULL) != LKS_STATUS_INVALID_ARGUMENT) {
            goto cleanup;
        }
    }
    printf("Merge invalid arguments: all five rejected; writable output remains NULL\n");

    valid = 1;

cleanup:
    lks_group_destroy(result);
    lks_group_destroy(incoming);
    lks_group_destroy(base);
    lks_group_destroy(empty);
    lks_group_destroy(a_result);
    lks_group_destroy(a_incoming);
    lks_group_destroy(a_base);
    if (!valid) {
        return 0;
    }

    {
        int base_values[500];
        int incoming_values[500];
        void *base_items[500];
        void *incoming_items[500];
        CompareCounter merge_counter;
        LksGroup *large_base = NULL;
        LksGroup *large_incoming = NULL;
        LksGroup *large_result = NULL;
        int large_valid = 1;

        merge_counter.calls = 0;
        for (index = 0; index < 500; ++index) {
            base_values[index] = (int)(index * 2);
            incoming_values[index] = (int)(index * 2 + 1);
            base_items[index] = &base_values[index];
            incoming_items[index] = &incoming_values[index];
        }
        if (lks_group_build(base_items, 500, &comparator, &large_base) != LKS_STATUS_OK ||
            lks_group_build(incoming_items, 500, &comparator, &large_incoming) != LKS_STATUS_OK) {
            large_valid = 0;
        }
        counted_comparator.compare = compare_int_counted;
        counted_comparator.context = &merge_counter;
        if (large_valid && lks_group_merge(large_base, large_incoming,
                &counted_comparator, &large_result) != LKS_STATUS_OK) {
            large_valid = 0;
        }
        if (large_valid && lks_group_size(large_result) != 1000) {
            large_valid = 0;
        }
        for (index = 0; large_valid && index < 1000; ++index) {
            int expected = (int)index;
            if (lks_group_item_at(large_result, index) == NULL ||
                *(int *)lks_group_item_at(large_result, index) != expected) {
                large_valid = 0;
            }
        }
        for (index = 0; large_valid && index < 500; ++index) {
            size_t result_index = index * 2;
            int path_order;
            if (lks_group_item_at(large_result, result_index) != base_items[index] ||
                lks_path_compare(lks_group_path_at(large_base, index),
                    lks_group_path_at(large_result, result_index), &path_order) !=
                    LKS_STATUS_OK || path_order != 0) {
                large_valid = 0;
            }
        }
        if (merge_counter.calls >= 4000) {
            large_valid = 0;
        }
        printf("Merge J: 500 + 500 -> 1000 ordered items; merge comparator calls %lu; 500 Base Paths preserved\n",
            (unsigned long)merge_counter.calls);
        lks_group_destroy(large_result);
        lks_group_destroy(large_incoming);
        lks_group_destroy(large_base);
        return large_valid;
    }
}

static int run_group_batch_demo(void)
{
    static const int values_b[] = { 40, 10, 30, 20 };
    static const int sorted_b[] = { 10, 20, 30, 40 };
    static const int values_c[] = { 90, 10, 70, 30, 80, 20, 60, 40, 100, 50 };
    static const int sorted_c0[] = { 10, 30, 70, 90 };
    static const int sorted_c1[] = { 20, 40, 60, 80 };
    static const int sorted_c2[] = { 50, 100 };
    static const int values_e[] = { 4, 1, 3, 2 };
    static const int values_f[] = { 30, 10, 20 };
    static const int sorted_f[] = { 10, 20, 30 };
    LksComparator comparator;
    LksComparator invalid_comparator;
    LksGroupBatch *batch;
    LksGroupBatch *empty_batch;
    LksGroupBatch *one_batch;
    LksGroupBatch *big_batch;
    LksGroupBatch *remainder_batch;
    void *items_b[4];
    void *items_c[10];
    void *items_e[4];
    void *items_f[3];
    void **items_1000;
    void **items_103;
    int *values_1000;
    int values_103[103];
    size_t index;
    size_t group_index;
    size_t size_sum;
    int valid;
    LksStatus status;

    comparator.compare = compare_int;
    comparator.context = NULL;

    empty_batch = NULL;
    status = lks_group_batch_build(NULL, 0, 100, &comparator, &empty_batch);
    if (status != LKS_STATUS_OK || empty_batch == NULL ||
        lks_group_batch_total_size(empty_batch) != 0 ||
        lks_group_batch_group_count(empty_batch) != 0 ||
        lks_group_batch_group_size(empty_batch) != 100 ||
        lks_group_batch_group_at(empty_batch, 0) != NULL) {
        lks_group_batch_destroy(empty_batch);
        return 0;
    }
    lks_group_batch_destroy(empty_batch);
    lks_group_batch_destroy(NULL);
    printf("Batch A empty: total 0, groups 0, configured size 100\n");

    for (index = 0; index < 4; ++index) {
        items_b[index] = (void *)&values_b[index];
    }
    one_batch = NULL;
    status = lks_group_batch_build(items_b, 4, 4, &comparator, &one_batch);
    valid = status == LKS_STATUS_OK && one_batch != NULL &&
        lks_group_batch_group_count(one_batch) == 1 &&
        lks_group_batch_total_size(one_batch) == 4 &&
        lks_group_batch_group_size(one_batch) == 4 &&
        batch_group_check(lks_group_batch_group_at(one_batch, 0), &comparator,
            sorted_b, 4, values_b, 0, 4, 1, 0, "Batch B group 0");
    lks_group_batch_destroy(one_batch);
    if (!valid) {
        return 0;
    }
    printf("Batch B: one group, values 10 20 30 40\n");

    for (index = 0; index < 10; ++index) {
        items_c[index] = (void *)&values_c[index];
    }
    batch = NULL;
    status = lks_group_batch_build(items_c, 10, 4, &comparator, &batch);
    if (status != LKS_STATUS_OK || batch == NULL ||
        lks_group_batch_group_count(batch) != 3 ||
        lks_group_batch_total_size(batch) != 10 ||
        lks_group_batch_group_size(batch) != 4 ||
        !batch_group_check(lks_group_batch_group_at(batch, 0), &comparator,
            sorted_c0, 4, values_c, 0, 4, 1, 1, "Batch C group 0") ||
        !batch_group_check(lks_group_batch_group_at(batch, 1), &comparator,
            sorted_c1, 4, values_c, 4, 4, 1, 1, "Batch C group 1") ||
        !batch_group_check(lks_group_batch_group_at(batch, 2), &comparator,
            sorted_c2, 2, values_c, 8, 2, 1, 1, "Batch C group 2")) {
        lks_group_batch_destroy(batch);
        return 0;
    }
    printf("Batch C chunks: [90 10 70 30] -> 10 30 70 90; ");
    printf("[80 20 60 40] -> 20 40 60 80; [100 50] -> 50 100\n");
    printf("Batch D local 000: 90, 80, and 100 each start their own Group at 000\n");

    for (index = 0; index < 10; ++index) {
        items_c[index] = NULL;
    }
    valid = batch_group_check(lks_group_batch_group_at(batch, 0), &comparator,
                sorted_c0, 4, values_c, 0, 4, 1, 1, "Batch G group 0") &&
        batch_group_check(lks_group_batch_group_at(batch, 1), &comparator,
                sorted_c1, 4, values_c, 4, 4, 1, 1, "Batch G group 1") &&
        batch_group_check(lks_group_batch_group_at(batch, 2), &comparator,
                sorted_c2, 2, values_c, 8, 2, 1, 1, "Batch G group 2");
    if (valid) {
        printf("Batch G/H: items[] cleared; values and original item addresses remain available\n");
    }
    lks_group_batch_destroy(batch);
    if (!valid) {
        return 0;
    }

    for (index = 0; index < 4; ++index) {
        items_e[index] = (void *)&values_e[index];
    }
    batch = NULL;
    status = lks_group_batch_build(items_e, 4, 1, &comparator, &batch);
    if (status != LKS_STATUS_OK || batch == NULL ||
        lks_group_batch_group_count(batch) != 4 ||
        lks_group_batch_total_size(batch) != 4) {
        lks_group_batch_destroy(batch);
        return 0;
    }
    for (index = 0; index < 4; ++index) {
        const LksGroup *group;
        char path_text[32];

        group = lks_group_batch_group_at(batch, index);
        if (group == NULL || lks_group_size(group) != 1 ||
            lks_group_item_at(group, 0) != &values_e[index] ||
            lks_path_format(lks_group_path_at(group, 0), path_text,
                sizeof(path_text)) != LKS_STATUS_OK ||
            strcmp(path_text, "000") != 0) {
            lks_group_batch_destroy(batch);
            return 0;
        }
    }
    printf("Batch E GroupSize 1: 4 singleton Groups, each Path 000\n");
    lks_group_batch_destroy(batch);

    for (index = 0; index < 3; ++index) {
        items_f[index] = (void *)&values_f[index];
    }
    batch = NULL;
    status = lks_group_batch_build(items_f, 3, 100, &comparator, &batch);
    valid = status == LKS_STATUS_OK && batch != NULL &&
        lks_group_batch_group_count(batch) == 1 &&
        lks_group_batch_total_size(batch) == 3 &&
        lks_group_batch_group_size(batch) == 100 &&
        batch_group_check(lks_group_batch_group_at(batch, 0), &comparator,
            sorted_f, 3, values_f, 0, 3, 1, 1, "Batch F larger GroupSize");
    lks_group_batch_destroy(batch);
    if (!valid) {
        return 0;
    }

    if (lks_group_batch_total_size(NULL) != 0 ||
        lks_group_batch_group_count(NULL) != 0 ||
        lks_group_batch_group_size(NULL) != 0 ||
        lks_group_batch_group_at(NULL, 0) != NULL) {
        return 0;
    }

    values_1000 = (int *)malloc(1000 * sizeof(*values_1000));
    items_1000 = (void **)malloc(1000 * sizeof(*items_1000));
    if (values_1000 == NULL || items_1000 == NULL) {
        free(items_1000);
        free(values_1000);
        return 0;
    }
    for (index = 0; index < 1000; ++index) {
        values_1000[index] = (int)((index * 37) % 1000);
        items_1000[index] = &values_1000[index];
    }
    big_batch = NULL;
    status = lks_group_batch_build(items_1000, 1000, 100, &comparator,
        &big_batch);
    valid = status == LKS_STATUS_OK && big_batch != NULL &&
        lks_group_batch_group_count(big_batch) == 10 &&
        lks_group_batch_total_size(big_batch) == 1000 &&
        lks_group_batch_group_size(big_batch) == 100;
    for (group_index = 0; valid && group_index < 10; ++group_index) {
        const LksGroup *group;

        group = lks_group_batch_group_at(big_batch, group_index);
        valid = lks_group_size(group) == 100 &&
            batch_group_check(group, &comparator, NULL, 100, values_1000,
                group_index * 100, 100, 1, 1, "Batch I chunk");
    }
    if (valid && lks_group_batch_group_at(big_batch, 10) != NULL) {
        valid = 0;
    }
    lks_group_batch_destroy(big_batch);
    free(items_1000);
    free(values_1000);
    if (!valid) {
        return 0;
    }
    printf("Batch I: 10 groups of 100; chunk membership and 10 local 000 Paths verified\n");

    items_103 = (void **)malloc(103 * sizeof(*items_103));
    if (items_103 == NULL) {
        return 0;
    }
    for (index = 0; index < 103; ++index) {
        values_103[index] = (int)((index * 29) % 103);
        items_103[index] = &values_103[index];
    }
    remainder_batch = NULL;
    status = lks_group_batch_build(items_103, 103, 20, &comparator,
        &remainder_batch);
    valid = status == LKS_STATUS_OK && remainder_batch != NULL &&
        lks_group_batch_group_count(remainder_batch) == 6 &&
        lks_group_batch_total_size(remainder_batch) == 103 &&
        lks_group_batch_group_size(remainder_batch) == 20;
    size_sum = 0;
    for (group_index = 0; valid && group_index < 6; ++group_index) {
        size_t expected_size;

        expected_size = group_index < 5 ? 20 : 3;
        if (lks_group_size(lks_group_batch_group_at(remainder_batch,
                group_index)) != expected_size) {
            valid = 0;
        } else {
            size_sum += expected_size;
        }
    }
    lks_group_batch_destroy(remainder_batch);
    free(items_103);
    if (!valid || size_sum != 103) {
        return 0;
    }
    printf("Batch J count 103 / size 20: groups 20,20,20,20,20,3; total 103\n");

    invalid_comparator = comparator;
    invalid_comparator.compare = NULL;
    if (lks_group_batch_build(NULL, 0, 10, &comparator, NULL) !=
            LKS_STATUS_INVALID_ARGUMENT ||
        !batch_expect_invalid(NULL, 0, 0, &comparator) ||
        !batch_expect_invalid(NULL, 0, 10, NULL) ||
        !batch_expect_invalid(NULL, 0, 10, &invalid_comparator) ||
        !batch_expect_invalid(NULL, 1, 10, &comparator)) {
        return 0;
    }
    printf("Batch K invalid parameters and NULL/out-of-range getters: passed\n");

    batch = NULL;
    status = lks_group_batch_build(NULL, 0, 1, &comparator, &batch);
    if (status != LKS_STATUS_OK || batch == NULL ||
        lks_group_batch_group_at(batch,
            lks_group_batch_group_count(batch)) != NULL) {
        lks_group_batch_destroy(batch);
        return 0;
    }
    lks_group_batch_destroy(batch);
    return 1;
}

static int run_group_batch_merge_demo(void)
{
    int odd_values[] = { 9, 1, 8, 2, 7, 3, 6, 4, 5, 0 };
    void *odd_items[10];
    void *odd_expected[] = { &odd_values[9], &odd_values[1], &odd_values[3],
        &odd_values[5], &odd_values[7], &odd_values[8], &odd_values[6],
        &odd_values[4], &odd_values[2], &odd_values[0] };
    int equal_values[8] = { 7, 7, 7, 7, 7, 7, 7, 7 };
    void *equal_items[8];
    int eight_values[16];
    void *eight_items[16];
    void *eight_expected[16];
    LksComparator comparator;
    LksComparator invalid_comparator;
    LksGroupBatch *batch = NULL;
    LksGroup *result = NULL;
    LksStatus status;
    size_t index;
    int valid = 0;

    comparator.compare = compare_int;
    comparator.context = NULL;

    status = lks_group_batch_build(NULL, 0, 3, &comparator, &batch);
    if (status != LKS_STATUS_OK || batch == NULL ||
        lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
        result == NULL || lks_group_size(result) != 0) {
        goto cleanup;
    }
    printf("Merge all empty Batch: independent empty Group returned\n");
    lks_group_destroy(result); result = NULL;
    lks_group_batch_destroy(batch); batch = NULL;

    {
        int values4[] = { 10, 20, 30, 40 };
        void *items4[] = { &values4[0], &values4[1], &values4[2], &values4[3] };
        void *expected4[] = { &values4[0], &values4[1], &values4[2], &values4[3] };
        const char *paths4[] = { "000", "0A0", "0A0/A0", "0A0/A0//A0" };
        if (lks_group_batch_build(items4, 4, 1, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_group_count(batch) != 4 ||
            lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            !merge_group_expect(result, expected4, paths4, 4, &comparator)) {
            goto cleanup;
        }
        for (index = 0; index < 4; ++index) {
            printf("Merge all 4 singleton: %d -> %s\n", values4[index], paths4[index]);
        }
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
    }

    {
        int values5[] = { 10, 20, 30, 40, 50 };
        void *items5[] = { &values5[0], &values5[1], &values5[2], &values5[3], &values5[4] };
        void *expected5[] = { &values5[0], &values5[1], &values5[2], &values5[3], &values5[4] };
        const char *paths5[] = { "000", "0A0", "0A0/A0", "0A0/A0//A0",
            "0A0/A0//A0///A0" };
        char carried_path[32];
        const LksGroup *last_original;

        if (lks_group_batch_build(items5, 5, 1, &comparator, &batch) != LKS_STATUS_OK) {
            goto cleanup;
        }
        last_original = lks_group_batch_group_at(batch, 4);
        if (lks_path_format(lks_group_path_at(last_original, 0), carried_path,
                sizeof(carried_path)) != LKS_STATUS_OK || strcmp(carried_path, "000") != 0 ||
            lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            !merge_group_expect(result, expected5, paths5, 5, &comparator) ||
            lks_path_format(lks_group_path_at(last_original, 0), carried_path,
                sizeof(carried_path)) != LKS_STATUS_OK || strcmp(carried_path, "000") != 0) {
            goto cleanup;
        }
        for (index = 0; index < 5; ++index) {
            printf("Merge all 5 singleton: %d -> %s\n", values5[index], paths5[index]);
        }
        printf("Merge all 5 singleton: G4 stayed at 000 in original Batch during carry\n");
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
    }

    {
        int values6[] = { 6, 1, 5, 2, 4, 3 };
        void *items6[] = { &values6[0], &values6[1], &values6[2],
            &values6[3], &values6[4], &values6[5] };
        int expected6[] = { 1, 2, 3, 4, 5, 6 };

        if (lks_group_batch_build(items6, 6, 1, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_group_count(batch) != 6 ||
            lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            lks_group_size(result) != 6) {
            goto cleanup;
        }
        for (index = 0; index < 6; ++index) {
            if (*(int *)lks_group_item_at(result, index) != expected6[index]) {
                goto cleanup;
            }
        }
        printf("Merge all 6 singleton: 1 2 3 4 5 6; size 6, no lost or duplicate items\n");
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
    }

    {
        int values_equal5[] = { 100, 100, 100, 100, 100 };
        void *items_equal5[] = { &values_equal5[0], &values_equal5[1],
            &values_equal5[2], &values_equal5[3], &values_equal5[4] };

        if (lks_group_batch_build(items_equal5, 5, 1, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            lks_group_size(result) != 5) {
            goto cleanup;
        }
        for (index = 0; index < 5; ++index) {
            if (lks_group_item_at(result, index) != &values_equal5[index]) {
                goto cleanup;
            }
        }
        printf("Merge all 5 equal singleton items: origins 0,1,2,3,4 retained by pointer\n");
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
    }

    {
        int values10[] = { 90, 10, 70, 30, 80, 20, 60, 40, 100, 50 };
        void *items10[10];
        int expected10[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
        const LksGroup *g0;
        void *g0_items[4];
        const LksPath *g0_paths[4];
        void *batch_items_snapshot[3][4];
        char batch_path_snapshot[3][4][128];
        size_t group_index;

        for (index = 0; index < 10; ++index) {
            items10[index] = &values10[index];
        }
        if (lks_group_batch_build(items10, 10, 4, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_group_count(batch) != 3) {
            goto cleanup;
        }
        g0 = lks_group_batch_group_at(batch, 0);
        for (group_index = 0; group_index < 3; ++group_index) {
            const LksGroup *group = lks_group_batch_group_at(batch, group_index);
            size_t group_size = lks_group_size(group);
            for (index = 0; index < group_size; ++index) {
                batch_items_snapshot[group_index][index] = lks_group_item_at(group, index);
                if (lks_path_format(lks_group_path_at(group, index),
                        batch_path_snapshot[group_index][index],
                        sizeof(batch_path_snapshot[group_index][index])) != LKS_STATUS_OK) {
                    goto cleanup;
                }
            }
        }
        for (index = 0; index < 4; ++index) {
            g0_items[index] = lks_group_item_at(g0, index);
            g0_paths[index] = lks_group_path_at(g0, index);
        }
        if (lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            lks_group_size(result) != 10) {
            goto cleanup;
        }
        for (index = 0; index < 10; ++index) {
            if (*(int *)lks_group_item_at(result, index) != expected10[index]) {
                goto cleanup;
            }
        }
        printf("Merge all GroupSize 4: 10 20 30 40 50 60 70 80 90 100\n");
        for (index = 0; index < 4; ++index) {
            size_t result_index;
            int path_order;
            char path_text[128];

            for (result_index = 0; result_index < 10; ++result_index) {
                if (lks_group_item_at(result, result_index) == g0_items[index]) {
                    break;
                }
            }
            if (result_index == 10 || lks_path_compare(g0_paths[index],
                    lks_group_path_at(result, result_index), &path_order) !=
                    LKS_STATUS_OK || path_order != 0 ||
                lks_path_format(g0_paths[index], path_text, sizeof(path_text)) != LKS_STATUS_OK) {
                goto cleanup;
            }
            printf("G0 item %d Path retained: %s\n", *(int *)g0_items[index], path_text);
        }
        for (group_index = 0; group_index < 3; ++group_index) {
            const LksGroup *group = lks_group_batch_group_at(batch, group_index);
            for (index = 0; index < lks_group_size(group); ++index) {
                char path_text[128];
                if (lks_group_item_at(group, index) != batch_items_snapshot[group_index][index] ||
                    lks_path_format(lks_group_path_at(group, index), path_text,
                        sizeof(path_text)) != LKS_STATUS_OK ||
                    strcmp(path_text, batch_path_snapshot[group_index][index]) != 0) {
                    goto cleanup;
                }
            }
        }
        printf("Merge all: input Batch items and Paths unchanged\n");
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
    }

    {
        int *values1024 = (int *)malloc(1024 * sizeof(*values1024));
        void **items1024 = (void **)malloc(1024 * sizeof(*items1024));
        CompareCounter merge_counter;
        LksComparator merge_comparator;
        const LksGroup *original_g0;
        const LksPath *original_item0_path;
        char item0_path_text[32];
        int big_valid = 1;

        if (values1024 == NULL || items1024 == NULL) {
            free(items1024);
            free(values1024);
            goto cleanup;
        }
        for (index = 0; index < 1024; ++index) {
            values1024[index] = (int)index;
            items1024[index] = &values1024[index];
        }
        if (lks_group_batch_build(items1024, 1024, 1, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_group_count(batch) != 1024) {
            free(items1024);
            free(values1024);
            goto cleanup;
        }
        original_g0 = lks_group_batch_group_at(batch, 0);
        original_item0_path = lks_group_path_at(original_g0, 0);
        if (lks_path_format(original_item0_path, item0_path_text,
                sizeof(item0_path_text)) != LKS_STATUS_OK ||
            strcmp(item0_path_text, "000") != 0) {
            free(items1024);
            free(values1024);
            goto cleanup;
        }
        merge_counter.calls = 0;
        merge_comparator.compare = compare_int_counted;
        merge_comparator.context = &merge_counter;
        if (lks_group_batch_merge_all(batch, &merge_comparator, &result) != LKS_STATUS_OK ||
            result == NULL || lks_group_size(result) != 1024) {
            big_valid = 0;
        }
        for (index = 0; big_valid && index < 1024; ++index) {
            int path_order;
            if (lks_group_item_at(result, index) != &values1024[index] ||
                (index > 0 && (lks_path_compare(lks_group_path_at(result, index - 1),
                    lks_group_path_at(result, index), &path_order) != LKS_STATUS_OK ||
                    path_order != -1))) {
                big_valid = 0;
            }
        }
        if (big_valid) {
            int path_order;
            if (lks_path_compare(original_item0_path,
                    lks_group_path_at(result, 0), &path_order) != LKS_STATUS_OK ||
                path_order != 0 || lks_path_format(lks_group_path_at(result, 0),
                    item0_path_text, sizeof(item0_path_text)) != LKS_STATUS_OK ||
                strcmp(item0_path_text, "000") != 0) {
                big_valid = 0;
            }
        }
        printf("Merge all 1024 singleton Groups: size 1024, order 0..1023, comparisons %lu, item 0 Path %s\n",
            (unsigned long)merge_counter.calls, item0_path_text);
        lks_group_destroy(result); result = NULL;
        lks_group_batch_destroy(batch); batch = NULL;
        free(items1024);
        free(values1024);
        if (!big_valid || merge_counter.calls >= 30000) {
            goto cleanup;
        }
    }

    for (index = 0; index < 10; ++index) {
        odd_items[index] = &odd_values[index];
    }
    if (lks_group_batch_build(odd_items, 10, 2, &comparator, &batch) != LKS_STATUS_OK ||
        lks_group_batch_group_count(batch) != 5) {
        goto cleanup;
    }
    {
        const LksGroup *first_group = lks_group_batch_group_at(batch, 0);
        void *first_items[2];
        void *batch_items_snapshot[5][2];
        char batch_paths_snapshot[5][2][128];
        size_t group_index;

        for (group_index = 0; group_index < 5; ++group_index) {
            const LksGroup *group = lks_group_batch_group_at(batch, group_index);
            for (index = 0; index < 2; ++index) {
                batch_items_snapshot[group_index][index] = lks_group_item_at(group, index);
                if (lks_path_format(lks_group_path_at(group, index),
                        batch_paths_snapshot[group_index][index],
                        sizeof(batch_paths_snapshot[group_index][index])) != LKS_STATUS_OK) {
                    goto cleanup;
                }
            }
        }
        for (index = 0; index < 2; ++index) {
            first_items[index] = lks_group_item_at(first_group, index);
        }
        status = lks_group_batch_merge_all(batch, &comparator, &result);
        if (status != LKS_STATUS_OK || result == NULL ||
            !merge_group_expect(result, odd_expected, NULL, 10, &comparator)) {
            goto cleanup;
        }
        for (index = 0; index < 2; ++index) {
            size_t result_index;
            int path_order;

            for (result_index = 0; result_index < 10; ++result_index) {
                if (lks_group_item_at(result, result_index) == first_items[index]) {
                    break;
                }
            }
            if (result_index == 10 ||
                lks_path_compare(lks_group_path_at(first_group, index),
                    lks_group_path_at(result, result_index), &path_order) !=
                    LKS_STATUS_OK || path_order != 0) {
                goto cleanup;
            }
        }
        for (group_index = 0; group_index < 5; ++group_index) {
            const LksGroup *group = lks_group_batch_group_at(batch, group_index);
            for (index = 0; index < 2; ++index) {
                char path_text[128];
                if (lks_group_item_at(group, index) !=
                        batch_items_snapshot[group_index][index] ||
                    lks_path_format(lks_group_path_at(group, index), path_text,
                        sizeof(path_text)) != LKS_STATUS_OK ||
                    strcmp(path_text, batch_paths_snapshot[group_index][index]) != 0) {
                    goto cleanup;
                }
            }
        }
    }
    printf("Merge all 5 groups: balanced pairs with odd carry; sorted 0..9; G0 Paths unchanged; Batch readable\n");
    lks_group_destroy(result); result = NULL;
    lks_group_batch_destroy(batch); batch = NULL;

    for (index = 0; index < 16; ++index) {
        eight_values[index] = (int)(15 - index);
        eight_items[index] = &eight_values[index];
        eight_expected[index] = &eight_values[15 - index];
    }
    if (lks_group_batch_build(eight_items, 16, 2, &comparator, &batch) != LKS_STATUS_OK ||
        lks_group_batch_group_count(batch) != 8) {
        goto cleanup;
    }
    {
        const LksGroup *first_group = lks_group_batch_group_at(batch, 0);
        const LksPath *old_paths[2];
        void *first_group_items[2];

        for (index = 0; index < 2; ++index) {
            old_paths[index] = lks_group_path_at(first_group, index);
            first_group_items[index] = lks_group_item_at(first_group, index);
        }
        if (lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            !merge_group_expect(result, eight_expected, NULL, 16, &comparator)) {
            goto cleanup;
        }
        for (index = 0; index < 2; ++index) {
            size_t result_index;
            int path_order;
            for (result_index = 0; result_index < 16; ++result_index) {
                if (lks_group_item_at(result, result_index) == first_group_items[index]) {
                    break;
                }
            }
            if (result_index == 16 || lks_path_compare(old_paths[index],
                    lks_group_path_at(result, result_index), &path_order) !=
                    LKS_STATUS_OK || path_order != 0) {
                goto cleanup;
            }
        }
    }
    printf("Merge all 8 groups: G0 remains on left Base lineage across rounds\n");
    lks_group_destroy(result); result = NULL;
    lks_group_batch_destroy(batch); batch = NULL;

    for (index = 0; index < 8; ++index) {
        equal_items[index] = &equal_values[index];
    }
    if (lks_group_batch_build(equal_items, 8, 2, &comparator, &batch) != LKS_STATUS_OK ||
        lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
        lks_group_size(result) != 8) {
        goto cleanup;
    }
    for (index = 0; index < 8; ++index) {
        if (lks_group_item_at(result, index) != equal_items[index]) {
            goto cleanup;
        }
    }
    printf("Merge all equal keys: original Group order and in-Group order retained\n");
    lks_group_destroy(result); result = NULL;
    lks_group_batch_destroy(batch); batch = NULL;

    {
        int single_values[] = { 3, 1, 2 };
        void *single_items[] = { &single_values[0], &single_values[1], &single_values[2] };
        const LksGroup *source;
        void *snapshot_items[3];
        const LksPath *snapshot_paths[3];

        if (lks_group_batch_build(single_items, 3, 3, &comparator, &batch) != LKS_STATUS_OK) {
            goto cleanup;
        }
        source = lks_group_batch_group_at(batch, 0);
        for (index = 0; index < 3; ++index) {
            snapshot_items[index] = lks_group_item_at(source, index);
            snapshot_paths[index] = lks_group_path_at(source, index);
        }
        if (lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            result == source || lks_group_size(result) != 3) {
            goto cleanup;
        }
        for (index = 0; index < 3; ++index) {
            int path_order;
            if (lks_group_item_at(result, index) != snapshot_items[index] ||
                lks_path_compare(snapshot_paths[index], lks_group_path_at(result, index),
                    &path_order) != LKS_STATUS_OK || path_order != 0) {
                goto cleanup;
            }
        }
        lks_group_batch_destroy(batch); batch = NULL;
        if (lks_group_item_at(result, 0) != &single_values[1] ||
            lks_group_item_at(result, 1) != &single_values[2] ||
            lks_group_item_at(result, 2) != &single_values[0]) {
            goto cleanup;
        }
        printf("Merge all one Group: independent Path-preserving clone survives Batch destruction\n");
    }
    lks_group_destroy(result); result = NULL;

    {
        int single_values[] = { 40, 10, 30, 20 };
        void *single_items[] = { &single_values[0], &single_values[1],
            &single_values[2], &single_values[3] };
        void *expected_items[] = { &single_values[1], &single_values[3],
            &single_values[2], &single_values[0] };
        const LksGroup *source;
        const LksPath *source_paths[4];
        char source_text[4][128];
        const char *expected_paths[4];

        if (lks_group_batch_build(single_items, 4, 100, &comparator, &batch) != LKS_STATUS_OK ||
            lks_group_batch_group_count(batch) != 1) {
            goto cleanup;
        }
        source = lks_group_batch_group_at(batch, 0);
        for (index = 0; index < 4; ++index) {
            source_paths[index] = lks_group_path_at(source, index);
            if (lks_path_format(source_paths[index], source_text[index],
                    sizeof(source_text[index])) != LKS_STATUS_OK) {
                goto cleanup;
            }
            expected_paths[index] = source_text[index];
        }
        if (lks_group_batch_merge_all(batch, &comparator, &result) != LKS_STATUS_OK ||
            result == source || !merge_group_expect(result, expected_items,
                expected_paths, 4, &comparator)) {
            goto cleanup;
        }
        for (index = 0; index < 4; ++index) {
            int path_order;
            if (lks_path_compare(source_paths[index], lks_group_path_at(result, index),
                    &path_order) != LKS_STATUS_OK || path_order != 0) {
                goto cleanup;
            }
        }
        lks_group_batch_destroy(batch); batch = NULL;
        for (index = 0; index < 4; ++index) {
            char result_text[128];
            if (lks_group_item_at(result, index) != expected_items[index] ||
                lks_path_format(lks_group_path_at(result, index), result_text,
                    sizeof(result_text)) != LKS_STATUS_OK ||
                strcmp(result_text, expected_paths[index]) != 0) {
                goto cleanup;
            }
        }
        printf("Merge all one GroupSize 100: 10 20 30 40 Paths cloned exactly; Result survives Batch destruction\n");
    }
    lks_group_destroy(result); result = NULL;

    if (lks_group_batch_build(equal_items, 8, 2, &comparator, &batch) != LKS_STATUS_OK) {
        goto cleanup;
    }
    invalid_comparator = comparator;
    invalid_comparator.compare = NULL;
    {
        LksGroup *invalid_output = (LksGroup *)lks_group_batch_group_at(batch, 0);
        if (lks_group_batch_merge_all(NULL, &comparator, &invalid_output) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_output != NULL) {
            goto cleanup;
        }
        invalid_output = (LksGroup *)lks_group_batch_group_at(batch, 0);
        if (lks_group_batch_merge_all(batch, NULL, &invalid_output) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_output != NULL) {
            goto cleanup;
        }
        invalid_output = (LksGroup *)lks_group_batch_group_at(batch, 0);
        if (lks_group_batch_merge_all(batch, &invalid_comparator, &invalid_output) != LKS_STATUS_INVALID_ARGUMENT ||
            invalid_output != NULL ||
            lks_group_batch_merge_all(batch, &comparator, NULL) != LKS_STATUS_INVALID_ARGUMENT) {
            goto cleanup;
        }
    }
    valid = 1;
    printf("Merge all invalid arguments: passed; output reset to NULL\n");

cleanup:
    lks_group_destroy(result);
    lks_group_batch_destroy(batch);
    return valid;
}



int main(int argc, char **argv)
{
    int stage13_result;

    if (argc == 2 && strcmp(argv[1], "--stage14.1-only") == 0) {
        return lks_run_stage14_1_property_tests();
    }
    if (argc == 2 && strcmp(argv[1], "--stage14.2-only") == 0) {
        if (lks_run_stage14_1_property_tests() != 0) return 1;
        if (lks_run_stage14_2_stress_tests() != 0) return 1;
        return lks_run_stage14_2_oom_tests();
    }
    if (argc == 2 && strcmp(argv[1], "--stage14.2-oom-only") == 0) {
        return lks_run_stage14_2_oom_tests();
    }
    if (argc == 2 && strcmp(argv[1], "--stage14.2-release-smoke") == 0) {
        return lks_run_stage14_2_release_smoke();
    }
    if (argc == 2 && strcmp(argv[1], "--stage14.3-frozen-smoke") == 0) {
        return lks_run_stage14_3_frozen_smoke();
    }
    if (argc == 2 && strcmp(argv[1], "--public-api-usage-smoke") == 0) {
        return lks_run_public_api_usage_smoke();
    }

#ifdef _DEBUG
    stage13_result = lks_run_stage13_3_final_validation();
#else
    stage13_result = lks_run_stage13_3_release_smoke();
#endif
    if (stage13_result != 0) return stage13_result;
    return lks_run_stage14_1_property_tests();
}







































