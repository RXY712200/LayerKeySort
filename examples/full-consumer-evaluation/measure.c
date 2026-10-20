/* Public API only. No correctness oracle or file I/O inside operation brackets.
 * Per-call clock brackets add overhead; separate phase wall time includes the
 * small consumer bookkeeping. Use independent fresh processes for repetitions. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <layerkeysort.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <time.h>
#include <unistd.h>
#endif
#ifdef EVAL_ALLOCATIONS
#include "allocation.h"
#define SCOPE(x) allocation_scope(x)
#else
#define SCOPE(x) ((void)(x))
#endif
typedef struct Item {size_t id;int priority;} Item;
static double seconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0)abort();
    return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static size_t rss(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS p;if(GetProcessMemoryInfo(GetCurrentProcess(),&p,sizeof(p)))return (size_t)p.WorkingSetSize;return 0;
#else
    FILE *f=fopen("/proc/self/statm","r");unsigned long pages=0,total=0;
    if(!f)return 0;
    if(fscanf(f,"%lu %lu",&total,&pages)!=2)pages=0;
    fclose(f);
    return (size_t)pages*(size_t)sysconf(_SC_PAGESIZE);
#endif
}
static void check(int ok){if(!ok){fprintf(stderr,"measurement validation failed\n");exit(2);}}
static int priority(const void *a,const void *b,void *ctx)
{const Item *x=a,*y=b;(void)ctx;return(x->priority>y->priority)-(x->priority<y->priority);}
static LksStatus assoc(void *item,uint64_t ordinal,void *ctx,const void **data,size_t *size)
{(void)ordinal;(void)ctx;*data=&((Item *)item)->id;*size=sizeof(size_t);return LKS_STATUS_OK;}
static LksStatus resolve(const void *data,size_t size,uint64_t ordinal,void *ctx,void **out)
{size_t id;(void)ordinal;if(size!=sizeof(id))return LKS_STATUS_INVALID_ARGUMENT;memcpy(&id,data,sizeof(id));*out=(Item *)ctx+id;return LKS_STATUS_OK;}
static double samples[1000000];static size_t sample_count;static double library_sum,phase_start;
#ifdef EVAL_ALLOCATIONS
static AllocationStats previous;
#endif
static int dblcmp(const void *a,const void *b){double x=*(const double *)a,y=*(const double *)b;return(x>y)-(x<y);}
static void begin(void)
{sample_count=0;library_sum=0;phase_start=seconds();
#ifdef EVAL_ALLOCATIONS
 previous=allocation_stats();
#endif
}
static void sample(double start)
{double elapsed=seconds()-start;library_sum+=elapsed;if(sample_count<1000000)samples[sample_count++]=elapsed;}
static void row(const char *phase,size_t n,size_t operations,size_t extra)
{
    double wall=seconds()-phase_start;size_t resident=rss();
    qsort(samples,sample_count,sizeof(double),dblcmp);
    printf("%s,%zu,%zu,%.9f,%.9f,%.9f,%.9f,%zu,%zu",phase,n,operations,library_sum,wall,
        sample_count?samples[(sample_count-1)*99/100]:0,sample_count?samples[sample_count-1]:0,resident,extra);
#ifdef EVAL_ALLOCATIONS
    {AllocationStats s=allocation_stats();printf(",%zu,%zu,%zu,%zu,%zu",s.allocations-previous.allocations,s.frees-previous.frees,s.live,s.bytes,s.traffic-previous.traffic);}
#else
    printf(",NA,NA,NA,NA,NA");
#endif
    putchar('\n');
}
int main(int argc,char **argv)
{
    size_t n=argc>1?(size_t)strtoull(argv[1],NULL,10):10000,i,k,operations;Item *items;
    const LksOrderHandle **handles;LksOrder *order;LksManagedOrder *managed;LksComparator comparator={priority,NULL};
    LksSnapshot *history[16]={0},*loaded=NULL;LksSnapshotOptions opt={"measure-document",16,assoc,NULL};
    unsigned char *wire;size_t bytes;double start;const LksOrderHandle *walk;size_t count;
    check(n>=64&&n<=1000000);items=calloc(n+1,sizeof(*items));handles=calloc(n,sizeof(*handles));check(items&&handles);
    for(i=0;i<n+1;i++){items[i].id=i;items[i].priority=(int)((i*7919)%257);}
    puts("phase,residents,operations,operation_bracket_seconds,phase_seconds,p99_bracket_seconds,max_bracket_seconds,process_working_set_bytes,wire_or_history_bytes,allocations,frees,live_allocations,live_requested_bytes,requested_traffic_bytes");
    begin();SCOPE(1);order=lks_order_create();SCOPE(0);check(order != NULL);row("empty",0,1,0);
    begin();for(i=0;i<n;i++){start=seconds();SCOPE(1);check(lks_order_insert_back(order,&items[i],&handles[i])==LKS_STATUS_OK);SCOPE(0);sample(start);}row("grow_back",n,n,0);
    walk=lks_order_first(order);for(i=0;i<n;i++){check(walk==handles[i]);walk=lks_order_next(walk);}check(!walk);
    operations=n<10000?20000:100000;
    begin();for(i=0;i<operations;i++){k=i%2?n-1:0;start=seconds();SCOPE(1);check(lks_order_move_before(order,handles[k],handles[n/2])==LKS_STATUS_OK);SCOPE(0);sample(start);}row("drag_hotspot",n,operations,0);
    begin();for(i=0;i<operations;i++){k=i%n;start=seconds();SCOPE(1);check(lks_order_move_front(order,handles[k])==LKS_STATUS_OK);SCOPE(0);sample(start);}row("distant_drag",n,operations,0);
    begin();for(i=0;i<operations;i++){
        const LksOrderHandle *anchor;
        k=(i*17)%n;anchor=lks_order_next(handles[k]);
        if(!anchor)anchor=lks_order_previous(handles[k]);
        start=seconds();SCOPE(1);check(lks_order_move_after(order,handles[k],anchor)==LKS_STATUS_OK);SCOPE(0);sample(start);
    }row("local_pair_drag",n,operations,0);
    begin();for(i=0;i<operations*2;i++){
        k=i%n;start=seconds();SCOPE(1);check(lks_order_remove(order,handles[k],NULL)==LKS_STATUS_OK);SCOPE(0);sample(start);handles[k]=NULL;
        start=seconds();SCOPE(1);check(lks_order_insert_before(order,handles[(k+1)%n],&items[k],&handles[k])==LKS_STATUS_OK);SCOPE(0);sample(start);
    }row("churn",n,operations*4,0);
    for(k=0;k<3;k++){
        char label[32];size_t j;
        begin();for(j=0;j<operations;j++){
            size_t victim=j%n;
            start=seconds();SCOPE(1);check(lks_order_remove(order,handles[victim],NULL)==LKS_STATUS_OK);SCOPE(0);sample(start);handles[victim]=NULL;
            start=seconds();SCOPE(1);check(lks_order_insert_before(order,handles[(victim+1)%n],&items[victim],&handles[victim])==LKS_STATUS_OK);SCOPE(0);sample(start);
        }
        (void)snprintf(label,sizeof(label),"churn_cycle_%zu",k+1);row(label,n,operations*2,0);
    }
    walk=lks_order_first(order);count=0;while(walk){Item *item=lks_order_item(walk);check(walk==handles[item->id]);++count;walk=lks_order_next(walk);}check(count==n);
    begin();for(i=0;i<16;i++){start=seconds();SCOPE(1);check(lks_order_snapshot_capture(order,&opt,&history[i])==LKS_STATUS_OK);SCOPE(0);sample(start);}bytes=lks_snapshot_serialized_size(history[0]);row("retained_16_captures",n,16,bytes*16);
    wire=malloc(bytes);check(wire != NULL);
    begin();for(i=0;i<16;i++){start=seconds();check(lks_snapshot_serialize(history[i],wire,bytes)==LKS_STATUS_OK);sample(start);}row("serialize",n,16,bytes);
    if(argc>2){
        /* Warm-cache buffered I/O, no durability/fsync guarantee. Kept separate
         * from library serialization and timed only when caller gives a path. */
        begin();for(i=0;i<8;i++){
            FILE *file;size_t written;int closed;
            start=seconds();file=fopen(argv[2],"wb");check(file!=NULL);
            written=fwrite(wire,1,bytes,file);closed=fclose(file);check(written==bytes&&!closed);sample(start);
        }row("buffered_file_save",n,8,bytes);
        begin();for(i=0;i<8;i++){
            FILE *file;size_t read;int closed;
            start=seconds();file=fopen(argv[2],"rb");check(file!=NULL);
            read=fread(wire,1,bytes,file);closed=fclose(file);check(read==bytes&&!closed);sample(start);
        }row("buffered_file_load",n,8,bytes);check(remove(argv[2])==0);
    }
    begin();for(i=0;i<8;i++){start=seconds();SCOPE(1);check(lks_snapshot_deserialize(wire,bytes,&loaded)==LKS_STATUS_OK);SCOPE(0);sample(start);SCOPE(1);lks_snapshot_destroy(loaded);SCOPE(0);loaded=NULL;}row("deserialize",n,8,bytes);
    begin();for(i=0;i<8;i++){LksOrder *restored=NULL;start=seconds();SCOPE(1);check(lks_snapshot_restore_order(history[0],resolve,items,&restored)==LKS_STATUS_OK);SCOPE(0);sample(start);check(lks_order_size(restored)==n);SCOPE(1);lks_order_destroy(restored);SCOPE(0);}row("restore",n,8,bytes);
    free(wire);begin();for(i=0;i<16;i++){SCOPE(1);lks_snapshot_destroy(history[i]);SCOPE(0);}row("history_released",n,16,0);
    begin();SCOPE(1);lks_order_destroy(order);SCOPE(0);row("live_released",0,1,0);
    SCOPE(1);managed=lks_managed_order_create(&comparator);SCOPE(0);check(managed != NULL);
    begin();for(i=0;i<n;i++){start=seconds();SCOPE(1);check(lks_managed_order_insert(managed,&items[i],&handles[i])==LKS_STATUS_OK);SCOPE(0);sample(start);}row("managed_duplicates",n,n,0);
    walk=lks_managed_order_first(managed);count=0;{int last=-1;while(walk){Item *item=lks_order_item(walk);check(item->priority>=last);last=item->priority;++count;walk=lks_order_next(walk);}}check(count==n);
    begin();SCOPE(1);lks_managed_order_destroy(managed);SCOPE(0);row("cleanup",0,1,0);
#ifdef EVAL_ALLOCATIONS
    check(allocation_stats().live==0&&allocation_stats().bytes==0);
#endif
    free(handles);free(items);return 0;
}
