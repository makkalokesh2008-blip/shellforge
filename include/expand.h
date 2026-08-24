#ifndef EXPAND_H
#define EXPAND_H

#include "parser.h"

/*
 * Expand variables and special shell values
 * inside parsed command arguments.
 */
int expand_pipeline(Pipeline *pipeline);

/*
 * Expand a single string.
 * The returned string must be freed by the caller.
 */
char *expand_string(const char *input);

/*
 * Expand one command's arguments.
 */
int expand_command(Command *cmd);

#endif
