#include "layerkeysort.h"
#include <stdio.h>

typedef struct Task { int priority; const char *name; } Task;
static int compare_priority(const void *left,const void *right,void *context)
{
    const Task *a=(const Task *)left,*b=(const Task *)right;
    (void)context;
    return (a->priority>b->priority)-(a->priority<b->priority);
}
int main(void)
{
    Task tasks[]={{2,"first equal"},{1,"earlier"},{2,"second equal"}};
    Task query={2,"query"};LksComparator comparator={compare_priority,NULL};
    LksManagedOrder *order=lks_managed_order_create(&comparator);
    const LksOrderHandle *handles[3],*previous,*equal,*next,*it;
    LksOrderCursor *cursor=NULL;size_t i;void *removed=NULL;
    if(!order)return 1;
    for(i=0;i<3;++i)if(lks_managed_order_insert(order,&tasks[i],&handles[i])!=LKS_STATUS_OK)goto fail;
    if(lks_managed_order_locate(order,&query,&previous,&equal,&next)!=LKS_STATUS_OK ||
        previous!=handles[1] || equal!=handles[0] || next || lks_order_next(equal)!=handles[2])goto fail;
    /* Equal priorities retain insertion order; cursor borrows the live container. */
    if(lks_managed_order_cursor_create(order,0,&cursor)!=LKS_STATUS_OK)goto fail;
    for(i=0;i<3;++i) {
        if(lks_order_cursor_next(cursor,&it)!=LKS_STATUS_OK || !it)goto fail;
        printf("%s\n",((Task *)lks_order_item(it))->name);
    }
    lks_order_cursor_destroy(cursor);cursor=NULL;
    if(lks_managed_order_remove(order,handles[0],&removed)!=LKS_STATUS_OK || removed!=&tasks[0])goto fail;
    /* Never access handles[0] again. Application still owns every Task. */
    lks_managed_order_destroy(order);return 0;
fail:
    lks_order_cursor_destroy(cursor);lks_managed_order_destroy(order);return 1;
}
