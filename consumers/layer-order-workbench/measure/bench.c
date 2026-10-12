#include "clock.h"
#include "workload.h"
#include "trace.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { uint64_t api, map, oracle, io, init, execute, destroy; size_t calls, spans; } Timings;
static int call_api(Workbench *wb, const Action *a, LksMiniHandle *h, LksMiniHandle *anchor,
                    void *payload, LksMiniHandle **created, int *relation, size_t *calls)
{
    LksMiniStatus s = LKS_MINI_OK;
    LksMiniHandle *cursor = NULL;
    size_t i;
    ++*calls;
    switch (a->command.kind) {
    case CMD_INSERT_FRONT: s = lks_mini_insert_front(wb->order, payload, created); break;
    case CMD_INSERT_BACK: s = lks_mini_insert_back(wb->order, payload, created); break;
    case CMD_INSERT_BEFORE: s = lks_mini_insert_before(wb->order, anchor, payload, created); break;
    case CMD_INSERT_AFTER: s = lks_mini_insert_after(wb->order, anchor, payload, created); break;
    case CMD_REMOVE: s = lks_mini_remove(wb->order, h); break;
    case CMD_MOVE_FRONT: s = lks_mini_move_front(wb->order, h); break;
    case CMD_MOVE_BACK: s = lks_mini_move_back(wb->order, h); break;
    case CMD_MOVE_BEFORE: s = lks_mini_move_before(wb->order, h, anchor); break;
    case CMD_MOVE_AFTER: s = lks_mini_move_after(wb->order, h, anchor); break;
    case CMD_COMPARE: s = lks_mini_compare(wb->order, h, anchor, relation); break;
    case CMD_LIST: case CMD_REVERSE:
        s = a->command.kind == CMD_LIST ? lks_mini_first(wb->order, &cursor) : lks_mini_last(wb->order, &cursor);
        for (i = 0; s == LKS_MINI_OK && cursor && i < a->before; ++i) {
            s = a->command.kind == CMD_LIST ? lks_mini_next(wb->order, cursor, &cursor) : lks_mini_prev(wb->order, cursor, &cursor);
            ++*calls;
        }
        if (cursor || i != a->before) return 0;
        break;
    default: return 0;
    }
    return s == LKS_MINI_OK && *relation == a->relation;
}
static void commit_map(Workbench *wb, const Action *a, LksMiniHandle *created)
{
    if (a->new_id) {
        wb->issued = a->new_id;
        wb->occurrences[a->new_id - 1u] = (LiveOccurrence){created, a->command.payload};
        if (a->command.payload) ++wb->objects[a->command.payload - 1u].references;
    } else if (a->command.kind == CMD_REMOVE) {
        LiveOccurrence *occ = &wb->occurrences[a->command.occurrence - 1u];
        if (occ->payload) --wb->objects[occ->payload - 1u].references;
        occ->handle = NULL; occ->payload = 0;
    }
}
static int run(const Workload *w, int pipeline, Timings *t)
{
    Workbench *wb = NULL;
    FILE *trace = NULL, *output = NULL;
    char record[WB_RECORD];
    uint64_t start, p;
    size_t i;
    int ok = 0;
    memset(t, 0, sizeof(*t));
    start = measure_now();
    wb = calloc(1, sizeof(*wb));
    if (!wb || !workload_setup(wb, w->initial)) goto done;
    if (pipeline) { trace = tmpfile(); output = tmpfile(); if (!trace || !output || !trace_header(trace, "generated", "file")) goto done; }
    t->init = measure_now() - start;
    if (!wb_verify(wb)) goto done; /* Outside timed execution in both modes. */
    start = measure_now();
    for (i = 0; i < w->count; ++i) {
        const Action *a = &w->actions[i];
        const Command *c = &a->command;
        Command parsed;
        LksMiniHandle *h, *anchor, *created = NULL;
        void *payload;
        int relation = 0;
        Result r = {0};
        if (pipeline) {
            p = measure_now();
            if (command_parse(a->raw, &parsed) || parsed.kind != c->kind || parsed.payload != c->payload ||
                parsed.occurrence != c->occurrence || parsed.anchor != c->anchor) goto done;
            t->io += measure_now() - p; ++t->spans;
        }
        p = measure_now();
        /* Same direct-index ID/handle/object mapping used by Phase 1. */
        h = c->occurrence ? wb->occurrences[c->occurrence - 1u].handle : NULL;
        anchor = c->anchor ? wb->occurrences[c->anchor - 1u].handle : NULL;
        payload = c->payload ? (void *)&wb->objects[c->payload - 1u] : NULL;
        t->map += measure_now() - p; ++t->spans;
        p = measure_now();
        if (!call_api(wb, a, h, anchor, payload, &created, &relation, &t->calls)) goto done;
        t->api += measure_now() - p; ++t->spans;
        p = measure_now();
        commit_map(wb, a, created);
        t->map += measure_now() - p; ++t->spans;
        ++wb->sequence;
        if (pipeline) {
            p = measure_now();
            workload_model(&wb->model, a);
            if (!wb_verify(wb)) goto done;
            t->oracle += measure_now() - p; ++t->spans;
            p = measure_now();
            r.status = "OK"; r.object = c->payload; r.occurrence = a->new_id ? a->new_id : c->occurrence;
            r.anchor = c->anchor; r.before = a->before; r.after = a->after; r.relation = relation;
            if (!trace_record(record, sizeof(record), a->raw, NULL, c, &r, wb) ||
                fprintf(trace, "%s\n", record) < 0 || fflush(trace) ||
                fprintf(output, "command=%zu %s status=OK object=%zu occ=%zu anchor=%zu size=%zu relation=%d\n",
                        wb->sequence, command_name(c->kind), r.object, r.occurrence, r.anchor, r.after, r.relation) < 0) goto done;
            if (c->kind == CMD_LIST || c->kind == CMD_REVERSE) {
                size_t j;
                for (j = 0; j < wb->model.count; ++j) {
                    const ModelEntry *entry = &wb->model.entries[c->kind == CMD_LIST ? j : wb->model.count - 1u - j];
                    if (fprintf(output, "occ=%zu object=%zu\n", entry->id, entry->payload) < 0) goto done;
                }
            }
            if (fflush(output)) goto done;
            t->io += measure_now() - p; ++t->spans;
        }
    }
    t->execute = measure_now() - start;
    if (!pipeline) for (i = 0; i < w->count; ++i) workload_model(&wb->model, &w->actions[i]);
    if (!wb_verify(wb) || wb->model.count != w->final) goto done; /* Outside timing. */
    start = measure_now();
    wb_destroy(wb); free(wb); wb = NULL;
    if (trace && fclose(trace)) { trace = NULL; goto done; } trace = NULL;
    if (output && fclose(output)) { output = NULL; goto done; } output = NULL;
    t->destroy = measure_now() - start;
    ok = 1;
done:
    if (wb) { wb_destroy(wb); free(wb); }
    if (trace) (void)fclose(trace);
    if (output) (void)fclose(output);
    return ok;
}
static void emit(const Workload *w)
{
    size_t i;
    puts("create Layer1\ncreate Layer2");
    for (i = 0; i < w->initial; ++i) {
        if (i % 3u) printf("insert-back %zu\n", i % 3u); else puts("insert-back NULL");
    }
    for (i = 0; i < w->count; ++i) puts(w->actions[i].raw);
    puts("quit");
}
int main(int argc, char **argv)
{
    size_t k, n, steps = 64;
    Workload *w = calloc(1, sizeof(*w));
    int i, failed = 1;
    uint64_t overhead;
    if (!w) return 1;
    if (argc == 2 && !strcmp(argv[1], "--verify-all")) {
        const size_t sizes[] = {16, 128, 2048};
        for (k = 0; k < MEASURE_CASES; ++k) for (n = 0; n < 3; ++n)
            if (!workload_make(w, k, sizes[n], steps) || !workload_correct(w)) goto done;
        puts("All 36 generated workloads: independent stepwise correctness PASS; retirement recovery PASS");
        failed = 0; goto done;
    }
    if (argc != 4) { fputs("usage: workbench_measure CASE SIZE isolated|pipeline|emit\n", stderr); goto done; }
    for (k = 0; k < MEASURE_CASES && strcmp(argv[1], measure_case(k)); ++k) {}
    if (!strcmp(argv[2], "16")) n = 16;
    else if (!strcmp(argv[2], "128")) n = 128;
    else if (!strcmp(argv[2], "2048")) n = 2048;
    else goto done;
    if (!workload_make(w, k, n, steps)) goto done;
    if (!strcmp(argv[3], "emit")) { emit(w); failed = 0; goto done; }
    if (strcmp(argv[3], "isolated") && strcmp(argv[3], "pipeline")) goto done;
    overhead = measure_empty_pair();
    puts("case,size,mode,phase,commands,initial,min_live,max_live,final,api_calls,compares,forward_compares,reverse_compares,distance_sum,distance_max,clock,resolution_ns,empty_pair_median_ns,timer_spans,init_ns,api_ns,map_ns,oracle_ns,parse_trace_output_ns,execute_ns,destroy_ns,compiler,build,sanitized,observed_step_ns,zero_pairs_of_1001");
    for (i = 0; i < 2; ++i) {
        Timings t;
        if (!run(w, !strcmp(argv[3], "pipeline"), &t)) goto done;
        printf("%s,%zu,%s,%s,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%s,",
            measure_case(k), n, argv[3], i ? "measured" : "warmup", w->count, w->initial,
            w->minimum, w->maximum, w->final, t.calls, w->compares, w->forward, w->reverse,
            w->distance_sum, w->distance_max, measure_clock_name());
        printf("%" PRIu64 ",%" PRIu64 ",%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s,%s,%d,%" PRIu64 ",%zu\n",
            measure_resolution(), overhead, t.spans, t.init, t.api, t.map, t.oracle, t.io, t.execute, t.destroy,
            MEASURE_COMPILER, MEASURE_BUILD, MEASURE_SANITIZED, measure_observed_step(), measure_zero_pairs());
    }
    failed = 0;
done:
    free(w);
    return failed;
}
