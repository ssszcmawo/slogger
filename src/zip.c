#define _GNU_SOURCE
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>

int create_zip(const char *folder, const char *zipname)
{
    if (!folder || !folder[0] || !zipname || !zipname[0]) return -1;

    pid_t pid = fork();
    if (pid == -1) return -1;

    if (pid == 0) {
        if (chdir(folder) == -1) _exit(127);

        char *const args[] = {
            "zip", "-q", "-r", "-m", "-j",
            (char*)zipname,
            ".",
            NULL
        };

        execvp("zip", args);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return -1;

    char src[PATH_MAX];
    snprintf(src, sizeof(src), "%s/%s", folder, zipname);

    char *folder_copy = strdup(folder);
    if (!folder_copy) return 0;
    char *parent = dirname(folder_copy);

    char dst[PATH_MAX];
    snprintf(dst, sizeof(dst), "%s/%s", parent, zipname);

    rename(src, dst);
    free(folder_copy);

    return 0;
}
