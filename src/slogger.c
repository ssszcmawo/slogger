#define _GNU_SOURCE
#include "slogger.h"
#include "zip.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>

#define CONSOLELOGGER 1
#define FILELOGGER    2
#define DEFAULT_MAX_FILE_SIZE   1048576L
#define DEFAULT_MAX_BACKUP_FILES 10
#define LOG_FILENAME_MAX        4096
#define LOG_BUFFER_SIZE         8192
#define TIMESTAMP_BUF_SIZE      32

typedef struct FileLog {
    FILE* output;
    char fileName[LOG_FILENAME_MAX];
    long maxFileSize;
    long currentFileSize;
    log_level_t level;
    int maxBackUpFiles;
    int archiveOldLogs;
    char archiveDir[LOG_FILENAME_MAX];
} FileLog;

typedef struct ConsoleLog {
    FILE* output;
    log_level_t level;
} ConsoleLog;

static ConsoleLog* g_console = NULL;
static FileLog* g_file = NULL;
static pthread_mutex_t l_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t logger_once = PTHREAD_ONCE_INIT;
static log_level_t current_log_level = INFO;
static int logger_type = 0;
static int logger_initialized = 0;

static inline void lock(void)   { pthread_mutex_lock(&l_mutex); }
static inline void unlock(void) { pthread_mutex_unlock(&l_mutex); }
static inline long getCurrentThreadID(void) { return (long)syscall(SYS_gettid); }
static inline void logger_init(void) { logger_initialized = 1; }
static inline void ensure_init(void) { pthread_once(&logger_once, logger_init); }
static inline int hasFlag(int flags, int flag) { return (flags & flag) != 0; }
static inline log_level_t get_log_level(void) { return current_log_level; }

void set_log_level(log_level_t level) { current_log_level = level; }

static int file_exists(const char* path) { struct stat st; return (stat(path, &st) == 0); }

static int count_regular_files(const char* dirpath)
{
    int count = 0;
    DIR* dir = opendir(dirpath);
    if (!dir) return 0;
    struct dirent* ent;
    while ((ent = readdir(dir))) {
        if (ent->d_type == DT_REG) count++;
        else if (ent->d_type == DT_UNKNOWN) {
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dirpath, ent->d_name);
            struct stat st;
            if (stat(full, &st) == 0 && S_ISREG(st.st_mode)) count++;
        }
    }
    closedir(dir);
    return count;
}

static const char* level_to_string(log_level_t lvl)
{
    switch (lvl) {
        case DEBUG:   return "DEBUG";
        case WARNING: return "WARN";
        case ERROR:   return "ERROR";
        default:      return "INFO";
    }
}

static const char* level_to_color(log_level_t lvl)
{
    switch (lvl) {
        case DEBUG:   return COLOR_BLUE;
        case WARNING: return COLOR_YELLOW;
        case ERROR:   return COLOR_RED;
        default:      return COLOR_GREEN;
    }
}

static void format_timestamp(char* buf, size_t size)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm);
    size_t len = strlen(buf);
    snprintf(buf + len, size - len, ".%06ld", (long)tv.tv_usec);
}

int init_consoleLog(FILE* output)
{
    ensure_init();
    lock();
    if (g_console) { unlock(); return 1; }
    ConsoleLog* cl = calloc(1, sizeof(ConsoleLog));
    if (!cl) { unlock(); return 0; }
    cl->output = output ? output : stdout;
    g_console = cl;
    logger_type |= CONSOLELOGGER;
    unlock();
    return 1;
}

int init_fileLog(const char* filename, long maxFileSize, bool archiveOrNot)
{
    ensure_init();
    lock();
    if (g_file) { unlock(); return 1; }

    FileLog* fl = calloc(1, sizeof(FileLog));
    if (!fl) { unlock(); return 0; }

    fl->maxFileSize = maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE;
    fl->maxBackUpFiles = DEFAULT_MAX_BACKUP_FILES;
    fl->archiveOldLogs = archiveOrNot;

    if (archiveOrNot) {
        snprintf(fl->archiveDir, sizeof(fl->archiveDir), "%s_archive", filename);
        fl->archiveDir[sizeof(fl->archiveDir)-1] = '\0';
        if (mkdir(fl->archiveDir, 0755) != 0 && errno != EEXIST) {
            free(fl); unlock(); return 0;
        }
        snprintf(fl->fileName, sizeof(fl->fileName), "%s/%s", fl->archiveDir, filename);
        fl->fileName[sizeof(fl->fileName)-1] = '\0';
    } else {
        strncpy(fl->fileName, filename, sizeof(fl->fileName)-1);
        fl->fileName[sizeof(fl->fileName)-1] = '\0';
        fl->archiveDir[0] = '\0';
    }

    fl->output = fopen(fl->fileName, "a");
    if (!fl->output) {
        fprintf(stderr, "ERROR: Cannot open log file %s\n", fl->fileName);
        free(fl); unlock(); return 0;
    }
    fseek(fl->output, 0, SEEK_END);
    fl->currentFileSize = ftell(fl->output);

    g_file = fl;
    logger_type |= FILELOGGER;
    unlock();
    return 1;
}

static void backup_filename(const char* baseName, int index, char* out, size_t outSize)
{
    if (index <= 0) snprintf(out, outSize, "%s", baseName);
    else            snprintf(out, outSize, "%s.%d", baseName, index);
}

static void rotate_backup_files(const char* baseName, int maxBackups, const char* archiveDir)
{
    char oldPath[PATH_MAX];
    char newPath[PATH_MAX];

    if (maxBackups > 0) {
        backup_filename(baseName, maxBackups, oldPath, sizeof(oldPath));
        if (file_exists(oldPath) && archiveDir && archiveDir[0]) {
            char archived[PATH_MAX];
            const char* slash = strrchr(baseName, '/');
            const char* name = slash ? slash + 1 : baseName;
            snprintf(archived, sizeof(archived), "%s/%s.%d", archiveDir, name, maxBackups);
            rename(oldPath, archived);
        }
    }

    for (int i = maxBackups; i >= 1; --i) {
        backup_filename(baseName, i - 1, oldPath, sizeof(oldPath));
        backup_filename(baseName, i, newPath, sizeof(newPath));
        if (file_exists(oldPath)) rename(oldPath, newPath);
    }
}

static int create_archive_and_cleanup(FileLog* fl)
{
    if (!fl->archiveOldLogs || !fl->archiveDir[0]) return 1;

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    const char* slash = strrchr(fl->fileName, '/');
    const char* fullname = slash ? slash + 1 : fl->fileName;
    const char* dot = strrchr(fullname, '.');
    char basename[128] = "log";

    if (dot && dot != fullname) {
        size_t len = dot - fullname;
        if (len >= sizeof(basename)) len = sizeof(basename) - 1;
        memcpy(basename, fullname, len);
        basename[len] = '\0';
    } else if (fullname[0]) {
        strncpy(basename, fullname, sizeof(basename) - 1);
    }

    char zip_name[PATH_MAX];
    snprintf(zip_name, sizeof(zip_name),
             "%s_archive_%04d%02d%02d_%02d%02d%02d.zip",
             basename,
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    char zip_fullpath[PATH_MAX];
    snprintf(zip_fullpath, sizeof(zip_fullpath), "%s/%s", fl->archiveDir, zip_name);

    if (create_zip(fl->archiveDir, zip_name) != 0) {
        fprintf(stderr, "ERROR: Failed to create archive %s\n", zip_fullpath);
        return 0;
    }

    return 1;
}

static int rotateFiles(void)
{
    if (!g_file || !g_file->output) return 0;
    if (g_file->currentFileSize < g_file->maxFileSize) return 1;

    fclose(g_file->output);
    g_file->output = NULL;

    if (g_file->archiveOldLogs && g_file->archiveDir[0]) {
        int cnt = count_regular_files(g_file->archiveDir);
        if (cnt >= g_file->maxBackUpFiles) {
            create_archive_and_cleanup(g_file);
        }
    }

    rotate_backup_files(g_file->fileName, g_file->maxBackUpFiles,
                        g_file->archiveOldLogs ? g_file->archiveDir : NULL);

    g_file->output = fopen(g_file->fileName, "w");
    if (!g_file->output) {
        fprintf(stderr, "CRITICAL: Cannot reopen log file %s\n", g_file->fileName);
        return 0;
    }
    g_file->currentFileSize = 0;
    return 1;
}

static void log_message(log_level_t level, const char* message)
{
    if (level < current_log_level) return;
    ensure_init();
    lock();

    char timestamp[TIMESTAMP_BUF_SIZE];
    format_timestamp(timestamp, sizeof(timestamp));
    long tid = getCurrentThreadID();

    if (hasFlag(logger_type, CONSOLELOGGER) && g_console) {
        fprintf(g_console->output, "%s%s [%s] [T%ld] %s%s\n",
                level_to_color(level), timestamp, level_to_string(level), tid, message, COLOR_RESET);
        fflush(g_console->output);
    }

    if (hasFlag(logger_type, FILELOGGER) && g_file && g_file->output) {
        long written = fprintf(g_file->output, "%s [%s] [T%ld] %s\n",
                               timestamp, level_to_string(level), tid, message);
        if (written > 0) {
            g_file->currentFileSize += written;
            if (g_file->currentFileSize >= g_file->maxFileSize) {
                unlock();
                rotateFiles();
                lock();
            }
        }
        fflush(g_file->output);
    }
    unlock();
}

void log_messagef(log_level_t level, const char* file, int line, const char* fmt, ...)
{
    if (level < current_log_level) return;
    char buffer[LOG_BUFFER_SIZE - 64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    char fullmsg[LOG_BUFFER_SIZE];
    snprintf(fullmsg, sizeof(fullmsg), "%s:%d: %s", file, line, buffer);
    log_message(level, fullmsg);
}

void close_logging(void)
{
    lock();
    if (g_console) { free(g_console); g_console = NULL; }
    if (g_file) {
        if (g_file->output) fclose(g_file->output);
        free(g_file);
        g_file = NULL;
    }
    logger_type = 0;
    logger_initialized = 0;
    unlock();
}
