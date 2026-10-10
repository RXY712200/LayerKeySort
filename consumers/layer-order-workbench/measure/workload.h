#ifndef WB_MEASURE_WORKLOAD_H
#define WB_MEASURE_WORKLOAD_H
#include "workbench.h"
#define MEASURE_CASES 12u
#define MEASURE_STEPS 4096u
typedef struct {
    Command command;
    size_t new_id, before, after, distance;
    int relation;
    char raw[WB_LINE];
} Action;
typedef struct {
    Action actions[MEASURE_STEPS];
    size_t count, initial, minimum, maximum, final;
    size_t compares, forward, reverse, distance_sum, distance_max;
} Workload;
const char *measure_case(size_t index);
int workload_make(Workload *out, size_t case_id, size_t n, size_t steps);
void workload_model(Oracle *model, const Action *action);
int workload_correct(const Workload *workload);
int workload_setup(Workbench *wb, size_t n);
#endif
