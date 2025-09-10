#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include "log.h"
#include <sys/types.h>

enum job_state {
    STOPPED,
    RUNNING,
    DONE
};

enum job_mask {
    JOBS_FOREGROUND = 1 << 0,
    JOBS_BACKGROUND = 1 << 1,
    JOBS_ALL = JOBS_FOREGROUND | JOBS_BACKGROUND
};

struct job_metadata {
    pid_t pid;
    pid_t pgid;
    enum job_state state;
    char *cmdline;
    int exit_status;
    int is_background;
    struct job_metadata *next;
};

void init_job_table();

int add_new_job(pid_t pid, pid_t pgid, enum job_state state, char* cmdline, int is_background);

int update_job_status(pid_t pid, enum job_state new_state);

//char *return_all_job_data(struct job_metadata *job);

struct job_metadata* get_job_by_pid(pid_t pid);

void display_all_jobs(int mask);

void jobs_memory_cleanup();

#endif

