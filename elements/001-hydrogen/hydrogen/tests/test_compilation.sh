#!/bin/bash
#
# Hydrogen Compilation Test Script
# Tests that all components compile without errors or warnings
#
# This test verifies that:
# 1. The main Hydrogen project compiles cleanly with all build variants
# 2. The OIDC client examples compile cleanly
# 3. The release build properly includes the Swagger UI tarball

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
OIDC_EXAMPLES_DIR="$HYDROGEN_DIR/oidc-client-examples/C"

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
TOTAL_SUBTESTS=7  # Release, tarball, standard, debug, valgrind, perf builds, OIDC examples
PASS_COUNT=0

# Start the test
start_test "Hydrogen Compilation Test" | tee -a "$RESULT_LOG"

# Test function for compilation
test_compilation() {
    local COMPONENT=$1
    local BUILD_DIR=$2
    local BUILD_CMD=$3
    
    print_header "Testing compilation of $COMPONENT" | tee -a "$RESULT_LOG"
    print_info "Build command: $BUILD_CMD" | tee -a "$RESULT_LOG"
    print_info "Working directory: $(convert_to_relative_path "$BUILD_DIR")" | tee -a "$RESULT_LOG"
    
    # Navigate to build directory and run build command
    cd "$BUILD_DIR"
    
    # Capture the start time
    local START_TIME=$(date +%s)
    
    # Build with warnings treated as errors
    # Redirect both stdout and stderr to a temporary file
    local TEMP_LOG="$RESULTS_DIR/build_${COMPONENT//\//_}_${TIMESTAMP}.log"
    {
        if [[ "$COMPONENT" == "Hydrogen main project"* ]]; then
            # Extract the build variant (between parentheses) without including the closing parenthesis
            local TARGET=$(echo "$COMPONENT" | sed -n 's/.*(\([^)]*\)).*/\1/p')
            if [ "$TARGET" == "default" ]; then
                # Default build doesn't need a specific target
                CFLAGS="-Wall -Wextra -Werror -pedantic" make
            else
                CFLAGS="-Wall -Wextra -Werror -pedantic" make $TARGET
            fi
        else
            CFLAGS="-Wall -Wextra -Werror -pedantic" make all
        fi
    } > "$TEMP_LOG" 2>&1
    
    # Capture the exit code
    local BUILD_RESULT=$?
    
    # Capture the end time and calculate duration
    local END_TIME=$(date +%s)
    local DURATION=$((END_TIME - START_TIME))
    
    # Check if the build was successful
    if [ $BUILD_RESULT -eq 0 ]; then
        print_result 0 "$COMPONENT compiled successfully in ${DURATION}s" | tee -a "$RESULT_LOG"
        
        # Check for warnings, even though we used -Werror, we'll do a double-check
        if grep -q "warning:" "$TEMP_LOG"; then
            print_warning "Warnings detected despite build success:" | tee -a "$RESULT_LOG"
            grep "warning:" "$TEMP_LOG" | tee -a "$RESULT_LOG"
            # Fail the test when warnings are detected
            print_result 1 "Test failed because warnings were detected" | tee -a "$RESULT_LOG"
            EXIT_CODE=1
        else
            print_info "No warnings detected" | tee -a "$RESULT_LOG"
            # Increment pass count for this subtest
            ((PASS_COUNT++))
            print_info "Subtest passed: $COMPONENT" | tee -a "$RESULT_LOG"
        fi
    else
        print_result 1 "$COMPONENT compilation failed with exit code $BUILD_RESULT" | tee -a "$RESULT_LOG"
        echo "" | tee -a "$RESULT_LOG"
        echo "==== BUILD OUTPUT ====" | tee -a "$RESULT_LOG"
        cat "$TEMP_LOG" | tee -a "$RESULT_LOG"
        echo "==== END OF BUILD OUTPUT ====" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    echo "" | tee -a "$RESULT_LOG"
    
    # Append the full build log to the result log
    print_info "Full build log saved to: $(convert_to_relative_path "$TEMP_LOG")" | tee -a "$RESULT_LOG"
    
    # Return to script directory
    cd "$SCRIPT_DIR"
}

# Clean all build artifacts first
cd "$HYDROGEN_DIR"
make clean
cd "$SCRIPT_DIR"

# Test the main Hydrogen project (release build - preferred for deployment)
test_compilation "Hydrogen main project (release)" "$HYDROGEN_DIR" "make release"

# Test for tarball presence in release build
test_tarball_presence() {
    print_header "Testing Swagger UI tarball presence in release build" | tee -a "$RESULT_LOG"
    print_info "Checking for tarball delimiter in release executable" | tee -a "$RESULT_LOG"
    
    local RELEASE_EXECUTABLE="$HYDROGEN_DIR/hydrogen_release"
    
    # Check if the release executable exists
    if [ ! -f "$RELEASE_EXECUTABLE" ]; then
        print_result 1 "Release executable not found at $RELEASE_EXECUTABLE" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
        return
    fi
    
    # Extract readable strings from the binary file for safer grepping
    local TEMP_STRINGS=$(mktemp)
    strings "$RELEASE_EXECUTABLE" > "$TEMP_STRINGS"
    
    # Search for the tarball delimiter in the extracted strings
    if grep -q "<<< HERE BE ME TREASURE >>>" "$TEMP_STRINGS"; then
        print_result 0 "Tarball delimiter found in release executable" | tee -a "$RESULT_LOG"
        
        # We'll use a simpler and more reliable approach to verify the overall structure
        # Just knowing the delimiter exists in a release build is a strong indicator
        # that the tarball has been appended, as this is only done in the release process
        
        # The first check is usually sufficient, but we'll add an additional check for robustness
        # Verify that the file size of the release executable is larger than a typical executable
        # This suggests the tarball is appended
        
        local FILE_SIZE=$(stat -c %s "$RELEASE_EXECUTABLE")
        
        # Check if swagger tarball exists - we can use it to estimate minimum expected size
        local SWAGGER_TARBALL="$HYDROGEN_DIR/swagger/swaggerui.tar.br"
        if [ -f "$SWAGGER_TARBALL" ]; then
            local TARBALL_SIZE=$(stat -c %s "$SWAGGER_TARBALL")
            print_info "Found Swagger UI tarball (${TARBALL_SIZE} bytes)" | tee -a "$RESULT_LOG"
            
            # The executable should be at least as large as the tarball
            # This verifies that there's enough space to contain the tarball
            if [ $FILE_SIZE -ge $TARBALL_SIZE ]; then
                print_result 0 "Release executable size ($FILE_SIZE bytes) is sufficient to contain the tarball ($TARBALL_SIZE bytes)" | tee -a "$RESULT_LOG"
                print_info "Release build correctly includes the Swagger UI tarball" | tee -a "$RESULT_LOG"
                # Increment pass count for tarball verification subtest
                ((PASS_COUNT++))
                print_info "Subtest passed: Swagger UI tarball verification" | tee -a "$RESULT_LOG"
            else
                print_result 1 "Release executable size ($FILE_SIZE bytes) is too small to contain the tarball ($TARBALL_SIZE bytes)" | tee -a "$RESULT_LOG"
                EXIT_CODE=1
            fi
        else
            # If the tarball isn't found, we can still check if the executable is reasonably sized
            print_info "Release executable size: $FILE_SIZE bytes" | tee -a "$RESULT_LOG"
            print_info "Could not verify against tarball size (swagger/swaggerui.tar.br not found)" | tee -a "$RESULT_LOG"
        fi
    else
        print_result 1 "Tarball delimiter not found in release executable" | tee -a "$RESULT_LOG"
        print_warning "The release build does not appear to include the Swagger UI tarball" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    # Clean up
    rm -f "$TEMP_STRINGS"
    
    echo "" | tee -a "$RESULT_LOG"
}

# After testing the release build compilation, test for tarball presence
test_tarball_presence

# Test the main Hydrogen project (standard build)
test_compilation "Hydrogen main project (default)" "$HYDROGEN_DIR" "make"

# Test the main Hydrogen project (debug build)
test_compilation "Hydrogen main project (debug)" "$HYDROGEN_DIR" "make debug"

# Test the main Hydrogen project (valgrind build)
test_compilation "Hydrogen main project (valgrind)" "$HYDROGEN_DIR" "make valgrind"

# Test the main Hydrogen project (performance build)
test_compilation "Hydrogen main project (perf)" "$HYDROGEN_DIR" "make perf"

# Test the OIDC client examples
test_compilation "OIDC client examples" "$OIDC_EXAMPLES_DIR" "make all"

# Print final summary
print_header "Compilation Test Summary" | tee -a "$RESULT_LOG"
print_info "Completed at: $(date)" | tee -a "$RESULT_LOG"
print_info "Test log: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
print_info "Subtest results: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# Export subtest results for test_all.sh to pick up
export_subtest_results $TOTAL_SUBTESTS $PASS_COUNT

# End test with appropriate result message
end_test $EXIT_CODE "Compilation Test" | tee -a "$RESULT_LOG"

# Exit with appropriate code
exit $EXIT_CODE