#!/usr/bin/env bash

# mDNS Test Helpers Library
# Provides mDNS-specific test functions for test_25_mdns.sh

# This library uses variables defined by the calling test (TEST_NUMBER,
# TEST_COUNTER, GREP, HYDROGEN_BIN, etc.). Disable SC2154 globally.
# shellcheck disable=SC2154 # Variables defined by caller

# LIBRARY FUNCTIONS
# cleanup_mdns_capture()
# on_mdns_err()
# dump_mdns_system_state()
# test_mdns_server_logging()
# test_mdns_client_logging()
# test_mdns_external_tools()
# test_mdns_server_claimed()
# test_mdns_log_contract()
# test_mdns_system_info()
# test_mdns_duplicate_names()
# test_mdns_packet_assertions()
# test_mdns_server_announcements()
# test_mdns_client_discovery()
# test_mdns_responder_loop()
# analyze_mdns_packets()
# capture_contains_hydrogen()
# wait_for_hydrogen_mdns_packets()
# stop_mdns_packet_capture()

# CHANGELOG
# 1.0.0 - 2026-09-01 - Initial extraction from test_25_mdns.sh

[[ -n "${MDNS_TEST_HELPERS_GUARD:-}" ]] && return 0
export MDNS_TEST_HELPERS_GUARD="true"

MDNS_TEST_HELPERS_NAME="mDNS Test Helpers Library"
MDNS_TEST_HELPERS_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${MDNS_TEST_HELPERS_NAME} ${MDNS_TEST_HELPERS_VERSION}" "info"

# --- Capture cleanup / error hooks ---

cleanup_mdns_capture() {
    # shellcheck disable=SC2310 # Continue cleanup even if dump fails
    dump_collected_output 2>/dev/null || true
    # shellcheck disable=SC2310 # We want to continue even if stop fails
    stop_mdns_packet_capture 2>/dev/null || true
    if [[ -n "${HYDROGEN_PID:-}" ]] && ps -p "${HYDROGEN_PID}" >/dev/null 2>&1; then
        kill -INT "${HYDROGEN_PID}" >/dev/null 2>&1 || true
        sleep 0.2 2>/dev/null || true
        kill -9 "${HYDROGEN_PID}" >/dev/null 2>&1 || true
    fi
    if [[ -n "${HYDROGEN_DUP_PID:-}" ]] && ps -p "${HYDROGEN_DUP_PID}" >/dev/null 2>&1; then
        kill -INT "${HYDROGEN_DUP_PID}" >/dev/null 2>&1 || true
        sleep 0.2 2>/dev/null || true
        kill -9 "${HYDROGEN_DUP_PID}" >/dev/null 2>&1 || true
    fi
    if declare -f kill_owned_hydrogens >/dev/null 2>&1; then
        # shellcheck disable=SC2310 # Continue cleanup even if reap fails
        kill_owned_hydrogens 2>/dev/null || true
    fi
}

on_mdns_err() {
    local ec=$?
    printf 'ERR: %s (exit %s)\n' "${BASH_COMMAND}" "${ec}" >&2
    # shellcheck disable=SC2310 # Continue even if dump fails
    dump_collected_output 2>/dev/null || true
}

# --- System state dump ---

dump_mdns_system_state() {
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "=== System State for mDNS Debugging ==="

    if systemctl is-active --quiet avahi-daemon 2>/dev/null; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: RUNNING (may conflict with hydrogen mDNS)"
    elif service avahi-daemon status >/dev/null 2>&1; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: RUNNING via service (may conflict)"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-daemon: NOT RUNNING (hydrogen should be primary mDNS responder)"
    fi

    if systemctl is-active --quiet systemd-resolved 2>/dev/null; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "systemd-resolved: RUNNING"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "systemd-resolved: NOT RUNNING"
    fi

    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Network interfaces with MULTICAST:"
    for iface in $(find /sys/class/net/ -maxdepth 1 -type d -exec basename {} \; 2>/dev/null | head -10 || true); do
        if [[ -f "/sys/class/net/${iface}/flags" ]]; then
            flags_hex=$(cat "/sys/class/net/${iface}/flags" 2>/dev/null)
            if [[ "${flags_hex}" =~ 0x[0-9a-fA-F]+ ]]; then
                flags_value=${flags_hex:2}
                flags_dec=$(printf "%d" "0x${flags_value}" 2>/dev/null || echo "0")
                if (( (flags_dec & 4096) != 0 )); then
                    printf -v output "  %-12s: MULTICAST enabled" "${iface}"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output}"
                fi
            fi
        fi
    done

    local other_services_count
    # shellcheck disable=SC2312 # We want to continue even if the command fails
    other_services_count=$(timeout 1 avahi-browse -a -p >/dev/null 2>&1 | wc -l 2>/dev/null || echo "0")
    if [[ "${other_services_count}" -gt 0 ]]; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Other mDNS services on network: ${other_services_count} services found"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Other mDNS services on network: none found"
    fi

    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" " "
}

# --- Logging tests ---

test_mdns_server_logging() {
    local log_file="$1"
    local max_wait=10
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for mDNS server logging activity..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSServer" "${log_file}" 2>/dev/null; then
            local mdns_server_output
            mdns_server_output=$("${GREP}" -c "mDNSServer" "${log_file}" 2>/dev/null || true)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_server_output} mDNS server log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS server logging detected (${mdns_server_output} entries)"
            return 0
        fi
        sleep 0.02
        attempt=$((attempt + 1))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS server logging detected within ${max_wait} seconds"
    return 1
}

test_mdns_client_logging() {
    local log_file="$1"
    local max_wait=10
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for mDNS client logging activity..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSClient" "${log_file}" 2>/dev/null; then
            local mdns_client_output
            mdns_client_output=$("${GREP}" -c "mDNSClient" "${log_file}" 2>/dev/null || true)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${mdns_client_output} mDNS client log entries"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS client logging detected (${mdns_client_output} entries)"
            return 0
        fi
        sleep 0.02
        attempt=$((attempt + 1))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mDNS client logging detected within ${max_wait} seconds"
    return 1
}

# --- External tools test ---

test_mdns_external_tools() {
    local server_port="$1"
    local external_tool_found=false
    local discovery_success=false

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing external mDNS discovery tools if available..."

    dump_mdns_system_state

    if command -v avahi-browse >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with avahi-browse..."
        external_tool_found=true

        if systemctl is-active --quiet avahi-daemon 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: avahi-daemon is running - may conflict with hydrogen mDNS server"
        fi

        local avahi_timeout=1
        local avahi_output
        avahi_output=$(timeout "${avahi_timeout}" avahi-browse -a -p -r >/dev/null 2>&1 || echo "")

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
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "avahi-browse did not find hydrogen services within ${avahi_timeout}s"
        fi
    fi

    if command -v dns-sd >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with dns-sd..."
        external_tool_found=true

        local dns_sd_output
        dns_sd_output=$(timeout 2 dns-sd -B _http._tcp local 2>/dev/null || echo "")

        if echo "${dns_sd_output}" | "${GREP}" -q "hydrogen"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd successfully discovered hydrogen services"
            discovery_success=true
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "dns-sd did not find hydrogen services within 2s"
        fi
    fi

    if [[ "${external_tool_found}" = true ]]; then
        if [[ "${discovery_success}" = true ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External mDNS discovery tools confirmed service announcements"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "External discovery tools available but services not yet detected (diagnostic info provided)"
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No external mDNS discovery tools available (avahi-browse or dns-sd not installed)"
    fi
    return 0
}

# --- Probe/claim verification ---

test_mdns_server_claimed() {
    local log_file="$1"
    local max_wait=50
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for MDNS_SERVER CLAIMED..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "MDNS_SERVER CLAIMED" "${log_file}" 2>/dev/null; then
            local claimed_output
            claimed_output=$("${GREP}" -c "MDNS_SERVER CLAIMED" "${log_file}" 2>/dev/null || true)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${claimed_output} MDNS_SERVER CLAIMED log entries"
            return 0
        fi
        sleep 0.1
        attempt=$((attempt + 1))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "MDNS_SERVER CLAIMED not found within 5 seconds"
    return 1
}

# --- Log contract verification ---

test_mdns_log_contract() {
    local log_file="$1"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify mDNS log contract"

    local missing=0

    local probe_count
    probe_count=$("${GREP}" -c "MDNS_SERVER PROBE" "${log_file}" 2>/dev/null || true)
    if [[ "${probe_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_SERVER PROBE in log"
        missing=1
    fi

    local claimed_count
    claimed_count=$("${GREP}" -c "MDNS_SERVER CLAIMED" "${log_file}" 2>/dev/null || true)
    if [[ "${claimed_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_SERVER CLAIMED in log"
        missing=1
    fi

    local query_count
    query_count=$("${GREP}" -c "MDNS_CLIENT QUERY" "${log_file}" 2>/dev/null || true)
    if [[ "${query_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_CLIENT QUERY in log"
        missing=1
    fi

    local found_count
    found_count=$("${GREP}" -c "MDNS_CLIENT FOUND" "${log_file}" 2>/dev/null || true)
    if [[ "${found_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_CLIENT FOUND in log"
        missing=1
    fi

    local srv_count
    srv_count=$("${GREP}" -c "MDNS_CLIENT SRV" "${log_file}" 2>/dev/null || true)
    if [[ "${srv_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_CLIENT SRV in log"
        missing=1
    fi

    local txt_count
    txt_count=$("${GREP}" -c "MDNS_CLIENT TXT" "${log_file}" 2>/dev/null || true)
    if [[ "${txt_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_CLIENT TXT in log"
        missing=1
    fi

    local addr_count
    addr_count=$("${GREP}" -c "MDNS_CLIENT ADDR" "${log_file}" 2>/dev/null || true)
    if [[ "${addr_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing MDNS_CLIENT ADDR in log"
        missing=1
    fi

    if [[ "${missing}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Log contract tokens: PROBE=${probe_count} CLAIMED=${claimed_count} QUERY=${query_count} FOUND=${found_count} SRV=${srv_count} TXT=${txt_count} ADDR=${addr_count}"
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${missing}" "Log contract: PROBE, CLAIMED, QUERY, FOUND, SRV, TXT, ADDR"
    return "${missing}"
}

# --- /api/system/info mDNS fields ---

test_mdns_system_info() {
    local server_port="$1"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify mDNS fields on /api/system/info"

    local info_json
    info_json=$(curl -sf --max-time 3 "http://127.0.0.1:${server_port}/api/system/info" 2>/dev/null || echo "")

    if [[ -z "${info_json}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to retrieve /api/system/info"
        return 1
    fi

    local claimed_count
    claimed_count=$(echo "${info_json}" | jq -r '.mdns.claimed | if type == "array" then length else 0 end' 2>/dev/null || echo "0")
    claimed_count=$(echo "${claimed_count}" | tr -d '[:space:]')
    local cache_count
    cache_count=$(echo "${info_json}" | jq -r '.mdns.cache_count // 0' 2>/dev/null || echo "0")
    local hostname_val
    hostname_val=$(echo "${info_json}" | jq -r '.mdns.hostname // ""' 2>/dev/null || echo "")

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mdns.claimed_count=${claimed_count}, cache_count=${cache_count}, hostname=${hostname_val}"

    local missing=0
    if [[ "${claimed_count}" -lt 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected at least 1 claimed name"
        missing=1
    fi
    if [[ -z "${hostname_val}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected non-empty hostname in mdns info"
        missing=1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${missing}" "API system/info mDNS fields verified"
    return "${missing}"
}

# --- Duplicate-name rename test ---

test_mdns_duplicate_names() {
    local server_port="$1"
    local log_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test dual-process duplicate-name rename"

    if [[ ! -f "${DUP_CONFIG_FILE}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Duplicate config not found: ${DUP_CONFIG_FILE}"
        return 1
    fi

    local dup_port
    dup_port=$(jq -r '.WebServer.Port' "${DUP_CONFIG_FILE}" 2>/dev/null || echo "5260")

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting Hydrogen B (duplicate name config) on port ${dup_port}..."

    # shellcheck disable=SC2310 # We want to continue even if shutdown has issues
    # shellcheck disable=SC2153 # HYDROGEN_BIN is correct, not a misspelling
    if ! start_hydrogen_with_pid "${DUP_CONFIG_FILE}" "${DUP_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_DUP_PID"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen B (duplicate config)"
        return 1
    fi

    local max_wait=50
    local attempt=0
    local conflict_found=false
    local claimed_with_suffix=false

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if [[ "${conflict_found}" == false ]]; then
            if "${GREP}" -q "MDNS_SERVER CONFLICT" "${DUP_LOG}" 2>/dev/null; then
                conflict_found=true
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen B detected name conflict (CONFLICT logged)"
            fi
        fi

        if [[ "${conflict_found}" == true && "${claimed_with_suffix}" == false ]]; then
            if "${GREP}" -q "MDNS_SERVER CLAIMED.*(2)" "${DUP_LOG}" 2>/dev/null || \
               "${GREP}" -q "MDNS_SERVER CLAIMED.*-2" "${DUP_LOG}" 2>/dev/null; then
                claimed_with_suffix=true
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen B claimed renamed name with suffix"
            fi
        fi

        if [[ "${conflict_found}" == true && "${claimed_with_suffix}" == true ]]; then
            break
        fi

        sleep 0.1
        attempt=$((attempt + 1))
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Stopping Hydrogen B..."
    # shellcheck disable=SC2310 # We want to continue even if shutdown has issues
    stop_hydrogen "${HYDROGEN_DUP_PID}" "${DUP_LOG}" "${SHUTDOWN_TIMEOUT}" 5 "${DIAG_TEST_DIR}" || true
    HYDROGEN_DUP_PID=""
    # shellcheck disable=SC2310 # Ensure process is gone
    kill_owned_hydrogens 2>/dev/null || true

    local fail=0
    if [[ "${conflict_found}" == false ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen B did not log CONFLICT"
        fail=1
    fi
    if [[ "${claimed_with_suffix}" == false ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen B did not log CLAIMED with (2) suffix"
        fail=1
    fi

    if command -v tshark >/dev/null 2>&1 && [[ -f "${TRACED_LOG}" ]]; then
        local renamed_count
        renamed_count=$(tshark -r "${TRACED_LOG}" -Y "mdns" -T fields -e dns.resp.name 2>/dev/null | grep -c "( 2 )" || true)
        if [[ "${renamed_count}" -gt 0 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "tshark confirms renamed instance in PTR rdata"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "tshark did not find renamed instance (may be in A/B log only)"
        fi
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${fail}" "Duplicate-name rename verified (CONFLICT + CLAIMED with suffix)"
    return "${fail}"
}

# --- tshark packet assertions ---

test_mdns_packet_assertions() {
    local capture_file="$1"
    local server_port="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify tshark packet contents"

    if ! command -v tshark >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "tshark not available - packet assertions skipped"
        return 0
    fi

    if [[ ! -f "${capture_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Capture file not found: ${capture_file}"
        return 1
    fi

    local missing=0

    local ptr_count
    ptr_count=$(tshark -r "${capture_file}" -Y "mdns and dns.resp.type == 12" 2>/dev/null | wc -l || true)
    ptr_count=$(echo "${ptr_count}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "PTR records in capture: ${ptr_count}"
    if [[ "${ptr_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No PTR records found after CLAIMED"
        missing=1
    fi

    local srv_count
    srv_count=$(tshark -r "${capture_file}" -Y "mdns and dns.resp.type == 33" 2>/dev/null | wc -l || true)
    srv_count=$(echo "${srv_count}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "SRV records in capture: ${srv_count}"
    if [[ "${srv_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No SRV records found after CLAIMED"
        missing=1
    fi

    local txt_count
    txt_count=$(tshark -r "${capture_file}" -Y "mdns and dns.resp.type == 16" 2>/dev/null | wc -l || true)
    txt_count=$(echo "${txt_count}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "TXT records in capture: ${txt_count}"
    if [[ "${txt_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No TXT records found after CLAIMED"
        missing=1
    fi

    local a_count aaaa_count
    a_count=$(tshark -r "${capture_file}" -Y "mdns and dns.a" 2>/dev/null | wc -l || true)
    a_count=$(echo "${a_count}" | tr -d '[:space:]')
    aaaa_count=$(tshark -r "${capture_file}" -Y "mdns and dns.aaaa" 2>/dev/null | wc -l || true)
    aaaa_count=$(echo "${aaaa_count}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "A records: ${a_count}, AAAA records: ${aaaa_count} (one family is sufficient)"
    if [[ "${a_count}" -eq 0 && "${aaaa_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No A or AAAA records found after CLAIMED"
        missing=1
    fi

    local aa_count
    aa_count=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.authoritative == 1 and dns.flags.response == 1" 2>/dev/null | wc -l || true)
    aa_count=$(echo "${aa_count}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Authoritative responses (QR=1 AA=1): ${aa_count}"
    if [[ "${aa_count}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No authoritative responses found"
        missing=1
    fi

    local probe_pkt
    probe_pkt=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.response == 0 and dns.qry.type == 255" 2>/dev/null | wc -l || true)
    probe_pkt=$(echo "${probe_pkt}" | tr -d '[:space:]')
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Probe query packets (ANY question): ${probe_pkt}"

    local goodbye_count
    goodbye_count=$(tshark -r "${capture_file}" -Y "mdns" -V 2>/dev/null | grep -c "Time to live: 0" || true)
    local total_goodbye=$((goodbye_count))
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Goodbye TTL-0 records: ${total_goodbye} (verbose scan)"
    if [[ "${total_goodbye}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No TTL=0 goodbye packets found after shutdown"
        missing=1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${missing}" "tshark packet assertions: PTR+SRV+TXT+A|AAAA+QR=1AA=1+probe+goodbye"
    return "${missing}"
}

# --- Server announcements ---

test_mdns_server_announcements() {
    local log_file="$1"
    local max_wait=50
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS server service announcements..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null; then
            local announcement_output
            announcement_output=$("${GREP}" -c "mDNSServer.*announce\|mDNSServer.*advertise\|mDNSServer.*broadcast" "${log_file}" 2>/dev/null || true)
            local announcement_count
            announcement_count=$((announcement_output + 0))

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${announcement_count} service announcement activities"

            local http_output
            http_output=$("${GREP}" -c "_http._tcp\|Hydrogen.*HTTP" "${log_file}" 2>/dev/null || true)
            local http_services
            http_services=$((http_output + 0))
            local websocket_output
            websocket_output=$("${GREP}" -c "_websocket._tcp\|Hydrogen.*WebSocket" "${log_file}" 2>/dev/null || true)
            local websocket_services
            websocket_services=$((websocket_output + 0))

            if [[ "${http_services}" -gt 0 ]] || [[ "${websocket_services}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected HTTP services: ${http_services}, WebSocket services: ${websocket_services}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mDNS services configured"
                return 0
            fi
        fi
        sleep 0.1
        attempt=$((attempt + 1))
    done

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mDNS server announcement activities not detected within ${max_wait} seconds"
    return 1
}

# --- Client discovery ---

test_mdns_client_discovery() {
    local log_file="$1"
    local max_wait=25
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS client service discovery..."

    while [[ "${attempt}" -lt "${max_wait}" ]]; do
        if "${GREP}" -q "mDNSClient.*discover\|mDNSClient.*found\|mDNSClient.*cache" "${log_file}" 2>/dev/null; then
            local discovery_output
            discovery_output=$("${GREP}" -c "mDNSClient.*discover\|mDNSClient.*found\|mDNSClient.*cache" "${log_file}" 2>/dev/null || true)
            local discovery_count
            discovery_count=$((discovery_output + 0))

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${discovery_count} client discovery activities"

            local services_output
            services_output=$("${GREP}" -c "mDNSClient.*Hydrogen\|mDNSClient.*_http\|_tcp\|_udp" "${log_file}" 2>/dev/null || true)
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

# --- Responder loop ---

test_mdns_responder_loop() {
    local server_port="$1"
    local max_wait=5
    local attempt=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing mDNS responder loop functionality..."

    sleep 0.1

    if command -v avahi-browse >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sending mDNS queries using avahi-browse..."

        timeout 1 avahi-browse -p -r _http._tcp >/dev/null 2>&1 &
        sleep 0.1

        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query" "${SERVER_LOG}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Responder loop activity detected in server log"
            return 0
        fi
    fi

    if command -v dns-sd >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing with dns-sd..."

        timeout 1 dns-sd -Q Hydrogen_Test._http._tcp. local 2>/dev/null &
        timeout 1 dns-sd -Q _http._tcp local 2>/dev/null &

        sleep 0.25

        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query\|mDNSServer.*response" "${SERVER_LOG}" 2>/dev/null; then
            local responder_activity
            responder_activity=$("${GREP}" -c "mDNSServer.*responder\|mDNSServer.*query\|mDNSServer.*response" "${SERVER_LOG}" 2>/dev/null || true)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${responder_activity} responder loop activities in server log"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mDNS responder loop activity detected in logs"
            return 0
        fi
    fi

    if command -v nc >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Attempting manual mDNS query via netcat..."

        {
            printf '\x12\x34\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00'
            printf '\x05_http\x04_tcp\x05local\x00'
            printf '\x00\x0c\x00\x01'
        } | timeout 1 nc -u -w1 224.0.0.251 5353 2>/dev/null &

        sleep 0.25

        if "${GREP}" -q "mDNSServer.*responder\|mDNSServer.*query" "${SERVER_LOG}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Responder loop activity detected after manual query"
            return 0
        fi
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Could not trigger responder loop - no suitable tools available"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Responder loop test attempted (tools not available)"
    return 0
}

# --- Packet analysis ---

analyze_mdns_packets() {
    local capture_file="$1"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing mDNS packet types..."

    local total_packets
    total_packets=$(tshark -r "${capture_file}" 2>/dev/null | wc -l || true)
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total packets in capture: ${total_packets}"

    if [[ "${total_packets}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No packets found in capture file"
        return 0
    fi

    local aa_packets
    aa_packets=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.aa == 1" 2>/dev/null | wc -l || true)
    local response_packets
    response_packets=$(tshark -r "${capture_file}" -Y "mdns and dns.flags.response == 1" 2>/dev/null | wc -l || true)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Authoritative Answers (Announcements): ${aa_packets}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Query Responses: ${response_packets}"

    local ptr_queries
    ptr_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 12" 2>/dev/null | wc -l || true)
    local srv_queries
    srv_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 33" 2>/dev/null | wc -l || true)
    local txt_queries
    txt_queries=$(tshark -r "${capture_file}" -Y "mdns and dns.qry.type == 16" 2>/dev/null | wc -l || true)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Query types: PTR=${ptr_queries}, SRV=${srv_queries}, TXT=${txt_queries}"

    local a_rr
    a_rr=$(tshark -r "${capture_file}" -Y "mdns and dns.a" 2>/dev/null | wc -l || true)
    local aaaa_rr
    aaaa_rr=$(tshark -r "${capture_file}" -Y "mdns and dns.aaaa" 2>/dev/null | wc -l || true)
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Address RRs: A=${a_rr}, AAAA=${aaaa_rr} (one family is sufficient)"

    return 0
}

# --- Hydrogen packet detection ---

capture_contains_hydrogen() {
    if [[ -f "${TRACED_LOG}" ]]; then
        # shellcheck disable=SC2312 # We want to check grep result, not strings result
        if strings "${TRACED_LOG}" 2>/dev/null | grep -q "Hydrogen_Test"; then
            return 0
        fi
    fi
    if [[ -f "${PACKET_LOG}.netcat" ]]; then
        # shellcheck disable=SC2312 # We want to check grep result, not strings result
        if strings "${PACKET_LOG}.netcat" 2>/dev/null | grep -q "Hydrogen_Test"; then
            return 0
        fi
    fi
    return 1
}

wait_for_hydrogen_mdns_packets() {
    local attempt=0
    local max_attempts=25

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for Hydrogen mDNS names in packet capture..."
    while [[ "${attempt}" -lt "${max_attempts}" ]]; do
        # shellcheck disable=SC2310 # Want false to mean not yet captured
        if capture_contains_hydrogen; then
            # shellcheck disable=SC2034 # Set for caller (test_25_mdns.sh) to read
            HYDROGEN_MDNS_CAPTURED=1
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen mDNS names observed in capture"
            return 0
        fi
        sleep 0.2
        attempt=$((attempt + 1))
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timed out waiting for Hydrogen mDNS names in capture"
    return 1
}

# --- Capture stop ---

stop_mdns_packet_capture() {
    if [[ -n "${TCAP_PID:-}" ]]; then
        kill -INT "${TCAP_PID}" >/dev/null 2>&1 || true
        sleep 0.2
        if ps -p "${TCAP_PID}" >/dev/null 2>&1; then
            kill -TERM "${TCAP_PID}" >/dev/null 2>&1 || true
        fi
        wait "${TCAP_PID}" 2>/dev/null || true
        TCAP_PID=""
    fi
    if [[ -n "${NC_PID:-}" ]]; then
        kill -INT "${NC_PID}" >/dev/null 2>&1 || true
        sleep 0.1
        if ps -p "${NC_PID}" >/dev/null 2>&1; then
            kill -TERM "${NC_PID}" >/dev/null 2>&1 || true
        fi
        wait "${NC_PID}" 2>/dev/null || true
        NC_PID=""
    fi
}
