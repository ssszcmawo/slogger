#include "slogger.h"
#include <string.h>
#include <stdlib.h>

static volatile int logger_initialized = 0;
static volatile int logger_type = 0;


static ConsoleLog* g_console = NULL;
static FileLog* g_file = NULL;
static NetworkLog* g_network = NULL;


static log_level_t parseLogLevel(const char* s);
static void set_log_level_conf(log_level_t* level,log_level_t new_level){
  *level = new_level;
}


void parseLine(const char* line){
  char *key, *val;

  key = strtok(line,"=");
  val = strtok(NULL,"=");


  if(strcmp(key,"logger") == 0){
    if(strcmp(val,"console") == 0){
      logger_type |= CONSOLELOGGER;
    }else if(strcmp(val,"file") == 0){
      logger_type |= FILELOGGER;
    }else if(strcmp(val,"network") == 0){
      logger_type |= NETWORKLOGGER;
    }else{
      fprintf(stderr,"ERROR:Invalid type of logger: %s\n",val);
      logger_type = 0;
    }
  }else if(strcmp(key,"logger.console.output") == 0){
    if(strcmp(val,"stdout") == 0){
      g_console->output = stdout;
    }else if(strcmp(val,"stderr") == 0){
      g_console->output = stderr;
    }else{
      fprintf(stderr,"ERROR: Invalid console logger output: %s\n",val);
      g_console->output = NULL;
    }
  }else if(strcmp(key,"logger.file.output") == 0){
    strncpy(g_file->fileName,val,sizeof(g_file->fileName));
  }else if(strcmp(key,"logger.file.maxFileSize") == 0){
    g_file->maxFileSize = atol(val);
  }else if(strcmp(key,"logger.file.level") == 0){
    set_log_level_conf(&(g_file->level),parseLogLevel(val));
  }else if(strcmp(key,"logger.console.level") == 0){
    set_log_level_conf(&(g_console->level),parseLogLevel(val));
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


