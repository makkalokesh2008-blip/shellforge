#define _POSIX_C_SOURCE 200809L

#include "executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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

/*
 * Build a readable command string for the job table.
 */
static char *build_command_string(Pipeline *pipeline)
{
    size_t size = 1;
    char *command;

    if (pipeline == NULL) {
        return NULL;
    }

    for (int i = 0; i < pipeline->command_count; i++) {
        Command *cmd = &pipeline->commands[i];

        for (int j = 0; j < cmd->argc; j++) {
            if (cmd->argv[j] != NULL) {
                size += strlen(cmd->argv[j]) + 1;
            }
        }

        if (i < pipeline->command_count - 1) {
            size += 3;
        }
    }

    if (pipeline->commands[pipeline->command_count - 1].background) {
        size += 3;
    }

    command = malloc(size);

    if (command == NULL) {
        return NULL;
    }

    command[0] = '\0';

    for (int i = 0; i < pipeline->command_count; i++) {
        Command *cmd = &pipeline->commands[i];

        for (int j = 0; j < cmd->argc; j++) {
            if (cmd->argv[j] == NULL) {
                continue;
            }

            if (command[0] != '\0') {
                strcat(command, " ");
            }

            strcat(command, cmd->argv[j]);
        }

        if (i < pipeline->command_count - 1) {
            strcat(command, " | ");
        }
    }

    if (pipeline->commands[pipeline->command_count - 1].background) {
        strcat(command, " &");
    }

    return command;
}

static int execute_command(Command *cmd)
{
    pid_t pid;
    int status;
    char *command_string = NULL;

    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return 0;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {

        /*
         * A background command gets its own process group.
         */
        if (cmd->background) {
            if (setpgid(0, 0) < 0) {
                perror("setpgid");
                exit(1);
            }
        }

        redirect_input(cmd, cmd->background);
        redirect_output(cmd);

        execvp(cmd->argv[0], cmd->argv);

        perror("execvp");
        exit(1);
    }

    /*
     * Parent also sets the process group to avoid a race
     * with the child.
     */
    if (cmd->background) {
        if (setpgid(pid, pid) < 0 && errno != EACCES) {
            perror("setpgid");
        }

        command_string = malloc(strlen(cmd->argv[0]) + 64);

        if (command_string != NULL) {
            command_string[0] = '\0';

            for (int i = 0; i < cmd->argc; i++) {
                if (i > 0) {
                    strcat(command_string, " ");
                }

                strcat(command_string, cmd->argv[i]);
            }

            strcat(command_string, " &");

            int job_id =
                job_add(pid, JOB_RUNNING, command_string);

            if (job_id > 0) {
                printf("[%d] %d\n", job_id, pid);
            }

            free(command_string);
        }

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
    pid_t pgid = 0;
    int status;
    int background;
    char *command_string = NULL;

    if (pipeline == NULL || pipeline->command_count == 0) {
        return -1;
    }

    /*
     * Single command.
     */
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

        /*
         * Child process.
         */
        if (pids[i] == 0) {

            /*
             * Put every process in the pipeline
             * into the same process group.
             */
            if (pgid == 0) {
                if (setpgid(0, 0) < 0) {
                    perror("setpgid");
                    exit(1);
                }
            } else {
                if (setpgid(0, pgid) < 0) {
                    perror("setpgid");
                    exit(1);
                }
            }

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
             * Explicit redirections are applied after
             * pipe setup so they can override pipe endpoints.
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

        /*
         * Parent chooses the first child's PID as the
         * process-group ID.
         */
        if (pgid == 0) {
            pgid = pids[i];
        }

        /*
         * Parent also sets the process group to avoid
         * races with the child.
         */
        if (setpgid(pids[i], pgid) < 0 && errno != EACCES) {
            perror("setpgid");
        }

        if (previous_read != -1) {
            close(previous_read);
        }

        if (i < pipeline->command_count - 1) {
            close(pipefd[1]);
            previous_read = pipefd[0];
        }
    }

    /*
     * Background pipeline:
     * add one job representing the entire pipeline.
     */
    if (background) {

        command_string = build_command_string(pipeline);

        if (command_string != NULL) {

            int job_id =
                job_add(pgid, JOB_RUNNING, command_string);

            if (job_id > 0) {
                printf("[%d] %d\n", job_id, pgid);
            }

            free(command_string);
        }

        fflush(stdout);
    } else {

        /*
         * Foreground pipeline waits for every process.
         */
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
