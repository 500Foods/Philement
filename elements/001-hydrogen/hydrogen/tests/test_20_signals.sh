#!/bin/bash

# Test: Signal Handling
# Tests signal handling capabilities including SIGINT, SIGTERM, SIGHUP, SIGUSR2, and multiple signals

# FUNCTIONS
# run_signal_test_parallel()
# analyze_signal_test_results()
# verify_clean_shutdown()
# verify_single_restart()
# verify_multi_restart()
# verify_crash_dump()
# verify_config_dump()
# verify_multi_signals()

# CHANGELOG
# 6.0.0 - 2025-08-09 - Complete refactor using lifecycle.sh and parallel execution pattern from Test 13
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.0 - 2025-07-30 - Shellcheck overhaul, general review
# 3.0.4 - 2025-07-15 - No more sleep
# 3.0.3 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.1 - 2025-07-02 - Performance optimization: removed all artificial delays (sleep statements) for dramatically faster execution while maintaining reliability
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic signal handling test

# Test configuration
TEST_NAME="Signal Handling"
TEST_ABBR="SIG"
TEST_NUMBER="20"
TEST_VERSION="6.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
TEST_CONFIG="${CONFIG_DIR}/hydrogen_test_20_signals.json"
TEST_CONFIG_BASE=$(basename "${TEST_CONFIG}")

# Signal test configuration - format: "signal:action:description:validation_func:cleanup_signal"
declare -A SIGNAL_TESTS=(
    ["SIGINT"]="SIGINT:terminate:Ctrl+C:verify_clean_shutdown:SIGINT"
    ["SIGTERM"]="SIGTERM:terminate:Terminate:verify_clean_shutdown:SIGTERM"
    ["SIGHUP_SINGLE"]="SIGHUP:restart:Single Restart:verify_single_restart:SIGTERM"
    ["SIGHUP_MULTI"]="SIGHUP:restart:Many Restarts:verify_multi_restart:SIGTERM"
    ["SIGUSR1"]="SIGUSR1:crash:Crash Dump:verify_crash_dump:SIGUSR1"
    ["SIGUSR2"]="SIGUSR2:config:Config Dump:verify_config_dump:SIGTERM"
    ["MULTI"]="MULTIPLE:terminate:Multiple Signals:verify_multi_signals:MULTIPLE"
)

# Signal test timeouts
RESTART_COUNT=5
STARTUP_TIMEOUT=5
SHUTDOWN_TIMEOUT=10
SIGNAL_TIMEOUT=15

# Parallel execution configuration
MAX_JOBS=$(nproc 2>/dev/null || echo 4)
declare -a PARALLEL_PIDS

print_subtest "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "Validate Test Configuration File"

if validate_config_file "${TEST_CONFIG}"; then
    print_message "Using minimal configuration for signal testing"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Validation functions for different signal types
verify_clean_shutdown() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local start_time
    start_time=${SECONDS}
    
    # Wait for process to exit
    while ps -p "${pid}" > /dev/null 2>&1; do
        if [[ $((SECONDS - start_time)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
            echo "SHUTDOWN_TIMEOUT" >> "${result_file}"
            return 1
        fi
        sleep 0.1
    done
    
    # Check for clean shutdown message
    if grep -q "SHUTDOWN COMPLETE" "${log_file}" 2>/dev/null; then
        echo "SHUTDOWN_SUCCESS" >> "${result_file}"
        return 0
    else
        echo "SHUTDOWN_FAILED" >> "${result_file}"
        return 1
    fi
}

verify_single_restart() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local start_time
    start_time=${SECONDS}
    
    # Wait for restart signal to be logged
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "RESTART_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if grep -q "SIGHUP received" "${log_file}" 2>/dev/null; then
            break
        fi
        sleep 0.1
    done
    
    # Wait for restart completion (simplified to match multi-restart logic)
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "RESTART_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        # Check for restart completion - either message works
        if (grep -q "Application restarts: 1" "${log_file}" 2>/dev/null || \
            grep -q "Restart count: 1" "${log_file}" 2>/dev/null || \
            grep -q "Restart completed successfully" "${log_file}" 2>/dev/null); then
            echo "RESTART_SUCCESS" >> "${result_file}"
            return 0
        fi
        sleep 0.1
    done
}

verify_multi_restart() {
    local pid="$1" 
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local current_count=1
    
    while [[ "${current_count}" -le "${RESTART_COUNT}" ]]; do
        # Send SIGHUP signal
        kill -SIGHUP "${pid}" || true
        
        # Wait for this restart to complete
        local start_time
        start_time=${SECONDS}
        local restart_completed=false
        
        while true; do
            if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
                echo "MULTI_RESTART_TIMEOUT_${current_count}" >> "${result_file}"
                return 1
            fi
            
            if (grep -q "Application restarts: ${current_count}" "${log_file}" 2>/dev/null || \
                grep -q "Restart count: ${current_count}" "${log_file}" 2>/dev/null); then
                restart_completed=true
                break
            fi
            sleep 0.1
        done
        
        if [[ "${restart_completed}" = true ]]; then
            if [[ "${current_count}" -eq "${RESTART_COUNT}" ]]; then
                echo "MULTI_RESTART_SUCCESS" >> "${result_file}"
                return 0
            fi
            current_count=$((current_count + 1))
        else
            echo "MULTI_RESTART_FAILED_${current_count}" >> "${result_file}"
            return 1
        fi
    done
    
    echo "MULTI_RESTART_FAILED" >> "${result_file}"
    return 1
}

verify_crash_dump() {
    local pid="$1"
    local log_file="$2" 
    local signal="$3"
    local result_file="$4"
    local binary_name
    binary_name=$(basename "${HYDROGEN_BIN}")
    local expected_core="${PROJECT_DIR}/${binary_name}.core.${pid}"
    local timeout="${SIGNAL_TIMEOUT}"
    local start_time
    start_time=${SECONDS}
    
    # Wait for core file to appear
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${timeout}" ]]; then
            echo "CRASH_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if [[ -f "${expected_core}" ]]; then
            echo "CRASH_DUMP_SUCCESS" >> "${result_file}"
            rm -f "${expected_core}" # Clean up
            return 0
        fi
        sleep 0.1
    done
}

verify_config_dump() {
    local pid="$1"
    local log_file="$2"
    local signal="$3" 
    local result_file="$4"
    local start_time
    start_time=${SECONDS}
    
    # Wait for dump to start
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "CONFIG_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Started" "${log_file}" 2>/dev/null; then
            break
        fi
        sleep 0.1
    done
    
    # Wait for dump to complete
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "CONFIG_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Complete" "${log_file}" 2>/dev/null; then
            echo "CONFIG_DUMP_SUCCESS" >> "${result_file}"
            return 0
        fi
        sleep 0.1
    done
}

verify_multi_signals() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    
    # Send multiple signals rapidly
    kill -SIGTERM "${pid}" 2>/dev/null || true
    kill -SIGINT "${pid}" 2>/dev/null || true
    
    # Wait for shutdown
    local start_time
    start_time=${SECONDS}
    while ps -p "${pid}" > /dev/null 2>&1; do
        if [[ $((SECONDS - start_time)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
            echo "MULTI_SIGNAL_TIMEOUT" >> "${result_file}"
            return 1
        fi
        sleep 0.1
    done
    
    # Check for single shutdown sequence
    local shutdown_count
    shutdown_count=$(grep -c "Initiating graceful shutdown sequence" "${log_file}" 2>/dev/null || echo "0")
    if [[ "${shutdown_count}" -eq 1 ]]; then
        echo "MULTI_SIGNAL_SUCCESS" >> "${result_file}"
        return 0
    else
        echo "MULTI_SIGNAL_MULTIPLE_SEQUENCES" >> "${result_file}"
        return 1
    fi
}

# Function to run a single signal test in parallel (background process)
run_signal_test_parallel() {
    local test_name="$1"
    local signal="$2"
    local action="$3"
    local description="$4"
    local validation_func="$5"
    local cleanup_signal="$6"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.log"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.result"
    
    # Clear result file
    true > "${result_file}"
    
    # Start hydrogen server directly
    "${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}
    
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.1
    done
    
    if [[ "${startup_success}" = true ]]; then
        echo "PID=${hydrogen_pid}" >> "${result_file}"
        echo "STARTUP_SUCCESS" >> "${result_file}"
        
        # Send the signal (except for multi-signal test which handles its own signaling)
        if [[ "${signal}" != "MULTIPLE" ]]; then
            kill -"${signal}" "${hydrogen_pid}" 2>/dev/null || true
        fi
        
        # Validate signal behavior using specific validation function
        if "${validation_func}" "${hydrogen_pid}" "${log_file}" "${signal}" "${result_file}"; then
            echo "VALIDATION_SUCCESS" >> "${result_file}"
        else
            echo "VALIDATION_FAILED" >> "${result_file}"
        fi
        
        # Clean up if needed (for restart tests, we need to stop the server)
        if [[ "${cleanup_signal}" != "${signal}" ]] && [[ "${cleanup_signal}" != "MULTIPLE" ]]; then
            if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
                kill -"${cleanup_signal}" "${hydrogen_pid}" 2>/dev/null || true
                # Simple shutdown wait
                local shutdown_start
                shutdown_start=${SECONDS}
                while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                    if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                        kill -9 "${hydrogen_pid}" 2>/dev/null || true
                        break
                    fi
                    sleep 0.1
                done
            fi
        fi
        
        echo "TEST_COMPLETE" >> "${result_file}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel signal test execution
analyze_signal_test_results() {
    local test_name="$1"
    local signal="$2"
    local description="$3"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.result"
    
    if [[ ! -f "${result_file}" ]]; then
        print_result 1 "No result file found for ${test_name}"
        return 1
    fi
    
    # Check startup
    if ! grep -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi
    
    # Check validation
    if grep -q "VALIDATION_SUCCESS" "${result_file}" 2>/dev/null; then
        return 0
    else
        # Check for specific failure reasons
        if grep -q "SHUTDOWN_FAILED" "${result_file}" 2>/dev/null; then
            print_result 1 "${signal} handling failed - no clean shutdown"
        elif grep -q "RESTART_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result 1 "${signal} restart failed or timed out"
        elif grep -q "CRASH_DUMP_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result 1 "${signal} failed to generate crash dump"
        elif grep -q "CONFIG_DUMP_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result 1 "${signal} handling failed - no config dump"
        elif grep -q "MULTI_SIGNAL_MULTIPLE_SEQUENCES" "${result_file}" 2>/dev/null; then
            print_result 1 "Multiple shutdown sequences detected"
        else
            print_result 1 "${signal} handling failed"
        fi
        return 1
    fi
}

print_message "Running signal tests in parallel for faster execution..."

# Start all signal tests in parallel with job limiting  
for test_config in "${!SIGNAL_TESTS[@]}"; do
    # shellcheck disable=SC2312 # Gettin fancy with the functions
    while (( $(jobs -r | wc -l) >= MAX_JOBS )); do
        wait -n  # Wait for any job to finish
    done
    
    # Parse test configuration
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"
    
    print_message "Starting parallel test: ${test_config} (${description})"
    
    # Run parallel signal test in background
    run_signal_test_parallel "${test_config}" "${signal}" "${action}" "${description}" "${validation_func}" "${cleanup_signal}" &
    PARALLEL_PIDS+=($!)
done

# Wait for all parallel tests to complete
print_message "Waiting for all ${#SIGNAL_TESTS[@]} parallel signal tests to complete..."
for pid in "${PARALLEL_PIDS[@]}"; do
    wait "${pid}"
done
print_message "All parallel tests completed, analyzing results..."

# Process results sequentially for clean output
for test_config in "${!SIGNAL_TESTS[@]}"; do
    # Parse test configuration
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"
    
    print_subtest "${signal} Signal Handling (${description})"
    print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"
    
    if analyze_signal_test_results "${test_config}" "${signal}" "${description}"; then
        print_result 0 "${signal} handled successfully"
        ((PASS_COUNT++))
    else
        EXIT_CODE=1
    fi
done

# Print summary
successful_tests=0
for test_config in "${!SIGNAL_TESTS[@]}"; do
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"
    result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_config}.result"
    if [[ -f "${result_file}" ]] && grep -q "VALIDATION_SUCCESS" "${result_file}" 2>/dev/null; then
        ((successful_tests++))
    fi
done

print_message "Summary: ${successful_tests}/${#SIGNAL_TESTS[@]} signal tests passed"

# Clean up any remaining core files
rm -rf "${PROJECT_DIR}"/*.core.* 2>/dev/null || true

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
