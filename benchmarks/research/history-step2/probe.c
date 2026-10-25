#include "probe.h"
#ifdef RESEARCH_PRIVATE
/* Only this research-only adapter sees private structures. Matching diagnostic
 * library is compiled from the selected Full source root with its opt-in macro. */
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <string.h>
static ResearchWork work;
void research_scope(int on){(void)on;}
ResearchMemory research_memory(void)
{
    LksAllocStats s=lks_alloc_stats_get();
    ResearchMemory m={s.alloc_calls,s.realloc_calls,s.free_calls,s.live_blocks,s.live_bytes,s.peak_live_bytes,s.total_successful_requested_bytes};
    return m;
}
void research_work_reset(void){memset(&work,0,sizeof(work));}
static const LksOrder *core(const void *o,int managed)
{return managed?((const LksManagedOrder *)o)->core:(const LksOrder *)o;}
void research_after(const void *o,int managed)
{
    const LksOrderWork *w=&core(o,managed)->work;
    work.assignments+=w->records_reassigned;work.splits+=w->splits;work.merges+=w->merges;
    work.redistributions+=w->redistributions;work.rotations+=w->rotations;work.index_writes+=w->index_blocks;
    if(w->records_reassigned>work.max_assignments)work.max_assignments=w->records_reassigned;
    if(w->index_blocks>work.max_index)work.max_index=w->index_blocks;
}
ResearchWork research_work(const void *o,int managed)
{
    ResearchWork result=work;const LksOrderBlock *b;
    if(!o)return result;
    result.minimum=65;result.height=core(o,managed)->root?(size_t)core(o,managed)->root->height:0;
    for(b=core(o,managed)->first;b;b=b->next){
        ++result.blocks;result.used+=b->count;result.slots+=LKS_ORDER_BLOCK_CAPACITY;
        if(b->count<result.minimum)result.minimum=b->count;
        if(b->count>result.maximum)result.maximum=b->count;
        if(b->count<=64)++result.occupancy[b->count];
    }
    if(!result.blocks)result.minimum=0;
    return result;
}
#else
#ifndef RESEARCH_WRAP
void research_scope(int on){(void)on;}
ResearchMemory research_memory(void){ResearchMemory m={0};return m;}
#endif
void research_work_reset(void){}
void research_after(const void *order,int managed){(void)order;(void)managed;}
ResearchWork research_work(const void *order,int managed)
{ResearchWork w={0};(void)order;(void)managed;return w;}
#endif
