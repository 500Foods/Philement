#!/bin/bash

# Network Utilities Library
# Provides network-related functions for test scripts, including TIME_WAIT socket management

# LIBRARY FUNCTIONS
# check_port_in_use()
# count_time_wait_sockets()
# check_time_wait_sockets()
# wait_for_time_wait_clear()
# wait_for_port_available()
# kill_processes_on_port()
# prepare_test_environment_with_time_wait()
# make_http_requests()

# CHANGELOG
# 2.2.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.1.0 - 2025-07-18 - Fixed subshell issue in check_time_wait_sockets function that prevented TIME_WAIT socket details from being displayed; added whitespace compression for cleaner output formatting
# 2.0.0 - 2025-07-02 - Initial creation from support_timewait.sh migration for test_55_socket_rebind.sh

# Guard clause to prevent multiple sourcing
[[ -n "${NETWORK_UTILS_GUARD}" ]] && return 0
export NETWORK_UTILS_GUARD="true"

# Library metadata
NETWORK_UTILS_NAME="Network Utilities Library"
NETWORK_UTILS_VERSION="2.2.0"
print_message "${NETWORK_UTILS_NAME} ${NETWORK_UTILS_VERSION}" "info"

# Function to check if a port is in use (robust version)
check_port_in_use() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        # Capture output and check exit status explicitly
        local ss_output
        ss_output=$(ss -tuln 2>/dev/null)
        echo "${ss_output}" | grep -q ":${port}\b"
        return $?
    elif command -v netstat >/dev/null 2>&1; then
        # Capture output and check exit status explicitly
        local netstat_output
        netstat_output=$(netstat -tuln 2>/dev/null)
        echo "${netstat_output}" | grep -q ":${port}\b"
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
        count=$(ss -tan | grep ":${port} " | grep "TIME-WAIT" | wc -l || true 2>/dev/null)
    elif command -v netstat &> /dev/null; then
        # Use netstat to count TIME_WAIT sockets
        # shellcheck disable=SC2126  # Intentionally using wc -l instead of grep -c to avoid integer expression errors
        count=$(netstat -tan | grep ":${port} " | grep "TIME_WAIT" | wc -l || true 2>/dev/null)
    else
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Neither ss nor netstat available - cannot count TIME_WAIT sockets"
        else
            echo "WARNING: Neither ss nor netstat available - cannot count TIME_WAIT sockets" >&2
        fi
        return 1
    fi
    
    # Clean up the count and ensure it's a valid integer
    count=$(echo "${count}" | tr -d '[:space:]' | grep -o '^[0-9]*' | head -1 || true)
    count=${count:-0}
    
    echo "${count}"
    return 0
}

# Function to check TIME_WAIT sockets and display information
check_time_wait_sockets() {
    local port="$1"
    local time_wait_count
    
    time_wait_count=$(count_time_wait_sockets "${port}")
    local result=$?
    
    if [[ "${result}" -ne 0 ]]; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Could not check for TIME_WAIT sockets"
        else
            echo "WARNING: Could not check for TIME_WAIT sockets" >&2
        fi
        return 1
    fi
    
    if [[ "${time_wait_count}" -gt 0 ]]; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Found ${time_wait_count} socket(s) in TIME_WAIT state on port ${port}"
        else
            echo "INFO: Found ${time_wait_count} socket(s) in TIME_WAIT state on port ${port}"
        fi
        if command -v ss &> /dev/null; then
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            # Also compress excessive whitespace for better formatting
            while IFS= read -r line; do
                if command -v print_output >/dev/null 2>&1; then
                    print_output "${line}"
                else
                    echo "${line}"
                fi
            done < <(ss -tan | grep ":${port} " | grep "TIME-WAIT" | sed 's/   */ /g' || true)
        elif command -v netstat &> /dev/null; then
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            # Also compress excessive whitespace for better formatting
            while IFS= read -r line; do
                if command -v print_output >/dev/null 2>&1; then
                    print_output "${line}"
                else
                    echo "${line}"
                fi
            done < <(netstat -tan | grep ":${port} " | grep "TIME_WAIT" | sed 's/   */ /g' || true)
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No TIME_WAIT sockets found on port ${port}"
        else
            echo "INFO: No TIME_WAIT sockets found on port ${port}"
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
        print_message "Waiting for TIME_WAIT sockets to clear for port ${port} (max ${max_wait_seconds}s)"
    else
        echo "INFO: Waiting for TIME_WAIT sockets to clear for port ${port} (max ${max_wait_seconds}s)"
    fi
    
    local elapsed=0
    local time_wait_count
    
    while [[ "${elapsed}" -lt "${max_wait_seconds}" ]]; do
        # Check for TIME_WAIT sockets
        time_wait_count=$(count_time_wait_sockets "${port}")
        
        if [[ "${time_wait_count}" -eq 0 ]]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "TIME_WAIT sockets cleared for port ${port} after ${elapsed}s"
            else
                echo "INFO: TIME_WAIT sockets cleared for port ${port} after ${elapsed}s"
            fi
            return 0
        fi
        
        if command -v print_message >/dev/null 2>&1; then
            print_message "Still waiting... ${time_wait_count} TIME_WAIT socket(s) remaining (${elapsed}s elapsed)"
        else
            echo "INFO: Still waiting... ${time_wait_count} TIME_WAIT socket(s) remaining (${elapsed}s elapsed)"
        fi
        sleep "${check_interval}"
        elapsed=$((elapsed + check_interval))
    done
    
    if command -v print_warning >/dev/null 2>&1; then
        print_warning "TIME_WAIT sockets did not clear within ${max_wait_seconds}s for port ${port}"
        print_warning "Remaining TIME_WAIT sockets:"
    else
        echo "WARNING: TIME_WAIT sockets did not clear within ${max_wait_seconds}s for port ${port}" >&2
        echo "WARNING: Remaining TIME_WAIT sockets:" >&2
    fi
    if command -v ss &> /dev/null; then
        ss -tuln | grep "TIME-WAIT.*:${port} " || true
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep "TIME_WAIT.*:${port} " || true
    fi
    return 1
}

# Function to wait for a port to become available
wait_for_port_available() {
    local port="$1"
    local max_wait_seconds="${2:-60}"  # Default 60 seconds
    local check_interval="${3:-2}"     # Check every 2 seconds
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Waiting for port ${port} to become available (max ${max_wait_seconds}s)"
    else
        echo "INFO: Waiting for port ${port} to become available (max ${max_wait_seconds}s)"
    fi
    
    local elapsed=0
    
    while [[ "${elapsed}" -lt "${max_wait_seconds}" ]]; do
        # Check if port is currently in use
        if ! check_port_in_use "${port}"; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port ${port} is now available after ${elapsed}s"
            else
                echo "INFO: Port ${port} is now available after ${elapsed}s"
            fi
            return 0
        fi
        
        # Check for TIME_WAIT sockets specifically
        local time_wait_count
        time_wait_count=$(count_time_wait_sockets "${port}")
        
        if [[ "${time_wait_count}" -gt 0 ]]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port ${port} has ${time_wait_count} TIME_WAIT socket(s), waiting... (${elapsed}s elapsed)"
            else
                echo "INFO: Port ${port} has ${time_wait_count} TIME_WAIT socket(s), waiting... (${elapsed}s elapsed)"
            fi
        else
            if command -v print_message >/dev/null 2>&1; then
                print_message "Port ${port} is in use by active connection, waiting... (${elapsed}s elapsed)"
            else
                echo "INFO: Port ${port} is in use by active connection, waiting... (${elapsed}s elapsed)"
            fi
        fi
        
        sleep "${check_interval}"
        elapsed=$((elapsed + check_interval))
    done
    
    if command -v print_warning >/dev/null 2>&1; then
        print_warning "Port ${port} did not become available within ${max_wait_seconds}s"
    else
        echo "WARNING: Port ${port} did not become available within ${max_wait_seconds}s" >&2
    fi
    return 1
}

# Function to kill processes using a specific port
kill_processes_on_port() {
    local port="$1"
    local killed_any=0
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Attempting to kill processes using port ${port}..."
    else
        echo "INFO: Attempting to kill processes using port ${port}..."
    fi
    
    # Try to identify and kill the process using the port
    if command -v lsof &> /dev/null; then
        local pids
        pids=$(lsof -ti:"${port}" 2>/dev/null || echo "")
        if [[ -n "${pids}" ]]; then
            if command -v print_message >/dev/null 2>&1; then
                print_message "Killing processes using port ${port}: ${pids}"
            else
                echo "INFO: Killing processes using port ${port}: ${pids}"
            fi
            echo "${pids}" | xargs -r kill -TERM 2>/dev/null || true
            # Brief wait for graceful termination
            sleep 0.1
            echo "${pids}" | xargs -r kill -KILL 2>/dev/null || true
            killed_any=1
        fi
    elif command -v fuser &> /dev/null; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Using fuser to kill processes on port ${port}"
        else
            echo "INFO: Using fuser to kill processes on port ${port}"
        fi
        if fuser -k "${port}/tcp" 2>/dev/null; then
            killed_any=1
        fi
        # Brief wait for cleanup
        sleep 0.1
    fi
    
    if [[ ${killed_any} -eq 1 ]]; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "Processes killed, cleanup completed"
        else
            echo "INFO: Processes killed, cleanup completed"
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No processes found using port ${port}"
        else
            echo "INFO: No processes found using port ${port}"
        fi
    fi
    
    return 0
}

# Function to prepare test environment with comprehensive TIME_WAIT handling
prepare_test_environment_with_time_wait() {
    local port="$1"
    local max_wait_seconds="${2:-90}"  # Default 90 seconds for TIME_WAIT to clear
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Preparing test environment with TIME_WAIT handling for port ${port}"
    else
        echo "INFO: Preparing test environment with TIME_WAIT handling for port ${port}"
    fi
    
    # Step 1: Check if port is currently bound by an active process
    if check_port_in_use "${port}"; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Port ${port} is currently in use by an active process"
        else
            echo "WARNING: Port ${port} is currently in use by an active process" >&2
        fi
        kill_processes_on_port "${port}"
        
        # Verify the port is now free
        if check_port_in_use "${port}"; then
            if command -v print_error >/dev/null 2>&1; then
                print_error "Port ${port} is still in use after attempting to free it"
            else
                echo "ERROR: Port ${port} is still in use after attempting to free it" >&2
            fi
            return 1
        fi
    fi
    
    # Step 3: Check for TIME_WAIT sockets and wait for them to clear
    local time_wait_count
    time_wait_count=$(count_time_wait_sockets "${port}")
    
    if [[ "${time_wait_count}" -gt 0 ]]; then
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "Found ${time_wait_count} TIME_WAIT socket(s) on port ${port}"
            print_message "Waiting for TIME_WAIT sockets to clear (max ${max_wait_seconds}s)..."
        else
            echo "WARNING: Found ${time_wait_count} TIME_WAIT socket(s) on port ${port}" >&2
            echo "INFO: Waiting for TIME_WAIT sockets to clear (max ${max_wait_seconds}s)..."
        fi
        
        if ! wait_for_time_wait_clear "${port}" "${max_wait_seconds}" 3; then
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
                ss -tan | grep ":${port} " | grep "TIME-WAIT" || true
            elif command -v netstat &> /dev/null; then
                netstat -tan | grep ":${port} " | grep "TIME_WAIT" || true
            fi
        fi
    else
        if command -v print_message >/dev/null 2>&1; then
            print_message "No TIME_WAIT sockets found on port ${port}"
        else
            echo "INFO: No TIME_WAIT sockets found on port ${port}"
        fi
    fi
    
    # Step 4: Final verification that port is available
    if check_port_in_use "${port}"; then
        if command -v print_error >/dev/null 2>&1; then
            print_error "Port ${port} is still in use after cleanup"
        else
            echo "ERROR: Port ${port} is still in use after cleanup" >&2
        fi
        return 1
    fi
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "Test environment prepared successfully for port ${port}"
    else
        echo "INFO: Test environment prepared successfully for port ${port}"
    fi
    return 0
}
# Function to make HTTP requests to create active connections
make_http_requests() {
    local base_url="$1"
    local results_dir="$2"
    local timestamp="$3"
    
    # Cache type checks
    local use_print_message=0 use_print_warning=0
    if type print_message >/dev/null 2>&1; then
        use_print_message=1
    fi
    if type print_warning >/dev/null 2>&1; then
        use_print_warning=1
    fi
    
    # Log start
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "Making HTTP requests to create active connections"
    else
        echo "INFO: Making HTTP requests to create active connections"
    fi
    
    # Extract port from base_url (e.g., http://localhost:8080 -> 8080)
    local port
    if [[ "${base_url}" =~ :([0-9]+) ]]; then
        port=${BASH_REMATCH[1]}
    else
        port=80
    fi
    
    # Wait for server to be ready
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "Waiting for server to be ready..."
    else
        echo "INFO: Waiting for server to be ready..."
    fi
    local max_wait_ms=5000  # 5s in milliseconds
    local check_interval_ms=100  # 0.2s in milliseconds
    local elapsed_ms=0
    local start_time
    start_time=$(date +%s%3N)  # Epoch time in milliseconds
    while [[ "${elapsed_ms}" -lt "${max_wait_ms}" ]]; do
        if check_port_in_use "${port}"; then
            local end_time
            end_time=$(date +%s%3N)
            elapsed_ms=$((end_time - start_time))
            if [[ "${use_print_message}" -eq 1 ]]; then
                print_message "Server is ready on port ${port} after ${elapsed_ms}ms"
            else
                echo "INFO: Server is ready on port ${port} after ${elapsed_ms}ms"
            fi
            break
        fi
        sleep 0.05
        elapsed_ms=$((elapsed_ms + check_interval_ms))
    done
    if [[ "${elapsed_ms}" -ge "${max_wait_ms}" ]]; then
        if [[ "${use_print_warning}" -eq 1 ]]; then
            print_warning "Server did not become ready on port ${port} within $((max_wait_ms / 1000))s"
        else
            echo "WARNING: Server did not become ready on port ${port} within $((max_wait_ms / 1000))s" >&2
        fi
    fi
    
    # Make requests to common web files
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "Requesting index.html..."
    else
        echo "INFO: Requesting index.html..."
    fi
    curl -s --max-time 5 "${base_url}/" -o "${results_dir}/index_response_${timestamp}.html" 2>/dev/null || true
    
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "Requesting favicon.ico..."
    else
        echo "INFO: Requesting favicon.ico..."
    fi
    curl -s --max-time 5 "${base_url}/favicon.ico" -o "${results_dir}/favicon_response_${timestamp}.ico" 2>/dev/null || true
    
    # Batch additional requests
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "Making additional requests to establish multiple connections..."
    else
        echo "INFO: Making additional requests to establish multiple connections..."
    fi
    curl -s --max-time 5 "${base_url}/robots.txt" "${base_url}/sitemap.xml" -o /dev/null 2>/dev/null || true
    
    if [[ "${use_print_message}" -eq 1 ]]; then
        print_message "HTTP requests completed - connections established"
    else
        echo "INFO: HTTP requests completed - connections established"
    fi
    
    return 0
}
