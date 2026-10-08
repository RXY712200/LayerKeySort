#include "layerkeysort_mini.h"
#include <stdio.h>

int main(void)
{
    int values[] = {10, 20, 30};
    LksMiniOrder *order = NULL;
    LksMiniHandle *first = NULL, *middle = NULL, *last = NULL, *cursor = NULL;
    size_t size = 0;
    if (lks_mini_create(&order) != LKS_MINI_OK) return 1;
    if (lks_mini_insert_back(order, &values[0], &first) != LKS_MINI_OK ||
        lks_mini_insert_back(order, &values[2], &last) != LKS_MINI_OK ||
        lks_mini_insert_before(order, last, &values[1], &middle) != LKS_MINI_OK ||
        lks_mini_remove(order, middle) != LKS_MINI_OK ||
        lks_mini_size(order, &size) != LKS_MINI_OK || size != 2 ||
        lks_mini_first(order, &cursor) != LKS_MINI_OK || cursor != first) {
        lks_mini_destroy(order);
        return 1;
    }
    while (cursor) {
        void *item = NULL;
        LksMiniHandle *next = NULL;
        if (lks_mini_item(order, cursor, &item) != LKS_MINI_OK ||
            lks_mini_next(order, cursor, &next) != LKS_MINI_OK) {
            lks_mini_destroy(order);
            return 1;
        }
        printf("%d\n", *(int *)item);
        cursor = next;
    }
    lks_mini_destroy(order); /* Stack-owned values remain caller-owned. */
    return 0;
}
