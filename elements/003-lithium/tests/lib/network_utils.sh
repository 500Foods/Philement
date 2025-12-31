#!/usr/bin/env bash

# Network Utilities Library
# Provides network-related functions for test scripts, including TIME_WAIT socket management

# LIBRARY FUNCTIONS
# curl_with_retry()
# check_port_in_use()
# count_time_wait_sockets()
# check_time_wait_sockets()
# make_http_requests()
# check_response_content()
# check_redirect_response()
# check_swagger_json()

# CHANGELOG
# 4.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 3.3.0 - 2025-09-19 - Added HTTP response testing functions from test_22_swagger.sh
#                    - Added check_response_content(), check_redirect_response(), check_swagger_json()
#                    - Support for test script refactoring and code reuse
# 3.2.0 - 2025-09-19 - Fixed curl_with_retry() to check file existence before using grep/head commands to prevent "No such file or directory" errors
# 3.1.0 - 2025-09-19 - Fixed curl_with_retry() to ensure parent directories exist before writing files
# 3.0.0 - 2025-09-19 - Added curl_with_retry() function for robust HTTP requests with retry logic and timing support
# 2.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 2.2.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.1.0 - 2025-07-18 - Fixed subshell issue in check_time_wait_sockets function that prevented TIME_WAIT socket details from being displayed; added whitespace compression for cleaner output formatting
# 2.0.0 - 2025-07-02 - Initial creation from support_timewait.sh migration for test_55_socket_rebind.sh

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${NETWORK_UTILS_GUARD:-}" ]] && return 0
export NETWORK_UTILS_GUARD="true"

# Library metadata
NETWORK_UTILS_NAME="Network Utilities Library"
NETWORK_UTILS_VERSION="4.0.0"
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

# Generic function for curl requests with retry logic
curl_with_retry() {
    local url="$1"
    local response_file="$2"
    local timing_file="$3"
    local follow_redirects="${4:-false}"
    local expected_content="${5:-}"
    local redirect_location="${6:-}"
    local verbose_headers="${7:-false}"


    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    local response_time="0.000"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP request attempt ${attempt} of ${max_attempts} (waiting for subsystem initialization)..."
        fi

        # Ensure parent directory exists for response file
        local response_dir
        response_dir=$(dirname "${response_file}")
        mkdir -p "${response_dir}" 2>/dev/null || true

        # Run curl with timing and capture exit code
        local timing_output
        local curl_cmd_base="curl -s --max-time 10 --compressed"

        if [[ "${follow_redirects}" = "true" ]]; then
            curl_cmd_base="${curl_cmd_base} -L"
        fi

        if [[ "${verbose_headers}" = "true" ]]; then
            curl_cmd_base="${curl_cmd_base} -v"
        fi

        # Use a simpler timing format to avoid issues with multiline strings

        if [[ "${verbose_headers}" = "true" ]]; then
            timing_output=$(${curl_cmd_base} -w '%{time_total}' -o "${response_file}" "${url}" 2>"${response_file}.headers")
            curl_exit_code=$?
            # Combine headers and body for response_file
            if [[ -f "${response_file}" ]] && [[ -f "${response_file}.headers" ]]; then
                cat "${response_file}.headers" "${response_file}" > "${response_file}.combined" 2>/dev/null || true
                mv "${response_file}.combined" "${response_file}" 2>/dev/null || true
            fi
        else
            timing_output=$(${curl_cmd_base} -w '%{time_total}' -o "${response_file}" "${url}" 2>/dev/null)
            curl_exit_code=$?
        fi

        # Extract total time from timing output (now it's just the time value)
        if [[ -n "${timing_output}" ]]; then
            # Remove any trailing 's' and ensure it's a valid number
            response_time=$(echo "${timing_output}" | sed 's/s$//' | sed 's/[^0-9.]//g' || echo "0.000")
            # Ensure we have a valid number
            if [[ ! "${response_time}" =~ ^[0-9]+\.[0-9]+$ ]]; then
                response_time="0.000"
            fi
        else
            response_time="0.000"
        fi

        # Write timing data to file if timing_file is provided and we have a valid response
        if [[ -n "${timing_file}" ]] && [[ "${response_time}" != "0.000" ]]; then
            echo "${response_time}" > "${timing_file}"
        fi

        # Timing extraction already done above, no need to do it again

        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if we got a 404 or other error response
            if [[ -f "${response_file}" ]] && { "${GREP}" -q "404 Not Found" "${response_file}" || "${GREP}" -q "<html>" "${response_file}"; }; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Endpoint still not ready after ${max_attempts} attempts"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Endpoint returned 404 or HTML error page"
                    if [[ -f "${response_file}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response content (first 5 lines):"
                        while IFS= read -r line; do
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                        done < <(head -n 5 "${response_file}" || true)
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Full response saved to: ${response_file}"
                    else
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response file not found: ${response_file}"
                    fi
                    return 1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Endpoint not ready yet (got 404/HTML), retrying..."
                    attempt=$(( attempt + 1 ))
                    continue
                fi
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully received response from ${url}"

            # Store timing data if file provided
            if [[ -n "${timing_file}" ]]; then
                echo "${response_time}" > "${timing_file}"
            fi

            # Check for expected content if provided
            if [[ -n "${expected_content}" ]]; then
                if [[ -f "${response_file}" ]] && "${GREP}" -q "${expected_content}" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content}"
                    fi
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response doesn't contain expected content: ${expected_content}"
                    if [[ -f "${response_file}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response excerpt (first 10 lines):"
                        while IFS= read -r line; do
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                        done < <(head -n 10 "${response_file}" || true)
                    else
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response file not found for excerpt: ${response_file}"
                    fi
                    return 1
                fi
            fi

            # Check for redirect if location provided
            if [[ -n "${redirect_location}" ]]; then
                if [[ -f "${response_file}" ]] && "${GREP}" -q "< HTTP/1.1 301" "${response_file}" && "${GREP}" -q "< Location: ${redirect_location}" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response is a 301 redirect to ${redirect_location} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response is a 301 redirect to ${redirect_location}"
                    fi
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response is not a redirect to ${redirect_location}"
                    if [[ -f "${response_file}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response headers:"
                        while IFS= read -r line; do
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                        done < <("${GREP}" -E "< HTTP/|< Location:" "${response_file}" || true)
                    else
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response file not found for headers: ${response_file}"
                    fi
                    return 1
                fi
            fi

            return 0
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to connect to server at ${url} (curl exit code: ${curl_exit_code})"
                return 1
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Connection failed on attempt ${attempt}, retrying..."
                attempt=$(( attempt + 1 ))
                continue
            fi
        fi
    done

    return 1
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

# Function to check HTTP response content with retry logic for subsystem readiness
check_response_content() {
    local url="$1"
    local expected_content="$2"
    local response_file="$3"
    local follow_redirects="$4"
    local timing_file="$5"  # New parameter for timing data

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 --compressed ${follow_redirects:+-L} \"${url}\""

    # shellcheck disable=SC2310 # Function invoked in if condition but we want set -e disabled here
    if curl_with_retry "${url}" "${response_file}" "${timing_file}" "${follow_redirects}" "${expected_content}"; then
        # Show response excerpt
        local line_count
        line_count=$(wc -l < "${response_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response contains ${line_count} lines"
        return 0
    else
        return 1
    fi
}

# Function to check HTTP redirect with retry logic for subsystem readiness
check_redirect_response() {
    local url="$1"
    local expected_location="$2"
    local redirect_file="$3"
    local timing_file="$4"  # New parameter for timing data

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -v -s --max-time 10 -o /dev/null \"${url}\""

    # Use curl with verbose output for headers
    # shellcheck disable=SC2310 # Function invoked in if condition but we want set -e disabled here
    if curl_with_retry "${url}" "${redirect_file}" "${timing_file}" "false" "" "${expected_location}" "true"; then
        return 0
    else
        return 1
    fi
}

# Function to check swagger.json file content with retry logic for subsystem readiness
check_swagger_json() {
    local url="$1"
    local response_file="$2"
    local timing_file="$3"  # New parameter for timing data

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 \"${url}\""

    # shellcheck disable=SC2310 # Function invoked in if condition but we want set -e disabled here
    if curl_with_retry "${url}" "${response_file}" "${timing_file}"; then
        # Check if it's valid JSON and contains expected swagger content
        if jq -e '.openapi // .swagger' "${response_file}" >/dev/null 2>&1; then
            local openapi_version
            openapi_version=$(jq -r '.openapi // .swagger // "unknown"' "${response_file}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Valid OpenAPI/Swagger specification found (version: ${openapi_version})"

            # Check for required Hydrogen API components
            if jq -e '.info.title' "${response_file}" >/dev/null 2>&1; then
                local api_title
                api_title=$(jq -r '.info.title' "${response_file}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "API Title: ${api_title}"

                if [[ "${api_title}" == *"Hydrogen"* ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "swagger.json contains valid Hydrogen API specification"
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "swagger.json doesn't appear to be for Hydrogen API (title: ${api_title})"
                    return 1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "swagger.json missing required 'info.title' field"
                return 1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "swagger.json contains invalid JSON or missing OpenAPI/Swagger version"
            return 1
        fi
    else
        return 1
    fi
}
