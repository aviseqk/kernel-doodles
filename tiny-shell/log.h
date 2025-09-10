/* Standard small logging utility for system prog projects in C */
/* A macro that defines level and exposes functions that will be enabled or disabled based on inputs given on compile time.
    Instead of standard output for such debug messages, these exposed functions will be used, this maintains cleanliness
    NOTE: This is only for printf and other debug functions, not the outputs needed for STDOUT and all.
    NOTE: This will be printing to STDERR, so as to keep the debug info separate, and not bringing the whole logging framework
*/
#ifndef LOG_H
#define LOG_H
#include <stdio.h>

// default log level - not print any debug or info messages
#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

// default timestamp logging - enable
#ifndef TIMESTAMP
#define TIMESTAMP
#endif

// Levels -> 0 = None, 1 = Error, 2 = Info, 3 = Debug
#define LOG_ERROR 1
#define LOG_INFO 2
#define LOG_DEBUG 3

#ifdef TIMESTAMP
#define LOG(level, fmt, ...) \
        do { \
            if (level <= LOG_LEVEL) { \
                fprintf(stderr, "[%s] [%s %s] " fmt "\n", (level == LOG_ERROR) ? "ERROR" : (level == LOG_INFO) ? "INFO" : "DEBUG", __DATE__, __TIME__, ##__VA_ARGS__);}   \
        } while(0) 

#else
#define LOG(level, fmt, ...) \
        do { \
            if (level <= LOG_LEVEL) { \
                fprintf(stderr, "[%s] " fmt "\n", (level == LOG_ERROR) ? "ERROR" : (level == LOG_INFO) ? "INFO" : "DEBUG", ##__VA_ARGS__);}   \
        } while(0) 

#endif
/* TODO replace or add a macro , in case of such iterative logs,
for the scenario when a whole log string is to be built on the go based on sub-logs/sub-strings
and then LOG is called once */

#endif