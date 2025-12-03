#ifndef SLOGGER_H
#define SLOGGER_H

#include <stdio.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <limits.h>


#define CONSOLELOGGER 1
#define FILELOGGER 2

#define MAX_FILE_NAME 64
#define DEFAULT_MAX_FILE_SIZE 1048576L
#define MAX_BUF_SIZE 4096
#define DEFAULT_MAX_BACKUP_FILES 10
#define LOG_FILENAME_MAX 4096
#define MAX_LINE_LEN 256

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

typedef enum LogLevel {
    INFO,
    DEBUG,
    WARNING,
    ERROR
} log_level_t;

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

void set_log_level(log_level_t level);
log_level_t get_log_level(void);

ConsoleLog* init_consoleLog(FILE* output);
void free_console_log(ConsoleLog* cl);

FileLog* init_fileLog(const char* filename, long maxFileSize,bool archiveOrNot);
int rotateFiles();
void free_file_log(FileLog* fl);


void log_message(log_level_t level, const char* message);
void clear_log_file(const char* filename);
void close_logging(void);

#endif
