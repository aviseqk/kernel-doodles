#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>

int parse_prompt_commands(char *command, char *argv_array[]);

void create_self_pipe();

void restore_terminate_signals();

void install_signal_handlers();

int filter_job_queries_and_display(char *args_array);

#endif

