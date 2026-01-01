#include "slogger.h"

int main() {
    // Initializes file-based logging
    // "logs/log.txt"      -> path to the log file
    // 1024 * 1024         -> maximum log file size in bytes (1 MB)
    // 1                   -> enable log file archiving (rotation)
    init_fileLog("logs/log.txt", 1024 * 1024, 1);

    // Sets the minimum log level to DEBUG
    // All log messages (DEBUG and higher) will be logged
    set_log_level(DEBUG);

    // Logs an informational message to the log file
    LOG_INFO("INFO MESSAGE");

    // Logs a debug message (useful for tracing program execution)
    LOG_DEBUG("DEBUG MESSAGE");

    // Properly closes the logger and flushes the log file
    close_logging();

    return 0;
}
