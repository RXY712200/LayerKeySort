#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../src/lks_slack_research_internal.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "slack check line %d\n", __LINE__); exit(1); } } while (0)
static uint32_t state = 0x193aab17u;
static uint32_t next_random(void) { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }

static void model(void)
{
    size_t d, j, cases = 0;
    int direction;
    for (direction = -1; direction <= 1; direction += 2) {
        for (d = 1; d <= 6; ++d) {
            LksSlackPlan plan = {0};
            plan.depth = d; plan.variable = d < 2 ? 1 : 2;
            plan.direction = (LksDirection)direction;
            for (j = 0; j < d; ++j) { plan.levels[j] = j * 256; plan.slots[j] = 32768; }
            for (j = 0; j < 2000; ++j) {
                LksPath *own = NULL, *bound;
                uint64_t rank = next_random() & (d == 1 ? 65535u : UINT32_MAX);
                size_t k, depth = next_random() % 8 + 1;
                int cmp;
                LksDirection bd = j % 7 == 0 ? LKS_DIRECTION_ZERO :
                    j % 3 == 0 ? (LksDirection)-direction : (LksDirection)direction;
                CHECK(lks_slack_materialize(&plan, rank, &own) == LKS_STATUS_OK);
                bound = bd == LKS_DIRECTION_ZERO ? lks_path_create_zero() :
                    lks_path_create_at_level(bd, next_random() & 65535u, j % 2 ? 0 : 255);
                CHECK(bound != NULL);
                for (k = 1; k < depth && bd != LKS_DIRECTION_ZERO; ++k)
                    CHECK(lks_path_append_at_level(bound, next_random() & 65535u,
                        256 * k + (j % 2 ? 0 : 255)) == LKS_STATUS_OK);
                CHECK(lks_path_compare(own, bound, &cmp) == LKS_STATUS_OK);
                CHECK(cmp == lks_slack_compare_rank(&plan, rank, bound));
                CHECK(lks_path_compare(own, own, &cmp) == LKS_STATUS_OK && cmp == 0);
                CHECK(lks_slack_compare_rank(&plan, rank, own) == 0);
                plan.lower = 0; plan.upper = UINT64_C(1) << (16 * plan.variable);
                CHECK(lks_slack_occupied(&plan, own));
                lks_path_destroy(own); lks_path_destroy(bound); ++cases;
            }
        }
    }
    /* Exact one-slot capacity, including root reversal, unequal-level outsiders,
     * parents/descendants, and empty intervals. Brute enumeration is the oracle. */
    for (direction = -1; direction <= 1; direction += 2) {
        unsigned int slots[] = {0, 1, 53, 32768, 65534, 65535};
        for (j = 0; j < sizeof(slots) / sizeof(slots[0]); ++j) {
            LksPath *reference = lks_path_create((LksDirection)direction, 32768);
            LksPath *a = lks_path_create((LksDirection)direction, slots[j]);
            LksSlackPlan p;
            int ready = lks_slack_plan(reference, direction > 0 ? a : NULL,
                direction > 0 ? NULL : a, 9, &p);
            uint64_t expected = 65535u - slots[j];
            CHECK(ready == (expected / 10 >= 16));
            if (ready) {
                uint64_t r, occupied = 0;
                CHECK(p.upper - p.lower == expected);
                for (r = 0; r < 65536; ++r) {
                    LksPath *path = lks_path_create((LksDirection)direction, (unsigned int)r);
                    int cmp;
                    CHECK(path != NULL && lks_path_compare(path, a, &cmp) == LKS_STATUS_OK);
                    occupied += direction > 0 ? cmp > 0 : cmp < 0;
                    lks_path_destroy(path);
                }
                CHECK(occupied == expected);
            }
            lks_path_destroy(reference); lks_path_destroy(a);
        }
    }
    {
        LksPath *parent = lks_path_create(LKS_DIRECTION_POSITIVE, 32768);
        LksPath *left = lks_path_clone(parent), *right = lks_path_clone(parent);
        LksSlackPlan p;
        CHECK(left != NULL && right != NULL);
        CHECK(lks_path_append(left, 0) == LKS_STATUS_OK);
        CHECK(lks_path_append(right, 65535) == LKS_STATUS_OK);
        CHECK(lks_slack_plan(left, left, right, 100, &p));
        CHECK(p.depth == 2 && p.upper - p.lower == 65534);
        CHECK(!lks_slack_occupied(&p, left) && !lks_slack_occupied(&p, right));
        CHECK(lks_slack_plan(left, parent, right, 100, &p));
        CHECK(p.upper - p.lower == 65535);
        CHECK(!lks_slack_plan(left, right, left, 100, &p));
        lks_path_destroy(parent); lks_path_destroy(left); lks_path_destroy(right);
    }
    printf("slack family projection cases=%zu; exhaustive one-slot and prefix/two-slot boundaries PASS\n", cases);
}

static int compare_item(const void *a, const void *b, void *context)
{ int x = *(const int *)a, y = *(const int *)b; (void)context; return (x > y) - (x < y); }

static void oom_case(int direction, int pending)
{
    enum { N = 130 };
    int items[N + 1];
    LksPath *original[N];
    const LksTreeNode *nodes[N + 1];
    LksComparator comparator = {compare_item, NULL};
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    size_t i, failures = 0;
    CHECK(tree != NULL);
    for (i = 0; i < N; ++i) {
        size_t k;
        items[i] = (int)(i * 2);
        original[i] = lks_path_create((LksDirection)direction, 32768);
        CHECK(original[i] != NULL);
        for (k = 1; k < 6; ++k)
            CHECK(lks_path_append(original[i], k == 5 ? 200u + (unsigned int)i : 32768u) == LKS_STATUS_OK);
        CHECK(lks_ordered_tree_test_seed_path(tree, original[i], &items[i]) == LKS_STATUS_OK);
    }
    items[N] = pending;
    for (i = 1; i < 10000; ++i) {
        const LksTreeNode *inserted = NULL;
        LksAllocStats before = lks_alloc_stats_get();
        LksStatus status;
        size_t k;
        lks_tree_repair_stats_reset();
        lks_alloc_test_fail_on_attempt(i);
        status = lks_ordered_tree_insert(tree, &items[N], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            CHECK(inserted != NULL && stats.slack_preparations > 0 && stats.successes == 1);
            CHECK(lks_tree_internal_fill_ordered(lks_ordered_tree_internal_index(tree), nodes, N + 1) == LKS_STATUS_OK);
            for (k = 1; k <= N; ++k) {
                int cmp;
                CHECK(*(int *)lks_tree_node_item(nodes[k - 1]) <= *(int *)lks_tree_node_item(nodes[k]));
                CHECK(lks_path_compare(lks_tree_node_path(nodes[k - 1]), lks_tree_node_path(nodes[k]), &cmp) == LKS_STATUS_OK && cmp < 0);
            }
            /* Removal must make no allocation and preserve every survivor Path. */
            {
                const LksPath *borrowed[N + 1];
                void *removed = NULL;
                for (k = 0; k <= N; ++k) borrowed[k] = lks_tree_node_path(nodes[k]);
                lks_alloc_test_fail_on_attempt(1);
                CHECK(lks_ordered_tree_remove_path(tree, lks_tree_node_path(inserted), &removed) == LKS_STATUS_OK && removed == &items[N]);
                CHECK(lks_alloc_test_get_attempt_count() == 0);
                lks_alloc_test_disable_failure();
                for (k = 0; k <= N; ++k) if (nodes[k] != inserted) {
                    const LksTreeNode *found = NULL;
                    CHECK(lks_ordered_tree_find_path(tree, borrowed[k], &found) == LKS_STATUS_OK && found == nodes[k]);
                }
            }
            break;
        }
        CHECK(status == LKS_STATUS_OUT_OF_MEMORY && inserted == NULL);
        CHECK(lks_ordered_tree_size(tree) == N);
        CHECK(lks_alloc_stats_get().live_bytes == before.live_bytes && lks_alloc_stats_get().live_blocks == before.live_blocks);
        for (k = 0; k < N; ++k) {
            const LksTreeNode *found = NULL;
            CHECK(lks_ordered_tree_find_path(tree, original[k], &found) == LKS_STATUS_OK && lks_tree_node_item(found) == &items[k]);
        }
        {
            LksTreeInternalProfile profile;
            CHECK(lks_tree_internal_profile(lks_ordered_tree_internal_index(tree), &profile) == LKS_STATUS_OK && profile.balance_valid);
        }
        ++failures;
    }
    CHECK(i < 10000 && failures > 0);
    printf("slack OOM direction=%d pending=%d failures=%zu PASS\n", direction, pending, failures);
    for (i = 0; i < N; ++i) lks_path_destroy(original[i]);
    lks_ordered_tree_destroy(tree);
    CHECK(lks_alloc_stats_get().live_bytes == 0);
}

int main(void)
{
    model();
    oom_case(1, 127); oom_case(-1, 127); oom_case(1, 128);
    return 0;
}
