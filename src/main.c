#include "slogger.h"

int main()
{
  init_consoleLog(stdout);
  set_log_level(INFO);

  printf("init console logging\n");

  LOG_INFO("INFO");
  LOG_DEBUG("DEBUG");
  LOG_ERROR("ERROR");
  LOG_WARN("WARN");
  LOG_TRACE("TRACE");

  close_logging();

  return 0;
}
