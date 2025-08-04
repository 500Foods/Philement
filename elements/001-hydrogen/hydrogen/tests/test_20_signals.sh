#!/bin/bash

# Test: Signal Handling
# Tests signal handling capabilities including SIGINT, SIGTERM, SIGHUP, SIGUSR2, and multiple signals

# FUNCTIONS
# wait_for_signal_startup()
# wait_for_signal_shutdown() 
# wait_for_restart_completion() 
# verify_config_dump() 

# CHANGELOG
# 5.0.0 0 2025-07-30 - Overhaul #1
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
TEST_VERSION="5.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
TEST_CONFIG="${CONFIG_DIR}/hydrogen_test_max.json"
TEST_CONFIG_BASE=$(basename "${TEST_CONFIG}")
LOG_FILE_SIGINT="${LOGS_DIR}/test_${TEST_NUMBER}_SIGINT_${TIMESTAMP}.log"
LOG_FILE_SIGTERM="${LOGS_DIR}/test_${TEST_NUMBER}_SIGTERM_${TIMESTAMP}.log"
LOG_FILE_SIGHUP="${LOGS_DIR}/test_${TEST_NUMBER}_SIGHUP_${TIMESTAMP}.log"
LOG_FILE_SIGUSR1="${LOGS_DIR}/test_${TEST_NUMBER}_SIGUSR1_${TIMESTAMP}.log"
LOG_FILE_SIGUSR2="${LOGS_DIR}/test_${TEST_NUMBER}_SIGUSR2_${TIMESTAMP}.log"
LOG_FILE_MULTI="${LOGS_DIR}/test_${TEST_NUMBER}_NULTI_${TIMESTAMP}.log"
LOG_FILE_RESTART="${LOGS_DIR}/test_${TEST_NUMBER}_RESTART_${TIMESTAMP}.log"

# Signal test configuration
RESTART_COUNT=5  # Number of SIGHUP restarts to test
STARTUP_TIMEOUT=5
SHUTDOWN_TIMEOUT=10
SIGNAL_TIMEOUT=15

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

# Function to wait for startup with signal-specific timeout
wait_for_signal_startup() {
    local log_file="$1"
    local timeout="$2"
    local start_time
    start_time=$(date +%s)
    
    while true; do
        if [[ $(($(date +%s || true) - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            return 0
        fi
    done
}

# Function to wait for shutdown with signal verification
wait_for_signal_shutdown() {
    local pid="$1"
    local timeout="$2"
    local log_file="$3"
    local start_time
    start_time=$(date +%s)
    
    while true; do
        if [[ $(($(date +%s || true) - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        if ! ps -p "${pid}" > /dev/null 2>&1; then
            if grep -q "Shutdown complete" "${log_file}" 2>/dev/null; then
                return 0
            else
                return 2  # Process died but no clean shutdown message
            fi
        fi
    done
}

# Function to wait for restart completion
wait_for_restart_completion() {
    local pid="$1"
    local timeout="$2"
    local expected_count="$3"
    local log_file="$4"
    local start_time
    start_time=$(date +%s || true)
    
    print_message "Waiting for restart completion (expecting restart count: ${expected_count})..."
    
    while true; do
        if [[ $(($(date +%s || true) - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        # Check for restart completion markers
        if grep -q "Restart completed successfully" "${log_file}" 2>/dev/null; then
            if grep -q "Application restarts: ${expected_count}" "${log_file}" 2>/dev/null || \
               grep -q "Restart count: ${expected_count}" "${log_file}" 2>/dev/null; then
                print_message "Verified restart completed successfully with count ${expected_count}"
                return 0
            fi
        fi
        
        # Alternative: Check for restart sequence completion
        if (grep -q "Application restarts: ${expected_count}" "${log_file}" 2>/dev/null || \
            grep -q "Restart count: ${expected_count}" "${log_file}" 2>/dev/null) && \
           grep -q "Initiating graceful restart sequence" "${log_file}" 2>/dev/null && \
           grep -q "Application started" "${log_file}" 2>/dev/null; then
            print_message "Verified restart via restart sequence with count ${expected_count}"
            return 0
        fi
    done
}

# Function to verify config dump
verify_config_dump() {
    local log_file="$1"
    local timeout="$2"
    local start_time
    start_time=$(date +%s || true)
    
    # Wait for dump to start
    while true; do
        if [[ $(($(date +%s || true) - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Started" "${log_file}" 2>/dev/null; then
            break
        fi
    done
    
    # Wait for dump to complete
    while true; do
        if [[ $(($(date +%s || true) - start_time)) -ge "${timeout}" ]]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Complete" "${log_file}" 2>/dev/null || true; then
            local start_line end_line dump_lines
            start_line=$(grep -n "APPCONFIG Dump Started" "${log_file}" | tail -1 | cut -d: -f1 || true)
            end_line=$(grep -n "APPCONFIG Dump Complete" "${log_file}" | tail -1 | cut -d: -f1 || true)
            if [[ -n "${start_line}" ]] && [[ -n "${end_line}" ]]; then
                dump_lines=$((end_line - start_line + 1))
                print_message "Config dump completed with ${dump_lines} lines"
                return 0
            fi
        fi
    done
}

print_subtest "SIGINT Signal Handling (Ctrl+C)"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_SIGINT}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_SIGINT}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_command "kill -SIGINT ${HYDROGEN_PID}"

    kill -SIGINT "${HYDROGEN_PID}"
    
    if wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_SIGINT}"; then
        print_result 0 "SIGINT handled successfully with clean shutdown"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGINT handling failed - no clean shutdown"
        kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for SIGINT test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "SIGTERM Signal Handling (Terminate)"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_SIGTERM}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_SIGTERM}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_command "kill -SIGTERM ${HYDROGEN_PID}"

    kill -SIGTERM "${HYDROGEN_PID}"
    
    if wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_SIGTERM}"; then
        print_result 0 "SIGTERM handled successfully with clean shutdown"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGTERM handling failed - no clean shutdown"
        kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for SIGTERM test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "SIGHUP Signal Handling (Single Restart)"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_SIGHUP}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_SIGHUP}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_command "kill -SIGHUP ${HYDROGEN_PID}"

    kill -SIGHUP "${HYDROGEN_PID}"
    
    # Wait for restart signal to be logged
    if grep -q "SIGHUP received" "${LOG_FILE_SIGHUP}" 2>/dev/null; then
        print_message "SIGHUP signal received, waiting for restart completion..."
        if wait_for_restart_completion "${HYDROGEN_PID}" "${SIGNAL_TIMEOUT}" 1 "${LOG_FILE_SIGHUP}"; then
            print_result 0 "SIGHUP handled successfully with restart"
            ((PASS_COUNT++))
        else
            print_result 1 "SIGHUP restart failed or timed out"
            EXIT_CODE=1
        fi
    else
        print_result 1 "SIGHUP signal not received or logged"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM "${HYDROGEN_PID}" 2>/dev/null || true
    wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_SIGHUP}" || true
else
    print_result 1 "Failed to start Hydrogen for SIGHUP test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "SIGHUP Signal Handling (Many Restarts)"
print_message "Testing multiple SIGHUP restarts (count: ${RESTART_COUNT})"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_RESTART}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_RESTART}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    
    restart_success=0
    current_count=1
    
    while [[ "${current_count}" -le "${RESTART_COUNT}" ]]; do

        print_message "Sending SIGHUP #${current_count} of ${RESTART_COUNT}..."
        print_command "kill -SIGHUP ${HYDROGEN_PID}"

        kill -SIGHUP "${HYDROGEN_PID}" || true
        
        # Wait for restart signal to be logged
        if grep -q "SIGHUP received" "${LOG_FILE_RESTART}" 2>/dev/null; then
            print_message "SIGHUP signal #${current_count} received, waiting for restart..."
            if wait_for_restart_completion "${HYDROGEN_PID}" "${SIGNAL_TIMEOUT}" "${current_count}" "${LOG_FILE_RESTART}"; then
                print_message "Restart #${current_count} of ${RESTART_COUNT} completed successfully"
                if [[ "${current_count}" -eq "${RESTART_COUNT}" ]]; then
                    restart_success=1
                    break
                fi
                current_count=$((current_count + 1))
            else
                print_message "Restart #${current_count} of ${RESTART_COUNT} failed"
                break
            fi
        else
            print_message "SIGHUP signal #${current_count} not received"
            break
        fi
    done
    
    if [[ "${restart_success}" -eq 1 ]]; then
        print_result 0 "Multiple SIGHUP restarts successful (${RESTART_COUNT} restarts)"
        ((PASS_COUNT++))
    else
        print_result 1 "Multiple SIGHUP restarts failed"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM "${HYDROGEN_PID}" 2>/dev/null || true
    wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_RESTART}" || true
else
    print_result 1 "Failed to start Hydrogen for multiple SIGHUP test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "SIGUSR1 Signal Handling (Crash Dump)"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_SIGUSR1}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_SIGUSR1}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_command "kill -SIGUSR1 ${HYDROGEN_PID}"

    kill -SIGUSR1 "${HYDROGEN_PID}"

    print_message "Looking for ${HYDROGEN_BIN}.core.${HYDROGEN_PID}"

    if [[ -f "${HYDROGEN_BIN}.core.${HYDROGEN_PID}" ]]; then
      print_message "Found ${HYDROGEN_BIN}.core.${HYDROGEN_PID}"
      ((PASS_COUNT++))
      print_result 0 "SIGUSR1 generated crash dump"
      rm -f  "${HYDROGEN_BIN}.core.${HYDROGEN_PID}"
    else
      print_message "Missing ${HYDROGEN_BIN}.core.${HYDROGEN_PID}"
      print_result 1 "SIGUSR1 failed to generate crash dump"
    fi

    # Clean up
    wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_SIGUSR1}" || true
else
    print_result 1 "Failed to start Hydrogen for SIGUSR1 test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "SIGUSR2 Signal Handling (Config Dump)"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_SIGUSR2}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_SIGUSR2}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_command "kill -SIGUSR2 ${HYDROGEN_PID}"

    kill -SIGUSR2 "${HYDROGEN_PID}"
    
    if verify_config_dump "${LOG_FILE_SIGUSR2}" "${SIGNAL_TIMEOUT}"; then
        print_result 0 "SIGUSR2 handled successfully with config dump"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGUSR2 handling failed - no config dump"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM "${HYDROGEN_PID}" 2>/dev/null || true
    wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_SIGUSR2}" || true
else
    print_result 1 "Failed to start Hydrogen for SIGUSR2 test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

print_subtest "Multiple Signal Handling"
print_message "Testing multiple signals sent simultaneously"
print_command "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

"${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${LOG_FILE_MULTI}" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "${LOG_FILE_MULTI}" "${STARTUP_TIMEOUT}"; then

    print_message "Hydrogen started successfully (PID: ${HYDROGEN_PID})"
    print_message "Sending multiple signals (SIGTERM, SIGINT)..."
    print_command "kill -SIGTERM ${HYDROGEN_PID} && kill -SIGINT ${HYDROGEN_PID}"
    
    kill -SIGTERM "${HYDROGEN_PID}"
    kill -SIGINT "${HYDROGEN_PID}"
    
    if wait_for_signal_shutdown "${HYDROGEN_PID}" "${SHUTDOWN_TIMEOUT}" "${LOG_FILE_MULTI}"; then
        # Check logs for single shutdown sequence
        shutdown_count=$(grep -c "Initiating graceful shutdown sequence" "${LOG_FILE_MULTI}" 2>/dev/null || echo "0")
        if [[ "${shutdown_count}" -eq 1 ]]; then
            print_result 0 "Multiple signals handled correctly (single shutdown sequence)"
            ((PASS_COUNT++))
        else
            print_result 1 "Multiple shutdown sequences detected (${shutdown_count} sequences)"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Multiple signal handling failed - no clean shutdown"
        kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for multiple signal test"
    kill -9 "${HYDROGEN_PID}" 2>/dev/null || true
    EXIT_CODE=1
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
