#define _POSIX_C_SOURCE 200809L

#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_builtin(const char *command)
{
    if (command == NULL)
        return 0;

    return strcmp(command, "cd") == 0 ||
           strcmp(command, "pwd") == 0 ||
           strcmp(command, "echo") == 0 ||
           strcmp(command, "exit") == 0;
}

int execute_builtin(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL)
        return 0;

    /* cd */
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        char *dir;

        if (cmd->argc == 1)
        {
            dir = getenv("HOME");

            if (dir == NULL)
            {
                fprintf(stderr, "cd: HOME not set\n");
                return 0;
            }
        }
        else if (cmd->argc == 2)
        {
            dir = cmd->argv[1];
        }
        else
        {
            fprintf(stderr, "cd: too many arguments\n");
            return 0;
        }

        if (chdir(dir) != 0)
            perror("cd");

        return 0;
    }

    /* pwd */
    if (strcmp(cmd->argv[0], "pwd") == 0)
    {
        if (cmd->argc > 1)
        {
            fprintf(stderr, "pwd: too many arguments\n");
            return 0;
        }

        char buffer[4096];

        if (getcwd(buffer, sizeof(buffer)) == NULL)
        {
            perror("pwd");
            return 0;
        }

        printf("%s\n", buffer);
        return 0;
    }

    /* echo */
    if (strcmp(cmd->argv[0], "echo") == 0)
    {
        for (int i = 1; i < cmd->argc; i++)
        {
            printf("%s", cmd->argv[i]);

            if (i < cmd->argc - 1)
                printf(" ");
        }

        printf("\n");
        return 0;
    }

    /* exit */
    if (strcmp(cmd->argv[0], "exit") == 0)
    {
        if (cmd->argc > 1)
        {
            fprintf(stderr, "exit: too many arguments\n");
            return 0;
        }

        return 1;
    }

    return 0;
}
