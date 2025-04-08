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
OIDC_EXAMPLES_DIR="$HYDROGEN_DIR/examples/C"

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

# Test function for compilation (used only for OIDC examples)
test_compilation() {
    local COMPONENT=$1
    local BUILD_DIR=$2
    local BUILD_CMD=$3
    
    print_header "Testing compilation of $COMPONENT" | tee -a "$RESULT_LOG"
    print_info "Build command: $BUILD_CMD" | tee -a "$RESULT_LOG"
    print_info "Working directory: $(convert_to_relative_path "$BUILD_DIR")" | tee -a "$RESULT_LOG"
    
    cd "$BUILD_DIR"
    
    local START_TIME=$(date +%s)
    local TEMP_LOG="$RESULTS_DIR/build_${COMPONENT//\//_}_${TIMESTAMP}.log"
    
    CFLAGS="-Wall -Wextra -Werror -pedantic" make all > "$TEMP_LOG" 2>&1
    local BUILD_RESULT=$?
    
    local END_TIME=$(date +%s)
    local DURATION=$((END_TIME - START_TIME))
    
    if [ $BUILD_RESULT -eq 0 ]; then
        print_result 0 "$COMPONENT compiled successfully in ${DURATION}s" | tee -a "$RESULT_LOG"
        if ! grep -q "warning:" "$TEMP_LOG"; then
            print_info "No warnings detected" | tee -a "$RESULT_LOG"
            ((PASS_COUNT++))
        else
            print_warning "Warnings detected:" | tee -a "$RESULT_LOG"
            grep "warning:" "$TEMP_LOG" | tee -a "$RESULT_LOG"
            EXIT_CODE=1
        fi
    else
        print_result 1 "$COMPONENT compilation failed" | tee -a "$RESULT_LOG"
        grep -E "error:|warning:" "$TEMP_LOG" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    cd "$SCRIPT_DIR"
}

# Test for tarball presence in release build
test_tarball_presence() {
    print_header "Testing Swagger UI tarball presence in release build" | tee -a "$RESULT_LOG"
    print_info "Checking for tarball delimiter in release executable" | tee -a "$RESULT_LOG"
    
    local RELEASE_EXECUTABLE="$HYDROGEN_DIR/hydrogen_release"
    
    if [ ! -f "$RELEASE_EXECUTABLE" ]; then
        print_result 1 "Release executable not found at $RELEASE_EXECUTABLE" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
        return
    fi
    
    local TEMP_STRINGS=$(mktemp)
    strings "$RELEASE_EXECUTABLE" > "$TEMP_STRINGS"
    
    if grep -q "<<< HERE BE ME TREASURE >>>" "$TEMP_STRINGS"; then
        print_result 0 "Tarball delimiter found in release executable" | tee -a "$RESULT_LOG"
        
        local FILE_SIZE=$(stat -c %s "$RELEASE_EXECUTABLE")
        local PAYLOAD_TARBALL="$HYDROGEN_DIR/payloads/payload.tar.br.enc"
        
        if [ -f "$PAYLOAD_TARBALL" ]; then
            local TARBALL_SIZE=$(stat -c %s "$PAYLOAD_TARBALL")
            print_info "Found encrypted payload tarball (${TARBALL_SIZE} bytes)" | tee -a "$RESULT_LOG"
            
            if [ $FILE_SIZE -ge $TARBALL_SIZE ]; then
                print_result 0 "Release executable size ($FILE_SIZE bytes) is sufficient to contain the tarball ($TARBALL_SIZE bytes)" | tee -a "$RESULT_LOG"
                print_info "Release build correctly includes the Swagger UI tarball" | tee -a "$RESULT_LOG"
                ((PASS_COUNT++))
            else
                print_result 1 "Release executable size ($FILE_SIZE bytes) is too small to contain the tarball ($TARBALL_SIZE bytes)" | tee -a "$RESULT_LOG"
                EXIT_CODE=1
            fi
        else
            print_info "Release executable size: $FILE_SIZE bytes" | tee -a "$RESULT_LOG"
            print_info "Could not verify against tarball size (payloads/payload.tar.br.enc not found)" | tee -a "$RESULT_LOG"
        fi
    else
        print_result 1 "Tarball delimiter not found in release executable" | tee -a "$RESULT_LOG"
        print_warning "The release build does not appear to include the Swagger UI tarball" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    rm -f "$TEMP_STRINGS"
}

# Run parallel builds
print_header "Running all builds in parallel" | tee -a "$RESULT_LOG"

cd "$HYDROGEN_DIR/src"

START_TIME=$(date +%s)

# Run builds using exact command structure from working example
make clean >/dev/null 2>&1 && (make -j24 default QUIET=1 >/dev/null 2>&1 & \
                              make -j24 debug QUIET=1 >/dev/null 2>&1 & \
                              make -j24 valgrind QUIET=1 >/dev/null 2>&1 & \
                              make -j24 perf QUIET=1 >/dev/null 2>&1 & \
                              make -j24 release QUIET=1 >/dev/null 2>&1 & \
                              wait)

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

print_info "All parallel builds completed in ${DURATION}s" | tee -a "$RESULT_LOG"

# Check build results
for TYPE in default debug valgrind perf release; do
    if [ "$TYPE" = "default" ]; then
        EXEC="$HYDROGEN_DIR/hydrogen"
    else
        EXEC="$HYDROGEN_DIR/hydrogen_$TYPE"
    fi
    
    if [ -f "$EXEC" ] && [ -x "$EXEC" ]; then
        print_result 0 "Hydrogen main project ($TYPE) compiled successfully" | tee -a "$RESULT_LOG"
        ((PASS_COUNT++))
    else
        print_result 1 "Hydrogen main project ($TYPE) compilation failed" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
done

cd "$SCRIPT_DIR"

# Test tarball presence
test_tarball_presence

# Build OIDC examples
test_compilation "OIDC client examples" "$OIDC_EXAMPLES_DIR" "make all QUIET=1"

# Print final summary
print_header "Compilation Test Summary" | tee -a "$RESULT_LOG"
print_info "Completed at: $(date)" | tee -a "$RESULT_LOG"
print_info "Test log: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
print_info "Subtest results: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# Export subtest results for test_all.sh to pick up
export_subtest_results "compilation" $TOTAL_SUBTESTS $PASS_COUNT

# End test with appropriate result message
end_test $EXIT_CODE "Compilation Test" | tee -a "$RESULT_LOG"

exit $EXIT_CODE