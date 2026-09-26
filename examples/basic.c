#include <stdio.h>
#include "layerkeysort.h"

typedef struct Item {
    int key;
    int source_order;
} Item;

static int compare_item_key(const void *left, const void *right, void *context)
{
    const Item *a = (const Item *)left;
    const Item *b = (const Item *)right;
    (void)context;
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

int main(void)
{
    Item items[] = {
        { 2, 0 }, { 1, 1 }, { 2, 2 },
        { 1, 3 }, { 2, 4 }, { 0, 5 }
    };
    void *item_pointers[sizeof(items) / sizeof(items[0])];
    LksComparator comparator = { compare_item_key, NULL };
    LksGroupBatch *batch = NULL;
    LksGroup *result = NULL;
    size_t index;
    LksStatus status;

    for (index = 0; index < sizeof(items) / sizeof(items[0]); ++index) {
        item_pointers[index] = &items[index];
    }
    status = lks_group_batch_build(item_pointers,
        sizeof(items) / sizeof(items[0]), 3, &comparator, &batch);
    if (status != LKS_STATUS_OK) {
        fprintf(stderr, "Batch build failed: %s\n", lks_status_string(status));
        return 1;
    }
    status = lks_group_batch_merge_all(batch, &comparator, &result);
    if (status != LKS_STATUS_OK) {
        fprintf(stderr, "Merge failed: %s\n", lks_status_string(status));
        lks_group_batch_destroy(batch);
        return 1;
    }
    for (index = 0; index < lks_group_size(result); ++index) {
        const Item *item = (const Item *)lks_group_item_at(result, index);
        printf("key=%d source_order=%d\n", item->key, item->source_order);
    }
    lks_group_destroy(result);
    lks_group_batch_destroy(batch);
    return 0;
}
