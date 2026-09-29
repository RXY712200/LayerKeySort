#include <stdio.h>
#include "layerkeysort.h"

typedef struct Item { int key; int original; } Item;

static int compare_items(const void *left, const void *right, void *context)
{
    const Item *a = (const Item *)left;
    const Item *b = (const Item *)right;
    (void)context;
    return (a->key > b->key) - (a->key < b->key);
}

int main(void)
{
    Item values[] = {{2, 0}, {1, 1}, {2, 2}, {1, 3}};
    void *items[] = {&values[0], &values[1], &values[2], &values[3]};
    size_t index;
    LksStatus status = lks_sort(items, 4, compare_items, NULL);
    if (status != LKS_STATUS_OK) {
        fprintf(stderr, "Sort failed: %s\n", lks_status_string(status));
        return 1;
    }
    for (index = 0; index < 4; ++index) {
        const Item *item = (const Item *)items[index];
        printf("key=%d original=%d\n", item->key, item->original);
    }
    return 0;
}
