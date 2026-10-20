/* A layer document: application IDs and payloads are separate from residents.
 * Only this adapter includes the public library header. History stores IDs,
 * never handles. The independent sequence model predicts each command first. */
#include <layerkeysort.h>
#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct Layer { size_t identity; char name[48]; int visible; } Layer;
typedef struct Residence { const LksOrderHandle *handle; Layer *layer; } Residence;
typedef struct History { LksSnapshot *snapshot; Sequence expected; } History;
typedef struct Document {
    LksOrder *order;
    Sequence expected;
    Residence residents[OCCURRENCE_LIMIT];
    Layer layers[256];
    History history[HISTORY_LIMIT];
    unsigned char association[8];
    size_t next_id;
    size_t capture_serial;
} Document;
static void fail(const char *s) { fprintf(stderr, "EVALUATOR_MISMATCH: %s\n", s); exit(2); }
static void require(int ok, const char *s) { if (!ok) fail(s); }
static size_t read_id(const void *data)
{
    const unsigned char *p = data; size_t i, id = 0;
    for (i = 0; i < 8; ++i) id = id * 256 + p[i];
    return id;
}
static LksStatus associate(void *item, uint64_t ordinal, void *ctx,
    const void **data, size_t *size)
{
    Document *d = ctx; size_t i, id = d->expected.ids[(size_t)ordinal];
    require(d->residents[id].layer == item, "capture callback occurrence");
    for (i = 8; i != 0; --i) { d->association[i-1] = (unsigned char)(id & 255); id >>= 8; }
    *data = d->association; *size = 8; return LKS_STATUS_OK;
}
static LksStatus resolve(const void *data, size_t size, uint64_t ordinal,
    void *ctx, void **out)
{
    Document *d = ctx; size_t id; (void)ordinal;
    if (size != 8) return LKS_STATUS_INVALID_ARGUMENT;
    id = read_id(data);
    if (!id || id >= d->next_id || !d->residents[id].layer) return LKS_STATUS_NOT_FOUND;
    *out = d->residents[id].layer; return LKS_STATUS_OK;
}
static void verify(Document *d)
{
    const LksOrderHandle *h = lks_order_first(d->order); size_t i, j;
    require(lks_order_size(d->order) == d->expected.count, "count");
    for (i = 0; i < d->expected.count; ++i) {
        Residence *r = &d->residents[d->expected.ids[i]];
        require(h && h == r->handle && lks_order_item(h) == r->layer, "forward/identity/payload");
        h = lks_order_next(h);
    }
    require(h == NULL, "forward endpoint"); h = lks_order_last(d->order);
    for (i = d->expected.count; i != 0; --i) {
        require(h == d->residents[d->expected.ids[i-1]].handle, "reverse");
        h = lks_order_previous(h);
    }
    require(h == NULL, "reverse endpoint");
    /* Exhaustive for small documents, deterministic sparse pairs otherwise. */
    for (i = 0; i < d->expected.count; i += d->expected.count > 32 ? 17 : 1)
        for (j = 0; j < d->expected.count; j += d->expected.count > 32 ? 19 : 1) {
            int c = 99;
            require(lks_order_compare(d->order, d->residents[d->expected.ids[i]].handle,
                d->residents[d->expected.ids[j]].handle, &c) == LKS_STATUS_OK,
                "compare status");
            require((c > 0) - (c < 0) == (i > j) - (i < j), "compare rank");
        }
}
static void history_check(const History *h)
{
    size_t i;
    require(lks_snapshot_count(h->snapshot) == h->expected.count, "history count");
    for (i = 0; i < h->expected.count; ++i) {
        const void *p = NULL; size_t n = 0;
        require(lks_snapshot_association(h->snapshot, i, &p, &n) == LKS_STATUS_OK &&
            n == 8 && read_id(p) == h->expected.ids[i], "immutable historical association");
    }
}
static int number(const char *s, size_t *out)
{
    char *end; unsigned long long v;
    if (!s || *s < '0' || *s > '9') return 0;
    errno = 0; v = strtoull(s, &end, 10);
    if (errno || *end || v > SIZE_MAX) return 0;
    *out = (size_t)v; return 1;
}
static int command(Document *d, char *line)
{
    char *t[6], *p = strtok(line, " \t\r\n"); size_t n = 0, a = 0, b = 0;
    Sequence next = d->expected; LksStatus status; const LksOrderHandle *h = NULL;
    while (p && n < 6) { t[n++] = p; p = strtok(NULL, " \t\r\n"); }
    if (!n) return 1;
    if (!strcmp(t[0], "list") && n == 1) return 1;
    if (!strcmp(t[0], "show") && n == 1) {
        size_t i;
        for (i = 0; i < d->expected.count; ++i) {
            size_t id = d->expected.ids[i]; Layer *layer = d->residents[id].layer;
            printf("layer occurrence=%zu object=%zu name=%s visible=%d\n",
                id, layer->identity, layer->name, layer->visible);
        }
        return 1;
    }
    if (!strcmp(t[0], "rename") && n == 3 && number(t[1], &a) && a < 256) {
        if (!t[2][0] || strlen(t[2]) >= sizeof(d->layers[a].name)) return 0;
        (void)snprintf(d->layers[a].name, sizeof(d->layers[a].name), "%s", t[2]);
        return 1;
    }
    if (!strcmp(t[0], "visible") && n == 3 && number(t[1], &a) && a < 256 && number(t[2], &b) && b <= 1) {
        d->layers[a].visible = (int)b; return 1;
    }
    if (!strcmp(t[0], "help") && n == 1) {
        puts("rename <object:0..255> <single-word-name>; visible <object> 0|1; add <object> front|back; insert <object> before|after <occurrence>; move <occurrence> front|back|before|after [anchor]; remove <occurrence>; capture <slot:0..31>; save|load <slot> <file>; restore <slot>; list; show; quit"); return 1;
    }
    if (!strcmp(t[0], "quit") && n == 1) return 2;
    if ((!strcmp(t[0], "add") && n == 3) || (!strcmp(t[0], "insert") && n == 4)) {
        size_t pos;
        if (!number(t[1], &a) || a >= 256 || d->next_id >= OCCURRENCE_LIMIT) return 0;
        if (!strcmp(t[0], "add")) {
            if (!strcmp(t[2], "front")) pos = 0;
            else if (!strcmp(t[2], "back")) pos = next.count;
            else return 0;
        } else {
            if (!number(t[3], &b) || b >= d->next_id || !d->residents[b].handle) return 0;
            pos = sequence_find(&next, b);
            if (!strcmp(t[2], "after")) ++pos;
            else if (strcmp(t[2], "before")) return 0;
        }
        if (!sequence_insert(&next, d->next_id, pos)) return 0;
        if (!strcmp(t[0], "add")) status = !strcmp(t[2], "front") ?
            lks_order_insert_front(d->order, &d->layers[a], &h) : lks_order_insert_back(d->order, &d->layers[a], &h);
        else status = !strcmp(t[2], "before") ?
            lks_order_insert_before(d->order, d->residents[b].handle, &d->layers[a], &h) :
            lks_order_insert_after(d->order, d->residents[b].handle, &d->layers[a], &h);
        if (status != LKS_STATUS_OK) return 0;
        d->residents[d->next_id++] = (Residence){h, &d->layers[a]}; d->expected = next; return 1;
    }
    if (!strcmp(t[0], "remove") && n == 2) {
        void *item = NULL;
        if (!number(t[1], &a) || a >= d->next_id || !d->residents[a].handle) return 0;
        require(sequence_remove(&next, a), "model remove");
        status = lks_order_remove(d->order, d->residents[a].handle, &item);
        if (status != LKS_STATUS_OK) return 0;
        require(item == d->residents[a].layer, "removed payload");
        d->residents[a].handle = NULL; d->expected = next; return 1;
    }
    if (!strcmp(t[0], "move") && (n == 3 || n == 4)) {
        if (!number(t[1], &a) || a >= d->next_id || !d->residents[a].handle) return 0;
        if (n == 4 && (!number(t[3], &b) || b >= d->next_id || !d->residents[b].handle)) return 0;
        if ((n == 3 && strcmp(t[2], "front") && strcmp(t[2], "back")) ||
            (n == 4 && strcmp(t[2], "before") && strcmp(t[2], "after"))) return 0;
        require(sequence_move(&next, a, b, t[2]), "model move");
        if (!strcmp(t[2], "front")) status = lks_order_move_front(d->order, d->residents[a].handle);
        else if (!strcmp(t[2], "back")) status = lks_order_move_back(d->order, d->residents[a].handle);
        else if (!strcmp(t[2], "before")) status = lks_order_move_before(d->order, d->residents[a].handle, d->residents[b].handle);
        else status = lks_order_move_after(d->order, d->residents[a].handle, d->residents[b].handle);
        if (status != LKS_STATUS_OK) return 0;
        d->expected = next; return 1;
    }
    if (n >= 2 && number(t[1], &a) && a < HISTORY_LIMIT) {
        History *hist = &d->history[a];
        if (!strcmp(t[0], "capture") && n == 2) {
            char ns[64]; LksSnapshot *s = NULL; LksSnapshotOptions options;
            if (d->capture_serial == SIZE_MAX) return 0;
            /* Each historical state needs a distinct domain. These demonstration
             * domains are process-local; a deployed editor must also supply a
             * persistent document/session identity before mixing exported keys. */
            (void)snprintf(ns, sizeof(ns), "evaluation-state-%zu", ++d->capture_serial);
            options = (LksSnapshotOptions){ns, strlen(ns), associate, d};
            status = lks_order_snapshot_capture(d->order, &options, &s);
            if (status != LKS_STATUS_OK) return 0;
            lks_snapshot_destroy(hist->snapshot); hist->snapshot = s; hist->expected = d->expected;
            require(lks_order_snapshot_is_current(d->order, s), "capture provenance");
            history_check(hist); return 1;
        }
        if (!strcmp(t[0], "restore") && n == 2 && hist->snapshot) {
            LksOrder *restored = NULL; size_t i;
            history_check(hist);
            status = lks_snapshot_restore_order(hist->snapshot, resolve, d, &restored);
            if (status != LKS_STATUS_OK) return 0;
            lks_order_destroy(d->order); d->order = restored;
            for (i = 1; i < d->next_id; ++i) d->residents[i].handle = NULL;
            d->expected = hist->expected; h = lks_order_first(restored);
            for (i = 0; i < d->expected.count; ++i) {
                d->residents[d->expected.ids[i]].handle = h; h = lks_order_next(h);
            }
            require(!lks_order_snapshot_is_current(restored, hist->snapshot), "fresh restore provenance"); return 1;
        }
        if ((!strcmp(t[0], "save") || !strcmp(t[0], "load")) && n == 3) {
            FILE *file; unsigned char *bytes; size_t size; int ok = 0;
            if (!strcmp(t[0], "save")) {
                if (!hist->snapshot) return 0;
                size = lks_snapshot_serialized_size(hist->snapshot);
                bytes = malloc(size); if (!bytes) return 0;
                status = lks_snapshot_serialize(hist->snapshot, bytes, size);
                file = status == LKS_STATUS_OK ? fopen(t[2], "wb") : NULL;
                if (file) { ok = fwrite(bytes, 1, size, file) == size; if (fclose(file)) ok = 0; }
            } else {
                long length; LksSnapshot *loaded = NULL; Sequence expected = {{0},0}; size_t i;
                file = fopen(t[2], "rb"); if (!file) return 0;
                if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 || length > 16777216 || fseek(file, 0, SEEK_SET)) { fclose(file); return 0; }
                size = (size_t)length; bytes = malloc(size ? size : 1);
                if (!bytes) { fclose(file); return 0; }
                ok = fread(bytes, 1, size, file) == size; fclose(file);
                if (ok) ok = lks_snapshot_deserialize(bytes, size, &loaded) == LKS_STATUS_OK;
                if (ok && lks_snapshot_count(loaded) <= DOCUMENT_LIMIT) {
                    for (i = 0; i < lks_snapshot_count(loaded) && ok; ++i) {
                        const void *data = NULL; size_t len = 0, id;
                        ok = lks_snapshot_association(loaded, i, &data, &len) == LKS_STATUS_OK && len == 8;
                        if (!ok) break;
                        id = read_id(data);
                        ok = id && id < d->next_id && sequence_find(&expected, id) == expected.count;
                        if (ok) ok = sequence_insert(&expected, id, expected.count);
                    }
                } else ok = 0;
                if (ok) { lks_snapshot_destroy(hist->snapshot); hist->snapshot = loaded; hist->expected = expected; }
                else lks_snapshot_destroy(loaded);
            }
            free(bytes); return ok;
        }
    }
    return 0;
}
int main(void)
{
    Document *d = calloc(1, sizeof(*d)); char line[1024]; size_t commands = 0, i;
    if (!d) return 3;
    d->order = lks_order_create(); if (!d->order) { free(d); return 3; }
    d->next_id = 1;
    for (i = 0; i < 256; ++i) {
        d->layers[i].identity = i; d->layers[i].visible = 1;
        (void)snprintf(d->layers[i].name, sizeof(d->layers[i].name), "Layer-%zu", i);
    }
    while (fgets(line, sizeof(line), stdin)) {
        int result;
        if (!strchr(line, '\n') && !feof(stdin)) {
            int c; while ((c = getchar()) != '\n' && c != EOF) { }
            result = 0;
        } else result = command(d, line);
        verify(d); ++commands;
        printf("%s %zu", result ? "OK" : "ERROR", d->expected.count);
        for (i = 0; i < d->expected.count; ++i) printf(" %zu", d->expected.ids[i]);
        putchar('\n');
        if (result == 2) break;
    }
    lks_order_destroy(d->order);
    for (i = 0; i < HISTORY_LIMIT; ++i) {
        if (d->history[i].snapshot) history_check(&d->history[i]);
        lks_snapshot_destroy(d->history[i].snapshot);
    }
    fprintf(stderr, "generated-or-interactive-consumer commands=%zu verified; source destroyed before history\n", commands);
    free(d); return 0;
}
