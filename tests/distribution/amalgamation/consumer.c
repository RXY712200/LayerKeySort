#include "layerkeysort.h"

int main(void)
{
    LksPath *path = 0;
    LksOrder *order = lks_order_create();
    const LksOrderHandle *handle = 0;
    int item = 1;
    char key[32];
    if (!order) return 4;
    if (lks_order_insert_back(order, &item, &handle) != LKS_STATUS_OK ||
        lks_order_first(order) != handle || lks_order_item(handle) != &item ||
        lks_order_remove(order, handle, 0) != LKS_STATUS_OK || lks_order_size(order)) {
        lks_order_destroy(order); return 5;
    }
    lks_order_destroy(order);
    if (lks_path_parse("0222", &path) != LKS_STATUS_OK || !path) return 1;
    if (lks_path_order_key_format(path, key, sizeof key) != LKS_STATUS_OK) {
        lks_path_destroy(path);
        return 2;
    }
    lks_path_destroy(path);
    return key[0] == 'L' && key[1] == 'K' && key[2] == '1' ? 0 : 3;
}
