/* Explicit export cost placement, not final performance acceptance. */
#include "layerkeysort.h"
#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"snapshot smoke line %d\n",__LINE__); exit(1); } } while (0)
static unsigned char smoke_bytes[64];
static double now(void)
{
#ifdef _WIN32
    LARGE_INTEGER t,f; QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timespec t; timespec_get(&t,TIME_UTC); return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
static LksStatus smoke_associate(void *item, uint64_t ordinal, void *context,
    const void **data, size_t *size)
{ (void)item; (void)ordinal; *data=smoke_bytes; *size=*(const size_t *)context; return LKS_STATUS_OK; }
static LksStatus smoke_resolve(const void *data, size_t size, uint64_t ordinal,
    void *context, void **item)
{ (void)data; (void)size; (void)ordinal; *item=context; return LKS_STATUS_OK; }
static void capture_case(size_t n,size_t association)
{
    int item=1; void **items=(void **)malloc(n*sizeof(*items)); size_t i,bytes;
    const unsigned char ns[16]={0}; LksSnapshotOptions opt={ns,16,smoke_associate,&association};
    LksOrder *o=NULL,*r=NULL; LksSnapshot *s=NULL,*loaded=NULL;
    double start,capture,keys,serialize,deserialize,restore;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    LksAllocStats before,after;
#endif
    char allocations[32]="NA",resident[32]="NA";
    unsigned char *blob; char key[64];
    REQUIRE(items); for(i=0;i<n;++i) items[i]=&item;
    REQUIRE(lks_order_bulk_build(items,n,&o)==LKS_STATUS_OK); free(items);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    before=lks_alloc_stats_get();
#endif
    start=now();
    REQUIRE(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK); capture=now()-start;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    after=lks_alloc_stats_get();
    snprintf(allocations,sizeof(allocations),"%zu",after.alloc_calls-before.alloc_calls+after.realloc_calls-before.realloc_calls);
    snprintf(resident,sizeof(resident),"%zu",after.live_bytes-before.live_bytes);
#endif
    start=now();
    for(i=0;i<n;++i) REQUIRE(lks_snapshot_key_format(s,i,key,sizeof(key))==LKS_STATUS_OK);
    keys=now()-start; bytes=lks_snapshot_serialized_size(s); blob=(unsigned char *)malloc(bytes); REQUIRE(blob);
    start=now(); REQUIRE(lks_snapshot_serialize(s,blob,bytes)==LKS_STATUS_OK); serialize=now()-start;
    start=now(); REQUIRE(lks_snapshot_deserialize(blob,bytes,&loaded)==LKS_STATUS_OK); deserialize=now()-start;
    start=now(); REQUIRE(lks_snapshot_restore_order(loaded,smoke_resolve,&item,&r)==LKS_STATUS_OK); restore=now()-start;
    REQUIRE(lks_order_size(r)==n);
    printf("capture,%zu,%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%s,%s,%zu,%zu,%zu,%zu,%zu,%zu\n",
        n,association,capture*1000,keys*1000,serialize*1000,deserialize*1000,restore*1000,
        allocations,resident, s->namespace_size, n*sizeof(LksSnapshotRow),
        s->association_size,s->association_capacity,bytes,n*lks_snapshot_key_length(s));
    lks_order_destroy(o); lks_order_destroy(r); lks_snapshot_destroy(s); lks_snapshot_destroy(loaded); free(blob);
}
static void frequency_case(size_t cadence,size_t mutations)
{
    LksOrder *o=lks_order_create(); const LksOrderHandle *h; int item=1;
    size_t i,exports=0; double start=now(),export_time=0; LksSnapshotOptions opt={smoke_bytes,16,NULL,NULL};
    REQUIRE(o);
    for(i=1;i<=mutations;++i) {
        REQUIRE(lks_order_insert_back(o,&item,&h)==LKS_STATUS_OK);
        if(cadence && i%cadence==0) {
            LksSnapshot *s=NULL; double t=now();
            REQUIRE(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK);
            lks_snapshot_destroy(s); export_time+=now()-t; ++exports;
        }
    }
    printf("frequency,%zu,%zu,%zu,%.6f,%.6f\n",mutations,cadence,exports,(now()-start)*1000,export_time*1000);
    lks_order_destroy(o);
}
static void import_case(size_t n)
{
    LksV3Lk1ImportEntry *entries=(LksV3Lk1ImportEntry *)malloc(n*sizeof(*entries));
    char (*keys)[100]=(char (*)[100])malloc(n*100); size_t i; int item=1; LksOrder *o=NULL; double start;
    REQUIRE(entries && keys);
    for(i=0;i<n;++i) {
        LksPath *p=lks_path_create_at_level(LKS_DIRECTION_POSITIVE,0,n-i);
        REQUIRE(p && lks_path_order_key_format(p,keys[i],100)==LKS_STATUS_OK);
        lks_path_destroy(p); entries[i].key=keys[i]; entries[i].item=&item;
    }
    start=now(); REQUIRE(lks_order_import_v3_lk1(entries,n,&o)==LKS_STATUS_OK);
    printf("import,%zu,%.6f\n",n,(now()-start)*1000);
    lks_order_destroy(o); free(entries); free(keys);
}
int main(int argc,char **argv)
{
    size_t scales[]={10000,100000,1000000}, associations[]={0,16,64},i,j;
    int smoke=argc>1 && !strcmp(argv[1],"smoke");
    puts("case,n,association_bytes,capture_ms,key_ms,serialize_ms,deserialize_ms,restore_ms,alloc_attempts,resident_delta_bytes,namespace_bytes,row_bytes,association_logical_bytes,association_capacity_bytes,wire_bytes,all_key_bytes");
    for(i=0;i<(smoke?1u:3u);++i) for(j=0;j<3;++j) capture_case(smoke?512:scales[i],associations[j]);
    frequency_case(0,smoke?1000:20000); frequency_case(10000,smoke?1000:20000);
    frequency_case(1000,smoke?1000:20000); frequency_case(100,smoke?1000:20000);
    import_case(smoke?512:10000); import_case(smoke?1024:100000);
    REQUIRE(!lks_alloc_stats_get().live_bytes); return 0;
}
