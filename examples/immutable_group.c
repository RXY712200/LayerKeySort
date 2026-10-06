/* Flat immutable ordering and a historical snapshot. Only the public header. */
#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
typedef struct Row { int priority; const char *id; } Row;
static int compare(const void *a, const void *b, void *context)
{
    const Row *x = (const Row *)a, *y = (const Row *)b;
    (void)context; return (x->priority > y->priority) - (x->priority < y->priority);
}
int main(void)
{
    Row rows[] = {{2,"base-a"},{1,"base-b"},{2,"incoming"}};
    void *base_items[] = {&rows[0],&rows[1]}, *incoming_items[] = {&rows[2]};
    LksComparator comparator = {compare,NULL};
    const unsigned char domain[] = {0x70,0x34};
    LksSnapshotOptions options = {domain,sizeof(domain),NULL,NULL};
    LksImmutableGroup *base = NULL, *incoming = NULL, *merged = NULL;
    LksSnapshot *snapshot = NULL; char key[64]; size_t i; int status = 1;
    if (lks_immutable_group_build(base_items,2,&comparator,&base) != LKS_STATUS_OK ||
        lks_immutable_group_build(incoming_items,1,&comparator,&incoming) != LKS_STATUS_OK ||
        lks_immutable_group_merge(base,incoming,&comparator,&merged) != LKS_STATUS_OK ||
        lks_immutable_group_snapshot_capture(merged,&options,&snapshot) != LKS_STATUS_OK) goto done;
    /* Equal Base occurrences precede Incoming. No application object is owned. */
    for (i=0;i<lks_immutable_group_size(merged);++i) puts(((Row *)lks_immutable_group_item_at(merged,i))->id);
    lks_immutable_group_destroy(merged); merged = NULL;
    for (i=0;i<lks_snapshot_count(snapshot);++i) {
        if (lks_snapshot_key_format(snapshot,i,key,sizeof(key)) != LKS_STATUS_OK) goto done;
        puts(key);
    }
    status = 0;
done:
    lks_snapshot_destroy(snapshot); lks_immutable_group_destroy(base);
    lks_immutable_group_destroy(incoming); lks_immutable_group_destroy(merged);
    return status;
}
