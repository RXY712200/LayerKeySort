#include "layerkeysort_mini.h"
#include <stdio.h>
#define REQUIRE(x) do { if (!(x)) { failed = 1; goto cleanup; } } while (0)
int main(void)
{
    LksMiniOrder *order = NULL, *other = NULL;
    LksMiniHandle *a = NULL, *b = NULL, *c = NULL, *cursor = NULL;
    int value = 7, result = 9, failed = 0;
    size_t count = 0;
    void *item = NULL;
    REQUIRE(lks_mini_create(&order) == LKS_MINI_OK);
    REQUIRE(lks_mini_create(&other) == LKS_MINI_OK);
    REQUIRE(lks_mini_insert_back(order, &value, &a) == LKS_MINI_OK);
    REQUIRE(lks_mini_insert_back(order, &value, &b) == LKS_MINI_OK && a != b);
    REQUIRE(lks_mini_insert_before(order, b, NULL, &c) == LKS_MINI_OK);
    REQUIRE(lks_mini_move_front(order, b) == LKS_MINI_OK);
    REQUIRE(lks_mini_compare(order, b, a, &result) == LKS_MINI_OK && result == -1);
    REQUIRE(lks_mini_move_back(other, a) == LKS_MINI_WRONG_ORDER);
    REQUIRE(lks_mini_first(order, &cursor) == LKS_MINI_OK && cursor == b);
    while (cursor) {
        REQUIRE(lks_mini_item(order, cursor, &item) == LKS_MINI_OK);
        REQUIRE(item == NULL || item == &value);
        ++count;
        REQUIRE(lks_mini_next(order, cursor, &cursor) == LKS_MINI_OK);
    }
    REQUIRE(count == 3);
    REQUIRE(lks_mini_remove(order, a) == LKS_MINI_OK);
    REQUIRE(lks_mini_item(order, b, &item) == LKS_MINI_OK && item == &value);
    REQUIRE(lks_mini_size(order, &count) == LKS_MINI_OK && count == 2);
cleanup:
    lks_mini_destroy(order); lks_mini_destroy(other);
    if (value != 7) return 1;
    if (!failed) puts("external Mini consumer: PASS");
    return failed;
}
