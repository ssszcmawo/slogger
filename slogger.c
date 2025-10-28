#define _GNU_SOURCE
#include "slogger.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>

static log_level_t current_log_level = INFO;
static pthread_mutex_t l_mutex;
static volatile int logger_initialized = 0;
static volatile int logger_type = 0;

static ConsoleLog* g_console = NULL;
static FileLog* g_file = NULL;

static void lock(void) {
    pthread_mutex_lock(&l_mutex);
}

static void unlock(void) {
    pthread_mutex_unlock(&l_mutex);
}

static long getCurrentThreadID(void) {
    return (long)syscall(SYS_gettid);
}

static void ensure_init(void) {
    if (logger_initialized) return;
    pthread_mutex_init(&l_mutex, NULL);
    logger_initialized = 1;
}

static void format_timestamp(char* buf, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm);
    size_t len = strlen(buf);
    snprintf(buf + len, (len < size) ? size - len : 0, ".%06ld", (long)tv.tv_usec);
}

static const char* level_to_string(log_level_t lvl) {
    switch (lvl) {
        case DEBUG: return "DEBUG";
        case WARNING: return "WARN";
        case ERROR: return "ERROR";
        default: return "INFO";
    }
}

void set_log_level(log_level_t level) {
    current_log_level = level;
}

log_level_t get_log_level(void) {
    return current_log_level;
}

ConsoleLog* init_consoleLog(FILE* output) {
    ensure_init();
    lock();
    ConsoleLog* cl = calloc(1, sizeof(ConsoleLog));
    cl->output = (output ? output : stdout);
    g_console = cl;
    logger_type |= CONSOLELOGGER;
    unlock();
    return cl;
}

FileLog* init_fileLog(const char* filename, long maxFileSize) {
    ensure_init();
    lock();
    FileLog* fl = calloc(1, sizeof(FileLog));
    strncpy(fl->fileName, filename, MAX_FILE_NAME - 1);
    fl->maxFileSize = (maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE);
    fl->output = fopen(filename, "a");
    fseek(fl->output, 0, SEEK_END);
    fl->currentFileSize = ftell(fl->output);
    g_file = fl;
    logger_type |= FILELOGGER;
    unlock();
    return fl;
}

void free_console_log(ConsoleLog* cl) {
    if (!cl) return;
    free(cl);
    g_console = NULL;
}

void free_file_log(FileLog* fl) {
    if (!fl) return;
    if (fl->output) fclose(fl->output);
    free(fl);
    g_file = NULL;
}

void log_message(log_level_t level, const char* message) {
    if (level < current_log_level) return;

    ensure_init();
    lock();

    char timestamp[32];
    format_timestamp(timestamp, sizeof(timestamp));
    long tid = getCurrentThreadID();

    if (logger_type & CONSOLELOGGER && g_console) {
        fprintf(g_console->output, "%s [%s] [T%ld] %s\n", timestamp, level_to_string(level), tid, message);
        fflush(g_console->output);
    }

    if (logger_type & FILELOGGER && g_file) {
        if (g_file->currentFileSize >= g_file->maxFileSize) {
            freopen(g_file->fileName, "w", g_file->output);
            g_file->currentFileSize = 0;
        }
        long size = fprintf(g_file->output, "%s [%s] [T%ld] %s\n", timestamp, level_to_string(level), tid, message);
        g_file->currentFileSize += size;
        fflush(g_file->output);
    }

    unlock();
}

void clear_log_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file) fclose(file);
}

void close_logging(void) {
    lock();
    if (g_console) free_console_log(g_console);
    if (g_file) free_file_log(g_file);
    unlock();
}

