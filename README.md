# slogger

Lightweight and dependency-free C logging library. Supports console and file logging, log rotation and formatted log output with file/line tracing.

## Features

* Console and file logging
* Log levels: DEBUG, INFO, WARNING, ERROR,TRACE
* Log rotation based on maximum file size
* Using SPSC Lock-Free Ring Buffer
* Zero external dependencies
* Compatible with both C and C++


## Project Structure

```
slogger/
├── common/                        #directory for utilities
│
├── examples/
│   ├── example_console_logger.c   #example of console logger
│   ├── example_file_logger.c      #example of file logger
│   ├── example_multi_logger.c     #example of multi logger
│
├── src/
│   ├── slogger.c                  # Core logging system
│   ├── slogger.h                  # Header to include in your project
│   
├── README.md
├── Makefile
```
## How to use (with Git submodule)
1. Add the logger repository as a submodule in your project:
```c
git submodule add <https://github.com/ssszcmawo/slogger> your_dir/slogger
git submodule update --init --recursive
```
2. Add the source files from the submodule to your build:
```c
#include "slogger.h"
```
3. Add all .c files from slogger/ to your build:

С
```c
 gcc -o myapp main.c \
    your_dir/slogger/src/slogger.c \
    your_dir/slogger/common/ring_buffer.c -pthread
```
C++ (CMakeLists)
```c
file(GLOB SLOGGER_SRC
    ${PROJECT_SOURCE_DIR}/your_dir/slogger/src/*.c
    ${PROJECT_SOURCE_DIR}/your_dir/slogger/common/*.c
)

add_library(slogger STATIC ${SLOGGER_SRC})

target_include_directories(slogger PUBLIC
    ${PROJECT_SOURCE_DIR}/your_dir/slogger/src
    ${PROJECT_SOURCE_DIR}/your_dir/slogger/common
)
```
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

