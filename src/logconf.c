#include "slogger.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static struct {
    FILE* output;
    char fileName[MAX_FILE_NAME];
    long maxFileSize;
    long currentFileSize;
    log_level_t level;
} file_log_conf;

static struct {
    FILE* output;
    log_level_t level;
}console_log_conf;

static struct {
  int sockfd;
  struct addrinfo* addr;
  char buf[MAX_BUF_SIZE];
  size_t len;
  log_level_t level;
}network_log_conf;


static volatile int logger_initialized = 0;

static char* port = NULL;
static char* host = NULL;

void parseLine(char* line);
static log_level_t parseLogLevel(const char* s);
static int hasFlag(int flags,int flag);
static void set_log_level_conf(log_level_t* level,log_level_t new_level);


int configure_logger(const char* filename){

  FILE* fp;
  char line[maxLineLen];

  if(filename == NULL){
    assert(0 && "filename must not be NULL");
    return 0;
  }


  if((fp = fopen(filename,"r")) == NULL){
    fprintf(stderr,"ERROR: Failed to open file : %s (logconf)\n",filename);
    return 0;
  }

    memset(&file_log_conf, 0, sizeof(file_log_conf));
    memset(&console_log_conf, 0, sizeof(console_log_conf));
    memset(&network_log_conf, 0, sizeof(network_log_conf));


  while(fgets(line,sizeof(line),fp) != NULL){


    if(line[0] == '\0'){
      continue;
    }

    parseLine(line);
  }
  fclose(fp);

  if(hasFlag(logger_initialized,CONSOLELOGGER)){
    if(!init_consoleLog(console_log_conf.output)){
      return 0;
    }

  }

  if(hasFlag(logger_initialized,FILELOGGER)){
    if(init_fileLog(file_log_conf.fileName,file_log_conf.maxFileSize)){
      return 0;
    }
  }

  if(hasFlag(logger_initialized,NETWORKLOGGER)){
    if(init_networkLog(host,port,network_log_conf.level)){
      return 0;
    }
  }


  if(logger_initialized == 0){
    return 0;
  }

  return 1;


}

void parseLine(char* line){
  if(!line) return;
  char *key, *val;

  key = strtok(line,"=");
  val = strtok(NULL,"=");

  if(!key || !val) return;

  size_t len = strlen(val);
  if(len > 0 && val[len - 1] == '\n') val[len - 1] = '\0';


  if(strcmp(key,"logger") == 0){
    if(strcmp(val,"console") == 0){
      logger_initialized |= CONSOLELOGGER;
    }else if(strcmp(val,"file") == 0){
      logger_initialized |= FILELOGGER;
    }else if(strcmp(val,"network") == 0){
      logger_initialized |= NETWORKLOGGER;
    }else{
      fprintf(stderr,"ERROR:Invalid type of logger: %s\n",val);
    }
  }else if(strcmp(key,"logger.console.output") == 0){
    if(strcmp(val,"stdout") == 0){
      console_log_conf.output = stdout;
    }else if(strcmp(val,"stderr") == 0){
      console_log_conf.output = stderr;
    }else{
      fprintf(stderr,"ERROR: Invalid console logger output: %s\n",val);
      console_log_conf.output = NULL;
    }
  }else if(strcmp(key,"logger.file.output") == 0){
    strncpy(file_log_conf.fileName,val,sizeof(file_log_conf.fileName));
  }else if(strcmp(key,"logger.file.maxFileSize") == 0){
    file_log_conf.maxFileSize = atol(val);
  }else if(strcmp(key,"logger.file.level") == 0){
    set_log_level_conf(&(file_log_conf.level),parseLogLevel(val));
  }else if(strcmp(key,"logger.console.level") == 0){
    set_log_level_conf(&(console_log_conf.level),parseLogLevel(val));
  }else if(strcmp(key,"PORT") == 0){
    if(port) free(port);
    port = strdup(val);
  }else if(strcmp(key,"HOST") == 0){
    if(host) free(host);
    host = strdup(val);
  }
}

static log_level_t parseLogLevel(const char* s){
  if(strcmp(s,"INFO") == 0){
    return INFO;
  }else if(strcmp(s,"DEBUG") == 0){
    return DEBUG;
  }else if(strcmp(s,"ERROR") == 0){
    return ERROR;
  }else if(strcmp(s,"WARNING") == 0){
    return WARNING;
  }else{
    fprintf(stderr,"ERROR: Invalid level: %s\n",s);
    return get_log_level();
  }
}

static int hasFlag(int flags,int flag){
  return (flags & flag) == flag;
}

static void set_log_level_conf(log_level_t* level,log_level_t new_level){
  *level = new_level;
}
