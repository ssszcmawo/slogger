CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I$(SRC_DIR) -I$(COMMON_DIR)

SRC_DIR     = src
COMMON_DIR  = common
EXAMPLES_DIR = examples
OBJ_DIR     = obj
BIN_DIR     = bin

SRCS        = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(COMMON_DIR)/*.c)
MAIN_SRC    = $(SRC_DIR)/main.c
OTHER_SRCS  = $(filter-out $(MAIN_SRC), $(SRCS))

MAIN_OBJ    = $(OBJ_DIR)/main.o
OBJS        = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter $(SRC_DIR)/%.c,$(OTHER_SRCS))) \
              $(patsubst $(COMMON_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter $(COMMON_DIR)/%.c,$(OTHER_SRCS)))

BIN = $(BIN_DIR)/slogger

all: $(BIN)

$(BIN): $(OBJS) $(MAIN_OBJ)
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(MAIN_OBJ): $(MAIN_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

EXAMPLE_SRCS = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLES_DIR)/%.c,$(BIN_DIR)/example-%,$(EXAMPLE_SRCS))

examples: $(EXAMPLE_BINS)

$(BIN_DIR)/example-%: $(EXAMPLES_DIR)/%.c $(OBJS) | $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all examples clean

