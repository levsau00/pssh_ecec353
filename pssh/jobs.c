#include <stdlib.h>
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
void free_job(Job *job)
{
    free(job->name);
    free(job->pids);
    free(job);
}