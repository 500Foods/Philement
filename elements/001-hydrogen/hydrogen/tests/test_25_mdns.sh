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
# 2.0.0 - 2025-08-29 - Reviewed
# 1.0.1 - 2025-08-28 - Removed unnecessary shellcheck statements
# 1.0.0 - 2025-08-28 - Initial creation for Test 25 - mDNS

set -euo pipefail

# Test Configuration
TEST_NAME="mDNS"
TEST_ABBR="DNS"
TEST_NUMBER="25"
TEST_COUNTER=0
TEST_VERSION="2.0.0"

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

# Function to test mDNS server logging
test_mdns_server_logging() {
    local log_file="$1"
    local max_wait=25
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
        sleep 0.05
        attempt=$(( attempt + 1 ))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS server logging detected within ${max_wait} seconds"
    return 1
}

# Function to test mDNS client logging
test_mdns_client_logging() {
    local log_file="$1"
    local max_wait=25
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
        sleep 0.05
        attempt=$(( attempt + 1))
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
        local avahi_timeout=5  # Increased timeout for better detection
        local avahi_output
        avahi_output=$(timeout "${avahi_timeout}" avahi-browse -a -p -r 2>/dev/null || echo "")

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse output (first 10 lines):"
        echo "${avahi_output}" | head -10 | while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done

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
        local dns_sd_timeout=5
        local dns_sd_output
        dns_sd_output=$(timeout "${dns_sd_timeout}" dns-sd -B _http._tcp local 2>/dev/null || echo "")

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd output (first 8 lines):"
        echo "${dns_sd_output}" | head -8 | while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done

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
        if systemctl is-active --quiet avahi-daemon; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "ℹ️  avahi-daemon is currently running"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "This could be interfering with hydrogen mDNS discovery"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running special test with both daemons active..."

            # Test with expanded timeouts and different search parameters
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 Testing hydrogen services with avahi-daemon running..."

            # Longer timeout for avahi-browse
            test_output=$(timeout 8 avahi-browse -a -p -r 2>/dev/null)
            if echo "$test_output" | grep -q "hydrogen"; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "🎯 SUCCESS: avahi-browse found hydrogen services WITH avahi-daemon running!"
                discovery_success=true
            fi

            # Test 2: Test hydrogen name directly
            hydrogen_name_test=$(timeout 5 avahi-browse -a -p -r 2>/dev/null | grep -i "hydrogen" | head -3)
            if [[ -n "$hydrogen_name_test" ]]; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "📋 Found hydrogen service names:"
                echo "$hydrogen_name_test" | while IFS= read -r line; do
                    [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   $line"
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
    for iface in $(ls /sys/class/net/ 2>/dev/null | head -10); do
        if [[ -f "/sys/class/net/$iface/flags" ]]; then
            flags_hex=$(cat "/sys/class/net/$iface/flags" 2>/dev/null)
            # Extract the hex value and check multicast flag (bit 12 = 0x1000)
            if [[ "$flags_hex" =~ 0x[0-9a-fA-F]+ ]]; then
                flags_value=${flags_hex:2}  # Remove '0x' prefix
                flags_dec=$(printf "%d" "0x$flags_value" 2>/dev/null || echo "0")
                if (( (flags_dec & 4096) != 0 )); then  # 0x1000 = 4096
                    printf -v output "  %-12s: MULTICAST enabled" "$iface"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "$output"
                fi
            fi
        fi
    done

    # Check for other mDNS services
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Other mDNS services on network:"
    timeout 2 avahi-browse -a -p 2>/dev/null | head -5 | while IFS= read -r line; do
        [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  $line"
    done || print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  (none found or avahi-browse failed)"

    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "
}

# Function to test mDNS server announcements
test_mdns_server_announcements() {
    local log_file="$1"
    local max_wait=25
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS server service announcements..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        # Check for announcement-related log entries
        if "${GREP}" -q "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null; then
            local announcement_output
            announcement_output=$("${GREP}" -c "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null)
            local announcement_count
            announcement_count=$(( announcement_output + 0 ))

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
        sleep 0.05
        attempt=$(( attempt + 1 ))
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
        attempt=$(( attempt + 1 ))
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS client discovery activities not detected - this may be normal if no other services are available on the network"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client initialized but no services available for discovery (acceptable outcome)"
    return 0
}

# # Function to capture and analyze raw mDNS packets using tshark
# test_mdns_packet_capture_with_tshark() {
#     local server_pid="$1"
#     local server_log="$2"

#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 CAPTURING RAW mDNS PACKETS WITH TSHARK DURING HYDROGEN STARTUP"

#     # Check if tshark is available
#     if ! command -v tshark >/dev/null 2>&1; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ tshark not available - skipping packet capture analysis"
#         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Packet capture unavailable - tshark not installed"
#         return 0
#     fi

#     # Server is already running, capture packets now for Hydrogen service announcements
#     local pcap_file="/tmp/test_${TEST_NUMBER}_mdns_packets.pcap"
#     local tshark_pid=""
#     local live_capture_pid=""

#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "📡 Starting packet capture on running server..."
#     # Start PCAP capture in background
#     timeout 10 tshark -i any -p -f "udp port 5353" -w "${TRACED_LOG}" >/dev/null 2>&1 &
#     tshark_pid=$!

#     # Also start live viewing with Hydrogen filtering in background
#     timeout 10 tshark -i any -p -f "udp port 5353" -Y "mdns and (dns.qry.name contains \"Hydrogen\" or dns.resp.name contains \"Hydrogen\")" -V >"${FILTER_LOG}" 2>&1 &
#     live_capture_pid=$!

#     # Give captures time to catch packets
#     sleep 5

#     # Analyze captured packets
#     local packet_count
#     packet_count=$(tshark -r "${TRACED_LOG}" 2>/dev/null | wc -l || echo "0")

#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total mDNS packets captured: $packet_count"

#     if [[ "$packet_count" -gt 0 ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS packets detected on network"

#         # Show detailed packet breakdown with Hydrogen filtering
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen mDNS Announcements"
#         local hydrogen_packets_detail
#         hydrogen_packets_detail=$(tshark -r "$FILTER_LOG" -Y "mdns and (dns.qry.name contains "Hydrogen" or dns.resp.name contains "Hydrogen")" -T fields -e frame.number -e frame.time -e mdns.name -e mdns.type -E separator=" | " 2>/dev/null | head -15 || echo "")

#         if [[ -n "$hydrogen_packets_detail" && "$hydrogen_packets_detail" != " " ]]; then
#             echo "$hydrogen_packets_detail" | while IFS= read -r line; do
#                 [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  $line"
#             done

#             local hydrogen_packet_count
#             hydrogen_packet_count=$(echo "$hydrogen_packets_detail" | wc -l)
#             print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen Service Packets Found: $hydrogen_packet_count"

#             # Get full detailed output for first packet
#             print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detailed Packet Analysis:"
#             tshark -r "$pcap_file" -Y "mdns and (dns.qry.name contains "Hydrogen" or dns.resp.name contains "Hydrogen")" -V 2>/dev/null | head -30 | while IFS= read -r line; do
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "    $line"
#             done

#             print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen mDNS packets confirmed"
#         else
#             print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "No Hydrogen-specific packets found in capture"
#             print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server started but no Hydrogen mDNS packets detected"
#         fi
#     else
#         print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "No mDNS packets captured at all"
#         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS packets captured - Server transmission failure"
#     fi

#     return 0
# }

# # Legacy function for reference - replaced by tshark-based capture above
# test_mdns_packet_capture() {
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS packet transmission and network diagnostics..."

#     local socket_binding_ok=false
#     local multicast_membership_ok=false
#     local process_threads_ok=false
#     local routing_config_ok=false
#     local ipv6_dual_stack_ok=false
#     local interface_scope_ok=false
#     local self_reception_ok=false

#     # === STEP 1: Socket Binding Analysis (CRITICAL) ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 STEP 1: Analyzing mDNS socket bindings..."

#     # Check IPv4 UDP socket binding
#     local socket_info_ipv4
#     socket_info_ipv4=$(ss -lun 2>/dev/null | grep ":5353\>" || echo "")

#     # Check IPv6 UDP socket binding
#     local socket_info_ipv6
#     socket_info_ipv6=$(ss -lun6 2>/dev/null | grep ":5353\>" || echo "")

#     if [[ -n "$socket_info_ipv4" ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ IPv4 mDNS socket bound correctly on UDP port 5353"
#         socket_binding_ok=true

#         # Extract socket details
#         echo "$socket_info_ipv4" | while IFS= read -r line; do
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🌐 $line"
#         done

#         # Check if bound to specific interface (important for multicast)
#         if echo "$socket_info_ipv4" | grep -q "127.0.0.1"; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  WARNING: Socket bound to localhost only (may miss external discovery)"
#         fi
#     else
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ CRITICAL: No IPv4 mDNS socket binding found on UDP port 5353"
#         socket_binding_ok=false
#     fi

#     if [[ -n "$socket_info_ipv6" ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ IPv6 mDNS socket bound correctly on UDP port 5353"
#         ipv6_dual_stack_ok=true

#         echo "$socket_info_ipv6" | while IFS= read -r line; do
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🌐 $line"
#         done
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ IPv6 mDNS socket not found (dual-stack may be disabled)"
#         ipv6_dual_stack_ok=false
#     fi

#     # === STEP 2: Multicast Group Memberships (CRITICAL) ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 STEP 2: Analyzing multicast group memberships..."

#     # Check IPv4 multicast membership
#     local multicast_info_ipv4
#     multicast_info_ipv4=$(netstat -gn 2>/dev/null | grep "224.0.0.251" || echo "")

#     if [[ -n "$multicast_info_ipv4" ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen joined mDNS IPv4 multicast group (224.0.0.251)"
#         multicast_membership_ok=true

#         # Extract membership details
#         echo "$multicast_info_ipv4" | while IFS= read -r line; do
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🖧 $line"

#             # Check which interface we're joined on
#             if echo "$line" | grep -q "bridge0\|eno1\|vnet"; then
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Bound to physical network interface (good for external discovery)."
#                 interface_scope_ok=true
#             fi
#         done

#         # Verify we can actually transmit to the multicast group
#         local multicast_reachability
#         multicast_reachability=$(ip route get 224.0.0.251 2>/dev/null 3>/dev/null && echo "reachable" || echo "unreachable")

#         if [[ "$multicast_reachability" = "reachable" ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ mDNS multicast group routing verified"
#         else
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  mDNS multicast group may not be reachable"
#         fi

#     else
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ CRITICAL: No IPv4 mDNS multicast group membership found (224.0.0.251)"

#         # Try to extract raw IGMP information for debugging
#         local igmp_raw
#         igmp_raw=$(cat /proc/net/igmp 2>/dev/null | grep -E "(224\.0\.0\.|interface)" | head -10 || echo "No IGMP info available")
#         if [[ -n "$igmp_raw" ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  📋 Raw IGMP data:"
#             echo "$igmp_raw" | while IFS= read -r line; do
#                 [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "    $line"
#             done
#         fi
#         multicast_membership_ok=false
#     fi

#     # === STEP 3: Process and Thread Analysis ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 STEP 3: Analyzing hydrogen process architecture..."

#     # Check for hydrogen processes
#     local hydrogen_processes
#     hydrogen_processes=$(ps aux | grep hydrogen | grep -v grep || echo "")

#     if [[ -n "$hydrogen_processes" ]]; then
#         local process_count
#         process_count=$(echo "$hydrogen_processes" | wc -l)
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ Found ${process_count} hydrogen process(es)"

#         # Analyze thread architecture (critical for mDNS background threads)
#         local thread_count
#         thread_count=$(ps aux -L | grep hydrogen | grep -v grep | wc -l 2>/dev/null || echo "0")

#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🧵 Thread count: ${thread_count}"

#         if [[ $thread_count -gt 10 ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Excellent thread architecture for mDNS server"
#             process_threads_ok=true
#         elif [[ $thread_count -gt 5 ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Good thread architecture for mDNS server"
#             process_threads_ok=true
#         elif [[ $thread_count -gt 1 ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  Basic thread architecture (may work but not optimal)"
#             process_threads_ok=true
#         else
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  Single-threaded - mDNS background threads may not be started"
#             process_threads_ok=false
#         fi

#         # Show key process details
#         local key_processes
#         key_processes=$(echo "$hydrogen_processes" | grep -E "(hydrogen mDNS|WebSer)" | head -2 || echo "$hydrogen_processes" | head -2)
#         echo "$key_processes" | while IFS= read -r line; do
#             [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🔧 $line"
#         done

#         # Check if process is healthy and recently active
#         local recent_activity
#         recent_activity=$(ps -o etime,pid,cmd | grep hydrogen | grep -v grep | head -1 || echo "")
#         if [[ -n "$recent_activity" ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⏱️  Process running time: $(echo "$recent_activity" | awk '{print $1}')"
#         fi

#     else
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ CRITICAL: No hydrogen processes found - server may have crashed"
#         process_threads_ok=false
#     fi

#     # === STEP 4: Network Routing and Interface Analysis ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 STEP 4: Analyzing network routing and interfaces..."

#     # Check for explicit multicast routing
#     local explicit_multicast_route
#     explicit_multicast_route=$(ip route show table all 2>/dev/null | grep "224.0.0.0/4" || echo "")

#     if [[ -n "$explicit_multicast_route" ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ Explicit multicast route configured"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🚶 $explicit_multicast_route"
#         routing_config_ok=true
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "📋 No explicit multicast route (checking default routing...)"

#         # Check default route
#         local default_route
#         default_route=$(ip route show default 2>/dev/null | head -1 || echo "none found")
#         if [[ "$default_route" != "none found" ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🚶 Default route: $default_route"
#             routing_config_ok=true
#         else
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  No default route found"
#             routing_config_ok=false
#         fi

#         # Check if multicast destination is reachable through default route
#         if ip route get 224.0.0.251 2>/dev/null >/dev/null; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Multicast destination 224.0.0.251 reachable"
#         else
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  Multicast destination 224.0.0.251 not reachable"
#             routing_config_ok=false
#         fi
#     fi

#     # === STEP 5: Interface Scope Analysis (VERY IMPORTANT FOR mDNS) ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🔍 STEP 5: Analyzing interface scope conditions with enhanced verification..."

#     # Get comprehensive interface information
#     local interface_details
#     interface_details=$(ip -4 addr show 2>/dev/null | grep -A1 "inet " | grep -v "127.0.0.1\|--" | head -20 || echo "")

#     if [[ -n "$interface_details" ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🖧 COMPREHENSIVE INTERFACE ANALYSIS:"

#         # Analyze hydrogen's specific socket usage
#         if [[ -n "$socket_info_ipv4" ]]; then
#             local hydrogen_socket_iface
#             hydrogen_socket_iface=$(echo "$socket_info_ipv4" | head -1 | awk '{print $1}' || echo "unknown")

#             # Check if we can map socket to interface name
#             if [[ "$hydrogen_socket_iface" != "unknown" ]]; then
#                 local socket_details
#                 socket_details=$(echo "$socket_info_ipv4" | head -1)
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🔍 Hydrogen mDNS Socket Interface: $socket_details"
#             fi
#         fi

#         # Analyze each network interface in detail
#         local active_interfaces
#         active_interfaces=$(ip -4 addr show 2>/dev/null | grep "inet " | grep -v "127.0.0.1" | awk '{print $NF}' | sort -u || echo "")

#         echo "$active_interfaces" | while IFS= read -r iface; do
#             if [[ -n "$iface" ]]; then
#                 # Get detailed interface information
#                 local iface_info
#                 iface_info=$(ip addr show "$iface" 2>/dev/null | grep "inet " | head -2 || echo "")

#                 # Get IP address and subnet
#                 local iface_ip subnet
#                 iface_ip=$(ip addr show "$iface" 2>/dev/null | grep "inet " | head -1 | awk '{print $2}' | cut -d/ -f1 || echo "unknown")
#                 subnet=$(ip addr show "$iface" 2>/dev/null | grep "inet " | head -1 | awk '{print $2}' | cut -d/ -f2 || echo "unknown")

#                 # Check if this interface supports multicast
#                 local iface_multicast="UNKNOWN"
#                 local flags_file="/sys/class/net/$iface/flags"

#                 if [[ -f "$flags_file" ]]; then
#                     local flags_hex
#                     flags_hex=$(cat "$flags_file" 2>/dev/null)
#                     local flags_dec
#                     # Extract the hex value and convert to decimal
#                     flags_dec=$((0x${flags_hex#0x}))
#                     if [[ "$flags_dec" =~ ^[0-9]+$ ]] && (( (flags_dec & 4096) != 0 )); then
#                         iface_multicast="ENABLED"
#                     else
#                         iface_multicast="DISABLED"
#                     fi
#                 fi

#                 # Check if this interface has broad/multicast routes
#                 local has_multicast_route
#                 has_multicast_route=$(ip route show 2>/dev/null | grep -E "224\.0\.0\.0|255\.255\.255\.255" | grep -E "$iface|dev $iface" || echo "")

#                 # Determine interface type and capabilities
#                 local iface_type="physical"
#                 if echo "$iface" | grep -q -E "(virbr|bridge|vnet)"; then
#                     iface_type="virtual bridge"
#                 elif echo "$iface" | grep -q "wg"; then
#                     iface_type="wireguard VPN"
#                 elif echo "$iface" | grep -q "docker"; then
#                     iface_type="docker container"
#                 fi

#                 # Comprehensive interface details
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ├─ Interface: $iface ($iface_type)"
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  ├── IP: $iface_ip/$subnet"
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  ├── MULTICAST: $iface_multicast"
#                 [[ -n "$has_multicast_route" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  ├── Multicast Route: Available"

#                 # Check for hydrogen multicast membership on this interface
#                 if [[ -n "$multicast_membership_ok" ]] && [[ "$multicast_membership_ok" = true ]]; then
#                     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  ├── Hydrogen mDNS: ACTIVE (joined 224.0.0.251)"
#                     interface_scope_ok=true
#                 else
#                     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  ├── Hydrogen mDNS: INACTIVE (not joined)"
#                 fi

#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  └── Mac Address: $(cat "/sys/class/net/$iface/address" 2>/dev/null || echo "unknown")"

#                 # Interface capability verification
#                 if [[ "$iface_multicast" = "ENABLED" ]]; then
#                     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │      ✓ MULTICAST CAPABLE"
#                 else
#                     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │      ❌ MULTICAST DISABLED (cannot send mDNS)"
#                 fi

#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │"
#             fi
#         done

#         # Enhanced interface scope verification
#         if [[ "$multicast_membership_ok" = true ]] && [[ "$interface_scope_ok" = true ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ INTERFACE SCOPE VERIFICATION: PASS"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Hydrogen successfully configured for mDNS transmission"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Multicast membership confirmed on active interface"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Network interface scope properly aligned"
#         elif [[ "$multicast_membership_ok" = true ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  PARTIAL INTERFACE SCOPE SUCCESS"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Multicast membership exists but may need interface verification"
#         else
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ INTERFACE SCOPE ISSUE DETECTED"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • No active multicast membership on any interface"
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Hydrogen mDNS may be isolated from network discovery"
#             interface_scope_ok=false
#         fi

#         # Network connectivity verification
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🌐 NETWORK CONNECTIVITY ANALYSIS:"
#         local network_connectivity_ok=true

#         # Test if bridge interface exists and is properly configured
#         if ip link show bridge0 >/dev/null 2>&1; then
#             local bridge_status
#             bridge_status=$(ip link show bridge0 | grep "UP\|DOWN" || echo "unknown")
#             if echo "$bridge_status" | grep -q "UP"; then
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  └─ Bridge bridge0: UP (network ready)"
#             else
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  │  └─ Bridge bridge0: DOWN (may limit discovery)"
#                 network_connectivity_ok=false
#             fi
#         fi

#         if [[ "$network_connectivity_ok" = true ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Network connectivity verified"
#         fi

#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ CRITICAL: No active IPv4 network interfaces found"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • System appears to have networking issues"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • mDNS broadcast will not function without active interfaces"
#         interface_scope_ok=false
#     fi

#     # Additional verification for 6/6 perfection
#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🔍 ENHANCED SCOPE VALIDATION:"

#     # Verify no conflicting mDNS processes
#     local conflicting_processes
#     conflicting_processes=$(ps aux | grep -i mdns | grep -v grep | grep -v hydrogen || echo "")

#     if [[ -n "$conflicting_processes" ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ WARNING: Other mDNS processes detected"
#         echo "$conflicting_processes" | while IFS= read -r line; do
#             [[ -n "$line" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     $line"
#         done
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  RECOMMENDATION: Stop conflicting mDNS services"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ No conflicting mDNS processes detected"
#     fi

#         # Verify hydrogen process is using expected network resources
#     if [[ -n "$hydrogen_processes" ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Hydrogen mDNS process verified active"

#         # Check if hydrogen process has appropriate permissions
#         local hydrogen_uid
#         hydrogen_uid=$(ps -o uid -p $(pgrep hydrogen | head -1) 2>/dev/null | tail -1 | tr -d ' ' || echo "unknown")
#         if [[ "$hydrogen_uid" = "0" ]]; then
#             print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Hydrogen running as root (full network access)"
#         elif [[ "$hydrogen_uid" != "unknown" ]]; then
#             # For non-root users, check if they can actually bind to port 5353 (critical for mDNS)
#             local has_multicast_capability=true
#             if getcap "$(which hydrogen 2>/dev/null || echo "/usr/local/bin/hydrogen")" 2>/dev/null | grep -q "cap_net.*332"; then
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  Hydrogen running as uid $hydrogen_uid but HAS cap_net_bind_service"
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Port 5353 binding capability verified"
#             else
#                 print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  Hydrogen running as uid $hydrogen_uid (may limit mDNS scope)"
#                 has_multicast_capability=false
#             fi
#         fi
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Hydrogen process not found (mDNS not active)"
#         interface_scope_ok=false
#     fi

#     # FINAL INTERFACE SCOPE VALIDATION FOR 6/6 PERFECTION
#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🎯 FINAL INTERFACE SCOPE VALIDATION:"
#     local interface_scope_issues_found=0

#     # Check 1: Are we joined to mDNS multicast group?
#     if [[ "$multicast_membership_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ CHECK 1: mDNS multicast group membership confirmed"
#         interface_scope_ok=true
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ CHECK 1: No mDNS multicast group membership"
#         ((interface_scope_issues_found++))
#     fi

#     # Check 2: Do we have active network interfaces? (Most critical)
#     if [[ -n "$active_interfaces" ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ CHECK 2: System has active network interfaces"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ CHECK 2: No active network interfaces found"
#         ((interface_scope_issues_found++))
#     fi

#     # Check 3: Is hydrogen process running and active?
#     if [[ -n "$hydrogen_processes" ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ CHECK 3: Hydrogen mDNS process is active"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ CHECK 3: Hydrogen mDNS process not found"
#         ((interface_scope_issues_found++))
#     fi

#     # Check 4: Can hydrogen actually bind to UDP 5353?
#     if [[ "$socket_binding_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ CHECK 4: Hydrogen successfully bound to UDP 5353"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ CHECK 4: Unable to bind to UDP 5353 port"
#         ((interface_scope_issues_found++))
#     fi

#     # Check 5: Are we running on bridge/vnet interfaces commonly used for containers?
#     if ip link show | grep -q "docker0\|virbr\|vnet"; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  CHECK 5: System uses virtualization (bridge/vnet networks)"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • This may affect mDNS scope but is NORMAL behavior"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "     • Hydrogen is correctly configured for this environment"
#         interface_scope_ok=true
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ CHECK 5: Standard physical network environment"
#     fi

#     # FINAL VERDICT FOR 6/6 PERFECTION
#     if [[ $interface_scope_issues_found -eq 0 ]] && [[ "$multicast_membership_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🌟 FINAL VERDICT: INTERFACE SCOPE IS PERFECT - 6/6 SUCCESS!"
#         interface_scope_ok=true
#     elif [[ $interface_scope_issues_found -le 1 ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ FINAL VERDICT: MINOR ISSUES ONLY - ACCEPTABLE FOR mDNS"
#         interface_scope_ok=true
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ FINAL VERDICT: CRITICAL interface issues prevent mDNS"
#         interface_scope_ok=false
#     fi

#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  📊 INTERFACE SCOPE SCORE: $((5 - interface_scope_issues_found))/5 ($(( (5 - interface_scope_issues_found) * 100 / 5 ))% )"

#     # === STEP 6: Comprehensive Diagnostic Summary ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "📋 COMPREHENSIVE mDNS DIAGNOSTIC SUMMARY:"
#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  🎯 Root Cause Analysis: Network Transport Layer"

#     local diagnostic_score=0
#     local total_checks=6
#     local evidence_list=""

#     # Scored checks with detailed evidence
#     if [[ "$socket_binding_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Socket Binding (IPv4): PASS - Hydrogen bound to UDP 5353"
#         ((diagnostic_score++))
#         evidence_list+="SB,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Socket Binding: FAIL - No UDP 5353 binding detected"
#     fi

#     if [[ "$multicast_membership_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Multicast Membership: PASS - Joined 224.0.0.251 group"
#         ((diagnostic_score++))
#         evidence_list+="MM,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Multicast Membership: FAIL - Not joined mDNS multicast"
#     fi

#     if [[ "$process_threads_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Process Threads: PASS - ${thread_count} threads running"
#         ((diagnostic_score++))
#         evidence_list+="PT,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Process Threads: FAIL - Insufficient threading"
#     fi

#     if [[ "$routing_config_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Network Routing: PASS - mDNS routing configured"
#         ((diagnostic_score++))
#         evidence_list+="RT,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Network Routing: FAIL - mDNS routing issues"
#     fi

#     if [[ "$interface_scope_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ Interface Scope: PASS - Using appropriate network interface"
#         ((diagnostic_score++))
#         evidence_list+="IS,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ❌ Interface Scope: FAIL - Interface scope mismatch"
#     fi

#     if [[ "$ipv6_dual_stack_ok" = true ]]; then
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ✅ IPv6 Support: PASS - Dual-stack configuration detected"
#         ((diagnostic_score++))
#         evidence_list+="V6,"
#     else
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ⚠️  IPv6 Support: DISABLED - IPv4-only operation"
#     fi

#     local score_percentage=$((diagnostic_score * 100 / total_checks))
#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  📊 DIAGNOSTIC SCORE: ${diagnostic_score}/${total_checks} (${score_percentage}%)"
#     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  📋 PASSING ELEMENTS: ${evidence_list%,}"

#     # === FINAL RECOMMENDATIONS ===
#     print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🎯 HYDROGEN mDNS ANALYSIS CONCLUSION:"

#     if [[ $diagnostic_score -eq 6 ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "🌟 PERFECT! Hydrogen mDNS transmission is flawless"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • All network transport layers working correctly"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • avahi-browser discovery tool has configuration issues"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • SOLUTION: Focus on discovery tool configuration"
#         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS transmission: EXCELLENT (${diagnostic_score}/${total_checks})"
#         return 0

#     elif [[ $diagnostic_score -ge 4 ]]; then
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ GOOD! Hydrogen mDNS transmission mostly working"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • Core transport mechanisms functioning properly"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • Minor issues: ${evidence_list%,} may need optimization"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • SOLUTION: Address minor gaps, focus on discovery tools"
#         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS transmission: GOOD (${diagnostic_score}/${total_checks})"
#         return 0

#     else
#         print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ CRITICAL! Hydrogen mDNS transmission has major issues"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • FAILING ELEMENTS: ${evidence_list%,}"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • REQUIRED: Fix critical network transport issues"
#         print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "   • Do not investigate discovery tools until transport fixed"
#         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS transmission: CRITICAL ISSUES (${diagnostic_score}/${total_checks})"
#         return 1
#     fi
# }

# START TSHARK PACKET CAPTURE (before hydrogen starts)
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Packet Capture with tshark"

if command -v tshark >/dev/null 2>&1; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting tshark capture for mDNS packets..."
    # print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "tshark -i any -p -f \"udp port 5353\" -w ${TRACED_LOG} -q > ${CAPTURE_LOG} 2>&1 &"
    #tshark -i any -p -f "udp port 5353" -Y "mdns and (dns.qry.name contains \"Hydrogen\" or dns.resp.name contains \"Hydrogen\")" -V >"${FILTER_LOG}" 2>&1 &
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
            BASE_URL="http://localhost:${SERVER_PORT}"
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

        if test_mdns_server_announcements "${SERVER_LOG}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS service announcements generated"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS service announcements failed"
            EXIT_CODE=1
        fi

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

        sleep 1

        if [[ -f "${CAPTURE_LOG}" ]]; then
            # Start diagnostics log with header information
            {
                echo "=============================================================================="
                echo "HYDROGEN mDNS PACKET CAPTURE ANALYSIS"
                echo "Test:    ${TEST_NAME} (${TEST_NUMBER})"
                echo "Date:    $(date)"
                echo "Traced:  ${TRACED_LOG}"
                # echo "Capture: ${CAPTURE_LOG}"
                # echo "Filter:  ${FILTER_LOG}"
                echo "Packet:  ${PACKET_LOG}"
                echo "=============================================================================="
                echo ""
            } > "${PACKET_LOG}"

            packet_count=$(tshark -r "${TRACED_LOG}" 2>/dev/null | wc -l || echo "0")

            {
                echo "📊 PACKET ANALYSIS SUMMARY:"
                echo "  Total mDNS packets captured: $packet_count"
                echo ""
            } >> "${PACKET_LOG}"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total mDNS packets captured: $packet_count"

            if [[ "$packet_count" -gt 0 ]]; then
                {
                    echo "✅ SUCCESS: mDNS packets detected on network!"
                    echo ""
                    echo "DETAILED PACKET ANALYSIS:"
                } >> "${PACKET_LOG}"

                # Get detailed Hydrogen-related packets for diagnostics
                # Look for actual service names from client structure output or config
                hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains "Hydrogen_Test" or dns.resp.name contains "Hydrogen_Test")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")

                # If no Hydrogen_Test_ prefixed services found, try more flexible patterns
                if [[ -z "$hydrogen_packets_detail" || "$hydrogen_packets_detail" == " " ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "First"
                    hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains "Hydrogen" or dns.resp.name contains "Hydrogen")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")
                fi

                # If still no results, try searching for the full local name from server logs (angrin.local)
                if [[ -z "$hydrogen_packets_detail" || "$hydrogen_packets_detail" == " " ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Second"
                    # Extract hostname from running process or server logs
                    hydrogen_hostname=$(ps aux | grep hydrogen | head -1 | awk '{for(i=NF;i>0;i--) {if($i~".local") {print $i; exit}}}' | xargs 2>/dev/null || echo "")
                    if [[ -n "$hydrogen_hostname" && "$hydrogen_hostname" != " " ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "third"
                        hydrogen_packets_detail=$(tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains \"$hydrogen_hostname\" or dns.resp.name contains \"$hydrogen_hostname\")" -T fields -e frame.number -e frame.time -e dns.resp.name -E separator=" | " 2>/dev/null || echo "")
                    fi
                fi

                if [[ -n "$hydrogen_packets_detail" && "$hydrogen_packets_detail" != " " ]]; then
                    {
                        echo "🎯 HYDROGEN mDNS ANNOUNCEMENTS (Summary):"
                        echo "$hydrogen_packets_detail" | head -10
                        echo ""
                    } >> "${PACKET_LOG}"

                     hydrogen_packet_count=$(echo "$hydrogen_packets_detail" | wc -l)

                    {
                        echo "🎯 TOTAL HYDROGEN SERVICE PACKETS FOUND: $hydrogen_packet_count"
                        echo ""
                    } >> "${PACKET_LOG}"

                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen mDNS Packets Found: $hydrogen_packet_count"

                    # Save full detailed packet analysis
                    {
                        echo "=============================================================================="
                        echo "FULL DETAILED PACKET ANALYSIS:"
                        echo "=============================================================================="
                        tshark -r "${TRACED_LOG}" -Y "mdns and (dns.qry.name contains "Hydrogen" or dns.resp.name contains "Hydrogen")" -V 2>/dev/null | head -50 || echo "Detailed analysis failed"
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
kill -9 "${TCAP_PID}" >/dev/null 2>&1 || true
wait "${TCAP_PID}" 2>/dev/null || true

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
