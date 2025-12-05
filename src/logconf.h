#ifndef LOGCONF_H
#define LOGCONF_H

#include "slogger.h"
#include <stdbool.h>

/**
 * Configures the logger by reading settings from a configuration file.
 *
 * This function parses a simple .ini-style configuration file and automatically
 * initializes console and/or file logging according to the specified options.
 *
 * Returns:
 *   1 on success
 *   0 on failure (invalid path, parse error, missing required fields, etc.)
 *
 * Example usage:
 *   if (!configure_logger("log.conf")) {
 *       fprintf(stderr, "Failed to configure logger!\n");
 *       exit(1);
 *   }
 */
int configure_logger(const char* config_path);

/**
 * Example of a valid configuration file (log.conf):
 *
 * # Supported logger types: console, file, both
 * logger = both
 *
 * # Console settings
 * logger.console.output = stdout        # stdout or stderr
 * logger.console.level  = DEBUG         # DEBUG, INFO, WARNING, ERROR
 *
 * # File logger settings
 * logger.file.output          = app.log
 * logger.file.maxFileSize     = 10485760      # in bytes (10 MB)
 * logger.file.archiveOldLogs  = true          # true/false, yes/no, 1/0
 *
 * # Optional: global log level (overrides individual levels)
 * logger.level = INFO
 *
 * # Lines starting with # are comments
 * # Empty lines and spaces around keys/values are ignored
 */

#endif // LOGCONF_H
