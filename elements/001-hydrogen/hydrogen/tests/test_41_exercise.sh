#!/usr/bin/env bash

# FUNCTIONS
# scrape_metrics()
# get_metric()
# run_auth_request()

set -euo pipefail

# Test Configuration
TEST_NAME="Exercise"
TEST_ABBR="EXE"
TEST_NUMBER="41"
TEST_COUNTER=0
TEST_VERSION="3.11.0"

TOTAL_REQUESTS=500
SNAPSHOT_INTERVAL=50
METRICS_DELAY=1  # Seconds to wait before scraping metrics
MIDPOINT_REQUEST=$(( TOTAL_REQUESTS / 2 ))  # For steady-state leak analysis (first run)
LEAK_THRESHOLD_KB_PER_REQ=1  # Max KB/request growth in second half before flagging as leak

# Prometheus scrape resilience (see scrape_metrics). Under ASAN the endpoint can momentarily fail to respond while under heavy auth load, so we retry.
SCRAPE_MAX_ATTEMPTS=5    # Total curl attempts per scrape before giving up
SCRAPE_CURL_TIMEOUT=15   # Per-attempt curl --max-time (seconds)
SCRAPE_RETRY_DELAY=2     # Seconds to wait between scrape attempts

# Extended native (non-ASAN) run parameters for memory growth observation
NATIVE_TOTAL_REQUESTS=5000
NATIVE_SNAPSHOT_INTERVAL=500
NATIVE_MIDPOINT_REQUEST=$(( NATIVE_TOTAL_REQUESTS / 2 ))  # Midpoint for native steady-state analysis

# Captured native steady-state growth (bytes/request) from the long native run. Surfaced in the footer title. Empty until the native analysis computes it.
NATIVE_STEADY_BYTES_PER_REQ=""

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Exercise configuration - uses conduit_utils.sh DATABASE_NAMES map
# DATABASE_NAMES is defined in conduit_utils.sh: PostgreSQL=Demo_PG, MySQL=Demo_MY, SQLite=Demo_SQL, DB2=Demo_DB2, MariaDB=Demo_MR, CockroachDB=Demo_CR, YugabyteDB=Demo_YB

# ── Locate Hydrogen Binary ──────────────────────────────────────────
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# ── Detect ASAN Debug Build for Thorough Leak Checks ─────────────────
# test_11 uses debug/ASAN but deliberately avoids DBs. test_41's purpose is DB/auth stress (conduit + 7 engines + 500 logins) + leak detection.
# When hydrogen_debug (ASAN-enabled) is present we launch the exercise server under it and post-process LeakSanitizer output from its log.
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Detect ASAN Debug Build"

ASAN_MODE=false
ASAN_HYDROGEN_BIN=""
# Probe common locations for the debug binary next to the regular one. We *always* resolve to an absolute path (or ./prefixed) here so that when we
# later do "export HYDROGEN_BIN=..." and conduit_utils does "${HYDROGEN_BIN}" ... in a subshell/background, it can actually exec it. Bare names like "hydrogen_debug"
# only work by luck in the test script's cwd context; they produce "command not found" in the launch context (exactly what the server log showed).
# Note on payload: the plain "hydrogen_debug" target (see cmake/CMakeLists-debug.cmake) only adds ASAN flags; it does *not* run the embed_payload.sh step that regular,
# coverage and release do. If the binary we pick lacks the appended encrypted payload, the server will fail to load PAYLOAD: resources (Swagger, migrations tagged PAYLOAD:acuranzo,
# terminal, etc.) and never print "STARTUP COMPLETE". We still allow it (with a warning) so that the long-timeout run + guarded ASAN analysis can reveal the real error in the log.
for cand in \
    "${HYDROGEN_BIN_BASE}_debug" \
    "hydrogen_debug" \
    "$(dirname "${HYDROGEN_BIN}" 2>/dev/null || echo ".")/hydrogen_debug" \
    "${PROJECT_DIR}/hydrogen_debug" \
    "build/hydrogen_debug" ; do
    if [[ -n "${cand}" ]]; then
        # Resolve to absolute (preferred) or ./relative so it is directly executable later
        local_resolved="${cand}"
        if command -v realpath >/dev/null 2>&1; then
            local_resolved=$(realpath "${cand}" 2>/dev/null || echo "${cand}")
        elif command -v readlink >/dev/null 2>&1; then
            # readlink -f is common; fall back gracefully
            local_resolved=$(readlink -f "${cand}" 2>/dev/null || echo "${cand}")
        fi
        if [[ "${local_resolved}" != /* && -f "${local_resolved}" ]]; then
            local_resolved="./${local_resolved}"
        fi
        if [[ -x "${local_resolved}" ]]; then
            if { readelf -s "${local_resolved}" || true; } | "${GREP}" -q "__asan"; then
                # Payload heuristic: look for evidence the encrypted payload was appended.
                if strings "${local_resolved}" 2>/dev/null | "${GREP}" -q -E 'PAYLOAD:|<<<< HERE BE ME TREASURE >>>' ; then
                    ASAN_HYDROGEN_BIN="${local_resolved}"
                else
                    ASAN_HYDROGEN_BIN="${local_resolved}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: ${local_resolved} has ASAN but may lack embedded payload (plain debug target skips embed_payload.sh)"
                fi
                ASAN_MODE=true
                break
            fi
        fi
    fi
done

    if [[ "${ASAN_MODE}" == true ]]; then
    # Resolve the candidate to an absolute path. The framework's find_hydrogen_binary often ends up with bare names (e.g. "hydrogen_coverage") that happen to be resolvable
    # in the test runner's context. For our explicit override to the debug binary we must ensure the value we export as HYDROGEN_BIN is directly executable by the shell in
    # run_conduit_server (which does "${HYDROGEN_BIN}" ...). Bare names fail when . is not on $PATH (common). Use REALPATH if available (provided by framework).
    if [[ -n "${REALPATH:-}" ]] && command -v "${REALPATH}" >/dev/null 2>&1; then
        ASAN_HYDROGEN_BIN=$("${REALPATH}" "${ASAN_HYDROGEN_BIN}" 2>/dev/null || echo "${ASAN_HYDROGEN_BIN}")
    else
        # Fallback: if it looks relative and exists here, make it ./relative so exec works
        if [[ "${ASAN_HYDROGEN_BIN}" != /* && -f "${ASAN_HYDROGEN_BIN}" ]]; then
            ASAN_HYDROGEN_BIN="./${ASAN_HYDROGEN_BIN}"
        fi
    fi
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN debug build found: ${ASAN_HYDROGEN_BIN}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "ASAN debug build available - exercise will run under LeakSanitizer"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No ASAN debug build found (or not executable/with __asan); RSS heuristic only"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No debug/ASAN build - using RSS growth heuristic for leak detection"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
fi

# ── Validate Environment ────────────────────────────────────────────
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment"
env_vars_valid=true

if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_NAME is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_USER_PASS:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_PASS is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_API_KEY is not set"
    env_vars_valid=false
fi

if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Environment validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
    EXIT_CODE=1
fi

# ── Validate Configuration ──────────────────────────────────────────
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Exercise Configuration"

CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_exercise.json"

if [[ -f "${CONFIG_FILE}" ]]; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${CONFIG_FILE}"; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise Configuration"
        SERVER_PORT=$(get_webserver_port "${CONFIG_FILE}")
        API_PREFIX=$(jq -r '.API.Prefix // "/api"' "${CONFIG_FILE}" 2>/dev/null || echo "/api")
        DB_COUNT=$(jq '.Databases.Connections | length' "${CONFIG_FILE}" 2>/dev/null || echo "0")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port: ${SERVER_PORT}, API prefix: ${API_PREFIX}, Databases: ${DB_COUNT}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration validated"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Config file not found: ${CONFIG_FILE}"
    EXIT_CODE=1
fi

# ── Exercise Parameters ─────────────────────────────────────────────
BASE_URL="http://localhost:${SERVER_PORT:-5410}"
PROMETHEUS_URL="${BASE_URL}${API_PREFIX:-/api}/system/prometheus"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise Parameters"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total requests: ${TOTAL_REQUESTS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Snapshot interval: every ${SNAPSHOT_INTERVAL} requests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics delay: ${METRICS_DELAY}s"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server URL: ${BASE_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Prometheus URL: ${PROMETHEUS_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Databases: ${DATABASE_NAMES[*]}"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Parameters configured"
PASS_COUNT=$(( PASS_COUNT + 1 ))

# ── Helper Functions ────────────────────────────────────────────────

# Function to scrape Prometheus metrics from the server Retries a few times because under ASAN (shadow memory + quarantine) the
# server can be momentarily unresponsive while servicing heavy auth load, producing empty/partial scrapes ("No metrics returned"). We retry with a
# short backoff and only accept a response that actually contains hydrogen_ metrics; on total failure we echo "" so callers can detect the miss.
scrape_metrics() {
    local prom_url="$1"
    local attempt response
    # Give the server a moment to update its internal stats before first try
    sleep "${METRICS_DELAY}"
    for ((attempt=1; attempt<=SCRAPE_MAX_ATTEMPTS; attempt++)); do
        response=$(curl -s --max-time "${SCRAPE_CURL_TIMEOUT}" "${prom_url}" 2>/dev/null || true)
        if [[ -n "${response}" ]] && echo "${response}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            echo "${response}"
            return 0
        fi
        # Backoff before the next attempt (skip the wait after the final try)
        if [[ "${attempt}" -lt "${SCRAPE_MAX_ATTEMPTS}" ]]; then
            sleep "${SCRAPE_RETRY_DELAY}"
        fi
    done
    # All attempts exhausted without valid hydrogen_ metrics
    echo ""
}

# Function to extract a metric value from Prometheus text format
# Handles lines like: metric_name 1234, Also handles lines with labels like: metric_name{label="value"} 12345
get_metric() {
    local metrics="$1"
    local name="$2"
    local value
    # Extract the last field from the first matching line, strip whitespace
    value=$(echo "${metrics}" | "${GREP}" "^${name}" 2>/dev/null | head -1 | awk '{print $NF}' | tr -d '[:space:]' || true)
    # If empty or not a number, default to 0
    if [[ -z "${value}" ]] || ! [[ "${value}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        echo "0"
    else
        echo "${value}"
    fi
}

# Function to run a single auth request against a specific database
run_auth_request() {
    local url="$1"
    local db_name="$2"
    local req_num="$3"

    local login_data
    if (( req_num % 2 == 0 )); then
        # Valid login
        login_data="{\"database\": \"${db_name}\", \"login_id\": \"${HYDROGEN_DEMO_USER_NAME}\", \"password\": \"${HYDROGEN_DEMO_USER_PASS}\", \"api_key\": \"${HYDROGEN_DEMO_API_KEY}\", \"tz\": \"America/Vancouver\"}"
    else
        # Invalid login (wrong password)
        login_data="{\"database\": \"${db_name}\", \"login_id\": \"${HYDROGEN_DEMO_USER_NAME}\", \"password\": \"WrongPassword123!\", \"api_key\": \"${HYDROGEN_DEMO_API_KEY}\", \"tz\": \"America/Vancouver\"}"
    fi

    curl -s -X POST -H "Content-Type: application/json" \
        -d "${login_data}" --max-time 5 --compressed \
        "${url}/api/auth/login" >/dev/null 2>&1 || true
}

# Only proceed with exercise if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    # ── Start Hydrogen Server (conduit_utils approach) ──────────────

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server"

    EXERCISE_LOG_SUFFIX="exercise"
    # LOG_PREFIX already embeds the timestamp (test_<n>_<TIMESTAMP>); do NOT append
    # ${TIMESTAMP} again or the resulting filenames carry a doubled timestamp.
    RESULT_FILE="${LOG_PREFIX}_${EXERCISE_LOG_SUFFIX}.result"
    METRICS_LOG="${LOG_PREFIX}_metrics.log"
    # Always define for shellcheck; set dedicated path only when running under ASAN debug build
    ASAN_EXERCISE_LOG=""
    if [[ "${ASAN_MODE}" == true ]]; then
        ASAN_EXERCISE_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}_asan.log"
    fi

    true > "${METRICS_LOG}"

    # If ASAN debug build is available, launch the exercise server under it so we can harvest LeakSanitizer output from its log at shutdown.
    # We still use the same port/config; we temporarily override HYDROGEN_BIN for the run_conduit_server call, then restore it.
    ORIGINAL_HYDROGEN_BIN="${HYDROGEN_BIN}"
    if [[ "${ASAN_MODE}" == true && -n "${ASAN_HYDROGEN_BIN}" ]]; then
        # Guarantee an absolute (or at worst ./ -prefixed) path for the launch. The probe above may have left a bare basename. conduit_utils does
        # "${HYDROGEN_BIN}" ... in a background subshell; bare names fail there with "command not found" unless . is in $PATH for that context.
        if [[ "${ASAN_HYDROGEN_BIN}" != /* ]]; then
            if command -v realpath >/dev/null 2>&1; then
                ASAN_HYDROGEN_BIN=$(realpath "${ASAN_HYDROGEN_BIN}" 2>/dev/null || echo "${ASAN_HYDROGEN_BIN}")
            else
                ASAN_HYDROGEN_BIN="$(pwd)/${ASAN_HYDROGEN_BIN##*/}"
            fi
        fi
        # Export for the conduit_utils server launch (it sources ${HYDROGEN_BIN})
        export HYDROGEN_BIN="${ASAN_HYDROGEN_BIN}"
        # Comprehensive ASAN options (leak_check_at_exit ensures report on SIGINT/SIGTERM shutdown)
        export ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=1:log_threads=1:print_stats=1"
        # Give ASAN-instrumented multi-DB startup (7 engines + migrations + Slow queues) plenty of time.
        # run_conduit_server and the webserver-ready loop honor these.
        export CONDUIT_STARTUP_TIMEOUT=180
        export CONDUIT_WEBSERVER_READY_TIMEOUT=180
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Launching exercise server under ASAN debug build for leak analysis"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN_OPTIONS=${ASAN_OPTIONS}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Startup timeouts extended for ASAN+7DBs (CONDUIT_STARTUP_TIMEOUT=${CONDUIT_STARTUP_TIMEOUT}, WEBSERVER_READY=${CONDUIT_WEBSERVER_READY_TIMEOUT})"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Effective HYDROGEN_BIN for launch: ${HYDROGEN_BIN}"
    fi

    # Use run_conduit_server() from conduit_utils.sh for proven startup, migration waiting, database readiness, and webserver readiness
    server_info=$(run_conduit_server "${CONFIG_FILE}" "${EXERCISE_LOG_SUFFIX}" "Exercise" "${RESULT_FILE}")

    # Check if server startup failed
    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server startup failed"
        EXIT_CODE=1
    else
        # Parse server info - format is "base_url:pid"
        # Conduit helper returns "base_url:pid" (two fields after the scheme/port). Use $NF for robustness.
        HYDROGEN_PID=$(echo "${server_info}" | awk -F: '{print $NF}')
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server started: ${server_info}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: build/tests/logs/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started successfully"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi

    # ── Build Ready Databases List ───────────────────────────────────

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Build Ready Databases List"

        # Build READY_DATABASES array from conduit_utils DATABASE_NAMES
        # check_database_readiness() was already called by run_conduit_server()
        declare -a READY_DATABASES=()
        for db_engine in "${!DATABASE_NAMES[@]}"; do
            if "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${RESULT_FILE}" 2>/dev/null; then
                READY_DATABASES+=("${DATABASE_NAMES[${db_engine}]}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Database ready: ${db_engine} (${DATABASE_NAMES[${db_engine}]})"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Database NOT ready: ${db_engine} (${DATABASE_NAMES[${db_engine}]})"
            fi
        done

        if [[ ${#READY_DATABASES[@]} -gt 0 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Databases ready: ${#READY_DATABASES[@]}/${#DATABASE_NAMES[@]}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No databases ready - cannot run exercise"
            EXIT_CODE=1
        fi
    fi

    # ── Collect Initial Metrics ─────────────────────────────────────

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Collect Initial Metrics"
        INITIAL_METRICS=$(scrape_metrics "${PROMETHEUS_URL}")

        if [[ -n "${INITIAL_METRICS}" ]] && echo "${INITIAL_METRICS}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            INITIAL_RSS=$(get_metric "${INITIAL_METRICS}" "hydrogen_process_resident_memory_bytes")
            INITIAL_QUERIES=$(get_metric "${INITIAL_METRICS}" "hydrogen_database_queries_executed_total")
            INITIAL_CONNS=$(get_metric "${INITIAL_METRICS}" "hydrogen_webserver_connections_current")
            INITIAL_REQUESTS=$(get_metric "${INITIAL_METRICS}" "hydrogen_webserver_requests_total")
            INITIAL_FDS=$(get_metric "${INITIAL_METRICS}" "hydrogen_process_open_fds")

            INITIAL_RSS_MB=$(echo "${INITIAL_RSS}" | awk '{printf "%.1f", $1/1024/1024}')

            # Log initial metrics to file only (no raw stdout)
            {
                echo "=== INITIAL METRICS ==="
                echo "Timestamp: $(date -Iseconds || true)"
                echo "RSS: ${INITIAL_RSS} bytes (${INITIAL_RSS_MB} MB)"
                echo "Queries: ${INITIAL_QUERIES}"
                echo "Connections: ${INITIAL_CONNS}"
                echo "HTTP Requests: ${INITIAL_REQUESTS}"
                echo "File Descriptors: ${INITIAL_FDS}"
                echo ""
            } >> "${METRICS_LOG}"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Initial RSS: ${INITIAL_RSS_MB} MB, Queries: ${INITIAL_QUERIES}, FDs: ${INITIAL_FDS}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Initial metrics collected"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: Prometheus endpoint returned no hydrogen metrics"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "URL: ${PROMETHEUS_URL}"
            # shellcheck disable=SC2310 # We want to see the response for debugging
            raw_response=$(curl -s --max-time 5 "${PROMETHEUS_URL}" 2>/dev/null | head -5 || echo "(empty)")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response preview: ${raw_response}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to collect initial metrics"
            INITIAL_RSS=0
            INITIAL_QUERIES=0
            INITIAL_CONNS=0
            INITIAL_REQUESTS=0
            INITIAL_FDS=0
            EXIT_CODE=1
        fi
    fi

    # ── Run Auth Exercise ───────────────────────────────────────────

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Run Auth Exercise"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running ${TOTAL_REQUESTS} auth requests across ${#READY_DATABASES[@]} ready databases..."
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using: ${READY_DATABASES[*]}"

        {
            echo "=== EXERCISE SNAPSHOTS ==="
            printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                "Request" "RSS_MB" "Delta_MB" "Queries" "Conns" "HTTP_Req" "FDs" "PrepCache" "InFlight" "PrepEvict" "Ctx" "DBCr" "DBCl" "PHit" "PMis"
        } >> "${METRICS_LOG}"

        DB_COUNT=${#READY_DATABASES[@]}
        REQUEST_COUNT=0
        FAILED_REQUESTS=0
        MIDPOINT_RSS=0  # Will capture RSS at midpoint for steady-state analysis

        for ((i=1; i<=TOTAL_REQUESTS; i++)); do
            # Round-robin across databases
            db_index=$(( (i - 1) % DB_COUNT ))
            db_name="${READY_DATABASES[${db_index}]}"

            run_auth_request "${BASE_URL}" "${db_name}" "${i}"
            REQUEST_COUNT=$((REQUEST_COUNT + 1))

            # Scrape metrics at snapshot intervals
            if (( REQUEST_COUNT % SNAPSHOT_INTERVAL == 0 )); then
                METRICS=$(scrape_metrics "${PROMETHEUS_URL}")
                if [[ -n "${METRICS}" ]] && echo "${METRICS}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
                    RSS=$(get_metric "${METRICS}" "hydrogen_process_resident_memory_bytes")
                    QUERIES=$(get_metric "${METRICS}" "hydrogen_database_queries_executed_total")
                    CONNS=$(get_metric "${METRICS}" "hydrogen_webserver_connections_current")
                    REQUESTS_TOTAL=$(get_metric "${METRICS}" "hydrogen_webserver_requests_total")
                    FDS=$(get_metric "${METRICS}" "hydrogen_process_open_fds")
                    PREP_CACHE=$(get_metric "${METRICS}" "hydrogen_database_prepared_statements_cached")
                    IN_FLIGHT=$(get_metric "${METRICS}" "hydrogen_http_requests_in_flight")
                    PREP_EVICT=$(get_metric "${METRICS}" "hydrogen_database_prepared_statements_evicted")
                    API_CTX=$(get_metric "${METRICS}" "hydrogen_http_contexts_current")
                    DB_CONNS_CREATED=$(get_metric "${METRICS}" "hydrogen_database_connections_created")
                    DB_CONNS_CLOSED=$(get_metric "${METRICS}" "hydrogen_database_connections_closed")
                    PREP_HITS=$(get_metric "${METRICS}" "hydrogen_database_prepared_statement_cache_hits")
                    PREP_MISSES=$(get_metric "${METRICS}" "hydrogen_database_prepared_statement_cache_misses")

                    RSS_MB=$(echo "${RSS}" | awk '{printf "%.1f", $1/1024/1024}')
                    RSS_DELTA=$(echo "${RSS} ${INITIAL_RSS}" | awk '{printf "%.1f", ($1-$2)/1024/1024}')

                    # Capture midpoint RSS for steady-state analysis. Use the first successful snapshot at or past the midpoint (">=" not "==") and
                    # require a real reading, so one dropped scrape at the exact midpoint request cannot silently disable the steady-state verdict later.
                    if [[ "${MIDPOINT_RSS}" -eq 0 && "${REQUEST_COUNT}" -ge "${MIDPOINT_REQUEST}" && "${RSS}" -gt 0 ]]; then
                        MIDPOINT_RSS="${RSS}"
                    fi

                    printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                        "[${REQUEST_COUNT}/${TOTAL_REQUESTS}]" \
                        "${RSS_MB}" "${RSS_DELTA}" "${QUERIES}" \
                        "${CONNS}" "${REQUESTS_TOTAL}" "${FDS}" "${PREP_CACHE}" "${IN_FLIGHT}" "${PREP_EVICT}" "${API_CTX}" \
                        "${DB_CONNS_CREATED}" "${DB_CONNS_CLOSED}" "${PREP_HITS}" "${PREP_MISSES}" \
                        >> "${METRICS_LOG}"

                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                        "[${REQUEST_COUNT}/${TOTAL_REQUESTS}] RSS: ${RSS_MB}MB (Δ${RSS_DELTA}MB) | Queries: ${QUERIES} | FDs: ${FDS} | PrepCache: ${PREP_CACHE} | InFlight: ${IN_FLIGHT} | Ctx: ${API_CTX} | DBConns: ${DB_CONNS_CREATED}/${DB_CONNS_CLOSED} | PrepHits/Miss: ${PREP_HITS}/${PREP_MISSES}"
                else
                    FAILED_REQUESTS=$((FAILED_REQUESTS + 1))
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                        "[${REQUEST_COUNT}/${TOTAL_REQUESTS}] WARNING: No metrics returned"
                fi
            fi
        done

        echo "REQUESTS_COMPLETED=${REQUEST_COUNT}" >> "${RESULT_FILE}"
        echo "FAILED_METRICS_SCRAPES=${FAILED_REQUESTS}" >> "${RESULT_FILE}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Exercise completed (${REQUEST_COUNT} requests)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi

    # ── Collect Final Metrics ───────────────────────────────────────

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Collect Final Metrics"

        # Extra delay before final snapshot for accurate stats
        sleep 2
        FINAL_METRICS=$(scrape_metrics "${PROMETHEUS_URL}")

        if [[ -n "${FINAL_METRICS}" ]] && echo "${FINAL_METRICS}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            FINAL_RSS=$(get_metric "${FINAL_METRICS}" "hydrogen_process_resident_memory_bytes")
            FINAL_QUERIES=$(get_metric "${FINAL_METRICS}" "hydrogen_database_queries_executed_total")
            FINAL_CONNS=$(get_metric "${FINAL_METRICS}" "hydrogen_webserver_connections_current")
            FINAL_REQUESTS=$(get_metric "${FINAL_METRICS}" "hydrogen_webserver_requests_total")
            FINAL_FDS=$(get_metric "${FINAL_METRICS}" "hydrogen_process_open_fds")
            FINAL_POST=$(get_metric "${FINAL_METRICS}" "hydrogen_http_post_requests_total")
            FINAL_API=$(get_metric "${FINAL_METRICS}" "hydrogen_http_api_requests_total")
            FINAL_BYTES_IN=$(get_metric "${FINAL_METRICS}" "hydrogen_http_request_bytes_received")
            FINAL_BYTES_OUT=$(get_metric "${FINAL_METRICS}" "hydrogen_http_request_bytes_sent")

            # Calculate deltas
            RSS_GROWTH=$(echo "${FINAL_RSS} ${INITIAL_RSS}" | awk '{print $1 - $2}')
            RSS_GROWTH_KB=$(echo "${RSS_GROWTH}" | awk '{printf "%.1f", $1/1024}')
            RSS_GROWTH_MB=$(echo "${RSS_GROWTH}" | awk '{printf "%.1f", $1/1024/1024}')
            FINAL_RSS_MB=$(echo "${FINAL_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
            QUERY_COUNT=$(echo "${FINAL_QUERIES} ${INITIAL_QUERIES}" | awk '{printf "%d", $1 - $2}')

            # Log final metrics to file only (no raw stdout)
            {
                echo ""
                echo "=== FINAL METRICS ==="
                echo "Timestamp: $(date -Iseconds || true)"
                echo "RSS: ${FINAL_RSS} bytes (${FINAL_RSS_MB} MB)"
                echo "Queries: ${FINAL_QUERIES}"
                echo "Connections: ${FINAL_CONNS}"
                echo "HTTP Requests: ${FINAL_REQUESTS}"
                echo "File Descriptors: ${FINAL_FDS}"
                echo "POST Requests: ${FINAL_POST}"
                echo "API Requests: ${FINAL_API}"
                echo "Bytes Received: ${FINAL_BYTES_IN}"
                echo "Bytes Sent: ${FINAL_BYTES_OUT}"
                echo ""
                echo "=== SUMMARY ==="
                echo "Total auth requests: ${REQUEST_COUNT:-0}"
                echo "Database queries (delta): ${QUERY_COUNT}"
                echo "Memory growth: ${RSS_GROWTH_KB} KB (${RSS_GROWTH_MB} MB)"
                echo "Initial RSS: ${INITIAL_RSS} bytes"
                echo "Final RSS: ${FINAL_RSS} bytes"
                echo "File descriptor change: ${INITIAL_FDS} -> ${FINAL_FDS}"
            } >> "${METRICS_LOG}"

            # Safe integer conversion for comparison (strip non-digit chars)
            query_count_check="${QUERY_COUNT//[!0-9-]/}"
            query_count_check="${query_count_check:-0}"
            if [[ "${query_count_check}" -gt 0 ]]; then
                GROWTH_PER_100=$(echo "${RSS_GROWTH} ${QUERY_COUNT}" | awk '{if ($2>0) printf "%.1f", $1*100/$2/1024; else print "0"}')
                echo "Growth per 100 queries: ${GROWTH_PER_100} KB" >> "${METRICS_LOG}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Growth per 100 queries: ${GROWTH_PER_100} KB"
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Final RSS: ${FINAL_RSS_MB} MB | Queries: ${FINAL_QUERIES} | HTTP: ${FINAL_REQUESTS} | POST: ${FINAL_POST} | FDs: ${FINAL_FDS}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "RSS growth: ${RSS_GROWTH_MB} MB over ${REQUEST_COUNT:-0} requests"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Final metrics collected"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to collect final metrics"
            EXIT_CODE=1
        fi
    fi

    # ── Memory Leak Detection ───────────────────────────────────────

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Memory Leak Detection"

        # Analyze steady-state growth: compare second-half growth to first-half
        # First half (warmup): initial → midpoint includes cache warming, JIT, etc.
        # Second half (steady): midpoint → final should show minimal growth if no leaks
        if [[ "${MIDPOINT_RSS}" -gt 0 ]] && [[ "${FINAL_RSS}" -gt 0 ]]; then
            SECOND_HALF_REQUESTS=$(( TOTAL_REQUESTS - MIDPOINT_REQUEST ))
            SECOND_HALF_GROWTH_BYTES=$(echo "${FINAL_RSS} ${MIDPOINT_RSS}" | awk '{printf "%d", $1 - $2}')
            SECOND_HALF_GROWTH_KB=$(echo "${SECOND_HALF_GROWTH_BYTES}" | awk '{printf "%.1f", $1/1024}')
            FIRST_HALF_GROWTH_KB=$(echo "${MIDPOINT_RSS} ${INITIAL_RSS}" | awk '{printf "%.1f", ($1-$2)/1024}')

            # Calculate per-request growth in second half (KB)
            STEADY_GROWTH_PER_REQ_KB="0"
            if [[ "${SECOND_HALF_REQUESTS}" -gt 0 ]]; then
                STEADY_GROWTH_PER_REQ_KB=$(echo "${SECOND_HALF_GROWTH_BYTES} ${SECOND_HALF_REQUESTS}" | awk '{printf "%.2f", $1/$2/1024}')
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Warmup growth (first ${MIDPOINT_REQUEST} reqs): ${FIRST_HALF_GROWTH_KB} KB"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state growth (last ${SECOND_HALF_REQUESTS} reqs): ${SECOND_HALF_GROWTH_KB} KB"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state rate: ${STEADY_GROWTH_PER_REQ_KB} KB/request (threshold: ${LEAK_THRESHOLD_KB_PER_REQ} KB/req)"

            {
                echo ""
                echo "=== LEAK ANALYSIS ==="
                echo "Warmup growth (first ${MIDPOINT_REQUEST} reqs): ${FIRST_HALF_GROWTH_KB} KB"
                echo "Steady-state growth (last ${SECOND_HALF_REQUESTS} reqs): ${SECOND_HALF_GROWTH_KB} KB"
                echo "Steady-state rate: ${STEADY_GROWTH_PER_REQ_KB} KB/request"
                echo "Threshold: ${LEAK_THRESHOLD_KB_PER_REQ} KB/request"
            } >> "${METRICS_LOG}"

            # Compare against threshold using awk for float comparison
            leak_detected=$(echo "${STEADY_GROWTH_PER_REQ_KB} ${LEAK_THRESHOLD_KB_PER_REQ}" | awk '{if ($1 > $2) print "yes"; else print "no"}')

            if [[ "${leak_detected}" == "yes" ]]; then
                if [[ "${ASAN_MODE}" == true ]]; then
                    # Under ASAN, high RSS growth is expected (shadow memory, quarantine, redzones, instrumented allocator).
                    # LeakSanitizer is the authoritative check; RSS heuristic is secondary/diagnostic only.
                    # Do not fail the test on RSS alone when ASAN reports clean (prevents false-positive leak reports).
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state rate ${STEADY_GROWTH_PER_REQ_KB} KB/req exceeds heuristic, but running under ASAN (shadow/quarantine overhead expected); deferring to LeakSanitizer result"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "RSS heuristic high under ASAN (informational; ASAN is authoritative)"
                    echo "LEAK_DETECTED=false" >> "${RESULT_FILE}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Memory leak suspected: ${STEADY_GROWTH_PER_REQ_KB} KB/req exceeds ${LEAK_THRESHOLD_KB_PER_REQ} KB/req threshold"
                    echo "LEAK_DETECTED=true" >> "${RESULT_FILE}"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No leak detected: ${STEADY_GROWTH_PER_REQ_KB} KB/req within ${LEAK_THRESHOLD_KB_PER_REQ} KB/req threshold"
                echo "LEAK_DETECTED=false" >> "${RESULT_FILE}"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Insufficient data for leak analysis (midpoint RSS not captured)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Leak analysis skipped (insufficient data)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        fi
    fi

    # ── Shutdown Server ─────────────────────────────────────────────

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown Server"
    if [[ -n "${HYDROGEN_PID:-}" ]]; then
        shutdown_conduit_server "${HYDROGEN_PID}" "${RESULT_FILE}"
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server stopped"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    # ── ASAN Leak Analysis (if we ran under hydrogen_debug) ─────────
    # The ASAN log is the same exercise log (ASAN writes to stderr which we
    # redirected into the server log). We saved a dedicated copy above for
    # clarity and will scan it for LeakSanitizer summaries.
    if [[ "${ASAN_MODE}" == true ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN LeakSanitizer Analysis"

        # Restore original binary for any subsequent tooling (not strictly needed here)
        export HYDROGEN_BIN="${ORIGINAL_HYDROGEN_BIN}"

         # Only attempt "no leaks" success if the server actually started and
         # the exercise phase was reached. A startup failure under ASAN should
         # not be reported as "no leaks detected".
         if [[ "${server_info}" == "FAILED:0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping ASAN leak analysis: server did not start successfully under ASAN debug build"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "ASAN: server startup failed under debug build - no leak data produced"
            # Do not increment PASS; keep failure visible. Do not force EXIT_CODE here;
            # the earlier "Server startup failed" already set EXIT_CODE=1.
        else
            # Prefer dedicated ASAN log if present; else fall back to regular server log
            ASAN_LOG_CANDIDATE="${ASAN_EXERCISE_LOG}"
            if [[ ! -f "${ASAN_LOG_CANDIDATE}" ]]; then
                ASAN_LOG_CANDIDATE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
            fi

            if [[ -f "${ASAN_LOG_CANDIDATE}" ]]; then
                # Copy for artifacts
                cp "${ASAN_LOG_CANDIDATE}" "${METRICS_LOG%.log}_asan.log" 2>/dev/null || true

                DIRECT_LEAKS=$("${GREP}" -c "Direct leak of" "${ASAN_LOG_CANDIDATE}" 2>/dev/null | head -1 || echo "0" || true)
                INDIRECT_LEAKS=$("${GREP}" -c "Indirect leak of" "${ASAN_LOG_CANDIDATE}" 2>/dev/null | head -1 || echo "0" || true)

                DIRECT_LEAKS=$(echo "${DIRECT_LEAKS}" | tr -d '\n\r' | "${GREP}" -o '[0-9]*' | head -1 || true)
                INDIRECT_LEAKS=$(echo "${INDIRECT_LEAKS}" | tr -d '\n\r' | "${GREP}" -o '[0-9]*' | head -1 || true)
                [[ -z "${DIRECT_LEAKS}" ]] && DIRECT_LEAKS=0
                [[ -z "${INDIRECT_LEAKS}" ]] && INDIRECT_LEAKS=0

                {
                    echo ""
                    echo "=== ASAN LEAKSANITIZER SUMMARY ==="
                    echo "Direct leaks: ${DIRECT_LEAKS}"
                    echo "Indirect leaks: ${INDIRECT_LEAKS}"
                    echo "Log: ${ASAN_LOG_CANDIDATE}"
                } >> "${METRICS_LOG}"

                if [[ "${DIRECT_LEAKS}" -gt 0 || "${INDIRECT_LEAKS}" -gt 0 ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN: Direct=${DIRECT_LEAKS}, Indirect=${INDIRECT_LEAKS}"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "ASAN detected leaks: ${DIRECT_LEAKS} direct, ${INDIRECT_LEAKS} indirect (see ${ASAN_LOG_CANDIDATE})"
                    EXIT_CODE=1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN: no leaks reported (Direct=0, Indirect=0)"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "ASAN: no leaks detected in exercise run"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "ASAN log not found for leak analysis (expected at ${ASAN_LOG_CANDIDATE})"
                EXIT_CODE=1
            fi
        fi
    fi

    # ── Native Release RSS Measurement ──────────────────────────────
    # Long native (non-ASAN) memory-growth run using hydrogen_release. RSS growth numbers here are authoritative for leak detection because
    # there is no shadow memory / quarantine overhead. Runs after ASAN shutdown. Separate metrics log for comparison. Always attempt
    # (diagnostic); previous EXIT_CODE from ASAN leaks does not block it.
    # This is a single subtest following harness norms: one TEST header, INFO lines for progress/diagnostics, and exactly one terminal PASS/FAIL per code path.
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Native RSS Measurement (${NATIVE_TOTAL_REQUESTS} requests)"

    # Locate a non-ASAN release binary (hydrogen_release preferred)
    RELEASE_BIN=""
    for cand in \
        "${HYDROGEN_BIN_BASE}_release" \
        "hydrogen_release" \
        "$(dirname "${HYDROGEN_BIN}" 2>/dev/null || echo ".")/hydrogen_release" \
        "${PROJECT_DIR}/hydrogen_release" \
        "build/hydrogen_release" ; do
        if [[ -n "${cand}" && -x "${cand}" ]]; then
            local_resolved="${cand}"
            if command -v realpath >/dev/null 2>&1; then
                local_resolved=$(realpath "${cand}" 2>/dev/null || echo "${cand}")
            fi
            if [[ "${local_resolved}" != /* && -f "${local_resolved}" ]]; then
                local_resolved="./${local_resolved}"
            fi
            if [[ -x "${local_resolved}" ]]; then
                if ! { readelf -s "${local_resolved}" || true; } | "${GREP}" -q "__asan" 2>/dev/null; then
                    RELEASE_BIN="${local_resolved}"
                    break
                fi
            fi
        fi
    done

    if [[ -z "${RELEASE_BIN}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "hydrogen_release not found or is ASAN-instrumented; skipping native RSS run"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Native RSS measurement skipped (no release binary)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting native RSS measurement with: ${RELEASE_BIN}"

        NATIVE_SUFFIX="exercise_native"
        # LOG_PREFIX already embeds the timestamp; avoid doubling it.
        NATIVE_RESULT="${LOG_PREFIX}_${NATIVE_SUFFIX}.result"
        NATIVE_METRICS="${LOG_PREFIX}_metrics_native.log"
        true > "${NATIVE_METRICS}"

        SAVED_HYDROGEN_BIN="${HYDROGEN_BIN}"
        export HYDROGEN_BIN="${RELEASE_BIN}"
        unset ASAN_OPTIONS 2>/dev/null || true

        # For the long native RSS measurement run we do not need TRACE/DEBUG spam.
        # STATE is sufficient for startup/migration messages that check_database_readiness needs.
        # This keeps the native metrics log reasonable in size (the 5000-iter run otherwise
        # produces hundreds of MB to GB of trace output) and removes logging I/O from the measurement.
        export HYDROGEN_LOG_LEVEL=STATE

        native_info=$(run_conduit_server "${CONFIG_FILE}" "${NATIVE_SUFFIX}" "Exercise-Native" "${NATIVE_RESULT}")

        if [[ "${native_info}" == "FAILED:0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping native RSS data collection"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Native server startup failed"
        else
            NATIVE_PID=$(echo "${native_info}" | awk -F: '{print $NF}')
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native server started: ${native_info}"

            # Rebuild ready list from this native instance
            declare -a NATIVE_DBS=()
            for db_engine in "${!DATABASE_NAMES[@]}"; do
                if "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${NATIVE_RESULT}" 2>/dev/null; then
                    NATIVE_DBS+=("${DATABASE_NAMES[${db_engine}]}")
                fi
            done

            N_INIT=$(scrape_metrics "${PROMETHEUS_URL}")
            N_INIT_RSS=$(get_metric "${N_INIT}" "hydrogen_process_resident_memory_bytes")
            N_INIT_Q=$(get_metric "${N_INIT}" "hydrogen_database_queries_executed_total")
            N_INIT_F=$(get_metric "${N_INIT}" "hydrogen_process_open_fds")
            N_INIT_MB=$(echo "${N_INIT_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native initial RSS: ${N_INIT_MB} MB, Queries: ${N_INIT_Q}, FDs: ${N_INIT_F}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native total requests: ${NATIVE_TOTAL_REQUESTS}, snapshot every ${NATIVE_SNAPSHOT_INTERVAL}"

            {
                echo "=== NATIVE RELEASE EXERCISE (hydrogen_release) ==="
                echo "Binary: ${RELEASE_BIN}"
                echo "Initial RSS: ${N_INIT_RSS} bytes (${N_INIT_MB} MB)"
                echo "Initial Queries: ${N_INIT_Q}"
                echo ""
                printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                    "Request" "RSS_MB" "Delta_MB" "Queries" "Conns" "HTTP_Req" "FDs" "PrepCache" "InFlight" "PrepEvict" "Ctx" "DBCr" "DBCl" "PHit" "PMis"
            } >> "${NATIVE_METRICS}"

            N_DB_CNT=${#NATIVE_DBS[@]}
            N_CNT=0
            N_MID=0

            # Server-health / measurement-quality tracking. A failed scrape is NOT the
            # same as "RSS == 0"; get_metric() returns 0 on a miss, which previously
            # silently corrupted the final growth math (e.g. "Growth: -77.0 MB") and
            # skipped the verdict ("no midpoint"). We now track the last GOOD snapshot
            # and count consecutive scrape failures so we can detect a server that has
            # crashed or hung under load and report that explicitly as a FAILURE.
            N_LAST_GOOD_RSS=0        # RSS (bytes) from the most recent successful scrape
            N_LAST_GOOD_CNT=0        # request count at that successful scrape
            N_SNAPSHOTS_OK=0         # number of successful periodic snapshots
            N_SNAPSHOTS_FAIL=0       # number of failed periodic snapshots
            N_CONSEC_FAILS=0         # consecutive failed periodic snapshots (reset on success)
            N_SERVER_DIED=false      # set true when the server stops responding mid-run
            # Abort the (long) native loop once the server is clearly unresponsive rather
            # than hammering a dead/hung process for thousands more requests. Two
            # consecutive missed snapshots (with SCRAPE_MAX_ATTEMPTS retries each) is a
            # strong signal the server is gone.
            N_MAX_CONSEC_FAILS=2

            for ((i=1; i<=NATIVE_TOTAL_REQUESTS; i++)); do
                ndx=$(( (i-1) % N_DB_CNT ))
                dbn="${NATIVE_DBS[${ndx}]}"
                run_auth_request "${BASE_URL}" "${dbn}" "${i}"
                N_CNT=$((N_CNT+1))

                if (( N_CNT % NATIVE_SNAPSHOT_INTERVAL == 0 )); then
                    NM=$(scrape_metrics "${PROMETHEUS_URL}")
                    if [[ -n "${NM}" ]] && echo "${NM}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
                        N_CONSEC_FAILS=0
                        N_SNAPSHOTS_OK=$(( N_SNAPSHOTS_OK + 1 ))
                        NR=$(get_metric "${NM}" "hydrogen_process_resident_memory_bytes")
                        NQ=$(get_metric "${NM}" "hydrogen_database_queries_executed_total")
                        # NB: do NOT name this "NC" - that is the framework's global
                        # No-Color reset escape (\033[0;37m). Reusing it here clobbered
                        # every subsequent line's color reset with the connections count,
                        # producing the stray leading/trailing digits and "DATA1"/"INFO2"
                        # label corruption in the native subtest output.
                        NCONN=$(get_metric "${NM}" "hydrogen_webserver_connections_current")
                        NRT=$(get_metric "${NM}" "hydrogen_webserver_requests_total")
                        NF=$(get_metric "${NM}" "hydrogen_process_open_fds")
                        NP=$(get_metric "${NM}" "hydrogen_database_prepared_statements_cached")
                        NI=$(get_metric "${NM}" "hydrogen_http_requests_in_flight")
                        NPE=$(get_metric "${NM}" "hydrogen_database_prepared_statements_evicted")
                        NAC=$(get_metric "${NM}" "hydrogen_http_contexts_current")
                        NDC=$(get_metric "${NM}" "hydrogen_database_connections_created")
                        NCL=$(get_metric "${NM}" "hydrogen_database_connections_closed")
                        NPH=$(get_metric "${NM}" "hydrogen_database_prepared_statement_cache_hits")
                        NPM=$(get_metric "${NM}" "hydrogen_database_prepared_statement_cache_misses")

                        NR_MB=$(echo "${NR}" | awk '{printf "%.1f", $1/1024/1024}')
                        ND=$(echo "${NR} ${N_INIT_RSS}" | awk '{printf "%.1f", ($1-$2)/1024/1024}')

                        # Remember the last successful RSS/count so final growth and
                        # steady-state analysis can use real measured data even if the
                        # very last (post-loop) scrape misses.
                        if [[ "${NR}" -gt 0 ]]; then
                            N_LAST_GOOD_RSS="${NR}"
                            N_LAST_GOOD_CNT="${N_CNT}"
                        fi

                        # Capture the midpoint RSS from the first successful snapshot at or
                        # past the midpoint request. Keying off ">=" (not exact equality)
                        # means a single missed midpoint snapshot no longer disables the
                        # entire steady-state verdict.
                        if [[ "${N_MID}" -eq 0 && "${N_CNT}" -ge "${NATIVE_MIDPOINT_REQUEST}" && "${NR}" -gt 0 ]]; then
                            N_MID="${NR}"
                        fi

                        printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                            "[${N_CNT}/${NATIVE_TOTAL_REQUESTS}]" \
                            "${NR_MB}" "${ND}" "${NQ}" "${NCONN}" "${NRT}" "${NF}" "${NP}" "${NI}" "${NPE}" "${NAC}" \
                            "${NDC}" "${NCL}" "${NPH}" "${NPM}" \
                            >> "${NATIVE_METRICS}"

                        # Periodic scraped metrics are data, not status: emit as DATA
                        # via print_output (same idiom as other tests dumping captured
                        # values), letting the framework collect/render it.
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" \
                            "[NATIVE ${N_CNT}/${NATIVE_TOTAL_REQUESTS}] RSS: ${NR_MB}MB (Δ${ND}MB) | Queries: ${NQ} | FDs: ${NF} | PrepCache: ${NP} | InFlight: ${NI} | Ctx: ${NAC} | DBConns: ${NDC}/${NCL} | PrepHits/Miss: ${NPH}/${NPM}"
                    else
                        # Scrape failed at this snapshot. Under sustained auth load the
                        # server can hang (observed: SQLite DQM stalling on tokens.valid_until
                        # errors), after which every scrape returns empty. Track this and
                        # bail out early instead of firing thousands of requests at a dead
                        # process and then reporting a bogus negative "growth".
                        N_SNAPSHOTS_FAIL=$(( N_SNAPSHOTS_FAIL + 1 ))
                        N_CONSEC_FAILS=$(( N_CONSEC_FAILS + 1 ))
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                            "[NATIVE ${N_CNT}/${NATIVE_TOTAL_REQUESTS}] WARNING: metrics scrape failed (consecutive failures: ${N_CONSEC_FAILS})"
                        {
                            echo "[${N_CNT}/${NATIVE_TOTAL_REQUESTS}] SCRAPE FAILED (consecutive: ${N_CONSEC_FAILS})"
                        } >> "${NATIVE_METRICS}"
                        if [[ "${N_CONSEC_FAILS}" -ge "${N_MAX_CONSEC_FAILS}" ]]; then
                            N_SERVER_DIED=true
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                                "Native server unresponsive after ${N_CNT} requests (${N_CONSEC_FAILS} consecutive failed scrapes) - aborting native measurement"
                            break
                        fi
                    fi
                fi
            done

            sleep 2
            NFN=$(scrape_metrics "${PROMETHEUS_URL}")
            if [[ -n "${NFN}" ]] && echo "${NFN}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
                NF_RSS=$(get_metric "${NFN}" "hydrogen_process_resident_memory_bytes")
                NF_Q=$(get_metric "${NFN}" "hydrogen_database_queries_executed_total")
                NF_F=$(get_metric "${NFN}" "hydrogen_process_open_fds")
            else
                # Final scrape missed. Do NOT treat this as RSS=0 (that produced the
                # infamous "-77.0 MB" growth). Fall back to the last good snapshot if we
                # have one; otherwise leave RSS at 0 and let the health check below FAIL.
                NF_RSS="${N_LAST_GOOD_RSS}"
                NF_Q=0
                NF_F=0
                if [[ "${NF_RSS}" -gt 0 ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Final native scrape missed; using last good snapshot at request ${N_LAST_GOOD_CNT} for final RSS"
                fi
            fi
            NF_MB=$(echo "${NF_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
            NG_MB=$(echo "${NF_RSS} ${N_INIT_RSS}" | awk '{printf "%.1f", ($1-$2)/1024/1024}')

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native final RSS: ${NF_MB} MB | Growth: ${NG_MB} MB over ${N_CNT} requests (target ${NATIVE_TOTAL_REQUESTS})"

            N_RESPONSIVE_NOTE="yes"
            if [[ "${N_SERVER_DIED}" == true ]]; then
                N_RESPONSIVE_NOTE="NO (unresponsive after ${N_CNT} requests)"
            fi
            {
                echo ""
                echo "=== NATIVE FINAL ==="
                echo "Final RSS: ${NF_RSS} bytes (${NF_MB} MB)"
                echo "Growth: ${NG_MB} MB"
                echo "Queries: ${NF_Q}  FDs: ${NF_F}"
                echo "Successful snapshots: ${N_SNAPSHOTS_OK}  Failed snapshots: ${N_SNAPSHOTS_FAIL}"
                echo "Server responsive throughout: ${N_RESPONSIVE_NOTE}"
            } >> "${NATIVE_METRICS}"

            # Stop the native server now so the subtest's terminal PASS/FAIL
            # (the steady-state verdict below) is the last line of the subtest.
            if [[ -n "${NATIVE_PID:-}" ]]; then
                shutdown_conduit_server "${NATIVE_PID}" "${NATIVE_RESULT}"
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native server stopped"

            # Server health is the first-class verdict for this subtest. A server that
            # stops responding to Prometheus under sustained auth load is a real defect
            # (not an "inconclusive" measurement), so report it as a FAIL with the point
            # of failure. This is what surfaced the SQLite tokens.valid_until hang.
            if [[ "${N_SERVER_DIED}" == true ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server hung/crashed under load; check ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${NATIVE_SUFFIX}.log"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Native server became unresponsive after ${N_CNT} requests (${N_SNAPSHOTS_OK} good / ${N_SNAPSHOTS_FAIL} failed snapshots)"
                echo "NATIVE_SERVER_DIED=true" >> "${NATIVE_RESULT}"
                echo "NATIVE_LAST_RESPONSIVE_REQUEST=${N_LAST_GOOD_CNT}" >> "${NATIVE_RESULT}"
                EXIT_CODE=1
            # Guard the analysis: require BOTH a captured midpoint and a valid final RSS.
            elif [[ "${N_MID}" -gt 0 && "${NF_RSS}" -gt 0 ]]; then
                NSH=$(( NATIVE_TOTAL_REQUESTS - NATIVE_MIDPOINT_REQUEST ))
                NSG=$(echo "${NF_RSS} ${N_MID}" | awk '{printf "%d", $1 - $2}')
                NSK=$(echo "${NSG} ${NSH}" | awk '{if ($2>0) printf "%.2f", $1/$2/1024; else print "0"}')
                # Native steady-state growth in *bytes* per request (last NSH reqs of the
                # 5000-request native run). Used for the footer title. Rounded to integer B.
                NSB=$(echo "${NSG} ${NSH}" | awk '{if ($2>0) printf "%d", $1/$2; else print "0"}')
                NATIVE_STEADY_BYTES_PER_REQ="${NSB}"
                NFH=$(echo "${N_MID} ${N_INIT_RSS}" | awk '{printf "%.1f", ($1-$2)/1024}')

                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native warmup (first ${NATIVE_MIDPOINT_REQUEST} reqs): ${NFH} KB"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native steady-state (last ${NSH} reqs): ${NSK} KB/request (${NSB} B/req) (threshold: ${LEAK_THRESHOLD_KB_PER_REQ})"

                {
                    echo ""
                    echo "=== NATIVE LEAK ANALYSIS ==="
                    echo "Warmup (first ${NATIVE_MIDPOINT_REQUEST} reqs): ${NFH} KB"
                    echo "Steady-state (last ${NSH} reqs): ${NSK} KB/request"
                    echo "Threshold: ${LEAK_THRESHOLD_KB_PER_REQ}"
                } >> "${NATIVE_METRICS}"

                nleak=$(echo "${NSK} ${LEAK_THRESHOLD_KB_PER_REQ}" | awk '{if ($1 > $2) print "yes"; else print "no"}')
                if [[ "${nleak}" == "yes" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Native memory growth: ${NSK} KB/req exceeds ${LEAK_THRESHOLD_KB_PER_REQ} KB/req"
                    echo "NATIVE_LEAK_DETECTED=true" >> "${NATIVE_RESULT}"
                    # Diagnostic run; do not force overall EXIT_CODE=1 here.
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Native growth within threshold: ${NSK} KB/req"
                    echo "NATIVE_LEAK_DETECTED=false" >> "${NATIVE_RESULT}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                fi
            else
                # Server stayed responsive but we still lack usable midpoint/final RSS.
                # This means the measurement itself did not produce enough data (e.g. too
                # few successful snapshots), which is a data-quality failure - report it
                # honestly rather than passing on missing data.
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Insufficient native data for analysis (good snapshots: ${N_SNAPSHOTS_OK}, midpoint RSS: ${N_MID}, final RSS: ${NF_RSS})"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Native measurement incomplete - could not compute steady-state growth"
                echo "NATIVE_MEASUREMENT_INCOMPLETE=true" >> "${NATIVE_RESULT}"
                EXIT_CODE=1
            fi
        fi

        export HYDROGEN_BIN="${SAVED_HYDROGEN_BIN}"
    fi

    # ── Print Summary ───────────────────────────────────────────────

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise complete!"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics log: ${METRICS_LOG}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"

else
    # Skip exercise if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping exercise due to prerequisite failures"
fi

# Augment the footer title with the measured native steady-state growth rate
# (bytes/request) from the long 5000-request native run, e.g.
#   41-EXE | Exercise  growth: 1,512 B/req
# The {BLUE}...{RESET} markers are rendered by the tables engine for color,
# matching the footer-title convention used by tests 30, 60 and 99.
# Only append when we actually captured a value (native run ran and analyzed).
if [[ -n "${NATIVE_STEADY_BYTES_PER_REQ}" ]]; then
    # shellcheck disable=SC2310 # format_number returns the raw value on locale failure
    NATIVE_GROWTH_DISPLAY=$(format_number "${NATIVE_STEADY_BYTES_PER_REQ}")
    TEST_NAME="${TEST_NAME}  {BLUE}growth: ${NATIVE_GROWTH_DISPLAY} B/req{RESET}"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
