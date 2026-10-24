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
#include <windows.h>
#include <psapi.h>
static void require(int ok){if(!ok){fputs("LONG_CHURN_MISMATCH\n",stderr);exit(2);}}
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/(double)b.QuadPart;}
static size_t rss(void){PROCESS_MEMORY_COUNTERS c;require(GetProcessMemoryInfo(GetCurrentProcess(),&c,sizeof(c))!=0);return(size_t)c.WorkingSetSize;}
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
int main(int argc,char **argv)
{
    size_t n,cycles,batch=100000,i,cycle;uint64_t seed=17,*items;
    Oracle model;LksOrder *order;const LksOrderHandle **map;uint32_t *victims,*anchors;const char *mode;
    require(argc==3);n=(size_t)strtoull(argv[1],NULL,10);cycles=(size_t)strtoull(argv[2],NULL,10);
    require(n>=64&&n<=1000000&&cycles>0&&cycles<=100);if(n<batch)batch=n;
#ifdef RESEARCH_PRIVATE
    mode="structural";
#elif defined(RESEARCH_WRAP)
    mode="allocation";
#else
    mode="production";
#endif
    items=malloc(n*sizeof(*items));map=calloc(n,sizeof(*map));victims=malloc(batch*sizeof(*victims));anchors=malloc(batch*sizeof(*anchors));
    require(items!=NULL&&map!=NULL&&victims!=NULL&&anchors!=NULL);oracle_init(&model,n);
    research_scope(1);order=lks_order_create();require(order!=NULL);
    for(i=0;i<n;i++){items[i]=i;require(lks_order_insert_back(order,&items[i],&map[i])==LKS_STATUS_OK);}
    research_scope(0);
    for(i=0;i<n;i++)oracle_insert(&model,(uint32_t)i,ABSENT);
    verify(&model,order,map,items);
    puts("mode,scale,cycle,pairs,wall_seconds,allocations,frees,live_allocations,live_requested_bytes,peak_requested_bytes,traffic_bytes,working_set_bytes,blocks,height");
    for(cycle=0;cycle<cycles;cycle++){
        ResearchMemory before,after;ResearchWork work;double start,elapsed;
        MEMORYSTATUSEX mem;mem.dwLength=sizeof(mem);require(GlobalMemoryStatusEx(&mem)!=0);
        if(mem.ullAvailPhys<UINT64_C(2147483648)){fputs("SAFE_BUDGET_ABORT\n",stderr);return 77;}
        for(i=0;i<batch;i++){
            uint32_t victim=(uint32_t)(research_random(&seed)%n),anchor=(uint32_t)(research_random(&seed)%n);
            if(victim==anchor)anchor=model.next[victim];
            victims[i]=victim;anchors[i]=anchor;oracle_remove(&model,victim);oracle_insert(&model,victim,anchor);
        }
        before=research_memory();research_work_reset();start=now();research_scope(1);
        for(i=0;i<batch;i++){
            uint32_t id=victims[i],anchor=anchors[i];
            require(lks_order_remove(order,map[id],NULL)==LKS_STATUS_OK);map[id]=NULL;research_after(order,0);
            require((anchor==ABSENT?lks_order_insert_back(order,&items[id],&map[id]):lks_order_insert_before(order,map[anchor],&items[id],&map[id]))==LKS_STATUS_OK);research_after(order,0);
        }
        research_scope(0);elapsed=now()-start;after=research_memory();work=research_work(order,0);
        printf("%s,%zu,%zu,%zu,%.9f",mode,n,cycle+1,batch,elapsed);
        if(strcmp(mode,"production"))printf(",%zu,%zu,%zu,%zu,%zu,%zu",after.allocations-before.allocations,after.frees-before.frees,after.live,after.bytes,after.peak,after.traffic-before.traffic);
        else printf(",NA,NA,NA,NA,NA,NA");
        printf(",%zu",rss());
        if(!strcmp(mode,"structural"))printf(",%zu,%zu\n",work.blocks,work.height);else puts(",NA,NA");
        if(cycle%10==0||cycle+1==cycles)verify(&model,order,map,items);
    }
    research_scope(1);lks_order_destroy(order);research_scope(0);require(research_memory().live==0&&research_memory().bytes==0);
    fprintf(stderr,"LONG_COMPLETE mode=%s n=%zu pairs=%zu tracked_cleanup=%s\n",mode,n,cycles*batch,strcmp(mode,"production")?"zero":"unavailable");
    free(items);free(map);free(victims);free(anchors);oracle_destroy(&model);return 0;
}
