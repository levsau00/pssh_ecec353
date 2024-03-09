#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "builtin.h"
#include "parse.h"

static char *builtin[] = {
    "exit",  /* exits the shell */
    "which", /* displays full path to command */
    NULL};

int is_builtin(char *cmd)
{
    int i;

    for (i = 0; builtin[i]; i++)
    {
        if (!strcmp(cmd, builtin[i]))
            return 1;
    }

    return 0;
}
char *command_found_builtin(const char *cmd)
{
    char *dir;
    char *tmp;
    char *PATH;
    char *state;
    char probe[PATH_MAX];

    char *ret;

    if (strstr(cmd, "/"))
    {
        if (access(cmd, X_OK) == 0)
        {
            ret = strdup(cmd);
            return ret;
        }
    }

    PATH = strdup(getenv("PATH"));
    for (tmp = PATH;; tmp = NULL)
    {

        dir = strtok_r(tmp, ":", &state);
        if (!dir)
            break;
        strncpy(probe, dir, PATH_MAX - 1);
        strncat(probe, "/", PATH_MAX - 1);
        strncat(probe, cmd, PATH_MAX - 1);
        if (access(probe, X_OK) == 0)
        {
            ret = strdup(probe);
            break;
        }
    }

    free(PATH);
    return ret;
}
void builtin_execute(Task T)
{
    char *path;
    if (!strcmp(T.cmd, "which"))
    {
        if (T.argv[1])
        {
            if (is_builtin(T.argv[1]))
            {
                printf("%s: shell built-in command\n", T.argv[1]);
            }
            else
            {
                path = command_found_builtin(T.argv[1]);
                if (path)
                {
                    printf("%s\n", path);
                    free(path);
                }
            }
        }
    }
    else
    {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
