/* Reclamation screen, NOT an alternate Full implementation or timing oracle.
 * Stable-address slab records have the compact production record's x64 size.
 * A surviving record pins its slab. Empty slabs are returned immediately. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Record { void *block, *item; uint16_t local, slot; } Record;
typedef struct Page {
    struct Page *previous, *next;
    size_t live, used, capacity, bytes;
    Record *free_record;
    Record records[];
} Page;
typedef struct Pool { Page *first, *current; size_t calls, frees, bytes, peak; } Pool;
static void require(int value) { if (!value) exit(2); }
static Record *acquire(Pool *pool, size_t capacity, void *item)
{
    Page *p = pool->current; Record *r;
    if (!p || p->used == p->capacity) {
        size_t bytes = offsetof(Page, records)+capacity*sizeof(Record);
        p = malloc(bytes); require(p != NULL); memset(p, 0, offsetof(Page, records));
        p->capacity = capacity; p->bytes = bytes; p->next = pool->first;
        if (p->next) p->next->previous = p;
        pool->first = pool->current = p; ++pool->calls; pool->bytes += bytes;
        if (pool->bytes > pool->peak) pool->peak = pool->bytes;
    }
    r = &p->records[p->used]; r->slot = (uint16_t)p->used++;
    r->local = 0; r->block = NULL; r->item = item; ++p->live;
    return r;
}
static void release(Pool *pool, Record *r)
{
    Page *p = (Page *)((unsigned char *)r - offsetof(Page, records) - r->slot*sizeof(Record));
    require(p->live != 0);
    if (--p->live == 0) {
        if (p->previous) p->previous->next = p->next; else pool->first = p->next;
        if (p->next) p->next->previous = p->previous;
        if (pool->current == p) pool->current = NULL;
        pool->bytes -= p->bytes; ++pool->frees; free(p);
    }
}
static uint32_t random_step(uint32_t *s) { *s = *s*UINT32_C(1664525)+UINT32_C(1013904223); return *s; }
int main(void)
{
    const size_t n = 1000000; size_t capacity, i, k;
    Record **handles = malloc(n*sizeof(*handles));
    uint32_t *ids = malloc(n*sizeof(*ids)), seed = 17;
    require(handles && ids);
    for (i = 0; i < n; ++i) ids[i] = (uint32_t)i;
    for (i = n; i > 1; --i) {
        size_t j = random_step(&seed)%i; uint32_t tmp = ids[i-1]; ids[i-1] = ids[j]; ids[j] = tmp;
    }
    puts("capacity,residents,live,requested_bytes,allocation_calls,free_calls,record_bytes,page_header_bytes");
    for (capacity = 16; capacity <= 256; capacity *= 4) {
        Pool pool = {0};
        for (i = 0; i < n; ++i) handles[i] = acquire(&pool, capacity, &ids[i]);
        printf("%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n", capacity,n,n,pool.bytes,pool.calls,pool.frees,sizeof(Record),offsetof(Page,records));
        for (k = 0; k < n; ++k) {
            size_t id = ids[k];
            require(handles[id]->item == &ids[id]); release(&pool, handles[id]); handles[id] = NULL;
            if (k+1 == n*9/10 || k+1 == n*99/100)
                printf("%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n", capacity,n,n-k-1,pool.bytes,pool.calls,pool.frees,sizeof(Record),offsetof(Page,records));
        }
        require(!pool.first && !pool.bytes && pool.calls == pool.frees);
        printf("%zu,%zu,0,0,%zu,%zu,%zu,%zu\n",capacity,n,pool.calls,pool.frees,sizeof(Record),offsetof(Page,records));
    }
    free(ids); free(handles); return 0;
}
