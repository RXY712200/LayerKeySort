#ifndef LKS_SNAPSHOT_CONSUMER_USAGE_H
#define LKS_SNAPSHOT_CONSUMER_USAGE_H
#include "layerkeysort.h"
#include <stdlib.h>
static int snapshot_consumer_compare(const void *a, const void *b, void *context)
{ (void)a; (void)b; (void)context; return 0; }
static LksStatus snapshot_consumer_resolve(const void *data, size_t size,
    uint64_t ordinal, void *context, void **out)
{ (void)data; (void)size; (void)ordinal; *out=context; return LKS_STATUS_OK; }
static int snapshot_consumer_usage(void)
{
    int item=7, c=1, result=1; const unsigned char ns[]={0,255};
    LksSnapshotOptions opt={ns,sizeof(ns),NULL,NULL};
    LksComparator cmp={snapshot_consumer_compare,NULL};
    LksOrder *o=lks_order_create(), *restored=NULL,*imported=NULL;
    LksManagedOrder *m=lks_managed_order_create(&cmp);
    LksSnapshot *s=NULL,*loaded=NULL,*ms=NULL;
    const LksOrderHandle *h=NULL; const void *view=NULL; size_t n=0;
    char key[64]; unsigned char *blob=NULL;
    LksV3Lk1ImportEntry entry={"LK1:1!",&item};
    if(!o || !m) goto done;
    if(lks_order_insert_back(o,&item,&h)!=LKS_STATUS_OK ||
       lks_order_snapshot_capture(o,&opt,&s)!=LKS_STATUS_OK ||
       !lks_order_snapshot_is_current(o,s) || lks_snapshot_count(s)!=1 ||
       lks_snapshot_namespace(s,&view,&n)!=LKS_STATUS_OK || n!=2 ||
       lks_snapshot_association(s,0,&view,&n)!=LKS_STATUS_OK || n ||
       lks_snapshot_key_length(s)!=25 ||
       lks_snapshot_key_format(s,0,key,sizeof(key))!=LKS_STATUS_OK ||
       lks_snapshot_key_validate(key)!=LKS_STATUS_OK ||
       lks_snapshot_key_compare(key,key,&c)!=LKS_STATUS_OK || c) goto done;
    n=lks_snapshot_serialized_size(s); blob=(unsigned char *)malloc(n);
    if(!blob || lks_snapshot_serialize(s,blob,n)!=LKS_STATUS_OK ||
       lks_snapshot_deserialize(blob,n,&loaded)!=LKS_STATUS_OK ||
       lks_snapshot_restore_order(loaded,snapshot_consumer_resolve,&item,&restored)!=LKS_STATUS_OK ||
       lks_order_import_v3_lk1(&entry,1,&imported)!=LKS_STATUS_OK ||
       lks_managed_order_insert(m,&item,&h)!=LKS_STATUS_OK ||
       lks_managed_order_snapshot_capture(m,&opt,&ms)!=LKS_STATUS_OK ||
       !lks_managed_order_snapshot_is_current(m,ms)) goto done;
    result=0;
done:
    free(blob); lks_snapshot_destroy(s); lks_snapshot_destroy(loaded); lks_snapshot_destroy(ms);
    lks_order_destroy(o); lks_order_destroy(restored); lks_order_destroy(imported);
    lks_managed_order_destroy(m); return result;
}
#endif
