#define _GNU_SOURCE
#include "utility.h"
#include "log.h"
#include "job_manager.h"

#define MAX_ARGS 64

struct child_event {
    pid_t pid;
    int status;
};
int sigchld_pipe[2];
pid_t shell_pgid;
struct sigaction sa_chld;


int main(int argc, char *argv[])
{
    printf("starting the tiny-shell\n");

    // setup signal for the signals - SIGCHLD (sent to parent when child dies), SIGINT(sent from terminal by Ctrl+C)
    install_signal_handlers();

    create_self_pipe();

    init_job_table();

    for(;;) {

        struct child_event chev;
        // before taking new input, drain the self-pipe of any child that must have ended
        while (read(sigchld_pipe[0], &chev, sizeof(chev)) > 0) {

            if (WIFEXITED(chev.status)) {
                update_job_status(chev.pid, DONE);
                printf("  [%d]  - Exited with status %d\n", chev.pid, WEXITSTATUS(chev.status));
            } else if (WIFSIGNALED(chev.status)) {
                update_job_status(chev.pid, DONE);
                printf("  [%d] - Killed by signal %d\n", chev.pid, WTERMSIG(chev.status));
            } else if (WIFSTOPPED(chev.status)) {
                update_job_status(chev.pid, STOPPED);
                printf("  [%d] - Stopped by signal %d\n", chev.pid, WSTOPSIG(chev.status));
            } else if (WIFCONTINUED(chev.status)) {
                //TODO //update_job_status(chev.pid, CONTINUED);
                printf("  [%d] - Continued\n", chev.pid);
            }
        }
    
        write(STDOUT_FILENO, "tiny-shell>", 11);

        char *args_array[MAX_ARGS];
        char *command = NULL;
        size_t bufsize = 0;
        ssize_t num_read;
        int arg_count = 0;
        int is_background = 0;

        // TODO ADVANCED -> can make I/O multiplexed, so that getline blocking function is not used and we can get info of child processes async, even when idle at prompt
        num_read = getline(&command, &bufsize, stdin);

        if(num_read == -1) {
            int readerr = errno;
            if (readerr == EINVAL || readerr == ENOMEM) {
                write(STDERR_FILENO, "unable to read", 15);
                free(command);
                continue;
            }
        }

        if (command[num_read - 1] == '\n') {
            command[num_read - 1] = '\0';
        }
        LOG(LOG_DEBUG, "command line from input: %s (size: %ld)", command, bufsize);

        arg_count = parse_prompt_commands(command, args_array);

        if (arg_count == 0) {   // case when only return was entered, meaning no command, only the newline was entered so bytes read = 1 otherwise > 1
            free(command);
            continue;
        }
        LOG(LOG_DEBUG, "arg count post parsing: %d", arg_count);

        if ((strncmp(args_array[0], "exit", 4) == 0) && arg_count == 1) {
            write(STDOUT_FILENO, "Exiting the tiny-shell", 23);
            free(command);
            jobs_memory_cleanup();
            break;
        }

        if (filter_job_queries_and_display(args_array[0]))
            continue;

        if (arg_count > 0 && strcmp(args_array[arg_count - 1], "&") == 0) {
            is_background = 1;
            args_array[arg_count - 1] = NULL;
        }

        pid_t wpid, cpid;
        int wstatus;
        switch((cpid = fork())) {
            case -1:
                perror("fork");
                free(command);
                jobs_memory_cleanup();
                break;
            
            case 0:
                // this is the child
                LOG(LOG_INFO, "[Child] Child PID is %d", (int)getpid());

                // restore the normal SIGINT signal for the child process so it can terminate if terminal commands
                restore_terminate_signals();
                
                // if (signal(SIGINT, NULL) == SIG_ERR) {
                //     perror("signal handler child");
                //     LOG(LOG_ERROR, "signal handler setup error in parent");
                //     exit(EXIT_FAILURE);
                // }
                
                /* replacing the process image with execve */
                int res = execvp(args_array[0], args_array);
                LOG(LOG_DEBUG, "[Child] Child exiting");
                if (res == -1)
                    exit(101);
                break;

            default:
                // this is the parent
                LOG(LOG_INFO, "[Parent] Parent PID is %d", (int)getpid());

                if (is_background) {
                    /* background job -> dont waitpid */
                    printf("Tis' a background job \n");
                    add_new_job(cpid, 0, RUNNING, command, 0);
                    continue;
                } else {
                    // the parent must wait to reap the child, otherwise zombie children would overwhelm the process table of kernel and no more forks allowed
                    
                    add_new_job(cpid, 0, RUNNING, command, 1);
                    
                    wpid = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED | WNOHANG);
                    if (wpid == -1) {
                        update_job_status(wpid, STOPPED);
                        perror("waitpid");
                        continue;
                    }
                    if (WIFSIGNALED(wstatus)) {
                        update_job_status(wpid, STOPPED);
                        LOG(LOG_INFO, "[Parent] Child closed by a signal");
                    } else if (WIFEXITED(wstatus)) {
                        update_job_status(wpid, DONE);
                        LOG(LOG_INFO, "[Parent] Child closed properly");
                    } else if (WEXITSTATUS(wstatus) == 101) {
                        update_job_status(wpid, DONE);
                        LOG(LOG_INFO, "[Parent] Child errored out as execvp failed");
                    } else {
                        update_job_status(wpid, STOPPED);
                        LOG(LOG_INFO, "[Parent] Child closed otherwise - status %d", wstatus);
                    }
                }
                LOG(LOG_DEBUG, "[Parent] Parent exiting");
                fflush(stdout);
                break;
        }

        free(command);
        jobs_memory_cleanup();
    }
    
    // explicitly ignoring these parameters as currently not being used
    (void)argc;
    (void)argv;
}

int filter_job_queries_and_display(char *command) {

    if (strncmp(command, "jobs", 4) ==0) {
        display_all_jobs(JOBS_ALL);
        return 1;
    }
        
    if (strncmp(command, "fg", 2) ==0) {
        display_all_jobs(JOBS_FOREGROUND);
        return 1;
    }

    if (strncmp(command, "bg", 4) ==0) {
        display_all_jobs(JOBS_BACKGROUND);
        return 1;
    }

    return 0;
}

/* This function takes a line of command and transforms it into an array of pointers to tokens to be used for exec* functions */
int parse_prompt_commands(char *command, char *argv_array[]) {
    int argc = 0;

    LOG(LOG_DEBUG, "command line from input %s", command); 

    /* converting the characters inputted into an array of strings that are token for exec family functions, and making them null-terminated too` */
    char *token = strtok(command, " \t\r\n");
    while(token != NULL && argc < MAX_ARGS - 1) {
        argv_array[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    argv_array[argc] = NULL;      // null-terminated for the exec* programs

    LOG(LOG_INFO, "Arguments: Count: %d Values: ", argc);
    if (argc > 0) {
        for (int i = 0; i <= argc; i++) {
            /* TODO creating macro for single log message for iterative sub-strings addition */
            LOG(LOG_DEBUG, " [%d] = %s  ", i, argv_array[i]);
        }
        //LOG(LOG_DEBUG, "\n");
    }

    return argc;
}

/* This function calls waitpid to reap all the children that changed state and uses async-signal-safe functions to
write information about those terminated/dead children to be reaped to the self pipe created above 
 * That's all the SIG_CHLD handler does. It's the main event loop's responsibility to read and cultivate the info in the pipe by reading
 and then displaying, adding to job, whatever the need be
*/
void sigchld_handler(int sig) {
    int saved_errno = errno;
    pid_t pid;
    int status;
    struct child_event ev;

    // continuous loop to reap as many children as possible and write event structures to the pipe atomically and async-signal-safe fashion
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        // if the pid returned by waitpid > 0, that means a child died and waitpid call returned for it
        ev.status = status;
        ev.pid = pid;

        write(sigchld_pipe[1], &ev, sizeof(ev));
    }
    errno = saved_errno;    // so that if any of above call fails, the errno does not change
    (void)sig;  // explicitly ignore the sig parameter
}

/* This helper function just restores the termination and kill signals for the child process, i.e. our command */
void restore_terminate_signals() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

/* This function install signal handlers for two cases:
    1. SIG_CHLD -> fired when children are killed, so the programmed handler is added, which reaps these dead childrens info and puts to self-pipe
    2. SIGINT | SIGQUIT ... -> other interrupt signals, which parent does not need to handle, so we disable them for parent, so that when terminal fires
        these signals, parent (i.e. our shell) doesn't catch it, rather the children catch it.
        So, during fork + exec call, we would restore these signals to their default actions, so child can terminate accordingly
*/
void install_signal_handlers() {
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {    // parent ignores SIGINT from terminal, so the shell doesn't die
        LOG(LOG_ERROR, "signal parent error SIGINT");
        return;
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) { LOG(LOG_ERROR, "signal parent error SIGTSTP"); return; }
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) { LOG(LOG_ERROR, "signal parent error SIGQUIT"); return; }

    // setup signal handler and mask for sig_chld
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction sig_child");
        _exit(EXIT_FAILURE);
    }
}

void create_self_pipe() {
    if ( pipe2(sigchld_pipe, O_NONBLOCK  | O_CLOEXEC) < 0 ) {   // set pipe on NONBLOCK so that signal handler never blocks on write
        LOG(LOG_ERROR, "pipe2 create error");
        exit(EXIT_FAILURE);
    }
    LOG(LOG_DEBUG, "self pipe file descriptor number %d - %d", sigchld_pipe[0], sigchld_pipe[1]);
}
