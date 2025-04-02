# ──────────────────────────────────────────────────────────────────────────────
# Hydrogen Build System Configuration
# ──────────────────────────────────────────────────────────────────────────────

# ──────────────────────────────────────────────────────────────────────────────
# Build Variants Overview
# ──────────────────────────────────────────────────────────────────────────────
# 1. Default Build (make)
#    Directory: build/
#    Purpose: Standard development
#    Features:
#    - O2 optimization level
#    - Full warnings enabled
#    - C17 standard support
#    - Balance of performance and debugging
#    Use when: Day-to-day development
#
# 2. Debug Build (make debug)
#    Directory: build_debug/
#    Purpose: Bug finding and analysis
#    Features:
#    - Debug symbols enabled (-g)
#    - Address Sanitizer integration
#    - Stack frame pointers preserved
#    - No optimization for better debugging
#    Use when: Tracking down bugs or memory issues
#
# 3. Valgrind Build (make valgrind)
#    Directory: build_valgrind/
#    Purpose: Memory analysis
#    Features:
#    - Debug symbols enabled (-g)
#    - Valgrind-compatible compilation
#    - No sanitizers (unlike debug build)
#    Use when: Deep memory analysis needed
#
# 4. Performance Build (make perf)
#    Directory: build_perf/
#    Purpose: Maximum speed
#    Features:
#    - O3 optimization level
#    - Native architecture tuning
#    - Fast math operations
#    - Function inlining
#    - Loop unrolling
#    - Link-time optimization
#    Use when: Need maximum runtime performance
#
# 5. Release Build (make release)
#    Directory: build_release/
#    Purpose: Production deployment
#    Features:
#    - Stripped symbols
#    - Maximum compatibility (x86-64)
#    - Link-time optimization
#    - Stack protection enabled
#    - UPX compression
#    - Encrypted payload included
#    Use when: Building for production/distribution

# ──────────────────────────────────────────────────────────────────────────────
# Build Strategy Priorities
# ──────────────────────────────────────────────────────────────────────────────
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
#
# ──────────────────────────────────────────────────────────────────────────────
# External Dependencies and Version Requirements
# ──────────────────────────────────────────────────────────────────────────────
# Required library versions and compatibility:
# - jansson >= 2.13: JSON parsing with security fixes
# - microhttpd >= 0.9.73: TLS 1.3 support
# - libwebsockets >= 4.2: WebSocket protocol v13
# - OpenSSL >= 1.1.1: TLS 1.3 compatibility
# - brotli >= 1.0.9: Improved compression ratios
# 1. Core System Libraries
#    - pthread (-lpthread)
#      • POSIX thread support for concurrent operations
#      • Thread creation, synchronization, and management
#      • Mutex and condition variable implementations
#    
#    - math (-lm)
#      • Standard C math library
#      • Mathematical operations and calculations
#
# 2. Data Processing
#    - jansson (-ljansson)
#      • Fast, reliable JSON parsing and generation
#      • Memory-safe object handling
#      • Structured data manipulation
#
#    - brotli (-lbrotlienc, -lbrotlidec)
#      • High-quality compression algorithm
#      • Encoder for compressing data (-lbrotlienc)
#      • Decoder for decompressing data (-lbrotlidec)
#
# 3. Network Communication
#    - microhttpd (-lmicrohttpd)
#      • Lightweight embedded HTTP server
#      • RESTful API endpoint handling
#      • HTTP/1.1 protocol implementation
#
#    - websockets (-lwebsockets)
#      • Full-duplex WebSocket communication
#      • Real-time messaging capabilities
#      • Automatic connection management
#
# 4. Security and Encryption
#    - OpenSSL (-lssl, -lcrypto)
#      • TLS/SSL protocol implementation (-lssl)
#      • Cryptographic functions (-lcrypto)
#      • Secure communication channels
#      • Certificate handling
#      • Random number generation

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
# Version information and build numbering
# ──────────────────────────────────────────────────────────────────────────────
# Version components:
# - MAJOR: Major version for breaking changes
# - MINOR: Minor version for new features
# - PATCH: Patch version for bug fixes
# - BUILD: Automatically incremented based on git history
#   • Counts total commits in current directory
#   • Adds 1000 as base offset for production builds
#   • Format: MAJOR.MINOR.PATCH.BUILD
#
# Release timestamp uses UTC for consistent cross-timezone builds
# Build type is used in version string and affects compilation flags
MAJOR = 1
MINOR = 0
PATCH = 0
GIT_BUILD = $(shell git log --oneline . | wc -l | tr -d ' ')
BUILD = $(shell expr $(GIT_BUILD) + 1000)

VERSION = $(MAJOR).$(MINOR).$(PATCH).$(BUILD)
RELEASE = $(shell date -u +'%Y-%m-%dT%H:%M:%SZ')
BUILD_TYPE = Regular

# ──────────────────────────────────────────────────────────────────────────────
# Compiler selection and feature configuration
# ──────────────────────────────────────────────────────────────────────────────
# Compiler flags explanation:
# - std=c17: Use modern C standard for advanced features
# - Wall/Wextra/pedantic: Enable comprehensive warnings
# - Werror: Treat all warnings as errors
# - O2: Default optimization level (balance of speed/debugging)
# - g: Include debug symbols in all builds
#
# Feature test macros:
# - _GNU_SOURCE: Enable GNU extensions (required for some APIs)
# - _POSIX_C_SOURCE=200809L: Enable POSIX.1-2008 features
CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -Werror -O2 -g -DVERSION=\"$(VERSION)\" -DRELEASE=\"$(RELEASE)\" -DBUILD_TYPE=\"$(BUILD_TYPE)\"
CFLAGS += -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L

# ──────────────────────────────────────────────────────────────────────────────
# External Dependencies
# ──────────────────────────────────────────────────────────────────────────────
LDFLAGS = -ljansson -lpthread -lmicrohttpd -lm -lssl -lcrypto -lwebsockets -lbrotlienc -lbrotlidec

# ──────────────────────────────────────────────────────────────────────────────
# Directory structure and path handling
# ──────────────────────────────────────────────────────────────────────────────
# The build system supports being called from either:
# 1. Project root directory (hydrogen/)
# 2. Source directory (hydrogen/src/)
#
# Path detection and adjustment:
# - ROOT_CALL: Detects if make was called from project root
# - PREFIX: Adjusts paths for build directories
# - SRC_DIR: Points to the source files
# - BIN_PREFIX: Ensures executables and resources are placed correctly
#
# This allows the build system to work correctly regardless of which
# directory it was invoked from, maintaining consistent paths for:
# - Source files
# - Build artifacts
# - Test scripts
# - Resource files
#
# Detect if we're being called from the project root or src directory
ROOT_CALL := $(if $(wildcard src/Makefile),yes,no)
PREFIX := $(if $(filter yes,$(ROOT_CALL)),src/,)
SRC_DIR := $(if $(filter yes,$(ROOT_CALL)),src,.)

# Build directories are always relative to SRC_DIR
BUILD_DIR = $(PREFIX)build
DEBUG_DIR = $(PREFIX)build_debug
VALGRIND_DIR = $(PREFIX)build_valgrind
PERF_DIR = $(PREFIX)build_perf
RELEASE_DIR = $(PREFIX)build_release

# Executables and resources are relative to the directory containing src/
BIN_PREFIX := $(if $(filter yes,$(ROOT_CALL)),,../)

# ──────────────────────────────────────────────────────────────────────────────
# Source discovery and object file mapping
# ──────────────────────────────────────────────────────────────────────────────
# Source file discovery:
# - Uses find to locate all .c files recursively
# - Handles nested directory structures
# - Maintains relative paths for organization
#
# Object file generation:
# - Preserves directory structure in build directories
# - Uses pattern substitution for consistent naming
# - Creates separate objects for each build variant
#
# Example transformation:
# src/api/system/info.c -> build/api/system/info.o
SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SRCS:.c=.o))
DEBUG_OBJS = $(patsubst $(SRC_DIR)/%,$(DEBUG_DIR)/%,$(SRCS:.c=.o))
VALGRIND_OBJS = $(patsubst $(SRC_DIR)/%,$(VALGRIND_DIR)/%,$(SRCS:.c=.o))
PERF_OBJS = $(patsubst $(SRC_DIR)/%,$(PERF_DIR)/%,$(SRCS:.c=.o))
RELEASE_OBJS = $(patsubst $(SRC_DIR)/%,$(RELEASE_DIR)/%,$(SRCS:.c=.o))

# ──────────────────────────────────────────────────────────────────────────────
# Build targets and executable naming
# ──────────────────────────────────────────────────────────────────────────────
# Target naming convention:
# - Default: 'hydrogen'
# - Debug: 'hydrogen_debug'
# - Valgrind: 'hydrogen_valgrind'
# - Performance: 'hydrogen_perf'
# - Release: 'hydrogen_release'
#
# All executables are placed relative to the project root
# using BIN_PREFIX to handle different invocation locations
TARGET = $(BIN_PREFIX)hydrogen
DEBUG_TARGET = $(BIN_PREFIX)hydrogen_debug
VALGRIND_TARGET = $(BIN_PREFIX)hydrogen_valgrind
PERF_TARGET = $(BIN_PREFIX)hydrogen_perf
RELEASE_TARGET = $(BIN_PREFIX)hydrogen_release

# ──────────────────────────────────────────────────────────────────────────────
# Phony targets and special build rules
# ──────────────────────────────────────────────────────────────────────────────
# Targets marked as phony for several reasons:
# - They don't create files matching their names
# - They should always execute when called
# - They represent actions rather than files
#
# Examples:
# - 'all': Builds everything but creates no 'all' file
# - 'clean': Removes files but creates no 'clean' file
# - 'trial': Runs tests but creates no 'trial' file
.PHONY: all clean debug valgrind perf release trial clean-executables

# ──────────────────────────────────────────────────────────────────────────────
# Default target: only build the standard variant
# ──────────────────────────────────────────────────────────────────────────────
default: $(TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Hydrogen$(NORMAL) $(GREEN)built successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Trial build: Focused error detection with filtered output
# ──────────────────────────────────────────────────────────────────────────────
# This target performs a clean build while filtering the output to show only
# important messages. It uses a two-stage grep pipeline:
#
# Stage 1 (grep -v): Removes noise by filtering out:
# - Compilation progress messages ("Compiling file.c")
# - Make directory traversal messages
# - Info messages and gcc command lines
#
# Stage 2 (grep -E): Shows only important messages:
# - Compiler warnings (warning:)
# - Compiler errors (error:)
# - Linker errors (undefined reference, collect2, ld returned)
# - Success message (built successfully)
#
# If the build succeeds with no warnings/errors, it automatically runs
# the shutdown test to verify basic functionality.
trial:
	@printf "$(CYAN)$(INFO) Starting trial build (warnings and errors only)...$(NORMAL)\n"
	@$(MAKE) clean > /dev/null 2>&1
	@BUILD_OUTPUT=$$($(MAKE) default 2>&1); \
	echo "$$BUILD_OUTPUT" | grep -v -E "^[[:space:]]*Compiling|^make(\[[0-9]+\])?: (Entering|Leaving) directory|^[[:space:]]*$(INFO)|^[[:space:]]*gcc" | \
	grep -E "error:|warning:|undefined reference|collect2|ld returned|built successfully" --color=always || true; \
	if echo "$$BUILD_OUTPUT" | grep -q "built successfully" && ! echo "$$BUILD_OUTPUT" | grep -q -E "warning:|error:|undefined reference|ld returned"; then \
		printf "\n$(CYAN)$(INFO) Build successful with no warnings/errors. Running shutdown test...$(NORMAL)\n"; \
		$(BIN_PREFIX)tests/test_20_shutdown.sh; \
	fi

# ──────────────────────────────────────────────────────────────────────────────
# Build all variants and resources
# ──────────────────────────────────────────────────────────────────────────────
# The 'all' target builds every variant in a specific order:
# 1. clean: Ensures fresh build environment
# 2. default: Basic development build
# 3. debug: For bug hunting with sanitizers
# 4. valgrind: For memory analysis
# 5. perf: For performance testing
# 6. release: For production deployment
# 7. payload: Generates and packages resources
#
# Each variant is built with its specific flags and configurations,
# ensuring a complete test suite across all build types.
all: clean default debug valgrind perf release payload
	@printf "\n$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"
	@printf "$(GREEN)$(PASS) $(BOLD)All variants built successfully:$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Default:     $(BOLD)$(TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Debug:       $(BOLD)$(DEBUG_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Valgrind:    $(BOLD)$(VALGRIND_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Performance: $(BOLD)$(PERF_TARGET)$(NORMAL)\n"
	@printf "  $(CYAN)$(INFO)  Release:     $(BOLD)$(RELEASE_TARGET)$(NORMAL)\n"
	@printf "$(BLUE)────────────────────────────────────────────────────────────────$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Payload generation and resource packaging
# ──────────────────────────────────────────────────────────────────────────────
# The payload target generates and packages essential resources:
# 1. OpenAPI/Swagger Documentation
#    - Generated from source code annotations
#    - Used for API documentation and testing
#    - Embedded in release builds for offline access
#
# 2. Resource Packaging
#    - Compresses web UI and documentation
#    - Encrypts sensitive configuration templates
#    - Bundles all resources into a single archive
#
# The packaged payload is used by both development builds
# (extracted on demand) and release builds (embedded in binary)
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

# ──────────────────────────────────────────────────────────────────────────────
# Release build: Production-ready binary with embedded resources
# ──────────────────────────────────────────────────────────────────────────────
# The release build process involves several steps:
# 1. Compilation with optimizations:
#    - Link-time optimization (-flto=auto)
#    - Stack protection (-fstack-protector)
#    - x86-64 compatibility (-march=x86-64)
#    - Strict aliasing disabled (-fno-strict-aliasing)
#
# 2. Binary preparation:
#    - Strips debug symbols (-s)
#    - Applies UPX compression for size reduction
#    - Creates an unencrypted copy (hydrogen_naked)
#
# 3. Payload integration:
#    - Appends encrypted payload to the end of the executable
#    - Adds a marker ("<<< HERE BE ME TREASURE >>>")
#    - Includes payload size as a 16-byte hex trailer
#
# The final binary contains both the executable and its resources,
# which are extracted at runtime if needed.

clean-executables:
	@printf "$(CYAN)$(INFO)  Cleaning previous release executables...$(NORMAL)\n"
	@rm -f $(RELEASE_TARGET) $(BIN_PREFIX)hydrogen_naked

release: BUILD_TYPE = Release
release: CFLAGS += -s -DNDEBUG -march=x86-64 -flto=auto -fstack-protector -Werror -fno-strict-aliasing
release: LDFLAGS += -flto=auto -fno-strict-aliasing
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
perf: CFLAGS += -O3 -march=native -ffast-math -finline-functions -funroll-loops -g -DNDEBUG
perf: LDFLAGS += -flto=auto
perf: $(PERF_TARGET)
	@printf "$(GREEN)$(PASS) $(BOLD)Performance-optimized build$(NORMAL) $(GREEN)completed successfully$(NORMAL)\n"

# ──────────────────────────────────────────────────────────────────────────────
# Linking rules and build-specific configurations
# ──────────────────────────────────────────────────────────────────────────────
# Each build variant has specific linking requirements:
# - Default: Standard linking with all dependencies
# - Debug: Includes ASan runtime and debug symbols
# - Valgrind: Pure debug symbols without sanitizers
# - Performance: Link-time optimization for speed
# - Release: Stripped symbols and embedded payload
#
# The linking phase combines all object files with their
# respective libraries, applying build-specific flags and
# producing the final executable for each variant.
$(TARGET): $(OBJS)
	@printf "$(CYAN)$(INFO)  Linking standard build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	@printf "$(CYAN)$(INFO)  Linking debug build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(VALGRIND_TARGET): $(VALGRIND_OBJS)
	@printf "$(CYAN)$(INFO)  Linking valgrind-compatible build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PERF_TARGET): $(PERF_OBJS)
	@printf "$(CYAN)$(INFO)  Linking performance-optimized build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(RELEASE_TARGET): $(RELEASE_OBJS)
	@printf "$(CYAN)$(INFO)  Linking release build...$(NORMAL)\n"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ──────────────────────────────────────────────────────────────────────────────
# Compilation rules with conditional output
# ──────────────────────────────────────────────────────────────────────────────
# Each build variant has its own compilation rule that:
# 1. Creates necessary directories automatically
# 2. Handles output differently for trial vs normal builds
# 3. Applies variant-specific compiler flags
#
# Output handling:
# - Trial build: Suppresses "Compiling" messages for cleaner output
# - Other builds: Shows progress for each compiled file
# - Debug/Valgrind/Perf/Release: Adds build type to output
#
# Directory creation is handled automatically via order-only
# prerequisites (the '|' in the rule declarations)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@if [ "$(MAKECMDGOALS)" != "trial" ]; then \
		printf "$(CYAN)Compiling$(NORMAL) $<\n"; \
	fi
	@$(CC) $(CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Debug] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(VALGRIND_DIR)/%.o: $(SRC_DIR)/%.c | $(VALGRIND_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Valgrind] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(PERF_DIR)/%.o: $(SRC_DIR)/%.c | $(PERF_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Performance] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_DIR)
	@mkdir -p $(dir $@)
	@printf "$(CYAN)Compiling$(NORMAL) [Release] $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $<

# ──────────────────────────────────────────────────────────────────────────────
# Build directory structure and organization
# ──────────────────────────────────────────────────────────────────────────────
# Each build variant maintains its own directory structure:
# - api/system: Core API implementation modules
# - Additional subdirectories created as needed
#
# Directory creation is handled:
# 1. During object file compilation (automatic)
# 2. Via explicit directory targets (fallback)
# 3. Through the global directory creation rule
#
# The api/system path is required for the core API modules
# and is created by default for all build variants.
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

# ──────────────────────────────────────────────────────────────────────────────
# Create subdirectories for object files
# ──────────────────────────────────────────────────────────────────────────────
$(shell mkdir -p $(sort $(dir $(OBJS)) $(dir $(DEBUG_OBJS)) $(dir $(VALGRIND_OBJS)) $(dir $(PERF_OBJS)) $(dir $(RELEASE_OBJS)) )) 

# ──────────────────────────────────────────────────────────────────────────────
# Clean build artifacts and crash dumps
# ──────────────────────────────────────────────────────────────────────────────
# Cleaning process:
# 1. Removes all build directories and their contents
#    - Includes all build variants (debug, release, etc.)
#    - Removes generated object files and dependencies
#
# 2. Removes all executable variants
#    - Main executables for each build type
#    - Includes the unencrypted 'naked' release copy
#
# 3. Cleans up crash dumps
#    - Removes core dumps from crash handler
#    - Searches one directory up for broader cleanup
#    - Matches pattern: *.core.* (PID-specific dumps)
clean:
	@printf "$(CYAN)$(INFO)  Cleaning all build artifacts...$(NORMAL)\n"
	@rm -rf $(BUILD_DIR)/ $(DEBUG_DIR)/ $(VALGRIND_DIR)/ $(RELEASE_DIR)/ $(PERF_DIR)/
	@rm -rf $(TARGET) $(DEBUG_TARGET) $(VALGRIND_TARGET) $(RELEASE_TARGET) $(PERF_TARGET) $(BIN_PREFIX)hydrogen_naked
	@printf "$(CYAN)$(INFO)  Removing crash core dumps...$(NORMAL)\n"
	@find .. -name "*.core.*" -type f -delete
	@printf "$(GREEN)$(PASS) Clean completed successfully$(NORMAL)\n"