#ifndef _jobs_h_
#define _jobs_h_

#include <fcntl.h>
#include "parse.h"

typedef enum
{
    STOPPED,
    TERM,
    BG,
    FG,
} JobStatus;
typedef struct
{
    char *name;
    pid_t *pids;
    unsigned int npids;
    pid_t pgid;
    JobStatus status;
} Job;

Job *new_job(char *name, Parse *P);
int next_jid(int *job_ids);
#endif /* _jobs_h_ */