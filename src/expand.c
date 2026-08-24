#define _POSIX_C_SOURCE 200809L

#include "expand.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int append_char(char **result, size_t *length, size_t *capacity, char c)
{
    if (*length + 1 >= *capacity) {
        size_t new_capacity = (*capacity) * 2;
        char *temp = realloc(*result, new_capacity);

        if (temp == NULL) {
            return -1;
        }

        *result = temp;
        *capacity = new_capacity;
    }

    (*result)[(*length)++] = c;
    (*result)[*length] = '\0';

    return 0;
}

static int append_text(char **result,
                       size_t *length,
                       size_t *capacity,
                       const char *text)
{
    if (text == NULL) {
        return 0;
    }

    while (*text != '\0') {
        if (append_char(result, length, capacity, *text) != 0) {
            return -1;
        }
        text++;
    }

    return 0;
}

char *expand_string(const char *input)
{
    if (input == NULL) {
        return NULL;
    }

    size_t capacity = 64;
    size_t length = 0;

    char *result = malloc(capacity);

    if (result == NULL) {
        return NULL;
    }

    result[0] = '\0';

    for (size_t i = 0; input[i] != '\0'; i++) {

        if (input[i] != '$') {
            if (append_char(&result, &length, &capacity, input[i]) != 0) {
                free(result);
                return NULL;
            }
            continue;
        }

        /* Handle $HOME, $USER, $PATH, etc. */
        if (input[i + 1] == '{') {
            size_t start = i + 2;
            size_t j = start;

            while (input[j] != '\0' && input[j] != '}') {
                j++;
            }

            if (input[j] == '}') {
                size_t name_length = j - start;

                char *name = malloc(name_length + 1);

                if (name == NULL) {
                    free(result);
                    return NULL;
                }

                memcpy(name, input + start, name_length);
                name[name_length] = '\0';

                const char *value = getenv(name);

                if (value != NULL) {
                    if (append_text(&result,
                                    &length,
                                    &capacity,
                                    value) != 0) {
                        free(name);
                        free(result);
                        return NULL;
                    }
                }

                free(name);
                i = j;
                continue;
            }
        }

        if (isalpha((unsigned char)input[i + 1]) ||
            input[i + 1] == '_') {

            size_t start = i + 1;
            size_t j = start;

            while (isalnum((unsigned char)input[j]) ||
                   input[j] == '_') {
                j++;
            }

            size_t name_length = j - start;

            char *name = malloc(name_length + 1);

            if (name == NULL) {
                free(result);
                return NULL;
            }

            memcpy(name, input + start, name_length);
            name[name_length] = '\0';

            const char *value = getenv(name);

            if (value != NULL) {
                if (append_text(&result,
                                &length,
                                &capacity,
                                value) != 0) {
                    free(name);
                    free(result);
                    return NULL;
                }
            }

            free(name);

            i = j - 1;
            continue;
        }

        /*
         * If '$' is not followed by a valid variable name,
         * preserve it literally.
         */
        if (append_char(&result, &length, &capacity, '$') != 0) {
            free(result);
            return NULL;
        }
    }

    return result;
}

int expand_command(Command *cmd)
{
    if (cmd == NULL) {
        return -1;
    }

    for (int i = 0; i < cmd->argc; i++) {

        char *expanded = expand_string(cmd->argv[i]);

        if (expanded == NULL) {
            return -1;
        }

        free(cmd->argv[i]);
        cmd->argv[i] = expanded;
    }

    if (cmd->input != NULL) {
        char *expanded = expand_string(cmd->input);

        if (expanded == NULL) {
            return -1;
        }

        free(cmd->input);
        cmd->input = expanded;
    }

    if (cmd->output != NULL) {
        char *expanded = expand_string(cmd->output);

        if (expanded == NULL) {
            return -1;
        }

        free(cmd->output);
        cmd->output = expanded;
    }

    return 0;
}

int expand_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL) {
        return -1;
    }

    for (int i = 0; i < pipeline->command_count; i++) {

        if (expand_command(&pipeline->commands[i]) != 0) {
            return -1;
        }
    }

    return 0;
}
