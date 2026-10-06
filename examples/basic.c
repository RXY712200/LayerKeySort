/* Minimal V4 relative ordering. Items are borrowed; handles expire on removal. */
#include "layerkeysort.h"
#include <stdio.h>
int main(void)
{
    int a=1,b=2; void *removed=NULL;
    LksOrder *order=lks_order_create(); const LksOrderHandle *first=NULL,*second=NULL;
    if(!order) return 1;
    if(lks_order_insert_back(order,&a,&first)!=LKS_STATUS_OK ||
       lks_order_insert_after(order,first,&b,&second)!=LKS_STATUS_OK) {
        lks_order_destroy(order); return 2;
    }
    printf("First item: %d\n",*(int *)lks_order_item(lks_order_first(order)));
    if(lks_order_remove(order,second,&removed)!=LKS_STATUS_OK || removed!=&b) {
        lks_order_destroy(order); return 3;
    }
    /* second is expired; first remains valid until source destruction. */
    lks_order_destroy(order); return 0;
}
