#include "command.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
static const char *const names[] = {
    "invalid", "create", "insert-front", "insert-back", "insert-before",
    "insert-after", "remove", "move-front", "move-back", "move-before",
    "move-after", "list", "list-reverse", "compare", "size", "help", "quit"
};
const char *command_name(CommandKind kind) { return names[(size_t)kind]; }
static int id(const char *s, size_t *out)
{
    size_t n = 0;
    if (!*s) return 0;
    for (; *s; ++s) {
        unsigned digit;
        if (*s < '0' || *s > '9') return 0;
        digit = (unsigned)(*s - '0');
        if (n > (WB_OCCURRENCES - digit) / 10u) return 0;
        n = n * 10u + digit;
    }
    if (!n || n > WB_OCCURRENCES) return 0;
    *out = n;
    return 1;
}
const char *command_parse(const char *line, Command *out)
{
    char copy[WB_LINE], *tokens[4];
    size_t count = 0, length = strlen(line), expected = 1, k;
    char *p;
    memset(out, 0, sizeof(*out));
    if (length >= sizeof(copy)) return "APP_LINE_LIMIT";
    memcpy(copy, line, length + 1u);
    p = copy;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) ++p;
        if (!*p) break;
        if (count == 4) return "APP_ARGUMENT_COUNT";
        tokens[count++] = p;
        while (*p && !isspace((unsigned char)*p)) ++p;
        if (*p) *p++ = '\0';
    }
    if (!count) return "APP_EMPTY_COMMAND";
    for (k = 1; k < sizeof(names) / sizeof(names[0]); ++k) {
        if (!strcmp(tokens[0], names[k])) { out->kind = (CommandKind)k; break; }
    }
    if (out->kind == CMD_BAD) return "APP_UNKNOWN_COMMAND";
    switch (out->kind) {
    case CMD_CREATE: case CMD_INSERT_FRONT: case CMD_INSERT_BACK:
    case CMD_REMOVE: case CMD_MOVE_FRONT: case CMD_MOVE_BACK: expected = 2; break;
    case CMD_INSERT_BEFORE: case CMD_INSERT_AFTER: case CMD_MOVE_BEFORE:
    case CMD_MOVE_AFTER: case CMD_COMPARE: expected = 3; break;
    default: break;
    }
    if (count != expected) return "APP_ARGUMENT_COUNT";
    if (out->kind == CMD_CREATE) {
        length = strlen(tokens[1]);
        if (!length || length >= WB_NAME) return "APP_NAME_LIMIT";
        for (k = 0; k < length; ++k) {
            unsigned char c = (unsigned char)tokens[1][k];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-'))
                return "APP_NAME_FORMAT";
        }
        memcpy(out->name, tokens[1], length + 1u);
    } else if (out->kind >= CMD_INSERT_FRONT && out->kind <= CMD_INSERT_AFTER) {
        /* Relative insertion syntax: insert-before PAYLOAD ANCHOR. */
        if (strcmp(tokens[1], "NULL") && !id(tokens[1], &out->payload))
            return "APP_ID_FORMAT";
        if (expected == 3 && !id(tokens[2], &out->anchor)) return "APP_ID_FORMAT";
    } else if (expected >= 2) {
        if (!id(tokens[1], &out->occurrence)) return "APP_ID_FORMAT";
        if (expected == 3 && !id(tokens[2], &out->anchor)) return "APP_ID_FORMAT";
    }
    return NULL;
}
void command_help(void)
{
    puts("create NAME (ASCII letters/digits/_/-; object IDs assigned sequentially)");
    puts("insert-front PAYLOAD | insert-back PAYLOAD (object ID or NULL)");
    puts("insert-before PAYLOAD ANCHOR | insert-after PAYLOAD ANCHOR");
    puts("remove OCC | move-front OCC | move-back OCC");
    puts("move-before OCC ANCHOR | move-after OCC ANCHOR | compare OCC OCC");
    puts("list | list-reverse | size | help | quit");
    puts("Occurrences get new IDs per successful insertion; retired IDs are rejected.");
}
