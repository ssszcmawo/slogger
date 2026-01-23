#include "slogger.h"

int main() {
    start_logging_thread();
    init_consoleLog(stdout);              // Console logging to stdout
    init_fileLog("logs/log.txt", 0);   // File logging
    
    set_log_level(DEBUG);

    LOG_INFO("MULTI LOGGING");
    LOG_WARN("MULTI LOGGING");
    LOG_ERROR("MULTI LOGGING");
    LOG_DEBUG("MULTI LOGGING");
    LOG_TRACE("MULTI LOGGING");

    close_logging();
    return 0;
}
