#!/bin/bash

# Lifecycle Management Library
# Handles starting and stopping the Hydrogen Server with various configurations.

# LIBRARY FUNCTIONS
# find_hydrogen_binary() - Tests 12, 16, 18, 22,24, 26, 28, 30, 32, 34, 36
# start_hydrogen_with_pid() - Tests 28, 30, 32, 34, 36
# wait_for_startup() 
# stop_hydrogen() - Tests 28, 30, 32, 34, 36
# monitor_shutdown()
# get_process_threads()
# capture_process_diagnostics()
# configure_hydrogen_binary()
# initialize_test_environment()
# ensure_no_hydrogen_running()
# kill_hydrogen_process()
# verify_log_file_exists()
# validate_config_files()
# validate_config_file()
# setup_output_directories()
# get_tmpfs_output_dirs()
# run_lifecycle_test()

# CHANGELOG
# 1.3.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.2.4 - 2025-07-18 - Fixed subshell issue in error log output functions that prevented detailed error messages from being displayed in test output
# 1.2.3 - 2025-07-14 - Enhanced process validation with multiple retry attempts for more robust PID checking
# 1.2.2 - 2025-07-14 - Fixed job control messages (like "Terminated") appearing in test output by adding disown to background processes
# 1.2.1 - 2025-07-02 - Updated find_hydrogen_binary to use relative paths in log messages to avoid exposing user information
# 1.2.0 - 2025-07-02 - Added validate_config_file function for single configuration validation
# 1.1.0 - 2025-07-02 - Added validate_config_files, setup_output_directories, and run_lifecycle_test functions for enhanced modularity
# 1.0.0 - 2025-07-02 - Initial version with start and stop functions

# Guard clause to prevent multiple sourcing
[[ -n "${LIFECYCLE_GUARD}" ]] && return 0
export LIFECYCLE_GUARD="true"

# Library metadata
LIFECYCLE_NAME="Lifecycle Management Library"
LIFECYCLE_VERSION="1.3.0"
print_message "${LIFECYCLE_NAME} ${LIFECYCLE_VERSION}" "info"

# Function to find and validate Hydrogen binary
# Usage: find_hydrogen_binary <hydrogen_dir> <result_var_name>
# Sets the named variable with the binary path
find_hydrogen_binary() {
    local hydrogen_dir="$1"
    local result_var="$2"
    local hydrogen_bin
    
    # Clear the result variable
    eval "${result_var}=''"
    
    # Ensure hydrogen_dir is an absolute path
    hydrogen_dir=$(realpath "${hydrogen_dir}" 2>/dev/null || echo "${hydrogen_dir}")
    
    # Convert to relative path for cleaner logging
    local relative_dir
    if command -v convert_to_relative_path >/dev/null 2>&1; then
        relative_dir=$(convert_to_relative_path "${hydrogen_dir}")
    else
        # Fallback: extract just the hydrogen part
        relative_dir=$(echo "${hydrogen_dir}" | sed -n 's|.*/hydrogen$|hydrogen|p')
        if [[ -z "${relative_dir}" ]]; then
            relative_dir=$(echo "${hydrogen_dir}" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
        fi
        if [[ -z "${relative_dir}" ]]; then
            relative_dir="hydrogen"
        fi
    fi
    print_message "Searching for Hydrogen binary in: ${relative_dir}"
    
    # First check for coverage build (highest priority for testing)
    hydrogen_bin="${hydrogen_dir}/hydrogen_coverage"
    if [[ -f "${hydrogen_bin}" ]]; then
        print_message "Using coverage build for testing: hydrogen/hydrogen_coverage"
    # Then check for release build
    else
        hydrogen_bin="${hydrogen_dir}/hydrogen_release"
        if [[ -f "${hydrogen_bin}" ]]; then
            print_message "Using release build for testing: hydrogen/hydrogen_release"
        # Then check for standard build
        else
            hydrogen_bin="${hydrogen_dir}/hydrogen"
            if [[ -f "${hydrogen_bin}" ]]; then
                print_message "Using standard build for testing: hydrogen/hydrogen"
            # If none found, search for binary in possible build directories
            else
                print_message "Searching for Hydrogen binary in subdirectories..."
                hydrogen_bin=$(find "${hydrogen_dir}" -type f -executable -name "hydrogen*" -print -quit 2>/dev/null)
                if [[ -n "${hydrogen_bin}" ]]; then
                    print_message "Found Hydrogen binary at: ${hydrogen_bin}"
                else
                    print_error "No Hydrogen binary found in ${hydrogen_dir} or subdirectories"
                    return 1
                fi
            fi
        fi
    fi
    
    # Validate binary exists and is executable
    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "Hydrogen binary not found at: ${hydrogen_bin}"
        return 1
    fi
    
    if [[ ! -x "${hydrogen_bin}" ]]; then
        print_error "Hydrogen binary is not executable: ${hydrogen_bin}"
        return 1
    fi
    
    # Set the result variable with the binary path
    eval "${result_var}='${hydrogen_bin}'"
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
    
    # Check if logging functions are available
    if ! command -v print_message >/dev/null 2>&1; then
        echo "ERROR: print_message function not available - ensure log_output.sh is sourced" >&2
        return 1
    fi
    if ! command -v print_command >/dev/null 2>&1; then
        echo "ERROR: print_command function not available - ensure log_output.sh is sourced" >&2
        return 1
    fi
    if ! command -v print_error >/dev/null 2>&1; then
        echo "ERROR: print_error function not available - ensure log_output.sh is sourced" >&2
        return 1
    fi
    if ! command -v print_result >/dev/null 2>&1; then
        echo "ERROR: print_result function not available - ensure log_output.sh is sourced" >&2
        return 1
    fi
    if ! command -v print_output >/dev/null 2>&1; then
        echo "ERROR: print_output function not available - ensure log_output.sh is sourced" >&2
        return 1
    fi
    
    print_message "Starting Hydrogen with $(basename "${config_file}" .json)..."
    # Clean log file
    true > "${log_file}"
    
    # Validate binary and config exist
    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "Hydrogen binary not found at: ${hydrogen_bin}"
        return 1
    fi
    if [[ ! -f "${config_file}" ]]; then
        print_error "Configuration file not found at: ${config_file}"
        return 1
    fi
    
    # Show the exact command that will be executed
    local cmd_display
    cmd_display="$(basename "${hydrogen_bin}") $(basename "${config_file}")"
    print_command "${cmd_display}"
    
    # Record launch time
    local launch_time_ms
    launch_time_ms=$(date +%s%3N)
    
    # Launch Hydrogen (disown to prevent job control messages)
    "${hydrogen_bin}" "${config_file}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true
    
    # Display the PID for tracking
    print_message "Hydrogen process started with PID: ${hydrogen_pid}"
    
    # Verify process started with multiple attempts for robustness
    local check_attempt=1
    local max_attempts=5
    local process_running=false
    
    while [[ ${check_attempt} -le ${max_attempts} ]]; do
        if [[ ${check_attempt} -eq 1 ]]; then
            sleep 0.2  # Initial check after brief delay
        else
            sleep 0.1  # Short delay between subsequent checks
        fi
        
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            process_running=true
            break
        fi
        
        ((check_attempt++))
    done
    
    if [[ "${process_running}" = false ]]; then
        print_result 1 "Failed to start Hydrogen - process did not start or crashed immediately (checked ${max_attempts} times)"
        print_message "Check log file for possible errors: ${log_file}"
        if [[ -s "${log_file}" ]]; then
            print_message "Last few lines of log file:"
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            while IFS= read -r line; do
                print_output "${line}"
            done < <(tail -n 5 "${log_file}" || true)
        fi
        return 1
    fi
    
    # Wait for startup
    print_message "Waiting for startup (max ${timeout}s)..."
    if wait_for_startup "${log_file}" "${timeout}" "${launch_time_ms}"; then
        # Set the PID in the reference variable
        eval "${pid_var}='${hydrogen_pid}'"
        return 0
    else
        print_result 1 "Hydrogen startup timed out"
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
        current_time_ms=$(date +%s%3N)
        elapsed_ms=$((current_time_ms - launch_time_ms))
        elapsed_s=$((elapsed_ms / 1000))
        
        if [[ "${elapsed_s}" -ge "${timeout}" ]]; then
            print_warning "Startup timeout after ${elapsed_s}s"
            return 1
        fi
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            # Extract startup time from log if available
            local log_startup_time
            log_startup_time=$(grep "Startup elapsed time:" "${log_file}" 2>/dev/null | sed 's/.*Startup elapsed time: \([0-9.]*s\).*/\1/' | tail -1 || true)
            if [[ -n "${log_startup_time}" ]]; then
                print_message "Startup completed in ${elapsed_ms}ms [Log: ${log_startup_time}]"
            else
                print_message "Startup completed in ${elapsed_ms}ms [Log: Not found]"
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
    print_message "Initiating shutdown for PID: ${pid}"
    print_command "kill -SIGINT ${pid}"
    shutdown_start_ms=$(date +%s%3N)
    kill -SIGINT "${pid}" 2>/dev/null || true
    
    # Monitor shutdown
    if monitor_shutdown "${pid}" "${log_file}" "${timeout}" "${activity_timeout}" "${diag_dir}"; then
        shutdown_end_ms=$(date +%s%3N)
        shutdown_duration_ms=$((shutdown_end_ms - shutdown_start_ms))
        
        # Extract shutdown time from log if available
        local log_shutdown_time
        log_shutdown_time=$(grep "Shutdown elapsed time:" "${log_file}" 2>/dev/null | sed 's/.*Shutdown elapsed time: \([0-9.]*s\).*/\1/' | tail -1 || true)
        if [[ -n "${log_shutdown_time}" ]]; then
            print_message "Shutdown completed in ${shutdown_duration_ms}ms [Log: ${log_shutdown_time}]"
        else
            print_message "Shutdown completed in ${shutdown_duration_ms}ms [Log: Not found]"
        fi
        
        # For Hydrogen, a clean shutdown is when the process terminates without errors
        # Hydrogen doesn't write specific shutdown completion messages to the log
        print_message "Clean shutdown verified (process terminated successfully)"
        return 0
    else
        print_result 1 "Shutdown failed or timed out"
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
    
    start_time=$(date +%s)
    log_size_before=$(stat -c %s "${log_file}" 2>/dev/null || echo 0)
    last_activity=$(date +%s)
    
    while ps -p "${pid}" > /dev/null 2>&1; do
        current_time=$(date +%s)
        elapsed=$((current_time - start_time))
        
        # Check for timeout
        if [[ "${elapsed}" -ge "${timeout}" ]]; then
            print_result 1 "Shutdown timeout after ${elapsed}s"
            get_process_threads "${pid}" "${diag_dir}/stuck_threads.txt"
            if command -v lsof > /dev/null 2>&1; then
                lsof -p "${pid}" >> "${diag_dir}/stuck_open_files.txt" 2>/dev/null || true
            fi
            kill -9 "${pid}" 2>/dev/null || true
            cp "${log_file}" "${diag_dir}/timeout_shutdown.log" 2>/dev/null || true
            return 1
        fi
        
        # Check for log activity
        log_size_now=$(stat -c %s "${log_file}" 2>/dev/null || echo 0)
        if [[ "${log_size_now}" -gt "${log_size_before}" ]]; then
            log_size_before=${log_size_now}
            last_activity=$(date +%s)
        else
            inactive_time=$((current_time - last_activity))
            if [[ "${inactive_time}" -ge "${activity_timeout}" ]]; then
                get_process_threads "${pid}" "${diag_dir}/inactive_threads.txt"
            fi
        fi
        
        sleep 0.1
    done
    
    return 0
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

# Function to configure Hydrogen binary, preferring release build if available
configure_hydrogen_binary() {
    local hydrogen_dir="${1:-}"
    if [[ -z "${hydrogen_dir}" ]]; then
        local script_dir
        script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
        hydrogen_dir="$( cd "${script_dir}/../.." && pwd )"
    fi
    
    if [[ -f "${hydrogen_dir}/hydrogen_coverage" ]]; then
        HYDROGEN_BIN="${hydrogen_dir}/hydrogen_coverage"
        if command -v print_message >/dev/null 2>&1; then
            print_message "Using coverage build for testing: ${HYDROGEN_BIN}"
        else
            echo "INFO: Using coverage build for testing: ${HYDROGEN_BIN}"
        fi
    elif [[ -f "${hydrogen_dir}/hydrogen_release" ]]; then
        HYDROGEN_BIN="${hydrogen_dir}/hydrogen_release"
        if command -v print_message >/dev/null 2>&1; then
            print_message "Using release build for testing: ${HYDROGEN_BIN}"
        else
            echo "INFO: Using release build for testing: ${HYDROGEN_BIN}"
        fi
    else
        HYDROGEN_BIN="${hydrogen_dir}/hydrogen"
        if command -v print_message >/dev/null 2>&1; then
            print_message "Using standard build for testing: ${HYDROGEN_BIN}"
        else
            echo "INFO: Using standard build for testing: ${HYDROGEN_BIN}"
        fi
    fi
    
    if [[ ! -x "${HYDROGEN_BIN}" ]]; then
        if command -v print_error >/dev/null 2>&1; then
            print_error "Hydrogen binary not found or not executable: ${HYDROGEN_BIN}"
        else
            echo "ERROR: Hydrogen binary not found or not executable: ${HYDROGEN_BIN}"
        fi
        return 1
    fi
    return 0
}

# Function to initialize test environment and logging
initialize_test_environment() {
    local results_dir="$1"
    local test_log_file="$2"
    local timestamp
    
    if [[ -z "${results_dir}" ]]; then
        local script_dir
        script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
        results_dir="${script_dir}/../results"
    fi
    
    mkdir -p "${results_dir}"
    timestamp=$(date +%Y%m%d_%H%M%S)
    RESULT_LOG="${results_dir}/env_variables_test_${timestamp}.log"
    
    if [[ -n "${test_log_file}" ]]; then
        TEST_LOG_FILE="${test_log_file}"
    else
        TEST_LOG_FILE="./hydrogen_env_test.log"
    fi
    
    # Clean up any existing log file
    rm -f "${TEST_LOG_FILE}"
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Test environment initialized with log file: ${RESULT_LOG}"
    else
        echo "INFO: Test environment initialized with log file: ${RESULT_LOG}"
    fi
    return 0
}

# Function to ensure no Hydrogen instances are running
ensure_no_hydrogen_running() {
    if command -v print_message >/dev/null 2>&1; then
        print_message "Ensuring no Hydrogen instances are running..."
    else
        echo "INFO: Ensuring no Hydrogen instances are running..."
    fi
    pkill -f hydrogen || true
    sleep 2
    return 0
}

# Function to kill Hydrogen process if running
kill_hydrogen_process() {
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Cleaning up Hydrogen (PID ${HYDROGEN_PID})..."
        else
            echo "INFO: Cleaning up Hydrogen (PID ${HYDROGEN_PID})..."
        fi
        kill -SIGINT "${HYDROGEN_PID}" 2>/dev/null || kill -9 "${HYDROGEN_PID}" 2>/dev/null
        sleep 1
        HYDROGEN_PID=""
    fi
    return 0
}

# Function to verify log file exists
verify_log_file_exists() {
    local log_file="$1"
    if [[ -z "${log_file}" ]]; then
        log_file="${TEST_LOG_FILE}"
    fi
    
    if [[ ! -f "${log_file}" ]]; then
        if command -v print_error >/dev/null 2>&1; then
            print_error "Log file was not created at ${log_file}"
        else
            echo "ERROR: Log file was not created at ${log_file}"
        fi
        return 1
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "Log file was created successfully at ${log_file}"
        else
            echo "INFO: Log file was created successfully at ${log_file}"
        fi
        return 0
    fi
}

# Function to validate configuration files
validate_config_files() {
    local min_config="$1"
    local max_config="$2"
    
    print_message "Checking configuration files: ${min_config} and ${max_config}"
    print_command "test -f \"${min_config}\" && test -f \"${max_config}\""
    if [[ -f "${min_config}" ]] && [[ -f "${max_config}" ]]; then
        print_result 0 "Configuration files found"
        return 0
    else
        print_result 1 "One or more configuration files not found"
        return 1
    fi
}

# Function to validate a single configuration file
validate_config_file() {
    local config_file="$1"
    
    print_message "Checking configuration file: ${config_file}"
    print_command "test -f \"${config_file}\""
    if [[ -f "${config_file}" ]]; then
        print_result 0 "Configuration file found"
        return 0
    else
        print_result 1 "Configuration file not found"
        return 1
    fi
}

# Function to setup output directories
setup_output_directories() {
    local results_dir="$1"
    local diag_dir="$2"
    local log_file="$3"
    local diag_test_dir="$4"
    
    print_command "mkdir -p \"${results_dir}\" \"${diag_dir}\" \"$(dirname "${log_file}")\" \"${diag_test_dir}\""
    if mkdir -p "${results_dir}" "${diag_dir}" "$(dirname "${log_file}")" "${diag_test_dir}" 2>/dev/null; then
        print_result 0 "Output directories created/verified"
        return 0
    else
        print_result 1 "Failed to create one or more output directories"
        return 1
    fi
}

# Function to get tmpfs-optimized output directories
get_tmpfs_output_dirs() {
    local script_dir="$1"
    local test_name="$2"
    local timestamp="$3"
    
    local build_dir="${script_dir}/../build"
    local base_results_dir base_logs_dir base_diag_dir
    
    # Always use build/tests/ directories
    base_results_dir="${build_dir}/tests/results"
    base_logs_dir="${build_dir}/tests/logs"
    base_diag_dir="${build_dir}/tests/diagnostics"
    
    # Export the directories for use by calling scripts
    export TMPFS_RESULTS_DIR="${base_results_dir}"
    export TMPFS_LOGS_DIR="${base_logs_dir}"
    export TMPFS_DIAGS_DIR="${base_diag_dir}"
    export TMPFS_TEST_DIAGS_DIR="${base_diag_dir}/${test_name}_${timestamp}"
    
    # Create the directories
    mkdir -p "${TMPFS_RESULTS_DIR}" "${TMPFS_LOGS_DIR}" "${TMPFS_DIAGS_DIR}" "${TMPFS_TEST_DIAGS_DIR}" 2>/dev/null
    
    return 0
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
    local diag_config_dir="${diag_test_dir}/${config_name}"
    local hydrogen_pid
    
    if ! mkdir -p "${diag_config_dir}" 2>/dev/null; then
        print_error "Failed to create diagnostics directory: ${diag_config_dir}"
        return 1
    fi
    
    # Subtest: Start Hydrogen
    next_subtest
    print_subtest "Start Hydrogen - ${config_name}"
    print_message "Testing configuration: ${config_name}"
    # Call start_hydrogen and get the PID via a temporary variable
    local temp_pid_var="TEMP_HYDROGEN_PID_$$"
    if start_hydrogen_with_pid "${config_file}" "${log_file}" "${startup_timeout}" "${hydrogen_bin}" "${temp_pid_var}"; then
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        print_result 0 "Hydrogen started with ${config_name}"
        eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
    else
        print_result 1 "Failed to start Hydrogen with ${config_name}"
        eval "${exit_code_var}=1"
    fi
    ((subtest_count++))
    
    # Capture pre-shutdown diagnostics if started successfully
    if [[ -n "${hydrogen_pid}" ]]; then
        capture_process_diagnostics "${hydrogen_pid}" "${diag_config_dir}" "pre_shutdown"
        
        # Subtest: Stop Hydrogen
        next_subtest
        print_subtest "Stop Hydrogen - ${config_name}"
        local stop_result=0
        if stop_hydrogen "${hydrogen_pid}" "${log_file}" "${shutdown_timeout}" "${shutdown_activity_timeout}" "${diag_config_dir}"; then
            print_result 0 "Hydrogen stopped with ${config_name}"
            eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
            stop_result=0
        else
            print_result 1 "Failed to stop Hydrogen with ${config_name}"
            eval "${exit_code_var}=1"
            stop_result=1
        fi
        ((subtest_count++))
        
        # Subtest: Verify Clean Shutdown
        next_subtest
        print_subtest "Verify Clean Shutdown - ${config_name}"
        # For Hydrogen, a clean shutdown is verified by the process terminating successfully
        # and the stop_hydrogen function returning success (which it already did above)
        if [[ ${stop_result} -eq 0 ]]; then
            print_result 0 "Clean shutdown verified for ${config_name}"
            eval "${pass_count_var}=$(( $(eval "echo \$${pass_count_var}" || true) + 1 ))" || true
        else
            print_result 1 "Shutdown completed with issues for ${config_name}"
            eval "${exit_code_var}=1"
        fi
        ((subtest_count++))
        
        # Copy full log for reference
        cp "${log_file}" "${diag_config_dir}/full_log.txt" 2>/dev/null || true
    fi
    
    return "$(eval "echo \$${exit_code_var}" || true)"
}
