#!/usr/bin/env bash

# Test: Crash Handler
# Tests that the crash handler correctly generates and formats core dumps

# FUNCTIONS
# verify_core_dump_config() 
# verify_core_file_content() 
# verify_debug_symbols() 
# verify_core_file() 
# analyze_core_with_gdb() 
# wait_for_crash_completion() 
# run_crash_test_parallel() 
# analyze_parallel_results() 
# run_crash_test_with_build() 

# CHANGELOG
# 6.2.0 - 2025-08-08 - Bit of cleanup to output paths again, general logging cleanup
# 6.1.0 - 2025-08-05 - Adjusted output paths - variables not avaialble in subshell
# 6.0.0 - 2025-08-04 - Overhauled (separately) by Grok 4 so it runs in half the time
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.0 - 2025-07-28 - Shellcheck fixes, Grok even gave it a full once-over. 
# 3.0.3 - 2025-07-14 - No more sleep
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.1 - 2025-07-01 - Updated to use predefined CMake build variants instead of parsing Makefile
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="Crash Handler"
TEST_ABBR="BUG"
TEST_NUMBER="13"
TEST_VERSION="6.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# More test configuration
TEST_CONFIG="${CONFIG_DIR}/hydrogen_test_13_crash_test.json"
STARTUP_TIMEOUT=10    
CRASH_TIMEOUT=5

# Global caches
declare CORE_PATTERN=""
declare CORE_LIMIT=""
declare -A DEBUG_SYMBOL_CACHE
declare -A FOUND_BUILDS 
declare -a BUILDS
declare -A BUILD_DESCRIPTIONS

# Function to verify core dump configuration
verify_core_dump_config() {
    # print_message "Checking core dump configuration..."
    
    # Cache core pattern and limit if not already set
    [[ -z "${CORE_PATTERN}" ]] && CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || echo "unknown")
    [[ -z "${CORE_LIMIT}" ]] && CORE_LIMIT=$(ulimit -c)
    
    # Check core pattern
    if [[ "${CORE_PATTERN}" == "core" ]]; then
        print_message "Core pattern is set to default 'core'"
    else
        print_warning "Core pattern is set to '${CORE_PATTERN}', might affect core dump location"
    fi
    
    # Check core dump size limit
    if [[ "${CORE_LIMIT}" == "unlimited" || "${CORE_LIMIT}" -gt 0 ]]; then
        print_message "Core dump size limit is adequate (${CORE_LIMIT})"
    else
        print_warning "Core dump size limit is too restrictive (${CORE_LIMIT})"
        # Try to set unlimited core dumps for this session
        ulimit -c unlimited
        CORE_LIMIT=$(ulimit -c)
        print_message "Attempted to set unlimited core dump size"
    fi
    
    return 0
}

# Function to verify core file content
verify_core_file_content() {
    local core_file="$1"
    local binary="$2"
    
    # print_message "Verifying core file contents..."
    
    # Check file size and verify it's a core file
    local core_size
    core_size=$(stat -c %s "${core_file}" 2>/dev/null || echo "0")
    if [[ "${core_size}" -lt 1024 ]]; then
        print_message "Core file is suspiciously small (${core_size} bytes)"
        return 1
    fi
    
    # Verify it's a valid core file
    readelf_output=$(readelf -h "${core_file}" 2>/dev/null)
    if echo "${readelf_output}" | grep -q "Core file"; then
        print_message "Valid core file found (${core_size} bytes)"
        return 0
    fi
    print_message "Not a valid core file"
    return 1
}

# Function to verify debug symbols are present
verify_debug_symbols() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local expect_symbols=1
    
    # Check cache first
    if [[ -n "${DEBUG_SYMBOL_CACHE[${binary_name}]}" ]]; then
        [[ "${DEBUG_SYMBOL_CACHE[${binary_name}]}" -eq 0 ]] && return 0
        return 1
    fi
    
    # Release and naked builds should not have debug symbols
    if [[ "${binary_name}" == *"release"* ]] || [[ "${binary_name}" == *"naked"* ]]; then
        expect_symbols=0
    fi
    
    # print_message "Checking debug symbols in ${binary_name}..."
    
    # Use readelf to check for debug symbols
    # shellcheck disable=SC2312 # Expect an empty return if no debugging is available
    if readelf --debug-dump=info "${binary}" 2>/dev/null | grep -q -m 1 "DW_AT_name"; then
        if [[ "${expect_symbols}" -eq 1 ]]; then
            # print_message "Debug symbols found in ${binary_name} (as expected)"
            DEBUG_SYMBOL_CACHE[${binary_name}]=0
            return 0
        fi
        # print_message "Debug symbols found in ${binary_name} (unexpected for release build)"
        DEBUG_SYMBOL_CACHE[${binary_name}]=1
        return 1
    fi
    if [[ "${expect_symbols}" -eq 1 ]]; then
        # print_message "No debug symbols found in ${binary_name} (symbols required)"
        DEBUG_SYMBOL_CACHE[${binary_name}]=1
        return 1
    fi
    # print_message "No debug symbols found in ${binary_name} (as expected for release build)"
    DEBUG_SYMBOL_CACHE[${binary_name}]=0
    return 0
}

# Function to verify core file exists and has correct name format
verify_core_file() {
    local binary="$1"
    local pid="$2"
    local binary_name
    binary_name=$(basename "${binary}")
    local expected_core="${PROJECT_DIR}/${binary_name}.core.${pid}"
    local timeout=${CRASH_TIMEOUT}
    local found=0
    local start_time="${SECONDS}"
    
    # print_message "Waiting for core file ${binary_name}.core.${pid}..."
    
    # Wait for core file to appear
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${timeout}" ]]; then
            break
        fi
        
        if [[ -f "${expected_core}" ]]; then
            found=1
            break
        fi
    done
    
    if [[ "${found}" -eq 1 ]]; then
        print_message "Core file ${binary_name}.core.${pid} found"
        return 0
    else
        print_message "Core file ${binary_name}.core.${pid} not found after ${timeout} seconds"
        # List any core files that might exist
        local core_files
        core_files=$(find "${PROJECT_DIR}" -name "*.core.*" -type f 2>/dev/null || echo "")
        if [[ -n "${core_files}" ]]; then
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
    temp_output="${LOG_PREFIX}${TIMESTAMP}_${build_name}.gdb"
    readelf --sections "${binary}" 2>/dev/null > "${temp_output}"
    if grep -q -m 1 ".debug_info" "${temp_output}"; then
        print_message "Debug info found in binary, using full output"
        gdb_flags=(-q -ex 'set print frame-arguments all' -ex 'set print object on')
    else
        print_message "No debug info found in binary, using limited output"
        gdb_flags=(-q -ex 'set print frame-arguments none' -ex 'set print object off')
    fi
    
    print_message "Analyzing core file with GDB..."
    gdb "${gdb_flags[@]}" -batch \
        -ex "set pagination off" \
        -ex "set print elements 0" \
        -ex "set height 0" \
        -ex "set width 0" \
        -ex "file ${binary}" \
        -ex "core-file ${core_file}" \
        -ex "thread" \
        -ex "info threads" \
        -ex "thread apply all bt full" \
        -ex "frame" \
        -ex "info registers" \
        -ex "info locals" \
        -ex "quit" > "${gdb_output_file}" 2> "${gdb_output_file}.stderr" || {
        print_message "GDB exited with error, checking output anyway..."
        cat "${gdb_output_file}.stderr" >> "${gdb_output_file}" 2>/dev/null || true
    }
    
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
        if [[ "${has_backtrace}" -eq 1 ]]; then
            print_message "GDB produced basic backtrace (expected for release-style build)"
            return 0
        fi
    elif [[ "${has_test_crash}" -eq 1 ]]; then
        print_message "GDB found test_crash_handler in backtrace"
        return 0
    else
        print_message "GDB failed to produce useful backtrace"
        print_message "GDB output preserved at: $(convert_to_relative_path "${gdb_output_file}" || true)"
        return 1
    fi
    
    print_message "GDB analysis failed to find expected patterns"
    return 1
}

# Function to wait for crash completion
wait_for_crash_completion() {
    local pid="$1"
    local timeout="$2"
    local start_time="${SECONDS}"
    
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        if ! ps -p "${pid}" > /dev/null 2>&1; then
            return 0
        fi
    done
}

# Function to run test with a specific build in parallel (writes results to files)
run_crash_test_parallel() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${binary_name}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${binary_name}.log"
    local gdb_output_file="${LOG_PREFIX}${TIMESTAMP}_${binary_name}.txt"
    
    # Clear files
    true > "${log_file}"
    true > "${result_file}"
    
    # Start hydrogen server in background with ASAN leak detection disabled
    ASAN_OPTIONS="detect_leaks=0" "${binary}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    echo "Starting ${binary} ${TEST_CONFIG} with PID ${hydrogen_pid}" > "${result_file}"

    # Wait for startup with active log monitoring
    local startup_start="${SECONDS}"
    local elapsed
    local startup_complete=false
    
    while true; do
        elapsed=$((SECONDS - startup_start))
        
        if [[ ${elapsed} -ge ${STARTUP_TIMEOUT} ]]; then
            echo "STARTUP_FAILED" > "${result_file}"
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
            return 1
        fi
        
        if ! ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            startup_complete=true
            break
        fi
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            startup_complete=true
            break
        fi
    done
    
    if [[ "${startup_complete}" != "true" ]] || ! ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        echo "STARTUP_FAILED" > "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        return 1
    else
      echo "Startup complete for ${binary} ${TEST_CONFIG} with PID ${hydrogen_pid}" > "${result_file}"
    fi
    
    # Send SIGUSR1 to trigger test crash handler
    kill -USR1 "${hydrogen_pid}"
    
    # Wait for process to crash
    if ! wait_for_crash_completion "${hydrogen_pid}" "${CRASH_TIMEOUT}"; then
        echo "CRASH_FAILED" > "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        return 1
    fi
    
    # Store PID for later analysis
    echo "PID=${hydrogen_pid}" >> "${result_file}"
    echo "SUCCESS" >> "${result_file}"
    
    # Wait for the process to fully exit
    wait "${hydrogen_pid}" 2>/dev/null || true
    
    # Perform GDB analysis if core file exists
    if verify_core_file "${binary}" "${hydrogen_pid}"; then
        local core_file="${PROJECT_DIR}/${binary_name}.core.${hydrogen_pid}"
        if verify_core_file_content "${core_file}" "${binary}"; then
            analyze_core_with_gdb "${binary}" "${core_file}" "${gdb_output_file}"
            echo "GDB_RESULT=$?" >> "${result_file}"
        else
            echo "GDB_RESULT=1" >> "${result_file}"
        fi
    else
        echo "GDB_RESULT=1" >> "${result_file}"
    fi
    
    return 0
}

# Function to analyze results from parallel crash test
analyze_parallel_results() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${binary_name}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${binary_name}.log"
    local gdb_output_file="${LOG_PREFIX}${TIMESTAMP}_${binary_name}.txt"
    
    # Check if result file exists
    if [[ ! -f "${result_file}" ]]; then
        print_result 1 "No result file found for ${binary_name}"
        return 1
    fi
    # print_message "tst: ..${result_file}"
    
    # Check if log file exists
    if [[ ! -f "${log_file}" ]]; then
        print_result 1 "No log file found for ${binary_name}"
        return 1
    fi
    # print_message "log: ..${log_file}"

    # Check if gdb file exists
    if [[ ! -f "${gdb_output_file}" ]]; then
        print_result 1 "No gdb file found for ${binary_name}"
        return 1
    fi
    # print_message "gdb: ..${gdb_output_file}"

    print_result 0 "All execution files found"

    # Read result file
    local result_status
    result_status=$(tail -n 2 "${result_file}" | head -n 1 || true)
    
    if [[ "${result_status}" != "SUCCESS" ]]; then
        print_message "${binary_name}: Test failed during execution (${result_status})"
        return 1
    fi
    
    # Extract PID from result file
    local hydrogen_pid
    hydrogen_pid=$(grep "PID=" "${result_file}" | cut -d'=' -f2 || true)
    
    if [[ -z "${hydrogen_pid}" ]]; then
        print_message "${binary_name}: Could not extract PID from result file"
        return 1
    fi
    
    # Extract GDB result
    local gdb_result
    gdb_result=$(grep "GDB_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1" || true)
    
    # Initialize result variables for this build
    local debug_result=1
    local core_result=1
    local log_result=1
    
    # Verify debug symbols
    if verify_debug_symbols "${binary}"; then
        debug_result=0
    fi
    
    # Verify core file creation
    if verify_core_file "${binary}" "${hydrogen_pid}"; then
        core_result=0
        
        # If core file exists, verify its contents
        local core_file="${PROJECT_DIR}/${binary_name}.core.${hydrogen_pid}"
        if ! verify_core_file_content "${core_file}" "${binary}"; then
            print_message "Core file exists but content verification failed"
            core_result=1
        fi
    fi
    
    # Verify crash handler log messages
    if grep -q -e "Received SIGUSR1" -e "Signal 11 received" "${log_file}" 2>/dev/null; then
        # print_message "Found crash handler messages in log"
        log_result=0
    else
        # print_message "Missing crash handler messages in log"
        log_result=1
    fi
    
    # Return results via global variables (since we need multiple return values)
    DEBUG_SYMBOL_RESULT=${debug_result}
    CORE_FILE_RESULT=${core_result}
    CRASH_LOG_RESULT=${log_result}
    GDB_ANALYSIS_RESULT=${gdb_result}
    
    # Return success only if all subtests passed
    if [[ "${debug_result}" -eq 0 ]] && [[ "${core_result}" -eq 0 ]] && [[ "${log_result}" -eq 0 ]] && [[ "${gdb_result}" -eq 0 ]]; then
        return 0
    else
        return 1
    fi
}

print_subtest "Validate Test Configuration File"

if validate_config_file "${TEST_CONFIG}"; then
    print_message "Using minimal configuration for crash testing"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

print_subtest "Verify Core Dump Configuration"

if ! verify_core_dump_config; then
    print_result 1 "Core dump configuration issues detected"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi
print_result 0 "Core dump configuration verified"
((PASS_COUNT++))

# Find all available builds
print_message "Discovering available Hydrogen builds..."

# Define expected build variants based on CMake targets
declare -a BUILD_VARIANTS=("hydrogen" "hydrogen_debug" "hydrogen_valgrind" "hydrogen_perf" "hydrogen_release" "hydrogen_coverage" "hydrogen_naked")
for target in "${BUILD_VARIANTS[@]}"; do
    if [[ -f "${PROJECT_DIR}/${target}" ]] && [[ -z "${FOUND_BUILDS[${target}]}" ]]; then
        FOUND_BUILDS[${target}]=1
        BUILDS+=("${PROJECT_DIR}/${target}")
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

if [[ ${#BUILDS[@]} -eq 0 ]]; then
    print_subtest "Find Hydrogen Builds"
    print_result 1 "No hydrogen builds found"

    EXIT_CODE=1
    
    # Export results and exit early
    print_test_completion "${TEST_NAME}"

    # Return status code if sourced, exit if run standalone
    if [[ "${ORCHESTRATION}" == "true" ]]; then
        return "${EXIT_CODE}"
    else    
        exit "${EXIT_CODE}"
    fi
fi

print_message "Found ${#BUILDS[@]} builds for testing:"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    print_message "  • ${build_name}: ${BUILD_DESCRIPTIONS[${build_name}]}"
done

# Test each build using parallel execution
declare -A BUILD_PASSED_SUBTESTS
declare -a PARALLEL_PIDS

print_message "Running crash tests in parallel for all builds..."

MAX_JOBS=$(nproc 2>/dev/null || echo 4)  # Default to 4 if nproc unavailable

# Start all builds in parallel with job limiting
for build in "${BUILDS[@]}"; do
    while (( $(jobs -r | wc -l || true) >= MAX_JOBS )); do
        wait -n  # Wait for any job to finish
    done
    build_name=$(basename "${build}")
    print_message "Starting parallel test for: ${build_name}"
    
    # Run parallel crash test in background
    run_crash_test_parallel "${build}" &
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
    
    print_subtest "Testing build: ${build_name} (${BUILD_DESCRIPTIONS[${build_name}]})"
    
    # Initialize subtest results for this build
    BUILD_PASSED_SUBTESTS[${build_name}]=0
    
    # Analyze the parallel test results
    analyze_parallel_results "${build}"
    
    print_subtest "Debug Symbols - ${build_name}"

    if [[ "${DEBUG_SYMBOL_RESULT}" -eq 0 ]]; then
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
    
    print_subtest "Core File Generation - ${build_name}"

    if [[ "${CORE_FILE_RESULT}" -eq 0 ]]; then
        print_result 0 "Core file generated successfully"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "Core file generation failed"
        EXIT_CODE=1
    fi
    
    print_subtest "Crash Handler Logging - ${build_name}"

    if [[ "${CRASH_LOG_RESULT}" -eq 0 ]]; then
        print_result 0 "Crash handler log messages verified"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "Crash handler log messages missing"
        EXIT_CODE=1
    fi
    
    print_subtest "GDB Analysis - ${build_name}"
    
    if [[ "${GDB_ANALYSIS_RESULT}" -eq 0 ]]; then
        print_result 0 "GDB backtrace analysis successful"
        ((PASS_COUNT++))
        ((BUILD_PASSED_SUBTESTS[${build_name}]++))
    else
        print_result 1 "GDB backtrace analysis failed"
        EXIT_CODE=1
    fi
    
    # Summary for this build
    if [[ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]]; then
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
    if [[ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]]; then
        ((successful_builds++))
    fi
done

print_message "Summary: ${successful_builds}/${#BUILDS[@]} builds passed all crash handler tests"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
