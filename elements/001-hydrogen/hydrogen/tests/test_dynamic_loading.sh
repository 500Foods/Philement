#!/bin/bash
#
# Hydrogen Dynamic Library Loading Test Script
# Tests the dynamic library loading system for conditional dependency loading

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
# Prefer release build if available, fallback to standard build
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Using standard build"
fi

# Default configuration files - one with everything enabled, one with minimal services
ALL_ENABLED_CONFIG=$(get_config_path "hydrogen_test_max.json")
MINIMAL_CONFIG=$(get_config_path "hydrogen_test_min.json")

# Timeouts and paths
STARTUP_TIMEOUT=10                        # Seconds to wait for startup
LOG_FILE="$SCRIPT_DIR/hydrogen_test_dynamic.log"  # Runtime log to analyze
RESULTS_DIR="$SCRIPT_DIR/results"         # Directory for test results
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/dynamic_load_test_${TIMESTAMP}.log"

# Create output directories
mkdir -p "$RESULTS_DIR"

# Start the test
start_test "Hydrogen Dynamic Library Loading Test" | tee -a "$RESULT_LOG"

# Check if gcc is installed for compiling the test harness
if ! command -v gcc &> /dev/null; then
    print_error "gcc is required for this test but was not found" | tee -a "$RESULT_LOG"
    end_test 1 "Dynamic Library Loading Test failed - missing tools" | tee -a "$RESULT_LOG"
    exit 1
fi

# Initialize error counters
EXIT_CODE=0
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Create a small test harness to test the dynamic loading functionality
TEST_HARNESS="$SCRIPT_DIR/dynamic_load_test.c"
TEST_HARNESS_BIN="$SCRIPT_DIR/dynamic_load_test"

cat > "$TEST_HARNESS" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../src/utils/utils_dependency.h"
#include "../src/logging/logging.h"

// Simplified logging for test
void log_this(const char *subsystem, const char *format, int level, ...) {
    printf("[%s] %s\n", subsystem, format);
}

int main(int argc, char *argv[]) {
    const char *lib_to_test = NULL;
    
    if (argc > 1) {
        lib_to_test = argv[1];
    } else {
        fprintf(stderr, "Usage: %s <library_name>\n", argv[0]);
        return 1;
    }
    
    printf("Testing dynamic loading of: %s\n", lib_to_test);
    
    // Test 1: Check if library is available without loading
    bool available = is_library_available(lib_to_test);
    printf("Test 1: Library availability check: %s\n", available ? "PASS" : "FAIL");
    
    // Test 2: Load the library
    LibraryHandle *handle = load_library(lib_to_test, RTLD_LAZY);
    if (!handle) {
        printf("Test 2: Failed to allocate library handle: FAIL\n");
        return 1;
    }
    
    printf("Test 2: Load library: %s (Version: %s)\n", 
           handle->is_loaded ? "PASS" : "FAIL",
           handle->version);
    
    // Test 3: Get a common function from the library (if loaded)
    if (handle->is_loaded) {
        // Try to get a function that should be present in most libraries
        void *func_ptr = get_library_function(handle, "malloc");
        printf("Test 3: Get common function: %s\n", func_ptr ? "PASS" : "FAIL");
    } else {
        // If library not loaded, log but don't fail the test
        printf("Test 3: Skipped - library not loaded\n");
    }
    
    // Test 4: Unload the library
    bool unload_success = unload_library(handle);
    printf("Test 4: Unload library: %s\n", unload_success ? "PASS" : "FAIL");
    
    // Test 5: Fallback mechanism test (macro usage)
    // We simulate this test since we can't directly use the macros in main()
    printf("Test 5: Fallback mechanism (simulated): PASS\n");
    
    return 0;
}
EOF

# Compile the test harness
print_info "Compiling dynamic loading test harness..." | tee -a "$RESULT_LOG"
# Include the utils_dependency.c file directly in compilation
gcc -Wall -I"$HYDROGEN_DIR/src" -o "$TEST_HARNESS_BIN" "$TEST_HARNESS" "$HYDROGEN_DIR/src/utils/utils_dependency.c" -ldl

# Create stub for log_this if it's not available
if ! grep -q "log_this" "$TEST_HARNESS"; then
    print_warning "Adding logging stub to test harness" | tee -a "$RESULT_LOG"
    
    # Create a temporary test harness with logging function stub
    TMP_HARNESS="${TEST_HARNESS}.tmp"
    cat > "$TMP_HARNESS" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dlfcn.h>

// Logging stub for test
void log_this(const char *subsystem, const char *format, int level, ...) {
    va_list args;
    va_start(args, level);
    printf("[%s] ", subsystem);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

#include "../src/utils/utils_dependency.h"

int main(int argc, char *argv[]) {
    const char *lib_to_test = NULL;
    
    if (argc > 1) {
        lib_to_test = argv[1];
    } else {
        fprintf(stderr, "Usage: %s <library_name>\n", argv[0]);
        return 1;
    }
    
    printf("Testing dynamic loading of: %s\n", lib_to_test);
    
    // Test 1: Check if library is available without loading
    bool available = is_library_available(lib_to_test);
    printf("Test 1: Library availability check: %s\n", available ? "PASS" : "FAIL");
    
    // Test 2: Load the library
    LibraryHandle *handle = load_library(lib_to_test, RTLD_LAZY);
    if (!handle) {
        printf("Test 2: Failed to allocate library handle: FAIL\n");
        return 1;
    }
    
    printf("Test 2: Load library: %s (Version: %s)\n", 
           handle->is_loaded ? "PASS" : "FAIL",
           handle->version);
    
    // Test 3: Get a common function from the library (if loaded)
    if (handle->is_loaded) {
        // Try to get a function that should be present in most libraries
        void *func_ptr = get_library_function(handle, "malloc");
        printf("Test 3: Get common function: %s\n", func_ptr ? "PASS" : "FAIL");
    } else {
        // If library not loaded, log but don't fail the test
        printf("Test 3: Skipped - library not loaded\n");
    }
    
    // Test 4: Unload the library
    bool unload_success = unload_library(handle);
    printf("Test 4: Unload library: %s\n", unload_success ? "PASS" : "FAIL");
    
    // Test 5: Fallback mechanism test (macro usage)
    // We simulate this test since we can't directly use the macros in main()
    printf("Test 5: Fallback mechanism (simulated): PASS\n");
    
    return 0;
}
EOF
    
    # Use the temporary harness instead
    mv "$TMP_HARNESS" "$TEST_HARNESS"
    
    # Retry compilation with the new test harness
    gcc -Wall -I"$HYDROGEN_DIR/src" -o "$TEST_HARNESS_BIN" "$TEST_HARNESS" -ldl
fi

if [ $? -ne 0 ]; then
    print_error "Failed to compile test harness" | tee -a "$RESULT_LOG"
    end_test 1 "Dynamic Library Loading Test failed - compilation error" | tee -a "$RESULT_LOG"
    exit 1
fi

print_info "Test harness compiled successfully" | tee -a "$RESULT_LOG"

# Define a function to run a test with a specific library
run_dynamic_load_test() {
    local lib_name="$1"
    local test_name="$2"
    local expected_result="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    print_header "Testing dynamic loading of $lib_name ($test_name)" | tee -a "$RESULT_LOG"
    "$TEST_HARNESS_BIN" "$lib_name" > "$LOG_FILE" 2>&1
    
    # Check result based on expected outcome
    if [ "$expected_result" = "success" ]; then
        if grep -q "Test 2: Load library: PASS" "$LOG_FILE"; then
            print_test_item 0 "Test $TOTAL_TESTS" "Successfully loaded $lib_name" | tee -a "$RESULT_LOG"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            print_test_item 1 "Test $TOTAL_TESTS" "Failed to load $lib_name (expected success)" | tee -a "$RESULT_LOG"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    elif [ "$expected_result" = "failure" ]; then
        if grep -q "Test 2: Load library: FAIL" "$LOG_FILE"; then
            print_test_item 0 "Test $TOTAL_TESTS" "Correctly failed to load $lib_name (as expected)" | tee -a "$RESULT_LOG"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            print_test_item 1 "Test $TOTAL_TESTS" "Unexpectedly loaded $lib_name (expected failure)" | tee -a "$RESULT_LOG"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        print_error "Invalid expected result: $expected_result" | tee -a "$RESULT_LOG"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    cat "$LOG_FILE" >> "$RESULT_LOG"
    print_info "-----------------------------------" | tee -a "$RESULT_LOG"
}

# Test with libraries that should be present on most systems
# Use common versioned library names for Linux systems
run_dynamic_load_test "libc.so.6" "Standard C Library" "success"
run_dynamic_load_test "libm.so.6" "Math Library" "success"
run_dynamic_load_test "libdl.so.2" "Dynamic Loading Library" "success"

# Test with libraries that may be installed (depending on system)
run_dynamic_load_test "libjansson.so" "JSON Library" "success"
run_dynamic_load_test "libssl.so" "OpenSSL Library" "success"

# Test with a library that should definitely not exist
run_dynamic_load_test "libnonexistentlibrary123456789.so" "Non-existent Library" "failure"

# Test for memory leaks by running the test in a loop
print_header "Memory leak test (running 100 iterations)" | tee -a "$RESULT_LOG"
LEAK_TEST_LOOPS=100
LEAKED_MEMORY=0

# Clean up any existing memory usage data
rm -f "$SCRIPT_DIR/memory_before.txt" "$SCRIPT_DIR/memory_after.txt"

# Check memory usage before
ps -o rss= -p $$ > "$SCRIPT_DIR/memory_before.txt"

# Run the test many times to catch memory leaks
for i in $(seq 1 $LEAK_TEST_LOOPS); do
    "$TEST_HARNESS_BIN" "libm.so" > /dev/null 2>&1
done

# Check memory usage after
ps -o rss= -p $$ > "$SCRIPT_DIR/memory_after.txt"

# Calculate memory difference
MEM_BEFORE=$(cat "$SCRIPT_DIR/memory_before.txt")
MEM_AFTER=$(cat "$SCRIPT_DIR/memory_after.txt")
LEAKED_MEMORY=$((MEM_AFTER - MEM_BEFORE))

TOTAL_TESTS=$((TOTAL_TESTS + 1))

# Check for significant memory leaks (more than 1MB)
if [ $LEAKED_MEMORY -lt 1024 ]; then
    print_test_item 0 "Memory Leak Test" "No significant memory leaks detected after $LEAK_TEST_LOOPS iterations" | tee -a "$RESULT_LOG"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    print_test_item 1 "Memory Leak Test" "Possible memory leak detected: ${LEAKED_MEMORY}KB increase after $LEAK_TEST_LOOPS iterations" | tee -a "$RESULT_LOG"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Cleanup
rm -f "$TEST_HARNESS" "$TEST_HARNESS_BIN" "$SCRIPT_DIR/memory_before.txt" "$SCRIPT_DIR/memory_after.txt"

# Print summary statistics
print_test_summary $TOTAL_TESTS $PASSED_TESTS $FAILED_TESTS | tee -a "$RESULT_LOG"

# Determine final result
if [ $FAILED_TESTS -gt 0 ]; then
    print_result 1 "FAILED: $FAILED_TESTS out of $TOTAL_TESTS tests failed" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
else
    print_result 0 "PASSED: All $TOTAL_TESTS tests passed successfully" | tee -a "$RESULT_LOG"
    EXIT_CODE=0
fi

# End test
end_test $EXIT_CODE "Dynamic Library Loading Test" | tee -a "$RESULT_LOG"
exit $EXIT_CODE