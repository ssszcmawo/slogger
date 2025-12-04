

## Example

### Console logging

```c
#include "slogger.h"

int main() {
    init_consoleLog(stderr);
    set_log_level(DEBUG);

    log_message(INFO, "console logging");
    log_messagef(DEBUG, "formatted message: %d %s", 42, "hello");

    close_logging();
    return 0;
}
````

### File logging

```c
#include "slogger.h"

int main() {
    init_fileLog("logs/log.txt", 1024 * 1024, true); // 1 MB max size, archive enabled
    set_log_level(DEBUG);

    log_message(INFO, "file logging");
    log_messagef(DEBUG, "format example: %d%c%s", 1, '2', "3");

    close_logging();
    return 0;
}
```

*Output of the file logger:*

```
INFO 2025-12-04 15:22:11.123456 [T12345] file logging
DEBUG 2025-12-04 15:22:11.123789 [T12345] format example: 123
```

### Multi logging (console + file)

```c
#include "slogger.h"

int main() {
    init_consoleLog(NULL);             // Console logging to stdout
    init_fileLog("logs/log.txt", 0, 0); // File logging with default settings

    log_message(INFO, "multi logging");

    close_logging();
    return 0;
}
```

## Features

* Console logging with colored output
* File logging with size-based rotation
* Optional archive of old logs
* Thread-safe logging
* Easy initialization and cleanup

