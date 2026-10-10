#include "oracle.h"
#include <string.h>
size_t oracle_index(const Oracle *model, size_t id)
{
    size_t i;
    for (i = 0; i < model->count; ++i) if (model->entries[i].id == id) return i;
    return model->count;
}
int oracle_compare(const Oracle *model, size_t a, size_t b)
{
    size_t i = oracle_index(model, a), j = oracle_index(model, b);
    return i < j ? -1 : i > j ? 1 : 0;
}
void oracle_insert(Oracle *model, size_t position, ModelEntry entry)
{
    memmove(model->entries + position + 1u, model->entries + position,
            (model->count - position) * sizeof(entry));
    model->entries[position] = entry;
    ++model->count;
}
void oracle_remove(Oracle *model, size_t position)
{
    memmove(model->entries + position, model->entries + position + 1u,
            (model->count - position - 1u) * sizeof(model->entries[0]));
    --model->count;
}
void oracle_move(Oracle *model, size_t id, CommandKind kind, size_t anchor)
{
    size_t from = oracle_index(model, id), to;
    ModelEntry entry = model->entries[from];
    if (id == anchor) return;
    /* Remove from the array, then resolve the remaining anchor by ID. */
    oracle_remove(model, from);
    if (kind == CMD_MOVE_FRONT) to = 0;
    else if (kind == CMD_MOVE_BACK) to = model->count;
    else to = oracle_index(model, anchor) + (kind == CMD_MOVE_AFTER ? 1u : 0u);
    oracle_insert(model, to, entry);
}
