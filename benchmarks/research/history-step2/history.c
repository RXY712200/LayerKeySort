#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
/* Public API experiment with an independent rotation/byte/wire oracle.
 * Whole-operation timing includes callbacks, never oracle verification. */
#include <layerkeysort.h>
#include "probe.h"
#include "history_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include "host.h"

typedef struct Context {
    uint64_t *items; size_t n, shift, calls, useful, resolved;
    unsigned char scratch[2048]; const char *payload;
} Context;
static const char *mode, *payload;
static size_t scale, retention, domain_size;
static double started;
static ResearchMemory before;
static void require(int value,const char *what)
{if(!value){fprintf(stderr,"HISTORY_FAILURE %s\n",what);exit(2);}}
static double now(void){return history_host_now();}
static size_t working_set(void){return history_host_rss();}
static size_t length_for(uint64_t id,const char *kind)
{
    if(!strcmp(kind,"empty")||!strcmp(kind,"none"))return 0;
    if(!strcmp(kind,"one"))return 1;
    if(!strcmp(kind,"eight"))return 8;
    if(!strcmp(kind,"fixed256"))return 256;
    if(!strcmp(kind,"variable"))return(size_t)((id*17+3)%129);
    return(size_t)((id*37+1)%2049);
}
static unsigned char byte_for(uint64_t id,size_t i)
{return(unsigned char)((id*131+(uint64_t)i*29+17)&255);}
static LksStatus association(void *item,uint64_t ordinal,void *context,const void **data,size_t *size)
{
    Context *c=context;uint64_t id=*(const uint64_t *)item;size_t i;
    require(ordinal==c->calls&&id==(ordinal+c->shift)%c->n,"callback order/occurrence");
    ++c->calls;*size=length_for(id,c->payload);c->useful+=*size;
    for(i=0;i<*size;i++)c->scratch[i]=byte_for(id,i);
    *data=*size?c->scratch:NULL;return LKS_STATUS_OK;
}
static void check_bytes(const void *data,size_t n,uint64_t id,const char *kind)
{
    size_t i;const unsigned char *p=data;require(n==length_for(id,kind),"association length");
    require(n==0||p!=NULL,"association pointer");for(i=0;i<n;i++)require(p[i]==byte_for(id,i),"owned association bytes");
}
static LksStatus resolve(const void *data,size_t size,uint64_t ordinal,void *context,void **item)
{
    Context *c=context;size_t id=(size_t)((ordinal+c->shift)%c->n);
    require(ordinal==c->resolved++,"resolver sequence");check_bytes(data,size,id,c->payload);
    *item=&c->items[id];return LKS_STATUS_OK;
}
static void begin(void)
{
    if(history_host_available()<UINT64_C(2147483648)){fputs("SAFE_BUDGET_ABORT\n",stderr);exit(77);}
    before=research_memory();started=now();research_scope(1);
}
static void end(const char *phase,size_t index,const LksSnapshot *snapshot,size_t useful,size_t calls,size_t wire,size_t scratch)
{
    double elapsed=now()-started;ResearchMemory m;HistoryStorage h;
    research_scope(0);m=research_memory();h=history_storage(snapshot);
    printf("%s,%s,%zu,%zu,%zu,%s,%zu,%.9f,%zu,%zu,%zu,%zu,%zu",mode,payload,scale,retention,domain_size,phase,index,elapsed,calls,useful,wire,scratch,working_set());
    if(strcmp(mode,"production"))printf(",%zu,%zu,%zu,%zu,%zu,%zu,%zu",m.allocations-before.allocations,m.reallocations-before.reallocations,m.frees-before.frees,m.live,m.bytes,m.peak,m.traffic-before.traffic);
    else printf(",NA,NA,NA,NA,NA,NA,NA");
    if(!strcmp(mode,"structural"))printf(",%zu,%zu,%zu,%zu",h.header,h.rows,h.capacity,h.domain);
    else printf(",NA,NA,NA,NA");
    printf(",%" PRIu64 "\n",snapshot?(uint64_t)lks_snapshot_key_length(snapshot)*scale:UINT64_C(0));
}
static void verify_snapshot(const LksSnapshot *s,const unsigned char *domain,Context *c)
{
    size_t row,size;const void *data;uint64_t digest=UINT64_C(1469598103934665603);
    require(lks_snapshot_count(s)==c->n,"snapshot count");
    require(lks_snapshot_namespace(s,&data,&size)==LKS_STATUS_OK&&size==domain_size&&!memcmp(data,domain,size),"namespace copy");
    for(row=0;row<c->n;row++){
        uint64_t id=(row+c->shift)%c->n;
        require(lks_snapshot_association(s,row,&data,&size)==LKS_STATUS_OK,"association view");
        check_bytes(data,size,id,c->payload);digest=(digest^id)*UINT64_C(1099511628211);
    }
    fprintf(stderr,"CHECK history shift=%zu count=%zu digest=%016" PRIx64 " bytes_verified=true\n",c->shift,c->n,digest);
}
static void be(unsigned char *p,uint64_t v,size_t n)
{while(n){p[--n]=(unsigned char)(v&255);v>>=8;}}
static void wire_oracle(const unsigned char *wire,size_t size,const unsigned char *domain,Context *c)
{
    /* Build expected fields independently; no Full decoder predicts bytes. */
    unsigned char header[24]={0};size_t row,at=24,i;
    memcpy(header,"LKS4SNP1",8);be(header+12,domain_size,4);be(header+16,c->n,8);
    require(size>=24+domain_size&&!memcmp(wire,header,24),"wire header");
    require(!memcmp(wire+at,domain,domain_size),"wire domain");at+=domain_size;
    for(row=0;row<c->n;row++){
        uint64_t id=(row+c->shift)%c->n;size_t len=length_for(id,c->payload);unsigned char field[8];be(field,len,8);
        require(size-at>=8&&!memcmp(wire+at,field,8),"wire length field");at+=8;
        require(size-at>=len,"wire bounds");for(i=0;i<len;i++)require(wire[at+i]==byte_for(id,i),"wire payload");at+=len;
    }
    require(at==size,"canonical wire length");fprintf(stderr,"WIRE independent_exact_bytes=%zu\n",size);
}
static void verify_order(const LksOrder *o,Context *c,const LksOrderHandle **original)
{
    size_t i;const LksOrderHandle *h=lks_order_first(o);
    require(lks_order_size(o)==c->n,"restored count");
    for(i=0;i<c->n;i++){
        size_t id=(i+c->shift)%c->n;require(h&&lks_order_item(h)==&c->items[id],"restored occurrence");
        if(original)require(h!=original[id],"fresh live handle");
        h=lks_order_next(h);
    }
    require(!h,"restored forward end");h=lks_order_last(o);
    for(i=c->n;i>0;i--){require(h&&lks_order_item(h)==&c->items[(i-1+c->shift)%c->n],"restored reverse");h=lks_order_previous(h);}
    require(!h,"restored reverse end");fprintf(stderr,"RESTORE exact_forward_reverse=%zu\n",c->n);
}
static size_t number(const char *s)
{char *e;unsigned long long n;errno=0;n=strtoull(s,&e,10);require(!errno&&!*e&&n<=SIZE_MAX,"numeric argument");return(size_t)n;}
int main(int argc,char **argv)
{
    Context c={0};LksOrder *order,*restored;LksSnapshot **history,*loaded;size_t *shifts,i,j,wire_size;
    const LksOrderHandle **handles;unsigned char *domain,*wire;
    require(argc==5,"usage: residents retention payload namespace_bytes");scale=number(argv[1]);retention=number(argv[2]);payload=argv[3];domain_size=number(argv[4]);
    require(scale>=64&&scale<=10000000&&retention>=2&&retention<=32&&domain_size>=1&&domain_size<=4096,"argument bounds");
    require(!strcmp(payload,"none")||!strcmp(payload,"empty")||!strcmp(payload,"one")||!strcmp(payload,"eight")||!strcmp(payload,"fixed256")||!strcmp(payload,"variable")||!strcmp(payload,"wide"),"payload kind");
#ifdef RESEARCH_PRIVATE
    mode="structural";
#elif defined(RESEARCH_WRAP)
    mode="allocation";
#else
    mode="production";
#endif
    c.n=scale;c.payload=payload;c.items=malloc(scale*sizeof(*c.items));handles=malloc(scale*sizeof(*handles));
    history=calloc(retention,sizeof(*history));shifts=calloc(retention,sizeof(*shifts));domain=malloc(domain_size);
    require(c.items&&handles&&history&&shifts&&domain,"application fixtures");for(i=0;i<domain_size;i++)domain[i]=(unsigned char)(i*19+7);
    puts("mode,payload,residents,retention,namespace,phase,index,wall_seconds,callbacks,useful_bytes,wire_bytes,temporary_pointer_bytes,working_set_bytes,allocations,reallocations,frees,live_allocations,live_requested_bytes,peak_requested_bytes,requested_traffic_bytes,header_bytes,row_bytes,association_capacity,namespace_bytes,all_key_bytes");
    begin();order=lks_order_create();require(order!=NULL,"create");
    for(i=0;i<scale;i++){c.items[i]=i;require(lks_order_insert_back(order,&c.items[i],&handles[i])==LKS_STATUS_OK,"source build");}
    end("source_build",0,NULL,0,0,0,0);
    /* Callback-only replay is a separate diagnostic, not a subtracted estimate. */
    c.calls=c.useful=0;begin();
    if(strcmp(payload,"none")){
        LksSnapshotAssociationFn volatile callback=association;
        for(i=0;i<scale;i++){const void *data;size_t size;require(callback(&c.items[i],i,&c,&data,&size)==LKS_STATUS_OK,"callback replay");}
    }
    end("callback_replay",0,NULL,c.useful,c.calls,0,0);
    for(i=0;i<retention;i++){
        LksSnapshotOptions opt={domain,domain_size,!strcmp(payload,"none")?NULL:association,&c};
        if(i==2){begin();for(j=0;j<scale/10;j++)require(lks_order_move_back(order,lks_order_first(order))==LKS_STATUS_OK,"substantial live mutation");c.shift=scale/10;end("live_mutation",0,NULL,0,0,0,0);}
        shifts[i]=c.shift;c.calls=c.useful=0;begin();require(lks_order_snapshot_capture(order,&opt,&history[i])==LKS_STATUS_OK,"capture");
        end("capture",i,history[i],c.useful,c.calls,lks_snapshot_serialized_size(history[i]),0);
        require(c.calls==(!strcmp(payload,"none")?0:scale)&&lks_order_snapshot_is_current(order,history[i]),"callback count/currentness");verify_snapshot(history[i],domain,&c);
    }
    {
        static const char hex[]="0123456789abcdef";
        size_t key_size=lks_snapshot_key_length(history[retention-1])+1,probes=scale<4096?scale:4096;
        char *key=malloc(key_size),*expected=malloc(key_size);require(key&&expected,"key buffers");
        memcpy(expected,"LS1.",4);for(i=0;i<domain_size;i++){expected[4+2*i]=hex[domain[i]>>4];expected[5+2*i]=hex[domain[i]&15];}
        expected[4+2*domain_size]='.';expected[key_size-1]=0;
        begin();for(i=0;i<probes;i++){
            uint64_t ordinal=(uint64_t)i*scale/probes;
            require(lks_snapshot_key_format(history[retention-1],(size_t)ordinal,key,key_size)==LKS_STATUS_OK,"LS1 format");
            for(j=0;j<16;j++)expected[5+2*domain_size+j]=hex[(ordinal>>(60-4*j))&15];
            require(!memcmp(key,expected,key_size)&&lks_snapshot_key_validate(key)==LKS_STATUS_OK,"independent LS1 bytes");
        }
        end("historical_keys",0,history[retention-1],0,0,0,0);free(key);free(expected);
    }
    wire_size=lks_snapshot_serialized_size(history[retention-1]);wire=malloc(wire_size+1);require(wire!=NULL,"caller output buffer");
    begin();require(lks_snapshot_serialize(history[retention-1],wire,wire_size)==LKS_STATUS_OK,"serialize");end("serialize",0,history[retention-1],c.useful,0,wire_size,0);wire_oracle(wire,wire_size,domain,&c);
    begin();require(lks_snapshot_deserialize(wire,wire_size,&loaded)==LKS_STATUS_OK,"deserialize");end("deserialize",0,loaded,c.useful,0,wire_size,0);
    require(!lks_order_snapshot_is_current(order,loaded),"loaded provenance");verify_snapshot(loaded,domain,&c);
    for(i=0;i<3;i++){
        c.resolved=0;begin();require(lks_snapshot_restore_order(loaded,resolve,&c,&restored)==LKS_STATUS_OK,"restore");end("restore",i,NULL,0,c.resolved,0,scale*sizeof(void *));
        require(c.resolved==scale,"resolver count");verify_order(restored,&c,handles);
        begin();lks_order_destroy(restored);end("restore_destroy",i,NULL,0,0,0,0);
    }
    {
        LksSnapshot *bad=NULL;unsigned char save=wire[8];wire[8]=1;
        require(lks_snapshot_deserialize(wire,wire_size,&bad)==LKS_STATUS_INVALID_ARGUMENT&&!bad,"malformed flags");wire[8]=save;
        require(lks_snapshot_deserialize(wire,wire_size-1,&bad)==LKS_STATUS_INVALID_ARGUMENT&&!bad,"truncation");
        wire[wire_size]=0;require(lks_snapshot_deserialize(wire,wire_size+1,&bad)==LKS_STATUS_INVALID_ARGUMENT&&!bad,"trailing bytes");
    }
    memset(wire,0xa5,wire_size);free(wire);wire=NULL;
    begin();for(i=0;i<retention;i+=2){lks_snapshot_destroy(history[i]);history[i]=NULL;}end("partial_history_destroy",0,NULL,0,0,0,0);
    begin();lks_order_destroy(order);end("source_destroy",0,NULL,0,0,0,0);
    free(c.items);c.items=NULL;
    for(i=1;i<retention;i+=2){c.shift=shifts[i];verify_snapshot(history[i],domain,&c);}
    begin();for(i=1;i<retention;i+=2)lks_snapshot_destroy(history[i]);end("history_destroy",0,NULL,0,0,0,0);
    c.shift=shifts[retention-1];verify_snapshot(loaded,domain,&c);
    begin();lks_snapshot_destroy(loaded);end("loaded_destroy",0,NULL,0,0,0,0);
    require(!research_memory().bytes&&!research_memory().live,"Full teardown");
    free(c.items);free(handles);free(history);free(shifts);free(domain);free(wire);
    fprintf(stderr,"HISTORY_COMPLETE mode=%s count=%zu retention=%zu payload=%s requested_cleanup=%s final_working_set=%zu\n",mode,scale,retention,payload,strcmp(mode,"production")?"zero":"unavailable",working_set());return 0;
}
