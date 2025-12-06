#define _GNU_SOURCE
#include "slogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define TEMP_CONFIG "temp_logger.conf"
#define LOG_FILE "temp_log.txt"

void cleanup() {
    remove(TEMP_CONFIG);
    remove(LOG_FILE);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "rm -rf %s_archive", LOG_FILE);
    system(cmd);
}

void test_console_logger() {
    FILE* f = fopen(TEMP_CONFIG, "w");
    assert(f != NULL);
    fprintf(f,
        "logger=console\n"
        "logger.console.output=stdout\n"
        "logger.console.level=DEBUG\n");
    fclose(f);

    assert(configure_logger(TEMP_CONFIG) == 1);
    assert(get_log_level() == DEBUG);

    LOG_DEBUG("Console test message");
    close_logging();
}

void test_file_logger() {
    FILE* f = fopen(TEMP_CONFIG, "w");
    assert(f != NULL);
    fprintf(f,
        "logger=file\n"
        "logger.file.output=%s\n"
        "logger.file.maxFileSize=1024\n"
        "logger.file.archiveOldLogs=true\n"
        "logger.level=INFO\n", LOG_FILE);
    fclose(f);

    assert(configure_logger(TEMP_CONFIG) == 1);
    assert(get_log_level() == INFO);

    LOG_INFO("File test message");

    FILE* log = fopen(LOG_FILE, "r"); 

    char line[256];
    int found = 0;
    while(fgets(line, sizeof(line), log)) {
        if(strstr(line, "File test message")) found = 1;
    }
    fclose(log);
    assert(found);

    close_logging();
}

void test_both_loggers() {
    FILE* f = fopen(TEMP_CONFIG, "w");
    assert(f != NULL);
    fprintf(f,
        "logger=both\n"
        "logger.console.output=stderr\n"
        "logger.file.output=%s\n"
        "logger.file.maxFileSize=1024\n"
        "logger.file.archiveOldLogs=false\n"
        "logger.level=WARNING\n", LOG_FILE);
    fclose(f);

    assert(configure_logger(TEMP_CONFIG) == 1);
    assert(get_log_level() == WARNING);

    LOG_WARN("Both logger test message");

    FILE* log = fopen(LOG_FILE, "r");
    assert(log != NULL);

    char line[256];
    int found = 0;
    while(fgets(line, sizeof(line), log)) {
        if(strstr(line, "Both logger test message")) found = 1;
    }
    fclose(log);
    assert(found);

    close_logging();
}

int main() {
    cleanup();
    test_console_logger();
    test_file_logger();
    test_both_loggers();
    cleanup();
    return 0;
}

