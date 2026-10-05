#ifndef LKS_RESEARCH_INTERNAL_H
#define LKS_RESEARCH_INTERNAL_H

/* Branch-only instrumentation. No public API or default runtime state. */
#ifdef LKS_RESEARCH_TRACE
#include "layerkeysort.h"
typedef struct LksResearchWindow {
    size_t candidate_depth, window, position, selected_left, selected_right;
    size_t left_depth, right_depth, generated, max_depth, next_window;
    size_t alloc_calls, requested_bytes;
    int full, accepted;
    LksStatus status;
    double scratch_ms, selection_ms, generation_ms, validation_ms, finish_ms;
    char left[256], right[256], before[256], after[256];
} LksResearchWindow;
void lks_research_trace_begin(int enabled);
const LksResearchWindow *lks_research_trace_get(size_t *count);
#endif
#endif
