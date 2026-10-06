/* Three editable layers: application labels are identity, handles are live position. */
#include "layerkeysort.h"
#include <stdio.h>
int main(void)
{
    const char *labels[]={"Background","Object","Overlay"};size_t i;
    LksOrder *order=lks_order_create();const LksOrderHandle *handles[3],*it;
    if(!order)return 1;
    for(i=0;i<3;++i)if(lks_order_insert_back(order,(void *)labels[i],&handles[i])!=LKS_STATUS_OK) {
        lks_order_destroy(order);return 2;
    }
    if(lks_order_move_before(order,handles[2],handles[1])!=LKS_STATUS_OK) {
        lks_order_destroy(order);return 3;
    }
    for(it=lks_order_first(order);it;it=lks_order_next(it))puts((const char *)lks_order_item(it));
    lks_order_destroy(order);return 0;
}
