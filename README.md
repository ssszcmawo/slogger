# slogger

Lightweight and dependency-free C logging library. Supports console and file logging, log rotation, simple ZIP archiving, and formatted log output with file/line tracing.

---

## Features

* Console and file logging
* Log levels: DEBUG, INFO, WARNING, ERROR
* Log rotation based on maximum file size
* Optional archiving of old logs into ZIP (store method only, no compression)
* Thread-safe using pthread mutex
* Minimal configuration via `.conf` file
* Zero external dependencies

---

## Project Structure

```
slogger/
├── src/
│   ├── slogger.c       # Core logging system
│   ├── config.c        # Config file parser
│   ├── zip.c           # Simple implementation of ZIP archive
│   └── ...
│   
├── tests/
│   ├── test_console_logger.c    # tests for console logger
│   ├── test_file_logger.c       # tests for file logger
│   ├── test_logconf.c           # tests for log configuration
├── README.md
├── Makefile
├── build.sh
```


## Builds
```c
./build.sh demo       # build the demo program (src/main.c), output: bin/myLogger_demo
./build.sh test       # build all test programs in tests/, output: bin/test_*
./build.sh tests      # same as 'test', build all test programs
./build.sh all        # build both demo and tests
./build.sh run        # run the demo program (bin/myLogger_demo), builds it if necessary
./build.sh clean      # remove all object files (obj/) and binaries (bin/*)
````

---


## Example

### Console logging

```c
#include "slogger.h"

int main() {
    init_consoleLog(stderr);
    set_log_level(DEBUG);

    LOG_INFO("INFO MESSAGE");
    LOG_DEBUG("DEBUG MESSAGE");
    LOG_ERROR("ERROR MESSAGE");
    LOG_WARN("WARNING MESSAGE");

    close_logging();
    return 0;
}
````

### File logging

```c
#include "slogger.h"

int main() {
    init_fileLog("logs/log.txt", 1024 * 1024, 1); // 1 MB max size, archive enabled
    set_log_level(DEBUG);

    LOG_INFO("INFO MESSAGE");
    LOG_DEBUG("DEBUG MESSAGE");

    close_logging();
    return 0;
}
```

*Output of the file logger:*

```
2025-12-06 20:31:45.784114 [INFO] [T7587] src/main.c:7: INFO MESSAGE
2025-12-06 20:31:45.784166 [DEBUG] [T7587] src/main.c:8: DEBUG MESSAGE
```

### Multi logging (console + file)

```c
#include "slogger.h"

int main() {
    init_consoleLog(NULL);             // Console logging to stdout
    init_fileLog("logs/log.txt", 0, 0); // File logging with default settings

    LOG_INFO("MULTI LOGGING");

    close_logging();
    return 0;
}
```

## Configuration Example (`logger.conf`)

```
logger=both
logger.console.output=stdout
logger.console.level=DEBUG
logger.file.output=logs/app.log
logger.file.maxFileSize=1048576
logger.file.archiveOldLogs=true
logger.level=INFO
```

---

