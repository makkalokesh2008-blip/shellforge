#define _POSIX_C_SOURCE 200809L

#include "executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

static void redirect_input(Command *cmd, int background)
{
    int fd;

    if (cmd->input != NULL) {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0) {
            perror(cmd->input);
            exit(1);
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }

        close(fd);
    } else if (background) {
        fd = open("/dev/null", O_RDONLY);

        if (fd < 0) {
            perror("/dev/null");
            exit(1);
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }

        close(fd);
    }
}

static void redirect_output(Command *cmd)
{
    int fd;

    if (cmd->output == NULL) {
        return;
    }

    if (cmd->append) {
        fd = open(cmd->output,
                  O_WRONLY | O_CREAT | O_APPEND,
                  0644);
    } else {
        fd = open(cmd->output,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0644);
    }

    if (fd < 0) {
        perror(cmd->output);
        exit(1);
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        close(fd);
        exit(1);
    }

    close(fd);
}

static int execute_command(Command *cmd)
{
    pid_t pid;
    int status;

    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return 0;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        redirect_input(cmd, cmd->background);
        redirect_output(cmd);

        execvp(cmd->argv[0], cmd->argv);

        perror("execvp");
        exit(1);
    }

    if (cmd->background) {
        printf("[Background PID: %d]\n", pid);
        fflush(stdout);
        return 0;
    }

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }

    return 0;
}

int execute_pipeline(Pipeline *pipeline)
{
    int i;
    int previous_read = -1;
    int pipefd[2];
    pid_t *pids;
    int status;
    int background;

    if (pipeline == NULL || pipeline->command_count == 0) {
        return -1;
    }

    if (pipeline->command_count == 1) {
        return execute_command(&pipeline->commands[0]);
    }

    background =
        pipeline->commands[pipeline->command_count - 1].background;

    pids = malloc(sizeof(pid_t) * pipeline->command_count);

    if (pids == NULL) {
        perror("malloc");
        return -1;
    }

    for (i = 0; i < pipeline->command_count; i++) {

        if (i < pipeline->command_count - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                free(pids);
                return -1;
            }
        }

        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            free(pids);
            return -1;
        }

        if (pids[i] == 0) {

            if (previous_read != -1) {
                if (dup2(previous_read, STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            if (i < pipeline->command_count - 1) {
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            if (previous_read != -1) {
                close(previous_read);
            }

            if (i < pipeline->command_count - 1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            /*
             * Apply explicit redirections after pipe setup.
             * This allows < and > to override the pipe endpoints.
             */
            redirect_input(
                &pipeline->commands[i],
                background && i == 0
            );

            redirect_output(&pipeline->commands[i]);

            execvp(pipeline->commands[i].argv[0],
                   pipeline->commands[i].argv);

            perror("execvp");
            exit(1);
        }

        if (previous_read != -1) {
            close(previous_read);
        }

        if (i < pipeline->command_count - 1) {
            close(pipefd[1]);
            previous_read = pipefd[0];
        }
    }

    if (background) {
        printf("[Background PID: %d]\n", pids[0]);
        fflush(stdout);
    } else {
        for (i = 0; i < pipeline->command_count; i++) {
            if (waitpid(pids[i], &status, 0) < 0) {
                if (errno != ECHILD) {
                    perror("waitpid");
                }
            }
        }
    }

    free(pids);

    return 0;
}
