#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "slogger.h"

void test_basic_console_logging() {

    printf("Testing Basic logging\n");

    ConsoleLog* cl = init_consoleLog(stdout);
    assert(cl != NULL);  // Console logger should initialize
    assert(cl->output == stdout);

    log_message(INFO, "Basic info message");
    log_message(DEBUG, "Basic debug message");
    log_message(ERROR, "Basic error message");

    free_console_log(cl);

    printf("Passed basic console logging.\n");

}

void test_log_levels() {

    printf("Testing ConsoleLogger: Log level filtering...\n");

    set_log_level(WARNING);

    ConsoleLog* cl = init_consoleLog(stdout);
    assert(cl != NULL);

    // INFO should be ignored
    log_message(INFO, "This INFO message should NOT appear");
    // WARNING and ERROR should appear
    log_message(WARNING, "This WARNING message should appear");
    log_message(ERROR, "This ERROR message should appear");

    free_console_log(cl);

    // Reset log level
    set_log_level(INFO);

    printf("Passed log level filtering test.\n");

}

void test_null_message() {

    printf("Testing ConsoleLogger: NULL message handling...\n");

    ConsoleLog* cl = init_consoleLog(stdout);
    assert(cl != NULL);

    // Passing NULL should not crash
    log_message(INFO, NULL);
    log_message(ERROR, NULL);

    free_console_log(cl);

    printf("Passed NULL message handling test.\n");

}

int main() {

    test_basic_console_logging();
    test_log_levels();
    test_null_message();

    printf("All ConsoleLogger full tests passed!\n");
    return 0;

}
