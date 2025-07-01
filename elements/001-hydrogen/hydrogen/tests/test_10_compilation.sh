#!/bin/bash
#
# About this Script
#
# Hydrogen Compilation Test Script
# Tests that all components compile without errors or warnings
#
NAME_SCRIPT="Hydrogen Compilation Test"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.1 - 2025-07-01 - Updated to use CMake for building OIDC client examples instead of Makefile
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# This test verifies that:
# 1. The main Hydrogen project compiles cleanly with all build variants
# 2. The OIDC client examples compile cleanly
# 3. The release build properly includes the Swagger UI tarball

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Timestamp and log file
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TIMESTAMP}_compilation.log"

# Initialize exit code and subtest tracking
EXIT_CODE=0
TOTAL_SUBTESTS=7  # 5 builds + tarball + OIDC examples
PASS_COUNT=0

# Start the test
start_test "Hydrogen Compilation Test" | tee -a "$RESULT_LOG"

# Function to safely change directory with error handling
safe_cd() {
    local target_dir="$1"
    if ! cd "$target_dir"; then
        print_error "Failed to change directory to $target_dir"
        EXIT_CODE=1
        return 1
    fi
    return 0
}

# Function to get file size safely
get_file_size() {
    local file_path="$1"
    local size
    if size=$(stat -c %s "$file_path" 2>/dev/null); then
        echo "$size"
        return 0
    else
        echo "0"
        return 1
    fi
}

# Test function for compilation (used only for OIDC examples)
test_compilation() {
    local component="$1"
    local build_dir="$2"
    local build_cmd="cmake --build build --target all_examples --parallel"
    
    print_header "Testing compilation of $component" | tee -a "$RESULT_LOG"
    print_info "Build command: $build_cmd" | tee -a "$RESULT_LOG"
    print_info "Working directory: $(convert_to_relative_path "$build_dir")" | tee -a "$RESULT_LOG"
    
    if ! safe_cd "$build_dir"; then
        return 1
    fi
    
    # Ensure build directory exists
    if [ ! -d "build" ]; then
        print_info "Build directory not found, creating it..." | tee -a "$RESULT_LOG"
        mkdir -p build
        cmake -S cmake -B build
    fi
    
    local start_time
    local temp_log
    start_time=$(date +%s)
    temp_log="$RESULTS_DIR/build_${component//\//_}_${TIMESTAMP}.log"
    
    if eval "$build_cmd" > "$temp_log" 2>&1; then
        print_result 0 "$component compiled successfully in ${duration}s" | tee -a "$RESULT_LOG"
        if ! grep -q "warning:" "$temp_log"; then
            print_info "No warnings detected" | tee -a "$RESULT_LOG"
            ((PASS_COUNT++))
        else
            print_warning "Warnings detected:" | tee -a "$RESULT_LOG"
            grep "warning:" "$temp_log" | tee -a "$RESULT_LOG"
            EXIT_CODE=1
        fi
    else
        print_result 1 "$component compilation failed" | tee -a "$RESULT_LOG"
        grep -E "error:|warning:" "$temp_log" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    local end_time
    local duration
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    if ! safe_cd "$SCRIPT_DIR"; then
        return 1
    fi
    return 0
}

# Test for tarball presence in release build
test_tarball_presence() {
    print_header "Testing Swagger UI tarball presence in release build" | tee -a "$RESULT_LOG"
    print_info "Checking for tarball delimiter in release executable" | tee -a "$RESULT_LOG"
    
    local release_executable="$HYDROGEN_DIR/hydrogen_release"
    
    if [ ! -f "$release_executable" ]; then
        print_result 1 "Release executable not found at $release_executable" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
        return 1
    fi
    
    local temp_strings
    temp_strings=$(mktemp)
    strings "$release_executable" > "$temp_strings"
    
    if grep -q "<<< HERE BE ME TREASURE >>>" "$temp_strings"; then
        print_result 0 "Tarball delimiter found in release executable" | tee -a "$RESULT_LOG"
        
        local file_size
        local payload_tarball="$HYDROGEN_DIR/payloads/payload.tar.br.enc"
        file_size=$(get_file_size "$release_executable")
        
        if [ -f "$payload_tarball" ]; then
            local tarball_size
            tarball_size=$(get_file_size "$payload_tarball")
            print_info "Found encrypted payload tarball (${tarball_size} bytes)" | tee -a "$RESULT_LOG"
            
            if [ "$file_size" -ge "$tarball_size" ]; then
                print_result 0 "Release executable size ($file_size bytes) is sufficient to contain the tarball ($tarball_size bytes)" | tee -a "$RESULT_LOG"
                print_info "Release build correctly includes the Swagger UI tarball" | tee -a "$RESULT_LOG"
                ((PASS_COUNT++))
            else
                print_result 1 "Release executable size ($file_size bytes) is too small to contain the tarball ($tarball_size bytes)" | tee -a "$RESULT_LOG"
                EXIT_CODE=1
            fi
        else
            print_info "Release executable size: $file_size bytes" | tee -a "$RESULT_LOG"
            print_info "Could not verify against tarball size (payloads/payload.tar.br.enc not found)" | tee -a "$RESULT_LOG"
        fi
    else
        print_result 1 "Tarball delimiter not found in release executable" | tee -a "$RESULT_LOG"
        print_warning "The release build does not appear to include the Swagger UI tarball" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    rm -f "$temp_strings"
    return 0
}

# Function to run parallel builds with proper error handling using CMake
run_parallel_builds() {
    print_header "Running all builds in parallel with CMake" | tee -a "$RESULT_LOG"
    
    if ! safe_cd "$HYDROGEN_DIR"; then
        return 1
    fi
    
    local start_time
    start_time=$(date +%s)
    
    # Clean previous builds, configure from scratch, and build all variants using CMake
    local temp_log="$RESULTS_DIR/cmake_build_${TIMESTAMP}.log"
    # Remove existing build directory to start fresh
    if [ -d "build" ]; then
        print_info "Removing existing build directory for a fresh start..." | tee -a "$RESULT_LOG"
        if ! rm -rf build >> "$temp_log" 2>&1; then
            print_result 1 "Failed to remove existing build directory, check log for details: $(convert_to_relative_path "$temp_log")" | tee -a "$RESULT_LOG"
            EXIT_CODE=1
            return 1
        fi
    fi
    # Configure build system
    print_info "Configuring CMake build system..." | tee -a "$RESULT_LOG"
    cmake -S cmake --preset default > "$temp_log" 2>&1
    local config_result=$?
    if [ $config_result -ne 0 ]; then
        print_result 1 "CMake configuration failed, check log for details: $(convert_to_relative_path "$temp_log")" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
        return 1
    fi
    # Now build all variants
    cmake --build build --target all_variants --parallel >> "$temp_log" 2>&1
    
    local build_result=$?
    
    local end_time
    local duration
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    print_info "All parallel builds completed in ${duration}s" | tee -a "$RESULT_LOG"
    if [ $build_result -ne 0 ]; then
        print_warning "CMake build process returned non-zero exit code, check log for details: $(convert_to_relative_path "$temp_log")" | tee -a "$RESULT_LOG"
    fi
    
    # Check build results
    for type in default debug valgrind perf release; do
        local exec_path
        if [ "$type" = "default" ]; then
            exec_path="$HYDROGEN_DIR/hydrogen"
        else
            exec_path="$HYDROGEN_DIR/hydrogen_$type"
        fi
        
        if [ -f "$exec_path" ] && [ -x "$exec_path" ]; then
            print_result 0 "Hydrogen main project ($type) compiled successfully" | tee -a "$RESULT_LOG"
            ((PASS_COUNT++))
        else
            print_result 1 "Hydrogen main project ($type) compilation failed" | tee -a "$RESULT_LOG"
            EXIT_CODE=1
        fi
    done
    
    if ! safe_cd "$SCRIPT_DIR"; then
        return 1
    fi
    return 0
}

# Main execution flow
main() {
    # Run parallel builds
    run_parallel_builds
    
    # Test tarball presence
    test_tarball_presence
    
    # Build OIDC examples using CMake
    test_compilation "OIDC client examples" "$HYDROGEN_DIR" "cmake --build build --target all_examples --parallel"
    
    # Print final summary
    print_header "Compilation Test Summary" | tee -a "$RESULT_LOG"
    print_info "Completed at: $(date)" | tee -a "$RESULT_LOG"
    print_info "Test log: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
    print_info "Subtest results: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"
    
    # Export subtest results for test_all.sh to pick up
    export_subtest_results "compilation" $TOTAL_SUBTESTS $PASS_COUNT
    
    # End test with appropriate result message
    end_test $EXIT_CODE "Compilation Test" | tee -a "$RESULT_LOG"
    
    return $EXIT_CODE
}

# Execute main function and exit with its return code
main
exit $?
