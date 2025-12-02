#include "slogger.h"
#include <stdio.h>

int main(int argc,char** argv) { 

  FileLog* fl = init_fileLog("log_file",DEFAULT_MAX_FILE_SIZE,1);

  if(!fl){
    fprintf(stderr,"ERROR: Could not init file logger\n");
    return 1;
  }

  log_message(INFO,"INFO MESSAGE");
  log_message(DEBUG,"DEBUG MESSAGE");
  log_message(ERROR,"ERROR MESSAGE");
  log_message(WARNING,"WARNING MESSAGE");

  close_logging();
  
  return 0;

}
