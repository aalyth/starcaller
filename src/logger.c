
#include "logger.h"

#include <time.h>
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <stdarg.h>

#include <pthread.h>

const char *MSG_ERROR = "ERROR";

typedef enum { DEBUG, INFO, WARN, ERROR, FATAL } LogLevel;

static void log_message(LogLevel, const char *, va_list);
static const char *log_level_to_string(LogLevel);

static const char *TIME_FORMAT = "%Y-%m-%d %H:%M:%S";

void log_debug(const char *message, ...)
{
        va_list args;
        va_start(args, message);
        log_message(DEBUG, message, args);
        va_end(args);
}

void log_info(const char *message, ...)
{
        va_list args;
        va_start(args, message);
        log_message(INFO, message, args);
        va_end(args);
}

void log_warn(const char *message, ...)
{
        va_list args;
        va_start(args, message);
        log_message(DEBUG, message, args);
        va_end(args);
}

void log_error(const char *message, ...)
{
        va_list args;
        va_start(args, message);
        log_message(DEBUG, message, args);
        va_end(args);
}

void log_fatal(int exit_code, const char *message, ...)
{
        va_list args;
        va_start(args, message);
        log_message(DEBUG, message, args);
        va_end(args);
        exit(exit_code);
}

static const char *log_level_to_string(LogLevel level)
{
        switch (level) {
        case DEBUG:
                return "DEBUG";
        case INFO:
                return "INFO";
        case WARN:
                return "WARN";
        case ERROR:
                return "ERROR";
        case FATAL:
                return "FATAL";
        default:
                return "UNKNOWN";
        }
}

static void log_message(LogLevel level, const char *message, va_list args)
{
        static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

        const time_t now = time(NULL);
        const char *log_level = log_level_to_string(level);

        pthread_mutex_lock(&log_mutex);
        char serialized_time[64];
        strftime(serialized_time, sizeof(serialized_time), TIME_FORMAT, localtime(&now));

        fprintf(stderr, "%s [%s] ", serialized_time, log_level);
        vfprintf(stderr, message, args);
        fprintf(stderr, "\n");

        fflush(stderr);
        pthread_mutex_unlock(&log_mutex);
}
