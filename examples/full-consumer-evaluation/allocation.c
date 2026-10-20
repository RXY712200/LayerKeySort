/* GNU linker wrappers. Never linked into the shipped editor or timing binary.
 * Counts requested bytes, not malloc metadata. Header changes allocator sizing;
 * instrumented RSS must not be confused with the uninstrumented process. */
#include "allocation.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef union Header {max_align_t alignment;struct {size_t bytes;int tracked;} data;} Header;
void *__real_malloc(size_t);
void __real_free(void *);
static AllocationStats stats;
static int active;
static size_t fail_index;
void allocation_scope(int on){active=on;}
void allocation_fail(size_t index){stats.attempts=0;fail_index=index;}
AllocationStats allocation_stats(void){return stats;}
void allocation_reset_counters(void){stats.attempts=stats.allocations=stats.frees=stats.traffic=0;stats.peak=stats.bytes;}
void *__wrap_malloc(size_t n)
{
    Header *h;
    if(active){++stats.attempts;if(fail_index&&stats.attempts==fail_index)return NULL;}
    if(n>SIZE_MAX-sizeof(*h))return NULL;
    h=__real_malloc(sizeof(*h)+n);if(!h)return NULL;
    h->data.bytes=n;h->data.tracked=active;
    if(active){++stats.allocations;++stats.live;stats.bytes+=n;stats.traffic+=n;if(stats.bytes>stats.peak)stats.peak=stats.bytes;}
    return h+1;
}
void __wrap_free(void *p)
{
    Header *h;if(!p)return;h=(Header *)p-1;
    if(h->data.tracked){++stats.frees;--stats.live;stats.bytes-=h->data.bytes;}
    __real_free(h);
}
void *__wrap_calloc(size_t n,size_t size)
{
    void *p;if(size&&n>SIZE_MAX/size)return NULL;p=__wrap_malloc(n*size);if(p)memset(p,0,n*size);return p;
}
void *__wrap_realloc(void *p,size_t size)
{
    Header *h;void *q;int saved=active;
    if(!p)return __wrap_malloc(size);
    if(!size){__wrap_free(p);return NULL;}
    h=(Header *)p-1; active=h->data.tracked;q=__wrap_malloc(size);active=saved;
    if(q){memcpy(q,p,h->data.bytes<size?h->data.bytes:size);__wrap_free(p);}return q;
}
