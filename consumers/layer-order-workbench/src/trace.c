#include "trace.h"
#include <stdarg.h>
#include <string.h>
static int source_valid(const char *source)
{
    return !strcmp(source, "human-terminal") || !strcmp(source, "codex-interactive") ||
           !strcmp(source, "script") || !strcmp(source, "generated") || !strcmp(source, "unspecified");
}
int trace_header(FILE *file, const char *source, const char *mode)
{
    if (!source_valid(source)) return 0;
    return fprintf(file, "LLOW\t1\t%s\t%s\n", source, mode) > 0 && fflush(file) == 0;
}
int input_line(FILE *file, char *line, size_t capacity, const char **error)
{
    int c, seen = 0;
    size_t n = 0;
    *error = NULL;
    while ((c = fgetc(file)) != EOF && c != '\n') {
        seen = 1;
        /* Resolve the line ending before the content-length check. Otherwise a
           legal 255-byte command followed by CRLF appears to have 256 bytes.
           Only the final CR is removed; an earlier CR remains input content. */
        if (c == '\r') {
            int following = fgetc(file);
            if (following == '\n' || following == EOF) { c = following; break; }
            if (ungetc(following, file) == EOF) return -1;
        }
        if (c == 0) { *error = "APP_INPUT_FORMAT"; continue; }
        if (n + 1u < capacity) line[n++] = (char)c;
        else if (!*error) *error = "APP_LINE_LIMIT";
    }
    if (ferror(file)) return -1;
    if (c == EOF && !seen) return 0;
    line[n] = '\0';
    return 1;
}
int trace_read_header(FILE *file)
{
    char line[128], source[48], mode[16];
    const char *error = NULL;
    int consumed = 0;
    if (input_line(file, line, sizeof(line), &error) != 1 || error) return 0;
    if (sscanf(line, "LLOW\t1\t%47[^\t]\t%15s%n", source, mode, &consumed) != 2 ||
        line[consumed] || !source_valid(source)) return 0;
    return !strcmp(mode, "stdin") || !strcmp(mode, "file") || !strcmp(mode, "replay");
}
static int append(char *out, size_t capacity, size_t *used, const char *format, ...)
{
    int written;
    va_list args;
    if (*used >= capacity) return 0;
    va_start(args, format);
    written = vsnprintf(out + *used, capacity - *used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= capacity - *used) return 0;
    *used += (size_t)written;
    return 1;
}
int trace_record(char *out, size_t capacity, const char *raw, const char *input_error,
                 const Command *c, const Result *r, const Workbench *wb)
{
    size_t i, used = 0;
    if (!append(out, capacity, &used, "%zu\t%s\t%s\t%zu\t%zu\t%zu\t%zu\t%s\t%zu\t%d\t",
                wb->sequence, command_name(c->kind), input_error ? input_error : "-",
                r->object, r->occurrence, r->anchor, r->before, r->status, r->after, r->relation)) return 0;
    for (i = 0; raw[i]; ++i)
        if (!append(out, capacity, &used, "%02x", (unsigned)(unsigned char)raw[i])) return 0;
    if (!append(out, capacity, &used, "\t")) return 0;
    if (!wb->model.count && !append(out, capacity, &used, "-")) return 0;
    for (i = 0; i < wb->model.count; ++i)
        if (!append(out, capacity, &used, "%s%zu", i ? "," : "", wb->model.entries[i].id)) return 0;
    return 1;
}
static int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
int trace_decode(const char *record, char *raw, const char **input_error)
{
    const char *fields[12], *p = record;
    size_t lengths[12], i, n;
    for (i = 0; i < 12; ++i) {
        fields[i] = p;
        while (*p && *p != '\t') ++p;
        lengths[i] = (size_t)(p - fields[i]);
        if (i < 11 && *p != '\t') return 0;
        if (i == 11 && *p) return 0;
        if (*p) ++p;
    }
    *input_error = NULL;
    if (lengths[2] == strlen("APP_LINE_LIMIT") && !strncmp(fields[2], "APP_LINE_LIMIT", lengths[2]))
        *input_error = "APP_LINE_LIMIT";
    else if (lengths[2] == strlen("APP_INPUT_FORMAT") && !strncmp(fields[2], "APP_INPUT_FORMAT", lengths[2]))
        *input_error = "APP_INPUT_FORMAT";
    else if (lengths[2] != 1 || fields[2][0] != '-') return 0;
    n = lengths[10];
    if (n % 2u || n / 2u >= WB_LINE) return 0;
    for (i = 0; i < n / 2u; ++i) {
        int a = hex(fields[10][2u * i]), b = hex(fields[10][2u * i + 1u]);
        if (a < 0 || b < 0 || !(a * 16 + b) || a * 16 + b == '\n') return 0;
        raw[i] = (char)(a * 16 + b);
    }
    raw[n / 2u] = '\0';
    return 1;
}
