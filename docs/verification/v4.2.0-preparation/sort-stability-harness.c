/* Isolated documentation verification, not a production regression fixture. */
#include <layerkeysort.h>
#include <stdio.h>

typedef struct Item { int key; unsigned id; } Item;

static int compare_key(const void *a, const void *b, void *context)
{
    const Item *left = (const Item *)a, *right = (const Item *)b;
    unsigned *calls = (unsigned *)context;
    ++*calls;
    return (left->key > right->key) - (left->key < right->key);
}

int main(void)
{
    Item values[] = {{3,0}, {1,1}, {2,2}, {1,3}, {3,4}, {2,5}, {1,6}, {0,7}};
    void *items[8];
    const unsigned expected[] = {7,1,3,6,2,5,0,4};
    unsigned calls = 0;
    size_t i;
    for (i = 0; i < 8; ++i) items[i] = &values[i];
    if (&values[1] == &values[3] || values[1].key != values[3].key) return 1;
    if (lks_sort(items, 8, compare_key, &calls) != LKS_STATUS_OK || calls == 0) return 2;
    for (i = 0; i < 8; ++i) {
        const Item *item = (const Item *)items[i];
        if (item != &values[expected[i]] || item->id != expected[i]) return 3;
        if (i > 0 && ((const Item *)items[i-1])->key > item->key) return 4;
    }
    puts("PASS: OK status, sorted keys, eight exact occurrence addresses, stable equal-key input order, comparator context");
    return 0;
}
