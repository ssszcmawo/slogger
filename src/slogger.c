#define _GNU_SOURCE
#include "slogger.h"
#include "zip.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <ftw.h>

#define TIMESTAMP_BUF_SIZE 32
#define LOG_BUFFER_SIZE 8192

static pthread_mutex_t l_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t logger_once = PTHREAD_ONCE_INIT;

static log_level_t current_log_level = INFO;
static int logger_type;
static int logger_initialized;

static ConsoleLog* g_console = NULL;
static FileLog*    g_file = NULL;



static void lock(void) {
  pthread_mutex_lock(&l_mutex);
}

static void unlock(void) {
  pthread_mutex_unlock(&l_mutex);
}

static long getCurrentThreadID(void) {
  return (long)syscall(SYS_gettid);
}

static void logger_init(void) {
    logger_initialized = 1;
}

static void ensure_init(void) {
    pthread_once(&logger_once, logger_init);
}

static int hasFlag(int flags, int flag) {

    return (flags & flag) != 0;

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

  if(g_console){
    unlock();
    return g_console;
  }

  ConsoleLog* cl = calloc(1, sizeof(ConsoleLog));

  if(!cl){
    unlock();
    return NULL;
  }

  cl->output = (output ? output : stdout);

  g_console = cl;
  logger_type |= CONSOLELOGGER;

  unlock();

  return cl;
}


FileLog* init_fileLog(const char* filename, long maxFileSize, bool archiveOrNot) {
  ensure_init();
  lock();
  if (g_file) { unlock(); return g_file; }

  FileLog* fl = calloc(1, sizeof(FileLog));
  if (!fl) { unlock(); return NULL; }

  strncpy(fl->fileName, filename, sizeof(fl->fileName) - 1);
  fl->fileName[sizeof(fl->fileName) - 1] = '\0';
  fl->maxFileSize = maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE;
  fl->maxBackUpFiles = DEFAULT_MAX_BACKUP_FILES;
  fl->archiveOldLogs = archiveOrNot;

  if (archiveOrNot) {
    snprintf(fl->archiveDir, sizeof(fl->archiveDir), "%s_archive", filename);
    fl->archiveDir[sizeof(fl->archiveDir) - 1] = '\0';
    if (mkdir(fl->archiveDir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "ERROR: Cannot create archive directory %s\n", fl->archiveDir);
    }
  }

  fl->output = fopen(filename, "a");
  if (!fl->output) { free(fl); unlock(); return NULL; }

  fseek(fl->output, 0, SEEK_END);
  fl->currentFileSize = ftell(fl->output);
  g_file = fl;
  logger_type |= FILELOGGER;
  unlock();
  return fl;
}

static int file_exists(const char* path) {
  struct stat st;
  return (stat(path, &st) == 0);
}

static void backup_filename(const char* baseName,int index,char* out,size_t outSize){

  if(index <= 0){

    snprintf(out,outSize,"%s",baseName);

  }else {

    snprintf(out,outSize,"%s.%d",baseName,index);

  }

}

static void rotate_backup_files(const char* baseName, int maxBackups, const char* archiveDir)
{
  char oldPath[PATH_MAX];
  char newPath[PATH_MAX];

  if (maxBackups > 0) {
    backup_filename(baseName, maxBackups, oldPath, sizeof(oldPath));
    if (file_exists(oldPath)) {
      if (archiveDir && archiveDir[0]) {
        char archived[PATH_MAX];
        snprintf(archived, sizeof(archived), "%s/%s.%d", archiveDir, baseName, maxBackups);
        if(rename(oldPath, archived) != 0){
          fprintf(stderr,"ERROR: Cannot move file %s -> %s\n",oldPath,archived);
        }
      } else {
        if(unlink(oldPath) != 0){
          fprintf(stderr,"ERROR: Cannot remove file %s\n",oldPath);
        }
      }
    }
  }

  for (int i = maxBackups; i >= 1; --i) {
    backup_filename(baseName, i - 1, oldPath, sizeof(oldPath));
    backup_filename(baseName, i,     newPath, sizeof(newPath));
    if (file_exists(oldPath)) {
      if(rename(oldPath, newPath) != 0){
        fprintf(stderr,"ERROR: Cannot rotate files %s -> %s\n",oldPath,newPath);
        
      }
    }
  }
}

int rotateFiles(void)
{
  if (!g_file || g_file->currentFileSize < g_file->maxFileSize)
    return 1;

  fclose(g_file->output);
  g_file->output = NULL;

  const char* archive_dir = (g_file->archiveOldLogs && g_file->archiveDir[0])
    ? g_file->archiveDir : NULL;

  rotate_backup_files(g_file->fileName, g_file->maxBackUpFiles, archive_dir);

  g_file->output = fopen(g_file->fileName, "w");
  if (!g_file->output) {
    fprintf(stderr, "ERROR: Cannot reopen log file %s\n", g_file->fileName);
    return 0;
  }

  g_file->currentFileSize = 0;
  return 1;
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

void free_archive_logs(FileLog* fl){

  if(rmdir(fl->archiveDir) != 0){

    fprintf(stderr,"ERROR: Could not delete directory: %s\n",fl->archiveDir);

    return;

  }

}
static const char* level_to_color(log_level_t lvl) {
  switch (lvl) {
    case DEBUG:   return COLOR_BLUE;
    case WARNING: return COLOR_YELLOW;
    case ERROR:   return COLOR_RED;
    default:      return COLOR_GREEN;
  }
}

void log_message(log_level_t level, const char* message) {
    if (level < current_log_level) return;

    ensure_init();
    lock();

    char timestamp[TIMESTAMP_BUF_SIZE];
    format_timestamp(timestamp, sizeof(timestamp));
    long tid = getCurrentThreadID();

    if (hasFlag(logger_type, CONSOLELOGGER)) {
        const char* color = level_to_color(level);
        fprintf(g_console->output, "%s%s [%s] [T%ld] %s%s\n",
                color, timestamp, level_to_string(level), tid, message, COLOR_RESET);
        fflush(g_console->output);
    }

    if (hasFlag(logger_type, FILELOGGER) && g_file && g_file->output) {
    long size = fprintf(g_file->output, "%s [%s] [T%ld] %s\n",
                        timestamp, level_to_string(level), tid, message);
    if (size > 0) {
        g_file->currentFileSize += size;

        if (g_file->currentFileSize >= g_file->maxFileSize) {
            unlock();                    
            if (!rotateFiles()) {
                fprintf(stderr, "CRITICAL: Log rotation failed permanently. Disabling file logging.\n");
                fclose(g_file->output);
                g_file->output = NULL;
            }
            lock();                   
        }
    }
    fflush(g_file->output);
}

    unlock();
}

void log_messagef(log_level_t level, const char* fmt, ...) {
    if (level < current_log_level) return;
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    log_message(level, buffer);
}

void clear_log_file(const char* filename) {
  FILE* file = fopen(filename, "w");
  if (file) fclose(file);
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    return remove(fpath);
}

static int remove_directory_recursively(const char *path) {
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void close_logging(void)
{
    lock();

    if (g_console) {
        free_console_log(g_console);
        g_console = NULL;
    }

    if (g_file) {
        if (g_file->archiveOldLogs && g_file->archiveDir[0]) {
            char zip_path[PATH_MAX];
            time_t t = time(NULL);
            struct tm tm;
            localtime_r(&t, &tm);

            const char *slash = strrchr(g_file->fileName, '/');
            const char *name = slash ? slash + 1 : g_file->fileName;
            char basename[128];
            const char *dot = strrchr(name, '.');
            if (dot && dot != name) {
                size_t len = dot - name;
                len = len < sizeof(basename)-1 ? len : sizeof(basename)-1;
                memcpy(basename, name, len);
                basename[len] = '\0';
            } else {
                strncpy(basename, name, sizeof(basename)-1);
                basename[sizeof(basename)-1] = '\0';
            }

            snprintf(zip_path, sizeof(zip_path),
                     "%s_archive_%04d%02d%02d_%02d%02d%02d.zip",
                     basename,
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);

            if (create_zip(g_file->archiveDir, zip_path) == 0) {
                remove_directory_recursively(g_file->archiveDir);
            } else {
                fprintf(stderr, "WARNING: Failed to create ZIP archive: %s\n"
                                "         Archive directory kept: %s\n",
                                zip_path, g_file->archiveDir);
            }

            g_file->archiveDir[0] = '\0';
        }

        if (g_file->output) {
            fclose(g_file->output);
            g_file->output = NULL;
        }
        free(g_file);
        g_file = NULL;
    } 

    logger_type = 0;
    logger_initialized = 0;
    unlock();
}
