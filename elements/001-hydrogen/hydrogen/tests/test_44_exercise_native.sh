#!/usr/bin/env bash

# Test: Memory Exercise Native - Multi-engine auth stress RSS measurement
# Single Hydrogen (hydrogen_release) with six DBs (YugabyteDB disabled), 5000 concurrent
# auth requests, steady-state RSS growth analysis. Companion to test_41 (ASAN/LSAN).
# Separate test number so the suite can run 41 and 44 in parallel (different ports).

# FUNCTIONS (lib/exercise_helpers.sh)
# scrape_metrics() get_metric() run_auth_request() run_auth_batch()

set -euo pipefail

TEST_NAME="Exercise Native"
TEST_ABBR="EXN"
TEST_NUMBER="44"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

TOTAL_REQUESTS=5000
SNAPSHOT_INTERVAL=500
METRICS_DELAY=0
MIDPOINT_REQUEST=$(( TOTAL_REQUESTS / 2 ))
LEAK_THRESHOLD_KB_PER_REQ=1

# Native is RTT-bound; higher concurrency is the exercise. Prefer a dedicated
# env so a shared CONCURRENCY= does not throttle test_44 when tuning ASAN.
# Override: EXERCISE_NATIVE_CONCURRENCY=28 ./tests/test_44_...
CONCURRENCY="${EXERCISE_NATIVE_CONCURRENCY:-${NATIVE_CONCURRENCY:-50}}"

SCRAPE_MAX_ATTEMPTS=5
SCRAPE_CURL_TIMEOUT=15
SCRAPE_RETRY_DELAY=2

STEADY_BYTES_PER_REQ=""

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
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary base: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# ── Locate release (non-ASAN) binary ────────────────────────────────
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Release Binary"

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
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "hydrogen_release not found or is ASAN-instrumented"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Release binary required for native RSS exercise"
    EXIT_CODE=1
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Release binary: ${RELEASE_BIN}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Release binary available"
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

CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_exercise_native.json"

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

BASE_URL="http://localhost:${SERVER_PORT:-5444}"
PROMETHEUS_URL="${BASE_URL}${API_PREFIX:-/api}/system/prometheus"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Exercise Parameters"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total requests: ${TOTAL_REQUESTS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Concurrency: ${CONCURRENCY} in-flight auth requests per batch"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Snapshot interval: every ${SNAPSHOT_INTERVAL} requests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics delay: ${METRICS_DELAY}s"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server URL: ${BASE_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Prometheus URL: ${PROMETHEUS_URL}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Enabled databases: ${DB_LIST:-}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Companion: test_41_exercise_asan.sh (ASAN/LSAN, port 5410)"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Parameters configured"
PASS_COUNT=$(( PASS_COUNT + 1 ))

if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server"

    EXERCISE_LOG_SUFFIX="exercise_native"
    RESULT_FILE="${LOG_PREFIX}_${EXERCISE_LOG_SUFFIX}.result"
    METRICS_LOG="${LOG_PREFIX}_metrics.log"

    true > "${METRICS_LOG}"

    export HYDROGEN_BIN="${RELEASE_BIN}"
    unset ASAN_OPTIONS 2>/dev/null || true
    # STATE keeps startup/migration markers without multi-GB TRACE spam under 5000 auths
    export HYDROGEN_LOG_LEVEL=STATE

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Launching native release: ${HYDROGEN_BIN}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HYDROGEN_LOG_LEVEL=${HYDROGEN_LOG_LEVEL}"

    server_info=$(run_conduit_server "${CONFIG_FILE}" "${EXERCISE_LOG_SUFFIX}" "Exercise-Native" "${RESULT_FILE}")

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
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Native RSS Measurement (${TOTAL_REQUESTS} requests)"

        N_INIT=$(scrape_metrics "${PROMETHEUS_URL}" "${METRICS_DELAY}")
        N_INIT_RSS=$(get_metric "${N_INIT}" "hydrogen_process_resident_memory_bytes")
        N_INIT_Q=$(get_metric "${N_INIT}" "hydrogen_database_queries_executed_total")
        N_INIT_F=$(get_metric "${N_INIT}" "hydrogen_process_open_fds")
        N_INIT_MB=$(echo "${N_INIT_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Initial RSS: ${N_INIT_MB} MB, Queries: ${N_INIT_Q}, FDs: ${N_INIT_F}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total requests: ${TOTAL_REQUESTS}, concurrency ${CONCURRENCY}, snapshot every ${SNAPSHOT_INTERVAL}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using: ${READY_DATABASES[*]}"

        {
            echo "=== NATIVE RELEASE EXERCISE (hydrogen_release) ==="
            echo "Binary: ${RELEASE_BIN}"
            echo "Concurrency: ${CONCURRENCY}"
            echo "Initial RSS: ${N_INIT_RSS} bytes (${N_INIT_MB} MB)"
            echo "Initial Queries: ${N_INIT_Q}"
            echo ""
            printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                "Request" "RSS_MB" "Delta_MB" "Queries" "Conns" "HTTP_Req" "FDs" "PrepCache" "InFlight" "PrepEvict" "Ctx" "DBCr" "DBCl" "PHit" "PMis"
        } >> "${METRICS_LOG}"

        N_CNT=0
        N_MID=0
        N_LAST_SNAPSHOT_AT=0
        N_LAST_GOOD_RSS=0
        N_LAST_GOOD_CNT=0
        N_SNAPSHOTS_OK=0
        N_SNAPSHOTS_FAIL=0
        N_CONSEC_FAILS=0
        N_SERVER_DIED=false
        N_MAX_CONSEC_FAILS=2

        while (( N_CNT < TOTAL_REQUESTS )); do
            n_batch="${CONCURRENCY}"
            n_remaining=$(( TOTAL_REQUESTS - N_CNT ))
            if (( n_batch > n_remaining )); then
                n_batch="${n_remaining}"
            fi

            run_auth_batch "${BASE_URL}" "$(( N_CNT + 1 ))" "${n_batch}" READY_DATABASES
            N_CNT=$(( N_CNT + n_batch ))

            if (( N_CNT - N_LAST_SNAPSHOT_AT >= SNAPSHOT_INTERVAL )) || (( N_CNT >= TOTAL_REQUESTS )); then
                N_LAST_SNAPSHOT_AT="${N_CNT}"
                NM=$(scrape_metrics "${PROMETHEUS_URL}" "${METRICS_DELAY}")
                if [[ -n "${NM}" ]] && echo "${NM}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
                    N_CONSEC_FAILS=0
                    N_SNAPSHOTS_OK=$(( N_SNAPSHOTS_OK + 1 ))
                    NR=$(get_metric "${NM}" "hydrogen_process_resident_memory_bytes")
                    NQ=$(get_metric "${NM}" "hydrogen_database_queries_executed_total")
                    # Do NOT name this NC — framework global No-Color escape
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

                    if [[ "${NR}" -gt 0 ]]; then
                        N_LAST_GOOD_RSS="${NR}"
                        N_LAST_GOOD_CNT="${N_CNT}"
                    fi

                    if [[ "${N_MID}" -eq 0 && "${N_CNT}" -ge "${MIDPOINT_REQUEST}" && "${NR}" -gt 0 ]]; then
                        N_MID="${NR}"
                    fi

                    printf "%-8s  %-10s  %-10s  %-10s  %-8s  %-10s  %-6s  %-8s  %-8s  %-7s  %-6s  %-6s  %-6s  %-7s  %-7s\n" \
                        "[${N_CNT}/${TOTAL_REQUESTS}]" \
                        "${NR_MB}" "${ND}" "${NQ}" "${NCONN}" "${NRT}" "${NF}" "${NP}" "${NI}" "${NPE}" "${NAC}" \
                        "${NDC}" "${NCL}" "${NPH}" "${NPM}" \
                        >> "${METRICS_LOG}"

                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" \
                        "[${N_CNT}/${TOTAL_REQUESTS}] RSS: ${NR_MB}MB (Δ${ND}MB) | Queries: ${NQ} | FDs: ${NF} | PrepCache: ${NP} | InFlight: ${NI} | Ctx: ${NAC} | DBConns: ${NDC}/${NCL} | PrepHits/Miss: ${NPH}/${NPM}"
                else
                    N_SNAPSHOTS_FAIL=$(( N_SNAPSHOTS_FAIL + 1 ))
                    N_CONSEC_FAILS=$(( N_CONSEC_FAILS + 1 ))
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                        "[${N_CNT}/${TOTAL_REQUESTS}] WARNING: metrics scrape failed (consecutive failures: ${N_CONSEC_FAILS})"
                    {
                        echo "[${N_CNT}/${TOTAL_REQUESTS}] SCRAPE FAILED (consecutive: ${N_CONSEC_FAILS})"
                    } >> "${METRICS_LOG}"
                    if [[ "${N_CONSEC_FAILS}" -ge "${N_MAX_CONSEC_FAILS}" ]]; then
                        N_SERVER_DIED=true
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                            "Server unresponsive after ${N_CNT} requests (${N_CONSEC_FAILS} consecutive failed scrapes) - aborting"
                        break
                    fi
                fi
            fi
        done

        sleep 0.25
        NFN=$(scrape_metrics "${PROMETHEUS_URL}" "${METRICS_DELAY}")
        if [[ -n "${NFN}" ]] && echo "${NFN}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            NF_RSS=$(get_metric "${NFN}" "hydrogen_process_resident_memory_bytes")
            NF_Q=$(get_metric "${NFN}" "hydrogen_database_queries_executed_total")
            NF_F=$(get_metric "${NFN}" "hydrogen_process_open_fds")
        else
            NF_RSS="${N_LAST_GOOD_RSS}"
            NF_Q=0
            NF_F=0
            if [[ "${NF_RSS}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Final scrape missed; using last good snapshot at request ${N_LAST_GOOD_CNT}"
            fi
        fi
        NF_MB=$(echo "${NF_RSS}" | awk '{printf "%.1f", $1/1024/1024}')
        NG_MB=$(echo "${NF_RSS} ${N_INIT_RSS}" | awk '{printf "%.1f", ($1-$2)/1024/1024}')

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Final RSS: ${NF_MB} MB | Growth: ${NG_MB} MB over ${N_CNT} requests (target ${TOTAL_REQUESTS})"

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
            echo "CONCURRENCY=${CONCURRENCY}"
            echo "REQUESTS_COMPLETED=${N_CNT}"
        } >> "${METRICS_LOG}"

        if [[ -n "${HYDROGEN_PID:-}" ]]; then
            shutdown_conduit_server "${HYDROGEN_PID}" "${RESULT_FILE}"
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server stopped"

        if [[ "${N_SERVER_DIED}" == true ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server hung/crashed under load; check ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server became unresponsive after ${N_CNT} requests (${N_SNAPSHOTS_OK} good / ${N_SNAPSHOTS_FAIL} failed snapshots)"
            echo "NATIVE_SERVER_DIED=true" >> "${RESULT_FILE}"
            echo "NATIVE_LAST_RESPONSIVE_REQUEST=${N_LAST_GOOD_CNT}" >> "${RESULT_FILE}"
            EXIT_CODE=1
        elif [[ "${N_MID}" -gt 0 && "${NF_RSS}" -gt 0 ]]; then
            NSH=$(( TOTAL_REQUESTS - MIDPOINT_REQUEST ))
            NSG=$(echo "${NF_RSS} ${N_MID}" | awk '{printf "%d", $1 - $2}')
            NSK=$(echo "${NSG} ${NSH}" | awk '{if ($2>0) printf "%.2f", $1/$2/1024; else print "0"}')
            NSB=$(echo "${NSG} ${NSH}" | awk '{if ($2>0) printf "%d", $1/$2; else print "0"}')
            STEADY_BYTES_PER_REQ="${NSB}"
            NFH=$(echo "${N_MID} ${N_INIT_RSS}" | awk '{printf "%.1f", ($1-$2)/1024}')

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Warmup (first ${MIDPOINT_REQUEST} reqs): ${NFH} KB"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Steady-state (last ${NSH} reqs): ${NSK} KB/request (${NSB} B/req) (threshold: ${LEAK_THRESHOLD_KB_PER_REQ})"

            {
                echo ""
                echo "=== NATIVE LEAK ANALYSIS ==="
                echo "Warmup (first ${MIDPOINT_REQUEST} reqs): ${NFH} KB"
                echo "Steady-state (last ${NSH} reqs): ${NSK} KB/request"
                echo "Steady-state bytes/request: ${NSB}"
                echo "Threshold: ${LEAK_THRESHOLD_KB_PER_REQ}"
            } >> "${METRICS_LOG}"

            nleak=$(echo "${NSK} ${LEAK_THRESHOLD_KB_PER_REQ}" | awk '{if ($1 > $2) print "yes"; else print "no"}')
            if [[ "${nleak}" == "yes" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Memory growth: ${NSK} KB/req exceeds ${LEAK_THRESHOLD_KB_PER_REQ} KB/req"
                echo "NATIVE_LEAK_DETECTED=true" >> "${RESULT_FILE}"
                EXIT_CODE=1
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Native growth within threshold: ${NSK} KB/req"
                echo "NATIVE_LEAK_DETECTED=false" >> "${RESULT_FILE}"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Insufficient data for analysis (good snapshots: ${N_SNAPSHOTS_OK}, midpoint RSS: ${N_MID}, final RSS: ${NF_RSS})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Measurement incomplete - could not compute steady-state growth"
            echo "NATIVE_MEASUREMENT_INCOMPLETE=true" >> "${RESULT_FILE}"
            EXIT_CODE=1
        fi
    fi

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Native exercise complete!"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Metrics log: ${METRICS_LOG}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${EXERCISE_LOG_SUFFIX}.log"

else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping exercise due to prerequisite failures"
fi

if [[ -n "${STEADY_BYTES_PER_REQ}" ]]; then
    # shellcheck disable=SC2310 # format_number returns the raw value on locale failure
    GROWTH_DISPLAY=$(format_number "${STEADY_BYTES_PER_REQ}")
    TEST_NAME="${TEST_NAME}  {BLUE}growth: ${GROWTH_DISPLAY} B/req{RESET}"
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
