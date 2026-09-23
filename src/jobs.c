#define _POSIX_C_SOURCE 200809L

#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static Job job_table[MAX_JOBS];
static int next_job_id = 1;

void jobs_init(void)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        job_table[i].job_id = 0;
        job_table[i].pgid = 0;
        job_table[i].state = JOB_DONE;
        job_table[i].command = NULL;
    }

    next_job_id = 1;
}

int job_add(pid_t pgid, JobState state, const char *command)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id == 0) {

            job_table[i].job_id = next_job_id++;
            job_table[i].pgid = pgid;
            job_table[i].state = state;

            if (command != NULL) {
                job_table[i].command = strdup(command);

                if (job_table[i].command == NULL) {
                    job_table[i].job_id = 0;
                    return -1;
                }
            } else {
                job_table[i].command = strdup("");
            }

            return job_table[i].job_id;
        }
    }

    fprintf(stderr, "jobs: maximum number of jobs reached\n");
    return -1;
}

Job *job_find(int job_id)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id == job_id) {
            return &job_table[i];
        }
    }

    return NULL;
}

Job *job_find_by_pgid(pid_t pgid)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id != 0 &&
            job_table[i].pgid == pgid) {
            return &job_table[i];
        }
    }

    return NULL;
}

void job_remove(int job_id)
{
    Job *job = job_find(job_id);

    if (job == NULL) {
        return;
    }

    free(job->command);

    job->command = NULL;
    job->job_id = 0;
    job->pgid = 0;
    job->state = JOB_DONE;
}

static const char *job_state_string(JobState state)
{
    switch (state) {
        case JOB_RUNNING:
            return "Running";

        case JOB_STOPPED:
            return "Stopped";

        case JOB_DONE:
            return "Done";

        default:
            return "Unknown";
    }
}

void jobs_print(void)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].job_id != 0) {
            printf("[%d] %-9s %s\n",
                   job_table[i].job_id,
                   job_state_string(job_table[i].state),
                   job_table[i].command ?
                   job_table[i].command : "");
        }
    }
}

void job_stop(pid_t pgid)
{
    Job *job = job_find_by_pgid(pgid);

    if (job != NULL) {
        job->state = JOB_STOPPED;
    }
}

void job_continue(pid_t pgid)
{
    Job *job = job_find_by_pgid(pgid);

    if (job != NULL) {
        job->state = JOB_RUNNING;
        kill(-pgid, SIGCONT);
    }
}

void job_done(pid_t pgid)
{
    Job *job = job_find_by_pgid(pgid);

    if (job != NULL) {
        job->state = JOB_DONE;
    }
}
