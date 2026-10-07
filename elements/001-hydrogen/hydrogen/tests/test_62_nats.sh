#!/usr/bin/env bash
# shellcheck disable=SC2310 # Lifecycle and helper predicates run in conditionals so one failed check does not abort the script.

# Test: NATS plaintext client (Phase 11)
#
# Hydrogen against a local nats-server on 127.0.0.1:5620.
# Assertions are Prometheus gauges. MockConnection stays false.
# Cache invalidation rows stay on the Phase 5 Unity tests.

# CHANGELOG
# 1.0.2 - 2026-10-06 - Queue check counts the jobs.refresh-all trace. A presence heartbeat is not a second delivery.
# 1.0.1 - 2026-10-06 - Watch received for 3s, give websocat a terminal so frames flush, sample a short quiet gap for the queue, recheck subjects after reconnect
# 1.0.0 - 2026-10-06 - Initial blackbox Test 62

set -euo pipefail

# Test configuration
TEST_NAME="NATS"
TEST_ABBR="NAT"
TEST_NUMBER="62"
TEST_COUNTER=0
TEST_VERSION="1.0.2"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/nats_helpers.sh # Locate, broker, gauges, and one PUB
[[ -n "${NATS_HELPERS_GUARD:-}" ]] || source "${LIB_DIR}/nats_helpers.sh"

# The disabled launch line is DEBUG. TRACE keeps it on the console log.
export HYDROGEN_LOG_LEVEL=TRACE
export WEBSOCKET_KEY="${WEBSOCKET_KEY:-nats62-websocket-key}"

PORT_NATS=5620
PORT_DISABLED=5621
PORT_A=5622
PORT_BAD=5623
PORT_B=5624
PORT_WS_A=5625
PORT_WS_B=5626
PORT_CLOSED=5628
PORT_MONITOR=5629
STARTUP_TIMEOUT=60
SHUTDOWN_TIMEOUT=30
SHUTDOWN_ACTIVITY=10

CONFIG_DISABLED="${CONFIG_DIR}/hydrogen_test_${TEST_NUMBER}_nats_disabled.json"
CONFIG_BAD="${CONFIG_DIR}/hydrogen_test_${TEST_NUMBER}_nats_bad.json"
CONFIG_LOCAL="${CONFIG_DIR}/hydrogen_test_${TEST_NUMBER}_nats_local.json"
SQLITE_ARTIFACT="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrotst.sqlite"

NATS62_HAVE_BIN=0
NATS62_BROKER_UP=0
NATS62_UP_MARK=-1
HYDROGEN_DISABLED_PID=""
HYDROGEN_BAD_PID=""
HYDROGEN_A_PID=""
HYDROGEN_B_PID=""
RUNTIME_BAD=""
RUNTIME_A=""
RUNTIME_B=""
WS_SUB_PID=""
WS_EMPTY_PID=""
WS_SUB_OUT=""
WS_EMPTY_OUT=""
WEBSOCAT_BIN=""

# shellcheck disable=SC2329 # invoked via trap EXIT
cleanup_nats62() {
    if [[ -n "${WS_SUB_PID:-}" ]]; then
        nat62_stop_ws "${WS_SUB_PID}"
        WS_SUB_PID=""
    fi
    if [[ -n "${WS_EMPTY_PID:-}" ]]; then
        nat62_stop_ws "${WS_EMPTY_PID}"
        WS_EMPTY_PID=""
    fi
    if [[ -n "${NATS_SERVER_PID:-}" ]]; then
        kill -INT "${NATS_SERVER_PID}" 2>/dev/null || true
    fi
    if declare -f _hydrogen_owned_exit_trap >/dev/null 2>&1; then
        _hydrogen_owned_exit_trap
    fi
}
trap cleanup_nats62 EXIT

nat62_pass() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "$1"
}

nat62_fail() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "$1"
    EXIT_CODE=1
}

# setsid makes this pid the process-group leader, so one signal stops socat and websocat.
nat62_stop_ws() {
    local pid="$1"

    if [[ -z "${pid}" ]]; then
        return 0
    fi
    kill -- "-${pid}" 2>/dev/null || kill "${pid}" 2>/dev/null || true
    wait "${pid}" 2>/dev/null || true
}

nat62_stop_quiet() {
    local pid="$1"
    local log_file="$2"

    if [[ -z "${pid}" ]]; then
        return 0
    fi
    if stop_hydrogen "${pid}" "${log_file}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY}" "${DIAG_TEST_DIR}"; then
        return 0
    fi
    return 1
}

nat62_sqlite_copy() {
    local dest="$1"

    if [[ ! -f "${SQLITE_ARTIFACT}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing SQLite artifact ${SQLITE_ARTIFACT}"
        return 1
    fi
    cp -a "${SQLITE_ARTIFACT}" "${dest}"
}

nat62_subsz_ok() {
    local file="$1"

    jq -e '
        def names: [.subscriptions_list[]?.subject];
        (names | index("cluster.philement.cache.invalidate")) != null and
        (names | index("cluster.philement.instance.app_state")) != null and
        (names | index("cluster.philement.order.updated")) != null and
        (names | index("cluster.philement.jobs.refresh-all")) != null and
        ([.subscriptions_list[]?
            | select(.subject == "cluster.philement.jobs.refresh-all")
            | .qgroup] | index("philement-refresh")) != null
    ' "${file}" >/dev/null
}

nat62_ws_has() {
    local file="$1"
    local filter="$2"
    local hit="${file}.hit"

    if [[ ! -s "${file}" ]]; then
        return 1
    fi
    jq -c -R --arg event "${filter}" 'gsub("\r$";"") | fromjson? | select(type == "object" and (.type == $event or (.type == "nats_event" and .event == $event)))' \
        "${file}" > "${hit}" 2>/dev/null || true
    [[ -s "${hit}" ]]
}

nat62_wait_ws() {
    local file="$1"
    local kind="$2"
    local budget="$3"
    local deadline=0

    deadline=$((SECONDS + budget))
    while (( SECONDS <= deadline )); do
        if nat62_ws_has "${file}" "${kind}"; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

nat62_open_ws() {
    local which="$1"
    local events="$2"
    local fifo="${DIAG_TEST_DIR}/ws_${which}.fifo"
    local outfile="${DIAG_TEST_DIR}/ws_${which}.out"
    local errfile="${DIAG_TEST_DIR}/ws_${which}.err"
    local launcher="${DIAG_TEST_DIR}/ws_${which}.launch"
    local address=""
    local pid=""

    rm -f "${fifo}" "${launcher}"
    mkfifo "${fifo}"
    : > "${outfile}"
    : > "${errfile}"
    # Not a .sh file: the shell lint walks every .sh under the tree, including
    # diagnostics. The key stays in the environment and is not written here.
    cat > "${launcher}" << 'EOF'
#!/usr/bin/env bash
exec "$WEBSOCAT_BIN" --protocol=hydrogen \
    -H="Authorization: Key ${WEBSOCKET_KEY}" \
    --ping-interval=1 \
    --no-close \
    "$WS_URL" <"$WS_FIFO" 2>"$WS_ERR"
EOF
    chmod +x "${launcher}"
    if [[ "${which}" == "sub" ]]; then
        exec 7<>"${fifo}"
    else
        exec 8<>"${fifo}"
    fi
    # websocat is Rust. A file redirect leaves stdout block-buffered, so the
    # second frame stays invisible until the process exits. socat's pty makes
    # that stdout a terminal and copies each line into the outfile.
    address="SYSTEM:bash ${launcher},pty,raw,echo=0"
    WEBSOCAT_BIN="${WEBSOCAT_BIN}" \
        WS_URL="ws://127.0.0.1:${PORT_WS_A}/wss" \
        WS_FIFO="${fifo}" \
        WS_ERR="${errfile}" \
        setsid socat -u "${address}" "OPEN:${outfile},creat,trunc" \
        >/dev/null 2>"${DIAG_TEST_DIR}/ws_${which}.socat" &
    pid=$!
    if [[ "${which}" == "sub" ]]; then
        WS_SUB_PID="${pid}"
        WS_SUB_OUT="${outfile}"
        printf '%s\n' "${events}" >&7
    else
        WS_EMPTY_PID="${pid}"
        WS_EMPTY_OUT="${outfile}"
        printf '%s\n' "${events}" >&8
    fi
}

nat62_close_empty_ws() {
    if [[ -n "${WS_EMPTY_PID}" ]]; then
        nat62_stop_ws "${WS_EMPTY_PID}"
        WS_EMPTY_PID=""
    fi
    exec 8>&- 2>/dev/null || true
}

nat62_received() {
    local port="$1"
    local file="$2"

    nats_scrape_prometheus "${port}" "${file}" || return 1
    nats_gauge_value "${file}" "hydrogen_nats_received_total"
}

# Lines appended after mark that contain needle. Heartbeats and the queue
# subject both increment received, and the trace names which one arrived.
nat62_log_count() {
    local file="$1"
    local mark="$2"
    local needle="$3"
    local slice="${DIAG_TEST_DIR}/log_count_slice.txt"
    local n=0
    local line=""

    tail -c +$((mark + 1)) "${file}" > "${slice}" 2>/dev/null || true
    while IFS= read -r line || [[ -n "${line}" ]]; do
        if [[ "${line}" == *"${needle}"* ]]; then
            n=$((n + 1))
        fi
    done < "${slice}"
    printf '%s' "${n}"
}

# --- 1. Locate coverage binary and nats-server ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate coverage binary and nats-server"
HYDROGEN_BIN="${PROJECT_DIR}/hydrogen_coverage"
if [[ -x "${HYDROGEN_BIN}" ]]; then
    HYDROGEN_BIN_BASE=$(basename "${HYDROGEN_BIN}")
    export HYDROGEN_BIN HYDROGEN_BIN_BASE
    NATS62_HAVE_BIN=1
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using coverage binary: ${HYDROGEN_BIN_BASE}"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "hydrogen_coverage is missing. Build it with cmake --build build --target coverage. Do not run mkt."
fi
if nats_locate_server; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using nats-server: ${NATS_SERVER_BIN}"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "nats-server was not found on PATH or at /usr/local/bin/nats-server. Install a local NATS 2.x server that speaks the plaintext client protocol. The cluster image is nats:2.11.4. Do not use the live DOKS broker."
fi
if [[ "${NATS62_HAVE_BIN}" -eq 1 ]] && [[ -n "${NATS_SERVER_BIN}" ]]; then
    nat62_pass "Coverage binary and nats-server located"
else
    nat62_fail "Coverage binary or nats-server was not located"
fi

# --- 2. Disabled ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Disabled NATS stays down"
DISABLED_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_disabled.log"
if [[ "${NATS62_HAVE_BIN}" -ne 1 ]]; then
    nat62_fail "hydrogen_coverage is not available"
elif [[ ! -f "${CONFIG_DISABLED}" ]]; then
    nat62_fail "Disabled configuration file is missing"
elif ! start_hydrogen_with_pid "${CONFIG_DISABLED}" "${DISABLED_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_DISABLED_PID"; then
    EXIT_CODE=1
else
    disabled_ok=0
    disabled_detail=""
    prom_disabled="${DIAG_TEST_DIR}/prom_disabled.txt"
    if nats_wait_gauge "${PORT_DISABLED}" "hydrogen_nats_enabled" eq 0 5 "${prom_disabled}"; then
        disabled_up=$(nats_gauge_value "${prom_disabled}" "hydrogen_nats_up" || true)
        if [[ "${disabled_up}" == "0" ]]; then
            disabled_ok=1
        else
            disabled_detail="hydrogen_nats_up is ${disabled_up:-missing}"
        fi
    else
        disabled_detail="hydrogen_nats_enabled did not read 0"
    fi
    if ! "${GREP}" -F -q "NATS subsystem is disabled, skipping launch" "${DISABLED_LOG}"; then
        disabled_ok=0
        disabled_detail="log is missing the disabled launch line"
    fi
    if ! nats_port_closed "${PORT_NATS}"; then
        disabled_ok=0
        disabled_detail="port ${PORT_NATS} is open"
    fi
    if [[ "${disabled_ok}" -ne 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Disabled check: ${disabled_detail}"
    fi
    if ! nat62_stop_quiet "${HYDROGEN_DISABLED_PID}" "${DISABLED_LOG}"; then
        EXIT_CODE=1
        HYDROGEN_DISABLED_PID=""
    else
        HYDROGEN_DISABLED_PID=""
        if [[ "${disabled_ok}" -eq 1 ]]; then
            nat62_pass "Disabled NATS is down and shutdown is clean"
        else
            nat62_fail "Disabled NATS check failed (${disabled_detail})"
        fi
    fi
fi

# --- 3. Unreachable ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Unreachable NATS stays degraded"
BAD_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_bad.log"
RUNTIME_BAD="${DIAG_TEST_DIR}/bad.json"
SQLITE_BAD="${DIAG_TEST_DIR}/bad.sqlite"
if [[ "${NATS62_HAVE_BIN}" -ne 1 ]]; then
    nat62_fail "hydrogen_coverage is not available"
elif [[ -z "${PAYLOAD_KEY:-}" ]]; then
    nat62_fail "PAYLOAD_KEY is not set"
elif ! nat62_sqlite_copy "${SQLITE_BAD}"; then
    nat62_fail "SQLite artifact is missing"
elif ! jq --arg database "${SQLITE_BAD}" '.Databases.Connections[0].Database = $database' "${CONFIG_BAD}" > "${RUNTIME_BAD}"; then
    nat62_fail "Could not rewrite the unreachable configuration"
elif ! start_hydrogen_with_pid "${RUNTIME_BAD}" "${BAD_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_BAD_PID"; then
    EXIT_CODE=1
else
    bad_ok=0
    bad_detail=""
    prom_bad="${DIAG_TEST_DIR}/prom_bad.txt"
    bad_deadline=$((SECONDS + 5))
    while (( SECONDS <= bad_deadline )); do
        if nats_scrape_prometheus "${PORT_BAD}" "${prom_bad}"; then
            bad_enabled=$(nats_gauge_value "${prom_bad}" "hydrogen_nats_enabled" || true)
            bad_up=$(nats_gauge_value "${prom_bad}" "hydrogen_nats_up" || true)
            bad_re=$(nats_gauge_value "${prom_bad}" "hydrogen_nats_reconnects_total" || true)
            if [[ -n "${bad_enabled}" && -n "${bad_up}" && -n "${bad_re}" ]]; then
                if (( bad_up != 0 || bad_re != 0 )); then
                    bad_detail="enabled ${bad_enabled} up ${bad_up} reconnects ${bad_re}"
                    break
                fi
                if (( bad_enabled == 1 )); then
                    bad_ok=1
                    break
                fi
            fi
        fi
        sleep 0.1
    done
    if [[ "${bad_ok}" -ne 1 && -z "${bad_detail}" ]]; then
        bad_detail="gauges did not show enabled 1, up 0, reconnects 0 within 5s"
    fi
    if ! "${GREP}" -F -q "NATS subsystem launch accepted" "${BAD_LOG}"; then
        bad_ok=0
        bad_detail="log is missing the launch accepted line"
    fi
    if ! nats_port_closed "${PORT_CLOSED}"; then
        bad_ok=0
        bad_detail="port ${PORT_CLOSED} is open"
    fi
    if [[ "${bad_ok}" -ne 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unreachable check: ${bad_detail}"
    fi
    if ! nat62_stop_quiet "${HYDROGEN_BAD_PID}" "${BAD_LOG}"; then
        EXIT_CODE=1
        HYDROGEN_BAD_PID=""
    else
        HYDROGEN_BAD_PID=""
        if [[ "${bad_ok}" -eq 1 ]]; then
            nat62_pass "Unreachable NATS is degraded and shutdown is clean"
        else
            nat62_fail "Unreachable NATS check failed (${bad_detail})"
        fi
    fi
fi

# --- 4. Broker ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Local nats-server monitor is up"
NATS_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_nats.log"
VARZ_FILE="${DIAG_TEST_DIR}/varz.json"
if [[ -z "${NATS_SERVER_BIN}" ]]; then
    nat62_fail "nats-server is not installed"
elif ! nats_start_server "${NATS_LOG}"; then
    nat62_fail "nats-server did not accept monitor connections on ${PORT_MONITOR}"
else
    varz_code=$(curl -sS -o "${VARZ_FILE}" -w "%{http_code}" --connect-timeout 1 --max-time 2 \
        "http://127.0.0.1:${PORT_MONITOR}/varz" || true)
    if [[ "${varz_code}" == "200" ]]; then
        NATS62_BROKER_UP=1
        nat62_pass "nats-server /varz returned HTTP 200"
    else
        nat62_fail "nats-server /varz returned HTTP ${varz_code:-000}"
    fi
fi

# --- 5. A reaches up and the four subjects are listed ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Instance A subscribes on the local broker"
A_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_a.log"
RUNTIME_A="${DIAG_TEST_DIR}/a.json"
SQLITE_A="${DIAG_TEST_DIR}/a.sqlite"
PROM_A="${DIAG_TEST_DIR}/prom_a.txt"
SUBSZ_FILE="${DIAG_TEST_DIR}/subsz.json"
if [[ "${NATS62_HAVE_BIN}" -ne 1 ]]; then
    nat62_fail "hydrogen_coverage is not available"
elif [[ "${NATS62_BROKER_UP}" -ne 1 ]]; then
    nat62_fail "nats-server is not running"
elif [[ -z "${PAYLOAD_KEY:-}" ]]; then
    nat62_fail "PAYLOAD_KEY is not set"
elif ! nat62_sqlite_copy "${SQLITE_A}"; then
    nat62_fail "SQLite artifact is missing"
elif ! jq --arg database "${SQLITE_A}" '.Databases.Connections[0].Database = $database' "${CONFIG_LOCAL}" > "${RUNTIME_A}"; then
    nat62_fail "Could not rewrite the local configuration"
elif ! start_hydrogen_with_pid "${RUNTIME_A}" "${A_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_A_PID"; then
    EXIT_CODE=1
elif ! nats_wait_gauge "${PORT_A}" "hydrogen_nats_up" eq 1 10 "${PROM_A}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "hydrogen_nats_up stayed ${NATS_GAUGE_LAST:-missing}"
    nat62_fail "Instance A did not reach hydrogen_nats_up 1 within 10s"
else
    NATS62_UP_MARK=${SECONDS}
    subsz_code=$(curl -sS -o "${SUBSZ_FILE}" -w "%{http_code}" --connect-timeout 1 --max-time 2 \
        "http://127.0.0.1:${PORT_MONITOR}/subsz?subs=1" || true)
    if [[ "${subsz_code}" == "200" ]] && nat62_subsz_ok "${SUBSZ_FILE}"; then
        nat62_pass "Instance A is up and the four subjects are subscribed"
    else
        nat62_fail "Monitor subscriptions did not list the four subjects"
    fi
fi

# --- 6. no_echo ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "no_echo leaves received at zero"
if [[ -z "${HYDROGEN_A_PID}" || "${NATS62_UP_MARK}" -lt 0 ]]; then
    nat62_fail "Instance A is not up"
else
    echo_ok=0
    echo_detail=""
    echo_deadline=$((NATS62_UP_MARK + 3))
    echo_pub=""
    echo_recv=""
    echo_peers=""
    while (( SECONDS <= echo_deadline )); do
        if nats_scrape_prometheus "${PORT_A}" "${PROM_A}"; then
            echo_pub=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_published_total" || true)
            echo_recv=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_received_total" || true)
            echo_peers=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_peers" || true)
            if [[ -n "${echo_pub}" && -n "${echo_recv}" && -n "${echo_peers}" ]]; then
                if (( echo_recv != 0 || echo_peers != 0 )); then
                    echo_detail="published ${echo_pub} received ${echo_recv} peers ${echo_peers}"
                    echo_ok=0
                    break
                fi
                if (( echo_pub >= 1 )); then
                    echo_ok=1
                fi
            fi
        fi
        sleep 0.1
    done
    if [[ "${echo_ok}" -eq 1 && -z "${echo_detail}" ]]; then
        nat62_pass "Published ${echo_pub}, received 0, peers 0 through 3s after up"
    else
        nat62_fail "no_echo sample was ${echo_detail:-published ${echo_pub:-missing} received ${echo_recv:-missing} peers ${echo_peers:-missing}}"
    fi
fi

# --- 7. skip-self ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Skip-self app_state does not add a peer"
if [[ -z "${HYDROGEN_A_PID}" || "${NATS62_BROKER_UP}" -ne 1 ]]; then
    nat62_fail "Instance A or the broker is not up"
else
    skip_before=$(nat62_received "${PORT_A}" "${PROM_A}" || true)
    skip_ok=0
    skip_detail=""
    if [[ -z "${skip_before}" ]]; then
        skip_detail="could not read received"
    elif ! nats_publish_envelope "cluster.philement.instance.app_state" "app_state" "nats-62-a" '{"state":"Alive"}' "${PORT_NATS}"; then
        skip_detail="outside app_state publish failed"
    else
        skip_deadline=$((SECONDS + 3))
        while (( SECONDS <= skip_deadline )); do
            if nats_scrape_prometheus "${PORT_A}" "${PROM_A}"; then
                skip_recv=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_received_total" || true)
                skip_peers=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_peers" || true)
                skip_alive=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_alive" || true)
                if [[ -n "${skip_recv}" && -n "${skip_peers}" && -n "${skip_alive}" ]]; then
                    if (( skip_recv == skip_before + 1 && skip_peers == 0 && skip_alive == 0 )); then
                        skip_ok=1
                        break
                    fi
                    if (( skip_recv > skip_before + 1 || skip_peers != 0 || skip_alive != 0 )); then
                        skip_detail="received ${skip_recv} peers ${skip_peers} alive ${skip_alive}"
                        break
                    fi
                fi
            fi
            sleep 0.1
        done
        if [[ "${skip_ok}" -ne 1 && -z "${skip_detail}" ]]; then
            skip_detail="received did not rise by 1"
        fi
    fi
    if [[ "${skip_ok}" -eq 1 ]]; then
        nat62_pass "Skip-self raised received by 1 and left peers and alive at 0"
    else
        nat62_fail "Skip-self check failed (${skip_detail})"
    fi
fi

# --- 8. Peer B ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Peer B is visible to A"
B_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_b.log"
RUNTIME_B="${DIAG_TEST_DIR}/b.json"
SQLITE_B="${DIAG_TEST_DIR}/b.sqlite"
PROM_B="${DIAG_TEST_DIR}/prom_b.txt"
if [[ -z "${HYDROGEN_A_PID}" || "${NATS62_BROKER_UP}" -ne 1 || ! -f "${RUNTIME_A}" ]]; then
    nat62_fail "Instance A is not up"
elif ! nat62_sqlite_copy "${SQLITE_B}"; then
    nat62_fail "SQLite artifact is missing"
elif ! jq --arg database "${SQLITE_B}" --argjson web "${PORT_B}" --argjson ws "${PORT_WS_B}" \
    '.Server.ServerName = "hydrogen-test-62-b"
     | .WebServer.Port = $web
     | .WebSocketServer.Port = $ws
     | .NATS.InstanceId = "nats-62-b"
     | .Databases.Connections[0].Database = $database' \
    "${RUNTIME_A}" > "${RUNTIME_B}"; then
    nat62_fail "Could not build the peer configuration"
elif ! start_hydrogen_with_pid "${RUNTIME_B}" "${B_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "HYDROGEN_B_PID"; then
    EXIT_CODE=1
elif ! nats_wait_gauge "${PORT_B}" "hydrogen_nats_up" eq 1 10 "${PROM_B}"; then
    nat62_fail "Instance B did not reach hydrogen_nats_up 1 within 10s"
else
    peer_ok=0
    peer_deadline=$((SECONDS + 5))
    peer_detail=""
    while (( SECONDS <= peer_deadline )); do
        if nats_scrape_prometheus "${PORT_A}" "${PROM_A}" && nats_scrape_prometheus "${PORT_B}" "${PROM_B}"; then
            peers_a=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_peers" || true)
            alive_a=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_alive" || true)
            peers_b=$(nats_gauge_value "${PROM_B}" "hydrogen_nats_peers" || true)
            alive_b=$(nats_gauge_value "${PROM_B}" "hydrogen_nats_alive" || true)
            if [[ -n "${peers_a}" && -n "${alive_a}" && -n "${peers_b}" && -n "${alive_b}" ]]; then
                peer_detail="A peers ${peers_a} alive ${alive_a}; B peers ${peers_b} alive ${alive_b}"
                if (( peers_a >= 1 && alive_a >= 1 && peers_b >= 1 && alive_b >= 1 )); then
                    peer_ok=1
                    break
                fi
            fi
        fi
        sleep 0.1
    done
    if [[ "${peer_ok}" -eq 1 ]]; then
        nat62_pass "Both instances report a live peer"
    else
        nat62_fail "Peer gauges did not rise within 5s (${peer_detail:-missing})"
    fi
fi

# --- 9. WebSocket relay ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket relay delivers order.updated"
WEBSOCAT_BIN=$(command -v websocat || true)
if [[ -z "${HYDROGEN_A_PID}" || "${NATS62_BROKER_UP}" -ne 1 ]]; then
    nat62_fail "Instance A or the broker is not up"
elif [[ -z "${WEBSOCAT_BIN}" ]]; then
    nat62_fail "websocat is not installed"
else
    relay_ok=0
    relay_detail=""
    nat62_open_ws "sub" '{"type":"nats_subscribe","events":["order.updated"]}'
    nat62_open_ws "empty" '{"type":"nats_subscribe","events":[]}'
    if ! nat62_wait_ws "${WS_SUB_OUT}" "nats_subscribe_ok" 5; then
        relay_detail="subscriber did not receive nats_subscribe_ok"
    elif ! nat62_wait_ws "${WS_EMPTY_OUT}" "nats_subscribe_ok" 5; then
        relay_detail="empty client did not receive nats_subscribe_ok"
    elif ! nats_publish_envelope "cluster.philement.order.updated" "order.updated" "nats-62-out" '{"id":"62"}' "${PORT_NATS}"; then
        relay_detail="outside order.updated publish failed"
    elif ! nat62_wait_ws "${WS_SUB_OUT}" "order.updated" 3; then
        relay_detail="subscriber did not receive nats_event order.updated"
    else
        sleep 0.3
        if nat62_ws_has "${WS_EMPTY_OUT}" "nats_event"; then
            relay_detail="empty client received a nats_event"
        else
            relay_ok=1
        fi
    fi
    nat62_close_empty_ws
    if [[ "${relay_ok}" -eq 1 ]]; then
        nat62_pass "Subscribed client received order.updated and the empty client did not"
    else
        nat62_fail "WebSocket relay check failed (${relay_detail})"
    fi
fi

# --- 10. Queue group ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Queue group delivers jobs.refresh-all to one member"
if [[ -z "${HYDROGEN_A_PID}" || -z "${HYDROGEN_B_PID}" || "${NATS62_BROKER_UP}" -ne 1 ]]; then
    nat62_fail "Both instances and the broker must be up"
elif [[ -z "${WS_SUB_OUT}" || ! -f "${WS_SUB_OUT}" ]]; then
    nat62_fail "Subscribed WebSocket client is not open"
else
    queue_ok=0
    queue_detail=""
    jobs_needle="NATS message on cluster.philement.jobs.refresh-all"
    app_needle="NATS message on cluster.philement.instance.app_state"
    # A heartbeat on one instance increments received on the other. The
    # trace names the subject, so the jobs line is the queue delivery.
    if ! nats_publisher_begin "${PORT_NATS}"; then
        queue_detail="jobs.refresh-all publisher did not connect"
    else
        queue_log_a=$(wc -c < "${A_LOG}" | tr -d '[:space:]')
        queue_log_b=$(wc -c < "${B_LOG}" | tr -d '[:space:]')
        queue_before_a=$(nat62_received "${PORT_A}" "${PROM_A}" || true)
        queue_before_b=$(nat62_received "${PORT_B}" "${PROM_B}" || true)
        queue_mark=$(wc -c < "${WS_SUB_OUT}" | tr -d '[:space:]')
        if [[ -z "${queue_before_a}" || -z "${queue_before_b}" ]]; then
            queue_detail="received gauge was missing before the publish"
            nats_publisher_end
        elif ! nats_publisher_send "cluster.philement.jobs.refresh-all" "jobs.refresh-all" "nats-62-out" '{"reason":"test"}'; then
            queue_detail="jobs.refresh-all publish failed"
            nats_publisher_end
        else
            nats_publisher_end
            queue_deadline=$((SECONDS + 2))
            queue_jobs_a=0
            queue_jobs_b=0
            while (( SECONDS <= queue_deadline )); do
                queue_jobs_a=$(nat62_log_count "${A_LOG}" "${queue_log_a}" "${jobs_needle}")
                queue_jobs_b=$(nat62_log_count "${B_LOG}" "${queue_log_b}" "${jobs_needle}")
                if (( queue_jobs_a + queue_jobs_b >= 1 )); then
                    break
                fi
                sleep 0.05
            done
            # A second copy would land in this pause. Heartbeats may move
            # the other gauge; they do not add a jobs.refresh-all line.
            sleep 0.3
            queue_jobs_a=$(nat62_log_count "${A_LOG}" "${queue_log_a}" "${jobs_needle}")
            queue_jobs_b=$(nat62_log_count "${B_LOG}" "${queue_log_b}" "${jobs_needle}")
            queue_app_a=$(nat62_log_count "${A_LOG}" "${queue_log_a}" "${app_needle}")
            queue_app_b=$(nat62_log_count "${B_LOG}" "${queue_log_b}" "${app_needle}")
            queue_after_a=$(nat62_received "${PORT_A}" "${PROM_A}" || true)
            queue_after_b=$(nat62_received "${PORT_B}" "${PROM_B}" || true)
            if [[ -z "${queue_after_a}" || -z "${queue_after_b}" ]]; then
                queue_detail="received gauge was missing after the publish"
            else
                queue_delta_a=$((queue_after_a - queue_before_a))
                queue_delta_b=$((queue_after_b - queue_before_b))
                if (( queue_jobs_a + queue_jobs_b != 1 )); then
                    queue_detail="jobs.refresh-all logged A ${queue_jobs_a} B ${queue_jobs_b}"
                elif (( queue_jobs_a == 1 && queue_delta_a < 1 )); then
                    queue_detail="A logged the message and received moved ${queue_delta_a}"
                elif (( queue_jobs_b == 1 && queue_delta_b < 1 )); then
                    queue_detail="B logged the message and received moved ${queue_delta_b}"
                else
                    queue_ok=1
                fi
            fi
            if [[ "${queue_ok}" -ne 1 && -z "${queue_detail}" ]]; then
                queue_detail="jobs A ${queue_jobs_a} B ${queue_jobs_b}, received A ${queue_delta_a:-missing} (app ${queue_app_a}) B ${queue_delta_b:-missing} (app ${queue_app_b})"
            fi
        fi
    fi
    if [[ "${queue_ok}" -eq 1 ]]; then
        queue_slice="${DIAG_TEST_DIR}/ws_sub_after_jobs.out"
        tail -c +$((queue_mark + 1)) "${WS_SUB_OUT}" > "${queue_slice}" || true
        if nat62_ws_has "${queue_slice}" "nats_event"; then
            queue_ok=0
            queue_detail="subscribed client received a nats_event for jobs.refresh-all"
        fi
    elif [[ -z "${queue_detail}" ]]; then
        queue_detail="jobs.refresh-all was not delivered"
    fi
    if [[ "${queue_ok}" -eq 1 ]]; then
        nat62_pass "jobs.refresh-all raised received on exactly one instance"
    else
        nat62_fail "Queue group check failed (${queue_detail})"
    fi
fi

# --- 11. Reconnect ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "A reconnects after the broker returns"
if [[ -z "${HYDROGEN_A_PID}" || "${NATS62_BROKER_UP}" -ne 1 ]]; then
    nat62_fail "Instance A or the broker is not up"
elif ! nats_stop_server; then
    NATS62_BROKER_UP=0
    nat62_fail "nats-server did not release port ${PORT_NATS}"
else
    NATS62_BROKER_UP=0
    reconnect_down=0
    down_re=""
    down_up=""
    down_deadline=$((SECONDS + 8))
    while (( SECONDS <= down_deadline )); do
        if nats_scrape_prometheus "${PORT_A}" "${PROM_A}"; then
            down_re=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_reconnects_total" || true)
            down_up=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_up" || true)
            if [[ -n "${down_re}" && -n "${down_up}" ]] && (( down_re >= 1 && down_up == 0 )); then
                reconnect_down=1
                break
            fi
        fi
        sleep 0.1
    done
    if [[ "${reconnect_down}" -ne 1 ]]; then
        nat62_fail "A did not report reconnects and a down link within 8s"
    elif ! nats_start_server "${NATS_LOG}"; then
        nat62_fail "nats-server did not restart"
    else
        NATS62_BROKER_UP=1
        if ! nats_wait_gauge "${PORT_A}" "hydrogen_nats_up" eq 1 8 "${PROM_A}"; then
            nat62_fail "A did not return to hydrogen_nats_up 1 within 8s"
        else
            sub_back=0
            sub_deadline=$((SECONDS + 2))
            while (( SECONDS <= sub_deadline )); do
                subsz_code=$(curl -sS -o "${SUBSZ_FILE}" -w "%{http_code}" --connect-timeout 1 --max-time 2 \
                    "http://127.0.0.1:${PORT_MONITOR}/subsz?subs=1" || true)
                if [[ "${subsz_code}" == "200" ]] && nat62_subsz_ok "${SUBSZ_FILE}"; then
                    sub_back=1
                    break
                fi
                sleep 0.1
            done
            if [[ "${sub_back}" -eq 1 ]]; then
                nat62_pass "A reconnected, the link is up, and the four subjects are back"
            else
                nat62_fail "A returned to up but the four subjects were not subscribed"
            fi
        fi
    fi
fi

# --- 12. B stops and A forgets the peer ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stopping B clears A's peers"
if [[ -z "${HYDROGEN_B_PID}" ]]; then
    nat62_fail "Instance B is not running"
elif ! nat62_stop_quiet "${HYDROGEN_B_PID}" "${B_LOG}"; then
    EXIT_CODE=1
    HYDROGEN_B_PID=""
else
    HYDROGEN_B_PID=""
    gone_ok=0
    gone_deadline=$((SECONDS + 5))
    while (( SECONDS <= gone_deadline )); do
        if nats_scrape_prometheus "${PORT_A}" "${PROM_A}"; then
            gone_peers=$(nats_gauge_value "${PROM_A}" "hydrogen_nats_peers" || true)
            if [[ "${gone_peers}" == "0" ]]; then
                gone_ok=1
                break
            fi
        fi
        sleep 0.1
    done
    a_clean=0
    a_missing=0
    if [[ -z "${HYDROGEN_A_PID}" ]]; then
        a_missing=1
    elif nat62_stop_quiet "${HYDROGEN_A_PID}" "${A_LOG}"; then
        a_clean=1
        HYDROGEN_A_PID=""
    else
        EXIT_CODE=1
        HYDROGEN_A_PID=""
    fi
    if [[ "${NATS62_BROKER_UP}" -eq 1 ]]; then
        if ! nats_stop_server; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "nats-server did not release its ports"
        fi
        NATS62_BROKER_UP=0
    fi
    if [[ "${a_clean}" -eq 1 && "${gone_ok}" -eq 1 ]]; then
        nat62_pass "B and A shut down clean and A's peers returned to 0"
    elif [[ "${a_clean}" -eq 1 ]]; then
        nat62_fail "A's peers did not return to 0 within 5s"
    elif [[ "${a_missing}" -eq 1 ]]; then
        nat62_fail "Instance A is not running"
    fi
fi

if [[ -n "${WS_SUB_PID}" ]]; then
    nat62_stop_ws "${WS_SUB_PID}"
    WS_SUB_PID=""
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
