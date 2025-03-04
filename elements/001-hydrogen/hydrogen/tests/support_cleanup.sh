#!/bin/bash
#
# Hydrogen Test Cleanup Utilities
# Provides functions for cleaning up test environment and ensuring proper system state
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Clean up previous test results and diagnostics
cleanup_old_tests() {
    print_info "Cleaning up previous test results and diagnostics..."
    
    # Define directories to clean
    RESULTS_DIR="$SCRIPT_DIR/results"
    DIAGNOSTICS_DIR="$SCRIPT_DIR/diagnostics"
    
    # Remove all files in results directory
    if [ -d "$RESULTS_DIR" ]; then
        rm -rf "$RESULTS_DIR"/*
        print_info "Removed old test results from $(convert_to_relative_path "$RESULTS_DIR")"
    fi
    
    # Remove all files in diagnostics directory
    if [ -d "$DIAGNOSTICS_DIR" ]; then
        rm -rf "$DIAGNOSTICS_DIR"/*
        print_info "Removed old test diagnostics from $(convert_to_relative_path "$DIAGNOSTICS_DIR")"
    fi
    
    # Remove any log files in the tests directory
    rm -f "$SCRIPT_DIR"/*.log
    print_info "Removed old log files from test directory"
    
    print_info "Cleanup complete"
}

# Function to ensure the Hydrogen server isn't running
ensure_server_not_running() {
    print_info "Checking for running Hydrogen instances..."
    
    # Check if any Hydrogen processes are running
    HYDROGEN_PIDS=$(pgrep -f "hydrogen" || echo "")
    
    if [ ! -z "$HYDROGEN_PIDS" ]; then
        print_warning "Found running Hydrogen processes with PIDs: $HYDROGEN_PIDS"
        
        # Try graceful shutdown first
        for PID in $HYDROGEN_PIDS; do
            print_info "Attempting graceful shutdown of PID $PID..."
            kill -SIGINT $PID 2>/dev/null || true
        done
        
        # Wait a bit to allow for graceful shutdown
        sleep 5
        
        # Check if processes are still running
        HYDROGEN_PIDS=$(pgrep -f "hydrogen" || echo "")
        if [ ! -z "$HYDROGEN_PIDS" ]; then
            print_warning "Forcing termination of remaining processes: $HYDROGEN_PIDS"
            for PID in $HYDROGEN_PIDS; do
                kill -9 $PID 2>/dev/null || true
            done
        fi
    else
        print_info "No running Hydrogen processes found"
    fi
    
    # Check if port 5000 is in use
    if command -v netstat &> /dev/null; then
        PORT_IN_USE=$(netstat -tuln | grep ":5000 " | wc -l)
        if [ $PORT_IN_USE -gt 0 ]; then
            print_warning "Port 5000 is still in use, waiting for release..."
            sleep 10
        fi
    fi
    
    # Final check to ensure resources are released
    sleep 2
    print_info "Resource cleanup complete"
}

# Function to make scripts executable
make_scripts_executable() {
    print_info "Making test scripts executable..."
    
    # Find all .sh files in the tests directory and make them executable
    chmod +x "$SCRIPT_DIR"/*.sh
    
    print_info "All scripts are now executable"
}

# Initialize test environment
initialize_test_environment() {
    # Default output directories
    RESULTS_DIR="$SCRIPT_DIR/results"
    mkdir -p "$RESULTS_DIR"
    
    DIAGNOSTICS_DIR="$SCRIPT_DIR/diagnostics"
    mkdir -p "$DIAGNOSTICS_DIR"
    
    # Get timestamp for this test run
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    
    # Return timestamp
    echo "$TIMESTAMP"
}