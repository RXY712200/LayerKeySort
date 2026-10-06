/* Comparable V3/V4 immutable operations. Monotonic timing; independent stable
 * oracle orders by key then original occurrence ID. Application arrays excluded
 * from requested library byte counts. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"groups line %d\n",__LINE__);exit(1); } } while(0)
typedef struct Item { size_t id;uint32_t key; } Item;
static size_t calls;
static int compare(const void *a,const void *b,void *context)
{ const Item *x=(const Item *)a,*y=(const Item *)b;(void)context;++calls;return (x->key>y->key)-(x->key<y->key); }
static int oracle(const void *a,const void *b)
{ const Item *x=*(Item *const *)a,*y=*(Item *const *)b;return x->key!=y->key?(x->key>y->key)-(x->key<y->key):(x->id>y->id)-(x->id<y->id); }
static double now(void)
{
#ifdef _WIN32
    LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;CHECK(!clock_gettime(CLOCK_MONOTONIC,&t));return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static void emit(const char *version,const char *operation,size_t n,int equal,int run,double elapsed,LksAllocStats before)
{
    LksAllocStats s=lks_alloc_stats_get();
    printf("{\"version\":\"%s\",\"operation\":\"%s\",\"n\":%zu,\"equal\":%d,\"run\":%d,\"ms\":%.6f,\"comparisons\":%zu,\"resident_bytes\":%zu,\"allocation_bytes\":%zu,\"allocations\":%zu,\"peak_requested\":%zu,\"oracle\":true}\n",
        version,operation,n,equal,run,elapsed*1000,calls,s.live_bytes-before.live_bytes,
        s.total_successful_requested_bytes-before.total_successful_requested_bytes,
        s.alloc_calls+s.realloc_calls-before.alloc_calls-before.realloc_calls,s.peak_live_bytes);
}
int main(int argc,char **argv)
{
    size_t n,i;int equal,run,v;Item *items;void **input,**expected;LksComparator cmp={compare,NULL};
    if(argc!=4)return 2;
    n=(size_t)strtoull(argv[1],NULL,10);equal=atoi(argv[2]);run=atoi(argv[3]);CHECK(n && n<SIZE_MAX/sizeof(Item));
    items=(Item *)malloc(n*sizeof(*items));input=(void **)malloc(n*sizeof(*input));expected=(void **)malloc(n*sizeof(*expected));CHECK(items && input && expected);
    for(i=0;i<n;++i) { items[i].id=i;items[i].key=equal?0:(uint32_t)(i*2654435761u);input[i]=expected[i]=&items[i]; }
    qsort(expected,n,sizeof(*expected),oracle);
    for(v=3;v<=4;++v) {
        LksGroup *a=NULL,*b=NULL,*g=NULL;LksGroupBatch *batch=NULL;
        LksImmutableGroup *ia=NULL,*ib=NULL,*ig=NULL;LksImmutableGroupBatch *ibatch=NULL;
        LksAllocStats before;double t;const char *version=v==3?"V3":"V4";
        CHECK(!lks_alloc_stats_reset());before=lks_alloc_stats_get();calls=0;t=now();
        CHECK((v==3?lks_group_build(input,n,&cmp,&g):lks_immutable_group_build(input,n,&cmp,&ig))==LKS_STATUS_OK);
        t=now()-t;for(i=0;i<n;++i) { CHECK((v==3?lks_group_item_at(g,i):lks_immutable_group_item_at(ig,i))==expected[i]); }
        emit(version,"build",n,equal,run,t,before);
        if(v==4) {
            LksSnapshot *snapshot=NULL;unsigned char ns[]={1,4};LksSnapshotOptions opt={ns,2,NULL,NULL};
            before=lks_alloc_stats_get();calls=0;t=now();CHECK(lks_immutable_group_snapshot_capture(ig,&opt,&snapshot)==LKS_STATUS_OK);t=now()-t;
            CHECK(!calls && lks_snapshot_count(snapshot)==n);emit(version,"capture",n,equal,run,t,before);lks_snapshot_destroy(snapshot);
        }
        lks_group_destroy(g);lks_immutable_group_destroy(ig);g=NULL;ig=NULL;
        if(v==3) { CHECK(lks_group_build(input,n/2,&cmp,&a)==LKS_STATUS_OK && lks_group_build(input+n/2,n-n/2,&cmp,&b)==LKS_STATUS_OK); }
        else { CHECK(lks_immutable_group_build(input,n/2,&cmp,&ia)==LKS_STATUS_OK && lks_immutable_group_build(input+n/2,n-n/2,&cmp,&ib)==LKS_STATUS_OK); }
        before=lks_alloc_stats_get();calls=0;t=now();CHECK((v==3?lks_group_merge(a,b,&cmp,&g):lks_immutable_group_merge(ia,ib,&cmp,&ig))==LKS_STATUS_OK);t=now()-t;
        for(i=0;i<n;++i) { CHECK((v==3?lks_group_item_at(g,i):lks_immutable_group_item_at(ig,i))==expected[i]); }
        emit(version,"merge",n,equal,run,t,before);
        lks_group_destroy(a);lks_group_destroy(b);lks_group_destroy(g);lks_immutable_group_destroy(ia);lks_immutable_group_destroy(ib);lks_immutable_group_destroy(ig);g=NULL;ig=NULL;
        CHECK(!lks_alloc_stats_reset());before=lks_alloc_stats_get();calls=0;t=now();
        CHECK((v==3?lks_group_batch_build(input,n,1024,&cmp,&batch):lks_immutable_group_batch_build(input,n,1024,&cmp,&ibatch))==LKS_STATUS_OK);t=now()-t;emit(version,"batch_build",n,equal,run,t,before);
        before=lks_alloc_stats_get();calls=0;t=now();CHECK((v==3?lks_group_batch_merge_all(batch,&cmp,&g):lks_immutable_group_batch_merge_all(ibatch,&cmp,&ig))==LKS_STATUS_OK);t=now()-t;
        for(i=0;i<n;++i) { CHECK((v==3?lks_group_item_at(g,i):lks_immutable_group_item_at(ig,i))==expected[i]); }
        emit(version,"batch_merge",n,equal,run,t,before);
        calls=0;t=now();lks_group_destroy(g);lks_group_batch_destroy(batch);lks_immutable_group_destroy(ig);lks_immutable_group_batch_destroy(ibatch);t=now()-t;
        CHECK(!lks_alloc_stats_get().live_bytes && !lks_alloc_stats_get().live_blocks);emit(version,"cleanup",n,equal,run,t,lks_alloc_stats_get());
    }
    free(items);free(input);free(expected);return 0;
}
