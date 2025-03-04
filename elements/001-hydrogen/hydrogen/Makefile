# ──────────────────────────────────────────────────────────────────────────────
# Hydrogen Build System Configuration
# ──────────────────────────────────────────────────────────────────────────────
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

# ──────────────────────────────────────────────────────────────────────────────
# Terminal formatting codes
# ──────────────────────────────────────────────────────────────────────────────
GREEN := \033[0;32m
RED := \033[0;31m
YELLOW := \033[0;33m
BLUE := \033[0;34m
CYAN := \033[0;36m
MAGENTA := \033[0;35m
BOLD := \033[1m
NORMAL := \033[0m

# Status symbols
PASS := ✅
FAIL := ❌
WARN := ⚠️
INFO := 🛈

# ──────────────────────────────────────────────────────────────────────────────
# Version information
# ──────────────────────────────────────────────────────────────────────────────
MAJOR = 1
MINOR = 0
PATCH = 0
GIT_BUILD = $(shell git log --oneline . | wc -l | tr -d ' ')
BUILD = $(shell expr $(GIT_BUILD) + 1000)

VERSION = $(MAJOR).$(MINOR).$(PATCH).$(BUILD)
RELEASE = $(shell date -u +'%Y-%m-%dT%H:%M:%SZ')
BUILD_TYPE = Regular

# ──────────────────────────────────────────────────────────────────────────────
# Compiler selection and optimization flags
# ──────────────────────────────────────────────────────────────────────────────
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -O2 -DVERSION=\"$(VERSION)\" -DRELEASE=\"$(RELEASE)\" -DBUILD_TYPE=\"$(BUILD_TYPE)\"

# ──────────────────────────────────────────────────────────────────────────────
# External Dependencies
# ──────────────────────────────────────────────────────────────────────────────
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

LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets -lbrotlienc -lbrotlidec

# ──────────────────────────────────────────────────────────────────────────────
# Directory structure and file paths
# ──────────────────────────────────────────────────────────────────────────────
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = build_debug
VALGRIND_DIR = build_valgrind
RELEASE_DIR = build_release
PERF_DIR = build_perf

# ──────────────────────────────────────────────────────────────────────────────
# Source and object files for different build configurations
# ──────────────────────────────────────────────────────────────────────────────
SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SRCS:.c=.o))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%,$(DEBUG_DIR)/%,$(SRCS:.c=.o))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%,$(VALGRIND_DIR)/%,$(SRCS:.c=.o))
RELEASE_OBJS = $(patsubst $(SRC_DIR)/%,$(RELEASE_DIR)/%,$(SRCS:.c=.o))
PERF_OBJS = $(patsubst $(SRC_DIR)/%,$(PERF_DIR)/%,$(SRCS:.c=.o))

# ──────────────────────────────────────────────────────────────────────────────
# Build targets
# ──────────────────────────────────────────────────────────────────────────────
TARGET = hydrogen
DEBUG_TARGET = hydrogen_debug
VALGRIND_TARGET = hydrogen_valgrind
RELEASE_TARGET = hydrogen_release
PERF_TARGET = hydrogen_perf

# ──────────────────────────────────────────────────────────────────────────────
# Phony targets (don't create files with these names)
# ──────────────────────────────────────────────────────────────────────────────
.PHONY: all clean debug valgrind release perf all-variants swagger

# ──────────────────────────────────────────────────────────────────────────────
# Default target: only build the standard variant
# ──────────────────────────────────────────────────────────────────────────────
default: $(TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Hydrogen$(NORMAL) $(GREEN)built successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Build all variants at once
# ──────────────────────────────────────────────────────────────────────────────
all: clean default debug valgrind release perf
	@printf "\n$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(GREEN)$(PASS) $(BOLD)All variants built successfully:$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Default:     $(BOLD)$(TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Debug:       $(BOLD)$(DEBUG_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Valgrind:    $(BOLD)$(VALGRIND_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Release:     $(BOLD)$(RELEASE_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Performance: $(BOLD)$(PERF_TARGET)$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

# Legacy target for backward compatibility
all-variants: all

# ──────────────────────────────────────────────────────────────────────────────
# Swagger documentation generation
# ──────────────────────────────────────────────────────────────────────────────
swagger:
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(BLUE)$(BOLD) Generating Swagger Documentation$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(CYAN)$(INFO)  Generating OpenAPI specification...$(NORMAL)\n"
	@cd swagger && ./swagger-generate.sh
	@printf "$(CYAN)$(INFO)  Packaging SwaggerUI interface...$(NORMAL)\n"
	@cd swagger && ./swaggerui-generate.sh
	@printf "$(GREEN)$(PASS) $(BOLD)Swagger documentation generated successfully!$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Release build: highly optimized with size reduction for deployment
# ──────────────────────────────────────────────────────────────────────────────
release: BUILD_TYPE = Release
release: CFLAGS += -s -DNDEBUG -march=x86-64 -flto=auto -fstack-protector -Werror -fno-strict-aliasing
release: LDFLAGS += -flto=auto -fno-strict-aliasing
release: $(RELEASE_TARGET) swagger
	@printf "$(CYAN)$(INFO)  Applying UPX compression to executable...$(NORMAL)\n"
	@which upx > /dev/null && upx --best $(RELEASE_TARGET) || printf "$(YELLOW)$(WARN) UPX not found, skipping compression$(NORMAL)\n"
	@printf "$(CYAN)$(INFO)  Appending Swagger UI to release executable...$(NORMAL)\n"
	@TAR_SIZE=$$(stat -c %s "swagger/swaggerui.tar.br"); \
	cat $(RELEASE_TARGET) "swagger/swaggerui.tar.br" > $(RELEASE_TARGET).tmp; \
	printf "<<< HERE BE ME TREASURE >>>" >> $(RELEASE_TARGET).tmp; \
	printf "$$(printf '%016x' $$TAR_SIZE | sed 's/\(..\)/\\x\1/g')" >> $(RELEASE_TARGET).tmp; \
	mv $(RELEASE_TARGET).tmp $(RELEASE_TARGET); \
	chmod +x $(RELEASE_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Release build with Swagger UI appended successfully!$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"


# ──────────────────────────────────────────────────────────────────────────────
# Debug build: includes Address Sanitizer and debug symbols
# ──────────────────────────────────────────────────────────────────────────────
debug: BUILD_TYPE = Debug
debug: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address
debug: $(DEBUG_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Debug build$(NORMAL) $(GREEN)with AddressSanitizer completed successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Valgrind build: includes debug symbols for memory checking
# ──────────────────────────────────────────────────────────────────────────────
valgrind: BUILD_TYPE = Valgrind-Compatible
valgrind: CFLAGS += -g
valgrind: $(VALGRIND_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Valgrind-compatible build$(NORMAL) $(GREEN)completed successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Performance build: maximum runtime performance, hardware-specific optimizations
# ──────────────────────────────────────────────────────────────────────────────
perf: BUILD_TYPE = Performance
perf: CFLAGS += -O3 -march=native -ffast-math -finline-functions -funroll-loops -DNDEBUG
perf: LDFLAGS += -flto=auto
perf: $(PERF_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Performance-optimized build$(NORMAL) $(GREEN)completed successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Linking rules for each build configuration
# ──────────────────────────────────────────────────────────────────────────────
$(TARGET): $(OBJS)
	@printf "$(CYAN)$(INFO)  Linking standard build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	@printf "$(CYAN)$(INFO)  Linking debug build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(VALGRIND_TARGET): $(VALGRIND_OBJS)
	@printf "$(CYAN)$(INFO)  Linking valgrind-compatible build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(RELEASE_TARGET): $(RELEASE_OBJS)
	@printf "$(CYAN)$(INFO)  Linking release build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PERF_TARGET): $(PERF_OBJS)
	@printf "$(CYAN)$(INFO)  Linking performance-optimized build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


# ──────────────────────────────────────────────────────────────────────────────
# Compilation rules for object files
# ──────────────────────────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Debug] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Valgrind] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Release] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(PERF_DIR)/%.o: $(SRC_DIR)/%.c | $(PERF_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Performance] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<


# ──────────────────────────────────────────────────────────────────────────────
# Create build directories if they don't exist
# ──────────────────────────────────────────────────────────────────────────────
$(BUILD_DIR):
	@printf "$(CYAN)$(INFO)  Creating build directories...$(NORMAL)\n"
	@mkdir -p $@
	@mkdir -p $(BUILD_DIR)/api/system

$(DEBUG_DIR):
	@printf "$(CYAN)$(INFO)  Creating debug build directories...$(NORMAL)\n"
	@mkdir -p $@
	@mkdir -p $(DEBUG_DIR)/api/system

$(VALGRIND_DIR):
	@printf "$(CYAN)$(INFO)  Creating valgrind build directories...$(NORMAL)\n"
	@mkdir -p $@
	@mkdir -p $(VALGRIND_DIR)/api/system

$(RELEASE_DIR):
	@printf "$(CYAN)$(INFO)  Creating release build directories...$(NORMAL)\n"
	@mkdir -p $@
	@mkdir -p $(RELEASE_DIR)/api/system

$(PERF_DIR):
	@printf "$(CYAN)$(INFO)  Creating performance build directories...$(NORMAL)\n"
	@mkdir -p $@
	@mkdir -p $(PERF_DIR)/api/system

# ──────────────────────────────────────────────────────────────────────────────
# Create subdirectories for object files
# ──────────────────────────────────────────────────────────────────────────────
$(shell mkdir -p $(sort $(dir $(OBJS)) $(dir $(DEBUG_OBJS)) $(dir $(VALGRIND_OBJS)) $(dir $(RELEASE_OBJS)) $(dir $(PERF_OBJS))))

# ──────────────────────────────────────────────────────────────────────────────
# Clean all build artifacts
# ──────────────────────────────────────────────────────────────────────────────
clean:
	@printf "$(CYAN)$(INFO)  Cleaning all build artifacts...$(NORMAL)\n"
	@rm -rf $(BUILD_DIR) $(DEBUG_DIR) $(VALGRIND_DIR) $(RELEASE_DIR) $(PERF_DIR) $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(RELEASE_TARGET) $(PERF_TARGET)
	@printf "$(GREEN)$(PASS) Clean completed successfully$(NORMAL)\n"