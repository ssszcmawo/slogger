#ifndef SLOGGER_H
#define SLOGGER_H

#include <stdio.h>
#include <pthread.h>
#include <netinet/in.h>

#define MAX_FILE_NAME 64
#define DEFAULT_MAX_FILE_SIZE 1048576L
#define MAX_BUF_SIZE 4096

enum {
    CONSOLELOGGER = 1 << 0,

    FILELOGGER    = 1 << 1,

    NETWORKLOGGER = 1 << 2
};

typedef enum LogLevel {
    INFO,
    DEBUG,
    WARNING,
    ERROR
} log_level_t;

typedef struct FileLog {
    FILE* output;
    char fileName[MAX_FILE_NAME];
    long maxFileSize;
    long currentFileSize;
    log_level_t level;
} FileLog;

typedef struct ConsoleLog {
    FILE* output;
    log_level_t level;
} ConsoleLog;

typedef struct NetworkLog{
  int sockfd;
  struct addrinfo* addr;
  char buf[MAX_BUF_SIZE];
  size_t len;
  log_level_t level;
}NetworkLog;

void set_log_level(log_level_t level);
log_level_t get_log_level(void);

ConsoleLog* init_consoleLog(FILE* output);
void free_console_log(ConsoleLog* cl);

FileLog* init_fileLog(const char* filename, long maxFileSize);
void free_file_log(FileLog* fl);

NetworkLog* init_networkLog(const char* host,const char* port,log_level_t level);
int network_log(NetworkLog* nl,char* message);

void log_message(log_level_t level, const char* message);
void clear_log_file(const char* filename);
void close_logging(void);

#endif
