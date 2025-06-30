#!/bin/bash
#
# Hydrogen Test TIME_WAIT Socket Management Utilities
# Provides specialized functions for handling TIME_WAIT socket conditions in tests
#
NAME_SCRIPT="Hydrogen TIME_WAIT Utilities"
VERS_SCRIPT="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-06-30 - Initial creation, extracted from support_utils.sh

# Function to display script version information
print_timewait_utils_version() {
    echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="
}

# Function to check if a port is in use (robust version)
check_port_in_use_robust() {
    local port="$1"
    if command -v ss &> /dev/null; then
        ss -tuln | grep ":$port " > /dev/null
        return $?
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep ":$port " > /dev/null
        return $?
    else
        echo "Neither ss nor netstat is available"
        return 1
    fi
}

# Function to count TIME_WAIT sockets for a specific port (robust version)
count_time_wait_sockets() {
    local port="$1"
    local count=0
    
    if command -v ss &> /dev/null; then
        # Use ss to count TIME_WAIT sockets
        # shellcheck disable=SC2126  # Intentionally using wc -l instead of grep -c to avoid integer expression errors
        count=$(ss -tan | grep ":$port " | grep "TIME-WAIT" | wc -l 2>/dev/null)
    elif command -v netstat &> /dev/null; then
        # Use netstat to count TIME_WAIT sockets
        # shellcheck disable=SC2126  # Intentionally using wc -l instead of grep -c to avoid integer expression errors
        count=$(netstat -tan | grep ":$port " | grep "TIME_WAIT" | wc -l 2>/dev/null)
    else
        print_warning "Neither ss nor netstat available - cannot count TIME_WAIT sockets"
        return 1
    fi
    
    # Clean up the count and ensure it's a valid integer
    count=$(echo "$count" | tr -d '[:space:]' | grep -o '^[0-9]*' | head -1)
    count=${count:-0}
    
    echo "$count"
    return 0
}

# Function to check TIME_WAIT sockets (robust version)
check_time_wait_sockets_robust() {
    local port="$1"
    local time_wait_count
    
    time_wait_count=$(count_time_wait_sockets "$port")
    local result=$?
    
    if [ $result -ne 0 ]; then
        print_warning "Could not check for TIME_WAIT sockets"
        return 1
    fi
    
    if [ "$time_wait_count" -gt 0 ]; then
        print_info "Found $time_wait_count socket(s) in TIME_WAIT state on port $port"
        if command -v ss &> /dev/null; then
            ss -tan | grep ":$port " | grep "TIME-WAIT"
        elif command -v netstat &> /dev/null; then
            netstat -tan | grep ":$port " | grep "TIME_WAIT"
        fi
    else
        print_info "No TIME_WAIT sockets found on port $port"
    fi
    
    return 0
}

# Function to wait for TIME_WAIT sockets to clear for a specific port
wait_for_time_wait_clear() {
    local port="$1"
    local max_wait_seconds="${2:-60}"  # Default 60 seconds (TIME_WAIT typically lasts 60s)
    local check_interval="${3:-2}"     # Check every 2 seconds
    
    print_header "Waiting for TIME_WAIT sockets to clear for port $port"
    
    local elapsed=0
    local time_wait_count
    
    while [ "$elapsed" -lt "$max_wait_seconds" ]; do
        # Check for TIME_WAIT sockets
        time_wait_count=$(count_time_wait_sockets "$port")
        
        if [ "$time_wait_count" -eq 0 ]; then
            print_info "TIME_WAIT sockets cleared for port $port after ${elapsed}s"
            return 0
        fi
        
        print_info "Still waiting... $time_wait_count TIME_WAIT socket(s) remaining (${elapsed}s elapsed)"
        sleep "$check_interval"
        elapsed=$((elapsed + check_interval))
    done
    
    print_warning "TIME_WAIT sockets did not clear within ${max_wait_seconds}s for port $port"
    print_warning "Remaining TIME_WAIT sockets:"
    if command -v ss &> /dev/null; then
        ss -tuln | grep "TIME-WAIT.*:$port " || true
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep "TIME_WAIT.*:$port " || true
    fi
    return 1
}

# Function to wait for a port to become available (smart time wait functionality)
wait_for_port_available() {
    local port="$1"
    local max_wait_seconds="${2:-60}"  # Default 60 seconds
    local check_interval="${3:-2}"     # Check every 2 seconds
    
    print_info "Waiting for port $port to become available..."
    
    local elapsed=0
    
    while [ "$elapsed" -lt "$max_wait_seconds" ]; do
        # Check if port is currently in use
        if ! check_port_in_use_robust "$port"; then
            print_info "Port $port is now available after ${elapsed}s"
            return 0
        fi
        
        # Check for TIME_WAIT sockets specifically
        local time_wait_count
        time_wait_count=$(count_time_wait_sockets "$port")
        
        if [ "$time_wait_count" -gt 0 ]; then
            print_info "Port $port has $time_wait_count TIME_WAIT socket(s), waiting... (${elapsed}s elapsed)"
        else
            print_info "Port $port is in use by active connection, waiting... (${elapsed}s elapsed)"
        fi
        
        sleep "$check_interval"
        elapsed=$((elapsed + check_interval))
    done
    
    print_warning "Port $port did not become available within ${max_wait_seconds}s"
    return 1
}

# Function to kill processes using a specific port
kill_processes_on_port() {
    local port="$1"
    local killed_any=0
    
    print_info "Attempting to kill processes using port $port..."
    
    # Try to identify and kill the process using the port
    if command -v lsof &> /dev/null; then
        local pids
        pids=$(lsof -ti:"$port" 2>/dev/null || echo "")
        if [ -n "$pids" ]; then
            print_info "Killing processes using port $port: $pids"
            echo "$pids" | xargs -r kill -TERM 2>/dev/null || true
            sleep 2
            echo "$pids" | xargs -r kill -KILL 2>/dev/null || true
            sleep 1
            killed_any=1
        fi
    elif command -v fuser &> /dev/null; then
        print_info "Using fuser to kill processes on port $port"
        if fuser -k "$port/tcp" 2>/dev/null; then
            killed_any=1
        fi
        sleep 2
    fi
    
    if [ $killed_any -eq 1 ]; then
        print_info "Processes killed, waiting for cleanup..."
        sleep 2
    else
        print_info "No processes found using port $port"
    fi
    
    return 0
}

# Function to prepare test environment with comprehensive TIME_WAIT handling
prepare_test_environment_with_time_wait_handling() {
    local port="$1"
    local max_wait_seconds="${2:-90}"  # Default 90 seconds for TIME_WAIT to clear
    
    print_header "Preparing test environment with TIME_WAIT handling for port $port"
    
    # Step 1: Kill any existing hydrogen processes
    print_info "Terminating any existing hydrogen processes..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    pkill -f hydrogen 2>/dev/null || true
    sleep 3
    
    # Step 2: Check if port is currently bound by an active process
    if check_port_in_use_robust "$port"; then
        print_warning "Port $port is currently in use by an active process"
        kill_processes_on_port "$port"
        
        # Verify the port is now free
        if check_port_in_use_robust "$port"; then
            print_error "Port $port is still in use after attempting to free it"
            return 1
        fi
    fi
    
    # Step 3: Check for TIME_WAIT sockets and wait for them to clear
    local time_wait_count
    time_wait_count=$(count_time_wait_sockets "$port")
    
    if [ "$time_wait_count" -gt 0 ]; then
        print_warning "Found $time_wait_count TIME_WAIT socket(s) on port $port"
        print_info "Waiting for TIME_WAIT sockets to clear (max ${max_wait_seconds}s)..."
        
        if ! wait_for_time_wait_clear "$port" "$max_wait_seconds" 3; then
            print_warning "TIME_WAIT sockets did not clear within ${max_wait_seconds}s"
            print_info "Continuing anyway - SO_REUSEADDR should handle this"
            
            # Show remaining TIME_WAIT sockets for debugging
            print_info "Remaining TIME_WAIT sockets:"
            if command -v ss &> /dev/null; then
                ss -tan | grep ":$port " | grep "TIME-WAIT" || true
            elif command -v netstat &> /dev/null; then
                netstat -tan | grep ":$port " | grep "TIME_WAIT" || true
            fi
        fi
    else
        print_info "No TIME_WAIT sockets found on port $port"
    fi
    
    # Step 4: Final verification that port is available
    if check_port_in_use_robust "$port"; then
        print_error "Port $port is still in use after cleanup"
        return 1
    fi
    
    print_info "Test environment prepared successfully for port $port"
    return 0
}

# Function to prepare test environment with TIME_WAIT handling (legacy compatibility)
prepare_test_environment_with_time_wait() {
    local port="$1"
    local max_wait_seconds="${2:-30}"  # Shorter default for test environments
    
    print_header "Preparing test environment for port $port"
    
    # Kill any existing hydrogen processes
    print_info "Terminating any existing hydrogen processes..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    sleep 2
    
    # Check if port is currently in use
    if check_port_in_use_robust "$port"; then
        print_warning "Port $port is currently in use"
        kill_processes_on_port "$port"
    fi
    
    # Wait for TIME_WAIT sockets to clear
    if ! wait_for_time_wait_clear "$port" "$max_wait_seconds"; then
        print_warning "TIME_WAIT sockets did not clear, but continuing with test"
        print_info "SO_REUSEADDR should handle this, but there may be binding issues"
    fi
    
    # Final verification that port is available
    if check_port_in_use_robust "$port"; then
        print_error "Port $port is still in use after cleanup"
        return 1
    fi
    
    print_info "Test environment prepared successfully for port $port"
    return 0
}
