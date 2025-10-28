#include "slogger.h" 

static log_level_t current_log_level = INFO;
static pthread_mutex_t l_mutex;
static volatile int logger_enabled = 0;
static volatile int logger_type;


static void lock(void){
  pthread_mutex_lock(&l_mutex);
}

static void unlock(void){
  pthread_mutex_unlock(&l_mutex);
}

static long getCurrentThreadID(){
  return syscall(SYS_thread_selfid);
}

static void init(void){

  if(logger_enabled){
    return;
  }

  pthread_mutex_init(&l_mutex,NULL);

  logger_enabled = 1;
}

void set_log_level(log_level_t level){
  current_log_level = level;
}

static void get_log_level(){
  return current_log_level;
}

ConsoleLog* init_consoleLog(FILE* output){
  output = (output != NULL) ? output : stdout;
  if(output != stdout && output != stderr){
  assert(0 && "output must be stdout or stderr");
  return 1;
  }

  init();
  lock();
  logger_type |= CONSOLELOGGER;
  ConsoleLog* cl = (ConsoleLog*)malloc(sizeof(ConsoleLog));
  if(!cl){
    return NULL;
  }

  cl->output = output;

  
  unlock();
  return cl;

}

void free_console_log(ConsoleLog* cl){
  if(!cl) return;
  free(cl);
}

void close_logging(){
  if(log_file && log_file != stdout){
    fclose(log_file);
    log_file = NULL;
  }
}

void log_message(log_level_t level,const char* text){ 
  if(level < current_log_level){
    return;
  }

  if(!text){
    fprintf(stderr,"Nothing to say\n");
    exit(EXIT_FAILURE);
  }
 
  time_t now = time(NULL);
  fprintf(log_file,"[%s] %s,",ctime(&now), text);
  fflush(log_file);
}

void clear_log_file(const char* filename){
  FILE* file = fopen(filename,"w");
  if(!file){
    fprintf(stderr, "Could not open file: %s\n",filename);
    return;
  }

  fclose(file);
  printf("File has been succesfully cleared\n");
  
}

