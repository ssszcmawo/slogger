# compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# Directories
SRC_DIR = src
TEST_DIR = tests
EXAMPLES_DIR = examples
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
MAIN_SRC = $(SRC_DIR)/main.c
OTHER_SRCS = $(filter-out $(MAIN_SRC), $(SRCS))

# Objects for main binary
MAIN_OBJ = $(OBJ_DIR)/main.o
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(OTHER_SRCS))

# Main binary
BIN = $(BIN_DIR)/slogger

# ========================
# Default: build main binary
# ========================
all: $(BIN)

$(BIN): $(OBJS) $(MAIN_OBJ)
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(MAIN_OBJ): $(MAIN_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# ========================
# Build tests: one binary per test
# ========================
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/test-%,$(TEST_SRCS))

test: $(TEST_BINS)

$(BIN_DIR)/test-%: $(TEST_DIR)/%.c $(OBJS) | $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $^ -o $@


# ========================
# Build examples: one binary per example
# ========================
EXAMPLE_SRCS = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLES_DIR)/%.c,$(BIN_DIR)/example-%,$(EXAMPLE_SRCS))

examples: $(EXAMPLE_BINS)

$(BIN_DIR)/example-%: $(EXAMPLES_DIR)/%.c $(OBJS) | $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $^ -o $@


# ========================
# Folders
# ========================
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)


# ========================
# Clean
# ========================
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all test examples clean

