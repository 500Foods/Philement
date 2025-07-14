#!/bin/bash
# Improved PID validation for start_hydrogen_with_pid function
# This shows the recommended enhancement to address the race condition

# Enhanced PID validation function
validate_process_start() {
    local pid="$1"
    local max_attempts=10
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if ps -p "$pid" > /dev/null 2>&1; then
            # Additional check: verify process is actually the hydrogen binary
            if ps -p "$pid" -o comm= 2>/dev/null | grep -q "hydrogen"; then
                return 0  # Process is valid and running
            fi
        fi
        
        sleep 0.1
        ((attempt++))
    done
    
    return 1  # Process failed to start or crashed
}

# Enhanced start_hydrogen_with_pid function (relevant portion)
start_hydrogen_with_pid_enhanced() {
    local config_file="$1"
    local log_file="$2"
    local timeout="$3"
    local hydrogen_bin="$4"
    local pid_var="$5"
    local hydrogen_pid
    
    # Clear the PID variable
    eval "$pid_var=''"
    
    # Validate parameters
    if [ ! -f "$hydrogen_bin" ]; then
        print_error "Hydrogen binary not found at: $hydrogen_bin"
        return 1
    fi
    if [ ! -f "$config_file" ]; then
        print_error "Configuration file not found at: $config_file"
        return 1
    fi
    
    # Launch Hydrogen
    "$hydrogen_bin" "$config_file" > "$log_file" 2>&1 &
    hydrogen_pid=$!
    disown "$hydrogen_pid" 2>/dev/null || true
    
    # Enhanced PID validation
    if ! validate_process_start "$hydrogen_pid"; then
        print_result 1 "Failed to start Hydrogen - process did not start or crashed immediately"
        return 1
    fi
    
    # Set the PID in the reference variable
    eval "$pid_var='$hydrogen_pid'"
    return 0
}
