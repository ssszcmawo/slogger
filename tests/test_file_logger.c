#define _GNU_SOURCE
#include "slogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>

#define LOG_FILE "test_file_log.txt"
#define DEFAULT_MAX_BACKUP_FILES 10

void cleanup() {
    remove(LOG_FILE);
    for(int i = 1; i <= DEFAULT_MAX_BACKUP_FILES * 10; i++){
        char fname[128];
        snprintf(fname, sizeof(fname), "%s.%d", LOG_FILE, i);
        remove(fname);
    }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s_archive", LOG_FILE);
    system(cmd);
}
void test_file_logging_basic() {
    cleanup();
    assert(init_fileLog(LOG_FILE, 1024, false) == 1);
    LOG_INFO("Basic log message");
    close_logging();
    FILE* f = fopen(LOG_FILE, "r");
    assert(f != NULL);
    char line[512];
    int found = 0;
    while(fgets(line, sizeof(line), f)) {
        if(strstr(line, "Basic log message")) found = 1;
    }
    fclose(f);
    assert(found);
}

void test_file_logging_rotation() {
    cleanup();
    assert(init_fileLog(LOG_FILE, 50, false) == 1);
    for(int i = 0; i < 20; i++){
        LOG_INFO("Rotating log entry %d", i);
    }
    close_logging();
    FILE* f = fopen(LOG_FILE, "r");
    assert(f != NULL);
    fclose(f);
    char backup[128];
    snprintf(backup, sizeof(backup), "%s.1", LOG_FILE);
    f = fopen(backup, "r");
    assert(f != NULL);
    fclose(f);
}

void test_file_logging_archive() {
    cleanup();
    assert(init_fileLog(LOG_FILE, 50, true) == 1);

    for(int i = 0; i < DEFAULT_MAX_BACKUP_FILES * 5; i++) {
        LOG_INFO("Archive log entry %d", i);
    }

    close_logging();

    DIR* dir = opendir("test_file_log.txt_archive");
    assert(dir != NULL);
    closedir(dir);

    int zip_found = 0;
    DIR *d = opendir(".");
    struct dirent *de;
    while((de = readdir(d))) {
        if(strstr(de->d_name, "_archive_") && strstr(de->d_name, ".zip")) {
            zip_found = 1;
            break;
        }
    }
    closedir(d);
    assert(zip_found == 1);
}
int main() {
    test_file_logging_basic();
    test_file_logging_rotation();
    test_file_logging_archive();
    cleanup();
    return 0;
}

