#ifndef WB_STATE_H
#define WB_STATE_H
#include "oracle.h"
#include "layerkeysort_mini.h"
typedef struct { size_t id, references; char name[WB_NAME]; } LayerObject;
typedef struct { LksMiniHandle *handle; size_t payload; } LiveOccurrence;
typedef struct {
    LksMiniOrder *order;
    LayerObject objects[WB_OBJECTS];
    LiveOccurrence occurrences[WB_OCCURRENCES];
    size_t object_count, issued, sequence;
    Oracle model;
} Workbench;
typedef struct {
    const char *status;
    size_t object, occurrence, anchor, before, after;
    int relation;
} Result;
int wb_init(Workbench *wb);
void wb_destroy(Workbench *wb);
int wb_execute(Workbench *wb, const Command *command, const char *parse_error, Result *result);
int wb_verify(const Workbench *wb);
int wb_list(const Workbench *wb, int reverse);
#endif
