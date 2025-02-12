
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -O2
LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets

SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = build_debug
VALGRIND_DIR = build_valgrind

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SRCS))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%.c,$(VALGRIND_DIR)/%.o,$(SRCS))

TARGET = hydrogen
DEBUG_TARGET = hydrogen_debug
VALGRIND_TARGET = hydrogen_valgrind

.PHONY: all clean debug valgrind

all: $(TARGET)

debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address
debug: $(DEBUG_TARGET)

valgrind: CFLAGS += -g
valgrind: $(VALGRIND_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(VALGRIND_TARGET): $(VALGRIND_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

$(DEBUG_DIR):
	mkdir -p $@

$(VALGRIND_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(DEBUG_DIR) $(VALGRIND_DIR) $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET)
