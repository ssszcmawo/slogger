#define _GNU_SOURCE
#include "slogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LEN 512
#define MAX_FILE_NAME 64
#define DEFAULT_MAX_FILE_SIZE 1048576L

static struct {
    FILE* output;
    char fileName[MAX_FILE_NAME];
    long maxFileSize;
    long currentFileSize;
    log_level_t level;
    int archiveOldLogs;
    char archiveDir[MAX_FILE_NAME];
} file_log_conf;
static struct {
    FILE* output;
    log_level_t level;
}console_log_conf;

static char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;

    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static int split_key_value(const char* line, char* key, size_t key_size, char* value, size_t value_size) {
    const char* eq = strchr(line, '=');
    if (!eq) return 0;

    size_t key_len = eq - line;
    size_t val_len = strlen(eq + 1);

    if (key_len >= key_size || val_len >= value_size) return 0;

    strncpy(key, line, key_len);
    key[key_len] = '\0';
    strcpy(value, eq + 1);

    trim(key);
    trim(value);
    return 1;
}

static log_level_t parse_log_level(const char* s) {
    if (strcasecmp(s, "DEBUG")   == 0) return DEBUG;
    if (strcasecmp(s, "INFO")    == 0) return INFO;
    if (strcasecmp(s, "WARNING") == 0) return WARNING;
    if (strcasecmp(s, "ERROR")   == 0) return ERROR;
    fprintf(stderr, "WARNING: Unknown log level '%s', using INFO\n", s);
    return INFO;
}

int configure_logger(const char* config_path) {
    if (!config_path || !config_path[0]) {
        fprintf(stderr, "ERROR: config path is NULL or empty\n");
        return 0;
    }

    FILE* fp = fopen(config_path, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open config file: %s (%s)\n", config_path, strerror(errno));
        return 0;
    }

    char line[MAX_LINE_LEN];
    char key[128], value[384];
    int console_enabled = 0;
    int file_enabled    = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        if (!split_key_value(line, key, sizeof(key), value, sizeof(value)))
            continue;

        if (strcmp(key, "logger") == 0) {
            if (strcasecmp(value, "console") == 0) console_enabled = 1;
            else if (strcasecmp(value, "file") == 0) file_enabled = 1;
            else if (strcasecmp(value, "both") == 0) { console_enabled = file_enabled = 1; }
            else fprintf(stderr, "WARNING: Unknown logger type: %s\n", value);

        } else if (strcmp(key, "logger.console.output") == 0) {
            if (strcasecmp(value, "stdout") == 0)      console_log_conf.output = stdout;
            else if (strcasecmp(value, "stderr") == 0) console_log_conf.output = stderr;
            else fprintf(stderr, "WARNING: Invalid console output: %s\n", value);

        } else if (strcmp(key, "logger.console.level") == 0) {
            set_log_level(parse_log_level(value));

        } else if (strcmp(key, "logger.file.output") == 0) {
            strncpy(file_log_conf.fileName, value, sizeof(file_log_conf.fileName)-1);
            file_log_conf.fileName[sizeof(file_log_conf.fileName)-1] = '\0';

        } else if (strcmp(key, "logger.file.maxFileSize") == 0) {
            char* end;
            long size = strtol(value, &end, 10);
            if (*end != '\0') fprintf(stderr, "WARNING: Invalid maxFileSize: %s\n", value);
            else file_log_conf.maxFileSize = (size > 0) ? size : DEFAULT_MAX_FILE_SIZE;

        } else if (strcmp(key, "logger.file.archiveOldLogs") == 0) {
            file_log_conf.archiveOldLogs = (strcasecmp(value, "true") == 0 ||
                                           strcasecmp(value, "1") == 0 ||
                                           strcasecmp(value, "yes") == 0);

        } else if (strcmp(key, "logger.level") == 0) {
            set_log_level(parse_log_level(value));
        }
    }

    fclose(fp);

    int success = 1;

    if (console_enabled) {
        if (!init_consoleLog(console_log_conf.output ? console_log_conf.output : stdout)) {
            fprintf(stderr, "ERROR: Failed to initialize console logger\n");
            success = 0;
        }
    }

    if (file_enabled) {
        if (file_log_conf.fileName[0] == '\0') {
            fprintf(stderr, "ERROR: File logger enabled but no output file specified\n");
            success = 0;
        } else {
            if (!init_fileLog(file_log_conf.fileName,
                             file_log_conf.maxFileSize,
                             file_log_conf.archiveOldLogs)) {
                fprintf(stderr, "ERROR: Failed to initialize file logger\n");
                success = 0;
            }
        }
    }

    if (!console_enabled && !file_enabled) {
        fprintf(stderr, "ERROR: No logger enabled in config\n");
        return 0;
    }

    return success;
}
