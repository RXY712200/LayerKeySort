/* Preliminary live-mutation screen. Separate production/diagnostic executables:
 * diagnostic timings include counters and must not be mixed with production. */
#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
typedef struct ScreenItem { int key; size_t id; } ScreenItem;
typedef struct ScreenWork { size_t assignments,arrays,blocks,splits,repairs,max_assign,max_blocks; } ScreenWork;
static size_t screen_calls;
static double screen_now(void)
{
#ifdef _WIN32
    LARGE_INTEGER ticks,frequency;QueryPerformanceCounter(&ticks);QueryPerformanceFrequency(&frequency);
    return (double)ticks.QuadPart/(double)frequency.QuadPart;
#else
    struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static uint32_t screen_random(uint32_t *s) { *s=*s*UINT32_C(1664525)+UINT32_C(1013904223);return *s; }
static int screen_compare(const void *a,const void *b,void *context)
{ const ScreenItem *x=(const ScreenItem *)a,*y=(const ScreenItem *)b;(void)context;++screen_calls;return (x->key>y->key)-(x->key<y->key); }
static int screen_time_compare(const void *a,const void *b)
{ double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y); }
static void screen_work(ScreenWork *w,const LksOrder *order)
{
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    const LksOrderWork *v=&order->work;
    w->assignments+=v->records_reassigned;w->arrays+=v->local_blocks;w->blocks+=v->index_blocks;
    w->splits+=v->splits;w->repairs+=v->merges+v->redistributions;
    if(v->records_reassigned>w->max_assign)w->max_assign=v->records_reassigned;
    if(v->index_blocks>w->max_blocks)w->max_blocks=v->index_blocks;
#else
    (void)w;(void)order;
#endif
}
static void screen_print(const char *name,size_t n,size_t operations,double *samples,
    ScreenWork *work,size_t allocations)
{
    size_t i;double total=0;
    for(i=0;i<operations;++i)total+=samples[i];
    qsort(samples,operations,sizeof(*samples),screen_time_compare);
    printf("%s,%zu,%zu,%.3f,%.3f,%.3f,%.3f,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",name,n,operations,total*1000,
        samples[(operations-1)*50/100]*1e6,samples[(operations-1)*99/100]*1e6,samples[operations-1]*1e6,
        screen_calls,work->assignments,work->arrays,work->blocks,work->splits,work->repairs,work->max_assign,work->max_blocks,allocations);
}
static int screen_explicit(unsigned pattern,size_t n,size_t operations)
{
    ScreenItem *items=(ScreenItem *)calloc(n,sizeof(*items));
    const LksOrderHandle **handles=(const LksOrderHandle **)calloc(n,sizeof(*handles));
    double *samples=(double *)calloc(operations,sizeof(*samples));
    LksOrder *order=lks_order_create();size_t i,seen=0;LksAllocStats before,after;
    uint32_t random=17;ScreenWork work={0};int result=1;const LksOrderHandle *it;
    static const char *names[]={"move_local","move_distant","move_random","insert_remove_mix"};
    if(!items || !handles || !samples || !order)goto done;
    for(i=0;i<n;++i){items[i].id=i;if(lks_order_insert_back(order,&items[i],&handles[i])!=LKS_STATUS_OK)goto done;}
    before=lks_alloc_stats_get();screen_calls=0;
    for(i=0;i<operations;++i) {
        LksStatus status;size_t from=0,to=0;double start;
        if(pattern==2){from=screen_random(&random)%n;to=screen_random(&random)%n;}
        start=screen_now();
        if(pattern==0)status=lks_order_move_before(order,handles[10],handles[i%2?20:40]);
        else if(pattern==1)status=i%2?lks_order_move_front(order,handles[0]):lks_order_move_back(order,handles[0]);
        else if(pattern==2)status=lks_order_move_before(order,handles[from],handles[to]);
        else status=i%2?lks_order_insert_front(order,&items[0],&handles[0]):lks_order_remove(order,handles[0],NULL);
        samples[i]=screen_now()-start;if(status!=LKS_STATUS_OK)goto done;screen_work(&work,order);
    }
    after=lks_alloc_stats_get();
    for(it=lks_order_first(order);it;it=lks_order_next(it)) { if(!lks_order_item(it))goto done;++seen; }
    if(seen!=n-(pattern==3 && operations%2))goto done;
    screen_print(names[pattern],n,operations,samples,&work,after.alloc_calls-before.alloc_calls);result=0;
done:
    lks_order_destroy(order);free(items);free(handles);free(samples);return result;
}
static int screen_managed(unsigned pattern,size_t n)
{
    ScreenItem *items=(ScreenItem *)calloc(n,sizeof(*items));double *samples=(double *)calloc(n,sizeof(*samples));
    LksComparator comparator={screen_compare,NULL};LksManagedOrder *order=lks_managed_order_create(&comparator);
    uint32_t random=17;size_t i,seen=0;ScreenWork work={0};LksAllocStats before,after;int result=1;
    const LksOrderHandle *h,*it;ScreenItem *previous=NULL;
    static const char *names[]={"managed_ascending","managed_random","managed_equal","managed_duplicates","managed_alternating"};
    if(!items || !samples || !order)goto done;
    before=lks_alloc_stats_get();screen_calls=0;
    for(i=0;i<n;++i) {
        double start;LksStatus status;int key=(int)i;
        if(pattern==1)key=(int)(screen_random(&random)%100000u);
        else if(pattern==2)key=0;
        else if(pattern==3)key=(int)(screen_random(&random)%16u);
        else if(pattern==4)key=i%2? (int)i:-(int)i;
        items[i].key=key;items[i].id=i;start=screen_now();
        status=lks_managed_order_insert(order,&items[i],&h);samples[i]=screen_now()-start;
        if(status!=LKS_STATUS_OK)goto done;
        screen_work(&work,order->core);
    }
    after=lks_alloc_stats_get();
    for(it=lks_managed_order_first(order);it;it=lks_order_next(it)) {
        ScreenItem *item=(ScreenItem *)lks_order_item(it);
        if(previous && (previous->key>item->key || (previous->key==item->key && previous->id>=item->id)))goto done;
        previous=item;++seen;
    }
    if(seen!=n)goto done;
    screen_print(names[pattern],n,n,samples,&work,after.alloc_calls-before.alloc_calls);result=0;
done:
    lks_managed_order_destroy(order);free(items);free(samples);return result;
}
int main(int argc,char **argv)
{
    unsigned pattern;size_t n=20000,operations=20000;
    if(argc>1){n=512;operations=1000;} /* fixed CI smoke, no untrusted size parser */
    (void)argv;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    puts("# diagnostic: counters enabled; timing includes instrumentation");
#else
    puts("# production: structural/allocation columns unavailable (zero)");
#endif
    puts("workload,N,operations,total_ms,p50_us,p99_us,max_us,comparator_calls,assignments,local_arrays,index_blocks,splits,repairs,max_assignments,max_index_blocks,allocations");
    for(pattern=0;pattern<4;++pattern)if(screen_explicit(pattern,n,operations))return 1;
    for(pattern=0;pattern<5;++pattern)if(screen_managed(pattern,n))return 1;
    return lks_alloc_stats_get().live_blocks?1:0;
}
