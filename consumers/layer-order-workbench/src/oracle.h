#ifndef WB_ORACLE_H
#define WB_ORACLE_H
#include "command.h"
/* This module has no Mini header, handles, links or traversal-derived answers. */
typedef struct { size_t id, payload; } ModelEntry;
typedef struct { ModelEntry entries[WB_LIVE]; size_t count; } Oracle;
size_t oracle_index(const Oracle *model, size_t id);
int oracle_compare(const Oracle *model, size_t a, size_t b);
void oracle_insert(Oracle *model, size_t position, ModelEntry entry);
void oracle_remove(Oracle *model, size_t position);
void oracle_move(Oracle *model, size_t id, CommandKind kind, size_t anchor);
#endif
