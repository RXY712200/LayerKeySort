/* Move only the boundary residents, keeping the original occupancy policy. */
static void v4_repair(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *left = block->next ? block : block->previous;
    LksOrderBlock *right = left->next;
    size_t old_left = left->count, old_right = right->count;
    size_t total = old_left + old_right;
    size_t target = total <= LKS_ORDER_BLOCK_CAPACITY ? total : total / 2;
    if (target >= old_left) {
        size_t moved = target - old_left;
        memcpy(left->records + old_left, right->records, moved * sizeof(*left->records));
        memmove(right->records, right->records + moved,
            (old_right - moved) * sizeof(*right->records));
    } else {
        size_t moved = old_left - target;
        memmove(right->records + moved, right->records, old_right * sizeof(*right->records));
        memcpy(right->records, left->records + target, moved * sizeof(*right->records));
    }
    left->count = target;
    right->count = total - target;
    /* The just-removed or moved resident can leave stale source locations. */
    v4_locations(order, left);
    if (!right->count) {
        V4_COUNT(order, local_blocks);
        V4_COUNT(order, merges);
        v4_detach(order, right);
        lks_free(right);
    } else {
        v4_locations(order, right);
        V4_COUNT(order, redistributions);
    }
}
