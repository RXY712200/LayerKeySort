#include "layerkeysort.h"
#include <stdio.h>

int main(void)
{
    int background = 10, player = 20, effects = 15;
    const LksOrderHandle *back = NULL, *actor = NULL, *effect = NULL, *it;
    LksOrder *order = lks_order_create();
    void *removed = NULL;
    int comparison;
    if (!order) return 1;
    if (lks_order_insert_front(order, &background, &back) != LKS_STATUS_OK ||
        lks_order_insert_back(order, &player, &actor) != LKS_STATUS_OK ||
        lks_order_insert_before(order, actor, &effects, &effect) != LKS_STATUS_OK ||
        lks_order_compare(order, back, actor, &comparison) != LKS_STATUS_OK ||
        comparison >= 0) {
        lks_order_destroy(order); return 1;
    }
    /* Logical traversal, with stable residence handles and caller-owned items. */
    for (it = lks_order_first(order); it; it = lks_order_next(it))
        printf("%d\n", *(int *)lks_order_item(it));
    if (lks_order_remove(order, effect, &removed) != LKS_STATUS_OK || removed != &effects) {
        lks_order_destroy(order); return 1;
    }
    effect = NULL; /* Removal expires this residence, never the caller's item. */
    if (lks_order_size(order) != 2 || lks_order_previous(actor) != back) {
        lks_order_destroy(order); return 1;
    }
    /* Move preserves the background handle and item association. */
    if (lks_order_move_back(order, back) != LKS_STATUS_OK ||
        lks_order_last(order) != back || lks_order_item(back) != &background) {
        lks_order_destroy(order); return 1;
    }
    lks_order_destroy(order);
    return 0;
}
