#ifndef RESEARCH_PROBE_H
#define RESEARCH_PROBE_H
#include <stddef.h>
typedef struct ResearchMemory {size_t allocations,reallocations,frees,live,bytes,peak,traffic;} ResearchMemory;
typedef struct ResearchWork {
    size_t assignments,splits,merges,redistributions,rotations,index_writes;
    size_t max_assignments,max_index,blocks,height,minimum,maximum,used,slots;
    size_t occupancy[65];
} ResearchWork;
void research_scope(int on);
ResearchMemory research_memory(void);
void research_work_reset(void);
void research_after(const void *order,int managed);
ResearchWork research_work(const void *order,int managed);
#endif
