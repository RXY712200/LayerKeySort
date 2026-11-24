/* Research implementation of bounded ordered-range redistribution.
 * Zero denotes a block that still uses AVL precedence. */
static void preview_tag_attach(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *left = block, *right = block;
    size_t width = 1;
    for (;;) {
        uint64_t lower = left->previous ? left->previous->tag : 0;
        uint64_t upper = right->next ? right->next->tag : UINT64_MAX;
        int bounded = (!left->previous || lower) && upper;
        if (bounded && upper > lower) {
            uint64_t stride = (upper - lower) / (width + 1);
            /* Spread a repaired range to leave room for further insertions. */
            if (stride >= (width == 1 ? 1 : width)) {
                if (width == 1 && !right->next && stride > UINT64_C(1099511627776))
                    stride = UINT64_C(1099511627776);
                uint64_t label = lower;
                for (LksOrderBlock *p = left;; p = p->next) {
                    label += stride;
                    p->tag = label;
                    if (p == right) break;
                }
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
                order->tag_writes += width;
                if (width > 1) ++order->tag_repairs;
                if (width > order->max_tag_writes) order->max_tag_writes = width;
#else
                (void)order;
#endif
                return;
            }
        }
        if (width == 64 || (!left->previous && !right->next)) {
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            ++order->tag_fallbacks;
#endif
            return;
        }
        if (left->previous && (!right->next || !lower || width % 2 == 0)) left = left->previous;
        else right = right->next;
        ++width;
    }
}
