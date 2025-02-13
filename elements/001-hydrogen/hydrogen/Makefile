# Compiler and basic flags
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -O2

# Required external library dependencies
# -ljansson    : JSON parsing and creation library
# -lpthread    : POSIX threads for concurrent operations
# -lmicrohttpd : Lightweight HTTP server functionality
# -lm          : Standard C math library
# -lssl        : OpenSSL for SSL/TLS encryption
# -lcrypto     : OpenSSL cryptographic functions
# -lwebsockets : WebSocket protocol implementation
LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets

# Directory structure
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = build_debug
VALGRIND_DIR = build_valgrind

# Source and object files for different build configurations
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SRCS))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%.c,$(VALGRIND_DIR)/%.o,$(SRCS))

# Build targets
TARGET = hydrogen
DEBUG_TARGET = hydrogen_debug
VALGRIND_TARGET = hydrogen_valgrind

# Phony targets (don't create files with these names)
.PHONY: all clean debug valgrind

# Default target: optimized build
all: $(TARGET)

# Debug build: includes Address Sanitizer and debug symbols
debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address
debug: $(DEBUG_TARGET)

# Valgrind build: includes debug symbols for memory checking
valgrind: CFLAGS += -g
valgrind: $(VALGRIND_TARGET)

# Linking rules for each build configuration
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(VALGRIND_TARGET): $(VALGRIND_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation rules for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Create build directories if they don't exist
$(BUILD_DIR):
	mkdir -p $@

$(DEBUG_DIR):
	mkdir -p $@

$(VALGRIND_DIR):
	mkdir -p $@

# Clean all build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DEBUG_DIR) $(VALGRIND_DIR) $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET)