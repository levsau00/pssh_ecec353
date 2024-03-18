#ifndef _builtin_h_
#define _builtin_h_

#include "parse.h"
#include "jobs.h"

int is_builtin (char* cmd);
void builtin_execute (Task T, Job **jobs, int *job_ids);
int builtin_which (Task T);
void builtin_jobs(Job **jobs, int *job_ids);
int is_valid_jobno(int jobno, int *job_ids);
int builtin_kill(Task T, Job **jobs, int *job_ids);
void builtin_fg(Task T, Job **jobs, int *job_ids);
int builtin_bg(Task T, Job **jobs, int *job_ids);
char *command_found_builtin(const char *cmd);
#endif /* _builtin_h_ */
