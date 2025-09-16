/* Program - job_manager.c */

#include "job_manager.h"
#include <stdlib.h>

static struct job_metadata* head = NULL;

struct job_metadata* get_new_job() {
    struct job_metadata* job = (struct job_metadata*)malloc(sizeof(struct job_metadata));
    job->next = NULL;

    return job;
}

int add_new_job(pid_t pid, pid_t pgid, enum job_state state, char* cmdline, int is_background) {
    struct job_metadata *ptr = head, *prev = NULL;
    
    if (head == NULL) {
        head = get_new_job();
        head->pid = pid;
        head->cmdline = cmdline;
        head->pgid = 0;
        head->state = state;
        head->is_background = is_background;
        return 1;
    }

    while (ptr != NULL) {
        prev = ptr;
        ptr = ptr->next;
    }

    struct job_metadata* job = get_new_job();
    if (job == NULL)
        return 0;
    prev->next = job;

    job->state = state;
    job->cmdline = cmdline;
    job->is_background = is_background;
    job->pid = pid;
    job->pgid = pgid;

    return 1;
}

int update_job_status(pid_t pid, enum job_state new_state) {
    struct job_metadata *ptr = head;

    while (pid != ptr->pid && ptr->next != NULL) {
        ptr = ptr->next;
    }
    printf("state %d  ", ptr->state);
    ptr->state = new_state;
    return 1;
}

void display_all_jobs(int mask) {

    struct job_metadata *ptr = head;
    if (head->next != NULL)
        ptr = head->next;

    while (ptr != NULL) {
        
        if ((mask & JOBS_FOREGROUND) && !ptr->is_background) {
             printf(" [%d] \t %s \t (%s)\n", ptr->pid, ptr->cmdline, 
               (ptr->state == DONE) ? "Done" :
               (ptr->state == RUNNING) ? "Running" :
               "Stopped");
        }

        if((mask & JOBS_BACKGROUND) && ptr->is_background) {
             printf(" [%d] \t %s \t (%s)  & \n", ptr->pid, ptr->cmdline,
               (ptr->state == DONE) ? "Done" :
               (ptr->state == RUNNING) ? "Running" :
               "Stopped");
        }
        ptr = ptr->next;
    }
    fflush(stdout);
}

struct job_metadata* get_job_by_pid(pid_t pid) {
    struct job_metadata *ptr = head;

    while (ptr != NULL || ptr->pid != pid)
        ptr = ptr->next;

    if (ptr->pid == pid)
        return ptr;
    return NULL;
}

void free_memory_of_job(struct job_metadata* job) {
    if (job != NULL) {
        free_memory_of_job(job->next);
        free(job);
    }
}

void jobs_memory_cleanup() {
    free_memory_of_job(head);
}

void init_job_table() {

    head = (struct job_metadata *)malloc(sizeof(struct job_metadata));
    head->pid = 0;
}

/* NOTE: used for testing the job table i.e. linked list */

/* 
int main() {
    
    init_job_table();

    
    //struct job_metadata *new_job = (struct job_metadata *)malloc(sizeof(struct job_metadata));
    
    //head->pid = 0;
    add_new_job(101,0, STOPPED, "pause &", 1);
    add_new_job(202,0, DONE, "cat &", 1);
    add_new_job(303, 0, RUNNING, "ls -ltr", 0);
    add_new_job(404, 0, RUNNING, "ls -ltr", 0);
    add_new_job(505, 0, RUNNING, "ls -ltr", 0);

    if (add_new_job(202, 0, RUNNING, "ls -ltr", 1) == 1) {
        display_all_jobs(JOBS_ALL);
        printf("No issues\n");
        display_all_jobs(JOBS_BACKGROUND);
    }
    else
        printf("theres a problem");
}
*/
