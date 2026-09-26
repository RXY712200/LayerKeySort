#include "hps.h"

typedef struct PublicApiItem {
    int key;
    int input_order;
} PublicApiItem;

static int public_api_compare_items(const void *left, const void *right,
    void *context)
{
    const PublicApiItem *a = (const PublicApiItem *)left;
    const PublicApiItem *b = (const PublicApiItem *)right;
    (void)context;
    if (a->key < b->key) return -1;
    if (a->key > b->key) return 1;
    return 0;
}

int hps_public_api_usage_smoke(void)
{
    PublicApiItem values[] = {
        { 2, 0 }, { 1, 1 }, { 2, 2 },
        { 1, 3 }, { 2, 4 }, { 0, 5 }
    };
    void *items[sizeof(values) / sizeof(values[0])];
    HpsComparator comparator = { public_api_compare_items, NULL };
    HpsGroupBatch *batch = NULL;
    HpsGroup *result = NULL;
    static const int expected_key[] = { 0, 1, 1, 2, 2, 2 };
    static const int expected_input_order[] = { 5, 1, 3, 0, 2, 4 };
    size_t index;
    int valid = 1;

    for (index = 0; index < sizeof(values) / sizeof(values[0]); ++index) {
        items[index] = &values[index];
    }
    if (hps_group_batch_build(items, sizeof(values) / sizeof(values[0]), 3,
            &comparator, &batch) != HPS_STATUS_OK || batch == NULL ||
        hps_group_batch_group_count(batch) != 2 ||
        hps_group_batch_total_size(batch) != 6) {
        valid = 0;
        goto cleanup;
    }
    if (hps_group_batch_merge_all(batch, &comparator, &result) != HPS_STATUS_OK ||
        result == NULL || hps_group_size(result) != 6) {
        valid = 0;
        goto cleanup;
    }
    for (index = 0; index < 6; ++index) {
        const PublicApiItem *item = (const PublicApiItem *)
            hps_group_item_at(result, index);
        if (item == NULL || item->key != expected_key[index] ||
            item->input_order != expected_input_order[index] ||
            item != &values[expected_input_order[index]]) {
            valid = 0;
            break;
        }
    }

cleanup:
    hps_group_destroy(result);
    hps_group_batch_destroy(batch);
    return valid;
}
