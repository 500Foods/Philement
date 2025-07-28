#!/bin/bash

# Test: Socket Rebinding
# Tests that the SO_REUSEADDR socket option allows immediate rebinding after shutdown
# with active HTTP connections that create TIME_WAIT sockets

# CHANGELOG
# 4.0.3 - 2025-07-15 - No more sleep
# 4.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 4.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 4.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established patterns from other migrated tests
# 3.0.0 - 2025-06-30 - Enhanced to make actual HTTP requests, creating proper TIME_WAIT conditions for realistic testing
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic socket rebinding test functionality

# Test Configuration
TEST_NAME="Socket Rebinding"
SCRIPT_VERSION="4.0.3"

# Get the directory where this script is located0
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "${SCRIPT_DIR}/.." && pwd )"

if [[ -z "${LOG_OUTPUT_SH_GUARD}" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "${SCRIPT_DIR}/lib/log_output.sh"
fi

# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/framework.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/file_utils.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/lifecycle.sh"
# shellcheck source=tests/lib/network_utils.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/network_utils.sh"

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Use build directory for test results
BUILD_DIR="${SCRIPT_DIR}/../build"
RESULTS_DIR="${BUILD_DIR}/tests/results"
mkdir -p "${RESULTS_DIR}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/socket_rebind_test_${TIMESTAMP}.log"

# Initialize test variables
TOTAL_SUBTESTS=7  # Binary/config, environment, first instance, HTTP requests, shutdown, TIME_WAIT check, second instance
PASS_COUNT=0
EXIT_CODE=0
FIRST_PID=""
SECOND_PID=""

# Function to safely kill a process with PID validation
# shellcheck disable=SC2317  # Function called indirectly via trap
safe_kill_process() {
    local pid="$1"
    local process_name="$2"
    
    # Validate PID is not empty and not zero
    if [ -z "${pid}" ] || [ "${pid}" = "0" ]; then
        return 1
    fi
    
    if ps -p "${pid}" > /dev/null 2>&1; then
        print_message "Cleaning up ${process_name} (PID ${pid})..." | tee -a "${RESULT_LOG}"
        kill -SIGINT "${pid}" 2>/dev/null || kill -9 "${pid}" 2>/dev/null
        return 0
    fi
    return 1
}

# Handle cleanup on script termination
# shellcheck disable=SC2317  # Function called indirectly via trap
cleanup() {
    # Kill any hydrogen processes started by this script
    safe_kill_process "${FIRST_PID}" "first instance" || true
    safe_kill_process "${SECOND_PID}" "second instance" || true
    
    exit 0
}

# Set up trap for clean termination
trap cleanup SIGINT SIGTERM EXIT

# Subtest 1: Find Hydrogen binary and configuration
next_subtest
print_subtest "Find Hydrogen binary and configuration" | tee -a "${RESULT_LOG}"

# Find Hydrogen binary
if ! find_hydrogen_binary "${HYDROGEN_DIR}" "HYDROGEN_BIN" || [ -z "${HYDROGEN_BIN}" ]; then
    print_result 1 "Failed to find Hydrogen binary"
    print_test_completion "${TEST_NAME}"
    exit 1
fi

# Find configuration file
CONFIG_FILE=$(get_config_path "hydrogen_test_api.json")
if [ ! -f "${CONFIG_FILE}" ]; then
    CONFIG_FILE=$(get_config_path "hydrogen_test_max.json")
    print_message "API test config not found, using max config" | tee -a "${RESULT_LOG}"
fi

if [ ! -f "${CONFIG_FILE}" ]; then
    print_result 1 "No suitable configuration file found"
    print_test_completion "${TEST_NAME}"
    exit 1
fi

print_message "Using config: $(convert_to_relative_path "${CONFIG_FILE}")" | tee -a "${RESULT_LOG}"

# Get the web server port
PORT=$(get_webserver_port "${CONFIG_FILE}")
print_message "Web server port: ${PORT}" | tee -a "${RESULT_LOG}"

print_result 0 "Binary and configuration found successfully"
((PASS_COUNT++))

# Subtest 2: Prepare test environment
next_subtest
print_subtest "Prepare test environment" | tee -a "${RESULT_LOG}"

# Check current environment status (no waiting - SO_REUSEADDR should handle TIME_WAIT)
print_message "Checking current environment status for port ${PORT}" | tee -a "${RESULT_LOG}"

# Check if port is currently bound by an active process
if check_port_in_use "${PORT}"; then
    print_warning "Port ${PORT} is currently in use by an active process" | tee -a "${RESULT_LOG}"
    kill_processes_on_port "${PORT}"
    
    # Verify the port is now free
    if check_port_in_use "${PORT}"; then
        print_result 1 "Port ${PORT} is still in use after cleanup"
        EXIT_CODE=1
    else
        print_message "Port ${PORT} is now available" | tee -a "${RESULT_LOG}"
    fi
fi

# Check for TIME_WAIT sockets (informational only - no waiting)
time_wait_count=$(count_time_wait_sockets "${PORT}")
if [ "${time_wait_count}" -gt 0 ]; then
    print_message "Found ${time_wait_count} TIME_WAIT socket(s) on port ${PORT} - SO_REUSEADDR will handle this" | tee -a "${RESULT_LOG}"
else
    print_message "No TIME_WAIT sockets found on port ${PORT}" | tee -a "${RESULT_LOG}"
fi

print_result 0 "Test environment prepared successfully"
((PASS_COUNT++))

# Subtest 3: Start first Hydrogen instance
next_subtest
print_subtest "Start first Hydrogen instance" | tee -a "${RESULT_LOG}"

# Start first instance using lifecycle.sh
FIRST_LOG="${RESULTS_DIR}/first_instance_${TIMESTAMP}.log"
if start_hydrogen_with_pid "${CONFIG_FILE}" "${FIRST_LOG}" 15 "${HYDROGEN_BIN}" "FIRST_PID" && [ -n "${FIRST_PID}" ]; then
    print_result 0 "First instance started successfully (PID: ${FIRST_PID})"
    ((PASS_COUNT++))
    
    # Verify port is bound
    if check_port_in_use "${PORT}"; then
        print_message "Port ${PORT} is bound by first instance" | tee -a "${RESULT_LOG}"
    else
        print_warning "Port ${PORT} does not appear to be bound" | tee -a "${RESULT_LOG}"
    fi
else
    print_result 1 "Failed to start first instance"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}"
    export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null
    exit 1
fi

# Make HTTP requests to create active connections
next_subtest
print_subtest "Make HTTP requests to create connections" | tee -a "${RESULT_LOG}"

BASE_URL="http://localhost:${PORT}"
if make_http_requests "${BASE_URL}" "${RESULTS_DIR}" "${TIMESTAMP}"; then
    print_result 0 "HTTP requests completed - connections established"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to make HTTP requests"
    EXIT_CODE=1
fi

# Subtest 4: Shutdown first instance
next_subtest
print_subtest "Shutdown first instance" | tee -a "${RESULT_LOG}"

# Stop first instance using lifecycle.sh
DIAGS_DIR="${RESULTS_DIR}/diagnostics_${TIMESTAMP}"
mkdir -p "${DIAGS_DIR}"

if stop_hydrogen "${FIRST_PID}" "${FIRST_LOG}" 15 5 "${DIAGS_DIR}"; then
    print_result 0 "First instance shutdown successfully"
    ((PASS_COUNT++))
else
    print_result 1 "First instance shutdown failed"
    EXIT_CODE=1
fi

# Check for TIME_WAIT sockets
next_subtest
print_subtest "Check TIME_WAIT sockets" | tee -a "${RESULT_LOG}"

check_time_wait_sockets "${PORT}" | tee -a "${RESULT_LOG}"
print_result 0 "TIME_WAIT socket check completed"
((PASS_COUNT++))

# Subtest 5: Immediately start second instance (the main test)
next_subtest
print_subtest "Start second instance immediately (SO_REUSEADDR test)" | tee -a "${RESULT_LOG}"

# Start second instance immediately using lifecycle.sh
SECOND_LOG="${RESULTS_DIR}/second_instance_${TIMESTAMP}.log"
if start_hydrogen_with_pid "${CONFIG_FILE}" "${SECOND_LOG}" 15 "${HYDROGEN_BIN}" "SECOND_PID" && [ -n "${SECOND_PID}" ]; then
    print_result 0 "Second instance started successfully (PID: ${SECOND_PID}) - SO_REUSEADDR working!"
    print_message "Immediate rebinding after shutdown works correctly" | tee -a "${RESULT_LOG}"
    ((PASS_COUNT++))
    
    # Verify port is bound by second instance
    if check_port_in_use "${PORT}"; then
        print_message "Port ${PORT} is bound by second instance" | tee -a "${RESULT_LOG}"
    else
        print_warning "Port ${PORT} does not appear to be bound by second instance" | tee -a "${RESULT_LOG}"
    fi
else
    print_result 1 "Failed to start second instance - SO_REUSEADDR may not be working"
    EXIT_CODE=1
fi

# Clean up second instance
if [ -n "${SECOND_PID}" ] && [ "${SECOND_PID}" != "0" ]; then
    if ps -p "${SECOND_PID}" > /dev/null 2>&1; then
        print_message "Terminating second instance (PID ${SECOND_PID})..." | tee -a "${RESULT_LOG}"
        kill -SIGINT "${SECOND_PID}" 2>/dev/null || true
        # Brief wait for graceful shutdown
        wait_count=0
        while [ ${wait_count} -lt 10 ] && ps -p "${SECOND_PID}" > /dev/null 2>&1; do
            # sleep 0.2
            ((wait_count++))
        done
        if ps -p "${SECOND_PID}" > /dev/null 2>&1; then
            print_warning "Second instance still running, forcing termination..." | tee -a "${RESULT_LOG}"
            kill -9 "${SECOND_PID}" 2>/dev/null || true
        fi
        print_message "Second instance cleanup completed" | tee -a "${RESULT_LOG}"
    fi
fi

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
