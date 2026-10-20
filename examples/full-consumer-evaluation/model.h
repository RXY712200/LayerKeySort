#ifndef EVAL_MODEL_H
#define EVAL_MODEL_H
#include <stddef.h>
enum { DOCUMENT_LIMIT = 4096, OCCURRENCE_LIMIT = 262144, HISTORY_LIMIT = 32 };
typedef struct Sequence { size_t ids[DOCUMENT_LIMIT]; size_t count; } Sequence;
/* No library headers, handles, items or adapter logic in this model. */
int sequence_insert(Sequence *s, size_t id, size_t position);
int sequence_remove(Sequence *s, size_t id);
int sequence_move(Sequence *s, size_t id, size_t anchor, const char *where);
size_t sequence_find(const Sequence *s, size_t id);
#endif
