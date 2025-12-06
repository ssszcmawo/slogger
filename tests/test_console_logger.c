#define _GNU_SOURCE
#include "slogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TEMP_LOG_FILE "temp_console_log.txt"

void cleanup() {
    remove(TEMP_LOG_FILE);
}

void test_console_logger() {
    cleanup();

    FILE* temp_out = fopen(TEMP_LOG_FILE, "w+");
    assert(temp_out != NULL);

    assert(init_consoleLog(temp_out) == 1);

    set_log_level(INFO);
    LOG_INFO("Info message");
    LOG_DEBUG("Debug message should NOT appear");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");

    fflush(temp_out);
    fseek(temp_out, 0, SEEK_SET);

    char buf[4096];
    size_t read_len = fread(buf, 1, sizeof(buf)-1, temp_out);
    buf[read_len] = '\0';

    assert(strstr(buf, "Info message") != NULL);
    assert(strstr(buf, "Warning message") != NULL);
    assert(strstr(buf, "Error message") != NULL);

    close_logging();
    fclose(temp_out);
    cleanup();
}

int main() {
    test_console_logger();
    return 0;
}

