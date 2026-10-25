/* Production target: public API only. The independent ID model is advanced
 * before each phase; timed replay never mutates or reads oracle links. Private
 * structural observation is compiled only in a separately labelled executable. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <layerkeysort.h>
#include "oracle.h"
#include "probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <time.h>
#include <unistd.h>
#endif
typedef struct Item {uint64_t id,key;} Item;
typedef struct Operation {uint32_t kind,id,anchor;} Operation;
typedef struct Fixture {
    size_t n,operations;Item *items;const LksOrderHandle **handles;
    Oracle expected;LksOrder *order;LksManagedOrder *managed;
    Operation *trace;uint64_t seed;unsigned char association[8];
} Fixture;
static uint64_t comparisons;
static double process_start,phase_start;
static ResearchMemory before;
static double samples[65536];static size_t sample_count;
static const char *profile,*mode;
static void require(int ok,const char *message)
{if(!ok){fprintf(stderr,"RESEARCH_FAILURE %s\n",message);exit(2);}}
static double clock_now(void)
{
#ifdef _WIN32
    LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);
    return(double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))abort();
    return(double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static size_t available(void)
{
#ifdef _WIN32
    MEMORYSTATUSEX s;s.dwLength=sizeof(s);require(GlobalMemoryStatusEx(&s)!=0,"memory status");return(size_t)s.ullAvailPhys;
#else
    return(size_t)sysconf(_SC_AVPHYS_PAGES)*(size_t)sysconf(_SC_PAGESIZE);
#endif
}
static size_t working_set(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS c;return GetProcessMemoryInfo(GetCurrentProcess(),&c,sizeof(c))?(size_t)c.WorkingSetSize:0;
#else
    FILE *f=fopen("/proc/self/statm","r");unsigned long total=0,pages=0;
    if(!f)return 0;
    if(fscanf(f,"%lu %lu",&total,&pages)!=2)pages=0;
    fclose(f);return(size_t)pages*(size_t)sysconf(_SC_PAGESIZE);
#endif
}
static void budget(size_t operation)
{
    if(operation%65536==0&&(available()<(size_t)2*1024*1024*1024||clock_now()-process_start>240)){
        fputs("SAFE_BUDGET_ABORT reserve<2GiB or process>240s\n",stderr);exit(77);
    }
}
static void phase_begin(size_t count)
{
    /* Constant across scales: changing sample density with N would confound
     * throughput trends. At the 10m limit this needs fewer than 40k samples. */
    (void)count;comparisons=0;sample_count=0;
    before=research_memory();research_work_reset();phase_start=clock_now();
}
static double sample_begin(size_t i)
{
    /* One rotated position per 256-operation stratum. A fixed zero offset
     * would sample only every-fourth missing lookup in the query workload. */
    size_t offset=(((i>>8)*73)+17)&255;
    return (i&255)==offset?clock_now():-1;
}
static void sample_end(double start)
{if(start>=0&&sample_count<65536)samples[sample_count++]=clock_now()-start;}
static int double_compare(const void *a,const void *b)
{double x=*(const double *)a,y=*(const double *)b;return(x>y)-(x<y);}
static void phase_end(Fixture *f,const char *name,size_t operations,size_t wire_bytes)
{
    double elapsed=clock_now()-phase_start;
    ResearchMemory m=research_memory();ResearchWork w=research_work(f->managed?(void *)f->managed:(void *)f->order,f->managed!=NULL);
    size_t i;char occupancy[768];size_t used=0;
    qsort(samples,sample_count,sizeof(double),double_compare);
    occupancy[0]=0;
    for(i=0;i<65;i++)if(w.occupancy[i])used+=(size_t)snprintf(occupancy+used,sizeof(occupancy)-used,"%s%zu:%zu",used?";":"",i,w.occupancy[i]);
    printf("%s,%s,%s,%zu,%zu,%zu,%.9f,%.3f,%zu,%.9f,%.9f,%.9f,%.9f",
        mode,profile,name,f->n,f->expected.count,operations,elapsed,elapsed>0?(double)operations/elapsed:0,
        sample_count,sample_count?samples[(sample_count-1)/2]:0,sample_count?samples[(sample_count-1)*95/100]:0,
        sample_count?samples[(sample_count-1)*99/100]:0,sample_count?samples[sample_count-1]:0);
    if(strcmp(mode,"production"))printf(",%zu,%zu,%zu,%zu,%zu,%zu,%zu",
        m.allocations-before.allocations,m.reallocations-before.reallocations,m.frees-before.frees,
        m.live,m.bytes,m.peak,m.traffic-before.traffic);
    else printf(",NA,NA,NA,NA,NA,NA,NA");
    printf(",%zu,%zu,%zu,%zu",working_set(),available(),wire_bytes,(size_t)comparisons);
    if(!strcmp(mode,"structural"))printf(",%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%s\n",
        w.assignments,w.max_assignments,w.index_writes,w.max_index,w.splits,w.merges,
        w.redistributions,w.rotations,w.blocks,w.height,occupancy);
    else puts(",NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA");
    fflush(stdout);
}
static void verify(Fixture *f)
{
    uint32_t id=f->expected.first;size_t count=0;double start=clock_now();uint64_t digest=UINT64_C(1469598103934665603);
    const LksOrderHandle *h=f->managed?lks_managed_order_first(f->managed):lks_order_first(f->order);
    require((f->managed?lks_managed_order_size(f->managed):lks_order_size(f->order))==f->expected.count,"count");
    while(id!=ABSENT){
        require(id<f->n&&count<f->expected.count,"oracle membership/cycle");
        require(h==f->handles[id]&&lks_order_item(h)==&f->items[id],"full forward identity");
        digest=(digest^id)*UINT64_C(1099511628211);
        if(count>0&&count%(f->expected.count/64+1)==0){
            const LksOrderHandle *first=f->handles[f->expected.first];int result=0;
            require((f->managed?lks_managed_order_compare(f->managed,first,h,&result):
                lks_order_compare(f->order,first,h,&result))==LKS_STATUS_OK&&result<0,"distributed index comparisons");
        }
        h=lks_order_next(h);id=f->expected.next[id];++count;
    }
    require(!h&&count==f->expected.count,"forward endpoint");
    id=f->expected.last;h=f->managed?lks_managed_order_last(f->managed):lks_order_last(f->order);count=0;
    while(id!=ABSENT){require(h==f->handles[id]&&count<f->expected.count,"reverse identity");h=lks_order_previous(h);id=f->expected.previous[id];++count;}
    require(!h&&count==f->expected.count,"reverse endpoint");
    /* Exact traversal proves all resident mappings; sampled comparisons check
     * index-based ordering independently of the traversal interfaces. */
    id=f->expected.first;
    for(count=0;id!=ABSENT&&count<64;count++){
        uint32_t next=f->expected.next[id];int result=99;
        if(next==ABSENT)break;
        require((f->managed?lks_managed_order_compare(f->managed,f->handles[id],f->handles[next],&result):
            lks_order_compare(f->order,f->handles[id],f->handles[next],&result))==LKS_STATUS_OK&&result<0,"sampled comparisons");
        id=next;
    }
    fprintf(stderr,"CHECKPOINT %s count=%zu exact_forward_reverse_identity=true digest=%016" PRIx64 " oracle_seconds=%.6f\n",profile,f->expected.count,digest,clock_now()-start);
}
static int item_compare(const void *a,const void *b,void *context)
{const Item *x=a,*y=b;(void)context;++comparisons;return(x->key>y->key)-(x->key<y->key);}
static Item *sort_items;
static int id_compare(const void *a,const void *b)
{
    uint32_t x=*(const uint32_t *)a,y=*(const uint32_t *)b;
    uint64_t kx=sort_items[x].key,ky=sort_items[y].key;
    return kx==ky?(x>y)-(x<y):(kx>ky)-(kx<ky);
}
static uint32_t insertion_anchor(size_t i,uint64_t *random)
{
    if(!i||!strcmp(profile,"append")||!strcmp(profile,"history"))return ABSENT;
    if(!strcmp(profile,"prepend"))return(uint32_t)(i-1);
    if(!strcmp(profile,"anchor"))return 0;
    return(uint32_t)(i-1-(research_random(random)%(i<32?i:32)));
}
static void build(Fixture *f,int managed)
{
    size_t i;uint64_t random=f->seed;double expected_start=clock_now();
    uint32_t *sorted=NULL;
    if(managed){
        sorted=malloc(f->n*sizeof(*sorted));require(sorted!=NULL,"expected sort storage");
        for(i=0;i<f->n;i++)sorted[i]=(uint32_t)i;
        sort_items=f->items;qsort(sorted,f->n,sizeof(*sorted),id_compare);
        for(i=0;i<f->n;i++)oracle_insert(&f->expected,sorted[i],ABSENT);
    }else for(i=0;i<f->n;i++)oracle_insert(&f->expected,(uint32_t)i,insertion_anchor(i,&random));
    fprintf(stderr,"ORACLE_BUILD seconds=%.6f bytes=%zu\n",clock_now()-expected_start,8*f->n+(managed?4*f->n:0));
    free(sorted);phase_begin(f->n);research_scope(1);
    if(managed){LksComparator cmp={item_compare,NULL};f->managed=lks_managed_order_create(&cmp);require(f->managed!=NULL,"managed create");}
    else {f->order=lks_order_create();require(f->order!=NULL,"order create");}
    random=f->seed;
    for(i=0;i<f->n;i++){
        LksStatus status;uint32_t anchor=managed?ABSENT:insertion_anchor(i,&random);double start=sample_begin(i);
        if(managed)status=lks_managed_order_insert(f->managed,&f->items[i],&f->handles[i]);
        else status=anchor==ABSENT?lks_order_insert_back(f->order,&f->items[i],&f->handles[i]):lks_order_insert_before(f->order,f->handles[anchor],&f->items[i],&f->handles[i]);
        sample_end(start);require(status==LKS_STATUS_OK,"insert");
        research_after(managed?(void *)f->managed:(void *)f->order,managed);budget(i);
    }
    research_scope(0);phase_end(f,"growth",f->n,0);verify(f);
}
static void prepare_trace(Fixture *f,int kind)
{
    size_t i;uint64_t random=f->seed+(uint64_t)kind;double start=clock_now();
    for(i=0;i<f->operations;i++){
        uint32_t id=(uint32_t)(research_random(&random)%f->n),anchor;
        if(kind==0){anchor=f->expected.next[id];if(anchor!=ABSENT)anchor=f->expected.next[anchor];}
        else if(kind==1)anchor=(uint32_t)(f->n/2);
        else if(kind==2)anchor=i%2?ABSENT:f->expected.first;
        else anchor=(uint32_t)(research_random(&random)%f->n);
        f->trace[i]=(Operation){(uint32_t)(kind>=4?1:0),id,anchor};
        if(kind>=4){
            if(kind==5){id=f->expected.first;anchor=(uint32_t)(f->n/2);f->trace[i].id=id;f->trace[i].anchor=anchor;}
            if(id==anchor)f->trace[i].anchor=anchor=f->expected.next[id];
            oracle_remove(&f->expected,id);oracle_insert(&f->expected,id,anchor);
        }else oracle_move(&f->expected,id,anchor);
    }
    fprintf(stderr,"ORACLE_TRACE kind=%d operations=%zu seconds=%.6f\n",kind,f->operations,clock_now()-start);
}
static void replay(Fixture *f,const char *name,int kind)
{
    size_t i;prepare_trace(f,kind);phase_begin(f->operations*(kind>=4?2:1));research_scope(1);
    for(i=0;i<f->operations;i++){
        Operation t=f->trace[i];LksStatus status;double start=sample_begin(i);
        if(t.kind){
            status=lks_order_remove(f->order,f->handles[t.id],NULL);require(status==LKS_STATUS_OK,"churn remove");
            research_after(f->order,0);f->handles[t.id]=NULL;
            status=t.anchor==ABSENT?lks_order_insert_back(f->order,&f->items[t.id],&f->handles[t.id]):
                lks_order_insert_before(f->order,f->handles[t.anchor],&f->items[t.id],&f->handles[t.id]);
        }else status=t.anchor==ABSENT?lks_order_move_back(f->order,f->handles[t.id]):lks_order_move_before(f->order,f->handles[t.id],f->handles[t.anchor]);
        sample_end(start);require(status==LKS_STATUS_OK,"trace replay");research_after(f->order,0);budget(i);
    }
    research_scope(0);phase_end(f,name,f->operations*(kind>=4?2:1),0);verify(f);
}
static void shrink_regrow(Fixture *f)
{
    size_t i;
    for(i=0;i<f->n;i+=2)oracle_remove(&f->expected,(uint32_t)i);
    phase_begin((f->n+1)/2);research_scope(1);
    for(i=0;i<f->n;i+=2){double start=sample_begin(i/2);require(lks_order_remove(f->order,f->handles[i],NULL)==LKS_STATUS_OK,"shrink");sample_end(start);f->handles[i]=NULL;research_after(f->order,0);budget(i);}
    research_scope(0);phase_end(f,"half_delete",(f->n+1)/2,0);verify(f);
    for(i=0;i<f->n;i+=2)oracle_insert(&f->expected,(uint32_t)i,ABSENT);
    phase_begin((f->n+1)/2);research_scope(1);
    for(i=0;i<f->n;i+=2){double start=sample_begin(i/2);require(lks_order_insert_back(f->order,&f->items[i],&f->handles[i])==LKS_STATUS_OK,"regrow");sample_end(start);research_after(f->order,0);budget(i);}
    research_scope(0);phase_end(f,"regrow",(f->n+1)/2,0);verify(f);
}
static LksStatus association(void *item,uint64_t row,void *ctx,const void **data,size_t *size)
{
    Fixture *f=ctx;uint64_t id=((Item *)item)->id;size_t i;(void)row;
    for(i=8;i>0;i--){f->association[i-1]=(unsigned char)(id&255);id>>=8;}
    *data=f->association;*size=8;return LKS_STATUS_OK;
}
static LksStatus resolver(const void *data,size_t size,uint64_t row,void *ctx,void **out)
{
    Fixture *f=ctx;const unsigned char *p=data;uint64_t id=0;size_t i;(void)row;
    if(size!=8)return LKS_STATUS_INVALID_ARGUMENT;
    for(i=0;i<8;i++)id=id*256+p[i];
    if(id>=f->n)return LKS_STATUS_NOT_FOUND;
    *out=&f->items[(size_t)id];return LKS_STATUS_OK;
}
static void history(Fixture *f)
{
    LksSnapshotOptions opt={"full-scale-study",16,association,f};LksSnapshot *s=NULL,*loaded=NULL,*retained=NULL;
    LksOrder *restored=NULL;unsigned char *wire;size_t bytes;uint32_t id;const LksOrderHandle *h;
    phase_begin(1);research_scope(1);require(lks_order_snapshot_capture(f->order,&opt,&s)==LKS_STATUS_OK,"capture");research_scope(0);phase_end(f,"capture",1,0);
    phase_begin(1);research_scope(1);require(lks_order_snapshot_capture(f->order,&opt,&retained)==LKS_STATUS_OK,"second history");research_scope(0);phase_end(f,"retained_two",1,0);
    bytes=lks_snapshot_serialized_size(s);wire=malloc(bytes);require(wire!=NULL,"wire buffer");
    phase_begin(1);require(lks_snapshot_serialize(s,wire,bytes)==LKS_STATUS_OK,"serialize");phase_end(f,"serialize",1,bytes);
    phase_begin(1);research_scope(1);require(lks_snapshot_deserialize(wire,bytes,&loaded)==LKS_STATUS_OK,"deserialize");research_scope(0);phase_end(f,"deserialize",1,bytes);
    phase_begin(1);research_scope(1);require(lks_snapshot_restore_order(loaded,resolver,f,&restored)==LKS_STATUS_OK,"restore");research_scope(0);phase_end(f,"restore",1,bytes);
    h=lks_order_first(restored);id=f->expected.first;
    while(id!=ABSENT){require(h!=NULL&&lks_order_item(h)==&f->items[id],"restored full expected sequence");h=lks_order_next(h);id=f->expected.next[id];}
    require(h==NULL&&lks_order_size(restored)==f->n,"restored count/end");
    phase_begin(1);research_scope(1);lks_order_destroy(restored);research_scope(0);phase_end(f,"restored_destroy",1,0);
    phase_begin(3);research_scope(1);lks_snapshot_destroy(s);lks_snapshot_destroy(loaded);lks_snapshot_destroy(retained);research_scope(0);phase_end(f,"history_destroy",3,0);free(wire);
}
static void locate(Fixture *f)
{
    size_t i;uint32_t *ids=malloc(f->n*sizeof(*ids)),id=f->expected.first;
    Item queries[4096];uint32_t expect[4096][3];uint64_t random=f->seed;
    require(ids!=NULL,"lookup expected storage");
    for(i=0;i<f->n;i++){ids[i]=id;id=f->expected.next[id];}
    for(i=0;i<4096;i++){
        size_t lo=0,hi=f->n,lower,upper;queries[i]=f->items[research_random(&random)%f->n];
        if(i%4==0)queries[i].key=UINT64_MAX;
        while(lo<hi){size_t mid=lo+(hi-lo)/2;if(f->items[ids[mid]].key<queries[i].key)lo=mid+1;else hi=mid;}lower=lo;hi=f->n;
        while(lo<hi){size_t mid=lo+(hi-lo)/2;if(f->items[ids[mid]].key<=queries[i].key)lo=mid+1;else hi=mid;}upper=lo;
        expect[i][0]=lower?ids[lower-1]:ABSENT;
        expect[i][1]=lower<f->n&&f->items[ids[lower]].key==queries[i].key?ids[lower]:ABSENT;
        expect[i][2]=upper<f->n?ids[upper]:ABSENT;
    }
    phase_begin(f->operations);research_scope(1);
    for(i=0;i<f->operations;i++){
        const LksOrderHandle *p=NULL,*e=NULL,*s=NULL;size_t q=i%4096;double start=sample_begin(i);
        require(lks_managed_order_locate(f->managed,&queries[q],&p,&e,&s)==LKS_STATUS_OK,"locate");sample_end(start);
        require(p==(expect[q][0]==ABSENT?NULL:f->handles[expect[q][0]])&&e==(expect[q][1]==ABSENT?NULL:f->handles[expect[q][1]])&&s==(expect[q][2]==ABSENT?NULL:f->handles[expect[q][2]]),"independent lookup bounds");budget(i);
    }
    research_scope(0);phase_end(f,"locate",f->operations,0);free(ids);
}
int main(int argc,char **argv)
{
    Fixture f={0};size_t i;uint64_t random;char *end;unsigned long long n;int managed;
    require(argc==4,"usage: profile residents seed");profile=argv[1];errno=0;n=strtoull(argv[2],&end,10);
    require(!errno&&!*end&&n>=64&&n<=10000000,"resident range 64..10m");f.n=(size_t)n;
    f.seed=strtoull(argv[3],&end,10);require(!*end,"seed");random=f.seed;process_start=clock_now();
#ifdef RESEARCH_PRIVATE
    mode="structural";
#elif defined(RESEARCH_WRAP)
    mode="allocation";
#else
    mode="production";
#endif
    managed=!strcmp(profile,"ascending")||!strcmp(profile,"descending")||!strcmp(profile,"random")||!strcmp(profile,"duplicates")||!strcmp(profile,"equal");
    require(managed||!strcmp(profile,"append")||!strcmp(profile,"prepend")||!strcmp(profile,"anchor")||!strcmp(profile,"local")||!strcmp(profile,"history"),"profile");
    require(strcmp(profile,"history")||f.n<=1000000,"history capped at 1m for memory safety");budget(0);
    f.operations=f.n<200000?f.n:200000;
    f.items=malloc(f.n*sizeof(*f.items));f.handles=calloc(f.n,sizeof(*f.handles));f.trace=malloc(f.operations*sizeof(*f.trace));
    require(f.items!=NULL&&f.handles!=NULL&&f.trace!=NULL,"application allocations");oracle_init(&f.expected,f.n);
    for(i=0;i<f.n;i++){
        uint64_t key;
        if(!strcmp(profile,"descending"))key=f.n-i;
        else if(!strcmp(profile,"random"))key=research_random(&random);
        else if(!strcmp(profile,"duplicates"))key=research_random(&random)%64;
        else if(!strcmp(profile,"equal"))key=1;
        else key=i;
        f.items[i]=(Item){i,key};
    }
    fprintf(stderr,"ENV mode=%s profile=%s n=%zu seed=%" PRIu64 " app_fixture_bytes=%zu available_before=%zu\n",mode,profile,f.n,f.seed,32*f.n+sizeof(Operation)*f.operations,available());
    puts("mode,profile,phase,scale,residents,operations,wall_seconds,operations_per_second,latency_samples,p50_seconds,p95_seconds,p99_seconds,max_sample_seconds,allocations,reallocations,frees,live_allocations,live_requested_bytes,peak_requested_bytes,requested_traffic_bytes,working_set_bytes,available_physical_bytes,wire_bytes,comparisons,location_assignments,max_operation_assignments,index_writes,max_operation_index_blocks,splits,merges,redistributions,rotations,blocks,height,occupancy_histogram");
    build(&f,managed);
    if(managed)locate(&f);
    else if(!strcmp(profile,"history"))history(&f);
    else{
        replay(&f,"local_move",0);replay(&f,"hotspot_move",1);replay(&f,"front_back_move",2);replay(&f,"uniform_move",3);
        replay(&f,"uniform_churn_1",4);replay(&f,"hotspot_churn",5);replay(&f,"uniform_churn_2",4);replay(&f,"uniform_churn_3",4);
        shrink_regrow(&f);
    }
    phase_begin(1);research_scope(1);if(f.managed)lks_managed_order_destroy(f.managed);else lks_order_destroy(f.order);f.managed=NULL;f.order=NULL;research_scope(0);phase_end(&f,"library_destroy",1,0);
    require(research_memory().live==0&&research_memory().bytes==0,"tracked cleanup");
    free(f.trace);free(f.handles);free(f.items);oracle_destroy(&f.expected);
    fprintf(stderr,"COMPLETE mode=%s profile=%s n=%zu cleanup_requested_zero=%s seconds=%.6f working_set_after_fixture_free=%zu\n",mode,profile,f.n,strcmp(mode,"production")?"true":"unavailable",clock_now()-process_start,working_set());
    return 0;
}
