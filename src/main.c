#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "lexer.h"
#include "parser.h"
#include "expand.h"

static void print_tokens(Token *tokens, int count)
{
    printf("\n------------ TOKENS ------------\n");

    for (int i = 0; i < count; i++) {
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

    char *line;

    while (1) {

        line = readline("shellforge$ ");

        if (line == NULL) {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        add_history(line);

        if (strcmp(line, "exit") == 0) {
            free(line);
            printf("Exiting...\n");
            break;
        }

        Token tokens[128];
        int token_count = 0;

        Lexer lexer;
        lexer_init(&lexer, line);

        while (token_count < 127) {

            tokens[token_count] = lexer_next_token(&lexer);

            if (tokens[token_count].type == TOKEN_END) {
                token_count++;
                break;
            }

            token_count++;
        }

        print_tokens(tokens, token_count);

        Pipeline pipeline;

        if (parse_tokens(tokens, token_count, &pipeline) == 0) {

            if (expand_pipeline(&pipeline) != 0) {
                printf("Error: expansion failed\n");
                free_pipeline(&pipeline);

                for (int i = 0; i < token_count; i++) {
                    free_token(&tokens[i]);
                }

                free(line);
                continue;
            }

            pipeline_print(&pipeline);

            free_pipeline(&pipeline);

        } else {
            printf("Error: invalid command syntax\n");
        }

        for (int i = 0; i < token_count; i++) {
            free_token(&tokens[i]);
        }

        free(line);
    }

    return 0;
}
