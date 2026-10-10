#include "trace.h"
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "CHECK line=%d: %s\n", __LINE__, #x); failed = 1; goto done; } } while (0)
static int execute(Workbench *wb, const char *text, const char *expected)
{
    Command command;
    Result result;
    const char *error = command_parse(text, &command);
    ++wb->sequence;
    return wb_execute(wb, &command, error, &result) && !strcmp(result.status, expected);
}
int main(void)
{
    int failed = 0, relation = 9;
    size_t count = 0;
    Command c;
    Oracle model = {0};
    Workbench *wb = calloc(1, sizeof(*wb));
    LksMiniOrder *other = NULL;
    LksMiniHandle *foreign = NULL, *output = NULL;
    CHECK(wb && wb_init(wb));
    CHECK(wb_verify(wb));
    CHECK(command_parse("move-before 1 2", &c) == NULL && c.occurrence == 1 && c.anchor == 2);
    CHECK(command_parse("insert-after NULL 2", &c) == NULL && c.payload == 0 && c.anchor == 2);
    CHECK(!strcmp(command_parse("compare 1", &c), "APP_ARGUMENT_COUNT"));
    CHECK(!strcmp(command_parse("remove 18446744073709551616", &c), "APP_ID_FORMAT"));
    CHECK(!strcmp(command_parse("remove -1", &c), "APP_ID_FORMAT"));
    oracle_insert(&model, 0, (ModelEntry){1, 1});
    oracle_insert(&model, 1, (ModelEntry){2, 0});
    oracle_insert(&model, 1, (ModelEntry){3, 1});
    CHECK(model.count == 3 && model.entries[1].id == 3);
    oracle_move(&model, 1, CMD_MOVE_AFTER, 2);
    CHECK(model.entries[0].id == 3 && model.entries[1].id == 2 && model.entries[2].id == 1);
    CHECK(oracle_compare(&model, 3, 1) == -1 && oracle_compare(&model, 1, 3) == 1);
    oracle_move(&model, 2, CMD_MOVE_BEFORE, 2);
    CHECK(model.entries[1].id == 2);
    CHECK(execute(wb, "create Ink", "OK"));
    CHECK(execute(wb, "insert-back 1", "OK"));
    CHECK(execute(wb, "move-front 1", "OK"));
    CHECK(execute(wb, "move-back 1", "OK"));
    CHECK(execute(wb, "move-before 1 1", "OK"));
    CHECK(execute(wb, "move-after 1 1", "OK"));
    CHECK(execute(wb, "insert-front NULL", "OK"));
    CHECK(execute(wb, "insert-after 1 2", "OK"));
    CHECK(wb->model.count == 3 && wb->objects[0].references == 2);
    CHECK(wb->occurrences[0].handle != wb->occurrences[2].handle);
    CHECK(execute(wb, "remove 1", "OK"));
    CHECK(!wb->occurrences[0].handle && wb->objects[0].references == 1);
    CHECK(execute(wb, "move-front 1", "APP_RETIRED_OCCURRENCE"));
    CHECK(execute(wb, "insert-back 1", "OK"));
    CHECK(wb->issued == 4 && wb->objects[0].references == 2);
    CHECK(lks_mini_create(&other) == LKS_MINI_OK);
    CHECK(lks_mini_insert_back(other, NULL, &foreign) == LKS_MINI_OK);
    output = foreign;
    CHECK(lks_mini_insert_before(wb->order, foreign, NULL, &output) == LKS_MINI_WRONG_ORDER && !output);
    CHECK(lks_mini_move_before(wb->order, wb->occurrences[1].handle, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_remove(wb->order, foreign) == LKS_MINI_WRONG_ORDER);
    CHECK(lks_mini_compare(wb->order, foreign, wb->occurrences[1].handle, &relation) == LKS_MINI_WRONG_ORDER && relation == 0);
    CHECK(lks_mini_size(NULL, &count) == LKS_MINI_INVALID_ARGUMENT && count == 0);
    CHECK(lks_mini_move_before(wb->order, NULL, foreign) == LKS_MINI_INVALID_ARGUMENT);
    CHECK(wb_verify(wb));
    /* Demonstrate that the independent oracle detects a real public-API reorder
       which deliberately bypasses the consumer command/model path. */
    CHECK(lks_mini_move_back(wb->order, wb->occurrences[1].handle) == LKS_MINI_OK);
    CHECK(!wb_verify(wb));
    puts("Expected deliberate divergence detected; public foreign-order/ownership checks PASS");
done:
    lks_mini_destroy(other);
    if (wb) wb_destroy(wb);
    free(wb);
    return failed;
}
