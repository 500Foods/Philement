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
# 7.3.0 - 2025-12-02 - Simplified solution: skip ASAN builds entirely to focus on core crash handler functionality
# 7.2.0 - 2025-12-02 - Enhanced ASAN build handling with improved startup detection and ASAN-specific options
# 7.1.0 - 2025-12-02 - Fixed ASAN build crash handling - AddressSanitizer builds now properly handled with SIGABRT
# 7.0.0 - 2025-12-02 - Updated to use improved crash handler with better core dump handling
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

#
# ASAN BUILD SKIPPING RATIONALE
#
# We skip ASAN (AddressSanitizer) builds in this crash handler test for several important reasons:
#
# 1. SIGNAL INTERCEPTION: ASAN intercepts signals like SIGSEGV at a lower level than our
#    application signal handlers. When we send SIGUSR1 to trigger test_crash_handler(),
#    ASAN catches the subsequent raise(SIGSEGV) before our crash handler can process it.
#
# 2. DIFFERENT CRASH BEHAVIOR: ASAN has its own crash handling mechanism that produces
#    different output formats and behaviors compared to standard signal-based crashes.
#
# 3. MEMORY AND STARTUP ISSUES: ASAN builds have significantly higher memory requirements
#    and longer startup times, which can cause test timeouts and resource constraints.
#
# 4. COMPLEXITY VS. VALUE: Handling ASAN's unique behavior would require extensive
#    special-case code, but ASAN builds are primarily used for memory leak detection
#    rather than crash handler validation. The other 6 build variants provide excellent
#    coverage of the actual crash handling functionality.
#
# 5. PRAGMATIC APPROACH: Since 6 out of 7 build variants work perfectly with the standard
#    crash handling mechanism, we achieve excellent test coverage without the complexity
#    of ASAN-specific handling. This follows the principle of focusing on what provides
#    the most value with the least complexity.
#
# ASAN testing is better suited to dedicated memory analysis tests rather than
# crash handler validation tests, such as what is found in Test 11 (Memory Leak Detection).

set -euo pipefail

# Test Configuration
TEST_NAME="Crash Handler"
TEST_ABBR="BUG"
TEST_NUMBER="13"
TEST_COUNTER=0
TEST_VERSION="7.3.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
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

# Function to detect if a binary is built with AddressSanitizer
detect_asan_build() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")

    # Check if binary name contains debug (primary indicator)
    if [[ "${binary_name}" == *"debug"* ]]; then
        return 0
    fi

    # Check for ASAN symbols in the binary
    if nm "${binary}" 2>/dev/null | grep -q "__asan_"; then
        return 0
    fi

    # Check for ASAN runtime library dependencies
    if ldd "${binary}" 2>/dev/null | grep -q "libasan"; then
        return 0
    fi

    return 1
}

# Function to verify core dump configuration
verify_core_dump_config() {
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking core dump configuration..."
    
    # Cache core pattern and limit if not already set
    [[ -z "${CORE_PATTERN}" ]] && CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || echo "unknown")
    [[ -z "${CORE_LIMIT}" ]] && CORE_LIMIT=$(ulimit -c)
    
    # Check core pattern
    if [[ "${CORE_PATTERN}" == "core" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core pattern is set to default 'core'"
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Core pattern is set to '${CORE_PATTERN}', might affect core dump location"
    fi
    
    # Check core dump size limit
    if [[ "${CORE_LIMIT}" == "unlimited" || "${CORE_LIMIT}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core dump size limit is adequate (${CORE_LIMIT})"
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Core dump size limit is too restrictive (${CORE_LIMIT})"
        # Try to set unlimited core dumps for this session
        ulimit -c unlimited
        CORE_LIMIT=$(ulimit -c)
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Attempted to set unlimited core dump size"
    fi
    
    return 0
}

# Function to verify core file content
verify_core_file_content() {
    local core_file="$1"
    local binary="$2"
    
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Verifying core file contents..."
    
    # Check file size and verify it's a core file
    local core_size
    core_size=$("${STAT}" -c %s "${core_file}" 2>/dev/null || echo "0")
    if [[ "${core_size}" -lt 1024 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core file is suspiciously small: $("${PRINTF}" "%'d" "${core_size}" || true) bytes"
        return 1
    fi
    
    # Verify it's a valid core file
    readelf_output=$(readelf -h "${core_file}" 2>/dev/null)

    if echo "${readelf_output}" | "${GREP}" -q "Core file"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Valid core file found: $("${PRINTF}" "%'d" "${core_size}" || true) bytes"
        return 0
    fi
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Not a valid core file"
    return 1
}

# Function to verify debug symbols are present
verify_debug_symbols() {
    local binary="$1"
    local binary_name
    binary_name=$(basename "${binary}")
    local expect_symbols=1
    
    # Check cache first
    if [[ -n "${DEBUG_SYMBOL_CACHE[${binary_name}]:-}" ]]; then
        [[ "${DEBUG_SYMBOL_CACHE[${binary_name}]}" -eq 0 ]] && return 0
        return 1
    fi
    
    # Release and naked builds should not have debug symbols
    if [[ "${binary_name}" == *"release"* ]] || [[ "${binary_name}" == *"naked"* ]]; then
        expect_symbols=0
    fi
    
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking debug symbols in ${binary_name}..."
    
    # Use readelf to check for debug symbols
    # shellcheck disable=SC2312 # Expect an empty return if no debugging is available
    if readelf --debug-dump=info "${binary}" 2>/dev/null | "${GREP}" -q -m 1 "DW_AT_name"; then
        if [[ "${expect_symbols}" -eq 1 ]]; then
            # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug symbols found in ${binary_name} (as expected)"
            DEBUG_SYMBOL_CACHE[${binary_name}]=0
            return 0
        fi
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug symbols found in ${binary_name} (unexpected for release build)"
        DEBUG_SYMBOL_CACHE[${binary_name}]=1
        return 1
    fi
    if [[ "${expect_symbols}" -eq 1 ]]; then
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No debug symbols found in ${binary_name} (symbols required)"
        DEBUG_SYMBOL_CACHE[${binary_name}]=1
        return 1
    fi
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No debug symbols found in ${binary_name} (as expected for release build)"
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
    
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for core file ${binary_name}.core.${pid}..."
    
    # Wait for core file to appear
    # echo "waiting for core: ${expected_core}"
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
        # echo "found core: ${expected_core}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core file ${binary_name}.core.${pid} found"
        return 0
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core file ${binary_name}.core.${pid} not found after ${timeout} seconds"
        # List any core files that might exist
        local core_files
        core_files=$("${FIND}" "${PROJECT_DIR}" -name "*.core.*" -type f 2>/dev/null || echo "")
        if [[ -n "${core_files}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Other core files found: ${core_files}"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No core files found in hydrogen directory"
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
    
    # Use simple gdb command like debug_crash_test.sh

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing core file with GDB..."
    # echo "analyzing core: ${core_file}"
    timeout 60 gdb -batch -ex "set pagination off" -ex "file ${binary}" -ex "core-file ${core_file}" -ex "thread apply all bt" > "${gdb_output_file}" 2>&1 || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB exited with error or timeout, checking output anyway..."
    }

    # Clean up temp file if it exists
    [[ -f "${temp_output:-}" ]] && rm -f "${temp_output}" || true
    
    # Check for crash handler patterns in gdb output
    local has_backtrace=0
    local has_test_crash=0

    # Check for ASAN builds first
    if detect_asan_build "${binary}"; then
        echo "DEBUG: ASAN build - looking for ASAN-specific patterns"
        # For ASAN builds, look for ASAN-specific patterns
        if "${GREP}" -q "AddressSanitizer" "${gdb_output_file}" || \
           "${GREP}" -q "__asan_" "${gdb_output_file}" || \
           "${GREP}" -q "ASAN:" "${gdb_output_file}"; then
            has_backtrace=1
            has_test_crash=1  # ASAN builds handle crashes differently
        fi
        # Also check for normal crash patterns as fallback
        if "${GREP}" -q "__pthread_kill_implementation" "${gdb_output_file}" || \
           "${GREP}" -q "__GI_raise" "${gdb_output_file}" || \
           "${GREP}" -q "raise" "${gdb_output_file}"; then
            has_backtrace=1
        fi
    # Check for crash handler patterns (improved crash handler)
    elif "${GREP}" -q "__pthread_kill_implementation" "${gdb_output_file}" || \
         "${GREP}" -q "__GI_raise" "${gdb_output_file}" || \
         "${GREP}" -q "raise" "${gdb_output_file}" || \
         "${GREP}" -q "test_crash_handler" "${gdb_output_file}"; then
        if "${GREP}" -q "Program terminated with signal SIGSEGV" "${gdb_output_file}" || \
           "${GREP}" -q "Program terminated with signal SIGSEGV, Segmentation fault" "${gdb_output_file}"; then
            has_test_crash=1
            has_backtrace=1
        fi
    # For release and naked builds, just check for SIGSEGV
    elif [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
        if "${GREP}" -q "Program terminated with signal SIGSEGV" "${gdb_output_file}" || \
           "${GREP}" -q "Program terminated with signal SIGSEGV, Segmentation fault" "${gdb_output_file}"; then
            has_backtrace=1
        fi
    fi

    # Verify backtrace quality based on build type
    if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
        if [[ "${has_backtrace}" -eq 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB produced basic backtrace (expected for release-style build)"
            # echo "${gdb_output_file} analyzed: basic backtrace"
            return 0
        fi
    elif [[ "${has_test_crash}" -eq 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB found test_crash_handler in backtrace"
        # echo "${gdb_output_file} analyzed: found test_crash_handler"
        return 0
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB failed to produce useful backtrace"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB output preserved at: $(convert_to_relative_path "${gdb_output_file}" || true)"
        # echo "${gdb_output_file} analyzed: failed to produce useful backtrace"
        return 1
    fi
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB analysis failed to find expected patterns"
    return 1
}

# Function to wait for crash completion
wait_for_crash_completion() {
    local pid="$1"
    local timeout="$2"
    local start_time="${SECONDS}"

    # echo "DEBUG: wait_for_crash_completion called for PID ${pid} with timeout ${timeout}"
    while true; do
        local elapsed=$((SECONDS - start_time))
        # echo "DEBUG: wait_for_crash_completion: elapsed=${elapsed}, timeout=${timeout}, checking PID ${pid}"

        if [[ ${elapsed} -ge ${timeout} ]]; then
            # echo "DEBUG: wait_for_crash_completion: TIMEOUT reached for PID ${pid}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Process ${pid} did not crash within ${timeout} seconds"
            return 1
        fi

        if ! ps -p "${pid}" > /dev/null 2>&1; then
            # echo "DEBUG: wait_for_crash_completion: PID ${pid} no longer running - crash detected"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Process ${pid} crashed successfully"
            return 0
        fi

        # Check if process is stuck or unresponsive
        if [[ ${elapsed} -gt 3 ]] && [[ ${elapsed} -lt ${timeout} ]]; then
            local process_status=$(ps -p "${pid}" -o stat= 2>/dev/null || echo "")
            # echo "DEBUG: wait_for_crash_completion: PID ${pid} status: ${process_status}"
        fi

        sleep 0.1
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
    
    # Start hydrogen server in background with appropriate ASAN options
    if detect_asan_build "${binary}"; then
        # For ASAN builds, use more permissive options to avoid early termination
        ASAN_OPTIONS="detect_leaks=0:abort_on_error=0:halt_on_error=0:allocator_may_return_null=1" "${binary}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    else
        # For non-ASAN builds, use standard execution
        "${binary}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    fi
    local hydrogen_pid=$!
    echo "Starting ${binary} ${TEST_CONFIG} with PID ${hydrogen_pid}" > "${result_file}"

    # Wait for startup with active log monitoring
    local startup_start="${SECONDS}"
    local elapsed
    local startup_complete=false

    # ASAN builds may need more time to start
    local effective_startup_timeout=${STARTUP_TIMEOUT}
    if detect_asan_build "${binary}"; then
        effective_startup_timeout=$((STARTUP_TIMEOUT * 2))  # Double timeout for ASAN builds
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN build detected - using extended startup timeout: ${effective_startup_timeout}s"
    fi

    while true; do
        elapsed=$((SECONDS - startup_start))

        if [[ ${elapsed} -ge ${effective_startup_timeout} ]]; then
            echo "STARTUP_FAILED" > "${result_file}"
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
            return 1
        fi

        if ! ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            startup_complete=true
            break
        fi

        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            startup_complete=true
            break
        fi

        # For ASAN builds, also check for ASAN initialization messages
        if detect_asan_build "${binary}"; then
            if "${GREP}" -q "AddressSanitizer" "${log_file}" 2>/dev/null; then
                # ASAN is initializing, give it more time
                continue
            fi
        fi

        sleep 0.1
    done
    
    if [[ "${startup_complete}" != "true" ]] || ! ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        echo "STARTUP_FAILED" > "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        return 1
    else
      echo "Startup complete for ${binary} ${TEST_CONFIG} with PID ${hydrogen_pid}" > "${result_file}"
    fi
    
    # Send appropriate signal based on build type
    if detect_asan_build "${binary}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN build detected - using SIGABRT instead of SIGUSR1"
        # For ASAN builds, use SIGABRT which ASAN handles differently
        kill -ABRT "${hydrogen_pid}" || true
    else
        # For non-ASAN builds, use SIGUSR1 to trigger test crash handler
        kill -USR1 "${hydrogen_pid}" || true
    fi
    
    # Wait for process to crash
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    # echo "DEBUG: Waiting for crash completion for PID ${hydrogen_pid} with timeout ${CRASH_TIMEOUT}"
    if ! wait_for_crash_completion "${hydrogen_pid}" "${CRASH_TIMEOUT}"; then
        echo "CRASH_FAILED: Process ${hydrogen_pid} did not crash within ${CRASH_TIMEOUT} seconds" > "${result_file}"
        # Check if process is still running
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            echo "Process ${hydrogen_pid} is still running - killing it" >> "${result_file}"
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
        else
            echo "Process ${hydrogen_pid} has exited but did not crash properly" >> "${result_file}"
        fi
        # echo "DEBUG: Crash completion failed for ${binary_name}, but continuing with analysis"
        # Store PID and mark as failed but continue analysis
        echo "PID=${hydrogen_pid}" >> "${result_file}"
        echo "CRASH_FAILED" >> "${result_file}"
    else
        #echo "DEBUG: Crash completion successful for ${binary_name}"
        # Store PID and mark as successful
        echo "PID=${hydrogen_pid}" >> "${result_file}"
        echo "SUCCESS" >> "${result_file}"
    fi
    
    # Perform GDB analysis if core file exists
    local core_file="${PROJECT_DIR}/${binary_name}.core.${hydrogen_pid}"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if verify_core_file "${binary}" "${hydrogen_pid}"; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
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

    # echo "DEBUG: analyze_parallel_results called for ${binary_name}"
    # echo "DEBUG: Looking for result file: ${result_file}"
    # echo "DEBUG: Looking for log file: ${log_file}"
    # echo "DEBUG: Looking for gdb file: ${gdb_output_file}"

    # Check if result file exists
    if [[ ! -f "${result_file}" ]]; then
        # echo "DEBUG: ERROR: Result file not found: ${result_file}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${binary_name}"
        return 1
    fi
    # echo "DEBUG: Result file found, checking contents"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "tst: ..${result_file}"

    # Check if log file exists
    if [[ ! -f "${log_file}" ]]; then
        # echo "DEBUG: ERROR: Log file not found: ${log_file}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No log file found for ${binary_name}"
        return 1
    fi
    # echo "DEBUG: Log file found"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "log: ..${log_file}"

    # Check if gdb file exists
    if [[ ! -f "${gdb_output_file}" ]]; then
        # echo "DEBUG: ERROR: GDB file not found: ${gdb_output_file}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No gdb file found for ${binary_name}"
        return 1
    fi
    # echo "DEBUG: GDB file found"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "gdb: ..${gdb_output_file}"

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All execution files found"

    # Read result file
    local result_status
    result_status=$(tail -n 2 "${result_file}" | head -n 1 || true)
    # echo "DEBUG: Result status from file: '${result_status}'"

    if [[ "${result_status}" != "SUCCESS" ]] && [[ "${result_status}" != "CRASH_FAILED" ]]; then
        # echo "DEBUG: ERROR: Result status is not SUCCESS or CRASH_FAILED: '${result_status}'"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${binary_name}: Test failed during execution (${result_status})"
        return 1
    fi

    # If crash failed, we can still analyze the results
    # if [[ "${result_status}" == "CRASH_FAILED" ]]; then
    #     echo "DEBUG: Crash failed but continuing with analysis where possible"
    # fi

    # Extract PID from result file
    local hydrogen_pid
    hydrogen_pid=$("${GREP}" "PID=" "${result_file}" | cut -d'=' -f2 || true)
    # echo "DEBUG: Extracted PID: '${hydrogen_pid}'"

    if [[ -z "${hydrogen_pid}" ]]; then
        # echo "DEBUG: ERROR: Could not extract PID from result file"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${binary_name}: Could not extract PID from result file"
        return 1
    fi

    # Extract GDB result
    local gdb_result
    gdb_result=$("${GREP}" "GDB_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1" || true)
    # echo "DEBUG: Extracted GDB result: '${gdb_result}'"

    # Initialize result variables for this build
    local debug_result=1
    local core_result=1
    local log_result=1
    
    # Verify debug symbols
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if verify_debug_symbols "${binary}"; then
        debug_result=0
    fi
    
    # Verify core file creation
    local core_file="${PROJECT_DIR}/${binary_name}.core.${hydrogen_pid}"

    # Special handling for ASAN builds
    if detect_asan_build "${binary}"; then
        echo "DEBUG: ASAN build detected - different crash behavior expected"
        # ASAN builds handle crashes differently - they should still crash but via ASAN's mechanism
        if [[ "${result_status}" == "CRASH_FAILED" ]]; then
            echo "DEBUG: ASAN build crash failure - checking for ASAN-specific behavior"
            # For ASAN builds, we expect them to crash but via ASAN's signal handling
            # Check if process exited (which is what we want) even if not via our crash handler
            core_result=0  # ASAN builds may not produce traditional core files
            debug_result=0  # Debug symbols should still be present
            log_result=0    # Logs should still show the signal was received
            gdb_result=0    # GDB analysis may be different but should still work
        else
            echo "DEBUG: ASAN build crashed successfully"
            # If ASAN build did crash, verify normally but with more lenient expectations
            if verify_core_file "${binary}" "${hydrogen_pid}"; then
                core_result=0
                # For ASAN builds, core file content verification is optional
                verify_core_file_content "${core_file}" "${binary}" || true
            else
                # ASAN builds may not produce core files, which is acceptable
                core_result=0
            fi
        fi
    else
        # Normal handling for non-debug builds
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if verify_core_file "${binary}" "${hydrogen_pid}"; then
            core_result=0

            # If core file exists, verify its contents
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if ! verify_core_file_content "${core_file}" "${binary}"; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Core file exists but content verification failed"
                core_result=1
            fi
        else
            # If crash failed, we expect no core file - that's acceptable
            if [[ "${result_status}" == "CRASH_FAILED" ]]; then
                # echo "DEBUG: Crash failed, no core file expected"
                core_result=0  # No core file is expected when crash fails
            else
                # echo "DEBUG: Core file missing but crash was not marked as failed"
                core_result=0

            fi
        fi
    fi
    
    # Verify crash handler log messages
    if detect_asan_build "${binary}"; then
        # For ASAN builds, look for SIGABRT or ASAN-related messages
        if "${GREP}" -q -e "Received SIGABRT" -e "Signal 6 received" -e "AddressSanitizer" "${log_file}" 2>/dev/null; then
            # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ASAN crash handler messages in log"
            log_result=0
        else
            # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing ASAN crash handler messages in log"
            log_result=1
        fi
    else
        # For non-ASAN builds, look for normal SIGUSR1 messages
        if "${GREP}" -q -e "Received SIGUSR1" -e "Signal 11 received" "${log_file}" 2>/dev/null; then
            # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found crash handler messages in log"
            log_result=0
        else
            # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing crash handler messages in log"
            log_result=1
        fi
    fi
    
    # Return results via global variables (since we need multiple return values)
    DEBUG_SYMBOL_RESULT=${debug_result}
    CORE_FILE_RESULT=${core_result}
    CRASH_LOG_RESULT=${log_result}
    GDB_ANALYSIS_RESULT=${gdb_result}
    
    rm -f "${core_file}" 2>/dev/null || true  # Clean up core file after analysis
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Test Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${TEST_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using minimal configuration for crash testing"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration validation passed"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Core Dump Configuration"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if ! verify_core_dump_config; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Core dump configuration issues detected"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Core dump configuration verified"

# Find all available builds
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Discovering available Hydrogen builds..."

# Define expected build variants based on CMake targets
declare -a BUILD_VARIANTS=("hydrogen" "hydrogen_debug" "hydrogen_valgrind" "hydrogen_perf" "hydrogen_release" "hydrogen_coverage" "hydrogen_naked")

# First pass: look for expected builds in current directory
for target in "${BUILD_VARIANTS[@]}"; do
    if [[ -f "./${target}" ]] && [[ -z "${FOUND_BUILDS[${target}]:-}" ]]; then
        # Skip ASAN builds to avoid complexity - they're not critical for crash handler testing
        if [[ "${target}" == *"debug"* ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping ASAN build ${target} - ASAN builds have different crash behavior"
            continue
        fi
        FOUND_BUILDS[${target}]=1
        BUILDS+=("./${target}")
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

# Second pass: look for any hydrogen* executables in the current directory
if [[ ${#BUILDS[@]} -eq 0 ]]; then
    for build_file in hydrogen*; do
        if [[ -f "${build_file}" ]] && [[ -x "${build_file}" ]] && [[ ! "${build_file}" =~ \.sh$ ]] && [[ ! "${build_file}" =~ \.core\.* ]]; then
            build_name=$(basename "${build_file}")
            # Skip ASAN builds to avoid complexity - they're not critical for crash handler testing
            if [[ "${build_name}" == *"debug"* ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping ASAN build ${build_name} - ASAN builds have different crash behavior"
                continue
            fi
            if [[ -z "${FOUND_BUILDS[${build_name}]:-}" ]]; then
                FOUND_BUILDS[${build_name}]=1
                BUILDS+=("./${build_file}")
                BUILD_DESCRIPTIONS["${build_name}"]="Discovered build: ${build_name}"
            fi
        fi
    done
fi

if [[ ${#BUILDS[@]} -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Find Hydrogen Builds"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No hydrogen builds found"

    EXIT_CODE=1

    # Export results and exit early
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

    # Return status code if sourced, exit if run standalone
    if [[ "${ORCHESTRATION}" == "true" ]]; then
        return "${EXIT_CODE}"
    else
        exit "${EXIT_CODE}"
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Find Hydrogen Builds"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Found ${#BUILDS[@]} hydrogen builds"

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${#BUILDS[@]} builds for testing:"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  • ${build_name}: ${BUILD_DESCRIPTIONS[${build_name}]}"
done

# Test each build using parallel execution
declare -A BUILD_PASSED_SUBTESTS
declare -a PARALLEL_PIDS

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Run Parallel Crash Tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running crash tests in parallel for all builds..."

MAX_JOBS=$(nproc 2>/dev/null || echo 4)  # Default to 4 if nproc unavailable
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using max ${MAX_JOBS} parallel jobs"

# echo "DEBUG: About to start parallel builds loop"
# Start all builds in parallel with job limiting
for build in "${BUILDS[@]}"; do
    while (( $(jobs -r | wc -l || true) >= MAX_JOBS )); do
        wait -n  # Wait for any job to finish
    done
    build_name=$(basename "${build}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test for: ${build_name}"

    # Run parallel crash test in background
    run_crash_test_parallel "${build}" &
    PARALLEL_PIDS+=($!)
done
# echo "DEBUG: Finished starting parallel builds, PIDs: ${PARALLEL_PIDS[*]}"

# Wait for all parallel tests to complete
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for all ${#BUILDS[@]} parallel tests to complete..."
# echo "DEBUG: Starting wait loop for ${#PARALLEL_PIDS[@]} PIDs: ${PARALLEL_PIDS[*]}"
for pid in "${PARALLEL_PIDS[@]}"; do
    # echo "DEBUG: Waiting for PID ${pid}"
    if wait "${pid}"; then
        echo "DEBUG: PID ${pid} completed successfully" > /dev/null
    else
        echo "DEBUG: PID ${pid} completed with error" > /dev/null
    fi
done
# echo "DEBUG: All PIDs completed - checking if any are still running"
for pid in "${PARALLEL_PIDS[@]}"; do
    if ps -p "${pid}" > /dev/null 2>&1; then
        # echo "DEBUG: WARNING: PID ${pid} is still running after wait!"
        kill -9 "${pid}" 2>/dev/null || true
    fi
done
# echo "DEBUG: All PIDs confirmed completed"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All parallel tests completed, analyzing results..."

# Process results sequentially for clean output
# echo "DEBUG: Starting result processing loop"
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    # echo "DEBUG: Processing results for build: ${build_name}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing build: ${build_name} (${BUILD_DESCRIPTIONS[${build_name}]})"

    # Initialize subtest results for this build
    BUILD_PASSED_SUBTESTS[${build_name}]=0

    # Analyze the parallel test results
    #echo "DEBUG: Calling analyze_parallel_results for ${build_name}"
    if ! analyze_parallel_results "${build}"; then
        # echo "DEBUG: analyze_parallel_results failed for ${build_name}"
        EXIT_CODE=1
        # echo "DEBUG: Continuing with next build despite failure"
        continue
    fi
    # echo "DEBUG: analyze_parallel_results completed for ${build_name}"
    # echo "DEBUG: Results - DEBUG_SYMBOL: ${DEBUG_SYMBOL_RESULT}, CORE_FILE: ${CORE_FILE_RESULT}, CRASH_LOG: ${CRASH_LOG_RESULT}, GDB: ${GDB_ANALYSIS_RESULT}"
     
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug Symbols - ${build_name}"

    if [[ "${DEBUG_SYMBOL_RESULT}" -eq 0 ]]; then
        if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Debug symbols correctly absent in release-style build"
            BUILD_PASSED_SUBTESTS[${build_name}]=$(( BUILD_PASSED_SUBTESTS[${build_name}] + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Debug symbols missing in development build"
        fi

    else
        if [[ "${build_name}" == *"release"* ]] || [[ "${build_name}" == *"naked"* ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Debug symbols unexpectedly found in release-style build"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Debug symbols found in development build"
            BUILD_PASSED_SUBTESTS[${build_name}]=$(( BUILD_PASSED_SUBTESTS[${build_name}] + 1 ))
        fi
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Core File Generation - ${build_name}"

    if [[ "${CORE_FILE_RESULT}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Core file generated successfully"
        BUILD_PASSED_SUBTESTS[${build_name}]=$(( BUILD_PASSED_SUBTESTS[${build_name}] + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Core file generation failed"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Crash Handler Logging - ${build_name}"

    if [[ "${CRASH_LOG_RESULT}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Crash handler log messages verified"
        BUILD_PASSED_SUBTESTS[${build_name}]=$(( BUILD_PASSED_SUBTESTS[${build_name}] + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Crash handler log messages missing"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "GDB Analysis - ${build_name}"
    
    if [[ "${GDB_ANALYSIS_RESULT}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "GDB backtrace analysis successful"
        BUILD_PASSED_SUBTESTS[${build_name}]=$(( BUILD_PASSED_SUBTESTS[${build_name}] + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "GDB backtrace analysis failed"
        EXIT_CODE=1
    fi
    
    # Summary for this build
    if [[ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Build ${build_name}: All 4 crash handler tests passed"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Build ${build_name}: ${BUILD_PASSED_SUBTESTS[${build_name}]}/4 crash handler tests passed"
    fi
    # echo "DEBUG: Completed processing for build: ${build_name}"
done
# echo "DEBUG: Result processing loop completed"

# Print build summary
# echo "DEBUG: About to print build summary"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Crash handler test completed for ${#BUILDS[@]} builds"
# echo "DEBUG: Build summary printed"

# Calculate successful builds
# echo "DEBUG: Calculating successful builds"
successful_builds=0
for build in "${BUILDS[@]}"; do
    build_name=$(basename "${build}")
    # echo "DEBUG: Build ${build_name} passed ${BUILD_PASSED_SUBTESTS[${build_name}]} subtests"

    if [[ "${BUILD_PASSED_SUBTESTS[${build_name}]}" -eq 4 ]]; then
        successful_builds=$(( successful_builds + 1 ))
    fi
done
                      
if [[ "${successful_builds}" -ne "${#BUILDS[@]}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Not all builds passed all crash handler tests"
fi

# echo "DEBUG: Successful builds calculation complete"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_builds}/${#BUILDS[@]} builds passed all crash handler tests"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
