#!/usr/bin/env bash

# Lifecycle Management Library
# Handles starting and stopping the Hydrogen Server with various configurations.

# LIBRARY FUNCTIONS
# find_hydrogen_binary()
# start_hydrogen_with_pid()
# wait_for_startup() 
# stop_hydrogen() 
# monitor_shutdown()
# validate_config_file()
# run_lifecycle_test()
# wait_for_server_ready
# get_process_threads() {
# capture_process_diagnostics() {

# CHANGELOG
# 2.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 1.7.0 - 2025-09-07 - Find hydrogen updated to include 'hydrogen' so it doesn't find the installer :-(
# 1.6.1 - 2025-08-08 - Removed validate_config_filees() - useed only oncee
# 1.6.0 - 2025-08-08 - Removed an extra copies of the logs being generated
# 1.5.0 - 2025-08-07 - Added better checks for showing shutdown time and total elapsed time
# 1.4.0 - 2025-08-03 - Was a bit too aggressive with function culling
# 1.3.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.3.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.2.4 - 2025-07-18 - Fixed subshell issue in error log output functions that prevented detailed error messages from being displayed in test output
# 1.2.3 - 2025-07-14 - Enhanced process validation with multiple retry attempts for more robust PID checking
# 1.2.2 - 2025-07-14 - Fixed job control messages (like "Terminated") appearing in test output by adding disown to background processes
# 1.2.1 - 2025-07-02 - Updated find_hydrogen_binary to use relative paths in log messages to avoid exposing user information
# 1.2.0 - 2025-07-02 - Added validate_config_file function for single configuration validation
# 1.1.0 - 2025-07-02 - Added validate_config_files, setup_output_directories, and run_lifecycle_test functions for enhanced modularity
# 1.0.0 - 2025-07-02 - Initial version with start and stop functions

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${LIFECYCLE_GUARD:-}" ]] && return 0
export LIFECYCLE_GUARD="true"

# Library metadata
LIFECYCLE_NAME="Lifecycle Management Library"
LIFECYCLE_VERSION="2.0.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${LIFECYCLE_NAME} ${LIFECYCLE_VERSION}" "info"

# Function to find and validate Hydrogen binary
# Usage: find_hydrogen_binary <hydrogen_dir> <result_var_name>
# Sets the named variable with the binary path
find_hydrogen_binary() {
    local hydrogen_dir="$1"
    local hydrogen_bin
    
    # Ensure hydrogen_dir is an absolute path
    # shellcheck disable=SC2154 # REALPATH defined externally in framework.sh
    hydrogen_dir=$("${REALPATH}" "${hydrogen_dir}" 2>/dev/null || echo "${hydrogen_dir}")
   
    # First check for coverage build (highest priority for testing)
    hydrogen_bin="${hydrogen_dir}/hydrogen_coverage"
    if [[ -f "${hydrogen_bin}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using coverage build for testing: hydrogen_coverage"
    # Then check for release build
    else
        hydrogen_bin="${hydrogen_dir}/hydrogen_release"
        if [[ -f "${hydrogen_bin}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using release build for testing: hydrogen_release"
        # Then check for debug build
        else
            hydrogen_bin="${hydrogen_dir}/hydrogen_debug"
            if [[ -f "${hydrogen_bin}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using debug build for testing: hydrogen"
            # If none found, search for binary in possible build directories
            else
                hydrogen_bin="${hydrogen_dir}/hydrogen"
                if [[ -f "${hydrogen_bin}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using debug build for testing: hydrogen"
                # If none found, search for binary in possible build directories
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Searching for Hydrogen binary in subdirectories..."
                    # shellcheck disable=SC2154 # FIND defined externally in framework.sh
                    hydrogen_bin=$("${FIND}" "${hydrogen_dir}" -type f -executable -name "hydrogen*" ! -name "*hydrogen_installer*" -print -quit 2>/dev/null)
                    if [[ -n "${hydrogen_bin}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found Hydrogen binary at: ${hydrogen_bin}"
                    else
                        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "No Hydrogen binary found in ${hydrogen_dir} or subdirectories"
                        return 1
                    fi
                fi
            fi
        fi
    fi
    
    # Validate binary exists and is executable
    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen binary not found at: ${hydrogen_bin}"
        return 1
    fi
    
    if [[ ! -x "${hydrogen_bin}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen binary is not executable: ${hydrogen_bin}"
        return 1
    fi
    
    HYDROGEN_BIN="${hydrogen_bin}"
    HYDROGEN_BIN_BASE=$(basename "${HYDROGEN_BIN}")
    export HYDROGEN_BIN HYDROGEN_BIN_BASE
    return 0
}

# Function to start Hydrogen application and capture PID via reference variable
# Usage: start_hydrogen_with_pid <config_file> <log_file> <timeout> <hydrogen_bin> <pid_var_name>
start_hydrogen_with_pid() {
    local config_file="$1"
    local log_file="$2"
    local timeout="$3"
    local hydrogen_bin="$4"
    local pid_var="$5"
    local hydrogen_pid
    
    # Clear the PID variable
    eval "${pid_var}=''"
    
    # Clean log file
    true > "${log_file}"
    
    # Validate binary and config exist
    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen binary not found at: ${hydrogen_bin}"
        return 1
    fi
    if [[ ! -f "${config_file}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration file not found at: ${config_file}"
        return 1
    fi
    
    # Show the exact command that will be executed
    local cmd_display
    cmd_display="$(basename "${hydrogen_bin}") $(basename "${config_file}")"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "${cmd_display}"
    
    # Record launch time
    local launch_time_ms
    # shellcheck disable=SC2154 # DATE defined externally in framework.sh
    launch_time_ms=$("${DATE}" +%s%3N)
    
    # Launch Hydrogen (disown to prevent job control messages)
    "${hydrogen_bin}" "${config_file}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true
    
    # Display the PID for tracking
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen process started with PID: ${hydrogen_pid}"
    
    # Verify process started with multiple attempts for robustness
#     local check_attempt=1
#     local max_attempts=100
#     local process_running=false
    
#     while [[ ${check_attempt} -le ${max_attempts} ]]; do
#         if [[ ${check_attempt} -eq 1 ]]; then
#             sleep 0.05  # Initial check after brief delay
#         else
#             sleep 0.05  # Short delay between subsequent checks
#         fi
        
# #        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
#             process_running=true
#             break
# #        fi
        
#         ((check_attempt++))
#     done
    
    # if [[ "${process_running}" = false ]]; then
    #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen - process did not start or crashed immediately (checked ${max_attempts} times)"
    #     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Check log file for possible errors: ${log_file}"
    #     if [[ -s "${log_file}" ]]; then
    #         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Last few lines of log file:"
    #         # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
    #         while IFS= read -r line; do
    #             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
    #         done < <(tail -n 5 "${log_file}" || true)
    #     fi
    #     return 1
    # fi
    
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

# Function to wait for startup completion
wait_for_startup() {
    local log_file="$1"
    local timeout="$2"
    local launch_time_ms="$3"
    local current_time_ms
    local elapsed_ms
    local elapsed_s
    
    while true; do
        current_time_ms=$("${DATE}" +%s%3N)
        elapsed_ms=$((current_time_ms - launch_time_ms))
        elapsed_s=$((elapsed_ms / 1000))
        
        if [[ "${elapsed_s}" -ge "${timeout}" ]]; then
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Startup timeout after ${elapsed_s}s"
            return 1
        fi
        
        # shellcheck disable=SC2154  # GREP defined externally in framework.sh
        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            # Extract startup time from log if available
            local log_startup_time
            log_startup_time=$("${GREP}" "Startup elapsed time:" "${log_file}" 2>/dev/null | sed 's/.*Startup elapsed time:  \([0-9.]*s\).*/\1/' | tail -1 || true)
            if [[ -n "${log_startup_time}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Startup completed in ${elapsed_ms}ms"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Logged startup time reported as ${log_startup_time}"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Startup completed in ${elapsed_ms}ms"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Logged startup time not found"
            fi
            return 0
        fi
        
        sleep 0.05
    done
}

# Function to stop Hydrogen application
stop_hydrogen() {
    local pid="$1"
    local log_file="$2"
    local timeout="$3"
    local activity_timeout="$4"
    local diag_dir="$5"
    local shutdown_start_ms
    local shutdown_end_ms
    local shutdown_duration_ms
    
    # Send shutdown signal
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Initiating shutdown for PID: ${pid}"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "kill -SIGINT ${pid}"
    shutdown_start_ms=$("${DATE}" +%s%3N)
    kill -SIGINT "${pid}" 2>/dev/null || true
    
    # Monitor shutdown
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if monitor_shutdown "${pid}" "${log_file}" "${timeout}" "${activity_timeout}" "${diag_dir}"; then
        shutdown_end_ms=$("${DATE}" +%s%3N)
        shutdown_duration_ms=$((shutdown_end_ms - shutdown_start_ms))
        
        # Extract shutdown time from log if available
        local log_shutdown_time
        local log_elapsed_time
        # shellcheck disable=SC2154,SC2016 # AWK defined externally in framework.sh, using single quotes on purpose
        times=$("${AWK}" '/Shutdown elapsed time:/ {s=$NF} /Total elapsed time:/ {t=$NF} END {print s, t}' "${log_file}")
        read -r log_shutdown_time log_elapsed_time <<< "${times}"
        if [[ -n "${log_shutdown_time}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown completed in ${shutdown_duration_ms}ms "
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Logged shutdown time reported as ${log_shutdown_time}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Logged elapsed time reported as ${log_elapsed_time}"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown completed in ${shutdown_duration_ms}ms [Log: Not found]"
        fi
        
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Shutdown failed or timed out"
        return 1
    fi
}

# Function to monitor shutdown process
monitor_shutdown() {
    local pid="$1"
    local log_file="$2"
    local timeout="$3"
    local activity_timeout="$4"
    local diag_dir="$5"
    
    local start_time
    local current_time
    local elapsed
    local log_size_before
    local log_size_now
    local last_activity
    local inactive_time
    
    start_time=$("${DATE}" +%s)
    # shellcheck disable=SC2154 # STAT defined externally in framework.sh
    log_size_before=$("${STAT}" -c %s "${log_file}" 2>/dev/null || echo 0)
    last_activity=$("${DATE}" +%s)
    
    while ps -p "${pid}" > /dev/null 2>&1; do
        current_time=$("${DATE}" +%s)
        elapsed=$((current_time - start_time))
        
        # Check for timeout
        if [[ "${elapsed}" -ge "${timeout}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Shutdown timeout after ${elapsed}s"
            get_process_threads "${pid}" "${diag_dir}/stuck_threads.txt"
            lsof -p "${pid}" >> "${diag_dir}/stuck_open_files.txt" 2>/dev/null || true
            kill -9 "${pid}" 2>/dev/null || true
            return 1
        fi
        
        # Check for log activity
        log_size_now=$("${STAT}" -c %s "${log_file}" 2>/dev/null || echo 0)
        if [[ "${log_size_now}" -gt "${log_size_before}" ]]; then
            log_size_before=${log_size_now}
            last_activity=$("${DATE}" +%s)
        else
            inactive_time=$((current_time - last_activity))
            if [[ "${inactive_time}" -ge "${activity_timeout}" ]]; then
                get_process_threads "${pid}" "${diag_dir}/inactive_threads.txt"
            fi
        fi
        
        sleep 0.05
    done
    
    return 0
}

# Function to validate a single configuration file
validate_config_file() {
    local config_file="$1"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking configuration file: ${config_file}"
    if [[ -f "${config_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file found"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file not found"
        return 1
    fi
}

# Function to run a lifecycle test for a specific configuration
run_lifecycle_test() {
    local config_file="$1"
    local config_name="$2"
    local diag_test_dir="$3"
    local startup_timeout="$4"
    local shutdown_timeout="$5"
    local shutdown_activity_timeout="$6"
    local hydrogen_bin="$7"
    local log_file="$8"
    local pass_count_var="$9"
    local exit_code_var="${10}"
    local subtest_count=0
    # shellcheck disable=SC2154 # LOG_PREFIX defined in
    local diag_config_dir="${LOG_PREFIX}${config_name}"
    local hydrogen_pid
    
    mkdir -p "${diag_test_dir}" "${diag_config_dir}" 2>/dev/null
    
    # Subtest: Start Hydrogen
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen - ${config_name}"
    
    # Call start_hydrogen and get the PID via a temporary variable
    local temp_pid_var="TEMP_HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${config_file}" "${log_file}" "${startup_timeout}" "${hydrogen_bin}" "${temp_pid_var}"; then
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen started with ${config_name}"
        eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
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
            eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
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
            eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Shutdown completed with issues for ${config_name}"
            eval "${exit_code_var}=1"
        fi
        subtest_count=$(( subtest_count + 1 ))
    fi
    
    return "$(eval "echo \$${exit_code_var}" || true)"
}

# Function to wait for server to be ready
wait_for_server_ready() {
    local base_url="$1"
    local max_attempts=100   # 2.5 seconds total (0.1s * 25)
    local attempt=1

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for server to be ready at ${base_url}..."

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        # Try multiple endpoints to check server readiness
        if curl -s --max-time 2 "${base_url}" >/dev/null 2>&1 || \
           curl -s --max-time 2 "${base_url}/" >/dev/null 2>&1 || \
           curl -s --max-time 2 "${base_url}/swagger/" >/dev/null 2>&1 || \
           curl -s --max-time 2 "${base_url}/apidocs/" >/dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server is ready after ${attempt} attempt(s)"
            return 0
        fi
        sleep 0.05
        ((attempt++))
    done

    print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Server failed to respond within the expected time"
    return 1
}

# Function to get process threads using pgrep
get_process_threads() {
    local pid="$1"
    local output_file="$2"
    
    # Use pgrep to find all threads for the process
    pgrep -P "${pid}" > "${output_file}" 2>/dev/null || true
    # Also include the main process
    echo "${pid}" >> "${output_file}" 2>/dev/null || true
}

# Function to capture process diagnostics
capture_process_diagnostics() {
    local pid="$1"
    local diag_dir="$2"
    local prefix="$3"
    
    # Capture thread information using pgrep
    get_process_threads "${pid}" "${diag_dir}/${prefix}_threads.txt"
    
    # Capture process status if available
    if [[ -f "/proc/${pid}/status" ]]; then
        cat "/proc/${pid}/status" > "${diag_dir}/${prefix}_status.txt" 2>/dev/null || true
    fi
    
    # Capture file descriptors if available
    if [[ -d "/proc/${pid}/fd/" ]]; then
        ls -l "/proc/${pid}/fd/" > "${diag_dir}/${prefix}_fds.txt" 2>/dev/null || true
    fi
}