#define _POSIX_C_SOURCE 200809L

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_command(Command *cmd)
{
    cmd->argc = 0;
    cmd->input = NULL;
    cmd->output = NULL;
    cmd->append = 0;
    cmd->background = 0;

    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }
}

static void init_pipeline(Pipeline *pipeline)
{
    pipeline->command_count = 0;

    for (int i = 0; i < MAX_COMMANDS; i++) {
        init_command(&pipeline->commands[i]);
    }
}

static int add_argument(Command *cmd, const char *value)
{
    if (cmd->argc >= MAX_ARGS - 1) {
        return -1;
    }

    cmd->argv[cmd->argc] = strdup(value);
    if (cmd->argv[cmd->argc] == NULL) {
        return -1;
    }

    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;

    return 0;
}

static int set_string(char **dest, const char *value)
{
    char *copy = strdup(value);

    if (copy == NULL) {
        return -1;
    }

    free(*dest);
    *dest = copy;

    return 0;
}

int parse_tokens(Token *tokens, int token_count, Pipeline *pipeline)
{
    if (tokens == NULL || pipeline == NULL || token_count <= 0) {
        return -1;
    }

    init_pipeline(pipeline);

    int command_index = 0;
    pipeline->command_count = 1;

    for (int i = 0; i < token_count; i++) {
        Token *token = &tokens[i];
        Command *cmd = &pipeline->commands[command_index];

        switch (token->type) {

            case TOKEN_WORD:
                if (token->value == NULL) {
                    return -1;
                }

                if (add_argument(cmd, token->value) != 0) {
                    return -1;
                }
                break;

            case TOKEN_REDIRECT_IN:
                if (i + 1 >= token_count ||
                    tokens[i + 1].type != TOKEN_WORD) {
                    return -1;
                }

                i++;

                if (set_string(&cmd->input, tokens[i].value) != 0) {
                    return -1;
                }
                break;

            case TOKEN_REDIRECT_OUT:
                if (i + 1 >= token_count ||
                    tokens[i + 1].type != TOKEN_WORD) {
                    return -1;
                }

                i++;

                cmd->append = 0;

                if (set_string(&cmd->output, tokens[i].value) != 0) {
                    return -1;
                }
                break;

            case TOKEN_REDIRECT_APPEND:
                if (i + 1 >= token_count ||
                    tokens[i + 1].type != TOKEN_WORD) {
                    return -1;
                }

                i++;

                cmd->append = 1;

                if (set_string(&cmd->output, tokens[i].value) != 0) {
                    return -1;
                }
                break;

            case TOKEN_PIPE:
                if (cmd->argc == 0 ||
                    command_index >= MAX_COMMANDS - 1) {
                    return -1;
                }

                command_index++;
                pipeline->command_count++;

                break;

            case TOKEN_AMPERSAND:
                cmd->background = 1;
                break;

            case TOKEN_END:
                return 0;

            case TOKEN_ERROR:
            default:
                return -1;
        }
    }

    return 0;
}

void free_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    for (int i = 0; i < pipeline->command_count; i++) {
        Command *cmd = &pipeline->commands[i];

        for (int j = 0; j < cmd->argc; j++) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }

        free(cmd->input);
        cmd->input = NULL;

        free(cmd->output);
        cmd->output = NULL;

        cmd->argc = 0;
        cmd->append = 0;
        cmd->background = 0;
    }

    pipeline->command_count = 0;
}

void pipeline_print(const Pipeline *pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    printf("\n---------------- PARSED COMMAND ----------------\n");

    for (int i = 0; i < pipeline->command_count; i++) {
        const Command *cmd = &pipeline->commands[i];

        printf("Command %d:\n", i + 1);

        printf("  argc      : %d\n", cmd->argc);

        printf("  argv      :");
        for (int j = 0; j < cmd->argc; j++) {
            printf(" [%s]", cmd->argv[j]);
        }
        printf("\n");

        printf("  input     : %s\n",
               cmd->input ? cmd->input : "none");

        printf("  output    : %s\n",
               cmd->output ? cmd->output : "none");

        printf("  append    : %d\n", cmd->append);

        printf("  background: %d\n", cmd->background);
    }

    printf("-----------------------------------------------\n");
}
