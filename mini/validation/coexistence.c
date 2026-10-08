/* Optional repository verification; not a required standalone Mini source. */
#include "layerkeysort.h"
#include "layerkeysort_mini.h"
#include <stdio.h>
#define REQUIRE(x) do { if (!(x)) { failed = 1; goto cleanup; } } while (0)
int main(void)
{
    LksOrder *full = lks_order_create();
    LksMiniOrder *mini = NULL;
    const LksOrderHandle *fa = NULL, *fb = NULL;
    LksMiniHandle *ma = NULL, *mb = NULL;
    int values[2] = {1,2}, result = 9, failed = 0;
    size_t count = 0;
    REQUIRE(full != NULL && lks_mini_create(&mini) == LKS_MINI_OK);
    REQUIRE(lks_order_insert_back(full, &values[0], &fa) == LKS_STATUS_OK);
    REQUIRE(lks_order_insert_back(full, &values[1], &fb) == LKS_STATUS_OK);
    REQUIRE(lks_mini_insert_back(mini, &values[0], &ma) == LKS_MINI_OK);
    REQUIRE(lks_mini_insert_back(mini, &values[1], &mb) == LKS_MINI_OK);
    REQUIRE(lks_order_move_back(full, fa) == LKS_STATUS_OK);
    REQUIRE(lks_order_compare(full, fb, fa, &result) == LKS_STATUS_OK && result < 0);
    REQUIRE(lks_mini_compare(mini, ma, mb, &result) == LKS_MINI_OK && result == -1);
    REQUIRE(lks_mini_move_front(mini, mb) == LKS_MINI_OK);
    REQUIRE(lks_order_first(full) == fb && lks_order_item(fa) == &values[0]);
    REQUIRE(lks_mini_compare(mini, mb, ma, &result) == LKS_MINI_OK && result == -1);
    REQUIRE(lks_order_size(full) == 2);
    REQUIRE(lks_mini_size(mini, &count) == LKS_MINI_OK && count == 2);
cleanup:
    lks_order_destroy(full); lks_mini_destroy(mini);
    if (!failed) puts("Full/Mini coexistence: PASS");
    return failed;
}
