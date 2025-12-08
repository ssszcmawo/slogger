#include "slogger.h"

int main() {
    init_fileLog("logs/log.txt", 1024 * 1024, 1); // 1 MB max size, archive enabled
    set_log_level(DEBUG);

    LOG_INFO("INFO MESSAGE");
    LOG_DEBUG("DEBUG MESSAGE");

    close_logging();
    return 0;
}
