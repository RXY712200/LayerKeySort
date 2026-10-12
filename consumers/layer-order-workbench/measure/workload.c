#include "workload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *cases[MEASURE_CASES] = {
    "sequential-insert", "head-tail-insert", "adjacent-move", "distant-move",
    "valid-noop", "remove-reinsert", "forward-traverse", "reverse-traverse",
    "self-compare", "near-compare", "far-compare", "mixed-editor"
};
const char *measure_case(size_t index) { return index < MEASURE_CASES ? cases[index] : NULL; }
void workload_model(Oracle *m, const Action *a)
{
    const Command *c = &a->command;
    if (a->new_id) {
        size_t p = c->kind == CMD_INSERT_FRONT ? 0 : m->count;
        if (c->kind == CMD_INSERT_BEFORE || c->kind == CMD_INSERT_AFTER)
            p = oracle_index(m, c->anchor) + (c->kind == CMD_INSERT_AFTER ? 1u : 0u);
        oracle_insert(m, p, (ModelEntry){a->new_id, c->payload});
    } else if (c->kind == CMD_REMOVE) oracle_remove(m, oracle_index(m, c->occurrence));
    else if (c->kind >= CMD_MOVE_FRONT && c->kind <= CMD_MOVE_AFTER)
        oracle_move(m, c->occurrence, c->kind, c->anchor);
}
static void raw_command(Action *a)
{
    const Command *c = &a->command;
    char payload[32];
    if (c->payload) (void)snprintf(payload, sizeof(payload), "%zu", c->payload);
    else (void)snprintf(payload, sizeof(payload), "NULL");
    if (c->kind == CMD_INSERT_FRONT || c->kind == CMD_INSERT_BACK)
        (void)snprintf(a->raw, sizeof(a->raw), "%s %s", command_name(c->kind), payload);
    else if (c->kind == CMD_INSERT_BEFORE || c->kind == CMD_INSERT_AFTER)
        (void)snprintf(a->raw, sizeof(a->raw), "%s %s %zu", command_name(c->kind), payload, c->anchor);
    else if (c->kind == CMD_COMPARE || c->kind == CMD_MOVE_BEFORE || c->kind == CMD_MOVE_AFTER)
        (void)snprintf(a->raw, sizeof(a->raw), "%s %zu %zu", command_name(c->kind), c->occurrence, c->anchor);
    else if (c->occurrence) (void)snprintf(a->raw, sizeof(a->raw), "%s %zu", command_name(c->kind), c->occurrence);
    else (void)snprintf(a->raw, sizeof(a->raw), "%s", command_name(c->kind));
}
int workload_make(Workload *w, size_t k, size_t n, size_t steps)
{
    Oracle m = {0};
    size_t i, issued = 0, count;
    if (k >= MEASURE_CASES || n < 2 || n > WB_LIVE || steps < 2 || steps > 1024) return 0;
    memset(w, 0, sizeof(*w));
    w->initial = k < 2 ? 0 : n;
    for (i = 0; i < w->initial; ++i) oracle_insert(&m, m.count, (ModelEntry){++issued, i % 3u});
    w->minimum = w->maximum = w->initial;
    /* Mixed runs finish a complete 12-command edit cycle, including reinsertion. */
    count = k < 2 ? n : k == 11 ? ((steps + 11u) / 12u) * 12u : steps;
    for (i = 0; i < count; ++i) {
        Action *a = &w->actions[w->count++];
        Command *c = &a->command;
        size_t middle = m.count / 2u;
        a->before = m.count;
        if (k < 2) { c->kind = k == 1 && i % 2u ? CMD_INSERT_FRONT : CMD_INSERT_BACK; c->payload = i % 3u; }
        else if (k == 2) {
            c->kind = i % 2u ? CMD_MOVE_AFTER : CMD_MOVE_BEFORE;
            c->occurrence = m.entries[middle].id;
            c->anchor = m.entries[i % 2u ? middle + 1u : middle - 1u].id;
        } else if (k == 3) { c->kind = i % 2u ? CMD_MOVE_BACK : CMD_MOVE_FRONT; c->occurrence = m.entries[middle].id; }
        else if (k == 4) {
            c->kind = CMD_MOVE_BEFORE; c->occurrence = m.entries[middle].id;
            c->anchor = i % 2u ? c->occurrence : m.entries[middle + 1u].id;
        } else if (k == 5) {
            c->kind = i % 2u ? CMD_INSERT_BACK : CMD_REMOVE;
            if (i % 2u) c->payload = (i / 2u) % 3u;
            else c->occurrence = m.entries[middle].id;
        } else if (k == 6 || k == 7) c->kind = k == 6 ? CMD_LIST : CMD_REVERSE;
        else if (k >= 8 && k <= 10) {
            size_t p = i % (m.count - 1u), q = p;
            if (k == 9) q = p + 1u;
            if (k == 10) { p = 0; q = m.count - 1u; }
            c->kind = CMD_COMPARE;
            c->occurrence = m.entries[i % 2u ? q : p].id;
            c->anchor = m.entries[i % 2u ? p : q].id;
        } else {
            switch (i % 12u) {
            case 0: c->kind = CMD_MOVE_FRONT; c->occurrence = m.entries[middle].id; break;
            case 1: c->kind = CMD_MOVE_BACK; c->occurrence = m.entries[0].id; break;
            case 2: c->kind = CMD_COMPARE; c->occurrence = m.entries[0].id; c->anchor = m.entries[m.count - 1u].id; break;
            case 3: c->kind = CMD_REMOVE; c->occurrence = m.entries[m.count / 4u].id; break;
            case 4: c->kind = CMD_INSERT_BACK; c->payload = i % 3u; break;
            case 5: c->kind = CMD_MOVE_BEFORE; c->occurrence = m.entries[middle].id; c->anchor = m.entries[middle - 1u].id; break;
            case 6: c->kind = CMD_MOVE_AFTER; c->occurrence = m.entries[0].id; c->anchor = m.entries[m.count - 1u].id; break;
            case 7: c->kind = CMD_COMPARE; c->occurrence = c->anchor = m.entries[middle].id; break;
            case 8: c->kind = CMD_LIST; break;
            case 9: c->kind = CMD_REVERSE; break;
            case 10: c->kind = CMD_MOVE_BEFORE; c->occurrence = c->anchor = m.entries[0].id; break;
            default: c->kind = CMD_COMPARE; c->occurrence = m.entries[0].id; c->anchor = m.entries[1].id; break;
            }
        }
        if (c->kind == CMD_COMPARE) {
            size_t p = oracle_index(&m, c->occurrence), q = oracle_index(&m, c->anchor);
            a->relation = (p > q) - (p < q);
            a->distance = p > q ? p - q : q - p;
            ++w->compares;
            w->forward += p < q ? 1u : 0u; w->reverse += p > q ? 1u : 0u;
            w->distance_sum += a->distance;
            if (a->distance > w->distance_max) w->distance_max = a->distance;
        }
        if (c->kind >= CMD_INSERT_FRONT && c->kind <= CMD_INSERT_AFTER) a->new_id = ++issued;
        raw_command(a);
        workload_model(&m, a);
        a->after = m.count;
        if (m.count < w->minimum) w->minimum = m.count;
        if (m.count > w->maximum) w->maximum = m.count;
    }
    w->final = m.count;
    return w->count <= MEASURE_STEPS && issued <= WB_OCCURRENCES;
}
int workload_setup(Workbench *wb, size_t n)
{
    size_t i;
    if (!wb_init(wb)) return 0;
    wb->object_count = 2;
    for (i = 0; i < 2; ++i) { wb->objects[i].id = i + 1u; (void)snprintf(wb->objects[i].name, WB_NAME, "Layer%zu", i + 1u); }
    for (i = 0; i < n; ++i) {
        size_t payload = i % 3u;
        LksMiniHandle *h = NULL;
        if (lks_mini_insert_back(wb->order, payload ? &wb->objects[payload - 1u] : NULL, &h) != LKS_MINI_OK) return 0;
        wb->occurrences[i] = (LiveOccurrence){h, payload};
        if (payload) ++wb->objects[payload - 1u].references;
        oracle_insert(&wb->model, wb->model.count, (ModelEntry){i + 1u, payload});
        ++wb->issued;
    }
    return 1;
}
int workload_correct(const Workload *w)
{
    Workbench *wb = calloc(1, sizeof(*wb));
    size_t i;
    int ok = 0;
    if (!wb || !workload_setup(wb, w->initial) || !wb_verify(wb)) goto done;
    for (i = 0; i < w->count; ++i) {
        Command c;
        Result r;
        const Action *a = &w->actions[i];
        const char *error = command_parse(a->raw, &c);
        ++wb->sequence;
        if (error || !wb_execute(wb, &c, NULL, &r) || strcmp(r.status, "OK") ||
            r.before != a->before || r.after != a->after || r.relation != a->relation ||
            (a->new_id && r.occurrence != a->new_id)) goto done;
    }
    /* A retired ID must be rejected by the application, without a freed call. */
    if (wb->model.count) {
        Command c = {0}; Result r;
        c.kind = CMD_REMOVE; c.occurrence = wb->model.entries[0].id;
        if (!wb_execute(wb, &c, NULL, &r) || strcmp(r.status, "OK")) goto done;
        c.kind = CMD_MOVE_FRONT;
        if (!wb_execute(wb, &c, NULL, &r) || strcmp(r.status, "APP_RETIRED_OCCURRENCE")) goto done;
    }
    ok = 1;
done:
    if (wb) { wb_destroy(wb); free(wb); }
    return ok;
}
