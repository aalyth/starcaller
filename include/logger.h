#ifndef STARCALLER_LOGGER_H
#define STARCALLER_LOGGER_H

void log_trace(const char *, ...) __attribute__((format(printf, 1, 2)));
void log_debug(const char *, ...) __attribute__((format(printf, 1, 2)));
void log_info(const char *, ...) __attribute__((format(printf, 1, 2)));
void log_warn(const char *, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *, ...) __attribute__((format(printf, 1, 2)));
void log_fatal(int, const char *, ...) __attribute__((format(printf, 2, 3)));

#endif
