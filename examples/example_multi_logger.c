#include "slogger.h"

int main() {
    init_consoleLog(NULL);             // Console logging to stdout
    init_fileLog("logs/log.txt", 0, 0); // File logging with default settings

    LOG_INFO("MULTI LOGGING");

    close_logging();
    return 0;
}
