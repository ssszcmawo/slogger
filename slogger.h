#ifndef SLOGGER_H
#define SLOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/syscall.h>

enum{

  CONSOLELOGGER = 1 << 0,

  FILELOGGER = 1 << 1,

  maxFileNameLength = 64;

  DefaultMaxFileSize = 1048576L

};

typedef enum LogLevel {
  INFO,
  DEBUG,
  WARNING,
  ERROR
} log_level_t;

typedef struct FileLog{
  FILE* output;
  char fileName[MAX_FILE_NAME];
  long maxFileSize;
  long currentFileSize;
} file_log_t;

typedef struct ConsoleLog{
  FILE* output;
} cons_log_t;


ConsoleLog* init_consoleLog(FILE* output);
void free_console_log(ConsoleLog* cl);
void log_message(log_level_t level,const char* text);
void clear_log_file(const char* filename);
void set_log_level(log_level_t level);
void get_log_level();
void init_logging(const char* filename);
void close_logging();

#endif // !SLOGGER_H
