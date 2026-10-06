#include "lks_legacy_internal.h"
/* Deterministic application-shaped synthetic traces. Identity and the flat
 * oracle are independent of the library representation. Validation and model
 * maintenance are outside foreground timers; process/cache interference still
 * exists, so compare only identical harness conditions. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"
#include "lks_snapshot_internal.h"
#include "lks_path_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#else
#include <unistd.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"trace line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef struct Item { uint64_t id; uint32_t key; } Item;
typedef struct Metrics {
    size_t assigns, arrays, index, splits, repairs, merges, rotations, noops;
    size_t max_assigns, max_arrays, max_index, max_splits, max_repairs;
    size_t rss_start,rss_built,rss_churn,rss_small,rss_empty,rss_retained,rss_released,rss_final;
    size_t live_built,live_churn,live_small,live_empty,snapshot_peak,snapshot_rows,association_capacity;
    size_t snapshot_count,wire_bytes,key_bytes,marker_bytes;
    double mutation,build,query,capture,format,serialize,load,restore,cleanup;
} Metrics;
static size_t comparisons;
static uint32_t random_next(uint32_t *state) { *state=*state*1664525u+1013904223u;return *state; }
static int compare(const void *a,const void *b,void *context)
{ const Item *x=(const Item *)a,*y=(const Item *)b;(void)context;++comparisons;return (x->key>y->key)-(x->key<y->key); }
static double now(void)
{
#ifdef _WIN32
    LARGE_INTEGER t,f;CHECK(QueryPerformanceCounter(&t) && QueryPerformanceFrequency(&f));return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;CHECK(!clock_gettime(CLOCK_MONOTONIC,&t));return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static size_t rss(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters;
    return GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))?(size_t)counters.WorkingSetSize:0;
#elif defined(__APPLE__)
    mach_task_basic_info_data_t info;mach_msg_type_number_t count=MACH_TASK_BASIC_INFO_COUNT;
    return task_info(mach_task_self(),MACH_TASK_BASIC_INFO,(task_info_t)&info,&count)==KERN_SUCCESS?(size_t)info.resident_size:0;
#else
    FILE *f=fopen("/proc/self/statm","r");unsigned long ignored=0,resident=0;
    if(!f)return 0;
    if(fscanf(f,"%lu %lu",&ignored,&resident)!=2)resident=0;
    fclose(f);return (size_t)resident*(size_t)sysconf(_SC_PAGESIZE);
#endif
}
static size_t upper(Item **model,size_t count,uint32_t key)
{ size_t lo=0,hi=count;while(lo<hi) { size_t mid=lo+(hi-lo)/2;if(model[mid]->key<=key)lo=mid+1;else hi=mid; }return lo; }
static void model_insert(Item **model,size_t *count,size_t at,Item *item)
{ memmove(model+at+1,model+at,(*count-at)*sizeof(*model));model[at]=item;++*count; }
static Item *model_remove(Item **model,size_t *count,size_t at)
{ Item *item=model[at];--*count;memmove(model+at,model+at+1,(*count-at)*sizeof(*model));return item; }
static void verify(const LksOrder *o,Item **model,size_t count,const LksOrderHandle **handles)
{
    const LksOrderHandle *h=lks_order_first(o);size_t i;
    CHECK(lks_order_size(o)==count);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    CHECK(lks_order_internal_valid(o));
#endif
    for(i=0;i<count;++i) { CHECK(h && lks_order_item(h)==model[i] && h==handles[model[i]->id]);h=lks_order_next(h); }CHECK(!h);
    h=lks_order_last(o);for(i=count;i>0;--i) { CHECK(h==handles[model[i-1]->id]);h=lks_order_previous(h); }CHECK(!h);
}
static void record(Metrics *m,const LksOrder *o)
{
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    const LksOrderWork *w=&o->work;size_t height=o->root?(size_t)o->root->height:0;
    CHECK(!w->tracking_overflow && w->records_reassigned<=4*LKS_ORDER_BLOCK_CAPACITY &&
        w->local_blocks<=4 && w->splits<=1 && w->merges+w->redistributions<=1 && w->index_blocks<=32*(height+2));
    m->assigns+=w->records_reassigned;m->arrays+=w->local_blocks;m->index+=w->index_blocks;
    m->splits+=w->splits;m->repairs+=w->merges+w->redistributions;m->merges+=w->merges;
    m->rotations+=w->rotations;m->noops+=w->noop_moves;
    if(w->records_reassigned>m->max_assigns)m->max_assigns=w->records_reassigned;
    if(w->local_blocks>m->max_arrays)m->max_arrays=w->local_blocks;
    if(w->index_blocks>m->max_index)m->max_index=w->index_blocks;
    if(w->splits>m->max_splits)m->max_splits=w->splits;
    if(w->merges+w->redistributions>m->max_repairs)m->max_repairs=w->merges+w->redistributions;
#else
    (void)m;(void)o;
#endif
}
static LksStatus associate(void *item,uint64_t ordinal,void *context,const void **data,size_t *size)
{ (void)ordinal;(void)context;*data=&((Item *)item)->id;*size=sizeof(uint64_t);return LKS_STATUS_OK; }
typedef struct Resolve { Item *items;size_t capacity; } Resolve;
static LksStatus resolve(const void *data,size_t size,uint64_t ordinal,void *context,void **item)
{ uint64_t id;Resolve *r=(Resolve *)context;(void)ordinal;*item=NULL;if(size!=8)return LKS_STATUS_INVALID_ARGUMENT;
  memcpy(&id,data,8);if(id>=r->capacity)return LKS_STATUS_INVALID_ARGUMENT;*item=&r->items[(size_t)id];return LKS_STATUS_OK; }
static void export_one(LksOrder *o,Item **model,size_t n,Resolve *resolver,Metrics *m,LksSnapshot **retained)
{
    unsigned char ns[]={0x70,0x34};LksSnapshotOptions options={ns,2,associate,NULL};
    LksSnapshot *s=NULL,*loaded=NULL;LksOrder *restored=NULL;size_t i,bytes,live=lks_alloc_stats_get().live_bytes;
    unsigned char *blob;char key[64];double t=now();
    CHECK(lks_order_snapshot_capture(o,&options,&s)==LKS_STATUS_OK);m->capture+=now()-t;
    ++m->snapshot_count;m->snapshot_rows+=n*sizeof(LksSnapshotRow);m->association_capacity+=s->association_capacity;
    if(lks_alloc_stats_get().live_bytes-live>m->snapshot_peak)m->snapshot_peak=lks_alloc_stats_get().live_bytes-live;
    CHECK(lks_order_snapshot_is_current(o,s) && lks_snapshot_count(s)==n);
    for(i=0;i<n;++i) { const void *data;size_t size;CHECK(lks_snapshot_association(s,i,&data,&size)==LKS_STATUS_OK && size==8 && !memcmp(data,&model[i]->id,8)); }
    t=now();for(i=0;i<n;++i)CHECK(lks_snapshot_key_format(s,i,key,sizeof(key))==LKS_STATUS_OK);m->format+=now()-t;
    m->key_bytes+=n*lks_snapshot_key_length(s);bytes=lks_snapshot_serialized_size(s);m->wire_bytes+=bytes;
    blob=(unsigned char *)malloc(bytes);CHECK(blob);t=now();CHECK(lks_snapshot_serialize(s,blob,bytes)==LKS_STATUS_OK);m->serialize+=now()-t;
    /* The first export also proves load/restore exactness. Later exports retain
     * the same row oracle without inflating the application cadence cost. */
    if(m->snapshot_count==1) {
        const LksOrderHandle *h;
        t=now();CHECK(lks_snapshot_deserialize(blob,bytes,&loaded)==LKS_STATUS_OK);m->load+=now()-t;
        t=now();CHECK(lks_snapshot_restore_order(loaded,resolve,resolver,&restored)==LKS_STATUS_OK);m->restore+=now()-t;
        h=lks_order_first(restored);for(i=0;i<n;++i) { CHECK(h && lks_order_item(h)==model[i]);h=lks_order_next(h); }CHECK(!h);
    }
    free(blob);lks_snapshot_destroy(loaded);lks_order_destroy(restored);
    if(retained)*retained=s;else lks_snapshot_destroy(s);
}
static int double_compare(const void *a,const void *b)
{ double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y); }
static void print_metrics(const char *family,size_t initial,size_t steps,size_t cadence,size_t seed,int run,
    Metrics *m,double *latency,size_t samples,size_t calls,size_t peak,size_t allocs,size_t traffic,size_t frees)
{
    qsort(latency,samples,sizeof(*latency),double_compare);
    printf("{\"family\":\"%s\",\"initial\":%zu,\"steps\":%zu,\"cadence\":%zu,\"seed\":%zu,\"run\":%d,\"capacity\":%u,\"diagnostic\":%d,",
        family,initial,steps,cadence,seed,run,(unsigned)LKS_ORDER_BLOCK_CAPACITY,
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
        1
#else
        0
#endif
    );
    printf("\"mutation_ms\":%.6f,\"p50_us\":%.6f,\"p95_us\":%.6f,\"p99_us\":%.6f,\"p999_us\":%.6f,\"max_us\":%.6f,",
        m->mutation*1000,latency[samples/2]*1e6,latency[(samples-1)*95/100]*1e6,latency[(samples-1)*99/100]*1e6,latency[(samples-1)*999/1000]*1e6,latency[samples-1]*1e6);
    printf("\"query_ms\":%.6f,\"capture_ms\":%.6f,\"format_ms\":%.6f,\"serialize_ms\":%.6f,\"load_ms\":%.6f,\"restore_ms\":%.6f,\"cleanup_ms\":%.6f,",
        m->query*1000,m->capture*1000,m->format*1000,m->serialize*1000,m->load*1000,m->restore*1000,m->cleanup*1000);
    printf("\"assignments\":%zu,\"local_arrays\":%zu,\"index_writes\":%zu,\"splits\":%zu,\"source_repairs\":%zu,\"merges\":%zu,\"rotations\":%zu,\"noops\":%zu,\"max_assignments\":%zu,\"max_arrays\":%zu,\"max_index\":%zu,\"max_splits\":%zu,\"max_repairs\":%zu,\"comparisons\":%zu,",
        m->assigns,m->arrays,m->index,m->splits,m->repairs,m->merges,m->rotations,m->noops,m->max_assigns,m->max_arrays,m->max_index,m->max_splits,m->max_repairs,calls);
    printf("\"allocations\":%zu,\"allocation_bytes\":%zu,\"frees\":%zu,\"peak_requested\":%zu,\"built_requested\":%zu,\"churn_requested\":%zu,\"small_requested\":%zu,\"empty_container_requested\":%zu,\"snapshot_peak\":%zu,\"snapshot_count\":%zu,\"snapshot_row_bytes\":%zu,\"association_capacity_bytes\":%zu,\"wire_bytes\":%zu,\"key_bytes\":%zu,",
        allocs,traffic,frees,peak,m->live_built,m->live_churn,m->live_small,m->live_empty,m->snapshot_peak,m->snapshot_count,m->snapshot_rows,m->association_capacity,m->wire_bytes,m->key_bytes);
    printf("\"rss_start\":%zu,\"rss_built\":%zu,\"rss_churn\":%zu,\"rss_small\":%zu,\"rss_empty\":%zu,\"rss_retained\":%zu,\"rss_released\":%zu,\"rss_final\":%zu,\"oracle\":true,\"cleanup_live_bytes\":%zu,\"cleanup_live_blocks\":%zu}\n",
        m->rss_start,m->rss_built,m->rss_churn,m->rss_small,m->rss_empty,m->rss_retained,m->rss_released,m->rss_final,lks_alloc_stats_get().live_bytes,lks_alloc_stats_get().live_blocks);
}
static void trace(const char *family,size_t initial,size_t steps,size_t cadence,uint32_t seed,int run)
{
    size_t capacity=initial+steps+1,n=0,created=0,i,samples=0,calls;uint32_t state=seed;
    Item *items=(Item *)calloc(capacity,sizeof(*items));Item **model=(Item **)malloc(capacity*sizeof(*model));
    const LksOrderHandle **handles=(const LksOrderHandle **)calloc(capacity,sizeof(*handles));
    double *latency=(double *)malloc(capacity*sizeof(*latency)),t;Metrics metrics={0};LksAllocStats stats;
    LksComparator cmp={compare,NULL};LksManagedOrder *managed=NULL;LksOrder *o;
    Item *pending=NULL;LksSnapshot *retained[8]={0};size_t retained_count=0;Resolve resolver={items,capacity};
    int is_managed=!strcmp(family,"ascending") || !strcmp(family,"descending") || !strcmp(family,"random") || !strcmp(family,"equal") || !strcmp(family,"duplicates") || !strcmp(family,"alternating") || !strcmp(family,"priority") || !strcmp(family,"managed_churn");
    int is_move=strstr(family,"drag")!=NULL || !strcmp(family,"front_back") || !strcmp(family,"large_moves");
    CHECK(items && model && handles && latency && !lks_alloc_stats_reset());comparisons=0;metrics.rss_start=rss();
    if(is_managed) { managed=lks_managed_order_create(&cmp);CHECK(managed);o=managed->core; }else { o=lks_order_create();CHECK(o); }
    for(i=0;i<initial+steps;++i) {
        int building=i<initial;size_t phase=building?i:i-initial;unsigned operation=0;size_t at=0,from=0,to=0;Item *item;LksStatus status;uint64_t revision=o->revision;
        if(!building) {
            if(is_move)operation=2;
            else if(!strcmp(family,"churn") || !strcmp(family,"delete_reinsert") || !strcmp(family,"managed_churn"))operation=(phase%2==0 && n)?1:0;
            else if(!strcmp(family,"long_churn"))operation=(phase%(2*initial)>=initial && n)?1:0;
            else if(!strcmp(family,"timeline") || !strcmp(family,"large_timeline") || !strcmp(family,"local"))operation=n && random_next(&state)%5==0?1:0;
        }
        if(!n)operation=0;
        if(operation==0) {
            item=pending?pending:&items[created];item->id=pending?item->id:created;
            if(!strcmp(family,"ascending"))item->key=(uint32_t)created;
            else if(!strcmp(family,"descending"))item->key=UINT32_MAX-(uint32_t)created;
            else if(!strcmp(family,"equal"))item->key=0;
            else if(!strcmp(family,"duplicates") || !strcmp(family,"managed_churn"))item->key=random_next(&state)%16;
            else if(!strcmp(family,"alternating"))item->key=(created%2)?UINT32_MAX-(uint32_t)created:(uint32_t)created;
            else item->key=random_next(&state);
            if(is_managed)at=upper(model,n,item->key);
            else if(building || !strcmp(family,"endpoint"))at=n;
            else if(!strcmp(family,"hotspot"))at=n/2;
            else if(!strcmp(family,"local"))at=n>32?n-1-random_next(&state)%32:n;
            else at=random_next(&state)%(n+1);
            t=now();
            if(is_managed)status=lks_managed_order_insert(managed,item,&handles[created]);
            else if(!at)status=lks_order_insert_front(o,item,&handles[created]);
            else if(at==n)status=lks_order_insert_back(o,item,&handles[created]);
            else if(phase%2)status=lks_order_insert_before(o,handles[model[at]->id],item,&handles[created]);
            else status=lks_order_insert_after(o,handles[model[at-1]->id],item,&handles[created]);
            latency[samples++]=now()-t;CHECK(status==LKS_STATUS_OK && o->revision==revision+1);
            if(pending) { handles[item->id]=handles[created];handles[created]=NULL;pending=NULL; }else ++created;
            model_insert(model,&n,at,item);
        }else if(operation==1) {
            at=random_next(&state)%n;
            if(is_managed)while(at && model[at-1]->key==model[at]->key)--at;
            item=model[at];t=now();
            status=is_managed?lks_managed_order_remove(managed,handles[item->id],NULL):lks_order_remove(o,handles[item->id],NULL);
            latency[samples++]=now()-t;CHECK(status==LKS_STATUS_OK && o->revision==revision+1);
            handles[item->id]=NULL;(void)model_remove(model,&n,at);
            if(!strcmp(family,"delete_reinsert"))pending=item;
        }else {
            const LksOrderHandle *handle,*anchor;unsigned kind=(unsigned)(phase%4);
            from=random_next(&state)%n;
            if(!strcmp(family,"local_drag"))to=(from+1+phase%7)%n;
            else if(!strcmp(family,"hotspot_drag"))to=n/2+(phase%17<n/2?phase%17:0);
            else to=(from+n/2)%n;
            if(!strcmp(family,"front_back")) { kind=(unsigned)(phase%2);to=kind?n:0; }
            else kind=2+(unsigned)(phase%2);
            handle=handles[model[from]->id];anchor=to<n?handles[model[to]->id]:NULL;
            at=to+(kind==3);if(from<at)--at;if(kind>=2 && from==to)at=from;
            t=now();status=kind==0?lks_order_move_front(o,handle):kind==1?lks_order_move_back(o,handle):kind==2?lks_order_move_before(o,handle,anchor):lks_order_move_after(o,handle,anchor);
            latency[samples++]=now()-t;CHECK(status==LKS_STATUS_OK && o->revision==revision+(at!=from));
            if(at!=from) { item=model_remove(model,&n,from);model_insert(model,&n,at,item); }
        }
        metrics.mutation+=latency[samples-1];if(building)metrics.build+=latency[samples-1];record(&metrics,o);
        if(i<64 || i%1024==0 || i+1==initial+steps) {
            verify(o,model,n,handles);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            if(!cadence && !retained_count)CHECK(lks_alloc_stats_get().live_bytes<=sizeof(*o)+(managed?sizeof(*managed):0)+n*sizeof(LksOrderHandle)+(n/LKS_ORDER_BLOCK_MIN+1)*sizeof(LksOrderBlock));
#endif
        }
        if(i+1==initial) { metrics.rss_built=rss();metrics.live_built=lks_alloc_stats_get().live_bytes; }
        if(!building && cadence && (phase+1)%cadence==0) {
            verify(o,model,n,handles);export_one(o,model,n,&resolver,&metrics,NULL);
        }
        if(!building && !strcmp(family,"retained") && retained_count<8 && (phase+1)%(steps/8?steps/8:1)==0) {
            export_one(o,model,n,&resolver,&metrics,&retained[retained_count++]);
        }
    }
    printf("{\"phase_details\":true,\"family\":\"%s\",\"initial\":%zu,\"steps\":%zu,\"run\":%d,\"build_ms\":%.6f,\"operation_ms\":%.6f}\n",family,initial,steps,run,metrics.build*1000,(metrics.mutation-metrics.build)*1000);
    calls=comparisons;verify(o,model,n,handles);metrics.rss_churn=rss();metrics.live_churn=lks_alloc_stats_get().live_bytes;
    t=now();for(i=0;i<1000 && n;++i) { int c;size_t a=i%n,b=(i*97)%n;
        CHECK(lks_order_compare(o,handles[model[a]->id],handles[model[b]->id],&c)==LKS_STATUS_OK && c==(a>b)-(a<b)); }
    metrics.query=now()-t;
    if(retained_count)metrics.rss_retained=rss();
    t=now();while(n) { Item *item=model[n-1];CHECK((is_managed?lks_managed_order_remove(managed,handles[item->id],NULL):lks_order_remove(o,handles[item->id],NULL))==LKS_STATUS_OK);--n;
        if(n==16) { metrics.live_small=lks_alloc_stats_get().live_bytes;metrics.rss_small=rss(); }
    }metrics.cleanup+=now()-t;
    metrics.live_empty=lks_alloc_stats_get().live_bytes;metrics.rss_empty=rss();
    t=now();if(managed)lks_managed_order_destroy(managed);else lks_order_destroy(o);metrics.cleanup+=now()-t;
    /* Historical associations remain owned after every resident and source dies. */
    for(i=0;i<retained_count;++i) { const void *data;size_t size;CHECK(lks_snapshot_count(retained[i]) && lks_snapshot_association(retained[i],0,&data,&size)==LKS_STATUS_OK && size==8);lks_snapshot_destroy(retained[i]); }
    metrics.rss_released=rss();stats=lks_alloc_stats_get();CHECK(!stats.live_bytes && !stats.live_blocks);
    free(items);free(model);free(handles);metrics.rss_final=rss();
    print_metrics(family,initial,steps,cadence,seed,run,&metrics,latency,samples,calls,stats.peak_live_bytes,stats.alloc_calls+stats.realloc_calls,stats.total_successful_requested_bytes,stats.free_calls);
    free(latency);
}
static void reference_trace(const char *family,size_t initial,size_t steps,uint32_t seed,int run)
{
    size_t capacity=initial+steps+1,n=0,created=0,i,samples=0,calls,max_depth=0,depth_sum=0,path_bytes=0,lk1_bytes=0;
    uint32_t state=seed;Item *items=(Item *)calloc(capacity,sizeof(*items));Item **model=(Item **)malloc(capacity*sizeof(*model));
    const LksTreeNode **nodes=(const LksTreeNode **)malloc(capacity*sizeof(*nodes));
    LksComparator cmp={compare,NULL};LksOrderedTree *tree;Metrics m={0};LksAllocStats stats;
    double *latency=(double *)malloc(capacity*sizeof(*latency)),t;LksTreeRepairStats repair;
    CHECK(items && model && nodes && latency && !lks_alloc_stats_reset());comparisons=0;lks_tree_repair_stats_reset();
    tree=lks_ordered_tree_create(&cmp);CHECK(tree);m.rss_start=rss();
    for(i=0;i<initial+steps;++i) {
        size_t phase=i<initial?i:i-initial,at;int remove=i>=initial && !strcmp(family,"managed_churn") && phase%2==0 && n;
        if(remove) {
            const LksTreeNode *left,*equal,*right;Item *item;
            at=random_next(&state)%n;while(at && model[at-1]->key==model[at]->key)--at;item=model[at];
            t=now();CHECK(lks_ordered_tree_locate(tree,item,&left,&equal,&right)==LKS_STATUS_OK && equal && lks_tree_node_item(equal)==item);
            CHECK(lks_ordered_tree_remove_path(tree,lks_tree_node_path(equal),NULL)==LKS_STATUS_OK);latency[samples++]=now()-t;
            (void)model_remove(model,&n,at);
        }else {
            Item *item=&items[created];const LksTreeNode *node;item->id=created;
            if(!strcmp(family,"ascending"))item->key=(uint32_t)created;
            else if(!strcmp(family,"descending"))item->key=UINT32_MAX-(uint32_t)created;
            else if(!strcmp(family,"equal"))item->key=0;
            else if(!strcmp(family,"duplicates") || !strcmp(family,"managed_churn"))item->key=random_next(&state)%16;
            else if(!strcmp(family,"alternating"))item->key=created%2?UINT32_MAX-(uint32_t)created:(uint32_t)created;
            else item->key=random_next(&state);
            at=upper(model,n,item->key);t=now();CHECK(lks_ordered_tree_insert(tree,item,&node)==LKS_STATUS_OK);latency[samples++]=now()-t;
            model_insert(model,&n,at,item);++created;
        }
        m.mutation+=latency[samples-1];
        if(i<64 || i%1024==0 || i+1==initial+steps) {
            LksTreeInternalProfile profile;size_t j;const LksTree *index=lks_ordered_tree_internal_index(tree);
            CHECK(lks_tree_internal_fill_ordered(index,nodes,n)==LKS_STATUS_OK && lks_ordered_tree_size(tree)==n);
            CHECK(lks_tree_internal_profile(index,&profile)==LKS_STATUS_OK && profile.balance_valid);
            for(j=0;j<n;++j)CHECK(lks_tree_node_item(nodes[j])==model[j]);
        }
    }
    calls=comparisons;m.live_churn=lks_alloc_stats_get().live_bytes;m.rss_churn=rss();
    CHECK(lks_tree_internal_fill_ordered(lks_ordered_tree_internal_index(tree),nodes,n)==LKS_STATUS_OK);
    for(i=0;i<n;++i) {
        const LksPath *p=lks_tree_node_path(nodes[i]);size_t depth=lks_path_depth(p);
        depth_sum+=depth;if(depth>max_depth)max_depth=depth;path_bytes+=lks_path_internal_storage_bytes(p);lk1_bytes+=lks_path_order_key_length(p);
    }
    t=now();for(i=0;i<n;++i) {
        const LksPath *p=lks_tree_node_path(nodes[i]);size_t size=lks_path_order_key_length(p)+1;char *key=(char *)malloc(size);
        CHECK(key && lks_path_order_key_format(p,key,size)==LKS_STATUS_OK);free(key);
    }m.format=now()-t;repair=lks_tree_repair_stats_get();
    t=now();lks_ordered_tree_destroy(tree);m.cleanup=now()-t;stats=lks_alloc_stats_get();CHECK(!stats.live_bytes && !stats.live_blocks);
    free(items);free(model);free(nodes);m.rss_final=rss();
    printf("{\"reference_details\":true,\"family\":\"%s\",\"initial\":%zu,\"steps\":%zu,\"run\":%d,\"resident_count\":%zu,\"path_depth_sum\":%zu,\"path_depth_max\":%zu,\"path_bytes\":%zu,\"lk1_bytes\":%zu,\"relabels\":%zu,\"nodes_relabelled\":%zu,\"max_relabel_region\":%zu,\"full_range_relabels\":%zu}\n",
        family,initial,steps,run,n,depth_sum,max_depth,path_bytes,lk1_bytes,repair.successes,repair.nodes_relabelled,repair.max_region_nodes,repair.full_range_relabels);
    { char name[64];(void)snprintf(name,sizeof(name),"V3_%s",family);
      print_metrics(name,initial,steps,0,seed,run,&m,latency,samples,calls,stats.peak_live_bytes,stats.alloc_calls+stats.realloc_calls,stats.total_successful_requested_bytes,stats.free_calls); }
    free(latency);
}
int main(int argc,char **argv)
{
    if(argc!=7) { fprintf(stderr,"usage: trace family initial steps cadence seed run\n");return 2; }
    { size_t initial=(size_t)strtoull(argv[2],NULL,10),steps=(size_t)strtoull(argv[3],NULL,10);
      CHECK(steps && initial<SIZE_MAX-steps && initial+steps<SIZE_MAX/sizeof(Item *));
      if(!strncmp(argv[1],"V3_",3))reference_trace(argv[1]+3,initial,steps,(uint32_t)strtoul(argv[5],NULL,10),atoi(argv[6]));
      else trace(argv[1],initial,steps,(size_t)strtoull(argv[4],NULL,10),(uint32_t)strtoul(argv[5],NULL,10),atoi(argv[6])); }
    return 0;
}
