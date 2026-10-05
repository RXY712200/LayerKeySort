/* Preliminary application-operation sanity screen, not a tuning/ranking suite.
 * Shared neighbor intents, flat oracle bookkeeping outside timed intervals.
 * V3 owns explicit Path keys in the application; V4 owns contextual records.
 * V3 has no public logical iterator, so traversal uses its private fill helper.
 * No benchmark helper participates in V4 production ordering. */
#include "layerkeysort.h"
#include "lks_tree_internal.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr,"smoke failure line %d\n",__LINE__); exit(1); } } while(0)
static double smoke_clock(void)
{
#ifdef _WIN32
    LARGE_INTEGER now, frequency;
    QueryPerformanceCounter(&now); QueryPerformanceFrequency(&frequency);
    return (double)now.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timespec now;
    REQUIRE(timespec_get(&now, TIME_UTC) == TIME_UTC);
    return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1000000.0;
#endif
}
static uint32_t smoke_random(uint32_t *state)
{ *state = *state * UINT32_C(1664525) + UINT32_C(1013904223); return *state; }
static void smoke_case(const char *name, size_t n, int mode, int v4)
{
    size_t *sequence = (size_t *)malloc(n*sizeof(*sequence));
    int *items = (int *)malloc(n*sizeof(*items));
    LksPath **paths = (LksPath **)calloc(n,sizeof(*paths));
    const LksOrderHandle **handles = (const LksOrderHandle **)calloc(n,sizeof(*handles));
    const LksTreeNode **nodes = (const LksTreeNode **)malloc(n*sizeof(*nodes));
    LksTree *tree = v4 ? NULL : lks_tree_create();
    LksOrder *order = v4 ? lks_order_create() : NULL;
    uint32_t state = 17;
    size_t i, count = 0, queries = 10000;
    uint64_t checksum = 0;
    double insert_ms = 0, remove_ms = 0, traversal_ms, compare_ms, tick;
    REQUIRE(sequence && items && paths && handles && nodes && (order || tree));
    for (i = 0; i < n; ++i) {
        size_t at = !i || mode == 0 ? count : mode == 1 ? count-1 : smoke_random(&state) % (count+1);
        items[i] = (int)i;
        tick = smoke_clock();
        if (v4) {
            LksStatus status = at == count ? lks_order_insert_back(order,&items[i],&handles[i]) :
                lks_order_insert_before(order,handles[sequence[at]],&items[i],&handles[i]);
            REQUIRE(status == LKS_STATUS_OK);
        } else {
            const LksTreeNode *node;
            if (mode == 0) {
                /* Known short explicit coordinates, two levels for N>65536.
                 * This deliberately does not measure V3 managed relabel policy. */
                paths[i] = lks_path_create(LKS_DIRECTION_POSITIVE,(unsigned int)(i/65536));
                REQUIRE(paths[i] && lks_path_append(paths[i],(unsigned int)(i%65536)) == LKS_STATUS_OK);
            } else if (!count) paths[i] = lks_path_create(LKS_DIRECTION_POSITIVE,32768);
            else if (!at) REQUIRE(lks_path_before(paths[sequence[0]],&paths[i]) == LKS_STATUS_OK);
            else if (at == count) REQUIRE(lks_path_after(paths[sequence[count-1]],&paths[i]) == LKS_STATUS_OK);
            else REQUIRE(lks_path_between(paths[sequence[at-1]],paths[sequence[at]],&paths[i]) == LKS_STATUS_OK);
            REQUIRE(paths[i] && lks_tree_insert(tree,paths[i],&items[i],&node) == LKS_STATUS_OK);
        }
        insert_ms += smoke_clock()-tick;
        memmove(sequence+at+1,sequence+at,(count-at)*sizeof(*sequence));
        sequence[at] = i; ++count;
    }
    tick = smoke_clock();
    if (v4) {
        const LksOrderHandle *handle = lks_order_first(order);
        for (i=0;i<count;++i) {
            REQUIRE(handle && lks_order_item(handle) == &items[sequence[i]]);
            checksum += (size_t)*(int *)lks_order_item(handle); handle = lks_order_next(handle);
        }
        REQUIRE(!handle);
    } else {
        REQUIRE(lks_tree_internal_fill_ordered(tree,nodes,count) == LKS_STATUS_OK);
        for (i=0;i<count;++i) {
            REQUIRE(lks_tree_node_item(nodes[i]) == &items[sequence[i]]);
            checksum += (size_t)*(int *)lks_tree_node_item(nodes[i]);
        }
    }
    traversal_ms = smoke_clock()-tick;
    tick = smoke_clock();
    for (i=0;i<queries;++i) {
        size_t a = smoke_random(&state)%count, b = smoke_random(&state)%count;
        int comparison;
        REQUIRE((v4 ? lks_order_compare(order,handles[sequence[a]],handles[sequence[b]],&comparison) :
            lks_path_compare(paths[sequence[a]],paths[sequence[b]],&comparison)) == LKS_STATUS_OK);
        REQUIRE(comparison == (a>b)-(a<b));
    }
    compare_ms = smoke_clock()-tick;
    /* Alternating front/back removals, oracle array maintenance untimed. */
    while (count) {
        size_t at = count&1 ? 0 : count-1, id = sequence[at];
        void *removed = NULL;
        tick = smoke_clock();
        REQUIRE((v4 ? lks_order_remove(order,handles[id],&removed) :
            lks_tree_remove_path(tree,paths[id],&removed)) == LKS_STATUS_OK && removed == &items[id]);
        remove_ms += smoke_clock()-tick;
        if (v4) handles[id] = NULL;
        else { lks_path_destroy(paths[id]); paths[id] = NULL; }
        memmove(sequence+at,sequence+at+1,(count-at-1)*sizeof(*sequence)); --count;
    }
    REQUIRE((v4 ? lks_order_size(order) : lks_tree_size(tree)) == 0);
    printf("%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%" PRIu64 "\n",name,v4?"V4":"V3",
        n,insert_ms,remove_ms,traversal_ms,compare_ms,checksum);
    lks_order_destroy(order); lks_tree_destroy(tree);
    free(sequence); free(items); free(paths); free(handles); free(nodes);
}
int main(void)
{
    int v4;
    puts("workload,engine,N,insert_total_ms,remove_total_ms,traversal_ms,compare_10000_ms,checksum");
    for(v4=0;v4<=1;++v4) {
        smoke_case("sequential_explicit_grid",100000,0,v4);
        smoke_case("same_neighbor",2000,1,v4);
        smoke_case("random_explicit",10000,2,v4);
    }
    return 0;
}
