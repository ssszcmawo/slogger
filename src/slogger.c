#define _GNU_SOURCE
#include "slogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <limits.h>

#define CONSOLELOGGER 1
#define FILELOGGER    2

#define DEFAULT_MAX_FILE_SIZE    (1024 * 1024)
#define DEFAULT_MAX_BACKUP_FILES 10

#define LOG_FILENAME_MAX   4096
#define LOG_BUFFER_SIZE    8192
#define TIMESTAMP_BUF_SIZE 32

typedef struct {
    FILE* output;
    log_level_t level;
} ConsoleLog;

typedef struct {
    FILE* output;
    char fileName[LOG_FILENAME_MAX];
    long maxFileSize;
    long currentFileSize;
    int maxBackups;
    log_level_t level;
} FileLog;

static ConsoleLog* g_console = NULL;
static FileLog*    g_file    = NULL;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t  g_once  = PTHREAD_ONCE_INIT;

static log_level_t g_log_level = INFO;
static int g_logger_type = 0;

static inline void lock(void)   { pthread_mutex_lock(&g_mutex); }
static inline void unlock(void) { pthread_mutex_unlock(&g_mutex); }

static inline long thread_id(void) {
    return (long)syscall(SYS_gettid);
}

static void logger_init_once(void) { }

static inline void ensure_init(void) {
    pthread_once(&g_once, logger_init_once);
}

void set_log_level(log_level_t level) {
    g_log_level = level;
}

static const char* level_to_string(log_level_t lvl) {
    switch (lvl) {
        case DEBUG:   return "DEBUG";
        case WARNING: return "WARN";
        case ERROR:   return "ERROR";
        case TRACE:   return "TRACE";
        default:      return "INFO";
    }
}

static const char* level_to_color(log_level_t lvl) {
    switch (lvl) {
        case DEBUG:   return COLOR_BLUE;
        case WARNING: return COLOR_YELLOW;
        case ERROR:   return COLOR_RED;
        case TRACE:   return COLOR_ORANGE;
        default:      return COLOR_GREEN;
    }
}

static void format_timestamp(char* buf, size_t size) {
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm);
    size_t len = strlen(buf);
    snprintf(buf + len, size - len, ".%06ld", (long)tv.tv_usec);
}

int init_consoleLog(FILE* output) {
    ensure_init();
    lock();

    if (g_console) {
        unlock();
        return 1;
    }

    g_console = calloc(1, sizeof(ConsoleLog));
    if (!g_console) {
        unlock();
        return 0;
    }

    g_console->output = output ? output : stdout;
    g_console->level  = g_log_level;

    g_logger_type |= CONSOLELOGGER;
    unlock();
    return 1;
}

static int get_file_size(const char* path, long* size) {
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    *size = st.st_size;
    return 1;
}

static void rotate_backups(const char* base, int max) {
    char old_path[PATH_MAX];
    char new_path[PATH_MAX];

    for (int i = max; i >= 2; --i) {
        snprintf(old_path, sizeof(old_path), "%s.%d", base, i - 1);
        snprintf(new_path, sizeof(new_path), "%s.%d", base, i);
        rename(old_path, new_path);
    }

    snprintf(new_path, sizeof(new_path), "%s.1", base);
    rename(base, new_path);
}

static int rotate_files(void) {
    if (!g_file || !g_file->output)
        return 0;

    if (g_file->currentFileSize < g_file->maxFileSize)
        return 1;

    fclose(g_file->output);
    g_file->output = NULL;

    rotate_backups(g_file->fileName, g_file->maxBackups);

    g_file->output = fopen(g_file->fileName, "w");
    if (!g_file->output)
        return 0;

    g_file->currentFileSize = 0;
    return 1;
}

int init_fileLog(const char* filename, long maxFileSize) {
    ensure_init();
    lock();

    if (g_file) {
        unlock();
        return 1;
    }

    FileLog* fl = calloc(1, sizeof(FileLog));
    if (!fl) {
        unlock();
        return 0;
    }

    fl->maxFileSize = maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE;
    fl->maxBackups  = DEFAULT_MAX_BACKUP_FILES;
    fl->level       = g_log_level;

    strncpy(fl->fileName, filename, sizeof(fl->fileName) - 1);

    fl->output = fopen(fl->fileName, "a");
    if (!fl->output) {
        free(fl);
        unlock();
        return 0;
    }

    get_file_size(fl->fileName, &fl->currentFileSize);

    g_file = fl;
    g_logger_type |= FILELOGGER;
    unlock();
    return 1;
}

static void log_message(log_level_t level, const char* msg) {
    if (level < g_log_level)
        return;

    ensure_init();
    lock();

    char ts[TIMESTAMP_BUF_SIZE];
    format_timestamp(ts, sizeof(ts));
    long tid = thread_id();

    if ((g_logger_type & CONSOLELOGGER) && g_console) {
        fprintf(g_console->output, "%s%s [%s] [T%ld] %s%s\n",
                level_to_color(level),
                ts,
                level_to_string(level),
                tid,
                msg,
                COLOR_RESET);
        fflush(g_console->output);
    }

    if ((g_logger_type & FILELOGGER) && g_file && g_file->output) {
        long written = fprintf(g_file->output, "%s [%s] [T%ld] %s\n",
                               ts,
                               level_to_string(level),
                               tid,
                               msg);
        if (written > 0) {
            g_file->currentFileSize += written;
            if (g_file->currentFileSize >= g_file->maxFileSize)
                rotate_files();
        }
        fflush(g_file->output);
    }

    unlock();
}

void log_messagef(log_level_t level, const char* file, int line, const char* fmt, ...) {
    if (level < g_log_level)
        return;

    char buf[LOG_BUFFER_SIZE - 64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    char msg[LOG_BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "%s:%d: %s", file, line, buf);
    log_message(level, msg);
}

void close_logging(void) {
    lock();

    if (g_console) {
        free(g_console);
        g_console = NULL;
    }

    if (g_file) {
        if (g_file->output)
            fclose(g_file->output);
        free(g_file);
        g_file = NULL;
    }

    g_logger_type = 0;
    unlock();
}

