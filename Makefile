CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin


SRCS = $(wildcard $(SRC_DIR)/*.c);
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))


TEST_BIN = $(BIN_DIR)/test_logger


all: $(TEST_BIN)



$(TEST_BIN): $(OBJS) $(TEST_OBJS)
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@



$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)


clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
