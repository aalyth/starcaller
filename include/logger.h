#ifndef STARCALLER_LOGGER_H
#define STARCALLER_LOGGER_H

void log_debug(const char *, ...);
void log_info(const char *, ...);
void log_warn(const char *, ...);
void log_error(const char *, ...);
void log_fatal(int, const char *, ...);

#endif
