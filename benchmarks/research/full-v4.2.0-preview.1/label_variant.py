"""Generate isolated dynamic midpoint-label prototype, never edit product files."""
from pathlib import Path
import sys

out = Path(sys.argv[1])
out.mkdir(exist_ok=True, parents=True)
here = Path(__file__).resolve().parent
header = Path('src/lks_order_internal.h').read_text()
header = header.replace('size_t subtree_blocks, count;', 'size_t subtree_blocks, count;\n    uint64_t precedence;')
header = header.replace('int callback_active;', 'int callback_active;\n    int precedence_disabled;')
(out/'lks_order_internal.h').write_text(header)
s = (here/'combined-order.c.txt').read_text()
needle = '    v4_mark_index(order, block);\n    ++order->blocks;'
assert s.count(needle)==1
s = s.replace(needle, '''    if (!anchor) order->precedence_disabled = 0;
    if (!order->precedence_disabled) {
        uint64_t lower = anchor ? anchor->precedence : 0;
        uint64_t upper = block->next ? block->next->precedence : UINT64_MAX;
        if (upper - lower <= 1) order->precedence_disabled = 1;
        else if (!block->next && upper - lower > UINT64_C(4294967296))
            block->precedence = lower + UINT64_C(4294967296);
        else block->precedence = lower + (upper - lower) / 2;
    }
''' + needle)
needle='    while (left->parent != right->parent) {'
assert s.count(needle)==1
s=s.replace(needle, '''    if (!order->precedence_disabled) {
        *out = left->precedence < right->precedence ? -1 : 1;
        return LKS_STATUS_OK;
    }
''' + needle)
(out/'order.c').write_text(s)
s = Path('src/order_bulk.c').read_text()
s = s.replace('b->owner = o; b->previous', 'b->owner = o; b->precedence = ((uint64_t)i + 1) * (UINT64_MAX / ((uint64_t)blocks + 1)); b->previous')
(out/'order_bulk.c').write_text(s)
