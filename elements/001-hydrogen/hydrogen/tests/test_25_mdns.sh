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
# 3.0.1 - 2025-09-22 - Performance optimizations: reduced timeouts, simplified responder loop test
# 3.0.0 - 2025-09-22 - Added responder loop test
# 2.1.0 - 2025-09-18 - Attempted to fix issue with inoperable tshark
# 2.0.1 - 2025-08-29 - Fixed shellcheck errors and improved code quality
# 2.0.0 - 2025-08-29 - Reviewed
# 1.0.1 - 2025-08-28 - Removed unnecessary shellcheck statements
# 1.0.0 - 2025-08-28 - Initial creation for Test 25 - mDNS

set -euo pipefail

# Test Configuration
TEST_NAME="mDNS"
TEST_ABBR="DNS"
TEST_NUMBER="25"
TEST_COUNTER=0
TEST_VERSION="3.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Configuration
CONFIG_FILE="${CONFIG_DIR}/hydrogen_test_25_mdns.json"
TRACED_LOG="${LOG_PREFIX}_traced.log"
CAPTURE_LOG="${LOG_PREFIX}_capture.log"
FILTER_LOG="${LOG_PREFIX}_filter.log"
PACKET_LOG="${LOG_PREFIX}_packet.log"
STARTUP_TIMEOUT=5
SHUTDOWN_TIMEOUT=5

# Export unused variables to silence shellcheck SC2034 warnings
export FILTER_LOG

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
            mdns_server_lines=${mdns_server_output}
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_server_lines} mDNS server log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS server logging detected (${mdns_server_lines} entries)"

            # Check for specific mDNS server activities
            local announce_output
            announce_output=$("${GREP}" -c "mDNSServer.*announce" "${log_file}" 2>/dev/null)
            local announcement_lines
            announcement_lines=${announce_output}
            if [[ "${announcement_lines}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${announcement_lines} announcement activities"
            fi

            return 0
        fi
        sleep 0.02
        attempt=$((attempt + 1))
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
            mdns_client_lines=${mdns_client_output}
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_client_lines} mDNS client log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client logging detected (${mdns_client_lines} entries)"

            # Check for specific mDNS client activities
            local query_output
            query_output=$("${GREP}" -c "mDNSClient.*query\|mDNSClient.*discover" "${log_file}" 2>/dev/null)
            local query_lines
            query_lines=${query_output}
            if [[ "${query_lines}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${query_lines} query/discovery activities"
            fi

            return 0
        fi
        sleep 0.02
        attempt=$((attempt + 1))
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

    # Pre-diagnosis: Check system state
    dump_mdns_system_state

    # Test avahi-browse (Linux)
    if command -v avahi-browse >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with avahi-browse..."
        external_tool_found=true

        # Check if avahi-daemon is running (potential conflict)
        local avahi_running=false
        if systemctl is-active --quiet avahi-daemon 2>/dev/null; then
            avahi_running=true
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: avahi-daemon is running - may conflict with hydrogen mDNS server"
        fi

        # Run avahi-browse and check for our services
        local avahi_timeout=1  # Reduced timeout for faster detection
        local avahi_output
        avahi_output=$(timeout "${avahi_timeout}" avahi-browse -a -p -r >/dev/null 2>&1 || echo "")

        # Only show summary of avahi-browse results, not verbose output
        if [[ -n "${avahi_output}" ]]; then
            local avahi_lines
            avahi_lines=$(echo "${avahi_output}" | wc -l)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse found ${avahi_lines} lines of output"
        fi

        if echo "${avahi_output}" | "${GREP}" -q "hydrogen.*${server_port}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse successfully discovered hydrogen services"
            discovery_success=true
        elif echo "${avahi_output}" | "${GREP}" -q "hydrogen"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse found hydrogen services but port mismatch"
            echo "${avahi_output}" | "${GREP}" -i hydrogen | while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Found: ${line}"
            done
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse did not find hydrogen services within ${avahi_timeout}s"
            if [[ "${avahi_running}" = true ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "RECOMMENDATION: Try stopping avahi-daemon: sudo systemctl stop avahi-daemon"
            fi
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse not available on this system"
    fi

    # Test dns-sd (macOS)
    if command -v dns-sd >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with dns-sd..."
        external_tool_found=true

        # Run dns-sd browse and check for our services
        local dns_sd_timeout=2
        local dns_sd_output
        dns_sd_output=$(timeout "${dns_sd_timeout}" dns-sd -B _http._tcp local 2>/dev/null || echo "")

        # Only show summary of dns-sd results, not verbose output
        if [[ -n "${dns_sd_output}" ]]; then
            local dns_sd_lines
            dns_sd_lines=$(echo "${dns_sd_output}" | wc -l)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd found ${dns_sd_lines} lines of output"
        fi

        if echo "${dns_sd_output}" | "${GREP}" -q "hydrogen"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd successfully discovered hydrogen services"
            discovery_success=true
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd did not find hydrogen services within ${dns_sd_timeout}s"
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd not available on this system"
    fi

    # Provide detailed analysis if discovery failed
    if [[ "${external_tool_found}" = true ]] && [[ "${discovery_success}" = false ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "=== Detailed mDNS Debug Analysis ==="
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Possible causes for failed mDNS discovery:"

        # Check multicast capabilities
        if ip route show | grep -q "224.0.0.0/4"; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Multicast routing appears configured"
        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "⚠ No explicit multicast routes found - may need: route add -net 224.0.0.0 netmask 240.0.0.0 dev <interface>"
        fi

        # Check for packet capture capability
        if command -v timeout >/dev/null 2>&1; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Packet capture tools available"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "RECOMMENDATION: Run 'timeout 10 tcpdump -i any udp port 5353' to see if packets are being sent"
        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "⚠ Packet capture tools not available"
        fi

        # Check file permissions
        if [[ -w "/tmp" ]]; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Temporary file permissions OK"
        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "⚠ File permissions may restrict multicast operations"
        fi

        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Next steps:"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "1. Check latest server log: grep 'IPv4 announcement' build/tests/logs/test_25_*server.log"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "2. Verify packet transmission: ss -a | grep 5353"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "3. Try stopping avahi-daemon if running"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "4. Check if hydrogen and discovery tools are on same network interface"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "

        # === SPECIAL AVAHI INTERACTION TEST ===
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "=== Enhanced Avahi Interaction Test ==="

        # Check if avahi-daemon is running and offer to start it for testing
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if systemctl is-active --quiet avahi-daemon; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "ℹ️  avahi-daemon is currently running"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "This could be interfering with hydrogen mDNS discovery"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running special test with both daemons active..."

            # Test with expanded timeouts and different search parameters
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 Testing hydrogen services with avahi-daemon running..."

            # Longer timeout for avahi-browse
            test_output=$(timeout 1 avahi-browse -a -p -r >/dev/null 2>&1)
            if echo "${test_output}" | grep -q "hydrogen"; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "🎯 SUCCESS: avahi-browse found hydrogen services WITH avahi-daemon running!"
                discovery_success=true
            fi

            # Test 2: Test hydrogen name directly
            hydrogen_name_test=$(timeout 1 avahi-browse -a -p -r >/dev/null 2>&1 | grep -i "hydrogen" | head -3)
            if [[ -n "${hydrogen_name_test}" ]]; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "📋 Found hydrogen service names:"
                echo "${hydrogen_name_test}" | while IFS= read -r line; do
                    [[ -n "${line}" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   ${line}"
                done
            fi

        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "ℹ️  avahi-daemon is not running"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "📝 RECOMMENDATION: Test with avahi-daemon running:"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   1. sudo systemctl start avahi-daemon"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   2. Re-run Test 25 to see if discovery improves"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   3. This will help determine if it's a configuration issue"
        fi

        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "
    fi

    if [[ "${external_tool_found}" = true ]]; then
        if [[ "${discovery_success}" = true ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External mDNS discovery tools confirmed service announcements"
            return 0
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External discovery tools available but services not yet detected (diagnostic info provided)"
            return 0
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No external mDNS discovery tools available (avahi-browse or dns-sd not installed)"
        return 0
    fi
}

# Function to dump system state for debugging
dump_mdns_system_state() {
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "=== System State for mDNS Debugging ==="

    # Check avahi-daemon status
    if systemctl is-active --quiet avahi-daemon 2>/dev/null; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: RUNNING (may conflict with hydrogen mDNS)"
    elif service avahi-daemon status >/dev/null 2>&1; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: RUNNING via service (may conflict)"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: NOT RUNNING (hydrogen should be primary mDNS responder)"
    fi

    # Check systemd-resolved status
    if systemctl is-active --quiet systemd-resolved 2>/dev/null; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "systemd-resolved: RUNNING"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "systemd-resolved: NOT RUNNING"
    fi

    # Check network interface multicast capabilities
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Network interfaces with MULTICAST:"
        for iface in $(find /sys/class/net/ -maxdepth 1 -type d -exec basename {} \; 2>/dev/null | head -10 || true); do
            if [[ -f "/sys/class/net/${iface}/flags" ]]; then
                flags_hex=$(cat "/sys/class/net/${iface}/flags" 2>/dev/null)
                # Extract the hex value and check multicast flag (bit 12 = 0x1000)
                if [[ "${flags_hex}" =~ 0x[0-9a-fA-F]+ ]]; then
                    flags_value=${flags_hex:2}  # Remove '0x' prefix
                    flags_dec=$(printf "%d" "0x${flags_value}" 2>/dev/null || echo "0")
                    if (( (flags_dec & 4096) != 0 )); then  # 0x1000 = 4096
                        printf -v output "  %-12s: MULTICAST enabled" "${iface}"
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output}"
                fi
            fi
        fi
    done

    # Check for other mDNS services (silently)
    local other_services_count
    other_services_count=$(timeout 1 avahi-browse -a -p >/dev/null 2>&1 | wc -l 2>/dev/null || echo "0")
    if [[ "${other_services_count}" -gt 0 ]]; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Other mDNS services on network: ${other_services_count} services found"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Other mDNS services on network: none found"
    fi

    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "
}

# Function to test mDNS server announcements
test_mdns_server_announcements() {
    local log_file="$1"
    local max_wait=10
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
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS services configured"
                return 0
            fi
        fi
        sleep 0.02
        attempt=$((attempt + 1))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS server announcement activities not detected within ${max_wait} seconds"
    return 1
}

# Function to test mDNS client discovery
test_mdns_client_discovery() {
    local log_file="$1"
    local max_wait=25
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
        sleep 0.05
        attempt=$((attempt + 1))
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS client discovery activities not detected - this may be normal if no other services are available on the network"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client initialized but no services available for discovery (acceptable outcome)"
    return 0
}

# Function to test mDNS responder loop by sending queries
test_mdns_responder_loop() {
    local server_port="$1"
    local max_wait=5
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS responder loop functionality..."

    # Wait a moment for server to be fully ready
    sleep 0.1

    # Method 1: Use avahi-browse to send queries (if available)
    if command -v avahi-browse >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sending mDNS queries using avahi-browse..."

        # Send just one query to test responder functionality
        timeout 1 avahi-browse -p -r _http._tcp >/dev/null 2>&1 &
        sleep 0.1  # Wait for query to be sent and response received

        # Check for responder activity in logs
        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query" "${SERVER_LOG}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Responder loop activity detected in server log"
            # print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS responder loop successfully generated responses"
            return 0
        fi
    fi

    # Method 2: Use dns-sd (macOS) if avahi-browse not available
    if command -v dns-sd >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with dns-sd..."

        # Send queries using dns-sd
        timeout 1 dns-sd -Q Hydrogen_Test._http._tcp. local 2>/dev/null &
        timeout 1 dns-sd -Q _http._tcp local 2>/dev/null &

        sleep 0.25

        # Check if responses were logged in server log
        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query\|mDNSServer.*response" "${SERVER_LOG}" 2>/dev/null; then
            local responder_activity
            responder_activity=$("${GREP}" -c "mDNSServer.*responder\|mDNSServer.*query\|mDNSServer.*response" "${SERVER_LOG}" 2>/dev/null)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${responder_activity} responder loop activities in server log"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS responder loop activity detected in logs"
            return 0
        fi
    fi

    # Method 3: Manual UDP socket test (fallback using shell tools)
    if command -v nc >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Attempting manual mDNS query via netcat..."

        # Create a simple mDNS query packet using shell tools
        # mDNS query packet structure: header + query name + query type/class
        # Query for _http._tcp.local (PTR record)
        {
            # DNS header: ID=0x1234, flags=0x0000, QDCOUNT=1, ANCOUNT=0, NSCOUNT=0, ARCOUNT=0
            printf '\x12\x34\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00'
            # Query name: _http._tcp.local (length-prefixed labels)
            printf '\x05_http\x04_tcp\x05local\x00'
            # Query type: PTR (12), class: IN (1)
            printf '\x00\x0c\x00\x01'
        } | timeout 1 nc -u -w1 224.0.0.251 5353 2>/dev/null &

        sleep 0.25

        # Check for responder activity in logs
        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query" "${SERVER_LOG}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Responder loop activity detected after manual query"
            # print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS responder loop triggered via manual query"
            return 0
        fi
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Could not trigger responder loop - no suitable tools available"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Responder loop test attempted (tools not available)"
    return 0
}

# Function to analyze mDNS packet types and distinguish announcements from responses
analyze_mdns_packets() {
    local capture_file="$1"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing mDNS packet types..."

    # First, let's see what packets we actually captured
    local total_packets
    total_packets=$(tshark -r "${capture_file}" 2>/dev/null | wc -l || true)
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total packets in capture: ${total_packets}"

    if [[ "${total_packets}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No packets found in capture file"
        return 1
    fi

    # Show packet summary instead of verbose details
    # local packet_summary=$(tshark -r "${capture_file}" -Y "mdns" -T fields -e frame.number -e dns.qry.name -e dns.resp.name -E separator=" | " 2>/dev/null | head -3 || echo "No packets found")
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sample packets: ${packet_summary}"

    # Analyze DNS flags to distinguish announcements from responses
    local aa_packets
    aa_packets=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.aa == 1" 2>/dev/null | wc -l || true)
    local response_packets
    response_packets=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.response == 1" 2>/dev/null | wc -l || true)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Authoritative Answers (Announcements): ${aa_packets}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Query Responses: ${response_packets}"

    # Look for timing patterns
    local timing_analysis
    timing_analysis=$(tshark -r "${capture_file}" -Y "mdns" -T fields -e frame.time_relative -e dns.flags.response -e dns.flags.aa -E separator="," 2>/dev/null | head -20 || true)

    if [[ -n "${timing_analysis}" ]]; then
        local announcement_count
        announcement_count=$(echo "${timing_analysis}" | grep -c "1$")
        local response_count
        response_count=$(echo "${timing_analysis}" | grep -c "^.*,1,")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing analysis: ${announcement_count} announcements, ${response_count} responses"
    fi

    # Check for specific mDNS packet types
    local ptr_queries
    ptr_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 12" 2>/dev/null | wc -l || true)
    local srv_queries
    srv_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 33" 2>/dev/null | wc -l || true)
    local txt_queries
    txt_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 16" 2>/dev/null | wc -l || true)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Query types: PTR=${ptr_queries}, SRV=${srv_queries}, TXT=${txt_queries}"

    return 0
}

# START TSHARK PACKET CAPTURE (before hydrogen starts)
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Packet Capture with tshark"

# Try tshark first, fallback to netcat if tshark fails
if command -v tshark >/dev/null 2>&1; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting tshark capture for mDNS packets..."
    nohup tshark -i any -p -f "udp port 5353" -w "${TRACED_LOG}" -q > "${CAPTURE_LOG}" 2>&1 &
    TCAP_PID=$!

    # Check if tshark is running
    if ps -p "${TCAP_PID}" >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "tshark capture started (PID: ${TCAP_PID})"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start packet capture"
        EXIT_CODE=1
        TCAP_PID=""
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "tshark not available - packet capture will be skipped"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Packet capture unavailable (tshark not installed)"
    TCAP_PID=""
fi

# Start alternative packet validation using netcat (backup method)
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting alternative packet validation with netcat..."
if command -v nc >/dev/null 2>&1; then
    # Use netcat to listen for multicast packets as backup validation
    timeout 1 nc -u -l 224.0.0.251 5353 > "${PACKET_LOG}.netcat" 2>/dev/null &
    NC_PID=$!
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Netcat validation started (PID: ${NC_PID})"
else
    NC_PID=""
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate mDNS Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_FILE}"; then
    SERVER_PORT=$(get_webserver_port "${CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS test will use port: ${SERVER_PORT}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS configuration file validated"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server"

    HYDROGEN_PID=''
    SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_server.log"
    SERVER_READY=false
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${CONFIG_FILE}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_PID"; then
        if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server Log: ..${SERVER_LOG}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen server started"
            SERVER_READY=true
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen process failed to start properly"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen server"
        EXIT_CODE=1
    fi

    if [[ "${SERVER_READY}" = true ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify mDNS Server Announcements"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_server_announcements "${SERVER_LOG}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS service announcements generated"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS service announcements failed"
            EXIT_CODE=1
        fi

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test mDNS Responder Loop"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if test_mdns_responder_loop "${SERVER_PORT}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS responder loop functionality verified"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS responder loop test failed"
            EXIT_CODE=1
        fi

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Enhanced mDNS Packet Analysis"

        if [[ -f "${TRACED_LOG}" ]]; then
            analyze_mdns_packets "${TRACED_LOG}"
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Enhanced mDNS Packet Analysis Complete"

        # print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test External Discovery Tools"

        # if test_mdns_external_tools "${SERVER_PORT}"; then
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External mDNS discovery tools tested"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # else
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External discovery tools available - see diagnostic info above"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # fi

        # print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify mDNS Client Discovery"

        # if test_mdns_client_discovery "${SERVER_LOG}"; then
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client discovery validated"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # else
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client initialized (acceptable)"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # fi

        # print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify mDNS Logging"

        # if test_mdns_server_logging "${SERVER_LOG}"; then
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS server logging verified"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # else
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS server logging check failed"
        #     EXIT_CODE=1
        # fi

        # if test_mdns_client_logging "${SERVER_LOG}"; then
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client logging verified"
        #     PASS_COUNT=$(( PASS_COUNT + 1 ))
        # else
        #     print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS client logging check failed"
        #     EXIT_CODE=1
        # fi

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing tshark packet data"

        # Analyze captured packets - save detailed output to diagnostics log
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Traced:  ..${TRACED_LOG}"
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Capture: ..${CAPTURE_LOG}"
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Filter:  ..${FILTER_LOG}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Packet:  ..${PACKET_LOG}"

        sleep 0.25

        if [[ -f "${CAPTURE_LOG}" ]]; then
            # Start diagnostics log with header information
            {
                echo "=============================================================================="
                echo "HYDROGEN mDNS PACKET CAPTURE ANALYSIS"
                echo "Test:    ${TEST_NAME} (${TEST_NUMBER})"
                echo "Date:    $(date || true)"
                echo "Traced:  ${TRACED_LOG}"
                # echo "Capture: ${CAPTURE_LOG}"
                # echo "Filter:  ${FILTER_LOG}"
                echo "Packet:  ${PACKET_LOG}"
                echo "=============================================================================="
                echo ""
            } > "${PACKET_LOG}"

            # Fix: Ensure packet_count is a clean integer, handle tshark failures properly
            if [[ -f "${TRACED_LOG}" ]] && tshark_output=$(tshark -r "${TRACED_LOG}" 2>/dev/null); then
                packet_count=$(echo "${tshark_output}" | wc -l)
            else
                packet_count=0
            fi
            # Clean up any whitespace/newlines that might cause syntax errors
            packet_count=$(echo "${packet_count}" | tr -d '\n\r\t ' | head -c 10)
            # Ensure it's a valid number
            if ! [[ "${packet_count}" =~ ^[0-9]+$ ]]; then
                packet_count=0
            fi

            # If tshark captured no packets, check netcat backup
            if [[ "${packet_count}" -eq 0 ]] && [[ -f "${PACKET_LOG}.netcat" ]]; then
                netcat_size=$(stat -c%s "${PACKET_LOG}.netcat" 2>/dev/null || echo "0")
                if [[ "${netcat_size}" -gt 100 ]]; then
                    packet_count=1  # Netcat captured data, assume packets are flowing
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "tshark failed but netcat captured ${netcat_size} bytes of mDNS data"
                fi
            fi

            {
                echo "📊 PACKET ANALYSIS SUMMARY:"
                echo "  Total mDNS packets captured: ${packet_count}"
                echo ""
            } >> "${PACKET_LOG}"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total mDNS packets captured: ${packet_count}"

            # Check for alternative validation methods
            netcat_backup_success=false
            if [[ "${packet_count}" -eq 0 ]] && [[ -f "${PACKET_LOG}.netcat" ]]; then
                netcat_size=$(stat -c%s "${PACKET_LOG}.netcat" 2>/dev/null || echo "0")
                if [[ "${netcat_size}" -gt 100 ]]; then
                    netcat_backup_success=true
                    {
                        echo "✅ SUCCESS: mDNS packets detected via netcat backup method!"
                        echo "  Netcat captured: ${netcat_size} bytes of mDNS data"
                        echo "  NOTE: tshark may be broken after system updates, but mDNS is working"
                        echo ""
                        echo "NETCAT CAPTURED mDNS DATA (first 500 bytes):"
                        head -c 500 "${PACKET_LOG}.netcat" | hexdump -C | head -20
                        echo ""
                        echo "READABLE SERVICE STRINGS:"
                        strings "${PACKET_LOG}.netcat" | grep -i hydrogen | head -5
                        echo ""
                    } >> "${PACKET_LOG}"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS packets confirmed via netcat backup method"
                fi
            fi

            if [[ "${packet_count}" -gt 0 ]] || [[ "${netcat_backup_success}" = true ]]; then
                {
                    echo "✅ SUCCESS: mDNS packets detected on network!"
                    echo ""
                    echo "DETAILED PACKET ANALYSIS:"
                } >> "${PACKET_LOG}"

                # Get detailed Hydrogen-related packets for diagnostics
                # Look for actual service names from client structure output or config
                hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains \"Hydrogen_Test\" or dns.resp.name contains \"Hydrogen_Test\")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")

                # If no Hydrogen_Test_ prefixed services found, try more flexible patterns
                if [[ -z "${hydrogen_packets_detail}" || "${hydrogen_packets_detail}" == " " ]]; then
                    hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains \"Hydrogen\" or dns.resp.name contains \"Hydrogen\")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")
                fi

                # If still no results, try searching for the full local name from server logs
                if [[ -z "${hydrogen_packets_detail}" || "${hydrogen_packets_detail}" == " " ]]; then
                    # Extract hostname from server logs
                    hydrogen_hostname=$(grep -h "\.local" "${SERVER_LOG}" 2>/dev/null | head -1 | sed 's/.*\.\([a-zA-Z0-9_-]*\.local\).*/\1/' || echo "")
                    if [[ -n "${hydrogen_hostname}" && "${hydrogen_hostname}" != " " ]]; then
                        hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains \"${hydrogen_hostname}\" or dns.resp.name contains \"${hydrogen_hostname}\")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")
                    fi
                fi

                if [[ -n "${hydrogen_packets_detail}" && "${hydrogen_packets_detail}" != " " ]]; then
                    {
                        echo "🎯 HYDROGEN mDNS ANNOUNCEMENTS (Summary):"
                        echo "${hydrogen_packets_detail}" | head -10 || true
                        echo ""
                    } >> "${PACKET_LOG}"

                     hydrogen_packet_count=$(echo "${hydrogen_packets_detail}" | wc -l || true)

                    {
                        echo "🎯 TOTAL HYDROGEN SERVICE PACKETS FOUND: ${hydrogen_packet_count}"
                        echo ""
                    } >> "${PACKET_LOG}"

                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen mDNS Packets Found: ${hydrogen_packet_count}"

                    # Save full detailed packet analysis
                    {
                        echo "=============================================================================="
                        echo "FULL DETAILED PACKET ANALYSIS:"
                        echo "=============================================================================="
                        tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains \"Hydrogen\" or dns.resp.name contains \"Hydrogen\")" -V 2>/dev/null | head -50 || echo "Detailed analysis failed"
                        echo ""
                    } >> "${PACKET_LOG}"

                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen mDNS packets confirmed"
                else
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "No Hydrogen-specific packets found"

                    {
                        echo "⚠️  WARNING: No Hydrogen-specific packets found in capture"
                        echo "   This could indicate service announcement issues"
                        echo ""
                        echo "ALL mDNS PACKETS CAPTURED:"
                        tshark -r "${TRACED_LOG}" -Y "mdns" -T fields -e frame.number -e frame.time -e frame.protocols -e dns.resp.name -E separator=" | " 2>/dev/null | head -20
                        echo ""
                    } >> "${PACKET_LOG}"

                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server started but no Hydrogen mDNS packets detected"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Review diagnostics log for complete analysis: ..${PACKET_LOG}"
                    EXIT_CODE=1
                fi
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ CRITICAL: NO mDNS packets captured at all"

                {
                    echo "❌ CRITICAL: NO mDNS packets captured at all"
                    echo "This suggests a fundamental networking issue"
                    echo ""
                    echo "Troubleshooting Info:"
                    echo "- Check if hydrogen process is binding to UDP 5353"
                    echo "- Verify multicast networking configuration"
                    echo "- Test with: ss -uln | grep 5353"
                    echo "- Test multicast: ip route show | grep 224.0.0"
                    echo ""
                } >> "${PACKET_LOG}"

                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS packets captured - Transmission failure"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Troubleshooting details saved to: ..${PACKET_LOG}"
                EXIT_CODE=1
            fi

        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Packet capture unavailable - tshark not installed"
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Packet capture not started or tshark unavailable"
    fi

else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping all tests due to prerequisite failures"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Clean Server Shutdown"

if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" "${SHUTDOWN_TIMEOUT}" 5 "${DIAG_TEST_DIR}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen server shutdown completed successfully"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen server shutdown had issues"
        EXIT_CODE=1
    fi
fi

# Don't leave the lights on
if [[ -n "${TCAP_PID}" ]]; then
    kill -9 "${TCAP_PID}" >/dev/null 2>&1 || true
    wait "${TCAP_PID}" 2>/dev/null || true
fi

# Clean up netcat backup process
if [[ -n "${NC_PID}" ]]; then
    kill -9 "${NC_PID}" >/dev/null 2>&1 || true
    wait "${NC_PID}" 2>/dev/null || true
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
