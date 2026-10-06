#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"Preview.2 line %d: %s\n",__LINE__,#c); exit(1); } } while (0)
typedef struct Item { int key; size_t id; } Item;
static size_t max_assign, max_arrays, max_index, move_ops, cross_moves, noops, move_splits, move_repairs;
static size_t calls, max_insert_calls, max_locate_calls;
static uint32_t rng(uint32_t *s) { *s = *s*UINT32_C(1664525)+UINT32_C(1013904223); return *s; }
static int compare(const void *a, const void *b, void *context)
{ const Item *x=(const Item *)a, *y=(const Item *)b; (void)context; ++calls; return (x->key>y->key)-(x->key<y->key); }
static void sequence_check(const LksOrder *o, const LksOrderHandle **seq, size_t n)
{
    const LksOrderHandle *h=lks_order_first(o); size_t i;
    CHECK(lks_order_internal_valid(o) && lks_order_size(o)==n);
    for(i=0;i<n;++i) { CHECK(h==seq[i] && h->item==seq[i]->item); h=lks_order_next(h); }
    CHECK(!h); h=lks_order_last(o);
    for(i=n;i>0;--i) { CHECK(h==seq[i-1]); h=lks_order_previous(h); }
    CHECK(!h);
    for(i=0;i<32 && n;++i) {
        size_t a=(i*7919u)%n, b=(i*104729u+7)%n; int c=9;
        CHECK(lks_order_compare(o,seq[a],seq[b],&c)==LKS_STATUS_OK && c==(a>b)-(a<b));
    }
}
static void work_check(const LksOrder *o, size_t height)
{
    const LksOrderWork *w=&o->work;
    CHECK(!w->tracking_overflow && w->records_reassigned<=4u*LKS_ORDER_BLOCK_CAPACITY);
    CHECK(w->local_blocks<=4 && w->splits<=1 && w->merges+w->redistributions<=1);
    CHECK(w->index_blocks<=32u*(height+2) && w->rotations<=4u*(height+2));
    if(w->records_reassigned>max_assign)max_assign=w->records_reassigned;
    if(w->local_blocks>max_arrays)max_arrays=w->local_blocks;
    if(w->index_blocks>max_index)max_index=w->index_blocks;
    cross_moves+=w->cross_block_moves; noops+=w->noop_moves;
    move_splits+=w->splits; move_repairs+=w->merges+w->redistributions; ++move_ops;
}
/* Independent flat sequence predicts move positions, never block ranks. */
static void move_model(LksOrder *o, const LksOrderHandle **seq,size_t n,
    size_t from,size_t anchor,unsigned kind)
{
    const LksOrderHandle *h=seq[from]; size_t to,at; uint64_t revision=o->revision;
    size_t height=o->root?(size_t)o->root->height:0;
    LksStatus status;
    if(kind==0) { to=0; status=lks_order_move_front(o,h); }
    else if(kind==1) { to=n; status=lks_order_move_back(o,h); }
    else { to=anchor+(kind==3); status=kind==2?lks_order_move_before(o,h,seq[anchor]):lks_order_move_after(o,h,seq[anchor]); }
    at=to; if(from<at)--at;
    if(kind>=2 && from==anchor)at=from;
    CHECK(status==LKS_STATUS_OK && o->revision==revision+(at!=from));
    if(at==from)CHECK(o->work.noop_moves==1 && !o->work.records_reassigned && !o->work.local_blocks && !o->work.index_blocks);
    else {
        if(from<at)memmove(seq+from,seq+from+1,(at-from)*sizeof(*seq));
        else memmove(seq+at+1,seq+at,(from-at)*sizeof(*seq));
        seq[at]=h;
    }
    work_check(o,height);
}
static void explicit_random(uint32_t seed)
{
    const size_t steps=30000, capacity=4096;
    Item *items=(Item *)calloc(steps+capacity,sizeof(*items));
    const LksOrderHandle **seq=(const LksOrderHandle **)calloc(capacity,sizeof(*seq));
    const LksOrderHandle **registry=(const LksOrderHandle **)calloc(steps+capacity,sizeof(*registry));
    LksOrder *o=lks_order_create(); size_t n=1024,created=1024,i; uint32_t state=seed;
    CHECK(items && seq && registry && o);
    for(i=0;i<n;++i) { items[i].id=i; CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);registry[i]=seq[i]; }
    for(i=0;i<steps;++i) {
        unsigned op=rng(&state)%9u;
        if(!n)op=0;
        if(n==capacity && op<4)op=4;
        if(op<4) {
            size_t at=op==0?0:op==1?n:rng(&state)%n+(op==3);
            const LksOrderHandle *h=NULL; LksStatus s; items[created].id=created;
            if(op==0)s=lks_order_insert_front(o,&items[created],&h);
            else if(op==1)s=lks_order_insert_back(o,&items[created],&h);
            else if(op==2)s=lks_order_insert_before(o,seq[at],&items[created],&h);
            else s=lks_order_insert_after(o,seq[at-1],&items[created],&h);
            CHECK(s==LKS_STATUS_OK && h->item==&items[created]);
            registry[created]=h;
            memmove(seq+at+1,seq+at,(n-at)*sizeof(*seq)); seq[at]=h; ++n; ++created;
        } else if(op==4) {
            size_t at=rng(&state)%n; void *item=NULL,*expected=seq[at]->item;
            registry[((Item *)expected)->id]=NULL;
            CHECK(lks_order_remove(o,seq[at],&item)==LKS_STATUS_OK && item==expected);
            memmove(seq+at,seq+at+1,(n-at-1)*sizeof(*seq)); --n;
        } else {
            size_t from=rng(&state)%n, anchor=rng(&state)%n;
            move_model(o,seq,n,from,anchor,op-5);
        }
        CHECK(lks_order_internal_valid(o));
        if(i%31==0) {
            size_t j;sequence_check(o,seq,n);
            for(j=0;j<created;++j)if(registry[j])CHECK(lks_order_item(registry[j])==&items[j]);
        }
    }
    sequence_check(o,seq,n); lks_order_destroy(o); free(registry); free(seq); free(items);
    printf("explicit oracle seed=%u steps=%zu PASS\n",(unsigned)seed,steps);
}
static void move_boundaries(void)
{
    enum { B=LKS_ORDER_BLOCK_CAPACITY, H=LKS_ORDER_BLOCK_MIN };
    Item items[(3*B)]; const LksOrderHandle *seq[(3*B)]; size_t i;
    LksOrder *o=lks_order_create(), *foreign=lks_order_create(); const LksOrderHandle *f;
    CHECK(o && foreign && lks_order_insert_back(foreign,&items[0],&f)==LKS_STATUS_OK);
    for(i=0;i<B;++i)CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);
    move_model(o,seq,B,(B/8),(3*B/4),2); move_model(o,seq,B,(3*B/4),(B/8),3);
    move_model(o,seq,B,0,(B-1),1); move_model(o,seq,B,(B-1),0,0);
    sequence_check(o,seq,B);
    CHECK(lks_order_move_before(o,seq[0],f)==LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_order_move_front(NULL,seq[0])==LKS_STATUS_INVALID_ARGUMENT);
    for(i=B;i<(3*H);++i)CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);
    CHECK(o->first->count==H && o->last->count==B);
    move_model(o,seq,(3*H),0,(3*H-1),3); /* split full adjacent destination, repair the now-underfull source with its new neighbor */
    sequence_check(o,seq,(3*H)); CHECK(o->work.splits==1 && o->work.merges==1);
    /* Fresh H+B fixture: source B -> destination H without allocation. */
    lks_order_destroy(o); o=lks_order_create();
    for(i=0;i<(3*H);++i)CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);
    move_model(o,seq,(3*H),(3*H-1),0,2); sequence_check(o,seq,(3*H));
    /* Fresh H+B+H: move H source into nonfull third; source redistributes
     * with the untouched full middle, and is not adjacent to destination. */
    lks_order_destroy(o); o=lks_order_create();
    for(i=0;i<(2*B);++i)CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);
    for(i=0;i<H;++i)move_model(o,seq,(2*B),(2*B-1),o->first->count,2);
    CHECK(o->first->count==H && o->first->next->count==B && o->last->count==H);
    move_model(o,seq,(2*B),0,(2*B-1),1);CHECK(o->work.redistributions==1);
    sequence_check(o,seq,(2*B));
    lks_order_destroy(o); lks_order_destroy(foreign);
}
static void long_moves(void)
{
    const size_t n=100001; size_t i;
    Item *items=(Item *)calloc(n,sizeof(*items));
    const LksOrderHandle **seq=(const LksOrderHandle **)calloc(n,sizeof(*seq));
    LksOrder *o=lks_order_create(); CHECK(items && seq && o);
    for(i=0;i<n;++i) { items[i].id=i; CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK); }
    for(i=0;i<2000;++i) {
        move_model(o,seq,n,i%2? n-1:0,i%2?0:n-1,i%2?0:1);
        if(i%100==0)CHECK(lks_order_internal_valid(o));
        if(i%500==0)sequence_check(o,seq,n);
    }
    sequence_check(o,seq,n); lks_order_destroy(o);free(seq);free(items);
    puts("100001-resident distant move campaign 2000 moves PASS");
}
static void cursor_tests(void)
{
    Item items[130]; size_t i; LksOrder *o=lks_order_create();
    const LksOrderHandle *a,*b,*h; LksOrderCursor *c=NULL; uint64_t revision;
    CHECK(o && lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && !h); lks_order_cursor_destroy(c);
    CHECK(lks_order_insert_back(o,&items[0],&a)==LKS_STATUS_OK && lks_order_insert_back(o,&items[1],&b)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    revision=o->revision; lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
    CHECK(lks_order_move_front(o,a)==LKS_STATUS_OK && lks_order_move_back(o,b)==LKS_STATUS_OK);
    CHECK(lks_order_move_before(o,a,a)==LKS_STATUS_OK && lks_order_move_after(o,b,b)==LKS_STATUS_OK);
    CHECK(lks_order_move_before(o,a,b)==LKS_STATUS_OK && lks_order_move_after(o,b,a)==LKS_STATUS_OK);
    CHECK(o->revision==revision && !lks_alloc_test_get_attempt_count());
    CHECK(lks_order_insert_back(o,&items[2],&h)==LKS_STATUS_OUT_OF_MEMORY && !h);
    lks_alloc_test_disable_failure();
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==a);
    CHECK(lks_order_move_front(o,b)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED && !h);lks_order_cursor_destroy(c);
    CHECK(lks_order_cursor_create(o,1,&c)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==a);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==b);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && !h);lks_order_cursor_destroy(c);
    CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    CHECK(lks_order_remove(o,b,NULL)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED && !h);lks_order_cursor_destroy(c);
    for(i=2;i<130;++i)CHECK(lks_order_insert_back(o,&items[i],&h)==LKS_STATUS_OK);
    o->revision=UINT64_MAX;
    CHECK(lks_order_move_front(o,lks_order_first(o))==LKS_STATUS_OK);
    CHECK(lks_order_move_back(o,lks_order_first(o))==LKS_STATUS_CAPACITY_LIMIT);
    CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK);lks_order_cursor_destroy(c);
    lks_order_destroy(o);
    puts("cursor revision/no-op/failure/capacity cases PASS");
}
static void move_oom(void)
{
    enum { B=LKS_ORDER_BLOCK_CAPACITY, H=LKS_ORDER_BLOCK_MIN };
    Item items[(3*H)]; const LksOrderHandle *seq[(3*H)],*h; LksOrder *o=lks_order_create();
    LksOrderCursor *c; size_t i; LksAllocStats before,after; LksOrderBlock left,right; uint64_t revision;
    CHECK(o);
    for(i=0;i<(3*H);++i)CHECK(lks_order_insert_back(o,&items[i],&seq[i])==LKS_STATUS_OK);
    CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    left=*o->first;right=*o->last;revision=o->revision;before=lks_alloc_stats_get();
    lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
    CHECK(lks_order_move_back(o,seq[0])==LKS_STATUS_OUT_OF_MEMORY);
    CHECK(!memcmp(&left,o->first,sizeof(left)) && !memcmp(&right,o->last,sizeof(right)) && o->revision==revision);
    after=lks_alloc_stats_get();CHECK(after.live_blocks==before.live_blocks && after.live_bytes==before.live_bytes);
    sequence_check(o,seq,(3*H));CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==seq[0]);
    lks_alloc_test_disable_failure();move_model(o,seq,(3*H),0,(3*H-1),1);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED);lks_order_cursor_destroy(c);
    /* Non-full destination and same-block paths allocate nothing. */
    lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
    move_model(o,seq,(3*H),(3*H-1),0,0);move_model(o,seq,(3*H),1,10,2);
    CHECK(!lks_alloc_test_get_attempt_count());lks_alloc_test_disable_failure();
    sequence_check(o,seq,(3*H));lks_order_destroy(o);puts("move spare OOM + allocation-free moves PASS");
}
static void managed_check(LksManagedOrder *o,const LksOrderHandle **seq,size_t n)
{ CHECK(lks_managed_order_size(o)==n);sequence_check(o->core,seq,n); }
static void locate_check(LksManagedOrder *o,const LksOrderHandle **seq,size_t n,int key)
{
    Item q={key,0};const LksOrderHandle *p,*e,*s;size_t lo=0,hi;size_t before=calls,limit=2u*(o->core->root?(size_t)o->core->root->height:0)+17u;
    while(lo<n && ((Item *)seq[lo]->item)->key<key)++lo;
    hi=lo;while(hi<n && ((Item *)seq[hi]->item)->key==key)++hi;
    CHECK(lks_managed_order_locate(o,&q,&p,&e,&s)==LKS_STATUS_OK);
    CHECK(p==(lo?seq[lo-1]:NULL) && e==(hi>lo?seq[lo]:NULL) && s==(hi<n?seq[hi]:NULL));
    CHECK(calls-before<=limit);if(calls-before>max_locate_calls)max_locate_calls=calls-before;
}
static void managed_campaign(unsigned pattern,size_t n)
{
    Item *items=(Item *)calloc(n,sizeof(*items));const LksOrderHandle **seq=(const LksOrderHandle **)calloc(n,sizeof(*seq));
    LksComparator descriptor={compare,NULL};LksManagedOrder *o=lks_managed_order_create(&descriptor);
    uint32_t state=17;size_t i,count=0; CHECK(items && seq && o);
    descriptor.compare=NULL; /* Facade copied the descriptor. */
    for(i=0;i<n;++i) {
        size_t at=count,before=calls,limit=(o->core->root?(size_t)o->core->root->height:0)+8;
        const LksOrderHandle *h;int key=0;
        if(pattern==0)key=(int)i;
        else if(pattern==1)key=-(int)i;
        else if(pattern==2)key=(int)(rng(&state)%100000u);
        else if(pattern==4)key=(int)(rng(&state)%8u);
        else if(pattern==5)key=i%2? (int)i:-(int)i;
        else if(pattern==6)key=i%5?10:(int)(rng(&state)%21u);
        items[i].key=key;items[i].id=i;
        while(at && ((Item *)seq[at-1]->item)->key>key)--at;
        CHECK(lks_managed_order_insert(o,&items[i],&h)==LKS_STATUS_OK);
        CHECK(calls-before<=limit);if(calls-before>max_insert_calls)max_insert_calls=calls-before;
        memmove(seq+at+1,seq+at,(count-at)*sizeof(*seq));seq[at]=h;++count;
        if(i%257==0)managed_check(o,seq,count);
    }
    managed_check(o,seq,count);locate_check(o,seq,count,-100001);locate_check(o,seq,count,100001);
    locate_check(o,seq,count,items[n/2].key);locate_check(o,seq,count,0);
    while(count) {
        size_t at=rng(&state)%count,before=calls;void *item=seq[at]->item,*removed;
        lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
        CHECK(lks_managed_order_remove(o,seq[at],&removed)==LKS_STATUS_OK && removed==item);
        CHECK(calls==before && !lks_alloc_test_get_attempt_count());lks_alloc_test_disable_failure();
        memmove(seq+at,seq+at+1,(count-at-1)*sizeof(*seq));--count;
        if(count%257==0)managed_check(o,seq,count);
    }
    lks_managed_order_destroy(o);free(seq);free(items);
    printf("managed pattern=%u N=%zu stable oracle/remove/call envelope PASS\n",pattern,n);
}
static void managed_oom(void)
{
    LksComparator descriptor={compare,NULL};size_t fail,i,initial;
    for(fail=1;fail<=3;++fail) {
        LksManagedOrder *o;lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(fail);
        o=lks_managed_order_create(&descriptor);lks_alloc_test_disable_failure();
        CHECK((fail<=2 && !o)||(fail==3 && o));lks_managed_order_destroy(o);
    }
    for(initial=0;initial<=LKS_ORDER_BLOCK_CAPACITY;initial=initial==0?1:initial==1?LKS_ORDER_BLOCK_CAPACITY:LKS_ORDER_BLOCK_CAPACITY+1) {
        for(fail=1;fail<=3;++fail) {
            Item items[LKS_ORDER_BLOCK_CAPACITY+1];LksManagedOrder *o=lks_managed_order_create(&descriptor);const LksOrderHandle *seq[LKS_ORDER_BLOCK_CAPACITY+1],*h=NULL;
            LksAllocStats before,after;LksOrderCursor *c;uint64_t revision;LksStatus status;
            CHECK(o);for(i=0;i<=initial;++i){items[i].key=0;items[i].id=i;}
            for(i=0;i<initial;++i)CHECK(lks_managed_order_insert(o,&items[i],&seq[i])==LKS_STATUS_OK);
            CHECK(lks_managed_order_cursor_create(o,0,&c)==LKS_STATUS_OK);before=lks_alloc_stats_get();revision=o->core->revision;
            lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(fail);
            status=lks_managed_order_insert(o,&items[initial],&h);lks_alloc_test_disable_failure();
            if(status==LKS_STATUS_OUT_OF_MEMORY) {
                CHECK(!h && o->core->revision==revision);after=lks_alloc_stats_get();
                CHECK(before.live_bytes==after.live_bytes && before.live_blocks==after.live_blocks);
                managed_check(o,seq,initial);CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK);
            } else { CHECK(status==LKS_STATUS_OK);CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED); }
            lks_order_cursor_destroy(c);lks_managed_order_destroy(o);
        }
    }
    { LksOrder *o=lks_order_create();LksOrderCursor *c=NULL;
      lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
      CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OUT_OF_MEMORY && !c);
      lks_alloc_test_disable_failure();CHECK(lks_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
      lks_order_cursor_destroy(c);lks_order_destroy(o); }
    puts("managed creation/insertion + cursor OOM sweeps PASS");
}

typedef struct Reentry { LksManagedOrder *order; LksOrderCursor *cursor; const LksOrderHandle *handle; size_t tested; } Reentry;
static int reentry_compare(const void *a,const void *b,void *context)
{
    Reentry *r=(Reentry *)context;const LksOrderHandle *h,*p,*e,*s;LksOrderCursor *c=NULL;int result=9;void *item=(void *)a;
    ++r->tested;
    CHECK(lks_managed_order_insert(r->order,item,&h)==LKS_STATUS_REENTRANT && !h);
    CHECK(lks_managed_order_locate(r->order,a,&p,&e,&s)==LKS_STATUS_REENTRANT && !p && !e && !s);
    CHECK(lks_managed_order_remove(r->order,r->handle,&item)==LKS_STATUS_REENTRANT && !item);
    CHECK(lks_managed_order_compare(r->order,r->handle,r->handle,&result)==LKS_STATUS_REENTRANT && !result);
    CHECK(lks_managed_order_cursor_create(r->order,0,&c)==LKS_STATUS_REENTRANT && !c);
    CHECK(lks_order_cursor_next(r->cursor,&h)==LKS_STATUS_REENTRANT && !h);
    CHECK(!lks_managed_order_first(r->order) && !lks_managed_order_last(r->order) && !lks_managed_order_size(r->order));
    CHECK(!lks_order_next(r->handle) && !lks_order_previous(r->handle) && !lks_order_item(r->handle));
    lks_managed_order_destroy(r->order); /* busy destruction deliberately rejected */
    return compare(a,b,NULL);
}
static void managed_interactions(void)
{
    Item items[513]; const LksOrderHandle *seq[513],*h,*p,*e,*s;
    LksComparator d={compare,NULL};LksManagedOrder *o=lks_managed_order_create(&d);
    LksOrderCursor *c;size_t i;uint32_t state=17;
    CHECK(o);for(i=0;i<513;++i){items[i].key=(int)(i%11);items[i].id=i;}
    for(i=0;i<513;++i) {
        size_t at=i;while(at && ((Item *)seq[at-1]->item)->key>items[i].key)--at;
        CHECK(lks_managed_order_insert(o,&items[i],&h)==LKS_STATUS_OK);
        memmove(seq+at+1,seq+at,(i-at)*sizeof(*seq));seq[at]=h;
    }
    CHECK(lks_managed_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
    CHECK(lks_managed_order_locate(o,&items[0],&p,&e,&s)==LKS_STATUS_OK);
    for(i=0;i<513;++i)CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==seq[i]);
    CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && !h);lks_order_cursor_destroy(c);
    CHECK(lks_managed_order_cursor_create(o,1,&c)==LKS_STATUS_OK);
    for(i=513;i>0;--i)CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_OK && h==seq[i-1]);
    lks_order_cursor_destroy(c);
    for(i=0;i<5000;++i) {
        size_t at=rng(&state)%513,pos=512,before=calls;Item *item=(Item *)seq[at]->item;void *removed;
        CHECK(lks_managed_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
        CHECK(lks_managed_order_remove(o,seq[at],&removed)==LKS_STATUS_OK && removed==item && calls==before);
        CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED);lks_order_cursor_destroy(c);
        memmove(seq+at,seq+at+1,(512-at)*sizeof(*seq));item->key=(int)(rng(&state)%17u);
        while(pos && ((Item *)seq[pos-1]->item)->key>item->key)--pos;
        CHECK(lks_managed_order_cursor_create(o,0,&c)==LKS_STATUS_OK);
        CHECK(lks_managed_order_insert(o,item,&h)==LKS_STATUS_OK);
        memmove(seq+pos+1,seq+pos,(512-pos)*sizeof(*seq));seq[pos]=h;
        CHECK(lks_order_cursor_next(c,&h)==LKS_STATUS_INVALIDATED);lks_order_cursor_destroy(c);
        if(i%31==0){managed_check(o,seq,513);locate_check(o,seq,513,item->key);}
    }
    managed_check(o,seq,513);lks_managed_order_destroy(o);
    { Reentry r={NULL,NULL,NULL,0};LksComparator guard={reentry_compare,&r};Item a={1,0},b={2,1};
      r.order=lks_managed_order_create(&guard);CHECK(r.order);
      CHECK(lks_managed_order_insert(r.order,&a,&r.handle)==LKS_STATUS_OK);
      CHECK(lks_managed_order_cursor_create(r.order,0,&r.cursor)==LKS_STATUS_OK);
      CHECK(lks_managed_order_insert(r.order,&b,&h)==LKS_STATUS_OK && r.tested);
      CHECK(lks_order_internal_valid(r.order->core) && lks_managed_order_size(r.order)==2);
      lks_order_cursor_destroy(r.cursor);lks_managed_order_destroy(r.order); }
    puts("managed cursor, 5000 remove/change/reinsert cycles, comparator reentry PASS");
}
int main(void)
{
    unsigned p;CHECK(lks_alloc_stats_reset()==0);
    cursor_tests();move_boundaries();move_oom();
    explicit_random(17);explicit_random(UINT32_C(1779033703));explicit_random(UINT32_C(3144134277));long_moves();
    for(p=0;p<7;++p)managed_campaign(p,p==3?100001:4096);
    managed_oom();
    managed_interactions();
    printf("move operations=%zu cross=%zu noops=%zu max_assignments=%zu max_arrays=%zu max_index=%zu splits=%zu repairs=%zu\n",move_ops,cross_moves,noops,max_assign,max_arrays,max_index,move_splits,move_repairs);
    printf("managed max_insert_calls=%zu max_locate_calls=%zu\n",max_insert_calls,max_locate_calls);
    CHECK(!lks_alloc_stats_get().live_bytes && !lks_alloc_stats_get().live_blocks);
    puts("V4 Preview.2 PASS; zero live allocations");return 0;
}
