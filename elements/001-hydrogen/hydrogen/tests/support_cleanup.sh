#!/bin/bash
#
# Hydrogen Test Cleanup Utilities
# Provides functions for cleaning up test environment and ensuring proper system state
#

# About this Script
NAME_SCRIPT="Hydrogen Test Cleanup Utilities"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.1 - 2025-06-16 - Added port checking and resource cleanup validation
# 1.0.0 - 2025-06-15 - Initial version with basic cleanup functions

# Script configuration
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
readonly SCRIPT_DIR

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Configuration constants
readonly DEFAULT_HYDROGEN_PORT=5000
readonly GRACEFUL_SHUTDOWN_TIMEOUT=5
readonly FINAL_CLEANUP_DELAY=2
readonly PORT_RELEASE_TIMEOUT=10

# Validate directory path before cleanup operations
validate_cleanup_directory() {
    local dir_path="$1"
    local dir_name="$2"
    
    # Ensure directory path is not empty and doesn't resolve to root
    if [ -z "$dir_path" ] || [ "$dir_path" = "/" ]; then
        print_warning "Invalid $dir_name directory path: '$dir_path' - skipping cleanup"
        return 1
    fi
    
    # Ensure directory exists and is within expected test structure
    if [ ! -d "$dir_path" ]; then
        print_info "$dir_name directory does not exist: $dir_path"
        return 1
    fi
    
    return 0
}

# Clean up files in a specific directory with safety checks
cleanup_directory() {
    local dir_path="$1"
    local dir_name="$2"
    
    if validate_cleanup_directory "$dir_path" "$dir_name"; then
        # Use parameter expansion with error checking to prevent /* expansion
        if [ -n "$(ls -A "${dir_path:?}" 2>/dev/null)" ]; then
            rm -rf "${dir_path:?}"/*
            print_info "Removed old $dir_name from $(convert_to_relative_path "$dir_path")"
        else
            print_info "$dir_name directory is already clean: $(convert_to_relative_path "$dir_path")"
        fi
    fi
}

# Clean up previous test results and diagnostics
cleanup_old_tests() {
    print_info "Cleaning up previous test results and diagnostics..."
    
    # Define directories to clean with validation
    local results_dir="$SCRIPT_DIR/results"
    local diagnostics_dir="$SCRIPT_DIR/diagnostics"
    local logs_dir="$SCRIPT_DIR/logs"
    
    # Clean each directory safely
    cleanup_directory "$results_dir" "test results"
    cleanup_directory "$diagnostics_dir" "test diagnostics"
    cleanup_directory "$logs_dir" "test logs"
    
    # Remove any log files in the tests directory
    if find "$SCRIPT_DIR" -maxdepth 1 -name "*.log" -type f | grep -q .; then
        rm -f "$SCRIPT_DIR"/*.log
        print_info "Removed old log files from test directory"
    else
        print_info "No log files found in test directory"
    fi
    
    print_info "Cleanup complete"
}

# Get list of running Hydrogen processes
get_hydrogen_processes() {
    pgrep -f "hydrogen" 2>/dev/null || true
}

# Attempt graceful shutdown of Hydrogen processes
graceful_shutdown_processes() {
    local pids="$1"
    
    if [ -n "$pids" ]; then
        print_info "Attempting graceful shutdown of processes: $pids"
        
        # Send SIGINT to each process
        for pid in $pids; do
            if kill -SIGINT "$pid" 2>/dev/null; then
                print_info "Sent SIGINT to PID $pid"
            else
                print_warning "Failed to send SIGINT to PID $pid (may have already exited)"
            fi
        done
        
        # Wait for graceful shutdown
        sleep $GRACEFUL_SHUTDOWN_TIMEOUT
        return 0
    fi
    
    return 1
}

# Force termination of remaining Hydrogen processes
force_terminate_processes() {
    local pids="$1"
    
    if [ -n "$pids" ]; then
        print_warning "Forcing termination of remaining processes: $pids"
        
        for pid in $pids; do
            if kill -9 "$pid" 2>/dev/null; then
                print_info "Force terminated PID $pid"
            else
                print_warning "Failed to force terminate PID $pid (may have already exited)"
            fi
        done
        return 0
    fi
    
    return 1
}

# Check if a specific port is in use
check_port_usage() {
    local port="$1"
    local port_count=0
    
    if command -v netstat &> /dev/null; then
        # Use grep -c instead of grep|wc -l for better performance
        port_count=$(netstat -tuln | grep -c ":$port " || true)
    elif command -v ss &> /dev/null; then
        # Alternative using ss command
        port_count=$(ss -tuln | grep -c ":$port " || true)
    else
        print_warning "Neither netstat nor ss available - cannot check port usage"
        return 0
    fi
    
    return "$port_count"
}

# Wait for port to be released
wait_for_port_release() {
    local port="$1"
    
    if check_port_usage "$port"; then
        local port_count=$?
        if [ "$port_count" -gt 0 ]; then
            print_warning "Port $port is still in use ($port_count connections), waiting for release..."
            sleep $PORT_RELEASE_TIMEOUT
            
            # Check again after waiting
            if check_port_usage "$port"; then
                local final_count=$?
                if [ "$final_count" -gt 0 ]; then
                    print_warning "Port $port still has $final_count connections after timeout"
                else
                    print_info "Port $port has been released"
                fi
            fi
        else
            print_info "Port $port is available"
        fi
    fi
}

# Function to ensure the Hydrogen server isn't running
ensure_server_not_running() {
    print_info "Checking for running Hydrogen instances..."
    
    # Get initial list of Hydrogen processes
    local hydrogen_pids
    hydrogen_pids=$(get_hydrogen_processes)
    
    if [ -n "$hydrogen_pids" ]; then
        print_warning "Found running Hydrogen processes with PIDs: $hydrogen_pids"
        
        # Attempt graceful shutdown
        if graceful_shutdown_processes "$hydrogen_pids"; then
            # Check if processes are still running after graceful shutdown
            hydrogen_pids=$(get_hydrogen_processes)
            
            # Force terminate any remaining processes
            if [ -n "$hydrogen_pids" ]; then
                force_terminate_processes "$hydrogen_pids"
            else
                print_info "All processes shut down gracefully"
            fi
        fi
    else
        print_info "No running Hydrogen processes found"
    fi
    
    # Check and wait for port release
    wait_for_port_release $DEFAULT_HYDROGEN_PORT
    
    # Final cleanup delay to ensure resources are released
    sleep $FINAL_CLEANUP_DELAY
    print_info "Resource cleanup complete"
}

# Function to make scripts executable
make_scripts_executable() {
    print_info "Making test scripts executable..."
    
    # Find and make all .sh files executable, with error handling
    local script_count=0
    if find "$SCRIPT_DIR" -maxdepth 1 -name "*.sh" -type f | grep -q .; then
        chmod +x "$SCRIPT_DIR"/*.sh
        script_count=$(find "$SCRIPT_DIR" -maxdepth 1 -name "*.sh" -type f | wc -l)
        print_info "Made $script_count test scripts executable"
    else
        print_warning "No shell scripts found in test directory"
    fi
}

# Create directory with error handling
create_directory_safe() {
    local dir_path="$1"
    local dir_name="$2"
    
    if mkdir -p "$dir_path" 2>/dev/null; then
        print_info "Created $dir_name directory: $(convert_to_relative_path "$dir_path")"
        return 0
    else
        print_warning "Failed to create $dir_name directory: $dir_path"
        return 1
    fi
}

# Initialize test environment with comprehensive setup
initialize_test_environment() {
    print_info "Initializing test environment..."
    
    # Create required directories
    local results_dir="$SCRIPT_DIR/results"
    local diagnostics_dir="$SCRIPT_DIR/diagnostics"
    local logs_dir="$SCRIPT_DIR/logs"
    
    create_directory_safe "$results_dir" "results"
    create_directory_safe "$diagnostics_dir" "diagnostics"
    create_directory_safe "$logs_dir" "logs"
    
    # Generate timestamp for this test run
    local timestamp
    timestamp=$(date +%Y%m%d_%H%M%S)
    
    print_info "Test environment initialized with timestamp: $timestamp"
    
    # Return timestamp for use by calling scripts
    echo "$timestamp"
}

# Validate test environment health
validate_test_environment() {
    print_info "Validating test environment health..."
    
    local validation_errors=0
    
    # Check required directories exist and are writable
    local required_dirs=("$SCRIPT_DIR/results" "$SCRIPT_DIR/diagnostics" "$SCRIPT_DIR/logs")
    
    for dir in "${required_dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            print_warning "Required directory missing: $(convert_to_relative_path "$dir")"
            ((validation_errors++))
        elif [ ! -w "$dir" ]; then
            print_warning "Directory not writable: $(convert_to_relative_path "$dir")"
            ((validation_errors++))
        fi
    done
    
    # Check for any remaining Hydrogen processes
    local remaining_processes
    remaining_processes=$(get_hydrogen_processes)
    if [ -n "$remaining_processes" ]; then
        print_warning "Hydrogen processes still running: $remaining_processes"
        ((validation_errors++))
    fi
    
    # Check port availability
    if check_port_usage $DEFAULT_HYDROGEN_PORT; then
        local port_usage=$?
        if [ "$port_usage" -gt 0 ]; then
            print_warning "Port $DEFAULT_HYDROGEN_PORT still in use ($port_usage connections)"
            ((validation_errors++))
        fi
    fi
    
    if [ $validation_errors -eq 0 ]; then
        print_info "Test environment validation passed"
        return 0
    else
        print_warning "Test environment validation found $validation_errors issues"
        return 1
    fi
}

# Comprehensive cleanup function that combines all cleanup operations
full_cleanup() {
    print_info "$NAME_SCRIPT v$VERS_SCRIPT - Starting full cleanup..."
    
    # Stop any running servers
    ensure_server_not_running
    
    # Clean up old test artifacts
    cleanup_old_tests
    
    # Make scripts executable
    make_scripts_executable
    
    # Initialize fresh environment
    local timestamp
    timestamp=$(initialize_test_environment)
    
    # Validate the environment
    if validate_test_environment; then
        print_info "Full cleanup completed successfully (timestamp: $timestamp)"
        return 0
    else
        print_warning "Full cleanup completed with warnings"
        return 1
    fi
}

# Main execution when script is run directly
main() {
    if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
        echo "$NAME_SCRIPT v$VERS_SCRIPT"
        echo "This script provides cleanup utilities for Hydrogen tests."
        echo ""
        echo "Available functions:"
        echo "  - cleanup_old_tests: Remove old test results and logs"
        echo "  - ensure_server_not_running: Stop Hydrogen processes and free ports"
        echo "  - make_scripts_executable: Make all test scripts executable"
        echo "  - initialize_test_environment: Set up fresh test environment"
        echo "  - validate_test_environment: Check environment health"
        echo "  - full_cleanup: Perform comprehensive cleanup and setup"
        echo ""
        echo "Usage: source $0  # Then call individual functions"
        echo "   or: $0         # For this help message"
    fi
}

# Execute main function when script is run directly
main "$@"
