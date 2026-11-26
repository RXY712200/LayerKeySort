#define _POSIX_C_SOURCE 200809L
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define OK(x) CHECK((x)==LKS_STATUS_OK)
#ifndef FLAT
#define FLAT 0
#endif
static double now(void)
{
#ifdef _WIN32
    LARGE_INTEGER t, f;
    CHECK(QueryPerformanceCounter(&t) && QueryPerformanceFrequency(&f));
    return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;
    CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);
    return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
typedef struct Item { size_t id; } Item;
typedef struct Context { size_t width, epoch, n; Item *items; unsigned char scratch[4096]; } Context;
static size_t assoc_size(const Context *c, size_t row)
{ return c->width == 4096 ? (row%31 ? 32u : 4096u) : c->width; }
static unsigned char value(size_t id, size_t row, size_t epoch, size_t offset)
{ return (unsigned char)(id*17u + row*3u + epoch*37u + offset*7u); }
static LksStatus association(void *item, uint64_t row, void *context, const void **out, size_t *size)
{
    Context *c=context;
    size_t id=((Item *)item)->id;
    *size=assoc_size(c,(size_t)row);
    for (size_t i=0;i<*size;++i) c->scratch[i]=value(id,(size_t)row,c->epoch,i);
    *out=c->scratch;
    return LKS_STATUS_OK;
}
static LksStatus resolve(const void *p, size_t n, uint64_t ordinal, void *context, void **out)
{
    Context *c=context;
    size_t row=(size_t)ordinal, id=(row+c->epoch)%c->n;
    CHECK(n==assoc_size(c,row));
    for(size_t i=0;i<n;++i) CHECK(((const unsigned char *)p)[i]==value(id,row,c->epoch,i));
    *out=c->items+id;
    return LKS_STATUS_OK;
}
static uint64_t load(const unsigned char *p,size_t n)
{ uint64_t x=0;for(size_t i=0;i<n;++i)x=x*256+p[i]; return x; }

#if FLAT
static void store(unsigned char *p, uint64_t x, size_t n)
{ for(size_t i=n;i;--i){p[i-1]=(unsigned char)x;x>>=8;} }
typedef struct Ref { struct Ref *next; Item *item; } Ref;
typedef struct Order { Ref *first, *last; size_t n; } Order;
typedef struct Snapshot { size_t n,d,used,cap; size_t *ends; unsigned char *domain,*bytes; } Snapshot;
static Order *create(void){Order *o=lks_alloc(sizeof(*o));CHECK(o);*o=(Order){0};return o;}
static void append(Order *o, Item *x)
{
    Ref *r=lks_alloc(sizeof(*r)); CHECK(r); *r=(Ref){NULL,x};
    if(o->last) o->last->next=r; else o->first=r;
    o->last=r; ++o->n;
}
static void move(Order *o)
{
    if(o->n>1){Ref *r=o->first; o->first=r->next; r->next=NULL; o->last->next=r; o->last=r;}
}
static void destroy(Order *o)
{ Ref *r=o->first;while(r){Ref *next=r->next;lks_free(r);r=next;}lks_free(o); }
static Snapshot *snap_new(size_t n,const unsigned char *domain,size_t d)
{
    Snapshot *s=lks_alloc(sizeof(*s)+n*sizeof(size_t)+d); CHECK(s);
    *s=(Snapshot){.n=n,.d=d,.ends=(size_t *)(s+1)};
    s->domain=(unsigned char *)(s->ends+n);memcpy(s->domain,domain,d);return s;
}
static Snapshot *capture(Order *o,const unsigned char *domain,size_t d,Context *c)
{
    Snapshot *s=snap_new(o->n,domain,d);
    size_t i=0;
    for(Ref *r=o->first;r;r=r->next){
        const void *data;size_t n;
        OK(association(r->item,(uint64_t)i,c,&data,&n));
        if(s->used+n>s->cap){
            size_t cap=s->cap?s->cap:64;
            while(cap<s->used+n)cap*=2;
            s->bytes=lks_realloc(s->bytes,cap);CHECK(s->bytes);s->cap=cap;
        }
        if(n)memcpy(s->bytes+s->used,data,n);
        s->used+=n;s->ends[i++]=s->used;
    }
    if(s->used!=s->cap){s->bytes=lks_realloc(s->bytes,s->used);CHECK(s->bytes);s->cap=s->used;}
    return s;
}
static void drop(Snapshot *s){lks_free(s->bytes);lks_free(s);}
static size_t length(const Snapshot *s){return 24+s->d+8*s->n+s->used;}
static void encode(const Snapshot *s,unsigned char *p,size_t n)
{
    CHECK(n==length(s)); memcpy(p,"LKS4SNP1",8);memset(p+8,0,4);store(p+12,s->d,4);store(p+16,s->n,8);
    memcpy(p+24,s->domain,s->d);p+=24+s->d;
    size_t start=0;
    for(size_t i=0;i<s->n;++i){size_t size=s->ends[i]-start;store(p,size,8);p+=8;
        if(size)memcpy(p,s->bytes+start,size);
        p+=size;start+=size;}
}
static Snapshot *decode(const unsigned char *p,size_t total)
{
    CHECK(total>=24&&!memcmp(p,"LKS4SNP1",8)&&!load(p+8,4));
    size_t n=(size_t)load(p+16,8),d=(size_t)load(p+12,4);
    CHECK(d&&d<=total-24&&n<=(total-24-d)/8);
    Snapshot *s=snap_new(n,p+24,d);
    size_t at=24+d;s->used=total-at-n*8;s->cap=s->used;
    if(s->used){s->bytes=lks_alloc(s->used);CHECK(s->bytes);}
    size_t copied=0;
    for(size_t i=0;i<n;++i){CHECK(total-at>=8);size_t size=(size_t)load(p+at,8);at+=8;
        CHECK(size<=total-at&&size<=s->used-copied);if(size)memcpy(s->bytes+copied,p+at,size);
        at+=size;copied+=size;s->ends[i]=copied;}
    CHECK(at==total&&copied==s->used);return s;
}
static Order *restore(Snapshot *s,Context *c)
{
    Order *o=create();size_t at=0;
    for(size_t i=0;i<s->n;++i){void *item;size_t n=s->ends[i]-at;
        OK(resolve(n?s->bytes+at:NULL,n,(uint64_t)i,c,&item));append(o,item);at+=n;}
    return o;
}
__attribute__((noipa)) static void key(const Snapshot *s,size_t row,char *buffer,size_t cap)
{
    static const char hex[]="0123456789abcdef";
    CHECK(cap>21+2*s->d);
    memcpy(buffer,"LS1.",4);size_t at=4;
    for(size_t i=0;i<s->d;++i){buffer[at++]=hex[s->domain[i]>>4];buffer[at++]=hex[s->domain[i]&15];}
    buffer[at++]='.';
    for(size_t i=0;i<16;++i)buffer[at++]=hex[((uint64_t)row>>(60-4*i))&15];
    buffer[at]=0;
}
static Item *first(Order *o){return o->first->item;}
#else
typedef LksOrder Order;
typedef LksSnapshot Snapshot;
#define create lks_order_create
#define destroy lks_order_destroy
#define drop lks_snapshot_destroy
#define length lks_snapshot_serialized_size
static void append(Order *o,Item *x){const LksOrderHandle *h;OK(lks_order_insert_back(o,x,&h));}
static void move(Order *o){OK(lks_order_move_back(o,lks_order_first(o)));}
static Snapshot *capture(Order *o,const unsigned char *d,size_t size,Context *c)
{Snapshot *s=NULL;LksSnapshotOptions options={d,size,association,c};OK(lks_order_snapshot_capture(o,&options,&s));return s;}
static void encode(const Snapshot *s,unsigned char *p,size_t n){OK(lks_snapshot_serialize(s,p,n));}
static Snapshot *decode(const unsigned char *p,size_t n)
{Snapshot *s=NULL;OK(lks_snapshot_deserialize(p,n,&s));return s;}
static Order *restore(Snapshot *s,Context *c)
{Order *o=NULL;OK(lks_snapshot_restore_order(s,resolve,c,&o));return o;}
static void key(const Snapshot *s,size_t row,char *p,size_t n)
{OK(lks_snapshot_key_format(s,row,p,n));}
static Item *first(Order *o){return lks_order_item(lks_order_first(o));}
#endif

int main(int argc,char **argv)
{
    CHECK(argc==5);
    size_t n=(size_t)strtoul(argv[1],NULL,10), repeats=(size_t)strtoul(argv[2],NULL,10),
           width=(size_t)strtoul(argv[3],NULL,10), d=(size_t)strtoul(argv[4],NULL,10);
    CHECK(n>=16&&repeats<=16&&width<=4096&&d&&d<=4096);
    unsigned char domain[4096];char keybuf[8214];
    Item *items=malloc(n*sizeof(*items));CHECK(items);
    Snapshot *history[16]={0};
    Context ctx={.width=width,.n=n,.items=items};
    double start=now();
    Order *order=create();CHECK(order);
    for(size_t i=0;i<n;++i){items[i].id=i;append(order,&items[i]);}
    double build=now()-start, edit=0, cap=0, format=0, wire=0, parse=0, reentry=0, cleanup=0;
    size_t bytes=0;
    for(size_t j=0;j<(repeats?repeats:16);++j){
        start=now();move(order);CHECK(first(order)->id==j+1);edit+=now()-start;
        if(repeats){
            ctx.epoch=j+1;memset(domain,(int)j,d);
            start=now();history[j]=capture(order,domain,d,&ctx);cap+=now()-start;
        }
    }
    size_t retained=lks_alloc_stats_get().live_bytes;
    destroy(order);free(items);ctx.items=NULL;
    items=malloc(n*sizeof(*items));CHECK(items);ctx.items=items;
    for(size_t i=0;i<n;++i) items[i].id=i;
    for(size_t j=0;j<repeats;++j){
        Snapshot *s=history[j];
        start=now();
        for(size_t i=0;i<n;++i){key(s,i,keybuf,sizeof(keybuf));CHECK(keybuf[4]==(j==0?'0':"0123456789abcdef"[j>>4]));}
        format+=now()-start;
        static const char hex[]="0123456789abcdef";
        CHECK(!memcmp(keybuf,"LS1.",4));
        for(size_t k=0;k<d;++k)CHECK(keybuf[4+2*k]==hex[j>>4]&&keybuf[5+2*k]==hex[j&15]);
        CHECK(keybuf[4+2*d]=='.');
        for(size_t k=0;k<16;++k)CHECK(keybuf[5+2*d+k]==hex[((uint64_t)(n-1)>>(60-4*k))&15]);
        CHECK(keybuf[21+2*d]==0);
        size_t len=length(s); bytes+=len;
        unsigned char *buf=malloc(len);CHECK(buf);
        start=now();encode(s,buf,len);wire+=now()-start;
        CHECK(!memcmp(buf,"LKS4SNP1",8)&&!load(buf+8,4)&&load(buf+12,4)==d&&load(buf+16,8)==n);
        for(size_t k=0;k<d;++k)CHECK(buf[24+k]==j);
        size_t at=24+d;
        for(size_t i=0;i<n;++i){
            size_t m=assoc_size(&ctx,i);
            CHECK(at+8<=len&&load(buf+at,8)==m);
            at+=8;CHECK(m<=len-at);
            for(size_t k=0;k<m;++k)CHECK(buf[at+k]==value((i+j+1)%n,i,j+1,k));
            at+=m;
        }
        CHECK(at==len);
        start=now();Snapshot *read=decode(buf,len);parse+=now()-start;
        free(buf);ctx.epoch=j+1;
        start=now();Order *next=restore(read,&ctx);reentry+=now()-start;
        CHECK(first(next)->id==j+1);move(next);CHECK(first(next)->id==j+2);
        start=now();destroy(next);drop(read);drop(s);cleanup+=now()-start;
    }
    free(items);
    LksAllocStats stats=lks_alloc_stats_get();
    CHECK(!stats.live_bytes&&!stats.live_blocks);
    printf("%zu,%zu,%zu,%zu,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%zu,%zu,%zu,%zu\n",
           n,repeats,width,d,build,edit,cap,format,wire,parse,reentry,cleanup,bytes,retained,
           stats.peak_live_bytes,stats.alloc_calls+stats.realloc_calls);
    return 0;
}
