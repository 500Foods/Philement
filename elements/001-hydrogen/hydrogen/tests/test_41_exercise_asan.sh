#!/usr/bin/env bash

# Test: Memory Exercise ASAN - Multi-engine auth stress under LeakSanitizer
# Single Hydrogen (hydrogen_debug) with six DBs (YugabyteDB disabled), 500 concurrent
# auth requests, RSS snapshots (informational under ASAN), post-shutdown LSAN scan.
# Native RSS measurement lives in test_44_exercise_native.sh (suite-parallel).

# FUNCTIONS (lib/exercise_helpers.sh)
# scrape_metrics() get_metric() run_auth_request() run_auth_batch()

set -euo pipefail

TEST_NAME="Exercise ASAN"
TEST_ABBR="EXA"
TEST_NUMBER="41"
TEST_COUNTER=0
TEST_VERSION="4.0.0"

TOTAL_REQUESTS=500
SNAPSHOT_INTERVAL=50
METRICS_DELAY=0.25
MIDPOINT_REQUEST=$(( TOTAL_REQUESTS / 2 ))

# ASAN is instrumentation-bound: more parallel curls rarely help and often hurt
# (queueing). Prefer ~1 in-flight per engine. Do NOT share CONCURRENCY with test_44
# (native wants 50). Override: EXERCISE_ASAN_CONCURRENCY=12 ./tests/test_41_...
CONCURRENCY="${EXERCISE_ASAN_CONCURRENCY:-${ASAN_CONCURRENCY:-6}}"

SCRAPE_MAX_ATTEMPTS=5
SCRAPE_CURL_TIMEOUT=15
SCRAPE_RETRY_DELAY=2

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment
# shellcheck source=tests/lib/conduit_utils.sh # Multi-DB server lifecycle helpers
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
# shellcheck source=tests/lib/exercise_helpers.sh # Shared auth exercise scrape/batch helpers
[[ -n "${EXERCISE_HELPERS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/exercise_helpers.sh"

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

# ── Detect ASAN Debug Build ─────────────────────────────────────────
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Detect ASAN Debug Build"

ASAN_MODE=false
ASAN_HYDROGEN_BIN=""
for cand in \
    "${HYDROGEN_BIN_BASE}_debug" \
    "hydrogen_debug" \
    "$(dirname "${HYDROGEN_BIN}" 2>/dev/null || echo ".")/hydrogen_debug" \
    "${PROJECT_DIR}/hydrogen_debug" \
    "build/hydrogen_debug" ; do
    if [[ -n "${cand}" ]]; then
        local_resolved="${cand}"
        if command -v realpath >/dev/null 2>&1; then
            local_resolved=$(realpath "${cand}" 2>/dev/null || echo "${cand}")
        elif command -v readlink >/dev/null 2>&1; then
            local_resolved=$(readlink -f "${cand}" 2>/dev/null || echo "${cand}")
        fi
        if [[ "${local_resolved}" != /* && -f "${local_resolved}" ]]; then
            local_resolved="./${local_resolved}"
        fi
        if [[ -x "${local_resolved}" ]]; then
            if { readelf -s "${local_resolved}" || true; } | "${GREP}" -q "__asan"; then
                ASAN_HYDROGEN_BIN="${local_resolved}"
                if ! strings "${local_resolved}" 2>/dev/null | "${GREP}" -q -E 'PAYLOAD:|<<<< HERE BE ME TREASURE >>>' ; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: ${local_resolved} has ASAN but may lack embedded payload (plain debug target skips embed_payload.sh)"
                fi
                ASAN_MODE=true
                break
            fi
        fi
    fi
done

if [[ "${ASAN_MODE}" == true ]]; then
    if [[ -n "${REALPATH:-}" ]] && command -v "${REALPATH}" >/dev/null 2>&1; then
        ASAN_HYDROGEN_BIN=$("${REALPATH}" "${ASAN_HYDROGEN_BIN}" 2>/dev/null || echo "${ASAN_HYDROGEN_BIN}")
    else
        if [[ "${ASAN_HYDROGEN_BIN}" != /* && -f "${ASAN_HYDROGEN_BIN}" ]]; then
            ASAN_HYDROGEN_BIN="./${ASAN_HYDROGEN_BIN}"
        fi
    fi
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN debug build found: ${ASAN_HYDROGEN_BIN}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "ASAN debug build available - exercise will run under LeakSanitizer"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No ASAN debug build found; cannot run LeakSanitizer exercise"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "ASAN debug build required for test_41 (use test_44 for native RSS)"
    EXIT_CODE=1
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
        DB_ENABLED=$(exercise_enabled_db_count "${CONFIG_FILE}")
        DB_LIST=$(exercise_enabled_db_names "${CONFIG_FILE}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port: ${SERVER_PORT}, API prefix: ${API_PREFIX}, Enabled databases: ${DB_ENABLED} (${DB_LIST})"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration validated"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Config file not found: ${CONFIG_FILE}"
    EXIT_CODE=1
fi

BASE_URL="http://localhost:${SERVER_PORT:-5410}"
PROMETHEUS_URL="${BASE_URL}${API_PREFIX:-/api}/system/prometheus"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise Parameters"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total requests: ${TOTAL_REQUESTS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Concurrency: ${CONCURRENCY} in-flight auth requests per batch"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Snapshot interval: every ${SNAPSHOT_INTERVAL} requests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics delay: ${METRICS_DELAY}s"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server URL: ${BASE_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Prometheus URL: ${PROMETHEUS_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Enabled databases: ${DB_LIST:-}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Companion: test_44_exercise_native.sh (native RSS, port 5444)"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Parameters configured"
PASS_COUNT=$(( PASS_COUNT + 1 ))

if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server"

    EXERCISE_LOG_SUFFIX="exercise"
    RESULT_FILE="${LOG_PREFIX}_${EXERCISE_LOG_SUFFIX}.result"
    METRICS_LOG="${LOG_PREFIX}_metrics.log"
    ASAN_EXERCISE_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}_asan.log"

    true > "${METRICS_LOG}"

    ORIGINAL_HYDROGEN_BIN="${HYDROGEN_BIN}"
    if [[ "${ASAN_HYDROGEN_BIN}" != /* ]]; then
        if command -v realpath >/dev/null 2>&1; then
            ASAN_HYDROGEN_BIN=$(realpath "${ASAN_HYDROGEN_BIN}" 2>/dev/null || echo "${ASAN_HYDROGEN_BIN}")
        else
            ASAN_HYDROGEN_BIN="$(pwd)/${ASAN_HYDROGEN_BIN##*/}"
        fi
    fi
    export HYDROGEN_BIN="${ASAN_HYDROGEN_BIN}"
    export ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=1:log_threads=1:print_stats=1"
    export CONDUIT_STARTUP_TIMEOUT=180
    export CONDUIT_WEBSERVER_READY_TIMEOUT=180
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Launching under ASAN: ${HYDROGEN_BIN}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN_OPTIONS=${ASAN_OPTIONS}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Startup timeouts: CONDUIT_STARTUP_TIMEOUT=${CONDUIT_STARTUP_TIMEOUT}s (multi-DB ASAN)"

    server_info=$(run_conduit_server "${CONFIG_FILE}" "${EXERCISE_LOG_SUFFIX}" "Exercise-ASAN" "${RESULT_FILE}")

    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server startup failed"
        EXIT_CODE=1
    else
        HYDROGEN_PID=$(echo "${server_info}" | awk -F: '{print $NF}')
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server started: ${server_info}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: build/tests/logs/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started successfully"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Build Ready Databases List"

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
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Databases ready: ${#READY_DATABASES[@]}/${DB_ENABLED:-6} enabled"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No databases ready - cannot run exercise"
            EXIT_CODE=1
        fi
    fi

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

            {
                echo "=== INITIAL METRICS (ASAN) ==="
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
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to collect initial metrics"
            INITIAL_RSS=0
            INITIAL_QUERIES=0
            INITIAL_CONNS=0
            INITIAL_REQUESTS=0
            INITIAL_FDS=0
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Run Auth Exercise"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running ${TOTAL_REQUESTS} auth requests across ${#READY_DATABASES[@]} ready databases (concurrency ${CONCURRENCY})..."
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using: ${READY_DATABASES[*]}"

        {
            echo "=== EXERCISE SNAPSHOTS ==="
            printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                "Request" "RSS_MB" "Delta_MB" "Queries" "Conns" "HTTP_Req" "FDs" "PrepCache" "InFlight" "PrepEvict" "Ctx" "DBCr" "DBCl" "PHit" "PMis"
        } >> "${METRICS_LOG}"

        REQUEST_COUNT=0
        FAILED_REQUESTS=0
        MIDPOINT_RSS=0
        LAST_SNAPSHOT_AT=0

        while (( REQUEST_COUNT < TOTAL_REQUESTS )); do
            batch_size="${CONCURRENCY}"
            remaining=$(( TOTAL_REQUESTS - REQUEST_COUNT ))
            if (( batch_size > remaining )); then
                batch_size="${remaining}"
            fi

            run_auth_batch "${BASE_URL}" "$(( REQUEST_COUNT + 1 ))" "${batch_size}" READY_DATABASES
            REQUEST_COUNT=$(( REQUEST_COUNT + batch_size ))

            if (( REQUEST_COUNT - LAST_SNAPSHOT_AT >= SNAPSHOT_INTERVAL )) || (( REQUEST_COUNT >= TOTAL_REQUESTS )); then
                LAST_SNAPSHOT_AT="${REQUEST_COUNT}"
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

        {
            echo "REQUESTS_COMPLETED=${REQUEST_COUNT}"
            echo "FAILED_METRICS_SCRAPES=${FAILED_REQUESTS}"
            echo "CONCURRENCY=${CONCURRENCY}"
        } >> "${RESULT_FILE}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Exercise completed (${REQUEST_COUNT} requests, concurrency ${CONCURRENCY})"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Collect Final Metrics"

        sleep 0.5
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

            RSS_GROWTH=$(echo "${FINAL_RSS} ${INITIAL_RSS}" | awk '{print $1 - $2}')
            RSS_GROWTH_KB=$(echo "${RSS_GROWTH}" | awk '{printf "%.1f", $1/1024}')
            RSS_GROWTH_MB=$(echo "${RSS_GROWTH}" | awk '{printf "%.1f", $1/1024/1024}')
            FINAL_RSS_MB=$(echo "${FINAL_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
            QUERY_COUNT=$(echo "${FINAL_QUERIES} ${INITIAL_QUERIES}" | awk '{printf "%d", $1 - $2}')

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

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Memory Leak Detection (RSS informational)"

        if [[ "${MIDPOINT_RSS}" -gt 0 ]] && [[ "${FINAL_RSS}" -gt 0 ]]; then
            SECOND_HALF_REQUESTS=$(( TOTAL_REQUESTS - MIDPOINT_REQUEST ))
            SECOND_HALF_GROWTH_BYTES=$(echo "${FINAL_RSS} ${MIDPOINT_RSS}" | awk '{printf "%d", $1 - $2}')
            SECOND_HALF_GROWTH_KB=$(echo "${SECOND_HALF_GROWTH_BYTES}" | awk '{printf "%.1f", $1/1024}')
            FIRST_HALF_GROWTH_KB=$(echo "${MIDPOINT_RSS} ${INITIAL_RSS}" | awk '{printf "%.1f", ($1-$2)/1024}')
            STEADY_GROWTH_PER_REQ_KB="0"
            if [[ "${SECOND_HALF_REQUESTS}" -gt 0 ]]; then
                STEADY_GROWTH_PER_REQ_KB=$(echo "${SECOND_HALF_GROWTH_BYTES} ${SECOND_HALF_REQUESTS}" | awk '{printf "%.2f", $1/$2/1024}')
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Warmup growth (first ${MIDPOINT_REQUEST} reqs): ${FIRST_HALF_GROWTH_KB} KB"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state growth (last ${SECOND_HALF_REQUESTS} reqs): ${SECOND_HALF_GROWTH_KB} KB"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state rate: ${STEADY_GROWTH_PER_REQ_KB} KB/request (RSS under ASAN is diagnostic only)"

            {
                echo ""
                echo "=== LEAK ANALYSIS (RSS secondary under ASAN) ==="
                echo "Warmup growth (first ${MIDPOINT_REQUEST} reqs): ${FIRST_HALF_GROWTH_KB} KB"
                echo "Steady-state growth (last ${SECOND_HALF_REQUESTS} reqs): ${SECOND_HALF_GROWTH_KB} KB"
                echo "Steady-state rate: ${STEADY_GROWTH_PER_REQ_KB} KB/request"
            } >> "${METRICS_LOG}"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "RSS heuristic deferred to LeakSanitizer (authoritative under ASAN)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "RSS diagnostic recorded; LSAN is authoritative"
            echo "LEAK_DETECTED=false" >> "${RESULT_FILE}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Insufficient data for RSS diagnostic (midpoint RSS not captured)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "RSS diagnostic skipped (insufficient data)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        fi
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown Server"
    if [[ -n "${HYDROGEN_PID:-}" ]]; then
        shutdown_conduit_server "${HYDROGEN_PID}" "${RESULT_FILE}"
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server stopped"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    if [[ "${ASAN_MODE}" == true ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN LeakSanitizer Analysis"

        export HYDROGEN_BIN="${ORIGINAL_HYDROGEN_BIN}"

        if [[ "${server_info:-}" == "FAILED:0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping ASAN leak analysis: server did not start successfully"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "ASAN: server startup failed under debug build - no leak data produced"
        else
            ASAN_LOG_CANDIDATE="${ASAN_EXERCISE_LOG}"
            if [[ ! -f "${ASAN_LOG_CANDIDATE}" ]]; then
                ASAN_LOG_CANDIDATE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
            fi

            if [[ -f "${ASAN_LOG_CANDIDATE}" ]]; then
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

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ASAN exercise complete!"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics log: ${METRICS_LOG}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native RSS: run test_44_exercise_native.sh (or full suite)"

else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping exercise due to prerequisite failures"
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
