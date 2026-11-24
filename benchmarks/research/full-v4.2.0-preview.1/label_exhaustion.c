#include "lks_order_internal.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) exit(1); } while (0)
int main(void) {
    int payload=1, cmp;
    LksOrder *order=lks_order_create();
    const LksOrderHandle *a,*b,*h;
    CHECK(order);
    CHECK(lks_order_insert_back(order,&payload,&a)==LKS_STATUS_OK);
    CHECK(lks_order_insert_back(order,&payload,&b)==LKS_STATUS_OK);
    for (size_t i=0; i<128; ++i) CHECK(lks_order_insert_back(order,&payload,&h)==LKS_STATUS_OK);
    size_t first=0;
    for(size_t i=1;i<=10000;++i) {
        CHECK(lks_order_insert_before(order,b,&payload,&h)==LKS_STATUS_OK);
        CHECK(lks_order_compare(order,a,h,&cmp)==LKS_STATUS_OK && cmp<0);
        CHECK(lks_order_compare(order,h,b,&cmp)==LKS_STATUS_OK && cmp<0);
        if (!first && order->precedence_disabled) first=i;
    }
    CHECK(first);
    printf("insertions,first_exhaustion,blocks\n10000,%zu,%zu\n",first,order->blocks);
    lks_order_destroy(order);
    return 0;
}
