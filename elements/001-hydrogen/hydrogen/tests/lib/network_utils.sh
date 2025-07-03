#!/bin/bash
#
# Hydrogen Test Suite - Network Utilities Library
# Provides network-related functions for test scripts, including TIME_WAIT socket management
#
NETWORK_UTILS_NAME="Hydrogen Network Utilities Library"
NETWORK_UTILS_VERSION="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-07-02 - Initial creation from support_timewait.sh migration for test_55_socket_rebind.sh

# Function to display script version information
print_network_utils_version() {
    echo "=== $NETWORK_UTILS_NAME v$NETWORK_UTILS_VERSION ==="
}

# Function to check if a port is in use (robust version)
check_port_in_use() {
    local port="$1"
    if command -v ss &> /dev/null; then
        ss -tuln | grep ":$port " > /dev/null
        return $?
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep ":$port " > /dev/null
        return $?
    else
        if command -v print_error >/dev/null 2>&1; then
            print_error "Neither ss nor netstat is available"
        else
            echo "ERROR: Neither ss nor netstat is available" >&2
        fi
        return 1
    fi
}

# Function to count TIME_WAIT sockets for a specific port
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
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Neither ss nor netstat available - cannot count TIME_WAIT sockets"
        else
            echo "WARNING: Neither ss nor netstat available - cannot count TIME_WAIT sockets" >&2
        fi
        return 1
    fi
    
    # Clean up the count and ensure it's a valid integer
    count=$(echo "$count" | tr -d '[:space:]' | grep -o '^[0-9]*' | head -1)
    count=${count:-0}
    
    echo "$count"
    return 0
}

# Function to check TIME_WAIT sockets and display information
check_time_wait_sockets() {
    local port="$1"
    local time_wait_count
    
    time_wait_count=$(count_time_wait_sockets "$port")
    local result=$?
    
    if [ $result -ne 0 ]; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Could not check for TIME_WAIT sockets"
        else
            echo "WARNING: Could not check for TIME_WAIT sockets" >&2
        fi
        return 1
    fi
    
    if [ "$time_wait_count" -gt 0 ]; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Found $time_wait_count socket(s) in TIME_WAIT state on port $port"
        else
            echo "INFO: Found $time_wait_count socket(s) in TIME_WAIT state on port $port"
        fi
        if command -v ss &> /dev/null; then
            ss -tan | grep ":$port " | grep "TIME-WAIT" | while IFS= read -r line; do
                if command -v print_output >/dev/null 2>&1; then
                    print_output "$line"
                else
                    echo "$line"
                fi
            done
        elif command -v netstat &> /dev/null; then
            netstat -tan | grep ":$port " | grep "TIME_WAIT" | while IFS= read -r line; do
                if command -v print_output >/dev/null 2>&1; then
                    print_output "$line"
                else
                    echo "$line"
                fi
            done
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No TIME_WAIT sockets found on port $port"
        else
            echo "INFO: No TIME_WAIT sockets found on port $port"
        fi
    fi
    
    return 0
}

# Function to wait for TIME_WAIT sockets to clear for a specific port
wait_for_time_wait_clear() {
    local port="$1"
    local max_wait_seconds="${2:-60}"  # Default 60 seconds (TIME_WAIT typically lasts 60s)
    local check_interval="${3:-2}"     # Check every 2 seconds
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Waiting for TIME_WAIT sockets to clear for port $port (max ${max_wait_seconds}s)"
    else
        echo "INFO: Waiting for TIME_WAIT sockets to clear for port $port (max ${max_wait_seconds}s)"
    fi
    
    local elapsed=0
    local time_wait_count
    
    while [ "$elapsed" -lt "$max_wait_seconds" ]; do
        # Check for TIME_WAIT sockets
        time_wait_count=$(count_time_wait_sockets "$port")
        
        if [ "$time_wait_count" -eq 0 ]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "TIME_WAIT sockets cleared for port $port after ${elapsed}s"
            else
                echo "INFO: TIME_WAIT sockets cleared for port $port after ${elapsed}s"
            fi
            return 0
        fi
        
        if command -v print_message >/dev/null 2>&1; then
            print_message "Still waiting... $time_wait_count TIME_WAIT socket(s) remaining (${elapsed}s elapsed)"
        else
            echo "INFO: Still waiting... $time_wait_count TIME_WAIT socket(s) remaining (${elapsed}s elapsed)"
        fi
        sleep "$check_interval"
        elapsed=$((elapsed + check_interval))
    done
    
    if command -v print_warning >/dev/null 2>&1; then
        print_warning "TIME_WAIT sockets did not clear within ${max_wait_seconds}s for port $port"
        print_warning "Remaining TIME_WAIT sockets:"
    else
        echo "WARNING: TIME_WAIT sockets did not clear within ${max_wait_seconds}s for port $port" >&2
        echo "WARNING: Remaining TIME_WAIT sockets:" >&2
    fi
    if command -v ss &> /dev/null; then
        ss -tuln | grep "TIME-WAIT.*:$port " || true
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep "TIME_WAIT.*:$port " || true
    fi
    return 1
}

# Function to wait for a port to become available
wait_for_port_available() {
    local port="$1"
    local max_wait_seconds="${2:-60}"  # Default 60 seconds
    local check_interval="${3:-2}"     # Check every 2 seconds
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Waiting for port $port to become available (max ${max_wait_seconds}s)"
    else
        echo "INFO: Waiting for port $port to become available (max ${max_wait_seconds}s)"
    fi
    
    local elapsed=0
    
    while [ "$elapsed" -lt "$max_wait_seconds" ]; do
        # Check if port is currently in use
        if ! check_port_in_use "$port"; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port $port is now available after ${elapsed}s"
            else
                echo "INFO: Port $port is now available after ${elapsed}s"
            fi
            return 0
        fi
        
        # Check for TIME_WAIT sockets specifically
        local time_wait_count
        time_wait_count=$(count_time_wait_sockets "$port")
        
        if [ "$time_wait_count" -gt 0 ]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port $port has $time_wait_count TIME_WAIT socket(s), waiting... (${elapsed}s elapsed)"
            else
                echo "INFO: Port $port has $time_wait_count TIME_WAIT socket(s), waiting... (${elapsed}s elapsed)"
            fi
        else
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port $port is in use by active connection, waiting... (${elapsed}s elapsed)"
            else
                echo "INFO: Port $port is in use by active connection, waiting... (${elapsed}s elapsed)"
            fi
        fi
        
        sleep "$check_interval"
        elapsed=$((elapsed + check_interval))
    done
    
    if command -v print_warning >/dev/null 2>&1; then
        print_warning "Port $port did not become available within ${max_wait_seconds}s"
    else
        echo "WARNING: Port $port did not become available within ${max_wait_seconds}s" >&2
    fi
    return 1
}

# Function to kill processes using a specific port
kill_processes_on_port() {
    local port="$1"
    local killed_any=0
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Attempting to kill processes using port $port..."
    else
        echo "INFO: Attempting to kill processes using port $port..."
    fi
    
    # Try to identify and kill the process using the port
    if command -v lsof &> /dev/null; then
        local pids
        pids=$(lsof -ti:"$port" 2>/dev/null || echo "")
        if [ -n "$pids" ]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Killing processes using port $port: $pids"
            else
                echo "INFO: Killing processes using port $port: $pids"
            fi
            echo "$pids" | xargs -r kill -TERM 2>/dev/null || true
            # Brief wait for graceful termination
            sleep 1
            echo "$pids" | xargs -r kill -KILL 2>/dev/null || true
            killed_any=1
        fi
    elif command -v fuser &> /dev/null; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Using fuser to kill processes on port $port"
        else
            echo "INFO: Using fuser to kill processes on port $port"
        fi
        if fuser -k "$port/tcp" 2>/dev/null; then
            killed_any=1
        fi
        # Brief wait for cleanup
        sleep 1
    fi
    
    if [ $killed_any -eq 1 ]; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Processes killed, cleanup completed"
        else
            echo "INFO: Processes killed, cleanup completed"
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No processes found using port $port"
        else
            echo "INFO: No processes found using port $port"
        fi
    fi
    
    return 0
}

# Function to prepare test environment with comprehensive TIME_WAIT handling
prepare_test_environment_with_time_wait() {
    local port="$1"
    local max_wait_seconds="${2:-90}"  # Default 90 seconds for TIME_WAIT to clear
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Preparing test environment with TIME_WAIT handling for port $port"
    else
        echo "INFO: Preparing test environment with TIME_WAIT handling for port $port"
    fi
    
    # Step 1: Check if port is currently bound by an active process
    if check_port_in_use "$port"; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Port $port is currently in use by an active process"
        else
            echo "WARNING: Port $port is currently in use by an active process" >&2
        fi
        kill_processes_on_port "$port"
        
        # Verify the port is now free
        if check_port_in_use "$port"; then
            if command -v print_error >/dev/null 2>&1; then
                print_error "Port $port is still in use after attempting to free it"
            else
                echo "ERROR: Port $port is still in use after attempting to free it" >&2
            fi
            return 1
        fi
    fi
    
    # Step 3: Check for TIME_WAIT sockets and wait for them to clear
    local time_wait_count
    time_wait_count=$(count_time_wait_sockets "$port")
    
    if [ "$time_wait_count" -gt 0 ]; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Found $time_wait_count TIME_WAIT socket(s) on port $port"
            print_message "Waiting for TIME_WAIT sockets to clear (max ${max_wait_seconds}s)..."
        else
            echo "WARNING: Found $time_wait_count TIME_WAIT socket(s) on port $port" >&2
            echo "INFO: Waiting for TIME_WAIT sockets to clear (max ${max_wait_seconds}s)..."
        fi
        
        if ! wait_for_time_wait_clear "$port" "$max_wait_seconds" 3; then
            if command -v print_warning >/dev/null 2>&1; then
                print_warning "TIME_WAIT sockets did not clear within ${max_wait_seconds}s"
                print_message "Continuing anyway - SO_REUSEADDR should handle this"
            else
                echo "WARNING: TIME_WAIT sockets did not clear within ${max_wait_seconds}s" >&2
                echo "INFO: Continuing anyway - SO_REUSEADDR should handle this"
            fi
            
            # Show remaining TIME_WAIT sockets for debugging
            if command -v print_message >/dev/null 2>&1; then
                print_message "Remaining TIME_WAIT sockets:"
            else
                echo "INFO: Remaining TIME_WAIT sockets:"
            fi
            if command -v ss &> /dev/null; then
                ss -tan | grep ":$port " | grep "TIME-WAIT" || true
            elif command -v netstat &> /dev/null; then
                netstat -tan | grep ":$port " | grep "TIME_WAIT" || true
            fi
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No TIME_WAIT sockets found on port $port"
        else
            echo "INFO: No TIME_WAIT sockets found on port $port"
        fi
    fi
    
    # Step 4: Final verification that port is available
    if check_port_in_use "$port"; then
        if command -v print_error >/dev/null 2>&1; then
            print_error "Port $port is still in use after cleanup"
        else
            echo "ERROR: Port $port is still in use after cleanup" >&2
        fi
        return 1
    fi
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Test environment prepared successfully for port $port"
    else
        echo "INFO: Test environment prepared successfully for port $port"
    fi
    return 0
}

# Function to make HTTP requests to create active connections
make_http_requests() {
    local base_url="$1"
    local results_dir="$2"
    local timestamp="$3"
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Making HTTP requests to create active connections"
    else
        echo "INFO: Making HTTP requests to create active connections"
    fi
    
    # Extract port from base_url (assumes format like http://localhost:8080)
    local port
    port=$(echo "$base_url" | grep -oE ':[0-9]+' | tr -d ':' || echo "80")
    
    # Wait for server to be ready by checking if the port is in use
    if command -v print_message >/dev/null 2>&1; then
        print_message "Waiting for server to be ready..."
    else
        echo "INFO: Waiting for server to be ready..."
    fi
    local max_wait_seconds=5
    local check_interval=0.2
    local elapsed=0.0
    while [ "$(echo "$elapsed < $max_wait_seconds" | bc)" -eq 1 ]; do
        if check_port_in_use "$port"; then
            local elapsed_ms
            elapsed_ms=$(echo "scale=0; $elapsed * 1000" | bc)
            if command -v print_message >/dev/null 2>&1; then
                print_message "Server is ready on port $port after ${elapsed_ms}ms"
            else
                echo "INFO: Server is ready on port $port after ${elapsed_ms}ms"
            fi
            break
        fi
        sleep "$check_interval"
        elapsed=$(echo "$elapsed + $check_interval" | bc)
    done
    if [ "$(echo "$elapsed >= $max_wait_seconds" | bc)" -eq 1 ]; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Server did not become ready on port $port within ${max_wait_seconds}s"
        else
            echo "WARNING: Server did not become ready on port $port within ${max_wait_seconds}s" >&2
        fi
    fi
    
    # Make requests to common web files to ensure connections are established
    if command -v print_message >/dev/null 2>&1; then
        print_message "Requesting index.html..."
    else
        echo "INFO: Requesting index.html..."
    fi
    curl -s --max-time 5 "$base_url/" > "$results_dir/index_response_${timestamp}.html" 2>/dev/null || true
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Requesting favicon.ico..."
    else
        echo "INFO: Requesting favicon.ico..."
    fi
    curl -s --max-time 5 "$base_url/favicon.ico" > "$results_dir/favicon_response_${timestamp}.ico" 2>/dev/null || true
    
    # Make a few more requests to ensure multiple connections
    if command -v print_message >/dev/null 2>&1; then
        print_message "Making additional requests to establish multiple connections..."
    else
        echo "INFO: Making additional requests to establish multiple connections..."
    fi
    curl -s --max-time 5 "$base_url/robots.txt" > /dev/null 2>&1 || true
    curl -s --max-time 5 "$base_url/sitemap.xml" > /dev/null 2>&1 || true
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "HTTP requests completed - connections established"
    else
        echo "INFO: HTTP requests completed - connections established"
    fi
    
    return 0
}
