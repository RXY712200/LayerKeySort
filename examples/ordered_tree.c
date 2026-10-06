/* V4 comparator-managed order; callback and items remain valid until destruction. */
#include "layerkeysort.h"
#include <stdio.h>
static int compare(const void *a,const void *b,void *context)
{ int x=*(const int *)a,y=*(const int *)b;(void)context;return (x>y)-(x<y); }
int main(void)
{
    int items[]={3,1,2};size_t i;LksComparator cmp={compare,NULL};
    LksManagedOrder *order=lks_managed_order_create(&cmp);const LksOrderHandle *h=NULL;
    if(!order)return 1;
    for(i=0;i<3;++i)if(lks_managed_order_insert(order,&items[i],&h)!=LKS_STATUS_OK) {
        lks_managed_order_destroy(order);return 2;
    }
    printf("Smallest: %d\n",*(int *)lks_order_item(lks_managed_order_first(order)));
    lks_managed_order_destroy(order);return 0;
}
