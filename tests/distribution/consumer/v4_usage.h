#ifndef LKS_CONSUMER_V4_USAGE_H
#define LKS_CONSUMER_V4_USAGE_H
/* Public-header-only use, compiled as both C and C++ by external consumers. */
#include "layerkeysort.h"
static int v4_consumer_usage(void)
{
    LksOrder *order = lks_order_create();
    const LksOrderHandle *a = 0, *b = 0, *c = 0, *d = 0;
    int items[4] = {1, 2, 3, 4}, comparison;
    void *removed = 0;
    int failed = 1;
    if (!order) return 1;
    if (lks_order_insert_front(order, &items[0], &a) != LKS_STATUS_OK ||
        lks_order_insert_back(order, &items[3], &d) != LKS_STATUS_OK ||
        lks_order_insert_before(order, d, &items[2], &c) != LKS_STATUS_OK ||
        lks_order_insert_after(order, a, &items[1], &b) != LKS_STATUS_OK) goto done;
    if (lks_order_size(order) != 4 || lks_order_first(order) != a ||
        lks_order_last(order) != d || lks_order_next(a) != b ||
        lks_order_previous(d) != c || lks_order_item(c) != &items[2]) goto done;
    if (lks_order_compare(order, a, d, &comparison) != LKS_STATUS_OK || comparison >= 0 ||
        lks_order_remove(order, b, &removed) != LKS_STATUS_OK || removed != &items[1] ||
        lks_order_next(a) != c) goto done;
    failed = 0;
done:
    lks_order_destroy(order);
    return failed;
}
#endif
