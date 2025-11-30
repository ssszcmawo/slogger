#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include "slogger.h"

#define TEST_LOG_FILE "test_log.txt"
#define TEST_ARCHIVE_DIR "archiveLogsDir"

void cleanup() {
    remove(TEST_LOG_FILE); // remove main log
    for(int i = 1; i <= DEFAULT_MAX_BACKUP_FILES; i++){
        char fname[128];
        snprintf(fname, sizeof(fname), "%s.%d", TEST_LOG_FILE, i);
        remove(fname); // remove rotated backups
    }
    rmdir(TEST_ARCHIVE_DIR); // remove archive directory if exists
}

void test_basic_logging() {

    clear_log_file(TEST_LOG_FILE);

    FileLog* fl = init_fileLog(TEST_LOG_FILE, 1024, 1);

    assert(fl != NULL);

    log_message(INFO, "Basic log message");
    fflush(fl->output);

    FILE* f = fopen(TEST_LOG_FILE, "r");

    assert(f != NULL);

    char buf[256];
    int found = 0;

    while(fgets(buf, sizeof(buf), f)) {

        if(strstr(buf, "Basic log message")) found = 1;

    }

    fclose(f);
    assert(found);

    free_file_log(fl);

}

void test_rotation_and_backup() {

    FileLog* fl = init_fileLog(TEST_LOG_FILE, 50, 1); // small size to trigger rotation
    assert(fl != NULL);

    for(int i = 0; i < 10; i++){

        log_message(INFO, "Rotating log entry");
        fflush(fl->output);
        fl->currentFileSize = ftell(fl->output); // simulate increasing file size
    //
    }

    assert(rotateFiles()); // should rotate and create backup
    free_file_log(fl);
}

void test_archive_creation() {
    FileLog* fl = init_fileLog(TEST_LOG_FILE, 50, 1);
    assert(fl != NULL);

    log_message(INFO, "Archive test message");
    fflush(fl->output);

    fl->currentFileSize = fl->maxFileSize + 1; // force rotation
    assert(rotateFiles());

    // Check archive directory exists
    DIR* dir = opendir(TEST_ARCHIVE_DIR);
    assert(dir != NULL);
    closedir(dir);

    free_file_log(fl);
}

int main() {

    cleanup();  

    test_basic_logging(); 

    test_rotation_and_backup(); 

    test_archive_creation();


    cleanup();
    printf("All FileLogger full tests passed!\n");
    return 0;
}
