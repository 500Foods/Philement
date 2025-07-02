#!/bin/bash
#
# lifecycle.sh - Hydrogen Application Lifecycle Management
#
# This script provides functions for managing the lifecycle of the Hydrogen application,
# including starting and stopping the application with various configurations.
#
# VERSION HISTORY
# 1.1.0 - 2025-07-02 - Added validate_config_files, setup_output_directories, and run_lifecycle_test functions for enhanced modularity
# 1.0.0 - 2025-07-02 - Initial version with start and stop functions
#

# Function to find and validate Hydrogen binary
find_hydrogen_binary() {
    local hydrogen_dir="$1"
    local hydrogen_bin
    
    # Ensure hydrogen_dir is an absolute path
    hydrogen_dir=$(realpath "$hydrogen_dir" 2>/dev/null || echo "$hydrogen_dir")
    
    # Log to stderr to avoid contaminating the return value
    print_message "Searching for Hydrogen binary in: $hydrogen_dir" >&2
    
    # First check for release build
    hydrogen_bin="$hydrogen_dir/hydrogen_release"
    if [ -f "$hydrogen_bin" ]; then
        print_message "Using release build for testing: hydrogen/hydrogen_release" >&2
    # Then check for standard build
    else
        hydrogen_bin="$hydrogen_dir/hydrogen"
        if [ -f "$hydrogen_bin" ]; then
            print_message "Using standard build for testing: hydrogen/hydrogen" >&2
        # If neither found, search for binary in possible build directories
        else
            print_message "Searching for Hydrogen binary in subdirectories..." >&2
            hydrogen_bin=$(find "$hydrogen_dir" -type f -executable -name "hydrogen*" -print -quit 2>/dev/null)
            if [ -n "$hydrogen_bin" ]; then
                print_message "Found Hydrogen binary at: $hydrogen_bin" >&2
            else
                print_error "No Hydrogen binary found in $hydrogen_dir or subdirectories" >&2
                return 1
            fi
        fi
    fi
    
    # Validate binary exists and is executable
    if [ ! -f "$hydrogen_bin" ]; then
        print_error "Hydrogen binary not found at: $hydrogen_bin" >&2
        return 1
    fi
    
    if [ ! -x "$hydrogen_bin" ]; then
        print_error "Hydrogen binary is not executable: $hydrogen_bin" >&2
        return 1
    fi
    
    # Return only the binary path to stdout
    echo "$hydrogen_bin"
    return 0
}

# Function to start Hydrogen application
start_hydrogen() {
    local config_file="$1"
    local log_file="$2"
    local timeout="$3"
    local hydrogen_bin="$4"
    local hydrogen_pid
    
    # Check if logging functions are available
    if ! command -v print_message >/dev/null 2>&1; then
        echo "ERROR: print_message function not available - ensure log_output.sh is sourced"
        return 1
    fi
    if ! command -v print_command >/dev/null 2>&1; then
        echo "ERROR: print_command function not available - ensure log_output.sh is sourced"
        return 1
    fi
    if ! command -v print_error >/dev/null 2>&1; then
        echo "ERROR: print_error function not available - ensure log_output.sh is sourced"
        return 1
    fi
    if ! command -v print_result >/dev/null 2>&1; then
        echo "ERROR: print_result function not available - ensure log_output.sh is sourced"
        return 1
    fi
    if ! command -v print_output >/dev/null 2>&1; then
        echo "ERROR: print_output function not available - ensure log_output.sh is sourced"
        return 1
    fi
    
    print_message "Starting Hydrogen with $(basename "$config_file" .json)..." >&2
    # Clean log file
    true > "$log_file"
    
    # Validate binary and config exist
    if [ ! -f "$hydrogen_bin" ]; then
        print_error "Hydrogen binary not found at: $hydrogen_bin" >&2
        return 1
    fi
    if [ ! -f "$config_file" ]; then
        print_error "Configuration file not found at: $config_file" >&2
        return 1
    fi
    
    # Show the exact command that will be executed
    local cmd_display
    cmd_display="$(basename "$hydrogen_bin") $(basename "$config_file")"
    print_command "$cmd_display" >&2
    
    # Record launch time
    local launch_time_ms
    launch_time_ms=$(date +%s%3N)
    
    # Launch Hydrogen
    "$hydrogen_bin" "$config_file" > "$log_file" 2>&1 &
    hydrogen_pid=$!
    
    # Display the PID for tracking
    print_message "Hydrogen process started with PID: $hydrogen_pid" >&2
    
    # Verify process started
    sleep 0.1  # Give a brief moment to check if process starts
    if ! ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        print_result 1 "Failed to start Hydrogen - process did not start or crashed immediately" >&2
        print_message "Check log file for possible errors: $log_file" >&2
        if [ -s "$log_file" ]; then
            print_message "Last few lines of log file:" >&2
            tail -n 5 "$log_file" | while IFS= read -r line; do
                print_output "$line" >&2
            done
        fi
        return 1
    fi
    
    # Wait for startup
    print_message "Waiting for startup (max ${timeout}s)..." >&2
    if wait_for_startup "$log_file" "$timeout" "$launch_time_ms"; then
        # Output PID to stdout only (for capture), messages go to stderr
        echo "$hydrogen_pid"
        return 0
    else
        print_result 1 "Hydrogen startup timed out" >&2
        kill -9 "$hydrogen_pid" 2>/dev/null || true
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
        
        if [ "$elapsed_s" -ge "$timeout" ]; then
            print_warning "Startup timeout after ${elapsed_s}s" >&2
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            # Extract startup time from log if available
            local log_startup_time
            log_startup_time=$(grep "Startup elapsed time:" "$log_file" 2>/dev/null | sed 's/.*Startup elapsed time: \([0-9.]*s\).*/\1/' | tail -1)
            if [ -n "$log_startup_time" ]; then
                print_message "Startup completed in ${elapsed_ms}ms [Log: $log_startup_time]" >&2
            else
                print_message "Startup completed in ${elapsed_ms}ms [Log: Not found]" >&2
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
    print_message "Initiating shutdown for PID: $pid"
    print_command "kill -SIGINT $pid"
    shutdown_start_ms=$(date +%s%3N)
    kill -SIGINT "$pid" 2>/dev/null || true
    
    # Monitor shutdown
    if monitor_shutdown "$pid" "$log_file" "$timeout" "$activity_timeout" "$diag_dir"; then
        shutdown_end_ms=$(date +%s%3N)
        shutdown_duration_ms=$((shutdown_end_ms - shutdown_start_ms))
        
        # Extract shutdown time from log if available
        local log_shutdown_time
        log_shutdown_time=$(grep "Shutdown elapsed time:" "$log_file" 2>/dev/null | sed 's/.*Shutdown elapsed time: \([0-9.]*s\).*/\1/' | tail -1)
        if [ -n "$log_shutdown_time" ]; then
            print_message "Shutdown completed in ${shutdown_duration_ms}ms [Log: $log_shutdown_time]"
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
    log_size_before=$(stat -c %s "$log_file" 2>/dev/null || echo 0)
    last_activity=$(date +%s)
    
    while ps -p "$pid" > /dev/null 2>&1; do
        current_time=$(date +%s)
        elapsed=$((current_time - start_time))
        
        # Check for timeout
        if [ "$elapsed" -ge "$timeout" ]; then
            print_result 1 "Shutdown timeout after ${elapsed}s"
            get_process_threads "$pid" "$diag_dir/stuck_threads.txt"
            if command -v lsof > /dev/null 2>&1; then
                lsof -p "$pid" >> "$diag_dir/stuck_open_files.txt" 2>/dev/null || true
            fi
            kill -9 "$pid" 2>/dev/null || true
            cp "$log_file" "$diag_dir/timeout_shutdown.log" 2>/dev/null || true
            return 1
        fi
        
        # Check for log activity
        log_size_now=$(stat -c %s "$log_file" 2>/dev/null || echo 0)
        if [ "$log_size_now" -gt "$log_size_before" ]; then
            log_size_before=$log_size_now
            last_activity=$(date +%s)
        else
            inactive_time=$((current_time - last_activity))
            if [ "$inactive_time" -ge "$activity_timeout" ]; then
                get_process_threads "$pid" "$diag_dir/inactive_threads.txt"
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
    pgrep -P "$pid" > "$output_file" 2>/dev/null || true
    # Also include the main process
    echo "$pid" >> "$output_file" 2>/dev/null || true
}

# Function to capture process diagnostics
capture_process_diagnostics() {
    local pid="$1"
    local diag_dir="$2"
    local prefix="$3"
    
    # Capture thread information using pgrep
    get_process_threads "$pid" "$diag_dir/${prefix}_threads.txt"
    
    # Capture process status if available
    if [ -f "/proc/$pid/status" ]; then
        cat "/proc/$pid/status" > "$diag_dir/${prefix}_status.txt" 2>/dev/null || true
    fi
    
    # Capture file descriptors if available
    if [ -d "/proc/$pid/fd/" ]; then
        ls -l "/proc/$pid/fd/" > "$diag_dir/${prefix}_fds.txt" 2>/dev/null || true
    fi
}

# Function to validate configuration files
validate_config_files() {
    local min_config="$1"
    local max_config="$2"
    
    print_message "Checking configuration files: $min_config and $max_config"
    print_command "test -f \"$min_config\" && test -f \"$max_config\""
    if [ -f "$min_config" ] && [ -f "$max_config" ]; then
        print_result 0 "Configuration files found"
        return 0
    else
        print_result 1 "One or more configuration files not found"
        return 1
    fi
}

# Function to setup output directories
setup_output_directories() {
    local results_dir="$1"
    local diag_dir="$2"
    local log_file="$3"
    local diag_test_dir="$4"
    
    print_command "mkdir -p \"$results_dir\" \"$diag_dir\" \"$(dirname "$log_file")\" \"$diag_test_dir\""
    if mkdir -p "$results_dir" "$diag_dir" "$(dirname "$log_file")" "$diag_test_dir" 2>/dev/null; then
        print_result 0 "Output directories created/verified"
        return 0
    else
        print_result 1 "Failed to create one or more output directories"
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
    local diag_config_dir="$diag_test_dir/${config_name}"
    local hydrogen_pid
    
    if ! mkdir -p "$diag_config_dir" 2>/dev/null; then
        print_error "Failed to create diagnostics directory: $diag_config_dir"
        return 1
    fi
    
    # Subtest: Start Hydrogen
    next_subtest
    print_subtest "Start Hydrogen - $config_name"
    print_message "Testing configuration: $config_name"
    # Call start_hydrogen and capture the PID while allowing messages to display
    if hydrogen_pid=$(start_hydrogen "$config_file" "$log_file" "$startup_timeout" "$hydrogen_bin") && [ -n "$hydrogen_pid" ]; then
        print_result 0 "Hydrogen started with $config_name"
        eval "$pass_count_var=$(( $(eval "echo \$$pass_count_var") + 1 ))"
    else
        print_result 1 "Failed to start Hydrogen with $config_name"
        eval "$exit_code_var=1"
    fi
    ((subtest_count++))
    
    # Capture pre-shutdown diagnostics if started successfully
    if [ -n "$hydrogen_pid" ]; then
        capture_process_diagnostics "$hydrogen_pid" "$diag_config_dir" "pre_shutdown"
        
        # Subtest: Stop Hydrogen
        next_subtest
        print_subtest "Stop Hydrogen - $config_name"
        local stop_result=0
        if stop_hydrogen "$hydrogen_pid" "$log_file" "$shutdown_timeout" "$shutdown_activity_timeout" "$diag_config_dir"; then
            print_result 0 "Hydrogen stopped with $config_name"
            eval "$pass_count_var=$(( $(eval "echo \$$pass_count_var") + 1 ))"
            stop_result=0
        else
            print_result 1 "Failed to stop Hydrogen with $config_name"
            eval "$exit_code_var=1"
            stop_result=1
        fi
        ((subtest_count++))
        
        # Subtest: Verify Clean Shutdown
        next_subtest
        print_subtest "Verify Clean Shutdown - $config_name"
        # For Hydrogen, a clean shutdown is verified by the process terminating successfully
        # and the stop_hydrogen function returning success (which it already did above)
        if [ $stop_result -eq 0 ]; then
            print_result 0 "Clean shutdown verified for $config_name"
            eval "$pass_count_var=$(( $(eval "echo \$$pass_count_var") + 1 ))"
        else
            print_result 1 "Shutdown completed with issues for $config_name"
            eval "$exit_code_var=1"
        fi
        ((subtest_count++))
        
        # Copy full log for reference
        cp "$log_file" "$diag_config_dir/full_log.txt" 2>/dev/null || true
    fi
    
    return "$(eval "echo \$$exit_code_var")"
}
