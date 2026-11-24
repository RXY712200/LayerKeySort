#define _POSIX_C_SOURCE 200809L
/* Reconstruct, fingerprint, intervene once. No restored-state substitution. */
#include <layerkeysort.h>
#include "lks_order_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
static double now(void) {
#ifdef _WIN32
LARGE_INTEGER c,f;QueryPerformanceCounter(&c);QueryPerformanceFrequency(&f);return(double)c.QuadPart/f.QuadPart;
#else
struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(double)t.tv_sec+t.tv_nsec*1e-9;
#endif
}
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
#include "lks_alloc_internal.h"
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"locality:%d %s\n",__LINE__,#x); exit(2); } } while(0)
static uint64_t fingerprint(const LksOrder *o) {
    uint64_t z=UINT64_C(1469598103934665603);
    for (const LksOrderBlock *b=o->first;b;b=b->next) {
        z=(z^b->count)*UINT64_C(1099511628211);
        for(size_t i=0;i<b->count;++i) z=(z^*(const size_t*)b->records[i]->item)*UINT64_C(1099511628211);
    }
    return z;
}
int main(void) {
    puts("n,repeat,intervention,fingerprint,source_count,destination_count,distance,seconds,alloc_attempts,reassigned,index_blocks,cross,splits,noop");
    const size_t sizes[]={128,1024,10016};
    for(size_t s=0;s<3;++s) {
        size_t n=sizes[s];uint64_t expected_fingerprint=0;
        for(size_t r=0;r<30;++r) for(size_t k=0;k<6;++k) {
            LksOrder *o=lks_order_create();CHECK(o);
            const LksOrderHandle **h=calloc(n,sizeof(*h));size_t *items=calloc(n,sizeof(*items));size_t *model=calloc(n,sizeof(*model));CHECK(h&&items&&model);
            for(size_t i=0;i<n;++i) {items[i]=model[i]=i;CHECK(lks_order_insert_back(o,items+i,h+i)==LKS_STATUS_OK);}
            uint64_t f=fingerprint(o);if(!expected_fingerprint)expected_fingerprint=f;CHECK(f==expected_fingerprint);
            size_t source=0,anchor=k==0?4:k==1?n-1:k==2?40:k==3?n-65:k==4?0:1;
            const char *names[]={"same","cross_full","cross_nonfull_near","cross_nonfull_far","self_noop","adjacent_noop"};
            size_t sc=h[source]->block->count,dc=h[anchor]->block->count;
            if(k==0)CHECK(h[source]->block==h[anchor]->block);
            if(k==1)CHECK(h[source]->block!=h[anchor]->block && dc==64);
            if(k==2||k==3)CHECK(h[source]->block!=h[anchor]->block && dc<64);
            size_t attempts=0;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            attempts=lks_alloc_test_get_attempt_count();
#endif
            double start=now();CHECK(lks_order_move_before(o,h[source],h[anchor])==LKS_STATUS_OK);double elapsed=now()-start;
            if(k<4) {
                size_t dst=anchor-(source<anchor);
                if(source<dst) for(size_t i=source;i<dst;++i)model[i]=model[i+1];
                else for(size_t i=source;i>dst;--i)model[i]=model[i-1];
                model[dst]=source;
            }
            const LksOrderHandle *p=lks_order_first(o);
            for(size_t i=0;i<n;++i) {CHECK(p==h[model[i]] && lks_order_item(p)==items+model[i]);p=lks_order_next(p);}CHECK(!p);
            size_t assigned=0,index=0,cross=0,splits=0,noop=0;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            attempts=lks_alloc_test_get_attempt_count()-attempts;
            assigned=o->work.records_reassigned;index=o->work.index_blocks;cross=o->work.cross_block_moves;splits=o->work.splits;noop=o->work.noop_moves;
            CHECK(lks_order_internal_valid(o));
            if(k>=4)CHECK(attempts==0 && noop==1);
            if(k==1)CHECK(attempts==1 && cross==1);
#endif
            printf("%zu,%zu,%s,%" PRIu64 ",%zu,%zu,%zu,%.9f,%zu,%zu,%zu,%zu,%zu,%zu\n",n,r,names[k],f,sc,dc,source>anchor?source-anchor:anchor-source,elapsed,attempts,assigned,index,cross,splits,noop);
            lks_order_destroy(o);free(h);free(items);free(model);
        }
    }
    return 0;
}
