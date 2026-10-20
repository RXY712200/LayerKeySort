#include "model.h"
#include <string.h>
size_t sequence_find(const Sequence *s, size_t id)
{
    size_t i;
    for (i = 0; i < s->count; ++i) if (s->ids[i] == id) return i;
    return s->count;
}
int sequence_insert(Sequence *s, size_t id, size_t position)
{
    if (position > s->count || s->count == DOCUMENT_LIMIT) return 0;
    memmove(s->ids + position + 1, s->ids + position,
        (s->count - position) * sizeof(size_t));
    s->ids[position] = id; ++s->count; return 1;
}
int sequence_remove(Sequence *s, size_t id)
{
    size_t p = sequence_find(s, id);
    if (p == s->count) return 0;
    memmove(s->ids + p, s->ids + p + 1, (s->count - p - 1) * sizeof(size_t));
    --s->count; return 1;
}
int sequence_move(Sequence *s, size_t id, size_t anchor, const char *where)
{
    size_t p;
    if (sequence_find(s, id) == s->count) return 0;
    if (strcmp(where, "before") == 0 || strcmp(where, "after") == 0) {
        if (sequence_find(s, anchor) == s->count) return 0;
        if (id == anchor) return 1;
    }
    if (!sequence_remove(s, id)) return 0;
    if (strcmp(where, "front") == 0) p = 0;
    else if (strcmp(where, "back") == 0) p = s->count;
    else {
        p = sequence_find(s, anchor);
        if (strcmp(where, "after") == 0) ++p;
    }
    return sequence_insert(s, id, p);
}
