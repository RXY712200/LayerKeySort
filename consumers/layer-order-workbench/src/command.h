#ifndef WB_COMMAND_H
#define WB_COMMAND_H
#include <stddef.h>
#define WB_LINE 256u
#define WB_NAME 48u
#define WB_LIVE 2048u
#define WB_OBJECTS 256u
#define WB_OCCURRENCES 16384u
typedef enum {
    CMD_BAD, CMD_CREATE, CMD_INSERT_FRONT, CMD_INSERT_BACK,
    CMD_INSERT_BEFORE, CMD_INSERT_AFTER, CMD_REMOVE, CMD_MOVE_FRONT,
    CMD_MOVE_BACK, CMD_MOVE_BEFORE, CMD_MOVE_AFTER, CMD_LIST,
    CMD_REVERSE, CMD_COMPARE, CMD_SIZE, CMD_HELP, CMD_QUIT
} CommandKind;
typedef struct {
    CommandKind kind;
    size_t payload, occurrence, anchor;
    char name[WB_NAME];
} Command;
/* Returns a precise application parse error, or NULL. NULL payload is ID 0. */
const char *command_parse(const char *line, Command *out);
const char *command_name(CommandKind kind);
void command_help(void);
#endif
