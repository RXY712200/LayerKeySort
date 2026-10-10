#include "workbench.h"
#include <stdio.h>
#include <string.h>
static const char *mini_status(LksMiniStatus status)
{
    switch (status) {
    case LKS_MINI_OK: return "OK";
    case LKS_MINI_INVALID_ARGUMENT: return "MINI_INVALID_ARGUMENT";
    case LKS_MINI_WRONG_ORDER: return "MINI_WRONG_ORDER";
    case LKS_MINI_OUT_OF_MEMORY: return "MINI_OUT_OF_MEMORY";
    case LKS_MINI_CAPACITY_LIMIT: return "MINI_CAPACITY_LIMIT";
    default: return "MINI_UNKNOWN_STATUS";
    }
}
int wb_init(Workbench *wb)
{
    memset(wb, 0, sizeof(*wb));
    return lks_mini_create(&wb->order) == LKS_MINI_OK;
}
void wb_destroy(Workbench *wb)
{
    size_t i;
    /* Mini owns occurrences; application object storage outlives destruction. */
    lks_mini_destroy(wb->order);
    wb->order = NULL;
    for (i = 0; i < wb->issued; ++i) wb->occurrences[i].handle = NULL;
}
static size_t handle_id(const Workbench *wb, const LksMiniHandle *handle)
{
    size_t i;
    for (i = 0; i < wb->issued; ++i)
        if (wb->occurrences[i].handle && wb->occurrences[i].handle == handle) return i + 1u;
    return 0;
}
static int mismatch(const Workbench *wb, const char *reason, size_t expected_id)
{
    size_t i;
    LksMiniHandle *cursor = NULL;
    fprintf(stderr, "ORACLE_MISMATCH command=%zu occurrence=%zu reason=%s\nexpected:",
            wb->sequence, expected_id, reason);
    for (i = 0; i < wb->model.count; ++i) fprintf(stderr, " %zu", wb->model.entries[i].id);
    fputs("\nactual:", stderr);
    if (lks_mini_first(wb->order, &cursor) == LKS_MINI_OK) {
        for (i = 0; cursor && i <= WB_LIVE; ++i) {
            size_t id = handle_id(wb, cursor);
            if (!id) { fputs(" <unknown-live-handle>", stderr); break; }
            fprintf(stderr, " %zu", id);
            /* Never follow an unknown or retired handle even in diagnostics. */
            if (lks_mini_next(wb->order, cursor, &cursor) != LKS_MINI_OK) break;
        }
        if (cursor && i > WB_LIVE) fputs(" <bounded-traversal-limit>", stderr);
    }
    fputc('\n', stderr);
    return 0;
}
static int check_pair(const Workbench *wb, size_t i, size_t j)
{
    const ModelEntry *a = &wb->model.entries[i], *b = &wb->model.entries[j];
    int expected = i < j ? -1 : i > j ? 1 : 0, actual = 9;
    if (lks_mini_compare(wb->order, wb->occurrences[a->id - 1u].handle,
                         wb->occurrences[b->id - 1u].handle, &actual) != LKS_MINI_OK ||
        actual != expected) return mismatch(wb, "comparison", a->id);
    return 1;
}
int wb_verify(const Workbench *wb)
{
    size_t n = 0, i, j, live = 0, references[WB_OBJECTS] = {0};
    LksMiniHandle *cursor = NULL;
    if (lks_mini_size(wb->order, &n) != LKS_MINI_OK || n != wb->model.count)
        return mismatch(wb, "size", 0);
    for (i = 0; i < wb->issued; ++i) if (wb->occurrences[i].handle) ++live;
    if (live != n) return mismatch(wb, "active-map-count", 0);
    for (j = 0; j < 2; ++j) {
        LksMiniStatus status = j ? lks_mini_last(wb->order, &cursor) : lks_mini_first(wb->order, &cursor);
        if (status != LKS_MINI_OK) return mismatch(wb, "endpoint-status", 0);
        for (i = 0; i < n; ++i) {
            const ModelEntry *entry = &wb->model.entries[j ? n - i - 1u : i];
            const LiveOccurrence *occ = &wb->occurrences[entry->id - 1u];
            void *actual = NULL, *expected = entry->payload ? (void *)&wb->objects[entry->payload - 1u] : NULL;
            if (!cursor || cursor != occ->handle || occ->payload != entry->payload)
                return mismatch(wb, "sequence/handle/payload-ID", entry->id);
            if (lks_mini_item(wb->order, cursor, &actual) != LKS_MINI_OK || actual != expected)
                return mismatch(wb, "payload-pointer", entry->id);
            if (!j && entry->payload) ++references[entry->payload - 1u];
            status = j ? lks_mini_prev(wb->order, cursor, &cursor) : lks_mini_next(wb->order, cursor, &cursor);
            if (status != LKS_MINI_OK) return mismatch(wb, "traversal-status", entry->id);
        }
        if (cursor) return mismatch(wb, "extra-occurrence", 0);
    }
    for (i = 0; i < wb->object_count; ++i)
        if (wb->objects[i].id != i + 1u || wb->objects[i].references != references[i])
            return mismatch(wb, "object-ownership/accounting", 0);
    if (n <= 32) {
        for (i = 0; i < n; ++i) for (j = 0; j < n; ++j)
            if (!check_pair(wb, i, j)) return 0;
    } else {
        /* Deterministic 32 pairs, plus both endpoints and every self relation. */
        for (i = 0; i < 32; ++i)
            if (!check_pair(wb, (i * 61u + 7u) % n, (i * 127u + 3u) % n)) return 0;
        if (!check_pair(wb, 0, n - 1u) || !check_pair(wb, n - 1u, 0)) return 0;
        for (i = 0; i < n; ++i) if (!check_pair(wb, i, i)) return 0;
    }
    return 1;
}
static const char *live_error(const Workbench *wb, size_t id)
{
    if (!id || id > wb->issued) return "APP_UNKNOWN_OCCURRENCE";
    if (!wb->occurrences[id - 1u].handle) return "APP_RETIRED_OCCURRENCE";
    return NULL;
}
int wb_execute(Workbench *wb, const Command *c, const char *parse_error, Result *r)
{
    LksMiniStatus status = LKS_MINI_OK;
    LksMiniHandle *created = NULL, *handle = NULL, *anchor = NULL;
    Oracle planned = wb->model;
    void *payload = NULL;
    const char *error = parse_error;
    size_t position = 0, id = 0;
    memset(r, 0, sizeof(*r));
    r->object = c->payload; r->occurrence = c->occurrence; r->anchor = c->anchor;
    r->before = wb->model.count;
    if (!error && c->payload > wb->object_count) error = "APP_UNKNOWN_OBJECT";
    if (!error && c->occurrence) error = live_error(wb, c->occurrence);
    if (!error && c->anchor) error = live_error(wb, c->anchor);
    if (!error && c->kind == CMD_CREATE && wb->object_count == WB_OBJECTS) error = "APP_OBJECT_LIMIT";
    if (!error && c->kind >= CMD_INSERT_FRONT && c->kind <= CMD_INSERT_AFTER &&
        (wb->model.count == WB_LIVE || wb->issued == WB_OCCURRENCES)) error = "APP_OCCURRENCE_LIMIT";
    if (error) { r->status = error; r->after = r->before; return wb_verify(wb); }
    if (c->occurrence) handle = wb->occurrences[c->occurrence - 1u].handle;
    if (c->anchor) anchor = wb->occurrences[c->anchor - 1u].handle;
    if (c->payload) payload = &wb->objects[c->payload - 1u];
    if (c->kind >= CMD_INSERT_FRONT && c->kind <= CMD_INSERT_AFTER) {
        id = wb->issued + 1u;
        if (c->kind == CMD_INSERT_BACK) position = planned.count;
        if (c->kind == CMD_INSERT_BEFORE || c->kind == CMD_INSERT_AFTER)
            position = oracle_index(&planned, c->anchor) + (c->kind == CMD_INSERT_AFTER ? 1u : 0u);
        oracle_insert(&planned, position, (ModelEntry){id, c->payload});
    } else if (c->kind == CMD_REMOVE) oracle_remove(&planned, oracle_index(&planned, c->occurrence));
    else if (c->kind >= CMD_MOVE_FRONT && c->kind <= CMD_MOVE_AFTER)
        oracle_move(&planned, c->occurrence, c->kind, c->anchor);
    switch (c->kind) {
    case CMD_INSERT_FRONT: status = lks_mini_insert_front(wb->order, payload, &created); break;
    case CMD_INSERT_BACK: status = lks_mini_insert_back(wb->order, payload, &created); break;
    case CMD_INSERT_BEFORE: status = lks_mini_insert_before(wb->order, anchor, payload, &created); break;
    case CMD_INSERT_AFTER: status = lks_mini_insert_after(wb->order, anchor, payload, &created); break;
    case CMD_REMOVE: status = lks_mini_remove(wb->order, handle); break;
    case CMD_MOVE_FRONT: status = lks_mini_move_front(wb->order, handle); break;
    case CMD_MOVE_BACK: status = lks_mini_move_back(wb->order, handle); break;
    case CMD_MOVE_BEFORE: status = lks_mini_move_before(wb->order, handle, anchor); break;
    case CMD_MOVE_AFTER: status = lks_mini_move_after(wb->order, handle, anchor); break;
    case CMD_COMPARE:
        status = lks_mini_compare(wb->order, handle, anchor, &r->relation);
        if (status == LKS_MINI_OK && r->relation != oracle_compare(&wb->model, c->occurrence, c->anchor))
            return mismatch(wb, "requested-comparison", c->occurrence);
        break;
    default: break;
    }
    r->status = mini_status(status);
    if (status == LKS_MINI_OK) {
        if (id) {
            if (!created) return mismatch(wb, "successful-insert-null-handle", id);
            wb->issued = id;
            wb->occurrences[id - 1u] = (LiveOccurrence){created, c->payload};
            if (c->payload) ++wb->objects[c->payload - 1u].references;
            r->occurrence = id;
        } else if (c->kind == CMD_REMOVE) {
            LiveOccurrence *removed = &wb->occurrences[c->occurrence - 1u];
            if (removed->payload) --wb->objects[removed->payload - 1u].references;
            /* Retire immediately. Never retain the freed handle, including on errors. */
            removed->handle = NULL; removed->payload = 0;
        } else if (c->kind == CMD_CREATE) {
            LayerObject *object = &wb->objects[wb->object_count++];
            object->id = wb->object_count;
            memcpy(object->name, c->name, sizeof(object->name));
            r->object = object->id;
        }
        wb->model = planned;
    } else {
        if (created || (c->kind == CMD_COMPARE && r->relation != 0))
            return mismatch(wb, "error-output-default", c->occurrence);
        if (status != LKS_MINI_OUT_OF_MEMORY) return mismatch(wb, "unexpected-valid-operation-status", c->occurrence);
    }
    r->after = wb->model.count;
    return wb_verify(wb);
}
int wb_list(const Workbench *wb, int reverse)
{
    LksMiniHandle *cursor = NULL;
    size_t i;
    LksMiniStatus s = reverse ? lks_mini_last(wb->order, &cursor) : lks_mini_first(wb->order, &cursor);
    if (s != LKS_MINI_OK) return 0;
    for (i = 0; i < wb->model.count; ++i) {
        const ModelEntry *entry = &wb->model.entries[reverse ? wb->model.count - i - 1u : i];
        void *item = NULL;
        if (cursor != wb->occurrences[entry->id - 1u].handle ||
            lks_mini_item(wb->order, cursor, &item) != LKS_MINI_OK) return 0;
        if (item) {
            const LayerObject *object = item;
            printf("occ=%zu object=%zu name=%s\n", entry->id, object->id, object->name);
        } else printf("occ=%zu object=NULL\n", entry->id);
        s = reverse ? lks_mini_prev(wb->order, cursor, &cursor) : lks_mini_next(wb->order, cursor, &cursor);
        if (s != LKS_MINI_OK) return 0;
    }
    return cursor == NULL;
}
