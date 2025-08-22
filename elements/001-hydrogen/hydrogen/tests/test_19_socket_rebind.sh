#!/usr/bin/env bash

# Test: Socket Rebinding
# Tests that the SO_REUSEADDR socket option allows immediate rebinding after shutdown
# with active HTTP connections that create TIME_WAIT sockets

# CHANGELOG
# 5.1.0 - 2025-08-08 - Review and adjustment to logs and JSON naming
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.3 - 2025-07-15 - No more sleep
# 4.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 4.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 4.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established patterns from other migrated tests
# 3.0.0 - 2025-06-30 - Enhanced to make actual HTTP requests, creating proper TIME_WAIT conditions for realistic testing
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic socket rebinding test functionality

set -euo pipefail

# Test Configuration
TEST_NAME="Socket Rebinding"
TEST_ABBR="SCK"
TEST_NUMBER="19"
TEST_COUNTER=0
TEST_VERSION="5.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_rebind.json"
FIRST_PID=""
SECOND_PID=""
FIRST_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_first.log"
SECOND_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_second.log"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_FILE}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated Configuration File"
else
    EXIT_CODE=1
fi

# Get the web server port
PORT=$(get_webserver_port "${CONFIG_FILE}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Web server port: ${PORT}" 

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Prepare test environment"

# Check current environment status (no waiting - SO_REUSEADDR should handle TIME_WAIT)
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking current environment status for port ${PORT}" 

# Check if port is currently bound by an active process
# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_port_in_use "${PORT}"; then
    print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} is currently in use by an active process" 
    # ll_processes_on_port "${PORT}"
    
    # Verify the port is now free
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if check_port_in_use "${PORT}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Port ${PORT} is still in use after cleanup"
        EXIT_CODE=1
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} is now available" 
    fi
fi

# Check for TIME_WAIT sockets (informational only - no waiting)
time_wait_count=$(count_time_wait_sockets "${PORT}")
if [[ "${time_wait_count}" -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${time_wait_count} TIME_WAIT socket(s) on port ${PORT} - SO_REUSEADDR will handle this" 
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No TIME_WAIT sockets found on port ${PORT}" 
fi

print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Test environment prepared successfully"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start first Hydrogen instance" 

# Start first instance using lifecycle.sh
# shellcheck disable=SC2310 # We want to continue even if the test fails
if start_hydrogen_with_pid "${CONFIG_FILE}" "${FIRST_LOG}" 15 "${HYDROGEN_BIN}" "FIRST_PID" && [[ -n "${FIRST_PID}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "First instance started successfully (PID: ${FIRST_PID})"
    
    # Verify port is bound
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if check_port_in_use "${PORT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} is bound by first instance" 
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} does not appear to be bound"
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start first instance"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}" 
    exit 1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Make HTTP requests to create connections"

BASE_URL="http://localhost:${PORT}"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if make_http_requests "${BASE_URL}" "${DIAG_TEST_DIR}" "${TIMESTAMP}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HTTP requests completed - connections established"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to make HTTP requests"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown first instance" 

# Stop first instance using lifecycle.sh
# shellcheck disable=SC2310 # We want to continue even if the test fails
if stop_hydrogen "${FIRST_PID}" "${FIRST_LOG}" 15 5 "${DIAGS_DIR}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "First instance shutdown successfully"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "First instance shutdown failed"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check TIME_WAIT sockets" 

check_time_wait_sockets "${PORT}" 
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "TIME_WAIT socket check completed"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start second instance immediately (SO_REUSEADDR test)" 

# Start second instance immediately using lifecycle.sh
# shellcheck disable=SC2310 # We want to continue even if the test fails
if start_hydrogen_with_pid "${CONFIG_FILE}" "${SECOND_LOG}" 15 "${HYDROGEN_BIN}" "SECOND_PID" && [[ -n "${SECOND_PID}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Second instance started successfully (PID: ${SECOND_PID}) - SO_REUSEADDR applied successfully"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Immediate rebinding after shutdown works correctly"
    
    # Verify port is bound by second instance
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if check_port_in_use "${PORT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} is bound by second instance" 
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Port ${PORT} does not appear to be bound by second instance" 
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start second instance - SO_REUSEADDR may not be working"
    EXIT_CODE=1
fi

# Clean up second instance
if [[ -n "${SECOND_PID}" ]] && [[ "${SECOND_PID}" != "0" ]]; then
    if ps -p "${SECOND_PID}" > /dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminating second instance (PID ${SECOND_PID})..." 
        kill -SIGINT "${SECOND_PID}" 2>/dev/null || true
        # Brief wait for graceful shutdown
        wait_count=0
        while [[ "${wait_count}" -lt 10 ]] && ps -p "${SECOND_PID}" > /dev/null 2>&1; do
            ((wait_count++))
        done
        if ps -p "${SECOND_PID}" > /dev/null 2>&1; then
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Second instance still running, forcing termination..." 
            kill -9 "${SECOND_PID}" 2>/dev/null || true
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Second instance cleanup completed" 
    fi
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
