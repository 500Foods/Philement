#!/usr/bin/env bash

# Test: Memory Exercise - Single Server Auth Stress Test
# Starts one Hydrogen server with all 7 database engines configured,
# runs 500 auth requests cycling across databases, and scrapes
# Prometheus metrics to track memory growth for leak detection.
#
# This test runs a single server with PostgreSQL, MySQL, SQLite, DB2,
# MariaDB, CockroachDB, and YugabyteDB all simultaneously configured.
# Auth requests cycle round-robin across all databases while metrics
# are collected at regular intervals.
#
# Uses conduit_utils.sh library for server lifecycle management,
# matching the proven approach from tests 50 and 51.

# FUNCTIONS
# scrape_metrics()
# get_metric()
# run_auth_request()

# CHANGELOG
# 3.2.0 - 2026-03-20 - Tightened leak threshold from 2 KB/req to 1 KB/req
#                     - Observed steady-state rate ~0.51 KB/req across 500 requests
#                     - 1 KB/req gives ~2x safety margin while catching leaks earlier
# 3.1.0 - 2026-03-19 - Added memory leak detection with steady-state growth analysis
#                     - Compares last-half growth rate to detect unbounded leaks
#                     - Threshold: 2 KB/request in second half flags as leak
#                     - Reports warmup vs steady-state growth separately
# 3.0.0 - 2026-03-19 - Adopted conduit_utils.sh approach from tests 50/51
#                     - Uses run_conduit_server() for startup/migration/readiness
#                     - Uses DATABASE_NAMES from conduit_utils.sh (Demo_PG, etc.)
#                     - Uses check_database_readiness() for per-DB readiness
#                     - Uses shutdown_conduit_server() for clean shutdown
#                     - Config updated to match test 50 database naming pattern
#                     - Removed ad-hoc startup/migration handling
#                     - Removed HYDROGEN_LOG_LEVEL=STATE (was hiding migration logs)
# 2.1.0 - 2026-03-19 - Per-database migration awareness
#                     - Only tests databases that successfully complete migration
#                     - DB-specific log checking (30s fast-fail, 180s max timeout)
#                     - Reports ready vs total databases before exercising
#                     - All output routed through print_message (no raw tee)
# 2.0.0 - 2026-03-19 - Rewrote to use single server with all 7 databases
#                     - Fixed metrics URL from /metrics to /api/system/prometheus
#                     - Added API prefix extraction from config
#                     - Added delay between metric scrapes for stats to update
#                     - Created dedicated config hydrogen_test_41_exercise.json
#                     - Cycles auth requests round-robin across all databases
#                     - Follows test_40 wiring patterns for framework integration
# 1.1.0 - 2026-03-19 - Initial implementation
# 1.0.0 - 2026-03-19 - Created

set -euo pipefail

# Test Configuration
TEST_NAME="Exercise"
TEST_ABBR="EXE"
TEST_NUMBER="41"
TEST_COUNTER=0
TEST_VERSION="3.2.0"

TOTAL_REQUESTS=500
SNAPSHOT_INTERVAL=50
METRICS_DELAY=1  # Seconds to wait before scraping metrics
MIDPOINT_REQUEST=$(( TOTAL_REQUESTS / 2 ))  # For steady-state leak analysis
LEAK_THRESHOLD_KB_PER_REQ=1  # Max KB/request growth in second half before flagging as leak

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Exercise configuration - uses conduit_utils.sh DATABASE_NAMES map
# DATABASE_NAMES is defined in conduit_utils.sh:
#   PostgreSQL=Demo_PG, MySQL=Demo_MY, SQLite=Demo_SQL, DB2=Demo_DB2,
#   MariaDB=Demo_MR, CockroachDB=Demo_CR, YugabyteDB=Demo_YB

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

# Function to scrape Prometheus metrics from the server
scrape_metrics() {
    local prom_url="$1"
    # Give the server a moment to update its internal stats
    sleep "${METRICS_DELAY}"
    curl -s --max-time 10 "${prom_url}" 2>/dev/null || echo ""
}

# Function to extract a metric value from Prometheus text format
# Handles lines like: metric_name 12345
# Also handles lines with labels like: metric_name{label="value"} 12345
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
    RESULT_FILE="${LOG_PREFIX}${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.result"
    METRICS_LOG="${LOG_PREFIX}${TIMESTAMP}_metrics.log"

    true > "${METRICS_LOG}"

    # Use run_conduit_server() from conduit_utils.sh for proven startup,
    # migration waiting, database readiness, and webserver readiness
    server_info=$(run_conduit_server "${CONFIG_FILE}" "${EXERCISE_LOG_SUFFIX}" "Exercise" "${RESULT_FILE}")

    # Check if server startup failed
    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server startup failed"
        EXIT_CODE=1
    else
        # Parse server info - format is "base_url:pid"
        HYDROGEN_PID=$(echo "${server_info}" | awk -F: '{print $4}')
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
            printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s\n" \
                "Request" "RSS_MB" "Delta_MB" "Queries" "Conns" "HTTP_Req" "FDs"
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

                    RSS_MB=$(echo "${RSS}" | awk '{printf "%.1f", $1/1024/1024}')
                    RSS_DELTA=$(echo "${RSS} ${INITIAL_RSS}" | awk '{printf "%.1f", ($1-$2)/1024/1024}')

                    # Capture midpoint RSS for steady-state analysis
                    if [[ "${REQUEST_COUNT}" -eq "${MIDPOINT_REQUEST}" ]]; then
                        MIDPOINT_RSS="${RSS}"
                    fi

                    printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s\n" \
                        "[${REQUEST_COUNT}/${TOTAL_REQUESTS}]" \
                        "${RSS_MB}" "${RSS_DELTA}" "${QUERIES}" \
                        "${CONNS}" "${REQUESTS_TOTAL}" "${FDS}" \
                        >> "${METRICS_LOG}"

                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                        "[${REQUEST_COUNT}/${TOTAL_REQUESTS}] RSS: ${RSS_MB}MB (Δ${RSS_DELTA}MB) | Queries: ${QUERIES} | HTTP: ${REQUESTS_TOTAL} | FDs: ${FDS}"
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
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Memory leak suspected: ${STEADY_GROWTH_PER_REQ_KB} KB/req exceeds ${LEAK_THRESHOLD_KB_PER_REQ} KB/req threshold"
                echo "LEAK_DETECTED=true" >> "${RESULT_FILE}"
                EXIT_CODE=1
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

    # ── Print Summary ───────────────────────────────────────────────

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise complete!"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics log: ${METRICS_LOG}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"

else
    # Skip exercise if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping exercise due to prerequisite failures"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
