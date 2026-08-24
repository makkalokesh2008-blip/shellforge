#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"

#define MAX_TOKENS 256

static void print_tokens(Token *tokens, int token_count)
{
    printf("\n------------ TOKENS ------------\n");

    for (int i = 0; i < token_count; i++)
    {
        printf("%2d : %-12s %s\n",
               i,
               token_type_to_string(tokens[i].type),
               tokens[i].value ? tokens[i].value : "");
    }

    printf("--------------------------------\n");
}

int main(void)
{
    printf("=====================================\n");
    printf("        Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    while (1)
    {
        char *line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        add_history(line);

        /* Tokenize input */
        Token tokens[MAX_TOKENS];
        int token_count = 0;

        Lexer lexer;
        lexer_init(&lexer, line);

        while (token_count < MAX_TOKENS)
        {
            tokens[token_count] = lexer_next_token(&lexer);

            if (tokens[token_count].type == TOKEN_ERROR)
            {
                printf("Lexer error\n");
                free_token(&tokens[token_count]);
                token_count++;
                break;
            }

            token_count++;

            if (tokens[token_count - 1].type == TOKEN_END)
                break;
        }

        print_tokens(tokens, token_count);

        /* Parse tokens */
        Pipeline pipeline;

        if (parse_tokens(tokens, token_count, &pipeline) != 0)
        {
            fprintf(stderr, "Parse error\n");

            for (int i = 0; i < token_count; i++)
                free_token(&tokens[i]);

            free(line);
            continue;
        }

        /* Free token memory */
        for (int i = 0; i < token_count; i++)
            free_token(&tokens[i]);

        /* Expand variables */
        if (expand_pipeline(&pipeline) != 0)
        {
            fprintf(stderr, "Expansion error\n");
            free_pipeline(&pipeline);
            free(line);
            continue;
        }

        /* Display parsed result */
        pipeline_print(&pipeline);

        /*
         * Built-in commands execute inside the shell process.
         * Only handle a single standalone command here.
         */
        if (pipeline.command_count == 1)
        {
            Command *cmd = &pipeline.commands[0];

            if (is_builtin(cmd->argv[0]))
            {
                int should_exit = execute_builtin(cmd);

                free_pipeline(&pipeline);
                free(line);

                if (should_exit)
                    break;

                continue;
            }
        }

        free_pipeline(&pipeline);
        free(line);
    }

    return 0;
}
