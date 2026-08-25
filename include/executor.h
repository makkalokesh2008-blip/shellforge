#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/*
 * Execute a complete pipeline.
 * Built-in commands are handled separately.
 * External commands are executed using fork(), execvp()
 * and waitpid().
 */
int execute_pipeline(Pipeline *pipeline);

#endif
