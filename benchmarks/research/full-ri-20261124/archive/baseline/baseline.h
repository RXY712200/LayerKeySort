#ifndef PREPILOT_BASELINE_H
#define PREPILOT_BASELINE_H
#include <stddef.h>
#include <stdint.h>
typedef struct Node Node;
typedef struct Sequence Sequence;
typedef struct Stats { size_t live, peak, allocations, frees, attempts; } Stats;
Sequence *seq_create(void);
void seq_destroy(Sequence *s);
Node *seq_insert(Sequence *s, Node *anchor, int after, void *item);
void seq_remove(Sequence *s, Node *h);
void seq_move(Sequence *s, Node *h, Node *anchor, int after);
int seq_compare(Node *a, Node *b);
Node *seq_first(Sequence *s);
Node *seq_last(Sequence *s);
Node *seq_next(Node *h);
Node *seq_previous(Node *h);
void *seq_item(Node *h);
size_t seq_size(Sequence *s);
int seq_valid(Sequence *s);
Stats seq_stats(void);
void seq_fail(size_t ordinal);
#endif
