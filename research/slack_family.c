#include "../src/lks_slack_research_internal.h"

static unsigned int slot_at(const LksSlackPlan *p, uint64_t rank, size_t i)
{
    unsigned int slot;
    if (i < p->depth - p->variable) return p->slots[i];
    slot = (unsigned int)((rank >> (16 * (p->depth - i - 1))) & 65535u);
    if (i == 0 && p->direction == LKS_DIRECTION_NEGATIVE) slot = 65535u - slot;
    return slot;
}

/* Restricted-family comparison, not a replacement for public Path ordering.
 * Tests compare this allocation-free projection against lks_path_compare. */
int lks_slack_compare_rank(const LksSlackPlan *p, uint64_t rank,
    const LksPath *path)
{
    LksDirection direction = lks_path_direction(path);
    size_t depth = lks_path_depth(path), i;
    if (p->direction != direction)
        return (p->direction > direction) - (p->direction < direction);
    for (i = 0; i < p->depth && i < depth; ++i) {
        size_t level;
        unsigned int slot, own = slot_at(p, rank, i);
        (void)lks_path_get_level(path, i, &level);
        (void)lks_path_get_slot(path, i, &slot);
        if (p->levels[i] != level) return p->levels[i] > level ? -1 : 1;
        if (own != slot) {
            int cmp = own > slot ? 1 : -1;
            return i == 0 && direction == LKS_DIRECTION_NEGATIVE ? -cmp : cmp;
        }
    }
    return (p->depth > depth) - (p->depth < depth);
}

static uint64_t boundary(const LksSlackPlan *p, const LksPath *bound,
    uint64_t limit, int upper)
{
    uint64_t low = 0, high = limit;
    if (bound == NULL) return upper ? limit : 0;
    /* Lower rank is first F(rank) > left; upper rank is first F(rank) >= right.
     * The half-open rank interval handles parents, descendants and outsiders. */
    while (low < high) {
        uint64_t middle = low + (high - low) / 2;
        int cmp = lks_slack_compare_rank(p, middle, bound);
        if (cmp < 0 || (!upper && cmp == 0)) low = middle + 1;
        else high = middle;
    }
    return low;
}

int lks_slack_occupied(const LksSlackPlan *p, const LksPath *path)
{
    uint64_t rank = boundary(p, path, p->upper, 1);
    return rank >= p->lower && rank < p->upper &&
        lks_slack_compare_rank(p, rank, path) == 0;
}

int lks_slack_plan(const LksPath *reference, const LksPath *left,
    const LksPath *right, size_t population, LksSlackPlan *out)
{
    LksSlackPlan p;
    size_t depth, i, maximum;
    if (reference == NULL || population == 0 || population == (size_t)-1 ||
        lks_path_direction(reference) == LKS_DIRECTION_ZERO) return 0;
    p.direction = lks_path_direction(reference);
    maximum = lks_path_depth(reference);
    if (maximum > 6) maximum = 6;
    for (depth = 1; depth <= maximum; ++depth) {
        uint64_t capacity, divisor, limit;
        p.depth = depth; p.variable = depth < 2 ? 1 : 2;
        for (i = 0; i < depth; ++i) {
            (void)lks_path_get_level(reference, i, &p.levels[i]);
            (void)lks_path_get_slot(reference, i, &p.slots[i]);
        }
        limit = UINT64_C(1) << (16 * p.variable);
        p.lower = boundary(&p, left, limit, 0);
        p.upper = boundary(&p, right, limit, 1);
        if (p.upper <= p.lower) continue;
        capacity = p.upper - p.lower;
        /* population includes pending item. Require sixteen rank units per
         * gap, including both exterior gaps, not just N free coordinates.
         * Reject before allocating Paths. Works even on 32-bit size_t. */
        if (population >= capacity) continue;
        divisor = (uint64_t)population + 1;
        p.stride = capacity / divisor;
        if (p.stride < 16) continue;
        *out = p;
        return 1;
    }
    return 0;
}

LksStatus lks_slack_materialize(const LksSlackPlan *p, uint64_t rank,
    LksPath **out)
{
    LksPath *path;
    size_t i;
    LksStatus status;
    *out = NULL;
    path = lks_path_create_at_level(p->direction, slot_at(p, rank, 0), p->levels[0]);
    if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    for (i = 1; i < p->depth; ++i) {
        status = lks_path_append_at_level(path, slot_at(p, rank, i), p->levels[i]);
        if (status != LKS_STATUS_OK) { lks_path_destroy(path); return status; }
    }
    *out = path;
    return LKS_STATUS_OK;
}
