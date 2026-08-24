#ifndef PARSER_H
#define PARSER_H

#include "token.h"

#define MAX_COMMANDS 16
#define MAX_ARGS 64

typedef struct {
    char *argv[MAX_ARGS];
    int argc;

    char *input;
    char *output;

    int append;
    int background;
} Command;

typedef struct {
    Command commands[MAX_COMMANDS];
    int command_count;
} Pipeline;

int parse_tokens(Token *tokens, int token_count, Pipeline *pipeline);

void free_pipeline(Pipeline *pipeline);

void pipeline_print(const Pipeline *pipeline);

#endif
