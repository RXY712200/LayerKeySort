#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d\n",__LINE__); exit(1); } } while(0)

static void report(const LksOrder *o, const char *phase, size_t cycle)
{
    size_t zeros=0, cluster=0, longest=0, occupancy[LKS_ORDER_BLOCK_CAPACITY+1]={0};
    for(const LksOrderBlock *b=o->first;b;b=b->next){
        ++occupancy[b->count];
        if(!b->tag){++zeros;if(++cluster>longest)longest=cluster;}else cluster=0;
    }
    LksAllocStats a=lks_alloc_stats_get();
    printf("%s,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu",phase,cycle,o->count,o->blocks,
           zeros,longest,a.live_bytes,a.peak_live_bytes,a.total_successful_requested_bytes,
           o->tag_writes,o->tag_fallbacks);
    for(size_t i=0;i<=LKS_ORDER_BLOCK_CAPACITY;++i)if(occupancy[i])printf(",%zu:%zu",i,occupancy[i]);
    puts("");
}
int main(int argc,char **argv)
{
    CHECK(argc==3);
    size_t n=(size_t)strtoul(argv[1],NULL,10), scattered=(size_t)strtoul(argv[2],NULL,10);
    CHECK(n>=1000&&n<=1000000&&n%100==0&&scattered<=1);
    const LksOrderHandle **refs=calloc(n,sizeof(*refs));CHECK(refs);
    LksOrder *o=lks_order_create();int item=7;CHECK(o);
    for(size_t cycle=0;cycle<2;++cycle){
        for(size_t i=0;i<n;++i)if(!refs[i])CHECK(lks_order_insert_back(o,&item,refs+i)==LKS_STATUS_OK);
        CHECK(lks_order_internal_valid(o));
        report(o,"built",cycle);
        for(size_t i=0;i<n;++i)if(scattered?i%100!=0:i>=n/100){
            void *out=NULL;CHECK(lks_order_remove(o,refs[i],&out)==LKS_STATUS_OK&&out==&item);refs[i]=NULL;
        }
        CHECK(lks_order_size(o)==n/100&&lks_order_internal_valid(o));
        const LksOrderHandle *h=lks_order_first(o);
        for(size_t i=0;i<n;++i)if(refs[i]){CHECK(h==refs[i]);h=lks_order_next(h);}
        CHECK(!h);
        report(o,"sparse",cycle);
    }
    lks_order_destroy(o);free(refs);
    CHECK(!lks_alloc_stats_get().live_bytes);
    return 0;
}
