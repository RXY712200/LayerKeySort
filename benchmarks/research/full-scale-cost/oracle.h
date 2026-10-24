#ifndef RESEARCH_ORACLE_H
#define RESEARCH_ORACLE_H
#include <stddef.h>
#include <stdint.h>
#define ABSENT UINT32_MAX
/* Independent ID-linked sequence. No Full pointers, blocks, ranks or headers. */
typedef struct Oracle {
    uint32_t *previous, *next, first, last;
    size_t count, capacity;
} Oracle;
void oracle_init(Oracle *o,size_t capacity);
void oracle_destroy(Oracle *o);
void oracle_insert(Oracle *o,uint32_t id,uint32_t before);
void oracle_remove(Oracle *o,uint32_t id);
void oracle_move(Oracle *o,uint32_t id,uint32_t before);
uint64_t research_random(uint64_t *state);
#endif
