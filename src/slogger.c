#define _GNU_SOURCE
#include "slogger.h"
#include "ring_buffer.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#define CONSOLELOGGER 1
#define FILELOGGER 2

#define DEFAULT_MAX_FILE_SIZE (1024 * 1024)
#define DEFAULT_MAX_BACKUP_FILES 10

#define LOG_FILENAME_MAX 4096
#define LOG_BUFFER_SIZE 8192
#define TIMESTAMP_BUF_SIZE 32

#define RING_SIZE 1024
#define LOG_TEXT_SIZE 512
#define TS_SIZE 32

typedef struct {
  FILE *output;
  log_level_t level;
} ConsoleLog;

typedef struct {
  FILE *output;
  char fileName[LOG_FILENAME_MAX];
  long maxFileSize;
  long currentFileSize;
  int maxBackups;
  log_level_t level;
} FileLog;

typedef struct {
  log_level_t level;
  long tid;
  char timestamp[TIMESTAMP_BUF_SIZE];
  char text[LOG_TEXT_SIZE];
} log_entry_t;

static ConsoleLog *g_console = NULL;
static FileLog *g_file = NULL;

static ring_buffer_t g_ring;
static log_entry_t g_storage[RING_SIZE];

static pthread_t g_thread;
static atomic_int g_run = 0;

static log_level_t g_log_level = INFO;
static int g_logger_type = 0;

static atomic_int dropped_log = 0;

static inline long thread_id(void) { return (long)syscall(SYS_gettid); }

void set_log_level(log_level_t level) { g_log_level = level; }

static const char *level_to_string(log_level_t lvl) {
  switch (lvl) {
  case DEBUG:
    return "DEBUG";
  case WARNING:
    return "WARN";
  case ERROR:
    return "ERROR";
  case TRACE:
    return "TRACE";
  default:
    return "INFO";
  }
}

static const char *level_to_color(log_level_t lvl) {
  switch (lvl) {
  case DEBUG:
    return COLOR_BLUE;
  case WARNING:
    return COLOR_YELLOW;
  case ERROR:
    return COLOR_RED;
  case TRACE:
    return COLOR_ORANGE;
  default:
    return COLOR_GREEN;
  }
}

static void format_timestamp(char *buf, size_t size) {
  struct timeval tv;
  struct tm tm;
  if (gettimeofday(&tv, NULL) != 0) {
    perror("gettimeofday failed");
    snprintf(buf, size, "1970-01-01 00:00:00.000000");
    return;
  }
  localtime_r(&tv.tv_sec, &tm);
  strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm);
  size_t len = strlen(buf);
  snprintf(buf + len, size - len, ".%06ld", (long)tv.tv_usec);
}

int init_consoleLog(FILE *output) {
  if (g_console)
    return 1;

  g_console = calloc(1, sizeof(ConsoleLog));
  if (!g_console) {
    perror("Failed to allocate ConsoleLog");
    return 0;
  }

  g_console->output = output ? output : stdout;
  g_console->level = g_log_level;

  g_logger_type |= CONSOLELOGGER;
  return 1;
}

static int get_file_size(const char *path, long *size) {
  struct stat st;
  if (stat(path, &st) != 0) {
    if (errno != ENOENT)
      perror("stat failed");
    *size = 0;
    return 0;
  }
  *size = st.st_size;
  return 1;
}

static int rotate_backups(const char *base, int max) {
  char old_path[PATH_MAX];
  char new_path[PATH_MAX];

  for (int i = max; i >= 2; --i) {
    snprintf(old_path, sizeof(old_path), "%s.%d", base, i - 1);
    snprintf(new_path, sizeof(new_path), "%s.%d", base, i);
    if (rename(old_path, new_path) != 0 && errno != ENOENT) {
      fprintf(stderr, "Failed to rename %s to %s: %s\n", old_path, new_path,
              strerror(errno));
      return 0;
    }
  }

  snprintf(new_path, sizeof(new_path), "%s.1", base);
  if (rename(base, new_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "Failed to rename %s to %s: %s\n", base, new_path,
            strerror(errno));
    return 0;
  }
  return 1;
}

static int rotate_files(void) {
  if (!g_file || !g_file->output)
    return 0;

  if (g_file->currentFileSize < g_file->maxFileSize)
    return 1;

  if (fclose(g_file->output) != 0) {
    perror("Failed to close log file before rotation");
  }
  g_file->output = NULL;

  if (!rotate_backups(g_file->fileName, g_file->maxBackups)) {
    fprintf(stderr, "Failed to rotate backup files\n");
  }

  g_file->output = fopen(g_file->fileName, "w");
  if (!g_file->output) {
    perror("Failed to open new log file after rotation");
    return 0;
  }
  g_file->currentFileSize = 0;
  return 1;
}

int init_fileLog(const char *filename, long maxFileSize) {
  if (g_file)
    return 1;

  FileLog *fl = calloc(1, sizeof(FileLog));
  if (!fl) {
    perror("Failed to allocate FileLog");
    return 0;
  }

  fl->maxFileSize = maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE;
  fl->maxBackups = DEFAULT_MAX_BACKUP_FILES;
  fl->level = g_log_level;

  strncpy(fl->fileName, filename, sizeof(fl->fileName) - 1);

  fl->output = fopen(fl->fileName, "a");
  if (!fl->output) {
    perror("Failed to open log file");
    free(fl);
    return 0;
  }

  get_file_size(fl->fileName, &fl->currentFileSize);

  g_file = fl;
  g_logger_type |= FILELOGGER;
  return 1;
}

static void log_message(log_level_t level, const char *msg) {
  if (level < g_log_level)
    return;

  char ts[TIMESTAMP_BUF_SIZE];
  format_timestamp(ts, sizeof(ts));
  long tid = thread_id();

  if ((g_logger_type & CONSOLELOGGER) && g_console) {
    if (fprintf(g_console->output, "%s%s [%s] [T%ld] %s%s\n",
                level_to_color(level), ts, level_to_string(level), tid, msg,
                COLOR_RESET) < 0) {
      perror("Failed to write to console");
    }
    if (fflush(g_console->output) != 0)
      perror("Failed to flush console");
  }

  if ((g_logger_type & FILELOGGER) && g_file && g_file->output) {
    long written = fprintf(g_file->output, "%s [%s] [T%ld] %s\n", ts,
                           level_to_string(level), tid, msg);
    if (written < 0) {
      perror("Failed to write to log file");
    } else {
      g_file->currentFileSize += written;
      if (g_file->currentFileSize >= g_file->maxFileSize)
        if (!rotate_files())
          fprintf(stderr, "Failed to rotate log files\n");
    }
    if (fflush(g_file->output) != 0)
      perror("Failed to flush log file");
  }
}

void log_messagef(log_level_t level, const char *file, int line,
                  const char *fmt, ...) {
  if (level < g_log_level)
    return;

  log_entry_t entry;
  entry.level = level;
  entry.tid = thread_id();
  format_timestamp(entry.timestamp, sizeof(entry.timestamp));

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(entry.text, sizeof(entry.text), fmt, ap);
  va_end(ap);

  char final[LOG_TEXT_SIZE];
  snprintf(final, sizeof(final), "%s:%d: %.*s", file, line,
           (int)(sizeof(final) - 50), entry.text);
  strncpy(entry.text, final, sizeof(entry.text) - 1);
  entry.text[LOG_TEXT_SIZE - 1] = '\0';

  if (!ring_push(&g_ring, &entry, sizeof(log_entry_t))) {
    dropped_log++;
    fprintf(stderr, "Logger ring buffer full, dropped %d logs\n", dropped_log);
  }
}

static void *logger_thread(void * /*arg*/) {
  log_entry_t entry;
  while (atomic_load(&g_run)) {
    while (ring_pop(&g_ring, &entry, sizeof(entry))) {
      log_message(entry.level, entry.text);
    }
    usleep(1000);
  }

  while (ring_pop(&g_ring, &entry, sizeof(entry))) {
    log_message(entry.level, entry.text);
  }
  return NULL;
}

void start_logging_thread(void) {
  ring_init(&g_ring, g_storage, RING_SIZE);
  atomic_store(&g_run, 1);
  if (pthread_create(&g_thread, NULL, logger_thread, NULL) != 0) {
    perror("Failed to create logger thread");
    atomic_store(&g_run, 0);
  }
}

void close_logging(void) {
  atomic_store(&g_run, 0);
  if (pthread_join(g_thread, NULL) != 0)
    perror("Failed to join logger thread");

  if (g_console) {
    if (g_console->output && g_console->output != stdout &&
        fclose(g_console->output) != 0)
      perror("Failed to close console output");
    free(g_console);
    g_console = NULL;
  }

  if (g_file) {
    if (g_file->output && fclose(g_file->output) != 0)
      perror("Failed to close log file");
    free(g_file);
    g_file = NULL;
  }

  g_logger_type = 0;
}
