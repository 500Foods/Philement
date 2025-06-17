#!/bin/bash
#
# Hydrogen Startup/Shutdown Test Script
# Tests startup and shutdown with both minimal and maximal configurations
#
NAME_SCRIPT="Hydrogen Startup/Shutdown Test"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Function to find and validate hydrogen binary
find_hydrogen_binary() {
    local hydrogen_bin
    if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
        hydrogen_bin="$HYDROGEN_DIR/hydrogen_release"
        print_info "Using release build for testing" >&2
    else
        hydrogen_bin="$HYDROGEN_DIR/hydrogen"
        print_info "Using standard build" >&2
    fi
    
    # Validate binary exists and is executable
    if [ ! -f "$hydrogen_bin" ]; then
        print_error "Hydrogen binary not found: $hydrogen_bin" >&2
        return 1
    fi
    
    if [ ! -x "$hydrogen_bin" ]; then
        print_error "Hydrogen binary is not executable: $hydrogen_bin" >&2
        return 1
    fi
    
    echo "$hydrogen_bin"
    return 0
}

# Configuration
if ! HYDROGEN_BIN=$(find_hydrogen_binary); then
    exit 1
fi

# Test configurations
MIN_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_min.json"
MAX_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_max.json"

# Validate configuration files exist
validate_config_files() {
    local config_files=("$MIN_CONFIG" "$MAX_CONFIG")
    local file
    
    for file in "${config_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Configuration file not found: $file"
            return 1
        fi
    done
    return 0
}

if ! validate_config_files; then
    exit 1
fi

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Output files and directories
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_test.log"
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"

# Create output directories
create_output_directories() {
    local dirs=("$RESULTS_DIR" "$DIAG_DIR" "$(dirname "$LOG_FILE")")
    local dir
    
    for dir in "${dirs[@]}"; do
        if ! mkdir -p "$dir"; then
            print_error "Failed to create directory: $dir"
            return 1
        fi
    done
    return 0
}

if ! create_output_directories; then
    exit 1
fi

# Generate timestamp and result paths
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/startup_shutdown_test_${TIMESTAMP}.log"
DIAG_TEST_DIR="$DIAG_DIR/startup_shutdown_test_${TIMESTAMP}"

if ! mkdir -p "$DIAG_TEST_DIR"; then
    print_error "Failed to create diagnostics directory: $DIAG_TEST_DIR"
    exit 1
fi

# Initialize test counters
TOTAL_SUBTESTS=6  # 3 subtests × 2 configs
PASSED_SUBTESTS=0

# Start the test
start_test "Hydrogen Startup/Shutdown Test" | tee -a "$RESULT_LOG"

# Function to check if process is running using ps
is_process_running() {
    local pid=$1
    ps -p "$pid" > /dev/null 2>&1
}

# Function to get process threads using pgrep
get_process_threads() {
    local pid=$1
    local output_file=$2
    
    # Use pgrep to find all threads for the process
    pgrep -P "$pid" > "$output_file" 2>/dev/null || true
    # Also include the main process
    echo "$pid" >> "$output_file" 2>/dev/null || true
}

# Function to capture process diagnostics
capture_process_diagnostics() {
    local pid=$1
    local diag_dir=$2
    local prefix=$3
    
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

# Function to wait for startup completion
wait_for_startup() {
    local log_file=$1
    local timeout=$2
    local start_time
    local current_time
    local elapsed
    
    start_time=$(date +%s)
    
    while true; do
        current_time=$(date +%s)
        elapsed=$((current_time - start_time))
        
        if [ "$elapsed" -ge "$timeout" ]; then
            print_warning "Startup timeout after ${elapsed}s"
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            print_info "Startup completed in ${elapsed}s"
            return 0
        fi
        
        sleep 0.2
    done
}

# Function to monitor shutdown process
monitor_shutdown() {
    local pid=$1
    local log_file=$2
    local timeout=$3
    local activity_timeout=$4
    local diag_dir=$5
    
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
    
    while is_process_running "$pid"; do
        current_time=$(date +%s)
        elapsed=$((current_time - start_time))
        
        # Check for timeout
        if [ "$elapsed" -ge "$timeout" ]; then
            print_result 1 "Shutdown timeout after ${elapsed}s"
            
            # Collect diagnostics using pgrep
            get_process_threads "$pid" "$diag_dir/stuck_threads.txt"
            
            # Collect open files if lsof is available
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
                # Collect diagnostics but continue waiting
                get_process_threads "$pid" "$diag_dir/inactive_threads.txt"
            fi
        fi
        
        sleep 0.2
    done
    
    return 0
}

# Function to test a specific configuration
test_configuration() {
    local config_file="$1"
    local config_name
    local diag_config_dir
    local hydrogen_pid
    local shutdown_start
    local shutdown_end
    local shutdown_duration
    local startup_complete=false
    local shutdown_complete=false
    
    config_name=$(basename "$config_file" .json)
    diag_config_dir="$DIAG_TEST_DIR/${config_name}"
    
    if ! mkdir -p "$diag_config_dir"; then
        print_error "Failed to create diagnostics directory: $diag_config_dir"
        return 1
    fi
    
    print_header "Testing configuration: $config_name" | tee -a "$RESULT_LOG"
    
    # Clean log file - use true to avoid shellcheck SC2188
    true > "$LOG_FILE"
    
    # Launch Hydrogen
    print_info "Starting Hydrogen with $config_name..." | tee -a "$RESULT_LOG"
    $HYDROGEN_BIN "$config_file" > "$LOG_FILE" 2>&1 &
    hydrogen_pid=$!
    
    # Verify process started
    if ! is_process_running "$hydrogen_pid"; then
        print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    # Wait for startup
    print_info "Waiting for startup (max ${STARTUP_TIMEOUT}s)..." | tee -a "$RESULT_LOG"
    
    if wait_for_startup "$LOG_FILE" "$STARTUP_TIMEOUT"; then
        startup_complete=true
        ((PASSED_SUBTESTS++))
    fi
    
    # Capture pre-shutdown diagnostics
    capture_process_diagnostics "$hydrogen_pid" "$diag_config_dir" "pre_shutdown"
    
    # Send shutdown signal
    print_info "Initiating shutdown..." | tee -a "$RESULT_LOG"
    shutdown_start=$(date +%s)
    kill -SIGINT "$hydrogen_pid" 2>/dev/null || true
    
    # Monitor shutdown
    if monitor_shutdown "$hydrogen_pid" "$LOG_FILE" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$diag_config_dir"; then
        shutdown_end=$(date +%s)
        shutdown_duration=$((shutdown_end - shutdown_start))
        
        # Subtest 2: Shutdown timing
        if [ $shutdown_duration -lt $SHUTDOWN_TIMEOUT ]; then
            print_info "Shutdown completed in ${shutdown_duration}s" | tee -a "$RESULT_LOG"
            ((PASSED_SUBTESTS++))
        fi
        
        # Subtest 3: Clean shutdown verification
        if grep -q "Shutdown complete" "$LOG_FILE" 2>/dev/null; then
            print_info "Clean shutdown verified" | tee -a "$RESULT_LOG"
            ((PASSED_SUBTESTS++))
            shutdown_complete=true
        else
            print_warning "Shutdown completed but with issues" | tee -a "$RESULT_LOG"
            cp "$LOG_FILE" "$diag_config_dir/shutdown_issues.log" 2>/dev/null || true
        fi
    fi
    
    # Copy full log for reference
    cp "$LOG_FILE" "$diag_config_dir/full_log.txt" 2>/dev/null || true
    
    # Determine overall result for this configuration
    if [ "$startup_complete" = true ] && [ "$shutdown_complete" = true ]; then
        print_result 0 "Configuration $config_name test passed" | tee -a "$RESULT_LOG"
        return 0
    else
        print_result 1 "Configuration $config_name test failed" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Test minimal configuration
MIN_RESULT=0
print_header "Testing Minimal Configuration" | tee -a "$RESULT_LOG"
test_configuration "$MIN_CONFIG"
MIN_RESULT=$?

# Test maximal configuration
MAX_RESULT=0
print_header "Testing Maximal Configuration" | tee -a "$RESULT_LOG"
test_configuration "$MAX_CONFIG"
MAX_RESULT=$?

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASSED_SUBTESTS

# Print final summary
print_header "Test Summary" | tee -a "$RESULT_LOG"
print_info "Total subtests: $TOTAL_SUBTESTS" | tee -a "$RESULT_LOG"
print_info "Passed subtests: $PASSED_SUBTESTS" | tee -a "$RESULT_LOG"

# Determine final exit status
if [ $MIN_RESULT -eq 0 ] && [ $MAX_RESULT -eq 0 ]; then
    print_result 0 "All configuration tests passed" | tee -a "$RESULT_LOG"
    end_test 0 "Startup/Shutdown Test" | tee -a "$RESULT_LOG"
    exit 0
else
    print_result 1 "One or more configuration tests failed" | tee -a "$RESULT_LOG"
    end_test 1 "Startup/Shutdown Test" | tee -a "$RESULT_LOG"
    exit 1
fi
