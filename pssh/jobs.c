#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jobs.h"
#include "parse.h"

Job *new_job(char *name, Parse *P)
{
    Job *job = malloc(sizeof(Job));
    job->name = malloc(strlen(name) + 1);
    strcpy(job->name, name);
    job->npids = P->ntasks;
    job->pids = malloc(sizeof(pid_t) * P->ntasks);
    job->completed = 0;
    job->continued = 0;
    job->suspended = 0;

    if (P->background)
        job->status = BG;
    else
        job->status = FG;

    return job;
}
int next_jid(int *job_ids)
{
    int i;
    for (i = 0; i < 100; i++)
    {
        if (!job_ids[i])
        {
            job_ids[i]++;
            return i;
        }
    }
    return -1;
}

int find_jid(Job **jobs, pid_t pid)
{
    int i, j;
    for (i = 0; i < 100; i++)
    {
        if (!jobs[i])
            continue;
        for (j = 0; j < jobs[i]->npids; j++)
        {
            if (jobs[i]->pids[j] == pid)
            {
                return i;
            }
        }
    }
    return -1;
}


void print_bg_job(Job *job, int jid)
{
    int i;
    printf("[%d] ", jid);
    for (i = 0; i < job->npids; i++)
    {
        printf("%d ", job->pids[i]);
    }
    printf("\n");
}

void free_job(Job *job)
{
    free(job->name);
    free(job->pids);
    free(job);
}

void free_job_safe(Job **jobs, Job *job, int *job_ids)
{
    int job_id = find_jid(jobs, job->pids[0]);
    free_job(job);
    if (job_id != -1)
    {
        job_ids[job_id] = 0;
        jobs[job_id] = NULL;
    }
}