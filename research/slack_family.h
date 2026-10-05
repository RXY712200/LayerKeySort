#ifndef LKS_RESEARCH_SLACK_FAMILY_H
#define LKS_RESEARCH_SLACK_FAMILY_H
#include <stdint.h>
#include "layerkeysort.h"

/* Research-only, finite equal-depth family. The last one/two slots form a
 * big-endian rank; all levels and earlier slots are fixed. No resident state. */
typedef struct LksSlackPlan {
    LksDirection direction;
    size_t depth, variable;
    size_t levels[6];
    unsigned int slots[6];
    uint64_t lower, upper, stride;
} LksSlackPlan;

int lks_slack_plan(const LksPath *reference, const LksPath *left,
    const LksPath *right, size_t population, LksSlackPlan *out);
int lks_slack_compare_rank(const LksSlackPlan *plan, uint64_t rank,
    const LksPath *path);
int lks_slack_occupied(const LksSlackPlan *plan, const LksPath *path);
LksStatus lks_slack_materialize(const LksSlackPlan *plan, uint64_t rank,
    LksPath **out);
#endif
