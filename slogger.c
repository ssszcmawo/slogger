#define _GNU_SOURCE
#include "slogger.h"
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
#include <arpa/inet.h>



static log_level_t current_log_level = INFO;
static pthread_mutex_t l_mutex;
static volatile int logger_initialized = 0;
static volatile int logger_type = 0;

static ConsoleLog* g_console = NULL;
static FileLog* g_file = NULL;
static NetworkLog* g_network = NULL;

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

static void *get_in_addr(struct sockaddr* sa){
  if(sa->sa_family = AF_INET){
    return&(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

NetworkLog* init_networkLog(const char* host,const char* port,log_level_t level){
  ensure_init();

  lock();

  NetworkLog* nl = calloc(1,sizeof(NetworkLog));

  int sockfd;
  struct addrinfo hints,*servinfo;
  int rv;

  memset(&hints,0,sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if((rv == getaddrinfo(host,port,&hints,&servinfo)) != 0){
    fprintf(stderr,"getaddinfo: %s",gai_strerror(rv));

    free(nl);

    unlock();

    return NULL;
  }


  struct addrinfo* p;

  for(p = servinfo; p != NULL; p = p->ai_next){
    if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
      perror("socket");
      continue;
    }

    if(connect(sockfd,p->ai_addr,p->ai_addrlen) == -1){
      perror("connect");
      close(sockfd);
      sockfd = -1;
      continue;
    }

    break;
  }

  if(sockfd == -1){
    fprintf(stderr, "Failed to connect to %s:%s\n", host, port);

    free(nl);
    freeaddrinfo(servinfo);

    unlock();

    return NULL;
  }

  nl->sockfd = sockfd;
  nl->addr = servinfo;
  nl->level = level;

  g_network = nl;

  logger_type |= NETWORKLOGGER;

  unlock();

  return nl;
}



void free_network_log(NetworkLog* nl){
  if(!nl) return;

  if(nl->addr) freeaddrinfo(nl->addr);

  free(nl);

  g_network = NULL;
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
  if(logger_type & CONSOLELOGGER && g_console){

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


