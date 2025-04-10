#!/bin/bash
#
# Crash Handler Test
# Tests that the crash handler correctly generates and formats core dumps
# - Verifies core file creation
# - Validates core file format
# - Checks GDB compatibility
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
CONFIG_FILE=$(get_config_path "hydrogen_test_min.json")
if [ ! -f "$CONFIG_FILE" ]; then
    print_result 1 "Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Function to verify core dump configuration
verify_core_dump_config() {
    print_info "Checking core dump configuration..."
    
    # Check core pattern
    local core_pattern=$(cat /proc/sys/kernel/core_pattern)
    if [[ "$core_pattern" == "core" ]]; then
        print_result 0 "Core pattern is set to default 'core'"
    else
        print_warning "Core pattern is set to '$core_pattern', might affect core dump location"
    fi
    
    # Check core dump size limit
    local core_limit=$(ulimit -c)
    if [[ "$core_limit" == "unlimited" || "$core_limit" -gt 0 ]]; then
        print_result 0 "Core dump size limit is adequate ($core_limit)"
    else
        print_result 1 "Core dump size limit is too restrictive ($core_limit)"
        # Try to set unlimited core dumps for this session
        ulimit -c unlimited
        print_info "Attempted to set unlimited core dump size"
    fi
    
    # Only warn if core_pattern is not "core"
    if [[ "$core_pattern" != "core" ]]; then
        print_warning "Core pattern is not set to 'core', this may affect core dump location"
    fi
    
    # Only warn if core limit is too low
    if [[ "$core_limit" != "unlimited" && "$core_limit" -eq 0 ]]; then
        print_warning "Core dump size limit is too restrictive ($core_limit)"
        print_warning "You may need to run: ulimit -c unlimited"
    fi
    
    # Return success as long as we can proceed
    return 0
}

# Function to verify core file content
verify_core_file_content() {
    local core_file="$1"
    local binary="$2"
    
    print_info "Verifying core file contents..."
    
    # Check file size and verify it's a core file
    local core_size=$(stat -c %s "$core_file")
    if [ $core_size -lt 1024 ]; then
        print_result 1 "Core file is suspiciously small ($core_size bytes)"
        return 1
    fi
    
    # Verify it's a valid core file
    if readelf -h "$core_file" | grep -q "Core file"; then
        print_result 0 "Valid core file found"
        return 0
    else
        print_result 1 "Not a valid core file"
        return 1
    fi
}

# Function to verify debug symbols are present
verify_debug_symbols() {
    local binary="$1"
    local binary_name=$(basename "$binary")
    local expect_symbols=1
    
    # Release builds should not have debug symbols
    if [[ "$binary_name" == *"release"* ]]; then
        expect_symbols=0
    fi
    
    print_info "Checking debug symbols in $binary_name..."
    
    # Use readelf to check for debug symbols
    if readelf --debug-dump=info "$binary" 2>/dev/null | grep -q "DW_AT_name"; then
        if [ $expect_symbols -eq 1 ]; then
            print_result 0 "Debug symbols found in $binary_name (as expected)"
            return 0
        else
            print_result 1 "Debug symbols found in $binary_name (unexpected for release build)"
            return 1
        fi
    else
        if [ $expect_symbols -eq 1 ]; then
            print_result 1 "No debug symbols found in $binary_name (symbols required)"
            return 1
        else
            print_result 0 "No debug symbols found in $binary_name (as expected for release build)"
            return 0
        fi
    fi
}

# Function to verify core file exists and has correct name format
verify_core_file() {
    local binary="$1"
    local pid="$2"
    local binary_name=$(basename "$binary")
    local expected_core="${HYDROGEN_DIR}/${binary_name}.core.${pid}"
    local timeout=5
    local found=0
    
    print_info "Waiting for core file ${binary_name}.core.${pid}..."
    
    # Wait up to 5 seconds for core file to appear
    for ((i=1; i<=timeout; i++)); do
        if [ -f "${expected_core}" ]; then
            found=1
            break
        fi
        sleep 1
    done
    
    if [ $found -eq 1 ]; then
        print_result 0 "Core file ${binary_name}.core.${pid} found"
        return 0
    else
        print_result 1 "Core file ${binary_name}.core.${pid} not found after ${timeout} seconds"
        ls -l "${HYDROGEN_DIR}"/*.core.* 2>/dev/null || echo "No core files found"
        return 1
    fi
}

# Function to run test with a specific build
run_test_with_build() {
    local binary="$1"
    local binary_name=$(basename "$binary")
    
    print_info "Testing with build: $binary_name"
    
    # Start hydrogen server in background with ASAN leak detection disabled
    print_info "Starting hydrogen server (with leak detection disabled)..."
    ASAN_OPTIONS="detect_leaks=0" $binary "$CONFIG_FILE" > "$SCRIPT_DIR/hydrogen_crash_test.log" 2>&1 &
    HYDROGEN_PID=$!
    
    # Wait for startup with active log monitoring
    print_info "Waiting for startup (max 10s)..."
    STARTUP_START=$(date +%s)
    STARTUP_TIMEOUT=10
    STARTUP_COMPLETE=false
    
    while true; do
        # Check if we've exceeded the timeout
        CURRENT_TIME=$(date +%s)
        ELAPSED=$((CURRENT_TIME - STARTUP_START))
        
        if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
            print_result 1 "Startup timeout after ${ELAPSED}s"
            return 1
        fi
        
        # Check if process is still running
        if ! kill -0 $HYDROGEN_PID 2>/dev/null; then
            # This is an expected part of the crash handler test
            print_warning "Server crashed during startup (expected behavior)"
            STARTUP_COMPLETE=true
            break
        fi
        
        # Check for startup completion message
        if grep -q "Application started" "$SCRIPT_DIR/hydrogen_crash_test.log"; then
            STARTUP_COMPLETE=true
            print_info "Startup completed in ${ELAPSED}s"
            break
        fi
        
        sleep 0.2
    done
    
    # Verify server is running and startup completed
    if [ "$STARTUP_COMPLETE" != "true" ]; then
        print_result 1 "Server failed to start properly"
        return 1
    fi
    
    print_header "Testing Crash Handler with $binary_name"
    
    # Send SIGUSR1 to trigger test crash handler
    print_info "Triggering test crash handler with SIGUSR1..."
    kill -USR1 $HYDROGEN_PID
    
    # Wait for process to exit
    wait $HYDROGEN_PID
    CRASH_EXIT_CODE=$?
    
    # Check exit code matches SIGSEGV (11) from the null dereference
    if [ $CRASH_EXIT_CODE -eq $((128 + 11)) ]; then
        print_info "Process exited with SIGSEGV (expected for crash test)"
    else
        print_warning "Process exited with unexpected code: $CRASH_EXIT_CODE (expected SIGSEGV)"
        return 1
    fi
    
    # Verify core dump configuration first
    verify_core_dump_config
    local config_result=$?
    if [ $config_result -ne 0 ]; then
        print_warning "Core dump configuration issues detected, test results may be affected"
    fi
    
    # Initialize local result variables
    local core_result=1
    local log_result=1
    local gdb_result=1
    
    # Verify core file creation
    verify_core_file "$binary" $HYDROGEN_PID
    core_result=$?
    
    # If core file exists, verify its contents
    if [ $core_result -eq 0 ]; then
        local core_file="${HYDROGEN_DIR}/${binary_name}.core.${HYDROGEN_PID}"
        verify_core_file_content "$core_file" "$binary"
        if [ $? -ne 0 ]; then
            print_result 1 "Core file exists but content verification failed"
            core_result=1
        fi
    fi
    
    if [ $core_result -eq 0 ]; then
        # Check for key crash handler messages, ignoring log level/category
        if grep -q "Received SIGUSR1" "$SCRIPT_DIR/hydrogen_crash_test.log" && \
           grep -q "Signal 11 received" "$SCRIPT_DIR/hydrogen_crash_test.log"; then
            print_result 0 "Found crash handler messages"
            log_result=0
        else
            print_result 1 "Missing crash handler messages"
            log_result=1
        fi
        
        # Analyze core file with GDB
        local binary_dir=$(dirname "$binary")
        analyze_core_with_gdb "$binary" "${HYDROGEN_DIR}/${binary_name}.core.${HYDROGEN_PID}" "$GDB_OUTPUT_DIR/${binary_name}_${TIMESTAMP}.txt"
        gdb_result=$?
        
        if [ $gdb_result -eq 0 ]; then
            print_info "GDB analysis output:"
            cat "$GDB_OUTPUT_DIR/${binary_name}_${TIMESTAMP}.txt"
        fi
    fi
    
    # Save logs and results
    if [ -f "$SCRIPT_DIR/hydrogen_crash_test.log" ]; then
        cp "$SCRIPT_DIR/hydrogen_crash_test.log" "$RESULTS_DIR/crash_test_${TIMESTAMP}_${binary_name}.log"
    fi
    
    # Clean up test files (preserve core files for failed tests)
    rm -f "$SCRIPT_DIR/hydrogen_crash_test.log"
    if [ $core_result -eq 0 ] && [ $log_result -eq 0 ] && [ $gdb_result -eq 0 ]; then
        rm -f "${HYDROGEN_DIR}/${binary_name}.core.${HYDROGEN_PID}"
    else
        print_info "Preserving core file for debugging: ${binary_name}.core.${HYDROGEN_PID}"
        failed=""
        [ $core_result -ne 0 ] && failed="${failed}core_file "
        [ $log_result -ne 0 ] && failed="${failed}crash_log "
        [ $gdb_result -ne 0 ] && failed="${failed}gdb_analysis"
        print_info "Failed checks: ${failed# }"
    fi
    
    # Set global results for the caller
    CORE_FILE_RESULT=$core_result
    CRASH_LOG_RESULT=$log_result
    GDB_ANALYSIS_RESULT=$gdb_result
    
    # Return success only if all subtests passed
    if [ $core_result -eq 0 ] && [ $log_result -eq 0 ] && [ $gdb_result -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Function to analyze core file with GDB
analyze_core_with_gdb() {
    local binary="$1"
    local core_file="$2"
    local gdb_output_file="$3"
    local build_name=$(basename "$binary")
    
    # Check for debug info and set GDB flags accordingly
    local gdb_flags="-q"
    if readelf --sections "$binary" | grep -q ".debug_info"; then
        print_info "Debug info found in binary, using full output"
        gdb_flags="-q -ex 'set print frame-arguments all' -ex 'set print object on'"
    else
        print_info "No debug info found in binary, using limited output"
        gdb_flags="-q -ex 'set print frame-arguments none' -ex 'set print object off'"
    fi
    
    # Create GDB commands file
    cat > gdb_commands.txt << EOF
set pagination off
set print elements 0
set height 0
set width 0
file ${binary}
core-file ${core_file}
thread
info threads
thread apply all bt full
frame
info registers
info locals
quit
EOF
    
    print_info "Analyzing core file with GDB..."
    if ! gdb ${gdb_flags} -batch -x gdb_commands.txt > "${gdb_output_file}" 2> "${gdb_output_file}.stderr"; then
        print_warning "GDB exited with error, checking output anyway..."
        cat "${gdb_output_file}.stderr" >> "${gdb_output_file}"
    fi
    
    # Check for test_crash_handler in backtrace
    local has_backtrace=0
    local backtrace_quality=""
    local has_test_crash=0

    # Look for test_crash_handler in backtrace
    if grep -q "test_crash_handler.*at.*hydrogen\.c:[0-9]" "${gdb_output_file}" && \
       grep -q "Program terminated with signal SIGSEGV" "${gdb_output_file}"; then
        has_test_crash=1
        has_backtrace=1
        backtrace_quality="test crash handler found with expected output"
    # For release builds, just check for SIGSEGV
    elif [[ "$build_name" == *"release"* ]] && grep -q "Program terminated with signal SIGSEGV" "${gdb_output_file}"; then
        has_backtrace=1
        backtrace_quality="basic crash info (release build)"
    fi

    # Verify backtrace quality based on build type
    if [[ "$build_name" == *"release"* ]]; then
        if [ $has_backtrace -eq 1 ]; then
            print_result 0 "GDB produced basic backtrace (expected for release build)"
            return 0
        fi
    elif [ $has_test_crash -eq 1 ]; then
        print_result 0 "GDB found test_crash_handler in backtrace"
        return 0
    else
        print_result 1 "GDB failed to produce useful backtrace"
        print_info "GDB output preserved at: $(convert_to_relative_path "${gdb_output_file}")"
        return 1
    fi
}

# Start the test
start_test "Hydrogen Crash Handler Test"

# Verify core dump configuration before running any tests
verify_core_dump_config
CONFIG_RESULT=$?
if [ $CONFIG_RESULT -ne 0 ]; then
    print_warning "Core dump configuration issues detected, some tests may fail"
    print_warning "Please ensure core dumps are enabled and properly configured"
    print_warning "You may need to run: ulimit -c unlimited"
fi

# Create results directories if they don't exist
RESULTS_DIR="$SCRIPT_DIR/results"
GDB_OUTPUT_DIR="$RESULTS_DIR/gdb_analysis"
mkdir -p "$RESULTS_DIR" "$GDB_OUTPUT_DIR"

# Store the timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_LOG="$RESULTS_DIR/crash_test_${TIMESTAMP}.log"

# Initialize test result variables
CORE_FILE_RESULT=1
GDB_ANALYSIS_RESULT=1
CRASH_LOG_RESULT=1

# Find all available builds and their descriptions
declare -A FOUND_BUILDS  # Track which builds we've found
declare -a BUILDS
declare -A BUILD_DESCRIPTIONS

# Parse all build targets from Makefile
while IFS= read -r line; do
    if [[ $line =~ ^([A-Z_]*TARGET)[[:space:]]*=[[:space:]]*\$\(BIN_PREFIX\)([[:alnum:]_]+)[[:space:]]*$ ]]; then
        target="${BASH_REMATCH[2]}"
        if [ -f "$HYDROGEN_DIR/$target" ] && [ -z "${FOUND_BUILDS[$target]}" ]; then
            FOUND_BUILDS[$target]=1
            BUILDS+=("$HYDROGEN_DIR/$target")
            # Extract description from build variant comments
            if [[ "$target" == "hydrogen" ]]; then
                desc="Standard development build"
            elif [[ "$target" == "hydrogen_debug" ]]; then
                desc="Bug finding and analysis"
            elif [[ "$target" == "hydrogen_valgrind" ]]; then
                desc="Memory analysis"
            elif [[ "$target" == "hydrogen_perf" ]]; then
                desc="Maximum speed"
            elif [[ "$target" == "hydrogen_release" ]]; then
                desc="Production deployment"
            else
                desc="Build variant: $target"
            fi
            if [ -n "$desc" ]; then
                BUILD_DESCRIPTIONS["$target"]="$desc"
            else
                BUILD_DESCRIPTIONS["$target"]="Build variant: $target"
            fi
        fi
    fi
done < "$HYDROGEN_DIR/src/Makefile"

if [ ${#BUILDS[@]} -eq 0 ]; then
    print_result 1 "No hydrogen builds found"
    exit 1
fi

print_info "Found builds:"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    print_info "  • ${build_name}: ${BUILD_DESCRIPTIONS[$build_name]}"
done

# Debug output for build discovery
print_info "Build discovery details:"
print_info "Total builds found: ${#BUILDS[@]}"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    print_info "  • Build: $build_name"
    print_info "    - Description: ${BUILD_DESCRIPTIONS[$build_name]}"
    print_info "    - Path: $(convert_to_relative_path "$build")"
    print_info "    - Exists: $([ -f "$build" ] && echo "Yes" || echo "No")"
done

print_info "Using minimal configuration: $(convert_to_relative_path "$CONFIG_FILE")"

# Track results for each build and their subtests
declare -A BUILD_RESULTS
declare -A BUILD_SUBTESTS
declare -A BUILD_PASSED_SUBTESTS

# Test each build
TOTAL_BUILDS=0
TOTAL_PASSED_BUILDS=0
TOTAL_SUBTESTS=0
TOTAL_PASSED_SUBTESTS=0

for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    ((TOTAL_BUILDS++))
    
    print_header "Testing build: $build_name"
    print_info "Description: ${BUILD_DESCRIPTIONS[$build_name]}"
    
    # Initialize subtest counters for this build
    BUILD_SUBTESTS[$build_name]=4  # Debug symbols, core file, log message, GDB analysis
    BUILD_PASSED_SUBTESTS[$build_name]=0
    
    # Verify debug symbols first
    verify_debug_symbols "$build"
    DEBUG_SYMBOL_RESULT=$?
    
    # Run the build test
    run_test_with_build "$build"
    BUILD_RESULTS[$build_name]=$?
    
    # Update passed subtests count based on individual checks
    if [ $DEBUG_SYMBOL_RESULT -eq 0 ]; then
        ((BUILD_PASSED_SUBTESTS[$build_name]+=1))
    fi
    if [ $CORE_FILE_RESULT -eq 0 ]; then
        ((BUILD_PASSED_SUBTESTS[$build_name]+=1))
    fi
    if [ $CRASH_LOG_RESULT -eq 0 ]; then
        ((BUILD_PASSED_SUBTESTS[$build_name]+=1))
    fi
    if [ $GDB_ANALYSIS_RESULT -eq 0 ]; then
        ((BUILD_PASSED_SUBTESTS[$build_name]+=1))
    fi
    
    # Update totals
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + ${BUILD_SUBTESTS[$build_name]}))
    TOTAL_PASSED_SUBTESTS=$((TOTAL_PASSED_SUBTESTS + ${BUILD_PASSED_SUBTESTS[$build_name]}))
    
    # All builds have 4 tests (debug symbols presence/absence, core dump, logs, GDB)
    if [ ${BUILD_PASSED_SUBTESTS[$build_name]} -eq ${BUILD_SUBTESTS[$build_name]} ]; then
        print_result 0 "Build $build_name passed all subtests"
        ((TOTAL_PASSED_BUILDS++))
    else
        print_result 1 "Build $build_name passed ${BUILD_PASSED_SUBTESTS[$build_name]} of ${BUILD_SUBTESTS[$build_name]} subtests"
    fi
    echo ""
done

# Print test summary
print_header "Build Test Summary"
echo -e "${BLUE}${BOLD}Test results by build type:${NC}"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    # Determine if all subtests passed
    if [ ${BUILD_PASSED_SUBTESTS[$build_name]} -eq ${BUILD_SUBTESTS[$build_name]} ]; then
        echo -e "${GREEN}${PASS_ICON} ${build_name}${NC}"
        echo -e "   ${CYAN}Description:${NC} ${BUILD_DESCRIPTIONS[$build_name]}"
        echo -e "   ${CYAN}Status:${NC} All checks passed"
        echo -e "   ${CYAN}Details:${NC}"
        if [[ "$build_name" == *"release"* ]]; then
            echo -e "      • Debug symbols verified (not present as expected)"
            echo -e "      • Core dump generated successfully"
            echo -e "      • Crash handler log message verified"
            echo -e "      • GDB backtrace analysis successful"
        else
            echo -e "      • Debug symbols verified (present as expected)"
            echo -e "      • Core dump generated successfully"
            echo -e "      • Crash handler log message verified"
            echo -e "      • GDB backtrace analysis successful"
        fi
    else
        echo -e "${RED}${FAIL_ICON} ${build_name}${NC}"
        echo -e "   ${CYAN}Description:${NC} ${BUILD_DESCRIPTIONS[$build_name]}"
        echo -e "   ${CYAN}Status:${NC} ${BUILD_PASSED_SUBTESTS[$build_name]} of ${BUILD_SUBTESTS[$build_name]} checks passed"
        echo -e "   ${CYAN}Details:${NC}"
        if [[ "$build_name" == *"release"* ]]; then
            [ $DEBUG_SYMBOL_RESULT -eq 0 ] && echo -e "      • Debug symbols verified (not present)" || echo -e "      ${RED}✗${NC} Debug symbols unexpectedly found"
            [ $CORE_FILE_RESULT -eq 0 ] && echo -e "      • Core dump generated successfully" || echo -e "      ${RED}✗${NC} Core dump generation failed"
            [ $CRASH_LOG_RESULT -eq 0 ] && echo -e "      • Crash handler log message verified" || echo -e "      ${RED}✗${NC} Crash handler log message not found"
            [ $GDB_ANALYSIS_RESULT -eq 0 ] && echo -e "      • GDB backtrace analysis successful" || echo -e "      ${RED}✗${NC} GDB backtrace analysis failed"
        else
            [ $DEBUG_SYMBOL_RESULT -eq 0 ] && echo -e "      • Debug symbols verified (present)" || echo -e "      ${RED}✗${NC} Debug symbols not found"
            [ $CORE_FILE_RESULT -eq 0 ] && echo -e "      • Core dump generated successfully" || echo -e "      ${RED}✗${NC} Core dump generation failed"
            [ $CRASH_LOG_RESULT -eq 0 ] && echo -e "      • Crash handler log message verified" || echo -e "      ${RED}✗${NC} Crash handler log message not found"
            [ $GDB_ANALYSIS_RESULT -eq 0 ] && echo -e "      • GDB backtrace analysis successful" || echo -e "      ${RED}✗${NC} GDB backtrace analysis failed"
        fi
        # Only show debug info for actual failures
        if [ ${BUILD_PASSED_SUBTESTS[$build_name]} -lt ${BUILD_SUBTESTS[$build_name]} ]; then
            echo -e "   ${CYAN}Debug Info:${NC} Check $(convert_to_relative_path "${GDB_OUTPUT_DIR}")/${build_name}_${TIMESTAMP}.txt"
        fi
    fi
    echo ""
done

# Calculate and export test results
TOTAL_SUBTESTS=0
TOTAL_PASSED_SUBTESTS=0

print_info "Calculating test results:"
print_info "Expected subtests per build: 4 (debug symbols, core dump, logs, GDB)"

# Each build has 4 subtests
for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    build_total=${BUILD_SUBTESTS[$build_name]}
    build_passed=${BUILD_PASSED_SUBTESTS[$build_name]}
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + build_total))
    TOTAL_PASSED_SUBTESTS=$((TOTAL_PASSED_SUBTESTS + build_passed))
    
    print_info "  • $build_name results:"
    print_info "    - Total subtests: $build_total"
    print_info "    - Passed subtests: $build_passed"
    print_info "    - Individual results:"
    print_info "      * Debug symbols: $([ $DEBUG_SYMBOL_RESULT -eq 0 ] && echo "Pass" || echo "Fail")"
    print_info "      * Core dump: $([ $CORE_FILE_RESULT -eq 0 ] && echo "Pass" || echo "Fail")"
    print_info "      * Crash log: $([ $CRASH_LOG_RESULT -eq 0 ] && echo "Pass" || echo "Fail")"
    print_info "      * GDB analysis: $([ $GDB_ANALYSIS_RESULT -eq 0 ] && echo "Pass" || echo "Fail")"
done

# Export results using the utility function with explicit test name
print_info "Writing final results..."
print_info "Final totals:"
print_info "  • Total subtests: $TOTAL_SUBTESTS"
print_info "  • Passed subtests: $TOTAL_PASSED_SUBTESTS"

# Get test name from script name (keeping the number prefix)
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results with correct test name format
export_subtest_results "$TEST_NAME" "$TOTAL_SUBTESTS" "$TOTAL_PASSED_SUBTESTS"

# Log test results
print_info "Crash Handler Test: $TOTAL_PASSED_BUILDS of $TOTAL_BUILDS builds passed"
print_info "Check details: $TOTAL_PASSED_SUBTESTS of $TOTAL_SUBTESTS checks passed across all builds"

# Determine final test result - require at least one build to pass all checks
TEST_RESULT=1
if [ $TOTAL_PASSED_BUILDS -gt 0 ]; then
    print_result 0 "$TOTAL_PASSED_BUILDS of $TOTAL_BUILDS builds passed all checks"
    TEST_RESULT=0
else
    print_result 1 "No builds passed all required checks"
fi


# End the test
end_test $TEST_RESULT "Crash Handler Test"

# Clean up GDB commands file
rm -f gdb_commands.txt

# Clean up any remaining core files from successful tests
for build in "${BUILDS[@]}"; do
    build_name=$(basename "$build")
    if [ ${BUILD_RESULTS[$build_name]} -eq 0 ]; then
        rm -f "${HYDROGEN_DIR}/${build_name}.core.*"
    fi
done

exit $TEST_RESULT