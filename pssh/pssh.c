#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <wait.h>
#include <fcntl.h>
#include <signal.h>
#include "builtin.h"
#include "parse.h"
#include "jobs.h"

/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0

#define WRITE_SIDE 1
#define READ_SIDE 0

Job *jobs[100];
int job_ids[110];

void handler(int sig)
{
    pid_t chld;
    int status;
    switch (sig)
    {
    case SIGCHLD:
        while ((chld = waitpid(-1, &status, WNOHANG | WCONTINUED | WUNTRACED)) > 0)
        {
            if (WIFCONTINUED(status))
            {
                /* child state changed from STOPPED to RUNNING (received SIGCONT) */
                printf("Child w/ pid %d continued\n", chld);
            }
            else if (WIFSTOPPED(status))
            {
                /* child state changed to STOPPED (received SIGSTOP, SIGTTOU, or SIGTTIN) */
                printf("Child w/ pid %d stopped\n", chld);
            }
            else if (WIFEXITED(status))
            {
                /* child exited normally */
                printf("Child w/ pid %d exited\n", chld);
            }
            else if (WIFSIGNALED(status))
            {
                /* child exited due to uncaught signal */
                printf("Child w/ pid %d killed\n", chld);
            }
            else
            {
                /* waited on terminated child */
            }
        }
        break;
    case SIGTTOU:
        printf("SIGTTOU\n");
        break;
    default:
        printf("Other signal\n");
        break;
    }
}

void print_banner()
{
    printf("                    ________   \n");
    printf("_________________________  /_  \n");
    printf("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf("__  /_/ /(__  )_(__  )_  / / / \n");
    printf("_  .___//____/ /____/ /_/ /_/  \n");
    printf("/_/ Type 'exit' or ctrl+c to quit\n\n");
}

/* **returns** a string used to build the prompt
 * (DO NOT JUST printf() IN HERE!)
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char *build_prompt()
{
    char *full;
    char *cwd;
    char prompt[] = "$ ";
    if ((cwd = getcwd(NULL, 0)))
    {
        full = malloc(strlen(cwd) + strlen(prompt) + 1);
        sprintf(full, "%s%s", cwd, prompt);
        free(cwd);
        return full;
    }

    return "$ ";
}

/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found(const char *cmd)
{
    char *dir;
    char *tmp;
    char *PATH;
    char *state;
    char probe[PATH_MAX];

    int ret = 0;

    if (access(cmd, X_OK) == 0)
        return 1;

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
            ret = 1;
            break;
        }
    }

    free(PATH);
    return ret;
}

static void redirect(int fd_old, int fd_new)
{
    if (fd_new != fd_old)
    {
        dup2(fd_new, fd_old);
        close(fd_new);
    }
}
static int close_safe(int fd)
{
    if ((fd != STDIN_FILENO) && (fd != STDOUT_FILENO))
        return (close(fd));

    return -1;
}
static void run(Task *T, int in, int out)
{
    redirect(STDIN_FILENO, in);
    redirect(STDOUT_FILENO, out);

    if (is_builtin(T->cmd))
        builtin_execute(*T);
    else if (command_found(T->cmd))
        execvp(T->cmd, T->argv);
}
static int get_infile(Parse *P)
{
    if (P->infile)
        return open(P->infile, O_RDONLY);
    else
        return STDIN_FILENO;
}
static int get_outfile(Parse *P)
{
    if (P->outfile)
        return open(P->outfile, O_WRONLY);
    else
        return STDOUT_FILENO;
}
static int is_possible(Parse *P)
{
    unsigned int t;
    Task *T;
    int fd;

    for (t = 0; t < P->ntasks; t++)
    {
        T = &P->tasks[t];
        if (!is_builtin(T->cmd) && !command_found(T->cmd))
        {
            fprintf(stderr, "pssh: command not found: %s\n", T->cmd);
            return 0;
        }

        if (!strcmp(T->cmd, "exit"))
        {
            printf("Exiting...\n");
            exit(EXIT_SUCCESS);
        }
    }

    if (P->infile)
    {
        if (access(P->infile, R_OK) != 0)
        {
            fprintf(stderr, "pssh: no such file or directory: %s\n", P->infile);
            return 0;
        }
    }

    if (P->outfile)
    {
        if ((fd = creat(P->outfile, 0666)) == -1)
        {
            fprintf(stderr, "pssh: permission denied: %s\n", P->outfile);
            return 0;
        }
        close(fd);
    }

    return 1;
}
void print_job_pids(Job *job)
{
    printf("job pids:");
    unsigned int t;
    for (t = 0; t < job->npids; t++)
    {
        printf("%d ", job->pids[t]);
    }
    printf("\n");
}
/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks(Parse *P, int job_id)
{
    unsigned int t;
    int fd[2];
    int in, out;

    in = get_infile(P);
    for (t = 0; t < P->ntasks - 1; t++)
    {
        pipe(fd);
        jobs[job_id]->pids[t] = fork();

        setpgid(jobs[job_id]->pids[t], jobs[job_id]->pids[0]);
        if (jobs[job_id]->pids[t])
        {
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDIN_FILENO, jobs[job_id]->pids[0]);
            signal(SIGTTOU, SIG_DFL);
        }

        if (!jobs[job_id]->pids[t])
        {
            close(fd[READ_SIDE]);
            run(&P->tasks[t], in, fd[WRITE_SIDE]);
        }
        close(fd[WRITE_SIDE]);
        close_safe(in);

        in = fd[READ_SIDE];
    }

    out = get_outfile(P);

    // print_job_pids(jobs[job_id]);
    jobs[job_id]->pids[t] = fork();
    // print_job_pids(jobs[job_id]);
        
    setpgid(jobs[job_id]->pids[t], jobs[job_id]->pids[0]);
    if (jobs[job_id]->pids[t])
    {
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(STDIN_FILENO, jobs[job_id]->pids[0]);
        signal(SIGTTOU, SIG_DFL);
    }

    if (!jobs[job_id]->pids[t])
        run(&P->tasks[t], in, out);

    for (t = 0; t < P->ntasks; t++)
    {
        waitpid(jobs[job_id]->pids[t], NULL, 0);
    }
    close_safe(in);
}

int main(int argc, char **argv)
{
    char *cmdline;
    Parse *P;
    int next_id;
    memset(job_ids, 0, 100 * sizeof(int));

    signal(SIGCHLD, handler);
    signal(SIGSTOP, handler);
    signal(SIGQUIT, handler);
    signal(SIGTERM, handler);
    signal(SIGKILL, handler);
    signal(SIGTTOU, handler);
    signal(SIGTTIN, handler);

    print_banner();

    while (1)
    {
        // printf("top o while\n");
        cmdline = readline(build_prompt());
        if (!cmdline) /* EOF (ex: ctrl-d) */
            exit(EXIT_SUCCESS);
        // printf("cmdline read\n");
        P = parse_cmdline(cmdline);
        // printf("cmdline parsed\n");
        if (!P)
            goto next;

        if (P->invalid_syntax)
        {
            printf("pssh: invalid syntax\n");
            goto next;
        }
        if (!is_possible(P))
            goto next;
        if ((next_id = next_jid(job_ids)) < 0)
        {
            printf("pssh: job buffer is full\n");
            goto next;
        }

#if DEBUG_PARSE
        printf("debug parse\n");
        parse_debug(P);
#endif

        jobs[next_id] = new_job(cmdline, P);
        execute_tasks(P, next_id);
        // printf("tasks executed\n");

    next:
        // printf("in next\n");
        parse_destroy(&P);
        free(cmdline);
    }
}
