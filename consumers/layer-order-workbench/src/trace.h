#ifndef WB_TRACE_H
#define WB_TRACE_H
#include "workbench.h"
#include <stdio.h>
#define WB_RECORD 32768u
/* Versioned TSV, input hex-encoded; expected IDs come from the array oracle. */
int trace_header(FILE *file, const char *source, const char *mode);
int trace_read_header(FILE *file);
int trace_record(char *out, size_t capacity, const char *raw, const char *input_error,
                 const Command *command, const Result *result, const Workbench *wb);
int trace_decode(const char *record, char *raw, const char **input_error);
/* 1=line, 0=EOF, -1=I/O error; consumes an entire overlong/binary line. */
int input_line(FILE *file, char *line, size_t capacity, const char **error);
#endif
