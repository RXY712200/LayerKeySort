#include "trace.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
static int terminal_input(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdin));
#else
    return isatty(STDIN_FILENO);
#endif
}
static int ignored(const char *line)
{
    while (*line && isspace((unsigned char)*line)) ++line;
    return !*line || *line == '#';
}
int main(int argc, char **argv)
{
    const char *path = NULL, *trace_path = NULL, *source = "unspecified", *mode = "stdin";
    FILE *input = stdin, *trace = NULL;
    Workbench *wb = NULL;
    char raw[WB_LINE], *expected = NULL, *record = NULL;
    int i, replay = 0, failed = 1, prompted = 0, initialized = 0, footer = 0, quit_seen = 0;
    size_t errors = 0;
    for (i = 1; i < argc; ++i) {
        if ((!strcmp(argv[i], "--commands") || !strcmp(argv[i], "--replay")) && i + 1 < argc && !path) {
            replay = !strcmp(argv[i], "--replay"); path = argv[++i];
        } else if (!strcmp(argv[i], "--trace") && i + 1 < argc && !trace_path) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--source") && i + 1 < argc) source = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            puts("layer_order_workbench [--commands FILE | --replay TRACE] [--trace NEW_FILE]");
            puts("  --source human-terminal|codex-interactive|script|generated|unspecified");
            command_help(); return 0;
        } else { fputs("APP_OPTIONS: invalid/conflicting/missing option\n", stderr); goto cleanup; }
    }
    if (strcmp(source, "human-terminal") && strcmp(source, "codex-interactive") &&
        strcmp(source, "script") && strcmp(source, "generated") && strcmp(source, "unspecified")) {
        fputs("APP_SOURCE: unsupported provenance\n", stderr); goto cleanup;
    }
    prompted = !path && terminal_input();
    if ((!strcmp(source, "human-terminal") || !strcmp(source, "codex-interactive")) && !prompted) {
        fputs("APP_SOURCE: terminal provenance requires actual terminal stdin\n", stderr); goto cleanup;
    }
    if (path) { input = fopen(path, "rb"); mode = replay ? "replay" : "file"; }
    if (!input) { fputs("APP_INPUT_IO: cannot open input\n", stderr); goto cleanup; }
    if (replay && !trace_read_header(input)) { fputs("APP_TRACE_FORMAT: invalid header\n", stderr); goto cleanup; }
    /* Exclusive creation prevents clobbering input or an earlier evidence file. */
    if (trace_path) {
        trace = fopen(trace_path, "wx");
        if (!trace || !trace_header(trace, source, mode)) { fputs("APP_TRACE_IO: create failed (existing file?)\n", stderr); goto cleanup; }
    }
    wb = calloc(1, sizeof(*wb)); expected = malloc(WB_RECORD); record = malloc(WB_RECORD);
    if (!wb || !expected || !record) { fputs("APP_OUT_OF_MEMORY: startup buffers\n", stderr); goto cleanup; }
    if (!wb_init(wb)) { fputs("MINI_OUT_OF_MEMORY: order creation\n", stderr); goto cleanup; }
    initialized = 1;
    if (prompted) puts("Local Layer Order Workbench / Phase 1. Type help. IDs are not handles.");
    while (1) {
        const char *input_error = NULL, *parse_error;
        Command command;
        Result result;
        int got;
        if (prompted) { fputs("layer> ", stdout); fflush(stdout); }
        got = input_line(input, replay ? expected : raw, replay ? WB_RECORD : WB_LINE, &input_error);
        if (got < 0) { fputs("APP_INPUT_IO: read failed\n", stderr); goto cleanup; }
        if (!got) break;
        if (replay) {
            if (!input_error && !strncmp(expected, "LLOW-END\t", 9)) {
                (void)snprintf(record, WB_RECORD, "LLOW-END\t%zu", wb->sequence);
                if (strcmp(expected, record) || input_line(input, expected, WB_RECORD, &input_error) != 0) {
                    fputs("APP_TRACE_FORMAT: invalid footer/trailing data\n", stderr); goto cleanup;
                }
                footer = 1;
                break;
            }
            if (quit_seen) { fputs("APP_TRACE_FORMAT: command after quit\n", stderr); goto cleanup; }
            if (input_error || !trace_decode(expected, raw, &input_error)) {
                fputs("APP_TRACE_FORMAT: malformed record\n", stderr); goto cleanup;
            }
        } else if (!input_error && ignored(raw)) continue;
        ++wb->sequence;
        parse_error = command_parse(raw, &command);
        if (input_error) parse_error = input_error;
        if (!wb_execute(wb, &command, parse_error, &result)) goto cleanup;
        if (!trace_record(record, WB_RECORD, raw, input_error, &command, &result, wb)) {
            fputs("APP_TRACE_LIMIT: record exceeds bound\n", stderr); goto cleanup;
        }
        if (replay && strcmp(record, expected)) {
            fprintf(stderr, "APP_REPLAY_MISMATCH command=%zu expected=%s\nactual=%s\n", wb->sequence, expected, record);
            goto cleanup;
        }
        if (trace && (fprintf(trace, "%s\n", record) < 0 || fflush(trace))) {
            fputs("APP_TRACE_IO: write failed\n", stderr); goto cleanup;
        }
        printf("command=%zu %s status=%s object=%zu occ=%zu anchor=%zu size=%zu relation=%d\n",
               wb->sequence, command_name(command.kind), result.status, result.object,
               result.occurrence, result.anchor, result.after, result.relation);
        if (strcmp(result.status, "OK")) ++errors;
        else {
            if ((command.kind == CMD_LIST || command.kind == CMD_REVERSE) &&
                !wb_list(wb, command.kind == CMD_REVERSE)) goto cleanup;
            if (command.kind == CMD_HELP) command_help();
            if (command.kind == CMD_QUIT) {
                if (replay) quit_seen = 1;
                else break;
            }
        }
    }
    if (replay && !footer) { fputs("APP_TRACE_FORMAT: missing completion footer\n", stderr); goto cleanup; }
    if (trace && (fprintf(trace, "LLOW-END\t%zu\n", wb->sequence) < 0 || fflush(trace))) {
        fputs("APP_TRACE_IO: footer write failed\n", stderr); goto cleanup;
    }
    printf("VERIFIED commands=%zu application_or_library_errors=%zu live=%zu issued=%zu objects=%zu\n",
           wb->sequence, errors, wb->model.count, wb->issued, wb->object_count);
    failed = 0;
cleanup:
    if (initialized) wb_destroy(wb);
    free(wb); free(expected); free(record);
    if (input && input != stdin && fclose(input)) failed = 1;
    if (trace && fclose(trace)) failed = 1;
    return failed ? 1 : 0;
}
