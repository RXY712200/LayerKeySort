#include "layerkeysort.h"

int main(void)
{
    LksPath *path = 0;
    char key[32];
    if (lks_path_parse("0222", &path) != LKS_STATUS_OK || !path) return 1;
    if (lks_path_order_key_format(path, key, sizeof key) != LKS_STATUS_OK) {
        lks_path_destroy(path);
        return 2;
    }
    lks_path_destroy(path);
    return key[0] == 'L' && key[1] == 'K' && key[2] == '1' ? 0 : 3;
}
