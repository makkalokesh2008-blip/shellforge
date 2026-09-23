#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
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
#include "jobs.h"

static int builtin_execute(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0) {
        return 0;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) {
        return 1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path =
            (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME");

        if (path == NULL) {
            path = ".";
        }

        if (chdir(path) != 0) {
            perror("cd");
        }

        return 0;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[4096];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }

        return 0;
    }

    if (strcmp(cmd->argv[0], "echo") == 0) {
        for (int i = 1; i < cmd->argc; i++) {
            if (i > 1) {
                printf(" ");
            }

            printf("%s", cmd->argv[i]);
        }

        printf("\n");
        return 0;
    }

    if (strcmp(cmd->argv[0], "jobs") == 0) {
        jobs_print();
        return 0;
    }

    if (strcmp(cmd->argv[0], "bg") == 0) {
        if (cmd->argc != 2) {
            fprintf(stderr, "bg: usage: bg <job_id>\n");
            return 0;
        }

        int job_id = atoi(cmd->argv[1]);
        Job *job = job_find(job_id);

        if (job == NULL) {
            fprintf(stderr, "bg: no such job\n");
            return 0;
        }

        job_continue(job->pgid);
        printf("[%d] Running %s\n",
               job->job_id,
               job->command);

        return 0;
    }

    if (strcmp(cmd->argv[0], "fg") == 0) {
        if (cmd->argc != 2) {
            fprintf(stderr, "fg: usage: fg <job_id>\n");
            return 0;
        }

        int job_id = atoi(cmd->argv[1]);
        Job *job = job_find(job_id);

        if (job == NULL) {
            fprintf(stderr, "fg: no such job\n");
            return 0;
        }

        pid_t pgid = job->pgid;

        if (job->state == JOB_STOPPED) {
            kill(-pgid, SIGCONT);
            job->state = JOB_RUNNING;
        }

        /*
         * Wait until SIGCHLD marks this job as done.
         */
        while (job->state == JOB_RUNNING) {
            pause();
        }

        if (job->state == JOB_DONE) {
            job_remove(job_id);
        }

        return 0;
    }

    return -1;
}

static void print_tokens(Token *tokens, int count)
{
    printf("\n------------ TOKENS ------------\n");

    for (int i = 0; i < count; i++) {
        printf(" %d : %-12s %s\n",
               i,
               token_type_to_string(tokens[i].type),
               tokens[i].value ? tokens[i].value : "");
    }

    printf("--------------------------------\n");
}

static void sigchld_handler(int sig)
{
    int saved_errno = errno;
    int status;
    pid_t pid;

    (void)sig;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job *job = job_find_by_pgid(pid);

        if (job != NULL) {
            if (WIFSTOPPED(status)) {
                job_stop(pid);
            } else if (WIFEXITED(status) ||
                       WIFSIGNALED(status)) {
                job_done(pid);
            }
        }
    }

    errno = saved_errno;
}

int main(void)
{
    char *line;

    jobs_init();

    signal(SIGCHLD, sigchld_handler);

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
