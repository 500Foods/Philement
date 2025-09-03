#!/usr/bin/env bash

# Test: Library Dependencies
# Tests the library dependency checking system added to initialization

# FUNCTIONS
# check_dependency_log()

# CHANGELOG
# 4.1.1 - 2025-09-03 - Added checks for database dependencies
# 4.0.0 - 2025-08-26 - Modified to start Hydrogen WITHOUT a config file to test default configuration
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.1.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.1.0 - 2025-07-06 - Updated to accept "Less" as a passing status in addition to "Good" and "Less Good" for dependency checks
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
# 1.0.0 - Initial version with library dependency checking

set -euo pipefail

# Test configuration
TEST_NAME="Library Dependencies"
TEST_ABBR="DEP"
TEST_NUMBER="14"
TEST_COUNTER=0
TEST_VERSION="4.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
# Note: This test starts Hydrogen WITHOUT a config file to test default configuration
STARTUP_TIMEOUT=10

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


print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Setup No-Config Test"

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting Hydrogen without config file to test default configuration"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No-config test setup complete"

# Timeouts and limits
SHUTDOWN_TIMEOUT=10   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=3  # Timeout if no new log activity

# Function to check dependency log messages and extract version information
check_dependency_log() {
    local dep_name="$1"
    local log_file="$2"
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Dependency: ${dep_name}"
    
    # Extract the full dependency line
    local dep_line
    dep_line=$("${GREP}" "DepCheck.*${dep_name}.*Status:" "${log_file}" 2>/dev/null)
    
    if [[ -n "${dep_line}" ]]; then
        # Extract version information using sed
        local expected_version found_version status
        expected_version=$(echo "${dep_line}" | "${SED}" -n 's/.*Expecting: \([^ ]*\).*/\1/p' || true)
        found_version=$(echo "${dep_line}" | "${SED}" -n 's/.*Found: \([^(]*\).*/\1/p' | "${SED}" 's/ *$//' || true)
        status=$(echo "${dep_line}" | "${SED}" -n 's/.*Status: \([^ ]*\).*/\1/p' || true)
        
        if [[ "${status}" = "Good" ]] || [[ "${status}" = "Less Good" ]] || [[ "${status}" = "Less" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Found ${dep_name} - Expected: ${expected_version}, Found: ${found_version}, Status: ${status}"
            return 0
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${dep_name} but status is not Good, Less Good, or Less - Expected: ${expected_version}, Found: ${found_version}, Status: ${status}"
            EXIT_CODE=1
            return 1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Did not find ${dep_name} dependency check in logs (skipped)"
        return 0
    fi
}

# Function to start Hydrogen without a config file
start_hydrogen_no_config() {
    local log_file="$1"
    local timeout="$2"
    local hydrogen_bin="$3"
    local pid_var="$4"
    local hydrogen_pid

    # Clear the PID variable
    eval "${pid_var}=''"

    # Clean log file
    true > "${log_file}"

    # Validate binary exists
    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen binary not found at: ${hydrogen_bin}"
        return 1
    fi

    # Show the exact command that will be executed
    local cmd_display
    cmd_display="$(basename "${hydrogen_bin}") (no config file)"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "${cmd_display}"

    # Record launch time
    local launch_time_ms
    launch_time_ms=$("${DATE}" +%s%3N)

    # Launch Hydrogen without config file (disown to prevent job control messages)
    "${hydrogen_bin}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true

    # Display the PID for tracking
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen process started with PID: ${hydrogen_pid}"

    # Wait for startup
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for startup (max ${timeout}s)..."
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if wait_for_startup "${log_file}" "${timeout}" "${launch_time_ms}"; then
        # Set the PID in the reference variable
        eval "${pid_var}='${hydrogen_pid}'"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen startup timed out"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        return 1
    fi
}

# Function to run a no-config lifecycle test
run_no_config_lifecycle_test() {
    local diag_test_dir="$1"
    local startup_timeout="$2"
    local shutdown_timeout="$3"
    local shutdown_activity_timeout="$4"
    local hydrogen_bin="$5"
    local log_file="$6"
    local pass_count_var="$7"
    local exit_code_var="$8"
    local subtest_count=0
    local config_name="no_config"
    # shellcheck disable=SC2154 # LOG_PREFIX defined in
    local diag_config_dir="${LOG_PREFIX}${config_name}"
    local hydrogen_pid

    mkdir -p "${diag_test_dir}" "${diag_config_dir}" 2>/dev/null

    # Subtest: Start Hydrogen
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen - ${config_name}"

    # Call start_hydrogen_no_config and get the PID via a temporary variable
    local temp_pid_var="TEMP_HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_no_config "${log_file}" "${startup_timeout}" "${hydrogen_bin}" "${temp_pid_var}"; then
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen started with ${config_name}"
        eval "${pass_count_var}=\$(( \${${pass_count_var}} + 1 ))" || true
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with ${config_name}"
        eval "${exit_code_var}=1"
    fi
    subtest_count=$(( subtest_count + 1 ))

    # Capture pre-shutdown diagnostics if started successfully
    if [[ -n "${hydrogen_pid}" ]]; then
        capture_process_diagnostics "${hydrogen_pid}" "${diag_config_dir}" "pre_shutdown"

        # Subtest: Stop Hydrogen
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen - ${config_name}"
        local stop_result=0
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${hydrogen_pid}" "${log_file}" "${shutdown_timeout}" "${shutdown_activity_timeout}" "${diag_config_dir}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen stopped with ${config_name}"
            eval "${pass_count_var}=\$(( \${${pass_count_var}} + 1 ))" || true
            stop_result=0
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to stop Hydrogen with ${config_name}"
            eval "${exit_code_var}=1"
            stop_result=1
        fi
        subtest_count=$(( subtest_count + 1 ))

        # Subtest: Verify Clean Shutdown
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Clean Shutdown - ${config_name}"
        # For Hydrogen, a clean shutdown is verified by the process terminating successfully
        # and the stop_hydrogen function returning success (which it already did above)
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Clean shutdown return code: ${stop_result}"
        if [[ ${stop_result} -eq 0 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Clean shutdown verified for ${config_name}"
            eval "${pass_count_var}=\$(( \${${pass_count_var}} + 1 ))" || true
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Shutdown completed with issues for ${config_name}"
            eval "${exit_code_var}=1"
        fi
        subtest_count=$(( subtest_count + 1 ))
    fi

    return "$(eval "echo \${${exit_code_var}}" || true)"
}

# Start Hydrogen for dependency checking (no config file)
run_no_config_lifecycle_test "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Analyze logs for dependency checks
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking dependency logs for library dependency detection"
check_dependency_log "pthreads" "${LOG_FILE}"
check_dependency_log "jansson" "${LOG_FILE}"
check_dependency_log "libm" "${LOG_FILE}"
check_dependency_log "microhttpd" "${LOG_FILE}"
check_dependency_log "libwebsockets" "${LOG_FILE}"
check_dependency_log "OpenSSL" "${LOG_FILE}"
check_dependency_log "libbrotlidec" "${LOG_FILE}"
check_dependency_log "libtar" "${LOG_FILE}"

# Check database dependencies (using system commands)
check_dependency_log "DB2" "${LOG_FILE}"
check_dependency_log "PostgreSQL" "${LOG_FILE}"
check_dependency_log "MySQL" "${LOG_FILE}"
check_dependency_log "SQLite" "${LOG_FILE}"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
