#include "layerkeysort.h"
#include "lks_immutable_group_internal.h"
#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"immutable group line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef struct IgItem { size_t key, id; } IgItem;
static size_t calls;
static const LksGroup *busy_group;
static const LksGroupBatch *busy_batch;
static LksSnapshotOptions options;
static int compare(const void *a,const void *b,void *ctx)
{
    const IgItem *x=(const IgItem *)a,*y=(const IgItem *)b; (void)ctx; ++calls;
    if (busy_group) {
        LksSnapshot *s=NULL;
        CHECK(!lks_group_size(busy_group) && !lks_group_item_at(busy_group,0));
        CHECK(lks_group_snapshot_capture(busy_group,&options,&s)==LKS_STATUS_REENTRANT);
        lks_group_destroy((LksGroup *)busy_group);
    }
    if (busy_batch) {
        LksGroup *g=NULL; LksComparator cmp={compare,NULL};
        CHECK(lks_group_batch_merge_all(busy_batch,&cmp,&g)==LKS_STATUS_REENTRANT && !g);
        lks_group_batch_destroy((LksGroupBatch *)busy_batch);
    }
    return (x->key>y->key)-(x->key<y->key);
}
static int oracle(const void *a,const void *b)
{
    const IgItem *x=*(IgItem *const *)a,*y=*(IgItem *const *)b;
    return x->key!=y->key?(x->key>y->key)-(x->key<y->key):(x->id>y->id)-(x->id<y->id);
}
static LksStatus association(void *item,uint64_t ordinal,void *context,const void **data,size_t *size)
{
    (void)ordinal; (void)context; *data=&((IgItem *)item)->id; *size=sizeof(size_t);
    if(busy_group) {
        LksSnapshot *s=NULL;
        CHECK(lks_group_snapshot_capture(busy_group,&options,&s)==LKS_STATUS_REENTRANT && !s);
        lks_group_destroy((LksGroup *)busy_group);
        if(busy_batch) lks_group_batch_destroy((LksGroupBatch *)busy_batch);
    }
    return LKS_STATUS_OK;
}
static void verify(const LksGroup *g,void **expected,size_t n)
{
    size_t i; CHECK(lks_group_size(g)==n);
    for(i=0;i<n;++i) CHECK(lks_group_item_at(g,i)==expected[i]);
    CHECK(!lks_group_item_at(g,n));
}
static void campaign(size_t n,size_t chunk,int equal)
{
    IgItem *items=(IgItem *)malloc((n+1)*sizeof(*items));
    void **input=(void **)malloc((n+1)*sizeof(*input)),**expected=(void **)malloc((n+1)*sizeof(*expected));
    LksComparator cmp={compare,NULL}; LksGroup *a=NULL,*b=NULL,*g=NULL,*merged=NULL;
    LksGroupBatch *batch=NULL; LksSnapshot *s=NULL,*live=NULL,*loaded=NULL;
    LksOrder *order=NULL; const LksOrderHandle *handle; size_t i,attempt;
    unsigned char ns[]={0,255}; options.namespace_data=ns; options.namespace_size=2; options.association=association; options.context=NULL;
    CHECK(items && input && expected);
    for(i=0;i<n;++i) { items[i].id=i; items[i].key=equal?0:(i*7919)%23; input[i]=expected[i]=&items[i]; }
    qsort(expected,n,sizeof(*expected),oracle);
    for(attempt=1;;++attempt) {
        size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(attempt); status=lks_group_build(input,n,&cmp,&g); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !g && lks_alloc_stats_get().live_bytes==bytes);
    }
    verify(g,expected,n); for(i=0;i<n;++i) CHECK(input[i]==&items[i]);
    CHECK(lks_group_build(input,n/2,&cmp,&a)==LKS_STATUS_OK);
    CHECK(lks_group_build(input+n/2,n-n/2,&cmp,&b)==LKS_STATUS_OK);
    for(attempt=1;;++attempt) {
        size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(attempt); status=lks_group_merge(a,b,&cmp,&merged); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !merged && lks_alloc_stats_get().live_bytes==bytes);
    }
    verify(merged,expected,n); lks_group_destroy(merged); merged=NULL;
    busy_group=a; CHECK(lks_group_merge(a,b,&cmp,&merged)==LKS_STATUS_OK); busy_group=NULL;
    verify(merged,expected,n); lks_group_destroy(merged); merged=NULL;
    for(attempt=1;;++attempt) {
        size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(attempt); status=lks_group_batch_build(input,n,chunk,&cmp,&batch); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !batch && lks_alloc_stats_get().live_bytes==bytes);
    }
    CHECK(lks_group_batch_size(batch)==n && lks_group_batch_group_size(batch)==chunk);
    CHECK(lks_group_batch_group_count(batch)==n/chunk+(n%chunk!=0));
    for(attempt=1;;++attempt) {
        size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(attempt); status=lks_group_batch_merge_all(batch,&cmp,&merged); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !merged && lks_alloc_stats_get().live_bytes==bytes);
    }
    verify(merged,expected,n); lks_group_destroy(merged); merged=NULL;
    busy_batch=batch; CHECK(lks_group_batch_merge_all(batch,&cmp,&merged)==LKS_STATUS_OK); busy_batch=NULL;
    verify(merged,expected,n);
    for(attempt=1;;++attempt) {
        size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(attempt); status=lks_group_snapshot_capture(g,&options,&s); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !s && !g->marker && lks_alloc_stats_get().live_bytes==bytes);
    }
    CHECK(lks_group_snapshot_is_current(g,s));
    {
        LksSnapshot *overflow=NULL;
#ifdef _MSC_VER
        lks_source_marker_test_refs(g->marker,LONG_MAX);
#else
        lks_source_marker_test_refs(g->marker,SIZE_MAX);
#endif
        CHECK(lks_group_snapshot_capture(g,&options,&overflow)==LKS_STATUS_CAPACITY_LIMIT && !overflow);
        lks_source_marker_test_refs(g->marker,2);
    }
    { LksSnapshot *later=NULL;
      for(attempt=1;;++attempt) {
          size_t bytes=lks_alloc_stats_get().live_bytes; LksStatus status;
          lks_alloc_test_fail_on_attempt(attempt); status=lks_group_snapshot_capture(g,&options,&later); lks_alloc_test_disable_failure();
          if(status==LKS_STATUS_OK) break;
          CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !later && lks_alloc_stats_get().live_bytes==bytes);
      }
      CHECK(later->marker==s->marker); lks_snapshot_destroy(later);
    }
    order=lks_order_create(); CHECK(order);
    for(i=0;i<n;++i) CHECK(lks_order_insert_back(order,expected[i],&handle)==LKS_STATUS_OK);
    calls=0; busy_group=g;
    CHECK(lks_group_snapshot_capture(g,&options,&live)==LKS_STATUS_OK && calls==0);
    busy_group=NULL; lks_snapshot_destroy(live); live=NULL;
    CHECK(lks_order_snapshot_capture(order,&options,&live)==LKS_STATUS_OK);
    { size_t size=lks_snapshot_serialized_size(s); unsigned char *x=(unsigned char *)malloc(size),*y=(unsigned char *)malloc(size);
      CHECK(x && y && lks_snapshot_serialize(s,x,size)==LKS_STATUS_OK && lks_snapshot_serialize(live,y,size)==LKS_STATUS_OK && !memcmp(x,y,size));
      CHECK(lks_snapshot_deserialize(x,size,&loaded)==LKS_STATUS_OK && !lks_group_snapshot_is_current(g,loaded));
      free(x);free(y);
    }
    if(n) {
        const LksGroup *chunk_group=lks_group_batch_group_at(batch,0); LksSnapshot *cs=NULL;
        busy_group=chunk_group; busy_batch=batch;
        CHECK(lks_group_snapshot_capture(chunk_group,&options,&cs)==LKS_STATUS_OK);
        busy_group=NULL; busy_batch=NULL; lks_snapshot_destroy(cs);
    }
    lks_order_destroy(order); lks_group_destroy(a); lks_group_destroy(b);
    lks_group_destroy(g); lks_group_destroy(merged); lks_group_batch_destroy(batch);
    free(input); free(expected); free(items);
    for(i=0;i<n;++i) { const void *data;size_t size; CHECK(lks_snapshot_association(s,i,&data,&size)==LKS_STATUS_OK && size==sizeof(size_t)); }
    lks_snapshot_destroy(s); lks_snapshot_destroy(live); lks_snapshot_destroy(loaded);
    CHECK(!lks_alloc_stats_get().live_bytes && !lks_alloc_stats_get().live_blocks);
}
int main(void)
{
    LksGroup *out=(LksGroup *)(uintptr_t)1;
    LksGroupBatch *batch=(LksGroupBatch *)(uintptr_t)1;
    LksComparator cmp={compare,NULL};void *bad[]={NULL};
    CHECK(lks_group_build(bad,1,&cmp,&out)==LKS_STATUS_INVALID_ARGUMENT && !out);
    CHECK(lks_group_build(NULL,0,NULL,&out)==LKS_STATUS_INVALID_ARGUMENT && !out);
    CHECK(lks_group_batch_build(NULL,0,0,&cmp,&batch)==LKS_STATUS_INVALID_ARGUMENT && !batch);
    CHECK(lks_group_merge(NULL,NULL,&cmp,&out)==LKS_STATUS_INVALID_ARGUMENT && !out);
    CHECK(!lks_group_size(NULL) && !lks_group_item_at(NULL,0));
    campaign(0,17,0); campaign(1,1,0); campaign(193,17,0); campaign(257,64,1); campaign(1024,127,0);
    puts("V4 immutable Group/Batch stability, callbacks, snapshots and OOM PASS");return 0;
}
