#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "lexer.h"

// Helper function to resolve history file path at ~/.shellforge_history
static char *get_history_path(void) {
    char *home = getenv("HOME");
    char *path;

    if (home) {
        size_t len = strlen(home) + strlen("/.shellforge_history") + 1;
        path = malloc(len);

        if (path) {
            sprintf(path, "%s/.shellforge_history", home);
            return path;
        }
    }

    return strdup(".shellforge_history");
}

int main(void)
{
    // Display a welcome banner when the shell starts
    printf("=====================================\n");
    printf("Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    // Load history at startup
    char *history_file = get_history_path();

    if (history_file) {
        read_history(history_file);
    }

    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        // Check if the command is "history"
        if (strcmp(line, "history") == 0)
        {
            printf("------ Command History ------\n");

            HIST_ENTRY **list = history_list();

            if (list) {
                for (int i = 0; list[i] != NULL; i++) {
                    printf("%2d  %s\n", i + 1, list[i]->line);
                }
            }

            printf("-----------------------------\n");

            add_history(line);
            free(line);
            continue;
        }

        add_history(line);

        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exiting...\n");
            break;
        }

        // Print tokenization results
        printf("\n---------------- TOKENS ----------------\n");

        Lexer lexer;
        lexer_init(&lexer, line);

        int token_index = 0;
        Token token;

        do {
            token = lexer_next_token(&lexer);

            printf(" %d : %-12s %s\n",
                   token_index++,
                   token_type_to_string(token.type),
                   token.value ? token.value : "");

            free_token(&token);

        } while (token.type != TOKEN_END);

        printf("----------------------------------------\n");

        free(line);
    }

    // Save history upon exit
    if (history_file) {
        write_history(history_file);
        free(history_file);
    }

    return 0;
}
