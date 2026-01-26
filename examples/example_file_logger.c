#include "slogger.h"

int main() {

    start_logging_thread();
    init_fileLog("logs/log.txt", 1024 * 1024);

    set_log_level(DEBUG);

    LOG_INFO("INFO MESSAGE");

    LOG_DEBUG("DEBUG MESSAGE");

    LOG_WARN("WARNING MESSAGE");

    LOG_ERROR("ERROR MESSAGE");

    LOG_TRACE("TRACE MESSAGE");

    close_logging();

    return 0;
}
