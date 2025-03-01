# Build System Configuration
#
# Build strategy prioritizes:
# 1. Code Quality
#    - Modern C17 standard for latest features
#    - Strict warning flags for code quality
#    - Pedantic mode for standard compliance
#    - O2 optimization for performance/safety balance
#
# 2. Development Experience
#    - Separate debug/valgrind builds
#    - Address sanitizer integration
#    - Clear error reporting
#    - Fast incremental builds
#
# 3. Maintainability
#    - Automatic dependency tracking
#    - Modular directory structure
#    - Consistent naming scheme
#    - Clean build separation
#
# 4. Production Readiness
#    - Optimized release builds
#    - Proper symbol handling
#    - Version management
#    - Deployment preparation
#    - Binary compression for minimal footprint

# Version information
VERSION = 1.0.0

# Compiler selection and optimization flags
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -O2

# External Dependencies
#
# Library selection rationale:
# 1. Core Functionality
#    - jansson: Efficient JSON handling with memory safety
#    - pthread: POSIX thread support for concurrency
#    - microhttpd: Lightweight embedded HTTP server
#
# 2. Security
#    - ssl/crypto: Industry-standard encryption
#    - Memory safety features
#    - Secure random number generation
#
# 3. Communication
#    - websockets: Full-duplex realtime messaging
#    - Efficient binary protocol support
#    - Automatic connection management
#
# 4. Build Variants
#    - Dynamic linking (default): Standard shared library usage
#    - Debug builds for development
#    - Valgrind builds for memory checking
#    - Release builds for deployment with maximum optimization

LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets

# Directory structure
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = build_debug
VALGRIND_DIR = build_valgrind
RELEASE_DIR = build_release

# Source and object files for different build configurations
SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SRCS:.c=.o))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%,$(DEBUG_DIR)/%,$(SRCS:.c=.o))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%,$(VALGRIND_DIR)/%,$(SRCS:.c=.o))
RELEASE_OBJS = $(patsubst $(SRC_DIR)/%,$(RELEASE_DIR)/%,$(SRCS:.c=.o))

# Build targets
TARGET = hydrogen
DEBUG_TARGET = hydrogen_debug
VALGRIND_TARGET = hydrogen_valgrind
RELEASE_TARGET = hydrogen_release

# Phony targets (don't create files with these names)
.PHONY: all clean debug valgrind release

# Default target: optimized build
all: $(TARGET)

# Release build: highly optimized with size reduction for deployment
release: CFLAGS += -s -DNDEBUG -march=x86-64 -flto=auto -fstack-protector -DVERSION=\"$(VERSION)\"
release: LDFLAGS += -flto=auto
release: $(RELEASE_TARGET)
	@which upx > /dev/null && upx --best $(RELEASE_TARGET) || echo "UPX not found, skipping compression"


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

$(RELEASE_TARGET): $(RELEASE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


# Compilation rules for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<


# Create build directories if they don't exist
$(BUILD_DIR):
	mkdir -p $@
	mkdir -p $(BUILD_DIR)/api/system

$(DEBUG_DIR):
	mkdir -p $@
	mkdir -p $(DEBUG_DIR)/api/system

$(VALGRIND_DIR):
	mkdir -p $@
	mkdir -p $(VALGRIND_DIR)/api/system

$(RELEASE_DIR):
	mkdir -p $@
	mkdir -p $(RELEASE_DIR)/api/system

# Create subdirectories for object files
$(shell mkdir -p $(sort $(dir $(OBJS)) $(dir $(DEBUG_OBJS)) $(dir $(VALGRIND_OBJS)) $(dir $(RELEASE_OBJS))))

# Clean all build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DEBUG_DIR) $(VALGRIND_DIR) $(RELEASE_DIR) $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(RELEASE_TARGET)