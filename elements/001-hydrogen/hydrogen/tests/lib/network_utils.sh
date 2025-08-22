#!/usr/bin/env bash

# Network Utilities Library
# Provides network-related functions for test scripts, including TIME_WAIT socket management

# LIBRARY FUNCTIONS
# check_port_in_use()
# count_time_wait_sockets()
# check_time_wait_sockets()
# make_http_requests()

# CHANGELOG
# 2.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 2.2.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.1.0 - 2025-07-18 - Fixed subshell issue in check_time_wait_sockets function that prevented TIME_WAIT socket details from being displayed; added whitespace compression for cleaner output formatting
# 2.0.0 - 2025-07-02 - Initial creation from support_timewait.sh migration for test_55_socket_rebind.sh

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${NETWORK_UTILS_GUARD:-}" ]] && return 0
export NETWORK_UTILS_GUARD="true"

# Library metadata
NETWORK_UTILS_NAME="Network Utilities Library"
NETWORK_UTILS_VERSION="2.2.1"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${NETWORK_UTILS_NAME} ${NETWORK_UTILS_VERSION}" "info"

# Function to check if a port is in use (robust version)
check_port_in_use() {
    local port="$1"
    # Capture output and check exit status explicitly
    local ss_output
    ss_output=$(ss -tuln 2>/dev/null)
    # shellcheck disable=SC2154  # GREP defined externally in framework.sh
    echo "${ss_output}" | "${GREP}" -q ":${port}\b"
    return $?
}

# Function to count TIME_WAIT sockets for a specific port
count_time_wait_sockets() {
    local port="$1"
    local count=0
    
    # Use ss to count TIME_WAIT sockets
    # shellcheck disable=SC2126,SC2154  # Intentionally using wc -l instead of grep -c to avoid integer expression errors
    count=$(ss -tan | "${GREP}" ":${port} " | "${GREP}" "TIME-WAIT" | wc -l || true 2>/dev/null)
    
    # Clean up the count and ensure it's a valid integer
    count=$(echo "${count}" | tr -d '[:space:]' | "${GREP}" -o '^[0-9]*' | head -1 || true)
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
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Could not check for TIME_WAIT sockets"
        return 1
    fi
    
    if [[ "${time_wait_count}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${time_wait_count} socket(s) in TIME_WAIT state on port ${port}"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        # Also compress excessive whitespace for better formatting
        # shellcheck disable=SC2154  # SED defined externally in framework.sh
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < <(ss -tan | "${GREP}" ":${port} " | "${GREP}" "TIME-WAIT" | "${SED}" 's/   */ /g' || true)
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No TIME_WAIT sockets found on port ${port}"
    fi
    
    return 0
}

# Function to make HTTP requests to create active connections
make_http_requests() {
    local base_url="$1"
    local results_dir="$2"
    local timestamp="$3"
    
    # Log start
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Making HTTP requests to create active connections"
    
    # Extract port from base_url (e.g., http://localhost:8080 -> 8080)
    local port
    if [[ "${base_url}" =~ :([0-9]+) ]]; then
        port=${BASH_REMATCH[1]}
    else
        port=80
    fi
    
    # Wait for server to be ready
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for server to be ready..."
    
    local max_wait_ms=5000  # 5s in milliseconds
    local check_interval_ms=100  # 0.2s in milliseconds
    local elapsed_ms=0
    local start_time
    # shellcheck disable=SC2154  # DATE defined externally in framework.sh
    start_time=$("${DATE}" +%s%3N)  # Epoch time in milliseconds
    while [[ "${elapsed_ms}" -lt "${max_wait_ms}" ]]; do
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_port_in_use "${port}"; then
            local end_time
            end_time=$("${DATE}" +%s%3N)
            elapsed_ms=$((end_time - start_time))
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server is ready on port ${port} after ${elapsed_ms}ms"
            break
        fi
        sleep 0.05
        elapsed_ms=$((elapsed_ms + check_interval_ms))
    done
    if [[ "${elapsed_ms}" -ge "${max_wait_ms}" ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Server did not become ready on port ${port} within $((max_wait_ms / 1000))s"
    fi
    
    # Make requests to common web files
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Requesting index.html..."
    curl -s --max-time 5 "${base_url}/" -o "${results_dir}/index_response_${timestamp}.html" 2>/dev/null || true
       
    return 0
}
