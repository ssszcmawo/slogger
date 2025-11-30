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
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>


static log_level_t current_log_level = INFO;
static pthread_mutex_t l_mutex;
static volatile int logger_initialized = 0;
static volatile int logger_type;

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

static int hasFlag(int flags, int flag)
{
    return (flags & flag) == flag;
}

static void *get_in_addr(struct sockaddr* sa){
  if(sa->sa_family == AF_INET){
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

FileLog* init_fileLog(const char* filename, long maxFileSize,int archiveOrNot) {
    ensure_init();

    lock();

    FileLog* fl = calloc(1, sizeof(FileLog));

  if(!fl){
    unlock();
    return NULL;
  }

    strncpy(fl->fileName, filename, MAX_FILE_NAME - 1);
    fl->fileName[MAX_FILE_NAME - 1] = '\0';

    fl->maxFileSize = (maxFileSize > 0 ? maxFileSize : DEFAULT_MAX_FILE_SIZE);
    fl->output = fopen(filename, "a");
    if(fl->output == NULL){
      fprintf(stderr,"ERROR: Failed to open file: %s\n",filename);
      free(fl);
      unlock();
      return NULL;
  }

    fseek(fl->output, 0, SEEK_END);
    fl->currentFileSize = ftell(fl->output);

    logger_type |= FILELOGGER;

    fl->maxBackUpFiles = DEFAULT_MAX_BACKUP_FILES;
    fl->archiveOldLogs = archiveOrNot;
    
    if(archiveOrNot == 1){

      const char dir[24] = "archiveLogsDir";

      if(mkdir(dir,0755) != 0){

        fprintf(stderr,"ERROR: Failed to create directory: %s\n",dir);

      }

      strncpy(fl->archiveDir,dir,MAX_FILE_NAME);

  }
 
    g_file = fl;

    unlock();

    return fl;
}

static int is_FileExists(const char* filename){

  FILE* fp;

  if((fp = fopen(filename,"r")) == NULL){

    return 0;

  }else{
    fclose(fp);
    return 1;

  }

}
static void getBackUpFileName(const char* baseName,int index,char* out,size_t outSize){
  if(index <= 0){
    snprintf(out,outSize,"%s",baseName);
  }else {
    snprintf(out,outSize,"%s.%d",baseName,index);
  }
}

static void archiveFile(const char* src, const char* archiveDir) {
    char dst[512];
    snprintf(dst, sizeof(dst), "%s/%s", archiveDir, src);
    if (rename(src, dst) != 0) {
        perror("Failed to archive file");
    }
}

static void moveToArchieve(){
  
  int i;

  char dst[512],src[512]; 
 
    for(i = (int)g_file->maxBackUpFiles;i > 0;i--){
      getBackUpFileName(g_file->fileName,i - 1,src,sizeof(src));
      getBackUpFileName(g_file->fileName,i,dst,sizeof(dst));

      if(i == g_file->maxBackUpFiles - 1 && is_FileExists(dst)){
        archiveFile(dst,g_file->archiveDir);
      }else{
        if(is_FileExists(dst)) remove(dst);
      }

      if(is_FileExists(src)) rename(src,dst);

    }
}

static void deleteOldLogs() {

    int i;

    char src[512], dst[512];

    for (i = (int)g_file->maxBackUpFiles - 1; i > 0; i--) {
        getBackUpFileName(g_file->fileName, i - 1, src, sizeof(src));
        getBackUpFileName(g_file->fileName, i, dst, sizeof(dst));

        if (is_FileExists(dst)) remove(dst);
        if (is_FileExists(src)) rename(src, dst);
    }
}

static int rotateFiles(){

  if(g_file->currentFileSize < g_file->maxFileSize){
    return g_file->output != NULL;
  }

  fclose(g_file->output);

  if(g_file->archiveOldLogs == 0){
    deleteOldLogs();
  }else{
    moveToArchieve();
  }


  g_file->output = fopen(g_file->fileName,"a");
  if(g_file->output == NULL){
    fprintf(stderr,"ERROR: Failed to open file: %s\n",g_file->fileName);
    return 0;
  }

    fseek(g_file->output, 0, SEEK_END);
    g_file->currentFileSize = ftell(g_file->output);

    return 1;

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

  if((rv = getaddrinfo(host,port,&hints,&servinfo)) != 0){
    fprintf(stderr,"getaddinfo: %s\n",gai_strerror(rv));

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
  nl->len = 0;

  g_network = nl;

  logger_type |= NETWORKLOGGER;

  unlock();

  return nl;
}

int network_log(NetworkLog* nl,char* message){
  if(!nl || !message) return -1;

  size_t len = strlen(message);
  ssize_t sent;

retry_send:
  sent = send(nl->sockfd,message,len,0);
  if(sent == -1){
    perror("send");

    if(errno == ECONNRESET || errno == EPIPE){
      close(nl->sockfd);

      struct addrinfo* p;
      int sockfd = -1;

      for(p = nl->addr; p != NULL; p = p->ai_next){

        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);

        if(sockfd == -1) continue;

        if(connect(sockfd,p->ai_addr,p->ai_addrlen) == -1){
          close(sockfd);
          sockfd = -1;
          continue;
        }

        break;

      }

      if(sockfd == -1){
        fprintf(stderr, "network_log: failed to reconnect\n");
        return -1;
      }

      nl->sockfd = sockfd;
      goto retry_send;
    }
    return -1;
  }

  if((size_t)sent < len){
    message += sent;
    len -= sent;
    goto retry_send;
  }

  return 0;
}

static void flush_buffer(NetworkLog* nl){
  if(nl->len > 0){
    send(nl->sockfd,nl->buf,nl->len,0);
    nl->len = 0;
  }
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

void free_archive_logs(FileLog* fl){

  if(rmdir(fl->archiveDir) != 0){

      fprintf(stderr,"ERROR: Could not delete directory: %s\n",fl->archiveDir);

      return;

  }

}

static void compress_old_logs(const char* dir){

  char cmd[512];

  snprintf(cmd,sizeof(cmd),"zip -r old_logs.zip %s > /dev/null 2>&1",dir);

  system(cmd);

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

    char timestamp[32];
    format_timestamp(timestamp, sizeof(timestamp));
    long tid = getCurrentThreadID();

    if (hasFlag(logger_type,CONSOLELOGGER)) {
        const char* color = level_to_color(level);
        fprintf(g_console->output, "%s%s [%s] [T%ld] %s%s\n",color, timestamp, level_to_string(level), tid, message,COLOR_RESET);
        fflush(g_console->output);
    }

    if (hasFlag(logger_type,FILELOGGER)) {
        if (rotateFiles()) {
            long size = fprintf(g_file->output, "%s [%s] [T%ld] %s\n", timestamp, level_to_string(level), tid, message);
            g_file->currentFileSize += size;
            fflush(g_file->output);
        } 
    }
    if(hasFlag(logger_type,NETWORKLOGGER)){
    g_network->len += snprintf(g_network->buf + g_network->len,sizeof(g_network->buf) - g_network->len,
                               "%s [%s] [T%ld] %s\n",timestamp,level_to_string(level),tid,message);
    network_log(g_network,g_network->buf);
    flush_buffer(g_network); 
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
    if (hasFlag(logger_type,FILELOGGER)){
    compress_old_logs(g_file->archiveDir);
    free_file_log(g_file);
  } 
    if(g_network) free_network_log(g_network);
    unlock();
}


