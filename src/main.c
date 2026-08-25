#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "executor.h"

static int builtin_execute(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0) {
        return 0;
    }

    /* exit */
    if (strcmp(cmd->argv[0], "exit") == 0) {
        return 1;
    }

    /* cd */
    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path;

        if (cmd->argc > 1) {
            path = cmd->argv[1];
        } else {
            path = getenv("HOME");
        }

        if (path == NULL) {
            path = ".";
        }

        if (chdir(path) != 0) {
            perror("cd");
        }

        return 0;
    }

    /* pwd */
    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[4096];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }

        return 0;
    }

    /* echo */
    if (strcmp(cmd->argv[0], "echo") == 0) {
        int i;

        for (i = 1; i < cmd->argc; i++) {
            if (i > 1) {
                printf(" ");
            }

            printf("%s", cmd->argv[i]);
        }

        printf("\n");
        return 0;
    }

    return -1;
}

static void print_tokens(Token *tokens, int count)
{
    int i;

    printf("\n------------ TOKENS ------------\n");

    for (i = 0; i < count; i++) {
        printf(" %d : %-12s %s\n",
               i,
               token_type_to_string(tokens[i].type),
               tokens[i].value ? tokens[i].value : "");
    }

    printf("--------------------------------\n");
}

int main(void)
{
    char *line;

    printf("=====================================\n");
    printf("        Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    while (1) {
        line = readline("shellforge$ ");

        if (line == NULL) {
            printf("\n");
            break;
        }

        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        add_history(line);

        Lexer lexer;
        lexer_init(&lexer, line);

        Token tokens[256];
        int token_count = 0;

        while (token_count < 255) {
            tokens[token_count] = lexer_next_token(&lexer);

            if (tokens[token_count].type == TOKEN_END) {
                token_count++;
                break;
            }

            token_count++;
        }

        print_tokens(tokens, token_count);

        Pipeline pipeline;

        if (parse_tokens(tokens, token_count, &pipeline) != 0) {
            fprintf(stderr, "Parse error\n");

            for (int i = 0; i < token_count; i++) {
                free_token(&tokens[i]);
            }

            free(line);
            continue;
        }

        if (expand_pipeline(&pipeline) != 0) {
            fprintf(stderr, "Expansion error\n");
            free_pipeline(&pipeline);

            for (int i = 0; i < token_count; i++) {
                free_token(&tokens[i]);
            }

            free(line);
            continue;
        }

        printf("\n---------------- PARSED COMMAND ----------------\n");
        pipeline_print(&pipeline);
        printf("-----------------------------------------------\n");

        /*
         * Built-ins are executed directly by the shell.
         * External commands are executed using executor.c.
         */
       if (pipeline.command_count == 1 &&
    pipeline.commands[0].input == NULL &&
    pipeline.commands[0].output == NULL &&
    !pipeline.commands[0].background) {
            int builtin_result =
                builtin_execute(&pipeline.commands[0]);

            if (builtin_result == 1) {
                free_pipeline(&pipeline);

                for (int i = 0; i < token_count; i++) {
                    free_token(&tokens[i]);
                }

                free(line);
                break;
            }

            if (builtin_result == -1) {
                execute_pipeline(&pipeline);
            }
        } else {
            execute_pipeline(&pipeline);
        }

        free_pipeline(&pipeline);

        for (int i = 0; i < token_count; i++) {
            free_token(&tokens[i]);
        }

        free(line);
    }

    return 0;
}
