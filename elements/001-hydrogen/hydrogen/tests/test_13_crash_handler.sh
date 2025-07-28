#!/bin/bash
#
# Test: Crash Handler
# Tests that the crash handler correctly generates and formats core dumps
#
# CHANGELOG
# 3.0.3 - 2025-07-14 - No more sleep
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.1 - 2025-07-01 - Updated to use predefined CMake build variants instead of parsing Makefile
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="Crash Handler"
SCRIPT_VERSION="3.0.3"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "${LOG_OUTPUT_SH_GUARD}" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "${SCRIPT_DIR}/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/lifecycle.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=0  # Will be calculated dynamically based on builds found
PASS_COUNT=0

# Crash test configuration
STARTUP_TIMEOUT=10
CRASH_TIMEOUT=5

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Use build directory for test results
BUILD_DIR="${SCRIPT_DIR}/../build"
RESULTS_DIR="${BUILD_DIR}/tests/results"
GDB_OUTPUT_DIR="${BUILD_DIR}/tests/diagnostics/gdb_analysis"
mkdir -p "${RESULTS_DIR}" "${GDB_OUTPUT_DIR}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Get Hydrogen directory
HYDROGEN_DIR="$( cd "${SCRIPT_DIR}/.." && pwd )"

# Test configuration
TEST_CONFIG=$(get_config_path "hydrogen_test_min.json")

# Validate configuration file exists
next_subtest
print_subtest "Validate Test Configuration File"
if validate_config_file "${TEST_CONFIG}"; then
    print_message "Using minimal configuration for crash testing"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Function to verify core dump configuration
verify_core_dump_config() {
    print_message "Checking core dump configuration..."
    
    # Check core pattern
    local core_pattern
    core_pattern=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || echo "unknown")
    if [[ "${core_pattern}" == "core" ]]; then
        print_message "Core pattern is set to default 'core'"
        return 0
    else
        print_warning "Core pattern is set to '${core_pattern}', might affect core dump location"
    fi
    
    # Check core dump size limit
    local core_limit
    core_limit=$(ulimit -c)
    if [[ "${core_limit}" == "unlimited" || "${core_limit}" -gt 0 ]]; then
        print_message "Core dump size limit is adequate (${core_limit})"
    else
        print_warning "Core dump size limit is too restrictive (${core_limit})"
        # Try to set unlimited core dumps for this session
        ulimit -c unlimited
        print_message "Attempted to set unlimited core dump size"
    fi
    
    return 0
}

# Function to verify core file content
verify_core_file_content() {
    local core_file="$1"
    local binary="$2"
    
    print_message "Verifying core file contents..."
    
    # Check file size and verify it's a core file
    local core_size
    core_size=$(stat -c %s "${core_file}" 2>/dev/null || echo "0")
    if [ "${core_size}" -lt 1024 ]; then
        print_message "Core file is suspiciously small (${core_size} bytes)"
        return 1
    fi
    
    # Verify it's a valid core file
    if readelf -h "${core_file}" 2>/dev/null | grep -q "Core file"; then
        print_message "Valid core file found (${core_size} bytes)"
        return 0
    else
        print_message "Not a valid core file"
        return 1
    fi
}

# Function to verify debug symbols are present
verify_debug_symbols() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local expect_symbols=1
    
    # Release and naked builds should not have debug symbols
    if [[ "${binary_name}" == *"release"* ]] || [[ "${binary_name}" == *"naked"* ]]; then
        expect_symbols=0
    fi
    
    print_message "Checking debug symbols in ${binary_name}..."
    
    # Use readelf to check for debug symbols
    if readelf --debug-dump=info "${binary}" 2>/dev/null | grep -q "DW_AT_name"; then
        if [ ${expect_symbols} -eq 1 ]; then
            print_message "Debug symbols found in ${binary_name} (as expected)"
            return 0
        else
            print_message "Debug symbols found in ${binary_name} (unexpected for release build)"
            return 1
        fi
    else
        if [ ${expect_symbols} -eq 1 ]; then
            print_message "No debug symbols found in ${binary_name} (symbols required)"
            return 1
        else
            print_message "No debug symbols found in ${binary_name} (as expected for release build)"
            return 0
        fi
    fi
}

# Function to verify core file exists and has correct name format
verify_core_file() {
    local binary="$1"
    local pid="$2"
    local binary_name
    binary_name=$(basename "${binary}")
    local expected_core="${HYDROGEN_DIR}/${binary_name}.core.${pid}"
    local timeout=${CRASH_TIMEOUT}
    local found=0
    local start_time
    start_time=$(date +%s)
    
    print_message "Waiting for core file ${binary_name}.core.${pid}..."
    
    # Wait for core file to appear
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "${timeout}" ]; then
            break
        fi
        
        if [ -f "${expected_core}" ]; then
            found=1
            break
        fi
        
        # sleep 0.1
    done
    
    if [ ${found} -eq 1 ]; then
        print_message "Core file ${binary_name}.core.${pid} found"
        return 0
    else
        print_message "Core file ${binary_name}.core.${pid} not found after ${timeout} seconds"
        # List any core files that might exist
        local core_files
        core_files=$(ls "${HYDROGEN_DIR}"/*.core.* 2>/dev/null || echo "")
        if [ -n "${core_files}" ]; then
            print_message "Other core files found: ${core_files}"
        else
            print_message "No core files found in hydrogen directory"
        fi
        return 1
    fi
}

# Function to analyze core file with GDB
analyze_core_with_gdb() {
    local binary="$1"
    local core_file="$2"
    local gdb_output_file="$3"
    local build_name
    build_name=$(basename "${binary}")
    
    # Check for debug info and set GDB flags accordingly
    local gdb_flags
    if readelf --sections "${binary}" 2>/dev/null | grep -q ".debug_info"; then
        print_message "Debug info found in binary, using full output"
        gdb_flags=(-q -ex 'set print frame-arguments all' -ex 'set print object on')
    else
        print_message "No debug info found in binary, using limited output"
        gdb_flags=(-q -ex 'set print frame-arguments none' -ex 'set print object off')
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
    
    print_message "Analyzing core file with GDB..."
    if ! gdb "${gdb_flags[@]}" -batch -x gdb_commands.txt > "${gdb_output_file}" 2> "${gdb_output_file}.stderr"; then
        print_message "GDB exited with error, checking output anyway..."
        cat "${gdb_output_file}.stderr" >> "${gdb_output_file}" 2>/dev/null || true
    fi
    
    # Check for test_crash_handler in backtrace
    local has_backtrace=0
    local has_test_crash=0

    # Look for test_crash_handler in backtrace
    if grep -q "test_crash_handler.*at.*hydrogen\.c:[0-9]" "${gdb_output_file}" && \
       grep -q "Program terminated with signal SIGSEGV" "${gdb_output_file}"; then
        has_test_crash=1
        has_backtrace=1
    # For release and naked builds, just check for SIGSEGV
    elif [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
        if grep -q "Program terminated with signal SIGSEGV" "${gdb_output_file}"; then
            has_backtrace=1
        fi
    fi

    # Verify backtrace quality based on build type
    if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
        if [ ${has_backtrace} -eq 1 ]; then
            print_message "GDB produced basic backtrace (expected for release-style build)"
            return 0
        fi
    elif [ ${has_test_crash} -eq 1 ]; then
        print_message "GDB found test_crash_handler in backtrace"
        return 0
    else
        print_message "GDB failed to produce useful backtrace"
        print_message "GDB output preserved at: $(convert_to_relative_path "${gdb_output_file}")"
        return 1
    fi
    
    print_message "GDB analysis failed to find expected patterns"
    return 1
}

# Function to wait for crash completion
wait_for_crash_completion() {
    local pid="$1"
    local timeout="$2"
    local start_time
    start_time=$(date +%s)
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "${timeout}" ]; then
            return 1
        fi
        
        if ! ps -p "${pid}" > /dev/null 2>&1; then
            return 0
        fi
        
        # sleep 0.1
    done
}

# Function to run test with a specific build in parallel (writes results to files)
run_crash_test_parallel() {
    local binary="$1"
    local result_dir="$2"
    local binary_name
    binary_name=$(basename "${binary}")
    local log_file="${SCRIPT_DIR}/hydrogen_crash_test_${binary_name}_${TIMESTAMP}.log"
    local result_file="${result_dir}/${binary_name}.result"
    
    # Clear files
    true > "${log_file}"
    true > "${result_file}"
    
    # Start hydrogen server in background with ASAN leak detection disabled
    ASAN_OPTIONS="detect_leaks=0" "${binary}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
    # Wait for startup with active log monitoring
    local startup_start current_time elapsed
    startup_start=$(date +%s)
    local startup_complete=false
    
    while true; do
        current_time=$(date +%s)
        elapsed=$((current_time - startup_start))
        
        if [ ${elapsed} -ge ${STARTUP_TIMEOUT} ]; then
            echo "STARTUP_FAILED" > "${result_file}"
            kill -9 ${hydrogen_pid} 2>/dev/null || true
            return 1
        fi
        
        if ! ps -p ${hydrogen_pid} > /dev/null 2>&1; then
            startup_complete=true
            break
        fi
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            startup_complete=true
            break
        fi
        
        # sleep 0.1
    done
    
    if [ "${startup_complete}" != "true" ] || ! ps -p ${hydrogen_pid} > /dev/null 2>&1; then
        echo "STARTUP_FAILED" > "${result_file}"
        kill -9 ${hydrogen_pid} 2>/dev/null || true
        return 1
    fi
    
    # Send SIGUSR1 to trigger test crash handler
    kill -USR1 ${hydrogen_pid}
    
    # Wait for process to crash
    if ! wait_for_crash_completion ${hydrogen_pid} ${CRASH_TIMEOUT}; then
        echo "CRASH_FAILED" > "${result_file}"
        kill -9 ${hydrogen_pid} 2>/dev/null || true
        return 1
    fi
    
    # Store PID for later analysis
    echo "PID=${hydrogen_pid}" >> "${result_file}"
    echo "SUCCESS" >> "${result_file}"
    
    # Wait for the process to fully exit
    wait ${hydrogen_pid} 2>/dev/null || true
    
    return 0
}

# Function to analyze results from parallel crash test
analyze_parallel_results() {
    local binary="$1"
    local result_dir="$2"
    local binary_name
    binary_name=$(basename "${binary}")
    local result_file="${result_dir}/${binary_name}.result"
    local log_file="${SCRIPT_DIR}/hydrogen_crash_test_${binary_name}_${TIMESTAMP}.log"
    
    # Check if result file exists
    if [ ! -f "${result_file}" ]; then
        print_message "No result file found for ${binary_name}"
        return 1
    fi
    
    # Read result file
    local result_status
    result_status=$(tail -n 1 "${result_file}")
    
    if [ "${result_status}" != "SUCCESS" ]; then
        print_message "${binary_name}: Test failed during execution (${result_status})"
        return 1
    fi
    
    # Extract PID from result file
    local hydrogen_pid
    hydrogen_pid=$(grep "PID=" "${result_file}" | cut -d'=' -f2)
    
    if [ -z "${hydrogen_pid}" ]; then
        print_message "${binary_name}: Could not extract PID from result file"
        return 1
    fi
    
    # Initialize result variables for this build
    local debug_result=1
    local core_result=1
    local log_result=1
    local gdb_result=1
    
    # Verify debug symbols
    if verify_debug_symbols "${binary}"; then
        debug_result=0
    fi
    
    # Verify core file creation
    if verify_core_file "${binary}" "${hydrogen_pid}"; then
        core_result=0
        
        # If core file exists, verify its contents
        local core_file="${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
        if ! verify_core_file_content "${core_file}" "${binary}"; then
            print_message "Core file exists but content verification failed"
            core_result=1
        fi
    fi
    
    # Verify crash handler log messages
    if grep -q "Received SIGUSR1" "${log_file}" 2>/dev/null && \
       grep -q "Signal 11 received" "${log_file}" 2>/dev/null; then
        print_message "Found crash handler messages in log"
        log_result=0
    else
        print_message "Missing crash handler messages in log"
        log_result=1
    fi
    
    # Analyze core file with GDB if core file was created
    if [ ${core_result} -eq 0 ]; then
        local core_file="${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
        if analyze_core_with_gdb "${binary}" "${core_file}" "${GDB_OUTPUT_DIR}/${binary_name}_${TIMESTAMP}.txt"; then
            gdb_result=0
        fi
    fi
    
    # Save logs and results
    if [ -f "${log_file}" ]; then
        cp "${log_file}" "${RESULTS_DIR}/crash_test_${TIMESTAMP}_${binary_name}.log"
    fi
    
    # Clean up test files (preserve core files for failed tests)
    rm -f "${log_file}"
    if [ ${core_result} -eq 0 ] && [ ${log_result} -eq 0 ] && [ ${gdb_result} -eq 0 ]; then
        rm -f "${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
    else
        print_message "Preserving core file for debugging: ${binary_name}.core.${hydrogen_pid}"
    fi
    
    # Clean up GDB commands file
    rm -f gdb_commands.txt
    
    # Return results via global variables (since we need multiple return values)
    DEBUG_SYMBOL_RESULT=${debug_result}
    CORE_FILE_RESULT=${core_result}
    CRASH_LOG_RESULT=${log_result}
    GDB_ANALYSIS_RESULT=${gdb_result}
    
    # Return success only if all subtests passed
    if [ ${debug_result} -eq 0 ] && [ ${core_result} -eq 0 ] && [ ${log_result} -eq 0 ] && [ ${gdb_result} -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Function to run test with a specific build
run_crash_test_with_build() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local log_file="${SCRIPT_DIR}/hydrogen_crash_test_${binary_name}_${TIMESTAMP}.log"
    
    print_message "Testing crash handler with build: ${binary_name}"
    
    # Clear log file
    true > "${log_file}"
    
    # Start hydrogen server in background with ASAN leak detection disabled
    print_message "Starting hydrogen server (with leak detection disabled)..."
    print_command "ASAN_OPTIONS=\"detect_leaks=0\" $(basename "${binary}") $(basename "${TEST_CONFIG}")"
    ASAN_OPTIONS="detect_leaks=0" "${binary}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
    # Wait for startup with active log monitoring
    print_message "Waiting for startup (max ${STARTUP_TIMEOUT}s)..."
    local startup_start current_time elapsed
    startup_start=$(date +%s)
    local startup_complete=false
    
    while true; do
        # Check if we've exceeded the timeout
        current_time=$(date +%s)
        elapsed=$((current_time - startup_start))
        
        if [ "${elapsed}" -ge "${STARTUP_TIMEOUT}" ]; then
            print_message "Startup timeout after ${elapsed}s"
            kill -9 ${hydrogen_pid} 2>/dev/null || true
            return 1
        fi
        
        # Check if process is still running
        if ! ps -p ${hydrogen_pid} > /dev/null 2>&1; then
            # This could be an expected part of the crash handler test
            print_message "Server crashed during startup (might be expected behavior)"
            startup_complete=true
            break
        fi
        
        # Check for startup completion message
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            startup_complete=true
            print_message "Startup completed in ${elapsed}s"
            break
        fi
        
        # sleep 0.1
    done
    
    # Verify server is running and startup completed
    if [ "${startup_complete}" != "true" ]; then
        print_message "Server failed to start properly"
        kill -9 ${hydrogen_pid} 2>/dev/null || true
        return 1
    fi
    
    # If process already crashed during startup, that's not what we want for this test
    if ! ps -p ${hydrogen_pid} > /dev/null 2>&1; then
        print_message "Process crashed during startup - not suitable for crash handler test"
        return 1
    fi
    
    # Send SIGUSR1 to trigger test crash handler
    print_message "Triggering test crash handler with SIGUSR1..."
    print_command "kill -USR1 ${hydrogen_pid}"
    kill -USR1 ${hydrogen_pid}
    
    # Wait for process to crash
    if wait_for_crash_completion "${hydrogen_pid}" "${CRASH_TIMEOUT}"; then
        print_message "Process crashed as expected"
    else
        print_message "Process did not crash within timeout"
        kill -9 ${hydrogen_pid} 2>/dev/null || true
        return 1
    fi
    
    # Wait for the process to fully exit and get exit code
    wait ${hydrogen_pid} 2>/dev/null || true
    local crash_exit_code=$?
    
    # Check exit code matches SIGSEGV (11) from the null dereference
    if [ ${crash_exit_code} -eq $((128 + 11)) ]; then
        print_message "Process exited with SIGSEGV (expected for crash test)"
    else
        print_message "Process exited with unexpected code: ${crash_exit_code} (expected SIGSEGV)"
    fi
    
    # Initialize result variables for this build
    local debug_result=1
    local core_result=1
    local log_result=1
    local gdb_result=1
    
    # Verify debug symbols
    if verify_debug_symbols "${binary}"; then
        debug_result=0
    fi
    
    # Verify core file creation
    if verify_core_file "${binary}" "${hydrogen_pid}"; then
        core_result=0
        
        # If core file exists, verify its contents
        local core_file="${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
        if ! verify_core_file_content "${core_file}" "${binary}"; then
            print_message "Core file exists but content verification failed"
            core_result=1
        fi
    fi
    
    # Verify crash handler log messages
    if grep -q "Received SIGUSR1" "${log_file}" 2>/dev/null && \
       grep -q "Signal 11 received" "${log_file}" 2>/dev/null; then
        print_message "Found crash handler messages in log"
        log_result=0
    else
        print_message "Missing crash handler messages in log"
        log_result=1
    fi
    
    # Analyze core file with GDB if core file was created
    if [ ${core_result} -eq 0 ]; then
        local core_file="${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
        if analyze_core_with_gdb "${binary}" "${core_file}" "${GDB_OUTPUT_DIR}/${binary_name}_${TIMESTAMP}.txt"; then
            gdb_result=0
        fi
    fi
    
    # Save logs and results
    if [ -f "${log_file}" ]; then
        cp "${log_file}" "${RESULTS_DIR}/crash_test_${TIMESTAMP}_${binary_name}.log"
    fi
    
    # Clean up test files (preserve core files for failed tests)
    rm -f "${log_file}"
    if [ ${core_result} -eq 0 ] && [ ${log_result} -eq 0 ] && [ ${gdb_result} -eq 0 ]; then
        rm -f "${HYDROGEN_DIR}/${binary_name}.core.${hydrogen_pid}"
    else
        print_message "Preserving core file for debugging: ${binary_name}.core.${hydrogen_pid}"
    fi
    
    # Clean up GDB commands file
    rm -f gdb_commands.txt
    
    # Return results via global variables (since we need multiple return values)
    DEBUG_SYMBOL_RESULT=${debug_result}
    CORE_FILE_RESULT=${core_result}
    CRASH_LOG_RESULT=${log_result}
    GDB_ANALYSIS_RESULT=${gdb_result}
    
    # Return success only if all subtests passed
    if [ ${debug_result} -eq 0 ] && [ ${core_result} -eq 0 ] && [ ${log_result} -eq 0 ] && [ ${gdb_result} -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Verify core dump configuration before running any tests
next_subtest
print_subtest "Verify Core Dump Configuration"
if verify_core_dump_config; then
    print_result 0 "Core dump configuration verified"
    ((PASS_COUNT++))
else
    print_result 1 "Core dump configuration issues detected"
    EXIT_CODE=1
fi

# Find all available builds
print_message "Discovering available Hydrogen builds..."

declare -A FOUND_BUILDS  # Track which builds we've found
declare -a BUILDS
declare -A BUILD_DESCRIPTIONS

# Define expected build variants based on CMake targets
declare -a BUILD_VARIANTS=("hydrogen" "hydrogen_debug" "hydrogen_valgrind" "hydrogen_perf" "hydrogen_release" "hydrogen_coverage" "hydrogen_naked")
for target in "${BUILD_VARIANTS[@]}"; do
    if [ -f "${HYDROGEN_DIR}/${target}" ] && [ -z "${FOUND_BUILDS[${target}]}" ]; then
        FOUND_BUILDS[${target}]=1
        BUILDS+=("${HYDROGEN_DIR}/${target}")
        # Assign description based on build variant
        case "${target}" in
            "hydrogen")
                BUILD_DESCRIPTIONS["${target}"]="Standard development build"
                ;;
            "hydrogen_debug")
                BUILD_DESCRIPTIONS["${target}"]="Bug finding and analysis"
                ;;
            "hydrogen_valgrind")
                BUILD_DESCRIPTIONS["${target}"]="Memory analysis"
                ;;
            "hydrogen_perf")
                BUILD_DESCRIPTIONS["${target}"]="Maximum speed"
                ;;
            "hydrogen_release")
                BUILD_DESCRIPTIONS["${target}"]="Production deployment"
                ;;
            "hydrogen_coverage")
                BUILD_DESCRIPTIONS["${target}"]="Code coverage analysis"
                ;;
            "hydrogen_naked")
                BUILD_DESCRIPTIONS["${target}"]="Minimal build without debugging"
                ;;
            *)
                BUILD_DESCRIPTIONS["${target}"]="Build variant: ${target}"
                ;;
        esac
    fi
done

if [ ${#BUILDS[@]} -eq 0 ]; then
    next_subtest
    print_subtest "Find Hydrogen Builds"
    print_result 1 "No hydrogen builds found"
    EXIT_CODE=1
    
    # Export results and exit early
    export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null
    print_test_completion "${TEST_NAME}"
    end_test ${EXIT_CODE} 3 ${PASS_COUNT} > /dev/null

    # Return status code if sourced, exit if run standalone
    if [[ "${ORCHESTRATION}" == "true" ]]; then
        return ${EXIT_CODE}
    else    
        exit ${EXIT_CODE}
    fi
fi

# Calculate total subtests: 2 initial + (4 per build)
TOTAL_SUBTESTS=$((2 + ${#BUILDS[@]} * 4))

print_message "Found ${#BUILDS[@]} builds for testing:"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    print_message "  • ${build_name}: ${BUILD_DESCRIPTIONS[${build_name}]}"
done

# Test each build using parallel execution
declare -A BUILD_PASSED_SUBTESTS
declare -a PARALLEL_PIDS

# Create temporary directory for parallel results
PARALLEL_RESULTS_DIR="${RESULTS_DIR}/parallel_${TIMESTAMP}"
mkdir -p "${PARALLEL_RESULTS_DIR}"

print_message "Running crash tests in parallel for all builds..."

# Start all builds in parallel
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    print_message "Starting parallel test for: ${build_name}"
    
    # Run parallel crash test in background
    run_crash_test_parallel "${build}" "${PARALLEL_RESULTS_DIR}" &
    PARALLEL_PIDS+=($!)
done

# Wait for all parallel tests to complete
print_message "Waiting for all ${#BUILDS[@]} parallel tests to complete..."
for pid in "${PARALLEL_PIDS[@]}"; do
    wait "${pid}"
done
print_message "All parallel tests completed, analyzing results..."

# Process results sequentially for clean output
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    
    print_message "Testing build: ${build_name} (${BUILD_DESCRIPTIONS[${build_name}]})"
    
    # Initialize subtest results for this build
    BUILD_PASSED_SUBTESTS[${build_name}]=0
    
    # Analyze the parallel test results
    analyze_parallel_results "${build}" "${PARALLEL_RESULTS_DIR}"
    
    # Test 1: Debug Symbols
    next_subtest
    print_subtest "Debug Symbols - ${build_name}"
    if [ "${DEBUG_SYMBOL_RESULT}" -eq 0 ]; then
        if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"coverage"* ]] || [[ "${build_name}" == *"naked"* ]]; then
            print_result 0 "Debug symbols correctly absent in release-style build"
        else
            print_result 0 "Debug symbols found in development build"
        fi
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"coverage"* ]] || [[ "${build_name}" == *"naked"* ]]; then
            print_result 1 "Debug symbols unexpectedly found in release-style build"
        else
            print_result 1 "Debug symbols missing in development build"
        fi
        EXIT_CODE=1
    fi
    
    # Test 2: Core File Generation
    next_subtest
    print_subtest "Core File Generation - ${build_name}"
    if [ "${CORE_FILE_RESULT}" -eq 0 ]; then
        print_result 0 "Core file generated successfully"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "Core file generation failed"
        EXIT_CODE=1
    fi
    
    # Test 3: Crash Handler Logging
    next_subtest
    print_subtest "Crash Handler Logging - ${build_name}"
    if [ "${CRASH_LOG_RESULT}" -eq 0 ]; then
        print_result 0 "Crash handler log messages verified"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "Crash handler log messages missing"
        EXIT_CODE=1
    fi
    
    # Test 4: GDB Analysis
    next_subtest
    print_subtest "GDB Analysis - ${build_name}"
    if [ "${GDB_ANALYSIS_RESULT}" -eq 0 ]; then
        print_result 0 "GDB backtrace analysis successful"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "GDB backtrace analysis failed"
        EXIT_CODE=1
    fi
    
    # Summary for this build
    if [ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]; then
        print_message "Build ${build_name}: All 4 crash handler tests passed"
    else
        print_message "Build ${build_name}: ${BUILD_PASSED_SUBTESTS[${build_name}]}/4 crash handler tests passed"
    fi
done

# Print build summary
print_message "Crash handler test completed for ${#BUILDS[@]} builds"

# Calculate successful builds
successful_builds=0
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    if [ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]; then
        ((successful_builds++))
    fi
done

print_message "Summary: ${successful_builds}/${#BUILDS[@]} builds passed all crash handler tests"

# Clean up parallel results directory
rm -rf "${PARALLEL_RESULTS_DIR}"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null

# Print completion table
print_test_completion "${TEST_NAME}"

end_test ${EXIT_CODE} ${TOTAL_SUBTESTS} ${PASS_COUNT} > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return ${EXIT_CODE}
else
    exit ${EXIT_CODE}
fi
