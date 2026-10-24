/* Separate GNU instrumentation; not linked into production timing. Unlike the
 * earlier consumer wrapper, realloc uses the real CRT realloc implementation. */
#include "probe.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef union Header {max_align_t alignment;struct {size_t size;int tracked;} record;} Header;
void *__real_malloc(size_t);
void *__real_realloc(void *,size_t);
void __real_free(void *);
static ResearchMemory stats;
static int active;
void research_scope(int on){active=on;}
ResearchMemory research_memory(void){return stats;}
static void peak(void){if(stats.bytes>stats.peak)stats.peak=stats.bytes;}
void *__wrap_malloc(size_t size)
{
    Header *h;if(size>SIZE_MAX-sizeof(*h))return NULL;
    h=__real_malloc(size+sizeof(*h));if(!h)return NULL;
    h->record.size=size;h->record.tracked=active;
    if(active){++stats.allocations;++stats.live;stats.bytes+=size;stats.traffic+=size;peak();}
    return h+1;
}
void __wrap_free(void *p)
{
    Header *h;if(!p)return;h=(Header *)p-1;
    if(h->record.tracked){++stats.frees;--stats.live;stats.bytes-=h->record.size;}
    __real_free(h);
}
void *__wrap_calloc(size_t n,size_t size)
{void *p;if(size&&n>SIZE_MAX/size)return NULL;p=__wrap_malloc(n*size);if(p)memset(p,0,n*size);return p;}
void *__wrap_realloc(void *p,size_t size)
{
    Header *old,*h;size_t previous;int tracked;
    if(!p)return __wrap_malloc(size);
    if(!size){__wrap_free(p);return NULL;}
    if(size>SIZE_MAX-sizeof(*h))return NULL;
    old=(Header *)p-1;previous=old->record.size;tracked=old->record.tracked;
    h=__real_realloc(old,sizeof(*h)+size);if(!h)return NULL;
    h->record.size=size;
    if(tracked){++stats.reallocations;stats.bytes=stats.bytes-previous+size;stats.traffic+=size;peak();}
    return h+1;
}
