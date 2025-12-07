#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

int create_zip(const char *folder, const char *zipname)
{
    if (!folder || !zipname || !folder[0] || !zipname[0]) return -1;

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "zip -q -r -j -m \"%s\" \"%s\"/*.log* 2>/dev/null || "
             "zip -q -r -j \"%s\" \"%s\" 2>/dev/null",
             zipname, folder, zipname, folder);

    if (system(cmd) != 0) return -1;

    mkdir(folder, 0755);
    return 0;
}
