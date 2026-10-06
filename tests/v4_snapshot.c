#include "lks_legacy_internal.h"
#include "layerkeysort.h"
#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"snapshot line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static unsigned char domain[] = {0, 255};
static LksSnapshotOptions options = {domain, sizeof(domain), NULL, NULL};
static size_t callbacks, comparator_calls, fail_row = SIZE_MAX, payload = sizeof(size_t);
static LksOrder *callback_source;
static LksManagedOrder *callback_managed;
static unsigned char scratch[4096];
static int compare(const void *a, const void *b, void *context)
{ (void)context; ++comparator_calls; return (*(const size_t *)a > *(const size_t *)b)-(*(const size_t *)a < *(const size_t *)b); }
static LksStatus associate(void *item, uint64_t ordinal, void *context,
    const void **data, size_t *size)
{
    (void)context; ++callbacks;
    if (ordinal == fail_row) return LKS_STATUS_NOT_FOUND;
    *size = payload;
    if (payload == SIZE_MAX) { *data = NULL; return LKS_STATUS_OK; }
    memset(scratch, 0xa5, sizeof(scratch)); memcpy(scratch, item, sizeof(size_t));
    *data = scratch;
    if (callback_source) {
        LksSnapshot *s = NULL; const LksOrderHandle *h = NULL; LksOrderCursor *c = NULL;
        CHECK(!lks_order_first(callback_source) && !lks_order_size(callback_source));
        CHECK(lks_order_insert_back(callback_source, item, &h) == LKS_STATUS_REENTRANT);
        CHECK(lks_order_cursor_create(callback_source, 0, &c) == LKS_STATUS_REENTRANT);
        CHECK(lks_order_snapshot_capture(callback_source, &options, &s) == LKS_STATUS_REENTRANT && !s);
        lks_order_destroy(callback_source);
    }
    if (callback_managed) {
        LksSnapshot *s = NULL; const LksOrderHandle *h = NULL;
        CHECK(!lks_managed_order_first(callback_managed));
        CHECK(lks_managed_order_insert(callback_managed, item, &h) == LKS_STATUS_REENTRANT);
        CHECK(lks_managed_order_snapshot_capture(callback_managed, &options, &s) == LKS_STATUS_REENTRANT);
        lks_managed_order_destroy(callback_managed);
    }
    return LKS_STATUS_OK;
}
static LksStatus resolve(const void *data, size_t size, uint64_t ordinal,
    void *context, void **item)
{
    size_t id = (size_t)ordinal; (void)size;
    if (data && size >= sizeof(id)) memcpy(&id, data, sizeof(id));
    *item = (size_t *)context+id; return LKS_STATUS_OK;
}
static LksOrder *build(size_t n, size_t *items)
{
    LksOrder *o = NULL; void **p = n ? (void **)malloc(n*sizeof(*p)) : NULL; size_t i;
    for (i = 0; i < n; ++i) { items[i] = i; p[i] = &items[i]; }
    CHECK(lks_order_bulk_build(p, n, &o) == LKS_STATUS_OK); free(p); return o;
}
static void zero_live(void)
{ CHECK(!lks_alloc_stats_get().live_blocks && !lks_alloc_stats_get().live_bytes); }
static void keys(void)
{
    static const struct { const char *key; uint64_t ordinal; } vectors[] = {
        {"LS1.00.0000000000000000",0}, {"LS1.ff.0000000000000001",1},
        {"LS1.00ff.00000000000000ff",255}, {"LS1.00ff.0000000000000100",256},
        {"LS1.00112233445566778899aabbccddeeff.00000000ffffffff",UINT32_MAX},
        {"LS1.00.fffffffffffffffe",UINT64_MAX-1}, {"LS1.00.ffffffffffffffff",UINT64_MAX}
    };
    static const char *bad[] = {"","LS2.00.0000000000000000","LS1..0000000000000000",
        "LS1.0.0000000000000000","LS1.FF.0000000000000000","LS1.00.000000000000000A",
        "LS1.00.0000000000000000.","LS1.00.00000000000000000","LS1.00 00.0000000000000000"};
    size_t i, j; int c; char text[100];
    for (i = 0; i < sizeof(vectors)/sizeof(*vectors); ++i) {
        size_t length = strlen(vectors[i].key), split = length-17, ns = (split-4)/2;
        unsigned char bytes[16]; LksSnapshot synthetic;
        for (j = 0; j < ns; ++j) { unsigned v; CHECK(sscanf(vectors[i].key+4+2*j,"%2x",&v)==1); bytes[j]=(unsigned char)v; }
        memset(&synthetic,0,sizeof(synthetic)); synthetic.namespace_data=bytes; synthetic.namespace_size=ns;
        CHECK(lks_snapshot_key_validate(vectors[i].key)==LKS_STATUS_OK);
        if (vectors[i].ordinal < SIZE_MAX) {
            synthetic.count=(size_t)vectors[i].ordinal+1;
            CHECK(lks_snapshot_key_format(&synthetic,(size_t)vectors[i].ordinal,text,sizeof(text))==LKS_STATUS_OK);
            CHECK(!strcmp(text,vectors[i].key) && lks_snapshot_key_length(&synthetic)==length);
        }
        for (j = 0; j < length; ++j) {
            memcpy(text,vectors[i].key,j); text[j]=0;
            CHECK(lks_snapshot_key_validate(text)==LKS_STATUS_INVALID_ARGUMENT);
            strcpy(text,vectors[i].key); text[j]=' ';
            CHECK(lks_snapshot_key_validate(text)==LKS_STATUS_INVALID_ARGUMENT);
        }
    }
    for(i=0;i<sizeof(bad)/sizeof(*bad);++i) CHECK(lks_snapshot_key_validate(bad[i])==LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_snapshot_key_compare(vectors[2].key,vectors[3].key,&c)==LKS_STATUS_OK && c<0 && strcmp(vectors[2].key,vectors[3].key)<0);
    CHECK(lks_snapshot_key_compare(vectors[0].key,vectors[1].key,&c)==LKS_STATUS_DOMAIN_MISMATCH && c==0);
    CHECK(lks_snapshot_key_compare(vectors[5].key,vectors[6].key,&c)==LKS_STATUS_OK && c<0 && strcmp(vectors[5].key,vectors[6].key)<0);
    for(i=0;i<1000;++i) {
        LksSnapshot synthetic; char a[100],b[100]; memset(&synthetic,0,sizeof(synthetic));
        synthetic.namespace_data=domain; synthetic.namespace_size=2; synthetic.count=1001;
        CHECK(lks_snapshot_key_format(&synthetic,i,a,sizeof(a))==LKS_STATUS_OK);
        CHECK(lks_snapshot_key_format(&synthetic,i+1,b,sizeof(b))==LKS_STATUS_OK);
        CHECK(lks_snapshot_key_compare(a,b,&c)==LKS_STATUS_OK && c<0 && strcmp(a,b)<0);
    }
}
static LksStatus golden_associate(void *item,uint64_t ordinal,void *context,
    const void **data,size_t *size)
{
    (void)item; (void)context;
    *data=ordinal==0 ? domain : NULL; *size=ordinal==0 ? 2 : 0;
    return LKS_STATUS_OK;
}
static void wire(void)
{
    /* Exact reviewable wire vectors: empty, empty row, binary and multiple rows. */
    static const char *hex[] = {
        "4c4b5334534e50310000000000000002000000000000000000ff",
        "4c4b5334534e50310000000000000002000000000000000100ff0000000000000000",
        "4c4b5334534e50310000000000000002000000000000000100ff000000000000000200ff",
        "4c4b5334534e50310000000000000002000000000000000200ff000000000000000200ff0000000000000000"
    };
    size_t v,i;
    for(v=0;v<4;++v) {
        unsigned char bytes[100],out[100],bad[101]; LksSnapshot *s=NULL,*t=NULL;
        size_t n=strlen(hex[v])/2;
        for(i=0;i<n;++i) { unsigned x; CHECK(sscanf(hex[v]+2*i,"%2x",&x)==1); bytes[i]=(unsigned char)x; }
        CHECK(lks_snapshot_deserialize(bytes,n,&s)==LKS_STATUS_OK);
        CHECK(lks_snapshot_serialized_size(s)==n && lks_snapshot_serialize(s,out,n)==LKS_STATUS_OK && !memcmp(bytes,out,n));
        { size_t items[2]; LksOrder *o=build(v==0?0:v==3?2:1,items);
          LksSnapshot *captured=NULL; LksSnapshotOptions opt=options;
          if(v>=2) opt.association=golden_associate;
          CHECK(lks_order_snapshot_capture(o,&opt,&captured)==LKS_STATUS_OK);
          CHECK(lks_snapshot_serialized_size(captured)==n && lks_snapshot_serialize(captured,out,n)==LKS_STATUS_OK && !memcmp(bytes,out,n));
          lks_order_destroy(o); lks_snapshot_destroy(captured);
        }
        memset(out,0xaa,sizeof(out)); CHECK(lks_snapshot_serialize(s,out,n-1)==LKS_STATUS_BUFFER_TOO_SMALL && out[0]==0xaa);
        for(i=0;i<n;++i) CHECK(lks_snapshot_deserialize(bytes,i,&t)==LKS_STATUS_INVALID_ARGUMENT && !t);
        memcpy(bad,bytes,n); bad[n]=0; CHECK(lks_snapshot_deserialize(bad,n+1,&t)==LKS_STATUS_INVALID_ARGUMENT && !t);
        for(i=0;i<24;++i) {
            memcpy(bad,bytes,n); bad[i]=255;
            CHECK(lks_snapshot_deserialize(bad,n,&t)==LKS_STATUS_INVALID_ARGUMENT && !t);
        }
        if(v>0) {
            memcpy(bad,bytes,n); memset(bad+26,255,8);
            CHECK(lks_snapshot_deserialize(bad,n,&t)==LKS_STATUS_INVALID_ARGUMENT && !t);
        }
        for(i=1;;++i) {
            size_t live=lks_alloc_stats_get().live_bytes; LksStatus status;
            lks_alloc_test_fail_on_attempt(i); status=lks_snapshot_deserialize(bytes,n,&t); lks_alloc_test_disable_failure();
            if(status==LKS_STATUS_OK) { lks_snapshot_destroy(t); break; }
            CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !t && lks_alloc_stats_get().live_bytes==live);
        }
        lks_snapshot_destroy(s);
    }
    zero_live();
}
static void capture_restore(size_t n)
{
    size_t *items=(size_t *)malloc((n+1)*sizeof(*items)), i;
    LksOrder *o=build(n,items),*r=NULL; LksSnapshot *s=NULL,*t=NULL;
    LksSnapshotOptions opt=options; LksOrderCursor *cursor=NULL; const LksOrderHandle *h=NULL;
    size_t live; uint64_t revision=o->revision; unsigned char *blob,*again; size_t bytes;
    opt.association=associate; payload=sizeof(size_t); callbacks=0;
    CHECK(lks_order_cursor_create(o,0,&cursor)==LKS_STATUS_OK);
    for(i=1;;++i) {
        LksStatus status; live=lks_alloc_stats_get().live_bytes;
        lks_alloc_test_fail_on_attempt(i); status=lks_order_snapshot_capture(o,&opt,&s); lks_alloc_test_disable_failure();
        CHECK(o->revision==revision && lks_order_internal_valid(o));
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !s && !o->source_marker && lks_alloc_stats_get().live_bytes==live);
    }
    CHECK(lks_order_snapshot_is_current(o,s));
    CHECK(lks_order_cursor_next(cursor,&h)==LKS_STATUS_OK);
    if(n) {
        CHECK(lks_order_move_front(o,lks_order_first(o))==LKS_STATUS_OK && lks_order_snapshot_is_current(o,s));
        lks_alloc_test_fail_on_attempt(1);
        CHECK(lks_order_insert_back(o,&items[n],&h)==LKS_STATUS_OUT_OF_MEMORY);
        lks_alloc_test_disable_failure(); CHECK(lks_order_snapshot_is_current(o,s));
    }
    for(i=1;;++i) {
        LksStatus status; live=lks_alloc_stats_get().live_bytes;
        lks_alloc_test_fail_on_attempt(i); status=lks_order_snapshot_capture(o,&opt,&t); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !t && lks_alloc_stats_get().live_bytes==live);
    }
    CHECK(t->marker==s->marker); lks_snapshot_destroy(t); t=NULL;
    if(n>1) CHECK(lks_order_move_back(o,lks_order_first(o))==LKS_STATUS_OK && !lks_order_snapshot_is_current(o,s));
    bytes=lks_snapshot_serialized_size(s); blob=(unsigned char *)malloc(bytes); again=(unsigned char *)malloc(bytes);
    CHECK(blob && again && lks_snapshot_serialize(s,blob,bytes)==LKS_STATUS_OK);
    CHECK(lks_snapshot_deserialize(blob,bytes,&t)==LKS_STATUS_OK && !lks_order_snapshot_is_current(o,t));
    CHECK(lks_snapshot_serialize(t,again,bytes)==LKS_STATUS_OK && !memcmp(blob,again,bytes));
    for(i=1;;++i) {
        LksStatus status; live=lks_alloc_stats_get().live_bytes;
        lks_alloc_test_fail_on_attempt(i); status=lks_snapshot_restore_order(t,resolve,items,&r); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !r && lks_alloc_stats_get().live_bytes==live);
    }
    CHECK(lks_order_internal_valid(r) && lks_order_size(r)==n && !lks_order_snapshot_is_current(r,s));
    for(h=lks_order_first(r),i=0;h;h=lks_order_next(h),++i) CHECK(lks_order_item(h)==&items[i]);
    CHECK(i==n);
    for(h=lks_order_last(r),i=n;h;h=lks_order_previous(h)) { CHECK(i>0); --i; CHECK(lks_order_item(h)==&items[i]); }
    CHECK(!i);
    { LksSnapshot *u=NULL; CHECK(lks_order_snapshot_capture(r,&opt,&u)==LKS_STATUS_OK);
      CHECK(u->marker!=s->marker && lks_snapshot_serialized_size(u)==bytes);
      CHECK(lks_snapshot_serialize(u,again,bytes)==LKS_STATUS_OK && !memcmp(blob,again,bytes)); lks_snapshot_destroy(u); }
    lks_order_cursor_destroy(cursor); lks_order_destroy(o); lks_order_destroy(r); free(items);
    CHECK(lks_snapshot_serialize(s,again,bytes)==LKS_STATUS_OK && !memcmp(blob,again,bytes));
    lks_snapshot_destroy(t); lks_snapshot_destroy(s); free(blob); free(again); zero_live();
}
static void callbacks_lifetime(void)
{
    size_t items[8],i; LksOrder *o=build(8,items); LksSnapshot *s=NULL,*history[8];
    LksSnapshotOptions opt=options; size_t live; opt.association=associate;
    for(i=0;i<8;++i) {
        fail_row=i; live=lks_alloc_stats_get().live_bytes;
        CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_NOT_FOUND && !s && !o->source_marker);
        CHECK(lks_alloc_stats_get().live_bytes==live && o->revision==0);
    }
    fail_row=SIZE_MAX; payload=SIZE_MAX;
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_INVALID_ARGUMENT && !s);
    payload=4096; callback_source=o;
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK); callback_source=NULL;
    CHECK(s->association_size==8*4096); lks_snapshot_destroy(s);
    for(i=0;i<8;++i) {
        payload=sizeof(size_t); CHECK(lks_order_snapshot_capture(o,&opt,&history[i])==LKS_STATUS_OK);
        CHECK(lks_order_move_back(o,lks_order_first(o))==LKS_STATUS_OK);
        if(i) CHECK(history[i]->marker==history[0]->marker && history[i]->revision!=history[0]->revision);
    }
#if defined(_MSC_VER)
    lks_source_marker_test_refs(o->source_marker,LONG_MAX);
#else
    lks_source_marker_test_refs(o->source_marker,SIZE_MAX);
#endif
    live=lks_alloc_stats_get().live_bytes;
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_CAPACITY_LIMIT && !s && lks_alloc_stats_get().live_bytes==live);
    lks_source_marker_test_refs(o->source_marker,9);
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK && lks_order_snapshot_is_current(o,s));
    CHECK(lks_order_remove(o,lks_order_first(o),NULL)==LKS_STATUS_OK && !lks_order_snapshot_is_current(o,s));
    lks_snapshot_destroy(s);
    lks_order_destroy(o);
    for(i=0;i<8;++i) {
        size_t row=(i*3)%8, size, id; const void *data; char key[40];
        CHECK(lks_snapshot_count(history[row])==8);
        CHECK(lks_snapshot_association(history[row],0,&data,&size)==LKS_STATUS_OK && size==sizeof(id));
        memcpy(&id,data,sizeof(id)); CHECK(id==row);
        CHECK(lks_snapshot_key_format(history[row],0,key,sizeof(key))==LKS_STATUS_OK && lks_snapshot_key_validate(key)==LKS_STATUS_OK);
        lks_snapshot_destroy(history[row]);
    }
    zero_live();
    { LksComparator cmp={compare,NULL}; LksManagedOrder *m=lks_managed_order_create(&cmp); const LksOrderHandle *h;
      for(i=0;i<8;++i) CHECK(lks_managed_order_insert(m,&items[7-i],&h)==LKS_STATUS_OK);
      comparator_calls=0; callback_managed=m; callbacks=0;
      CHECK(lks_managed_order_snapshot_capture(m,&opt,&s)==LKS_STATUS_OK && callbacks==8 && !comparator_calls);
      callback_managed=NULL; CHECK(lks_managed_order_snapshot_is_current(m,s));
      lks_alloc_test_fail_on_attempt(1); CHECK(lks_managed_order_insert(m,&items[0],&h)==LKS_STATUS_OUT_OF_MEMORY);
      lks_alloc_test_disable_failure(); CHECK(lks_managed_order_snapshot_is_current(m,s));
      CHECK(lks_managed_order_remove(m,lks_managed_order_first(m),NULL)==LKS_STATUS_OK && !lks_managed_order_snapshot_is_current(m,s));
      lks_managed_order_destroy(m); CHECK(lks_snapshot_count(s)==8); lks_snapshot_destroy(s); zero_live(); }
}
static void migration(void)
{
    size_t items[3]={0,1,2},i; LksOrder *o=NULL; LksV3Lk1ImportEntry entries[3];
    LksPath *paths[3]={lks_path_create(LKS_DIRECTION_POSITIVE,65535),lks_path_create_zero(),lks_path_create(LKS_DIRECTION_NEGATIVE,0)};
    char keys[3][100];
    CHECK(lks_order_import_v3_lk1(NULL,0,&o)==LKS_STATUS_OK && lks_order_internal_valid(o) && !lks_order_size(o));
    lks_order_destroy(o); o=NULL;
    for(i=0;i<3;++i) { CHECK(lks_path_order_key_format(paths[i],keys[i],100)==LKS_STATUS_OK); entries[i].key=keys[i]; entries[i].item=&items[i]; }
    for(i=1;;++i) {
        size_t live=lks_alloc_stats_get().live_bytes; LksStatus status;
        lks_alloc_test_fail_on_attempt(i); status=lks_order_import_v3_lk1(entries,3,&o); lks_alloc_test_disable_failure();
        if(status==LKS_STATUS_OK) break;
        CHECK(status==LKS_STATUS_OUT_OF_MEMORY && !o && lks_alloc_stats_get().live_bytes==live);
    }
    CHECK(lks_order_internal_valid(o) && lks_order_item(lks_order_first(o))==&items[2] && lks_order_item(lks_order_last(o))==&items[0]);
    lks_order_destroy(o); entries[1].key=entries[0].key;
    CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_ALREADY_EXISTS && !o);
    entries[1].key="LK1:invalid"; CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_INVALID_ARGUMENT && !o);
    entries[1].key="LK2:1!"; CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_INVALID_ARGUMENT && !o);
    entries[1].key=entries[0].key; entries[1].item=NULL;
    CHECK(lks_order_import_v3_lk1(entries,3,&o)==LKS_STATUS_INVALID_ARGUMENT && !o);
    for(i=0;i<3;++i) lks_path_destroy(paths[i]);
    zero_live();
}
static LksStatus resolve_fail(const void *data,size_t size,uint64_t ordinal,
    void *context,void **item)
{
    (void)data; (void)size; (void)context; *item=NULL;
    return ordinal==1 ? LKS_STATUS_NOT_FOUND : LKS_STATUS_OK;
}
static LksStatus resolve_status_fail(const void *data,size_t size,uint64_t ordinal,
    void *context,void **item)
{
    if(ordinal==fail_row) return LKS_STATUS_NOT_FOUND;
    return resolve(data,size,ordinal,context,item);
}
static void large_and_edges(void)
{
    size_t n=100000,i,*items=(size_t *)malloc(n*sizeof(*items)); LksOrder *o=build(n,items),*r=NULL;
    LksSnapshot *s=NULL; LksSnapshotOptions opt=options; const LksOrderHandle *h;
    unsigned char *blob; size_t bytes; LksSnapshot *loaded=NULL;
    opt.association=associate; payload=sizeof(size_t); callbacks=0;
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK && callbacks==n);
    bytes=lks_snapshot_serialized_size(s); blob=(unsigned char *)malloc(bytes);
    CHECK(lks_snapshot_serialize(s,blob,bytes)==LKS_STATUS_OK);
    CHECK(lks_snapshot_deserialize(blob,bytes,&loaded)==LKS_STATUS_OK);
    CHECK(lks_snapshot_restore_order(loaded,resolve,items,&r)==LKS_STATUS_OK && lks_order_internal_valid(r));
    for(i=0,h=lks_order_first(r);h;h=lks_order_next(h),++i) CHECK(lks_order_item(h)==items+i);
    CHECK(i==n); lks_order_destroy(r); r=NULL;
    for(i=0;i<3;++i) {
        size_t live=lks_alloc_stats_get().live_bytes;
        fail_row=i==0?0:i==1?n/2:n-1;
        CHECK(lks_snapshot_restore_order(loaded,resolve_status_fail,items,&r)==LKS_STATUS_NOT_FOUND && !r);
        CHECK(lks_alloc_stats_get().live_bytes==live);
    }
    fail_row=SIZE_MAX;
    CHECK(lks_snapshot_restore_order(loaded,resolve_fail,NULL,&r)==LKS_STATUS_INVALID_ARGUMENT && !r);
    lks_order_destroy(o); free(items); free(blob); lks_snapshot_destroy(s); lks_snapshot_destroy(loaded); zero_live();
    /* Large migration uses independent level-order expectation; shuffled inputs,
     * repeated item pointers, and one deep Path with maximum terminal level. */
    n=10000;
    { LksV3Lk1ImportEntry *entries=(LksV3Lk1ImportEntry *)malloc(n*sizeof(*entries));
      char (*text)[100]=(char (*)[100])malloc(n*100); size_t item=1;
      size_t *ids=(size_t *)malloc(n*sizeof(*ids)); CHECK(ids);
      for(i=0;i<n;++i) {
          size_t index=(i*7919)%n;
          LksPath *p=lks_path_create_at_level(LKS_DIRECTION_POSITIVE,0,index);
          CHECK(p && lks_path_order_key_format(p,text[index],100)==LKS_STATUS_OK);
          ids[index]=index; entries[i].key=text[index]; entries[i].item=&ids[index]; lks_path_destroy(p);
      }
      CHECK(lks_order_import_v3_lk1(entries,n,&o)==LKS_STATUS_OK && lks_order_internal_valid(o) && lks_order_size(o)==n);
      for(i=n,h=lks_order_first(o);h;h=lks_order_next(h)) {
          CHECK(i>0); --i; CHECK(lks_order_item(h)==ids+i);
      }
      CHECK(i==0);
      lks_order_destroy(o); free(entries); free(text); free(ids);
      { LksPath *p=lks_path_create(LKS_DIRECTION_NEGATIVE,65535); char *key; size_t length;
        LksV3Lk1ImportEntry e;
        for(i=1;i<10000;++i) CHECK(lks_path_append(p,(unsigned int)(i%65536))==LKS_STATUS_OK);
        CHECK(lks_path_append_at_level(p,0,SIZE_MAX)==LKS_STATUS_OK);
        length=lks_path_order_key_length(p); key=(char *)malloc(length+1);
        CHECK(lks_path_order_key_format(p,key,length+1)==LKS_STATUS_OK); e.key=key; e.item=&item;
        CHECK(lks_order_import_v3_lk1(&e,1,&o)==LKS_STATUS_OK && lks_order_size(o)==1);
        lks_order_destroy(o); lks_path_destroy(p); free(key);
      }
    }
    zero_live();
}
int main(void)
{
    static const size_t counts[]={0,1,63,64,65,127,128,129,191,192,193,1024}; size_t i;
    keys(); wire(); callbacks_lifetime();
    for(i=0;i<sizeof(counts)/sizeof(*counts);++i) capture_restore(counts[i]);
    migration(); large_and_edges(); puts("Preview.3 snapshot keys/wire/torture/lifetime/bulk/OOM PASS"); return 0;
}
