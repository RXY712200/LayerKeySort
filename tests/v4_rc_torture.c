/* RC input campaign: accepted bytes must round-trip exactly; rejected input
 * must never publish or leak. Expected grammar is frozen elsewhere. */
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"RC torture line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
static uint32_t random_next(uint32_t *state)
{ *state=*state*UINT32_C(1664525)+UINT32_C(1013904223);return *state; }
static size_t accepted, rejected;
static void check_wire(const unsigned char *bytes,size_t size)
{
    LksSnapshot *s=NULL;LksAllocStats before=lks_alloc_stats_get();
    LksStatus status=lks_snapshot_deserialize(bytes,size,&s);
    if(status==LKS_STATUS_OK) {
        unsigned char *copy=(unsigned char *)malloc(size?size:1);
        CHECK(copy && s && lks_snapshot_serialized_size(s)==size);
        CHECK(lks_snapshot_serialize(s,copy,size)==LKS_STATUS_OK && !memcmp(bytes,copy,size));
        free(copy);lks_snapshot_destroy(s);++accepted;
    } else { CHECK(status==LKS_STATUS_INVALID_ARGUMENT && !s);++rejected; }
    CHECK(lks_alloc_stats_get().live_bytes==before.live_bytes && lks_alloc_stats_get().live_blocks==before.live_blocks);
}
static void check_key(const char *text)
{
    int result=17;LksAllocStats before=lks_alloc_stats_get();
    LksStatus status=lks_snapshot_key_validate(text);
    CHECK(status==LKS_STATUS_OK || status==LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_snapshot_key_compare(text,text,&result)==status && result==0);
    CHECK(lks_alloc_stats_get().alloc_calls==before.alloc_calls && lks_alloc_stats_get().live_bytes==before.live_bytes);
}
static LksStatus association(void *item,uint64_t ordinal,void *context,const void **data,size_t *size)
{
    static const unsigned char payload[]={0,255,17,0,128};
    (void)item;(void)context;*data=payload;*size=(size_t)(ordinal%6);return LKS_STATUS_OK;
}
static void campaign(uint32_t seed,size_t iterations)
{
    LksOrder *o=lks_order_create();LksSnapshot *s=NULL;
    unsigned char domain[]={0,255,0,127},wire[512],bad[513];
    LksSnapshotOptions opt={domain,sizeof(domain),association,NULL};
    const LksOrderHandle *h;int item=1;size_t i,n,length;char key[256],text[256];
    CHECK(o);for(i=0;i<12;++i)CHECK(lks_order_insert_back(o,&item,&h)==LKS_STATUS_OK);
    CHECK(lks_order_snapshot_capture(o,&opt,&s)==LKS_STATUS_OK);
    n=lks_snapshot_serialized_size(s);CHECK(n<sizeof(wire));
    /* Export formatting/serialization has no allocation, even with a failpoint. */
    lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
    CHECK(lks_snapshot_serialize(s,wire,n)==LKS_STATUS_OK);
    CHECK(lks_snapshot_key_format(s,11,key,sizeof(key))==LKS_STATUS_OK);
    memset(bad,0xa5,sizeof(bad));CHECK(lks_snapshot_serialize(s,bad,n-1)==LKS_STATUS_BUFFER_TOO_SMALL);
    for(i=0;i<sizeof(bad);++i)CHECK(bad[i]==0xa5);
    CHECK(!lks_alloc_test_get_attempt_count());lks_alloc_test_disable_failure();
    length=strlen(key);check_wire(wire,n);
    for(i=0;i<n;++i)check_wire(wire,i);
    memcpy(bad,wire,n);bad[n]=0;check_wire(bad,n+1);
    for(i=0;i<iterations;++i) {
        size_t j,at=random_next(&seed)%n;
        memcpy(bad,wire,n);bad[at]^=(unsigned char)(1u+(random_next(&seed)%255u));check_wire(bad,n);
        /* Corrupt complete length/count fields, not just random payload bytes. */
        memcpy(bad,wire,n);memset(bad+(i%2?16:24+sizeof(domain)),255,8);check_wire(bad,n);
        at=random_next(&seed)%sizeof(bad);
        for(j=0;j<at;++j)bad[j]=(unsigned char)random_next(&seed);
        check_wire(bad,at);
        strcpy(text,key);at=random_next(&seed)%length;
        text[at]=(char)(random_next(&seed)%128u);check_key(text);
        memcpy(text,key,i%length);text[i%length]=0;check_key(text);
        at=random_next(&seed)%(sizeof(text)-1);
        for(j=0;j<at;++j)text[j]=(char)(random_next(&seed)%128u);
        text[at]=0;check_key(text);
    }
    lks_order_destroy(o); /* Snapshot carries copied bytes, never source/items. */
    check_wire(wire,n);lks_snapshot_destroy(s);
    CHECK(!lks_alloc_stats_get().live_bytes && !lks_alloc_stats_get().live_blocks);
}
static int compare(const void *a,const void *b,void *ctx)
{ (void)ctx;return (*(const int *)a>*(const int *)b)-(*(const int *)a<*(const int *)b); }
static void locate_alias_and_noalloc(void)
{
    LksComparator cmp={compare,NULL};LksManagedOrder *o=lks_managed_order_create(&cmp);
    const LksOrderHandle *h,*p,*e,*s;int a=1,b=2,result=9;
    CHECK(o && lks_managed_order_insert(o,&a,&h)==LKS_STATUS_OK);
    CHECK(lks_managed_order_insert(o,&b,&h)==LKS_STATUS_OK);
    CHECK(lks_managed_order_locate(o,&a,&p,&p,&s)==LKS_STATUS_INVALID_ARGUMENT && !p && !s);
    lks_alloc_test_reset_attempt_counter();lks_alloc_test_fail_on_attempt(1);
    CHECK(lks_managed_order_locate(o,&a,&p,&e,&s)==LKS_STATUS_OK && !p && e && s);
    CHECK(lks_managed_order_compare(o,e,s,&result)==LKS_STATUS_OK && result<0);
    CHECK(lks_managed_order_remove(o,e,NULL)==LKS_STATUS_OK);
    CHECK(!lks_alloc_test_get_attempt_count());lks_alloc_test_disable_failure();
    lks_managed_order_destroy(o);
}
int main(int argc,char **argv)
{
    size_t iterations=2000;
    if(argc==2 && !strcmp(argv[1],"--long"))iterations=20000;
    else if(argc!=1){fputs("usage: v4_rc_torture [--long]\n",stderr);return 2;}
    CHECK(lks_alloc_stats_reset()==0);
    campaign(17,iterations);campaign(UINT32_C(1779033703),iterations);campaign(UINT32_C(3144134277),iterations);
    locate_alias_and_noalloc();CHECK(!lks_alloc_stats_get().live_bytes && !lks_alloc_stats_get().live_blocks);
    printf("RC parser campaign iterations=%zu seeds=3 accepted=%zu rejected=%zu; zero live allocations PASS\n",iterations,accepted,rejected);
    return 0;
}
