#ifndef LKS_CONSUMER_V4_USAGE_H
#define LKS_CONSUMER_V4_USAGE_H
/* Public-header-only use, compiled as both C and C++ by external consumers. */
#include "layerkeysort.h"
static int v4_consumer_compare(const void *a, const void *b, void *context)
{
    int x=*(const int *)a, y=*(const int *)b;
    (void)context;
    return (x>y)-(x<y);
}
static int v4_consumer_usage(void)
{
    LksOrder *order = lks_order_create();
    LksManagedOrder *managed = 0;
    LksOrderCursor *cursor = 0;
    LksComparator comparator = {v4_consumer_compare, 0};
    const LksOrderHandle *a = 0, *b = 0, *c = 0, *d = 0;
    const LksOrderHandle *p = 0, *e = 0, *s = 0, *it = 0;
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
    if (lks_order_move_front(order,d)!=LKS_STATUS_OK ||
        lks_order_move_back(order,d)!=LKS_STATUS_OK ||
        lks_order_move_before(order,c,b)!=LKS_STATUS_OK ||
        lks_order_move_after(order,c,b)!=LKS_STATUS_OK) goto done;
    if (lks_order_compare(order, a, d, &comparison) != LKS_STATUS_OK || comparison >= 0 ||
        lks_order_remove(order, b, &removed) != LKS_STATUS_OK || removed != &items[1] ||
        lks_order_next(a) != c) goto done;
    if (lks_order_cursor_create(order,0,&cursor)!=LKS_STATUS_OK ||
        lks_order_cursor_next(cursor,&it)!=LKS_STATUS_OK || it!=a) goto done;
    lks_order_cursor_destroy(cursor); cursor=0;
    managed=lks_managed_order_create(&comparator);
    if (!managed || lks_managed_order_insert(managed,&items[3],&d)!=LKS_STATUS_OK ||
        lks_managed_order_insert(managed,&items[0],&a)!=LKS_STATUS_OK ||
        lks_managed_order_insert(managed,&items[1],&b)!=LKS_STATUS_OK) goto done;
    if (lks_managed_order_size(managed)!=3 || lks_managed_order_first(managed)!=a ||
        lks_managed_order_last(managed)!=d ||
        lks_managed_order_locate(managed,&items[1],&p,&e,&s)!=LKS_STATUS_OK || p!=a || e!=b || s!=d ||
        lks_managed_order_compare(managed,a,d,&comparison)!=LKS_STATUS_OK || comparison>=0 ||
        lks_managed_order_cursor_create(managed,1,&cursor)!=LKS_STATUS_OK ||
        lks_order_cursor_next(cursor,&it)!=LKS_STATUS_OK || it!=d ||
        lks_managed_order_remove(managed,b,&removed)!=LKS_STATUS_OK || removed!=&items[1] ||
        lks_order_cursor_next(cursor,&it)!=LKS_STATUS_INVALIDATED || it) goto done;
    failed = 0;
done:
    lks_order_cursor_destroy(cursor);
    lks_managed_order_destroy(managed);
    lks_order_destroy(order);
    return failed;
}
#endif
