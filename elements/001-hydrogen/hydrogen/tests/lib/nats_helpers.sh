#!/usr/bin/env bash
# shellcheck disable=SC2310,SC2154 # Conditionals must not abort the script. Globals come from framework.sh.

# NATS blackbox helpers for tests/test_62_nats.sh.
#
# Locate, start, and stop a local plaintext nats-server. Scrape
# Prometheus to a file and read one gauge. Publish one envelope.
# The publisher does not print a password. The test configs have none.

# CHANGELOG
# 1.0.1 - 2026-10-06 - Publisher CONNECT omits echo. A held connection sends one PUB after the caller samples gauges.
# 1.0.0 - 2026-10-06 - Initial helper for Test 62

[[ -n "${NATS_HELPERS_GUARD:-}" ]] && return 0
export NATS_HELPERS_GUARD="true"

NATS_HELPERS_NAME="NATS Test Helpers"
NATS_HELPERS_VERSION="1.0.1"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${NATS_HELPERS_NAME} ${NATS_HELPERS_VERSION}"

# command -v, then /usr/local/bin. A miss is a failed locate, not a substitute.
nats_locate_server() {
    local found=""

    if command -v nats-server >/dev/null 2>&1; then
        found=$(command -v nats-server)
    elif [[ -x /usr/local/bin/nats-server ]]; then
        found="/usr/local/bin/nats-server"
    fi
    if [[ -n "${found}" && -x "${found}" ]]; then
        NATS_SERVER_BIN="${found}"
        return 0
    fi
    NATS_SERVER_BIN=""
    return 1
}

# Bind the client port and the monitor port. Returns 0 once /varz is HTTP 200.
nats_start_server() {
    local log_file="$1"
    local code=""
    local i

    if [[ -z "${NATS_SERVER_BIN:-}" || ! -x "${NATS_SERVER_BIN}" ]]; then
        return 1
    fi
    : > "${log_file}"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${NATS_SERVER_BIN}") -a 127.0.0.1 -p 5620 -m 5629"
    "${NATS_SERVER_BIN}" -a 127.0.0.1 -p 5620 -m 5629 > "${log_file}" 2>&1 &
    NATS_SERVER_PID=$!
    disown "${NATS_SERVER_PID}" 2>/dev/null || true
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
        code=$(curl -sS -o /dev/null -w "%{http_code}" --connect-timeout 1 --max-time 1 \
            "http://127.0.0.1:5629/varz" || true)
        if [[ "${code}" == "200" ]]; then
            return 0
        fi
        if ! kill -0 "${NATS_SERVER_PID}" 2>/dev/null; then
            return 1
        fi
        sleep 0.25
    done
    return 1
}

# Interrupt the broker and wait until the client port refuses connections.
nats_stop_server() {
    local pid="${NATS_SERVER_PID:-}"
    local i

    if [[ -z "${pid}" ]]; then
        return 0
    fi
    if kill -0 "${pid}" 2>/dev/null; then
        kill -INT "${pid}" 2>/dev/null || true
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
            if ! kill -0 "${pid}" 2>/dev/null; then
                break
            fi
            sleep 0.1
        done
        if kill -0 "${pid}" 2>/dev/null; then
            kill -9 "${pid}" 2>/dev/null || true
        fi
    fi
    NATS_SERVER_PID=""
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
        if nats_port_closed 5620 && nats_port_closed 5629; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

# 0 when nothing accepts a TCP connection on 127.0.0.1:port.
nats_port_closed() {
    local port="$1"

    if "${TIMEOUT}" 0.3 bash -c "echo >/dev/tcp/127.0.0.1/${port}" >/dev/null 2>&1; then
        return 1
    fi
    return 0
}

# Save the Prometheus body. The caller reads the file. Do not pipe curl.
nats_scrape_prometheus() {
    local port="$1"
    local outfile="$2"
    local code=""

    code=$(curl -sS -o "${outfile}" -w "%{http_code}" --connect-timeout 1 --max-time 2 \
        "http://127.0.0.1:${port}/api/system/prometheus" || true)
    [[ "${code}" == "200" && -s "${outfile}" ]]
}

# Print one unlabeled gauge value from a saved Prometheus body.
nats_gauge_value() {
    local file="$1"
    local name="$2"
    local line=""
    local key=""
    local value=""

    while IFS= read -r line || [[ -n "${line}" ]]; do
        key="${line%% *}"
        if [[ "${key}" == "${name}" ]]; then
            value="${line#* }"
            printf '%s\n' "${value}"
            return 0
        fi
    done < "${file}"
    return 1
}

# Wait until one gauge equals (eq) or is at least (ge) expect, or the budget ends.
# The last saved body stays in outfile. NATS_GAUGE_LAST is the last value seen.
nats_wait_gauge() {
    local port="$1"
    local name="$2"
    local op="$3"
    local expect="$4"
    local budget="$5"
    local outfile="$6"
    local deadline=0
    local value=""

    NATS_GAUGE_LAST=""
    deadline=$((SECONDS + budget))
    while (( SECONDS <= deadline )); do
        if nats_scrape_prometheus "${port}" "${outfile}"; then
            if value=$(nats_gauge_value "${outfile}" "${name}"); then
                NATS_GAUGE_LAST="${value}"
                case "${op}" in
                    eq)
                        if (( value == expect )); then
                            return 0
                        fi
                        ;;
                    ge)
                        if (( value >= expect )); then
                            return 0
                        fi
                        ;;
                    *)
                        return 1
                        ;;
                esac
            fi
        fi
        sleep 0.1
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Gauge ${name} last value ${NATS_GAUGE_LAST:-missing}"
    return 1
}

# Read INFO, CONNECT without credentials and without echo set to false, PING, one PUB.
# echo false would hide this client's PUB from Hydrogen. The envelope field order
# is event, subject, timestamp, source, instance_id, data.
# Nothing written here includes a password, and the body is not printed.
nats_publish_envelope() {
    local subject="$1"
    local event="$2"
    local instance_id="$3"
    local data_json="$4"
    local port="${5:-5620}"
    local connect=""
    local body=""
    local stamp=""
    local line=""
    local i

    connect=$(jq -nc '{verbose:false, pedantic:false, protocol:1, lang:"hydrogen-test", version:"62"}') || return 1
    stamp=$("${DATE}" -u +%Y-%m-%dT%H:%M:%SZ)
    body=$(jq -nc \
        --arg event "${event}" \
        --arg subject "${subject}" \
        --arg timestamp "${stamp}" \
        --arg source "${instance_id}" \
        --arg instance_id "${instance_id}" \
        --argjson data "${data_json}" \
        '{event:$event, subject:$subject, timestamp:$timestamp, source:$source, instance_id:$instance_id, data:$data}') || return 1
    exec 3<>"/dev/tcp/127.0.0.1/${port}" || return 1
    if ! IFS= read -r -t 2 line <&3; then
        exec 3>&-
        return 1
    fi
    printf 'CONNECT %s\r\nPING\r\nPUB %s %s\r\n%s\r\n' \
        "${connect}" "${subject}" "${#body}" "${body}" >&3 || {
        exec 3>&-
        return 1
    }
    # PONG arrives in a few milliseconds. A 1s idle read lets heartbeats
    # move both received gauges before the caller samples them.
    read_wait=0.2
    seen_pong=0
    i=0
    while (( i < 8 )); do
        if ! IFS= read -r -t "${read_wait}" line <&3; then
            break
        fi
        line="${line%$'\r'}"
        if [[ "${line}" == -ERR* ]]; then
            exec 3>&-
            return 1
        fi
        if [[ "${line}" == PONG ]]; then
            seen_pong=1
            read_wait=0.05
        fi
        i=$((i + 1))
    done
    exec 3>&-
    [[ "${seen_pong}" -eq 1 ]]
}

# Handshake only. The caller samples gauges, then nats_publisher_send writes one PUB.
# Heartbeats during the handshake stay outside that sample.
nats_publisher_begin() {
    local port="${1:-5620}"
    local connect=""
    local line=""
    local read_wait=0.2
    local seen_pong=0
    local i=0

    exec 9>&- 2>/dev/null || true
    connect=$(jq -nc '{verbose:false, pedantic:false, protocol:1, lang:"hydrogen-test", version:"62"}') || return 1
    exec 9<>"/dev/tcp/127.0.0.1/${port}" || return 1
    if ! IFS= read -r -t 2 line <&9; then
        exec 9>&- 2>/dev/null || true
        return 1
    fi
    printf 'CONNECT %s\r\nPING\r\n' "${connect}" >&9 || {
        exec 9>&- 2>/dev/null || true
        return 1
    }
    while (( i < 8 )); do
        if ! IFS= read -r -t "${read_wait}" line <&9; then
            break
        fi
        line="${line%$'\r'}"
        if [[ "${line}" == -ERR* ]]; then
            exec 9>&- 2>/dev/null || true
            return 1
        fi
        if [[ "${line}" == PONG ]]; then
            seen_pong=1
            read_wait=0.05
        fi
        i=$((i + 1))
    done
    if [[ "${seen_pong}" -ne 1 ]]; then
        exec 9>&- 2>/dev/null || true
        return 1
    fi
    return 0
}

nats_publisher_send() {
    local subject="$1"
    local event="$2"
    local instance_id="$3"
    local data_json="$4"
    local body=""
    local stamp=""
    local line=""

    stamp=$("${DATE}" -u +%Y-%m-%dT%H:%M:%SZ)
    body=$(jq -nc \
        --arg event "${event}" \
        --arg subject "${subject}" \
        --arg timestamp "${stamp}" \
        --arg source "${instance_id}" \
        --arg instance_id "${instance_id}" \
        --argjson data "${data_json}" \
        '{event:$event, subject:$subject, timestamp:$timestamp, source:$source, instance_id:$instance_id, data:$data}') || return 1
    printf 'PUB %s %s\r\n%s\r\n' "${subject}" "${#body}" "${body}" >&9 || return 1
    if IFS= read -r -t 0.02 line <&9; then
        line="${line%$'\r'}"
        if [[ "${line}" == -ERR* ]]; then
            return 1
        fi
    fi
    return 0
}

nats_publisher_end() {
    exec 9>&- 2>/dev/null || true
}
