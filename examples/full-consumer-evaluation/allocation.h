#ifndef EVAL_ALLOCATION_H
#define EVAL_ALLOCATION_H
#include <stddef.h>
typedef struct AllocationStats {size_t attempts, allocations, frees, live, bytes, peak, traffic;} AllocationStats;
void allocation_scope(int enabled);
void allocation_fail(size_t index);
AllocationStats allocation_stats(void);
void allocation_reset_counters(void);
#endif
