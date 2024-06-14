CC = gcc
CFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wall -Wextra -std=c17 -I./src -I/usr/local/include
LDFLAGS = -fsanitize=address -ljansson -lpthread -lssl -lcrypto -lm -lwebsockets

SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET = nitro

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
        $(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
        mkdir -p $(BUILD_DIR)
        $(CC) $(CFLAGS) -c $< -o $@

clean:
        rm -rf $(BUILD_DIR) $(TARGET)

memcheck:
        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./nitro
