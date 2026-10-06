/* Requested-byte decomposition, separate from process RSS/timing in v4_trace.
 * Diagnostic allocation globals are used only on this single thread. */
#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_snapshot_internal.h"
#include "lks_immutable_group_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"storage line %d\n",__LINE__);exit(1); } } while(0)
static int compare(const void *a,const void *b,void *ctx)
{ size_t x=*(const size_t *)a,y=*(const size_t *)b;(void)ctx;return (x>y)-(x<y); }
static LksStatus association(void *item,uint64_t ordinal,void *ctx,const void **data,size_t *size)
{ (void)ordinal;(void)ctx;*data=item;*size=sizeof(size_t);return LKS_STATUS_OK; }
int main(int argc,char **argv)
{
    size_t n=argc>1?(size_t)strtoull(argv[1],NULL,10):100000,i,first_ids[8],before,marker_bytes;
    size_t *items;void **input;
    LksOrder *o=NULL;LksImmutableGroup *g=NULL;LksSnapshot *historical[8]={0},*gs=NULL;
    LksComparator cmp={compare,NULL};unsigned char ns[]={4,4};LksSnapshotOptions opt={ns,2,association,NULL};
    const LksOrderHandle *h;LksAllocStats s;unsigned char *x,*y;size_t bytes;
    CHECK(n>=8 && n<SIZE_MAX/sizeof(size_t));
    items=(size_t *)malloc(n*sizeof(*items));input=(void **)malloc(n*sizeof(*input));
    CHECK(items && input && !lks_alloc_stats_reset());
    for(i=0;i<n;++i) { items[i]=i;input[i]=&items[i]; }
    CHECK(lks_order_bulk_build(input,n,&o)==LKS_STATUS_OK && lks_order_internal_valid(o));
    printf("{\"phase\":\"live\",\"n\":%zu,\"record_bytes\":%zu,\"block_index_bytes\":%zu,\"blocks\":%zu,\"container_bytes\":%zu,\"requested_bytes\":%zu}\n",
        n,n*sizeof(LksOrderHandle),o->blocks*sizeof(LksOrderBlock),o->blocks,sizeof(*o),lks_alloc_stats_get().live_bytes);
    before=lks_alloc_stats_get().live_bytes;CHECK(lks_immutable_group_build(input,n,&cmp,&g)==LKS_STATUS_OK);
    printf("{\"phase\":\"group\",\"n\":%zu,\"flat_array_bytes\":%zu,\"object_bytes\":%zu,\"resident_delta\":%zu}\n",n,n*sizeof(void *),sizeof(*g),lks_alloc_stats_get().live_bytes-before);
    before=lks_alloc_stats_get().live_bytes;CHECK(lks_order_snapshot_capture(o,&opt,&historical[0])==LKS_STATUS_OK);first_ids[0]=0;
    marker_bytes=lks_alloc_stats_get().live_bytes-before-sizeof(LksSnapshot)-n*sizeof(LksSnapshotRow)-2-historical[0]->association_capacity;
    CHECK(lks_immutable_group_snapshot_capture(g,&opt,&gs)==LKS_STATUS_OK);
    bytes=lks_snapshot_serialized_size(gs);x=(unsigned char *)malloc(bytes);y=(unsigned char *)malloc(bytes);CHECK(x && y);
    CHECK(lks_snapshot_serialize(gs,x,bytes)==LKS_STATUS_OK && lks_snapshot_serialize(historical[0],y,bytes)==LKS_STATUS_OK && !memcmp(x,y,bytes));
    printf("{\"phase\":\"snapshot\",\"n\":%zu,\"row_bytes\":%zu,\"association_bytes\":%zu,\"association_capacity\":%zu,\"marker_bytes\":%zu,\"wire_bytes\":%zu,\"ls1_bytes\":%zu,\"group_live_identical\":true}\n",
        n,n*sizeof(LksSnapshotRow),historical[0]->association_size,historical[0]->association_capacity,marker_bytes,bytes,n*lks_snapshot_key_length(gs));
    free(x);free(y);lks_snapshot_destroy(gs);lks_immutable_group_destroy(g);
    for(i=1;i<8;++i) {
        h=lks_order_first(o);CHECK(lks_order_move_back(o,h)==LKS_STATUS_OK && lks_order_internal_valid(o));
        first_ids[i]=*(size_t *)lks_order_item(lks_order_first(o));CHECK(lks_order_snapshot_capture(o,&opt,&historical[i])==LKS_STATUS_OK);
    }
    s=lks_alloc_stats_get();printf("{\"phase\":\"retained\",\"n\":%zu,\"snapshot_count\":8,\"requested_bytes\":%zu,\"peak_bytes\":%zu}\n",n,s.live_bytes,s.peak_live_bytes);
    while((h=lks_order_last(o))!=NULL)CHECK(lks_order_remove(o,h,NULL)==LKS_STATUS_OK);
    CHECK(lks_order_internal_valid(o));printf("{\"phase\":\"empty_with_history\",\"requested_bytes\":%zu,\"resident_count\":0}\n",lks_alloc_stats_get().live_bytes);
    lks_order_destroy(o);free(items);free(input);
    for(i=0;i<8;++i) { const void *data;size_t size;CHECK(lks_snapshot_association(historical[i],0,&data,&size)==LKS_STATUS_OK && size==sizeof(size_t) && !memcmp(data,&first_ids[i],size));lks_snapshot_destroy(historical[i]); }
    s=lks_alloc_stats_get();CHECK(!s.live_bytes && !s.live_blocks);
    printf("{\"phase\":\"destroyed\",\"live_bytes\":%zu,\"live_blocks\":%zu,\"oracle\":true}\n",s.live_bytes,s.live_blocks);return 0;
}
