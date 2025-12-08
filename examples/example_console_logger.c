#include "slogger.h"

int main() {
    init_consoleLog(stderr);
    set_log_level(DEBUG);

    LOG_INFO("INFO MESSAGE");
    LOG_DEBUG("DEBUG MESSAGE");
    LOG_ERROR("ERROR MESSAGE");
    LOG_WARN("WARNING MESSAGE");


    close_logging();
    return 0;
}
