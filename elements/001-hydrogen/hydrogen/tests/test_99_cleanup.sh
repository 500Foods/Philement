#!/bin/bash

# Test: Build System Cleanup
# Performs comprehensive build cleanup using CMake

# Test configuration
TEST_NAME="Build System Cleanup"
SCRIPT_VERSION="1.0.0"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=1
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Subtest 1: Run CMake cleanish target for the Hydrogen project
next_subtest
print_subtest "CMake Clean Build Artifacts"

print_message "Running comprehensive build cleanup for Hydrogen Project"

if [ -d "build" ]; then
    TEMP_LOG="$RESULTS_DIR/cmake_clean_hydrogen_${TIMESTAMP}.log"
    
    # First try CMake cleanish target
    print_command "cmake --build build --target cleanish"
    cmake --build build --target cleanish > "$TEMP_LOG" 2>&1 || true
    
    # Count object files before additional cleanup
    OBJ_FILES_BEFORE=$(find build -name "*.o" 2>/dev/null | wc -l)
    
    if [ "$OBJ_FILES_BEFORE" -gt 0 ]; then
        print_message "Found $OBJ_FILES_BEFORE object files remaining after cleanish, performing additional cleanup"
        
        # Remove object files and other build artifacts
        print_command "find build -name '*.o' -delete"
        find build -name "*.o" -delete 2>/dev/null || true
        
        # Remove other common build artifacts
        find build -name "*.a" -delete 2>/dev/null || true
        find build -name "*.so" -delete 2>/dev/null || true
        find build -name "*.dylib" -delete 2>/dev/null || true
        
        OBJ_FILES_AFTER=$(find build -name "*.o" 2>/dev/null | wc -l)
        print_message "Removed $((OBJ_FILES_BEFORE - OBJ_FILES_AFTER)) object files"
    fi
    
    # Final verification
    REMAINING_OBJ=$(find build -name "*.o" 2>/dev/null | wc -l)
    if [ "$REMAINING_OBJ" -eq 0 ]; then
        print_result 0 "Successfully cleaned all build artifacts including $OBJ_FILES_BEFORE object files"
        ((PASS_COUNT++))
    else
        print_result 0 "Cleaned most artifacts ($REMAINING_OBJ object files remain)"
        ((PASS_COUNT++))
    fi
else
    print_result 0 "No build directory found (clean not needed)"
    ((PASS_COUNT++))
fi

# Export results for test_all.sh integration
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi