/* V0.1 common known-handle replay. Preparation/oracles are outside timing.
 * Deliberately no snapshot, comparator facade, or persistent-key benchmark. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <time.h>
#endif
#include "baseline.h"
#if FULL
#include "layerkeysort.h"
#ifdef DIAGNOSTIC
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#endif
#endif
#define REQUIRE(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(2); } } while(0)
#if FULL
typedef LksOrder S;
typedef const LksOrderHandle H;
static S *create(void) { return lks_order_create(); }
static H *insert(S *s,H *a,int after,void *item) {
    H *out=NULL;LksStatus st=a?(after?lks_order_insert_after(s,a,item,&out):lks_order_insert_before(s,a,item,&out)):lks_order_insert_back(s,item,&out);
    REQUIRE(st==LKS_STATUS_OK || st==LKS_STATUS_OUT_OF_MEMORY);return out;
}
static void remove_h(S *s,H *h) { void *p=NULL;REQUIRE(lks_order_remove(s,h,&p)==LKS_STATUS_OK && p); }
static int move_h(S *s,H *h,H *a,int after) { return (int)(after?lks_order_move_after(s,h,a):lks_order_move_before(s,h,a)); }
static int compare_h(S *s,H *a,H *b) { int c=9;REQUIRE(lks_order_compare(s,a,b,&c)==LKS_STATUS_OK);return (c>0)-(c<0); }
#define first lks_order_first
#define last lks_order_last
#define next_h lks_order_next
#define previous lks_order_previous
#define item_h lks_order_item
#define count_h lks_order_size
#define destroy lks_order_destroy
static int valid(S *s) {
#ifdef DIAGNOSTIC
 return lks_order_internal_valid(s);
#else
 (void)s;return 1;
#endif
}
static Stats stats_h(void) {
 Stats x={0};
#ifdef DIAGNOSTIC
 LksAllocStats a=lks_alloc_stats_get();x.live=a.live_bytes;x.peak=a.peak_live_bytes;x.allocations=a.alloc_calls;x.frees=a.free_calls;x.attempts=lks_alloc_test_get_attempt_count();
#endif
 return x;
}
#ifdef DIAGNOSTIC
static void fail_h(size_t at) {
 if(at)lks_alloc_test_fail_on_attempt(at);else lks_alloc_test_disable_failure();
}
#endif
#else
typedef Sequence S;typedef Node H;
#define create seq_create
#define insert seq_insert
#define remove_h seq_remove
static int move_h(S *s,H *h,H *a,int after) { seq_move(s,h,a,after);return 0; }
static int compare_h(S *s,H *a,H *b) { (void)s;return seq_compare(a,b); }
#define first seq_first
#define last seq_last
#define next_h seq_next
#define previous seq_previous
#define item_h seq_item
#define count_h seq_size
#define destroy seq_destroy
#define valid seq_valid
#define stats_h seq_stats
#define fail_h seq_fail
#endif
static int payload[64];
static void verify(S *s,H **registry,const size_t *model,size_t n) {
    H *h=first(s);REQUIRE(count_h(s)==n && valid(s));
    for(size_t i=0;i<n;++i) { size_t id=model[i];REQUIRE(h==registry[id] && item_h(h)==&payload[id%64]);h=next_h(h); }
    REQUIRE(!h);h=last(s);
    for(size_t i=n;i;--i) { REQUIRE(h==registry[model[i-1]]);h=previous(h); }
    REQUIRE(!h);
    for(size_t i=1;i<n;i+=97)REQUIRE(compare_h(s,registry[model[i-1]],registry[model[i]])<0);
}
static size_t locate(const size_t *model,size_t n,size_t id) {
    for(size_t i=0;i<n;++i)if(model[i]==id)return i;
    REQUIRE(0);return 0;
}
static void model_remove(size_t *model,size_t *n,size_t at) { memmove(model+at,model+at+1,(*n-at-1)*sizeof(*model));--*n; }
static void model_insert(size_t *model,size_t *n,size_t at,size_t id) { memmove(model+at+1,model+at,(*n-at)*sizeof(*model));model[at]=id;++*n; }
static double now(void) {
#ifdef _WIN32
 LARGE_INTEGER c,f;QueryPerformanceCounter(&c);QueryPerformanceFrequency(&f);return (double)c.QuadPart/(double)f.QuadPart;
#else
 struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec+(double)t.tv_nsec*1e-9;
#endif
}
static void rss(size_t *current,size_t *peak) {
 *current=*peak=0;
#ifdef _WIN32
 PROCESS_MEMORY_COUNTERS m;memset(&m,0,sizeof(m));m.cb=sizeof(m);
 REQUIRE(GetProcessMemoryInfo(GetCurrentProcess(),&m,sizeof(m)));*current=m.WorkingSetSize;*peak=m.PeakWorkingSetSize;
#endif
}
typedef struct Op { char kind;size_t a,b;int after,expected; } Op;
static void replay(const char *path,int correctness) {
    FILE *f=fopen(path,"r");size_t initial,operations,final_n;REQUIRE(f);
    REQUIRE(fscanf(f,"%zu %zu %zu",&initial,&operations,&final_n)==3);
    REQUIRE(initial<=100000 && operations<=20000 && final_n<=initial+operations);
    size_t capacity=initial+operations+1;
    H **handles=calloc(capacity,sizeof(*handles));size_t *model=calloc(capacity,sizeof(*model)),*expected=calloc(capacity,sizeof(*expected));
    Op *ops=calloc(operations,sizeof(*ops));REQUIRE(handles && model && expected && ops);
    for(size_t i=0;i<operations;++i)REQUIRE(fscanf(f," %c %zu %zu %d %d",&ops[i].kind,&ops[i].a,&ops[i].b,&ops[i].after,&ops[i].expected)==5);
    for(size_t i=0;i<final_n;++i)REQUIRE(fscanf(f,"%zu",expected+i)==1 && expected[i]<capacity);
    REQUIRE(fclose(f)==0);S *s=create();REQUIRE(s);size_t n=initial;
    for(size_t i=0;i<initial;++i) { handles[i]=insert(s,NULL,1,&payload[i%64]);REQUIRE(handles[i]);model[i]=i; }
    verify(s,handles,model,n);
    size_t reassigned=0,max_reassigned=0,splits=0,merges=0,redistributions=0,cross=0;
    double start=now();
    for(size_t i=0;i<operations;++i) {
        Op o=ops[i];REQUIRE(o.a<capacity && o.b<capacity);
        if(o.kind=='C')REQUIRE(compare_h(s,handles[o.a],handles[o.b])==o.expected);
        else if(o.kind=='I') { REQUIRE(!handles[o.a]);handles[o.a]=insert(s,handles[o.b],o.after,&payload[o.a%64]);REQUIRE(handles[o.a]); }
        else if(o.kind=='R') { REQUIRE(handles[o.a]);remove_h(s,handles[o.a]);handles[o.a]=NULL; }
        else { REQUIRE(o.kind=='M');REQUIRE(move_h(s,handles[o.a],handles[o.b],o.after)==0); }
#if FULL && defined(DIAGNOSTIC)
        if(o.kind!='C') { size_t k=s->work.records_reassigned;reassigned+=k;if(k>max_reassigned)max_reassigned=k;
            splits+=s->work.splits;merges+=s->work.merges;redistributions+=s->work.redistributions;cross+=s->work.cross_block_moves; }
#endif
        if(correctness) {
            if(o.kind=='I')model_insert(model,&n,locate(model,n,o.b)+(size_t)o.after,o.a);
            else if(o.kind=='R')model_remove(model,&n,locate(model,n,o.a));
            else if(o.kind=='M' && o.a!=o.b) { model_remove(model,&n,locate(model,n,o.a));model_insert(model,&n,locate(model,n,o.b)+(size_t)o.after,o.a); }
            if(i%128==0)verify(s,handles,model,n);
        }
    }
    double elapsed=now()-start;verify(s,handles,expected,final_n);
    size_t current,peak;rss(&current,&peak);Stats before=stats_h();destroy(s);Stats after=stats_h();REQUIRE(!after.live);
    printf("%.9f,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",elapsed,before.live,before.peak,before.allocations,before.frees,after.live,current,peak,reassigned,max_reassigned,splits,merges,redistributions,cross,after.frees);
    free(handles);free(model);free(expected);free(ops);
}
/* Failure tests use public observations plus Full's diagnostic invariant,
 * revision and cursor. Never inspect an expired handle. */
static void faults(void) {
#ifndef DIAGNOSTIC
    REQUIRE(0);
#else
    size_t cases=0,failures=0;
    fail_h(1);REQUIRE(!create() && stats_h().attempts==1);fail_h(0);++cases;++failures;
    for(size_t initial=0;initial<=96;initial+=32) {
        for(size_t at=1;at<=4;++at) {
            S *s=create();H *h[100];size_t model[100];REQUIRE(s);
            for(size_t i=0;i<initial;++i) { h[i]=insert(s,NULL,1,&payload[i%64]);REQUIRE(h[i]);model[i]=i; }
            Stats old=stats_h();
#if FULL
            uint64_t rev=s->revision;LksOrderCursor *cursor=NULL;REQUIRE(lks_order_cursor_create(s,0,&cursor)==LKS_STATUS_OK);old=stats_h();
#endif
            fail_h(at);H *out=insert(s,NULL,1,&payload[0]);size_t attempts=stats_h().attempts;fail_h(0);++cases;
            if(!out) { ++failures;REQUIRE(attempts==at && stats_h().live==old.live);verify(s,h,model,initial);
#if FULL
                H *next=NULL;REQUIRE(s->revision==rev && lks_order_cursor_next(cursor,&next)==LKS_STATUS_OK && next==(initial?h[0]:NULL));
#endif
            }
#if FULL
            lks_order_cursor_destroy(cursor);
#endif
            destroy(s);REQUIRE(!stats_h().live);if(out)break;
        }
    }
    S *s=create();H *h[96];size_t model[96];REQUIRE(s);
    for(size_t i=0;i<96;++i) { h[i]=insert(s,NULL,1,&payload[i%64]);REQUIRE(h[i]);model[i]=i; }
    Stats old=stats_h();
#if FULL
    uint64_t move_revision=s->revision;LksOrderCursor *move_cursor=NULL;
    REQUIRE(lks_order_cursor_create(s,0,&move_cursor)==LKS_STATUS_OK);old=stats_h();
#endif
    fail_h(1);int status=move_h(s,h[0],h[95],1);size_t attempts=stats_h().attempts;fail_h(0);
#if FULL
    REQUIRE(status==LKS_STATUS_OUT_OF_MEMORY && attempts==1 && stats_h().live==old.live);++failures;
    H *still_first=NULL;REQUIRE(s->revision==move_revision && lks_order_cursor_next(move_cursor,&still_first)==LKS_STATUS_OK && still_first==h[0]);
    lks_order_cursor_destroy(move_cursor);
#else
    REQUIRE(status==0 && attempts==0 && stats_h().live==old.live);model_remove(model,&(size_t){96},0);model[95]=0;
#endif
    verify(s,h,model,96);fail_h(1);REQUIRE(move_h(s,h[1],h[2],0)==0 && move_h(s,h[1],h[1],1)==0 && !stats_h().attempts);fail_h(0);
    for(size_t n=96;n;) { H *x=first(s);fail_h(1);remove_h(s,x);REQUIRE(!stats_h().attempts);fail_h(0);--n;REQUIRE(valid(s) && count_h(s)==n); }
    REQUIRE(!first(s) && !last(s));H *fresh=insert(s,NULL,1,&payload[0]);REQUIRE(fresh && item_h(fresh)==&payload[0]);destroy(s);REQUIRE(!stats_h().live);
    printf("fault_cases=%zu injected_failures=%zu cross_move_attempts=%zu drain_reinsert=ok live=0\n",cases+1,failures,attempts);
#endif
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"faults"))faults();
    else { REQUIRE(argc==3);replay(argv[1],atoi(argv[2])); }
    return 0;
}
