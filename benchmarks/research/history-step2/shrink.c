#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
/* Focused lifetime turnover campaign, separate from the fixed large-scale
 * matrix. No changes to study.c or the released library are needed. */
#include <layerkeysort.h>
#include "oracle.h"
#include "probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "host.h"
static void require(int ok){if(!ok){fputs("LONG_CHURN_MISMATCH\n",stderr);exit(2);}}
static double now(void){return history_host_now();}
static size_t rss(void){return history_host_rss();}
static void verify(const Oracle *model,LksOrder *order,const LksOrderHandle **map,const uint64_t *items)
{
    uint32_t id=model->first;size_t count=0;uint64_t digest=UINT64_C(1469598103934665603);
    const LksOrderHandle *h=lks_order_first(order);
    require(lks_order_size(order)==model->count);
    while(id!=ABSENT){require(h==map[id]&&lks_order_item(h)==&items[id]&&count<model->count);digest=(digest^id)*UINT64_C(1099511628211);h=lks_order_next(h);id=model->next[id];++count;}
    require(!h&&count==model->count);h=lks_order_last(order);id=model->last;count=0;
    while(id!=ABSENT){require(h==map[id]&&count<model->count);h=lks_order_previous(h);id=model->previous[id];++count;}
    require(!h&&count==model->count);
    fprintf(stderr,"LONG_CHECK count=%zu digest=%016" PRIx64 " exact_forward_reverse=true\n",count,digest);
}

static void row(const char *phase, const Oracle *model, LksOrder *order, double seconds)
{
    ResearchMemory m=research_memory(); ResearchWork w=research_work(order,0);
    printf("%s,%zu,%.9f,%zu,%zu,%zu,%zu,%zu,%zu\n",phase,model->count,seconds,m.bytes,m.peak,m.allocations,m.frees,rss(),w.blocks);
}
int main(void)
{
    const size_t n=1000000;size_t i,k;uint64_t seed=17,*items;
    const LksOrderHandle **map;uint32_t *ids;Oracle model;LksOrder *order;double start;
    items=malloc(n*sizeof(*items));map=calloc(n,sizeof(*map));ids=malloc(n*sizeof(*ids));
    require(items&&map&&ids);oracle_init(&model,n);
    for(i=0;i<n;i++){items[i]=i;ids[i]=(uint32_t)i;oracle_insert(&model,(uint32_t)i,ABSENT);}
    for(i=n;i>1;--i){size_t j=research_random(&seed)%i;uint32_t t=ids[j];ids[j]=ids[i-1];ids[i-1]=t;}
    puts("phase,residents,wall_seconds,requested_bytes,peak_bytes,allocations,frees,working_set_bytes,blocks");
    research_scope(1);start=now();order=lks_order_create();require(order!=NULL);
    for(i=0;i<n;i++)require(lks_order_insert_back(order,&items[i],&map[i])==LKS_STATUS_OK);
    research_scope(0);row("growth",&model,order,now()-start);verify(&model,order,map,items);
    /* Owner derivation adds an indirection to public reads. Measure repeated
     * scans explicitly instead of assuming the smaller record always wins. */
    {
        uint64_t checksum=0; size_t pass; const LksOrderHandle *h;
        start=now();
        for(pass=0;pass<16;pass++) {
            for(h=lks_order_first(order);h;h=lks_order_next(h)) checksum+=*(const uint64_t *)lks_order_item(h);
            for(h=lks_order_last(order);h;h=lks_order_previous(h)) checksum+=*(const uint64_t *)lks_order_item(h);
        }
        row("scan32",&model,order,now()-start);
        require(checksum==UINT64_C(16)*n*(n-1));
    }
    for(k=0;k<2;k++){
        size_t first=k?n*9/10:0,last=k?n*99/100:n*9/10;
        for(i=first;i<last;i++)oracle_remove(&model,ids[i]);
        start=now();research_scope(1);
        for(i=first;i<last;i++){require(lks_order_remove(order,map[ids[i]],NULL)==LKS_STATUS_OK);map[ids[i]]=NULL;}
        research_scope(0);row(k?"shrink99":"shrink90",&model,order,now()-start);verify(&model,order,map,items);
    }
    for(i=0;i<n*99/100;i++)oracle_insert(&model,ids[i],ABSENT);
    start=now();research_scope(1);
    for(i=0;i<n*99/100;i++)require(lks_order_insert_back(order,&items[ids[i]],&map[ids[i]])==LKS_STATUS_OK);
    research_scope(0);row("regrow",&model,order,now()-start);verify(&model,order,map,items);
    start=now();research_scope(1);lks_order_destroy(order);research_scope(0);model.count=0;
    row("destroy",&model,NULL,now()-start);require(research_memory().bytes==0&&research_memory().live==0);
    free(items);free(map);free(ids);oracle_destroy(&model);fprintf(stderr,"SHRINK_COMPLETE cleanup_zero=true\n");return 0;
}
