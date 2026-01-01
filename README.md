# slogger

Lightweight and dependency-free C logging library. Supports console and file logging, log rotation, simple ZIP archiving, and formatted log output with file/line tracing.

## Features

* Console and file logging
* Log levels: DEBUG, INFO, WARNING, ERROR
* Log rotation based on maximum file size
* Optional archiving of old logs into ZIP (store method only, no compression)
* Thread-safe using pthread mutex
* Minimal configuration via `.conf` file
* Zero external dependencies

## Project Structure

```
slogger/
├── examples/
│   ├── example_console_logger.c   #example of console logger
│   ├── example_file_logger.c      #example of file logger
│   ├── example_logger_conf.conf   #example of configuration file
│   ├── example_multi_logger.c     #example of multi logger
│
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
## How to use
1. Copy the entire `src/` folder into your project.

2. In your source files, include the header:
```c
#include "src/slogger.h"
#include "src/logconf.h"
```
3. Add all .c files from src/ to your build:
```c
 gcc -o myapp main.c src/slogger.c src/zip.c src/logconf.c -pthread
```
## Builds
```c
./build.sh demo       # build the demo program (src/main.c), output: bin/myLogger_demo
./build.sh test       # build all test programs in tests/, output: bin/test_*
./build.sh tests      # same as 'test', build all test programs
./build.sh examples   # build examples
./build.sh all        # build both demo and tests
./build.sh run        # run the demo program (bin/myLogger_demo), builds it if necessary
./build.sh clean      # remove all object files (obj/) and binaries (bin/*)
````

## Example

*Output of the console logger:*

<img width="1190" height="706" alt="Screenshot from 2025-12-08 17-59-48(1)" src="https://github.com/user-attachments/assets/9a945e26-d3fe-4cbd-8c1f-dd436b24b86b" />



*Output of the file logger:*


```
2025-12-08 18:06:23.812854 [DEBUG] [T8486] src/main.c:7: DEBUG MESSAGE
2025-12-08 18:06:23.812944 [ERROR] [T8486] src/main.c:8: ERROR MESSAGE
2025-12-08 18:06:23.812951 [WARN] [T8486] src/main.c:9: WARNING MESSAGE
2025-12-08 18:06:23.812956 [INFO] [T8486] src/main.c:12: INFO MESSAGE
2025-12-08 18:06:23.812961 [DEBUG] [T8486] src/main.c:13: DEBUG MESSAGE
2025-12-08 18:06:23.812966 [WARN] [T8486] src/main.c:14: WARN MESSAGE
2025-12-08 18:06:23.812970 [ERROR] [T8486] src/main.c:15: ERROR MESSAGE
2025-12-08 18:06:23.812975 [INFO] [T8486] src/main.c:12: INFO MESSAGE
2025-12-08 18:06:23.812979 [DEBUG] [T8486] src/main.c:13: DEBUG MESSAGE
2025-12-08 18:06:23.812984 [WARN] [T8486] src/main.c:14: WARN MESSAGE
2025-12-08 18:06:23.812989 [ERROR] [T8486] src/main.c:15: ERROR MESSAGE
```


---

