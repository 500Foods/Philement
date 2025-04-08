# ──────────────────────────────────────────────────────────────────────────────
# Hydrogen Build System Configuration
# 
# This Makefile provides a comprehensive build system for the Hydrogen project with
# multiple build variants:
# - Default: Standard build with optimizations and debug symbols
# - Debug: Build with AddressSanitizer for debugging memory issues
# - Valgrind: Special build optimized for memory analysis with Valgrind
# - Performance: Highly optimized build for maximum performance
# - Release: Stripped, compressed build with embedded encrypted payload
#
# Key features:
# - Automatic version numbering based on git commits
# - Multiple optimization levels for different use cases
# - Unused source file detection
# - UPX compression for release builds
# - Encrypted payload embedding
# - Comprehensive build artifact management
# 
# Available make targets:
# make          : Builds the default version with standard optimizations and debug symbols
# make all      : Builds all variants (default, debug, valgrind, perf, release) and generates payload
# make trial    : Runs a test build showing only warnings/errors, checks for unused files, runs shutdown test
# make debug    : Builds with AddressSanitizer enabled for catching memory issues
# make valgrind : Builds with -O0 and full debug info for optimal Valgrind analysis
# make perf     : Builds with aggressive optimizations (-O3, native arch, etc.) for maximum performance
# make release  : Creates stripped, compressed executable with embedded encrypted payload
# make payload  : Generates OpenAPI specs and packages payload contents
# make clean    : Removes all build artifacts and core dumps
# make cleanish : Same as clean but preserves hydrogen_release and hydrogen_naked
# make clean-executables : Removes only the executable files (used by release build)
# ──────────────────────────────────────────────────────────────────────────────

# Terminal output formatting for better readability
# These ANSI color codes are used throughout the build process to highlight
# different types of messages and status indicators
GREEN := \033[0;32m
RED := \033[0;31m
YELLOW := \033[0;33m
BLUE := \033[0;34m
CYAN := \033[0;36m
MAGENTA := \033[0;35m
BOLD := \033[1m
NORMAL := \033[0m

# Unicode status symbols for visual feedback
PASS := ✅
FAIL := ❌
WARN := ⚠️
INFO := 🛈

# Version information management
# - MAJOR.MINOR.PATCH follows semantic versioning
# - BUILD number is derived from git commit count plus offset
# - RELEASE timestamp is in ISO 8601 format
MAJOR = 1
MINOR = 0
PATCH = 0
GIT_BUILD = $(shell git log --oneline . | wc -l | tr -d ' ')
BUILD = $(shell expr $(GIT_BUILD) + 1000)
VERSION = $(MAJOR).$(MINOR).$(PATCH).$(BUILD)
RELEASE = $(shell date -u +'%Y-%m-%dT%H:%M:%SZ')
BUILD_TYPE = Regular

# Compiler configuration and base flags
# - Uses C17 standard with strict warning flags
# - Includes debug symbols (-g) and optimization (-O2) by default
# - Enables POSIX.1-2008 features and GNU extensions
# - Function/data section garbage collection enabled
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -Werror -O2 -g -DVERSION=\"$(VERSION)\" -DRELEASE=\"$(RELEASE)\" -DBUILD_TYPE=\"$(BUILD_TYPE)\" -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -ffunction-sections -fdata-sections
BASE_LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets -lbrotlienc -lbrotlidec

# Reset LD_PRELOAD globally to prevent unwanted library injection (e.g., from NoMachine)
LD_PRELOAD :=

# Directory structure management
# Detects if make is called from project root or src directory
# Adjusts paths accordingly to maintain consistent build structure
ROOT_CALL := $(if $(wildcard src/Makefile),yes,no)
PREFIX := $(if $(filter yes,$(ROOT_CALL)),src/,)
SRC_DIR := $(if $(filter yes,$(ROOT_CALL)),src,.)
BUILD_DIR = $(PREFIX)build
DEBUG_DIR = $(PREFIX)build_debug
VALGRIND_DIR = $(PREFIX)build_valgrind
PERF_DIR = $(PREFIX)build_perf
RELEASE_DIR = $(PREFIX)build_release
BIN_PREFIX := $(if $(filter yes,$(ROOT_CALL)),,../)
QUIET ?= 0

# Source file discovery and object file mapping
# Automatically finds all .c files and maps them to their respective build directories
SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SRCS:.c=.o))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%,$(DEBUG_DIR)/%,$(SRCS:.c=.o))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%,$(VALGRIND_DIR)/%,$(SRCS:.c=.o))
PERF_OBJS = $(patsubst $(SRC_DIR)/%,$(PERF_DIR)/%,$(SRCS:.c=.o))
RELEASE_OBJS = $(patsubst $(SRC_DIR)/%,$(RELEASE_DIR)/%,$(SRCS:.c=.o))

# Static library creation for each build variant
# These archives contain all object files for their respective builds
$(BUILD_DIR)/libhydrogen.a: $(OBJS)
	@printf "$(CYAN)$(INFO)  Creating archive for standard build...$(NORMAL)\n"
	$(AR) rcs $@ $^

$(DEBUG_DIR)/libhydrogen.a: $(DEBUG_OBJS)
	@printf "$(CYAN)$(INFO)  Creating archive for debug build...$(NORMAL)\n"
	$(AR) rcs $@ $^

$(VALGRIND_DIR)/libhydrogen.a: $(VALGRIND_OBJS)
	@printf "$(CYAN)$(INFO)  Creating archive for valgrind build...$(NORMAL)\n"
	$(AR) rcs $@ $^

$(PERF_DIR)/libhydrogen.a: $(PERF_OBJS)
	@printf "$(CYAN)$(INFO)  Creating archive for perf build...$(NORMAL)\n"
	$(AR) rcs $@ $^

$(RELEASE_DIR)/libhydrogen.a: $(RELEASE_OBJS)
	@printf "$(CYAN)$(INFO)  Creating archive for release build...$(NORMAL)\n"
	$(AR) rcs $@ $^

# Target executables for each build variant
TARGET = $(BIN_PREFIX)hydrogen
DEBUG_TARGET = $(BIN_PREFIX)hydrogen_debug
VALGRIND_TARGET = $(BIN_PREFIX)hydrogen_valgrind
PERF_TARGET = $(BIN_PREFIX)hydrogen_perf
RELEASE_TARGET = $(BIN_PREFIX)hydrogen_release

# Map files for detailed linker output analysis
# These files show how symbols are laid out in the final executable
MAP_FILE = $(BUILD_DIR)/hydrogen.map
DEBUG_MAP_FILE = $(DEBUG_DIR)/hydrogen_debug.map
VALGRIND_MAP_FILE = $(VALGRIND_DIR)/hydrogen_valgrind.map
PERF_MAP_FILE = $(PERF_DIR)/hydrogen_perf.map
RELEASE_MAP_FILE = $(RELEASE_DIR)/hydrogen_release.map

# Phony targets (targets that don't create files)
.PHONY: all clean cleanish debug valgrind perf release trial clean-executables
.PHONY: parallel-test
parallel-test: $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(PERF_TARGET) $(RELEASE_TARGET)

# Default build target with standard optimizations
default: $(TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Hydrogen$(NORMAL) $(GREEN)built successfully$(NORMAL)\n"

$(TARGET): LDFLAGS = -Wl,-Map=$(MAP_FILE) -Wl,--gc-sections $(BASE_LDFLAGS)
$(TARGET): $(BUILD_DIR)/libhydrogen.a
	@printf "$(CYAN)$(INFO)  Linking standard build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ -L$(BUILD_DIR) -lhydrogen $(LDFLAGS)

# Trial build with advanced diagnostics
# - Suppresses routine build messages
# - Shows only warnings and errors
# - Detects unused source files
# - Runs shutdown test
#
# Unused file detection:
# The trial target detects source files that aren't linked into the final executable
# by comparing object files in the map file against all source files. Files listed
# in .trial-ignore are filtered out from the warning output.
#
# .trial-ignore format:
# - Lines starting with # are comments explaining why files are ignored
# - File paths are relative to src/ directory (e.g., ./not_hydrogen.c)
# - Empty lines are ignored
#
# Process:
# 1. Extract object files that were linked into the final executable from the map file
# 2. Find all source files in the project
# 3. Map linked objects back to their source files
# 4. Read .trial-ignore file to get deliberately unlinked files
# 5. Compare to find truly unexpected unused files (those not in .trial-ignore)
trial:
	@printf "$(CYAN)$(INFO) Starting trial build (warnings and errors only)...$(NORMAL)\n"
	@$(MAKE) clean > /dev/null 2>&1
	@BUILD_OUTPUT=$$($(MAKE) default 2>&1); \
	echo "$$BUILD_OUTPUT" | grep -v -E "^[[:space:]]*Compiling|^make(\[[0-9]+\])?: (Entering|Leaving) directory|^[[:space:]]*$(INFO)|^[[:space:]]*gcc" | \
	grep -E "error:|warning:|undefined reference|collect2|ld returned|built successfully" --color=always || true; \
	if echo "$$BUILD_OUTPUT" | grep -q "built successfully" && ! echo "$$BUILD_OUTPUT" | grep -q -E "warning:|error:|undefined reference|ld returned"; then \
        printf "\n$(CYAN)$(INFO) Build successful with no warnings/errors. Running shutdown test...$(NORMAL)\n"; \
        $(BIN_PREFIX)tests/test_20_shutdown.sh; \
        printf "\n$(CYAN)$(INFO) Analyzing unused source files...$(NORMAL)\n"; \
        USED_OBJS=$$(grep -o "build/libhydrogen\.a([^)]*\.o)" $(MAP_FILE) | sed 's/build\/libhydrogen\.a(\([^)]*\.o\))/\1/' | sort -u); \
        ALL_SRCS=$$(echo "$(SRCS)" | tr ' ' '\n' | sort -u); \
        USED_SRCS=$$(for obj in $$USED_OBJS; do echo "$$ALL_SRCS" | grep "/$${obj%.o}\.c"; done | sort -u); \
        IGNORED_SRCS=$$(grep -v '^#' $(BIN_PREFIX).trial-ignore | grep -v '^$$' | sed 's|^\./|$(SRC_DIR)/|'); \
        UNUSED_SRCS=$$(comm -23 <(echo "$$ALL_SRCS") <(echo "$$USED_SRCS")); \
        REPORTABLE_SRCS=$$(comm -23 <(echo "$$UNUSED_SRCS") <(echo "$$IGNORED_SRCS" | sort)); \
        printf "Debug: USED_OBJS=$$(echo "$$USED_OBJS" | wc -l), ALL_SRCS=$$(echo "$$ALL_SRCS" | wc -l), USED_SRCS=$$(echo "$$USED_SRCS" | wc -l), UNUSED_SRCS=$$(echo "$$UNUSED_SRCS" | wc -l), IGNORED_SRCS=$$(echo "$$IGNORED_SRCS" | wc -l)\n"; \
        if [ -z "$$REPORTABLE_SRCS" ]; then \
            printf "$(GREEN)$(PASS) No unexpected unused source files detected$(NORMAL)\n"; \
        else \
            printf "$(YELLOW)$(WARN) Unexpected unused source files detected:$(NORMAL)\n"; \
            echo "$$REPORTABLE_SRCS" | while read -r file; do \
                printf "  $(YELLOW)- $$file$(NORMAL)\n"; \
            done; \
        fi; \
        printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"; \
    fi

# Build all variants and generate payload
all: default debug valgrind perf release payload
	@printf "\n$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(GREEN)$(PASS) $(BOLD)All variants built successfully:$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Default:     $(BOLD)$(TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Debug:       $(BOLD)$(DEBUG_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Valgrind:    $(BOLD)$(VALGRIND_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Performance: $(BOLD)$(PERF_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Release:     $(BOLD)$(RELEASE_TARGET)$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

# Payload generation for OpenAPI specs and other resources
payload:
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(BLUE)$(BOLD) Generating Payload Contents$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(CYAN)$(INFO)  Generating OpenAPI specification...$(NORMAL)\n"
	@cd $(BIN_PREFIX)payloads && ./swagger-generate.sh
	@printf "$(CYAN)$(INFO)  Packaging payload contents...$(NORMAL)\n"
	@cd $(BIN_PREFIX)payloads && ./payload-generate.sh
	@printf "$(GREEN)$(PASS) $(BOLD)Payload contents generated successfully!$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

# Release build with special optimizations
# - Strips debug symbols
# - Uses LTO and aggressive optimizations
# - Applies UPX compression
# - Embeds encrypted payload
clean-executables:
	@printf "$(CYAN)$(INFO)  Cleaning previous release executables...$(NORMAL)\n"
	@rm -f $(RELEASE_TARGET) $(BIN_PREFIX)hydrogen_naked

release: BUILD_TYPE = Release
release: CFLAGS += -s -DNDEBUG -march=x86-64 -flto=auto -fstack-protector -fno-strict-aliasing
release: LDFLAGS = -Wl,-Map=$(RELEASE_MAP_FILE) -Wl,--gc-sections -flto=auto -fno-strict-aliasing $(BASE_LDFLAGS)
release: clean-executables $(RELEASE_TARGET) payload
	@printf "$(CYAN)$(INFO)  Applying UPX compression to release executable...$(NORMAL)\n"
	@if ! which upx > /dev/null; then \
	    printf "$(YELLOW)$(WARN) UPX not found, skipping compression$(NORMAL)\n"; \
	else \
	    upx --best $(RELEASE_TARGET); \
	fi
	@printf "$(CYAN)$(INFO)  Creating UPX-compressed copy as hydrogen_naked...$(NORMAL)\n"
	@cp $(RELEASE_TARGET) $(BIN_PREFIX)hydrogen_naked
	@printf "$(CYAN)$(INFO)  Appending encrypted payload to release executable...$(NORMAL)\n"
	@PAYLOAD_SIZE=$$(stat -c %s "$(BIN_PREFIX)payloads/payload.tar.br.enc"); \
	cat $(RELEASE_TARGET) "$(BIN_PREFIX)payloads/payload.tar.br.enc" > $(RELEASE_TARGET).tmp; \
	printf "<<< HERE BE ME TREASURE >>>" >> $(RELEASE_TARGET).tmp; \
	printf "$$(printf '%016x' $$PAYLOAD_SIZE | sed 's/\(..\)/\\x\1/g')" >> $(RELEASE_TARGET).tmp; \
	mv $(RELEASE_TARGET).tmp $(RELEASE_TARGET); \
	chmod +x $(RELEASE_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Release build with encrypted payload appended successfully!$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

$(RELEASE_TARGET): $(RELEASE_DIR)/libhydrogen.a
	@printf "$(CYAN)$(INFO)  Linking release build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ -L$(RELEASE_DIR) -lhydrogen $(LDFLAGS)

# Debug build with AddressSanitizer
# Enables memory error detection and stack trace reporting
debug: BUILD_TYPE = Debug
debug: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
debug: LDFLAGS = -lasan -Wl,-Map=$(DEBUG_MAP_FILE) -Wl,--gc-sections -fsanitize=address $(BASE_LDFLAGS)
debug: $(DEBUG_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Debug build$(NORMAL) $(GREEN)with AddressSanitizer completed successfully$(NORMAL)\n"

$(DEBUG_TARGET): $(DEBUG_DIR)/libhydrogen.a
	@printf "$(CYAN)$(INFO)  Linking debug build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ -L$(DEBUG_DIR) -lhydrogen $(LDFLAGS)

# Valgrind-compatible build
# Disables optimizations for better analysis results
valgrind: BUILD_TYPE = Valgrind-Compatible
valgrind: CFLAGS := -std=c17 -Wall -Wextra -pedantic -Werror -O0 -g -DVERSION=\"$(VERSION)\" -DRELEASE=\"$(RELEASE)\" -DBUILD_TYPE=\"$(BUILD_TYPE)\" -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
valgrind: LDFLAGS = -Wl,-Map=$(VALGRIND_MAP_FILE) $(BASE_LDFLAGS)
valgrind: $(VALGRIND_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Valgrind-compatible build$(NORMAL) $(GREEN)completed successfully$(NORMAL)\n"

$(VALGRIND_TARGET): $(VALGRIND_DIR)/libhydrogen.a
	@printf "$(CYAN)$(INFO)  Linking valgrind-compatible build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ -L$(VALGRIND_DIR) -lhydrogen $(LDFLAGS)

# Performance-optimized build
# Enables aggressive optimizations and CPU-specific tuning
perf: BUILD_TYPE = Performance
perf: CFLAGS += -O3 -march=native -ffast-math -finline-functions -funroll-loops -DNDEBUG
perf: LDFLAGS = -Wl,-Map=$(PERF_MAP_FILE) -Wl,--gc-sections -flto=auto $(BASE_LDFLAGS)
perf: $(PERF_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Performance-optimized build$(NORMAL) $(GREEN)completed successfully$(NORMAL)\n"

$(PERF_TARGET): $(PERF_DIR)/libhydrogen.a
	@printf "$(CYAN)$(INFO)  Linking performance-optimized build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ -L$(PERF_DIR) -lhydrogen $(LDFLAGS)

# Compilation rules for different build variants
# Creates output directories as needed and shows progress
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(MAKECMDGOALS)" != "trial" ]; then \
		if [ "$(QUIET)" = "0" ]; then \
			printf "$(CYAN)Compiling$(NORMAL) $<\n"; \
		fi \
 	fi
	@$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(QUIET)" = "0" ]; then \
		printf "$(CYAN)Compiling$(NORMAL) [Debug] $<\n"; \
	fi
	@$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(QUIET)" = "0" ]; then \
		printf "$(CYAN)Compiling$(NORMAL) [Valgrind] $<\n"; \
	fi 
	@$(CC) $(CFLAGS) -c -o $@ $<

$(PERF_DIR)/%.o: $(SRC_DIR)/%.c | $(PERF_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(QUIET)" = "0" ]; then \
		printf "$(CYAN)Compiling$(NORMAL) [Performance] $<\n"; \
	fi
	@$(CC) $(CFLAGS) -c -o $@ $<

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(QUIET)" = "0" ]; then \
		printf "$(CYAN)Compiling$(NORMAL) [Release] $<\n"; \
	fi
	@$(CC) $(CFLAGS) -c -o $@ $<

# Build directory creation
# Creates the necessary directory structure for each build variant
$(BUILD_DIR):
	@printf "$(CYAN)$(INFO)  Creating build directories...$(NORMAL)\n"
	@mkdir -p $(BUILD_DIR)/api/system

$(DEBUG_DIR):
	@printf "$(CYAN)$(INFO)  Creating debug build directories...$(NORMAL)\n"
	@mkdir -p $(DEBUG_DIR)/api/system

$(VALGRIND_DIR):
	@printf "$(CYAN)$(INFO)  Creating valgrind build directories...$(NORMAL)\n"
	@mkdir -p $(VALGRIND_DIR)/api/system

$(PERF_DIR):
	@printf "$(CYAN)$(INFO)  Creating performance build directories...$(NORMAL)\n"
	@mkdir -p $(PERF_DIR)/api/system

$(RELEASE_DIR):
	@printf "$(CYAN)$(INFO)  Creating release build directories...$(NORMAL)\n"
	@mkdir -p $(RELEASE_DIR)/api/system

# Ensure all required subdirectories exist
$(shell mkdir -p $(sort $(dir $(OBJS)) $(dir $(DEBUG_OBJS)) $(dir $(VALGRIND_OBJS)) $(dir $(PERF_OBJS)) $(dir $(RELEASE_OBJS)) ))

# Cleanup
# Removes all build artifacts and core dumps
clean:
	@printf "$(CYAN)$(INFO)  Cleaning all build artifacts...$(NORMAL)\n"
	@rm -rf $(BUILD_DIR)/ $(DEBUG_DIR)/ $(VALGRIND_DIR)/ $(RELEASE_DIR)/ $(PERF_DIR)/
	@rm -rf $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(RELEASE_TARGET) $(PERF_TARGET) $(BIN_PREFIX)hydrogen_naked
	@printf "$(CYAN)$(INFO)  Removing crash core dumps...$(NORMAL)\n"
	@find .. -name "*.core.*" -type f -delete
	@printf "$(GREEN)$(PASS) Clean completed successfully$(NORMAL)\n"

# Cleanup preserving release executables
# Removes all build artifacts and core dumps except hydrogen_release and hydrogen_naked
cleanish:
	@printf "$(CYAN)$(INFO)  Cleaning build artifacts (preserving release executables)...$(NORMAL)\n"
	@rm -rf $(BUILD_DIR)/ $(DEBUG_DIR)/ $(VALGRIND_DIR)/ $(RELEASE_DIR)/ $(PERF_DIR)/
	@rm -f $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(PERF_TARGET)
	@printf "$(CYAN)$(INFO)  Removing crash core dumps...$(NORMAL)\n"
	@find .. -name "*.core.*" -type f -delete
	@printf "$(GREEN)$(PASS) Cleanish completed successfully (preserved $(BIN_PREFIX)hydrogen_release and $(BIN_PREFIX)hydrogen_naked)$(NORMAL)\n"