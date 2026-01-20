#include "slogger.h"

int main() {
    init_consoleLog(stdout);              // Console logging to stdout
    init_fileLog("logs/log.txt", 0);   // File logging

    LOG_INFO("MULTI LOGGING");

    close_logging();
    return 0;
}
