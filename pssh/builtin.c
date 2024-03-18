#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "builtin.h"
#include "parse.h"

static char *builtin[] = {
    "exit",  /* exits the shell */
    "which", /* displays full path to command */
    "jobs",  /* lists all jobs */
    "kill",  /* sends a signal to a process */
    "fg",    /* brings a job to the foreground */
    "bg",    /* sends a job to the background */
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

int is_valid_jobno(int jobno, int *job_ids)
{
    if (jobno > -1 && jobno < 100)
    {
        if (job_ids[jobno])
        {
            return 1;
        }
    }
    return 0;
}

int builtin_kill(Task T, Job **jobs, int *job_ids)
{
    char *help_str = "Usage: kill [-s <signal>] <pid> | %%<job>\n";
    int sig = 15;
    int targ_start = 1;
    int argc = num_args(T);
    if (argc == 1)
    {
        printf(help_str);
        return 1;
    }
    if (!strcmp("-s", T.argv[1]))
    {
        if (argc < 4)
        {
            printf("Usage: kill [-s <signal>] <pid> | %%<job>\n");
            return 0;
        }
        sig = atoi(T.argv[2]);
        if (sig == 0 && strcmp("0", T.argv[2]))
        {
            printf("pssh: invalid signal number: [%s]", T.argv[2]);
            return 0;
        }
        if (sig > 31 || sig < 0)
        {
            printf("pssh: invalid signal number: [%d]", sig);
            return 0;
        }
        targ_start = 3;
    }
    int i, pid, jobno;
    for (i = targ_start; i < argc; i++)
    {
        if (T.argv[i][0] == '%')
        {
            jobno = atoi(T.argv[i] + 1);
            if (!is_valid_jobno(jobno, job_ids))
            {
                printf("pssh: invalid job number: [%d]", jobno);
                return 0;
            }
            int j;
            for (j = 0; j < jobs[jobno]->npids; j++)
            {
                kill(jobs[jobno]->pids[j], sig);
            }
            if(sig == 18)
            {
                jobs[jobno]->status = BG;
            }
        }
        else
        {
            pid = atoi(T.argv[i]);
            kill(pid, sig);
        }
    }
    return 1;
}

void builtin_jobs(Job **jobs, int *job_ids)
{
    char *status;

    int i;
    for (i = 0; i < 100; i++)
    {

        if (job_ids[i])
        {
            switch (jobs[i]->status)
            {
            case STOPPED:
                status = "Stopped";
                break;
            case TERM:
                status = "Terminated";
                break;
            case BG:
                status = "Running";
                break;
            case FG:
                status = "Running";
                break;
            }
            printf("[%d] + %s    %s\n", i, status, jobs[i]->name);
        }
    }
}
void builtin_fg(Task T, Job **jobs, int *job_ids)
{
    int argc = num_args(T);
    int jobno;
    if (argc != 2)
    {
        printf("Usage: fg %%<job number>\n");
        // return 1;
    }
    jobno = atoi(T.argv[1] + 1);
    if (T.argv[1][0] != '%')
    {
        printf("pssh: invalid job number: [%s]\n", T.argv[1]);
        // return 0;
    }
    else if (!is_valid_jobno(jobno, job_ids))
    {
        printf("pssh: invalid job number: [%s]\n", T.argv[1]);
        // return 0;
    }
    else
    {
        printf("pssh: fg jobno %d; pgid: %d\n", jobno,jobs[jobno]->pgid);
        print_bg_job(jobs[jobno], jobno);
        if(jobs[jobno]->status == STOPPED)
        {
            int i;
            for (i = 0; i < jobs[jobno]->npids; i++)
            {
                kill(jobs[jobno]->pids[i], 18);
            }
        }
        // printf("pssh: fg jobno %d\n", jobno);
        jobs[jobno]->status = FG;

        set_fg_pgrp(getpgid(jobs[jobno]->pids[0]));
    }
    // return 1;
}

void builtin_execute(Task T, Job **jobs, int *job_ids)
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
    else if (!strcmp(T.cmd, "bg"))
    {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
    else
    {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
