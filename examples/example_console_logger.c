#include "slogger.h"

int main() {
    // Initializes the logger and sets the output stream
    // stderr is commonly used for logs and error messages
    init_consoleLog(stderr);

    // Sets the minimum log level to display
    // DEBUG means all log levels will be printed
    set_log_level(DEBUG);

    // Logs an informational message
    LOG_INFO("INFO MESSAGE");

    // Logs a debug message (useful for development and debugging)
    LOG_DEBUG("DEBUG MESSAGE");

    // Logs an error message (used for critical failures)
    LOG_ERROR("ERROR MESSAGE");

    // Logs a warning message (used for potential issues)
    LOG_WARN("WARNING MESSAGE");

    // Releases resources and properly shuts down the logger
    close_logging();

    return 0;
}
