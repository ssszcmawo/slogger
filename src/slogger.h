#ifndef SLOGGER_H
#define SLOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

#define COLOR_RESET  "\033[0m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_ORANGE "\033[38;5;208m"

typedef enum {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    TRACE
} log_level_t;

void set_log_level(log_level_t level);

int init_consoleLog(FILE* output);

int init_fileLog(const char* filename, long maxFileSize);

void log_messagef(log_level_t level,
                  const char* file,
                  int line,
                  const char* fmt,
                  ...);

void close_logging(void);

#define LOG_TRACE(fmt, ...) log_messagef(TRACE,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log_messagef(DEBUG,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_messagef(INFO,    __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_messagef(WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_messagef(ERROR,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

