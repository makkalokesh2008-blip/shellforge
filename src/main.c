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

static void builtin_help(void)
{
    printf("\n");
    printf("  ShellForge -- Unix-style shell written in C\n");
    printf("  ===========================================\n");
    printf("\n");
    printf("  Built-in commands:\n");
    printf("    help             show this message\n");
    printf("    exit             exit the shell\n");
    printf("    cd [dir]         change directory\n");
    printf("    pwd              print working directory\n");
    printf("    echo [args...]   print arguments\n");
    printf("    jobs             list background jobs\n");
    printf("    fg <id>          bring job to foreground\n");
    printf("    bg <id>          resume stopped job in background\n");
    printf("    history          show command history\n");
    printf("\n");
    printf("  Shell features:\n");
    printf("    Pipelines:         cmd1 | cmd2 | cmd3\n");
    printf("    Input redirect:    cmd < file\n");
    printf("    Output redirect:   cmd > file\n");
    printf("    Append redirect:   cmd >> file\n");
    printf("    Background exec:   cmd &\n");
    printf("    Variable expand:   echo $HOME, echo ${VAR}\n");
    printf("    Quoting:           single or double quotes\n");
    printf("\n");
    printf("  System calls: fork(2), execvp(2), waitpid(2),\n");
    printf("    pipe(2), dup2(2), open(2), chdir(2), getcwd(3),\n");
    printf("    kill(2), setpgid(2), signal(2)\n");
    printf("\n");
}

static int builtin_execute(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0) return 0;

    if (strcmp(cmd->argv[0], "help") == 0) {
        builtin_help(); return 0;
    }
    if (strcmp(cmd->argv[0], "exit") == 0) return 1;

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path = (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME");
        if (path == NULL) path = ".";
        if (chdir(path) != 0) perror("cd");
        return 0;
    }
    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        else perror("pwd");
        return 0;
    }
    if (strcmp(cmd->argv[0], "echo") == 0) {
        for (int i = 1; i < cmd->argc; i++) {
            if (i > 1) printf(" ");
            printf("%s", cmd->argv[i]);
        }
        printf("\n"); return 0;
    }
    if (strcmp(cmd->argv[0], "history") == 0) {
        HIST_ENTRY **hist = history_list();
        if (hist != NULL)
            for (int i = 0; hist[i] != NULL; i++)
                printf("  %4d  %s\n", i + 1, hist[i]->line);
        return 0;
    }
    if (strcmp(cmd->argv[0], "jobs") == 0) {
        jobs_print(); return 0;
    }
    if (strcmp(cmd->argv[0], "bg") == 0) {
        if (cmd->argc != 2) { fprintf(stderr, "bg: usage: bg <job_id>\n"); return 0; }
        int jid = atoi(cmd->argv[1]); Job *job = job_find(jid);
        if (!job) { fprintf(stderr, "bg: no such job\n"); return 0; }
        job_continue(job->pgid);
        printf("[%d] Running %s\n", job->job_id, job->command);
        return 0;
    }
    if (strcmp(cmd->argv[0], "fg") == 0) {
        if (cmd->argc != 2) { fprintf(stderr, "fg: usage: fg <job_id>\n"); return 0; }
        int jid = atoi(cmd->argv[1]); Job *job = job_find(jid);
        if (!job) { fprintf(stderr, "fg: no such job\n"); return 0; }
        pid_t pgid = job->pgid;
        if (job->state == JOB_STOPPED) { kill(-pgid, SIGCONT); job->state = JOB_RUNNING; }
        while (job->state == JOB_RUNNING) pause();
        if (job->state == JOB_DONE) job_remove(jid);
        return 0;
    }
    return -1;
}

static int debug_mode = 0;

static void debug_print_tokens(Token *tokens, int count)
{
    if (!debug_mode) return;
    printf("\n------------ TOKENS ------------\n");
    for (int i = 0; i < count; i++)
        printf(" %d : %-16s %s\n", i,
               token_type_to_string(tokens[i].type),
               tokens[i].value ? tokens[i].value : "");
    printf("--------------------------------\n");
}

static void debug_print_pipeline(const Pipeline *pipeline)
{
    if (!debug_mode) return;
    pipeline_print(pipeline);
}

static void sigchld_handler(int sig)
{
    int saved_errno = errno, status; pid_t pid; (void)sig;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job *job = job_find_by_pgid(pid);
        if (job) {
            if (WIFSTOPPED(status)) job_stop(pid);
            else if (WIFEXITED(status) || WIFSIGNALED(status)) job_done(pid);
        }
    }
    errno = saved_errno;
}

int main(void)
{
    char *line;
    const char *sf_debug = getenv("SF_DEBUG");
    if (sf_debug && strcmp(sf_debug, "1") == 0) debug_mode = 1;
    jobs_init();
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT,  SIG_IGN);
    printf("=====================================\n");
    printf("        ShellForge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");
    printf(" Type 'help' for available commands.\n\n");
    while (1) {
        line = readline("shellforge$ ");
        if (!line) { printf("\n"); break; }
        if (*line == '\0' || *line == '#') { free(line); continue; }
        add_history(line);
        Lexer lexer; lexer_init(&lexer, line);
        Token tokens[256]; int token_count = 0;
        while (token_count < 255) {
            tokens[token_count] = lexer_next_token(&lexer);
            if (tokens[token_count].type == TOKEN_END) { token_count++; break; }
            token_count++;
        }
        debug_print_tokens(tokens, token_count);
        Pipeline pipeline;
        if (parse_tokens(tokens, token_count, &pipeline) != 0) {
            fprintf(stderr, "shellforge: parse error\n");
            for (int i = 0; i < token_count; i++) free_token(&tokens[i]);
            free(line); continue;
        }
        if (expand_pipeline(&pipeline) != 0) {
            fprintf(stderr, "shellforge: expansion error\n");
            free_pipeline(&pipeline);
            for (int i = 0; i < token_count; i++) free_token(&tokens[i]);
            free(line); continue;
        }
        debug_print_pipeline(&pipeline);
        if (pipeline.command_count == 1) {
            Command *c = &pipeline.commands[0];
            /* echo and pwd with redirections: let execvp handle them
             * so that the redirect is applied by the child process. */
            int has_redir = (c->input != NULL || c->output != NULL);
            int force_exec = has_redir &&
                (c->argc > 0 && c->argv[0] != NULL &&
                 (strcmp(c->argv[0],"echo")==0 ||
                  strcmp(c->argv[0],"pwd")==0));
            int rc = force_exec ? -1 : builtin_execute(c);
            if (rc == 1) {
                free_pipeline(&pipeline);
                for (int i = 0; i < token_count; i++) free_token(&tokens[i]);
                free(line); break;
            }
            if (rc == 0) {
                free_pipeline(&pipeline);
                for (int i = 0; i < token_count; i++) free_token(&tokens[i]);
                free(line); continue;
            }
            execute_pipeline(&pipeline);
        } else {
            execute_pipeline(&pipeline);
        }
        free_pipeline(&pipeline);
        for (int i = 0; i < token_count; i++) free_token(&tokens[i]);
        free(line);
    }
    return 0;
}
