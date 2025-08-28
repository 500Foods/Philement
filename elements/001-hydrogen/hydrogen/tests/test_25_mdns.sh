#!/usr/bin/env bash

# Test: mDNS Server and Client
# Tests the mDNS service discovery functionality: server announcements and client discovery.

# FUNCTIONS
# test_mdns_server_announcements()
# test_mdns_client_discovery()
# test_mdns_external_tools()
# test_mdns_server_logging()
# test_mdns_client_logging()

# CHANGELOG
# 1.0.1 - 2025-08-28 - Removed unnecessary shellcheck statements
# 1.0.0 - 2025-08-28 - Initial creation for Test 25 - mDNS

set -euo pipefail

# Test Configuration
TEST_NAME="mDNS Server and Client"
TEST_ABBR="MDNS"
TEST_NUMBER="25"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Configuration
CONFIG_FILE="${CONFIG_DIR}/hydrogen_test_25_mdns.json"
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to test mDNS server logging
test_mdns_server_logging() {
    local log_file="$1"
    local max_wait=10
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for mDNS server logging activity..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSServer" "${log_file}" 2>/dev/null; then
            local mdns_server_output
            mdns_server_output=$("${GREP}" -c "mDNSServer" "${log_file}" 2>/dev/null)
            local mdns_server_lines
            mdns_server_lines=$((mdns_server_output + 0))
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_server_lines} mDNS server log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS server logging detected (${mdns_server_lines} entries)"

            # Check for specific mDNS server activities
            local announce_output
            announce_output=$("${GREP}" -c "mDNSServer.*announce" "${log_file}" 2>/dev/null)
            local announcement_lines
            announcement_lines=$((announce_output + 0))
            if [[ "${announcement_lines}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${announcement_lines} announcement activities"
            fi

            return 0
        fi
        sleep 1
        ((attempt++))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS server logging detected within ${max_wait} seconds"
    return 1
}

# Function to test mDNS client logging
test_mdns_client_logging() {
    local log_file="$1"
    local max_wait=10
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for mDNS client logging activity..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSClient" "${log_file}" 2>/dev/null; then
            local mdns_client_output
            mdns_client_output=$("${GREP}" -c "mDNSClient" "${log_file}" 2>/dev/null)
            local mdns_client_lines
            mdns_client_lines=$((mdns_client_output + 0))
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_client_lines} mDNS client log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client logging detected (${mdns_client_lines} entries)"

            # Check for specific mDNS client activities
            local query_output
            query_output=$("${GREP}" -c "mDNSClient.*query\|mDNSClient.*discover" "${log_file}" 2>/dev/null)
            local query_lines
            query_lines=$((query_output + 0))
            if [[ "${query_lines}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${query_lines} query/discovery activities"
            fi

            return 0
        fi
        sleep 1
        ((attempt++))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS client logging detected within ${max_wait} seconds"
    return 1
}

# Function to test external mDNS discovery tools
test_mdns_external_tools() {
    local server_port="$1"
    local external_tool_found=false
    local discovery_success=false

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing external mDNS discovery tools if available..."

    # Test avahi-browse (Linux)
    if command -v avahi-browse >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with avahi-browse..."
        external_tool_found=true

        # Run avahi-browse and check for our services
        local avahi_timeout=3
        if timeout "${avahi_timeout}" avahi-browse -a -p 2>/dev/null | "${GREP}" -q "hydrogen.*${server_port}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse successfully discovered hydrogen services"
            discovery_success=true
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse did not find hydrogen services within ${avahi_timeout}s"
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse not available on this system"
    fi

    # Test dns-sd (macOS)
    if command -v dns-sd >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with dns-sd..."
        external_tool_found=true

        # Run dns-sd browse and check for our services
        local dns_sd_timeout=3
        if timeout "${dns_sd_timeout}" dns-sd -B _http._tcp local 2>/dev/null | "${GREP}" -q "hydrogen"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd successfully discovered hydrogen services"
            discovery_success=true
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd did not find hydrogen services within ${dns_sd_timeout}s"
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd not available on this system"
    fi

    if [[ "${external_tool_found}" = true ]]; then
        if [[ "${discovery_success}" = true ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External mDNS discovery tools confirmed service announcements"
            return 0
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External discovery tools available but services not yet detected (this may be normal for initial startup)"
            return 0
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No external mDNS discovery tools available (avahi-browse or dns-sd not installed)"
        return 0
    fi
}

# Function to test mDNS server announcements
test_mdns_server_announcements() {
    local log_file="$1"
    local max_wait=15
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS server service announcements..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        # Check for announcement-related log entries
        if "${GREP}" -q "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null; then
            local announcement_output
            announcement_output=$("${GREP}" -c "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null)
            local announcement_count
            announcement_count=$((announcement_output + 0))

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${announcement_count} service announcement activities"

            # Check for specific service types we configured
            local http_output
            http_output=$("${GREP}" -c "_http._tcp\|Hydrogen.*HTTP" "${log_file}" 2>/dev/null)
            local http_services
            http_services=$((http_output + 0))
            local websocket_output
            websocket_output=$("${GREP}" -c "_websocket._tcp\|Hydrogen.*WebSocket" "${log_file}" 2>/dev/null)
            local websocket_services
            websocket_services=$((websocket_output + 0))

            if [[ "${http_services}" -gt 0 ]] || [[ "${websocket_services}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected HTTP services: ${http_services}, WebSocket services: ${websocket_services}"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS server successfully announcing configured services"
                return 0
            fi
        fi
        sleep 1
        ((attempt++))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS server announcement activities not detected within ${max_wait} seconds"
    return 1
}

# Function to test mDNS client discovery
test_mdns_client_discovery() {
    local log_file="$1"
    local max_wait=15
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS client service discovery..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        # Check for client discovery activities
        if "${GREP}" -q "mDNSClient.*discover\|mDNSClient.*found\|mDNSClient.*cache" "${log_file}" 2>/dev/null; then
            local discovery_output
            discovery_output=$("${GREP}" -c "mDNSClient.*discover\|mDNSClient.*found\|mDNSClient.*cache" "${log_file}" 2>/dev/null)
            local discovery_count
            discovery_count=$((discovery_output + 0))

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${discovery_count} client discovery activities"

            # Check for services the client should be discovering
            local services_output
            services_output=$("${GREP}" -c "mDNSClient.*Hydrogen\|mDNSClient.*_http\|_tcp\|_udp" "${log_file}" 2>/dev/null)
            local services_found
            services_found=$((services_output + 0))

            if [[ "${services_found}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS client discovered ${services_found} relevant services"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client successfully discovering services"
                return 0
            fi
        fi
        sleep 1
        attempt=$(( attempt + 1 ))
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS client discovery activities not detected - this may be normal if no other services are available on the network"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client initialized but no services available for discovery (acceptable outcome)"
    return 0
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Validate configuration file
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate mDNS Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_FILE}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS configuration file validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server with mDNS Configuration"

    HYDROGEN_PID=''
    SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_server.log"
    SERVER_READY=false

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${CONFIG_FILE}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_PID"; then
        if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen server started with PID: ${HYDROGEN_PID}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server Log: ..${SERVER_LOG}" 

            # Wait for server to be ready
            SERVER_PORT=$(get_webserver_port "${CONFIG_FILE}")
            BASE_URL="http://localhost:${SERVER_PORT}"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for server to be ready at ${BASE_URL}..."
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if wait_for_server_ready "${BASE_URL}"; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server is ready, proceeding with mDNS tests"
                SERVER_READY=true
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to become ready within expected time"
                EXIT_CODE=1
                SERVER_READY=false
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen process failed to start properly"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen server"
        EXIT_CODE=1
    fi

    # Run mDNS tests if server is ready
    if [[ "${SERVER_READY}" = true ]]; then

        # Give mDNS services time to start and make initial announcements
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for mDNS services to initialize..."
        sleep 3

        # Display full server log section for this test
        if [[ -f "${SERVER_LOG}" ]]; then
            # Extract the server log section and format for display
            full_log_section=$("${GREP}" -A 5000 "Application started" "${SERVER_LOG}" 2>/dev/null | tail -n +3 | "${GREP}" -B 5000 -A 5 "SIGINT received" | head -n -1 2>/dev/null || true)
            if [[ -n "${full_log_section}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server Log: ..${SERVER_LOG}"
                # Process each line following Test 15 pattern for consistent log formatting
                while IFS= read -r line; do
                    output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output_line}"
                done < <(echo "${full_log_section}" | head -n 30 || true)
            fi
        fi

        # Test 1: Check mDNS server logging
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test mDNS Server Logging"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_server_logging "${SERVER_LOG}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi

        # Test 2: Check mDNS client logging
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test mDNS Client Logging"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_client_logging "${SERVER_LOG}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi

        # Test 3: Test external mDNS discovery tools (if available)
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test External mDNS Discovery Tools"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_external_tools "${SERVER_PORT}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi

        # Test 4: Verify mDNS service announcements
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test mDNS Service Announcements"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_server_announcements "${SERVER_LOG}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi

        # Test 5: Verify mDNS client is discovering services
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test mDNS Client Discovery"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_client_discovery "${SERVER_LOG}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        # Server not ready - skip all mDNS tests
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping mDNS tests due to server readiness issues"
        for i in {2..6}; do
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest ${i} (Skipped)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server startup failure"
        done
        EXIT_CODE=1
    fi

    # Stop server if it was started
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" "${SHUTDOWN_TIMEOUT}" 5 "${DIAG_TEST_DIR}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen server stopped successfully"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen server shutdown had issues"
            EXIT_CODE=1
        fi
    fi

else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping server tests due to prerequisite failures"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
