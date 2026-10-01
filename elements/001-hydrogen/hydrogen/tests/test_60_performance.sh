#!/usr/bin/env bash

# Test: Performance Testing
# Tests API performance across 8 database engines with timing measurements
# Runs 5 iterations. Each iteration signs in, then scans the query catalog
# repeatedly and reads the lookup list. The comparison uses the median of
# the warm, successful query times. Sign-in time is reported beside that.

# FUNCTIONS
# run_performance_test_iteration()
# get_jwt_token()
# run_query_sequence()
# print_performance_summary()
# median_ms()
# format_seconds()

# CHANGELOG
# 1.0.4 - 2026-10-01 - Score the median of warm catalog scans, not the fastest sample
#                    - QueryRef 25 (Get Queries List) repeated, plus QueryRef 30
#                    - Sign-in time is reported and left out of the comparison
#                    - A run that returned an error cannot win
# 1.0.3 - 2026-10-01 - Eighth engine: MSSQL (Demo_MS, schema demoms)
# 1.0.2 - 2026-07-15 - Preserve conduit database mappings across sourced suite tests
#                    - Shared maps are now global instead of disappearing after the
#                      first run_single_test() function returns
# 1.0.1 - 2026-07-15 - Fix result tracking when no database is actually measured
#                    - An unmeasured (skipped) database no longer wins with a fake 0.000s
#                    - When zero timings are captured the test fails loudly instead of
#                      emitting a bogus "winner:  with 999999.999s" and falsely passing
# 1.0.0 - 2026-01-29 - Initial implementation
#                    - Tests API performance across 7 database engines
#                    - Runs 5 iterations to measure caching effectiveness
#                    - Times JWT acquisition and query sequences
#                    - Generates summary with fastest times per database

set -euo pipefail

# Test Configuration
TEST_NAME="Performance Test"
TEST_ABBR="PRF"
TEST_NUMBER="60"
TEST_COUNTER=0
TEST_VERSION="1.0.4"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 8 database engines
PERF_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_60_performance.json"
PERF_LOG_SUFFIX="performance"
PERF_DESCRIPTION="PERF"

# Number of iterations for performance testing. Iteration 1 is warmup and
# is shown in the table, then left out of the median.
PERF_ITERATIONS=5

# QueryRef 25 reads every stored query and computes the length of its name,
# summary, and code. That table is the largest body of text each migrated
# engine actually holds. Repeating it spends the iteration on that scan
# instead of on another HTTP round trip. QueryRef 30, the lookup list, runs
# once after these scans.
PERF_CATALOG_REPEATS=3

# Arrays to store timing results per database per iteration.
# PERF_TIMINGS is query time only. Sign-in time is kept separately.
declare -A PERF_TIMINGS
declare -A PERF_LOGIN_TIMINGS
declare -A PERF_DATA_TRANSFERRED
declare -A PERF_ERROR_COUNTS

# Count of database/iteration combinations that were actually measured. Used to
# detect the case where every database was skipped (not ready / not reachable) so
# the test can fail honestly instead of reporting a bogus winner.
PERF_MEASURED_COUNT=0

# Milliseconds as d.ddd seconds.
format_seconds() {
    local ms="$1"
    printf "%d.%03d" $((ms / 1000)) $((ms % 1000))
}

# Median of integer millisecond samples. An even count averages the two
# middle values. Prints nothing when there are no samples.
median_ms() {
    local -a samples=("$@")
    local count=${#samples[@]}
    if [[ ${count} -eq 0 ]]; then
        return 0
    fi

    # Five samples at most. Sort here so the median does not depend on sort(1).
    local -a sorted=("${samples[@]}")
    local i j key
    for ((i=1; i<count; i++)); do
        key="${sorted[${i}]}"
        j=$((i - 1))
        while [[ ${j} -ge 0 && ${sorted[${j}]} -gt ${key} ]]; do
            sorted[j + 1]="${sorted[${j}]}"
            j=$((j - 1))
        done
        sorted[j + 1]="${key}"
    done

    local mid=$((count / 2))
    if [[ $((count % 2)) -eq 1 ]]; then
        echo "${sorted[${mid}]}"
    else
        local low=$((mid - 1))
        echo $(( (sorted[low] + sorted[mid]) / 2 ))
    fi
}

# Demo credentials from environment variables
# shellcheck disable=SC2034 # Variables used in heredocs for JSON payloads
DEMO_USER_NAME="${HYDROGEN_DEMO_USER_NAME:-}"
# shellcheck disable=SC2034 # Variables used in heredocs for JSON payloads
DEMO_USER_PASS="${HYDROGEN_DEMO_USER_PASS:-}"
# shellcheck disable=SC2034 # Variables used in heredocs for JSON payloads
DEMO_API_KEY="${HYDROGEN_DEMO_API_KEY:-}"

# Function to get JWT token for a specific database and measure timing
# Returns: "jwt_token:elapsed_time_ms:data_transferred_bytes"
get_jwt_token_timed() {
    local base_url="$1"
    local db_engine="$2"
    local db_name="$3"
    local result_file="$4"
    local iteration="${5:-1}"

    local login_data
    login_data=$(cat <<EOF
{
    "database": "${db_name}",
    "login_id": "${HYDROGEN_DEMO_USER_NAME}",
    "password": "${HYDROGEN_DEMO_USER_PASS}",
    "api_key": "${HYDROGEN_DEMO_API_KEY}",
    "tz": "America/Vancouver"
}
EOF
)

    # Create a dedicated directory for this iteration's responses
    local responses_dir="${DIAG_TEST_DIR}/responses/iter${iteration}/${db_engine}"
    mkdir -p "${responses_dir}"

    local login_response_file="${responses_dir}/login.json"

    # Time the JWT acquisition
    local start_time end_time elapsed_ms
    start_time=$(date +%s%N)

    local http_status
    http_status=$(curl -s -X POST "${base_url}/api/auth/login" \
        -H "Content-Type: application/json" \
        -d "${login_data}" \
        -w "%{http_code}\n%{size_download}" \
        -o "${login_response_file}" 2>/dev/null)

    end_time=$(date +%s%N)
    elapsed_ms=$(((end_time - start_time) / 1000000))

    # Parse response size from curl output (second line)
    local data_transferred
    data_transferred=$(echo "${http_status}" | tail -n1)
    http_status=$(echo "${http_status}" | head -n1)

    local jwt_token=""
    if [[ "${http_status}" == "200" ]] && command -v jq >/dev/null 2>&1; then
        jwt_token=$(jq -r '.token' "${login_response_file}" 2>/dev/null || echo "")
        if [[ "${jwt_token}" == "null" ]]; then
            jwt_token=""
        fi
    fi

    # Return format: jwt_token:elapsed_ms:data_bytes:error
    if [[ -n "${jwt_token}" ]]; then
        echo "${jwt_token}:${elapsed_ms}:${data_transferred}:0"
    else
        echo ":${elapsed_ms}:${data_transferred}:1"
    fi
}

# Function to run a single query and measure timing
# Returns: "elapsed_time_ms:data_transferred_bytes:error"
run_single_query_timed() {
    local base_url="$1"
    local endpoint="$2"
    local payload="$3"
    local jwt_token="$4"
    local response_file="$5"

    local start_time end_time elapsed_ms
    start_time=$(date +%s%N)

    local curl_cmd=(curl -s -X POST "${base_url}${endpoint}")

    # Add headers
    curl_cmd+=(-H "Content-Type: application/json")
    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi

    # Add payload and output options
    curl_cmd+=(-d "${payload}")
    curl_cmd+=(-w "%{http_code}\n%{size_download}")
    curl_cmd+=(-o "${response_file}")

    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null)

    end_time=$(date +%s%N)
    elapsed_ms=$(((end_time - start_time) / 1000000))

    # Parse response - http_code is on first line, size_download on second
    local data_transferred
    data_transferred=$(echo "${http_status}" | tail -n1)
    http_status=$(echo "${http_status}" | head -n1)

    local error=0
    if [[ "${http_status}" != "200" ]]; then
        error=1
    fi

    echo "${elapsed_ms}:${data_transferred}:${error}"
}

# Function to run the complete query sequence for a database
# Returns total timing and data info
run_query_sequence() {
    local base_url="$1"
    local db_engine="$2"
    local jwt_token="$3"
    local iteration="$4"

    local total_time=0
    local total_data=0
    local total_errors=0

    # Create a dedicated directory for this iteration's responses
    local responses_dir="${DIAG_TEST_DIR}/responses/iter${iteration}/${db_engine}"
    mkdir -p "${responses_dir}"

    # Query 25: Get Queries List. Each call reads the stored statements and
    # computes LENGTH of name, summary, and code.
    local payload25
    payload25=$(cat <<EOF
{
  "query_ref": 25,
  "params": {}
}
EOF
)
    local repeat
    for ((repeat=1; repeat<=PERF_CATALOG_REPEATS; repeat++)); do
        local response25="${responses_dir}/q25_queries_${repeat}.json"
        local result25
        result25=$(run_single_query_timed "${base_url}" "/api/conduit/auth_query" "${payload25}" "${jwt_token}" "${response25}")
        total_time=$((total_time + $(echo "${result25}" | cut -d: -f1)))
        total_data=$((total_data + $(echo "${result25}" | cut -d: -f2)))
        # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
        total_errors=$((total_errors + $(echo "${result25}" | cut -d: -f3)))
    done

    # Query 30: Get Lookups List. The authenticated list the app loads.
    local payload30
    payload30=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)
    local response30="${responses_dir}/q30_lookups.json"
    local result30
    result30=$(run_single_query_timed "${base_url}" "/api/conduit/auth_query" "${payload30}" "${jwt_token}" "${response30}")
    total_time=$((total_time + $(echo "${result30}" | cut -d: -f1)))
    total_data=$((total_data + $(echo "${result30}" | cut -d: -f2)))
    # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
    total_errors=$((total_errors + $(echo "${result30}" | cut -d: -f3)))

    # Print the location of the response files
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Responses saved to: ${responses_dir}/"

    echo "${total_time}:${total_data}:${total_errors}"
}

# Function to run a single performance iteration
run_performance_iteration() {
    local base_url="$1"
    local result_file="$2"
    local iteration="$3"

    local iter_errors=0

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Iteration ${iteration} - ${db_engine}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in: ${DIAG_TEST_DIR}/responses/iter${iteration}/${db_engine}/"

        # Get JWT token with timing
        local jwt_result
        jwt_result=$(get_jwt_token_timed "${base_url}" "${db_engine}" "${db_name}" "${result_file}" "${iteration}")

        local jwt_token
        jwt_token=$(echo "${jwt_result}" | cut -d: -f1)
        local jwt_time
        jwt_time=$(echo "${jwt_result}" | cut -d: -f2)
        local jwt_error
        jwt_error=$(echo "${jwt_result}" | cut -d: -f4)

        if [[ -z "${jwt_token}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to get JWT for ${db_engine}"
            iter_errors=$((iter_errors + 1))
            continue
        fi

        # Run query sequence with timing
        local sequence_result
        sequence_result=$(run_query_sequence "${base_url}" "${db_engine}" "${jwt_token}" "${iteration}")

        local seq_time
        seq_time=$(echo "${sequence_result}" | cut -d: -f1)
        local seq_data
        seq_data=$(echo "${sequence_result}" | cut -d: -f2)
        local seq_errors
        seq_errors=$(echo "${sequence_result}" | cut -d: -f3)

        # Query time is the comparison. Sign-in is stored beside it.
        local total_errors=$((jwt_error + seq_errors))

        PERF_TIMINGS["${db_engine}_${iteration}"]="${seq_time}"
        PERF_LOGIN_TIMINGS["${db_engine}_${iteration}"]="${jwt_time}"
        PERF_DATA_TRANSFERRED["${db_engine}_${iteration}"]="${seq_data}"
        PERF_ERROR_COUNTS["${db_engine}_${iteration}"]="${total_errors}"
        PERF_MEASURED_COUNT=$((PERF_MEASURED_COUNT + 1))

        local query_sec login_sec
        query_sec=$(format_seconds "${seq_time}")
        login_sec=$(format_seconds "${jwt_time}")

        local formatted_data
        formatted_data=$(format_number "${seq_data}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${total_errors}" "${db_engine} Iter ${iteration}: ${query_sec}s queries, ${login_sec}s login, ${formatted_data} bytes, ${total_errors} errors"

        iter_errors=$((iter_errors + total_errors))
    done

    return "${iter_errors}"
}

# Function to print performance summary
print_performance_summary() {
    print_box "${TEST_NUMBER}" "${TEST_COUNTER}" "Performance Test Summary"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing Results (seconds)"

    # Build timing table output
    local timing_output=""

    # Print header
    timing_output+="$(printf "%-12s" "Database")"
    for ((i=1; i<=PERF_ITERATIONS; i++)); do
        timing_output+="$(printf " %10s" "Run${i}")"
    done
    timing_output+="$(printf " %10s" "Median")"
    timing_output+="$(printf " %10s" "Login")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"

    # Print separator
    timing_output=""
    timing_output+="$(printf "%-12s" "────────────")"
    for ((i=1; i<=PERF_ITERATIONS; i++)); do
        timing_output+="$(printf " %10s" "──────────")"
    done
    timing_output+="$(printf " %10s" "──────────")"
    timing_output+="$(printf " %10s" "──────────")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"

    # Winner is the lowest median of iterations 2-5 that returned no errors.
    # One clean warm run is not enough to crown an engine.
    local best_overall_db=""
    local best_overall_time=999999999
    local measured_count=0

    # Print results for each database
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local db_name="${DATABASE_NAMES[${db_engine}]}"
        local -a timings=()
        local -a errored=()
        local -a clean_samples=()
        local -a login_samples=()

        # Collect timings for this database. A skipped iteration (not ready,
        # or sign-in failed) leaves the key unset. Do not treat that as 0s.
        for ((i=1; i<=PERF_ITERATIONS; i++)); do
            local timing_key="${db_engine}_${i}"
            local timing_ms="${PERF_TIMINGS[${timing_key}]:-}"
            local login_ms="${PERF_LOGIN_TIMINGS[${timing_key}]:-}"
            local error_count="${PERF_ERROR_COUNTS[${timing_key}]:-0}"

            if [[ -n "${login_ms}" ]]; then
                login_samples+=("${login_ms}")
            fi
            if [[ -z "${timing_ms}" ]]; then
                timings+=("n/a")
                errored+=(0)
                continue
            fi

            timings+=("${timing_ms}")
            if [[ ${error_count} -gt 0 ]]; then
                errored+=(1)
            else
                errored+=(0)
                # Iteration 1 warms the statement and the page cache.
                if [[ ${i} -gt 1 ]]; then
                    clean_samples+=("${timing_ms}")
                fi
            fi
        done

        local median_ms=""
        if [[ ${#clean_samples[@]} -ge 2 ]]; then
            median_ms=$(median_ms "${clean_samples[@]}")
            measured_count=$((measured_count + 1))
            if [[ ${median_ms} -lt ${best_overall_time} ]]; then
                best_overall_time=${median_ms}
                best_overall_db="${db_name}"
            fi
        fi

        local login_ms_median=""
        if [[ ${#login_samples[@]} -gt 0 ]]; then
            login_ms_median=$(median_ms "${login_samples[@]}")
        fi

        # Build output line
        timing_output=""
        timing_output+="$(printf "%-12s" "${db_name}:")"

        local idx
        for idx in "${!timings[@]}"; do
            local cell
            if [[ "${timings[${idx}]}" == "n/a" ]]; then
                cell="n/a"
            else
                cell="$(format_seconds "${timings[${idx}]}")s"
                if [[ "${errored[${idx}]}" == "1" ]]; then
                    cell="${cell}*"
                fi
            fi
            timing_output+="$(printf " %10s" "${cell}")"
        done

        local median_cell="n/a"
        local login_cell="n/a"
        if [[ -n "${median_ms}" ]]; then
            median_cell="$(format_seconds "${median_ms}")s"
        fi
        if [[ -n "${login_ms_median}" ]]; then
            login_cell="$(format_seconds "${login_ms_median}")s"
        fi
        timing_output+="$(printf " %10s" "${median_cell}")"
        timing_output+="$(printf " %10s" "${login_cell}")"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Median uses iterations 2-${PERF_ITERATIONS} with no errors. Run 1 is warmup. A star marks a run that returned an error. Login is sign-in time and is not part of the runs."

    # Announce the winner (or report that nothing qualified)
    if [[ -z "${best_overall_db}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Winner: NONE - no engine had two clean warm runs"
        TEST_NAME=$(echo "Performance Test  {BLUE}winner: NONE - no clean warm runs{RESET}" || true)
        EXIT_CODE=1
    else
        local best_overall_sec
        best_overall_sec=$(format_seconds "${best_overall_time}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Winner: ${best_overall_db} median ${best_overall_sec}s (${measured_count} engines with a clean warm median)"
        TEST_NAME=$(echo "Performance Test  {BLUE}winner:  ${best_overall_db} median ${best_overall_sec}s{RESET}" || true)
    fi

    # Print data transferred summary
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Data Transferred (bytes)"

    local data_output=""

    # Print header
    data_output+="$(printf "%-12s" "Database")"
    for ((i=1; i<=PERF_ITERATIONS; i++)); do
        data_output+="$(printf " %10s" "Run${i}")"
    done
    data_output+="$(printf " %12s" "Total")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${data_output}"

    # Print separator
    data_output=""
    data_output+="$(printf "%-12s" "────────────")"
    for ((i=1; i<=PERF_ITERATIONS; i++)); do
        data_output+="$(printf " %10s" "──────────")"
    done
    data_output+="$(printf " %12s" "────────────")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${data_output}"

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local db_name="${DATABASE_NAMES[${db_engine}]}"
        local total_data=0

        data_output=""
        data_output+="$(printf "%-12s" "${db_name}:")"

        for ((i=1; i<=PERF_ITERATIONS; i++)); do
            local data_key="${db_engine}_${i}"
            local data_bytes="${PERF_DATA_TRANSFERRED[${data_key}]:-0}"
            # shellcheck disable=SC2004 # Arithmetic expansion with variable requires $
            total_data=$((total_data + data_bytes))
            local formatted_bytes
            formatted_bytes=$(format_number "${data_bytes}")
            data_output+="$(printf " %10s" "${formatted_bytes}")"
        done

        local formatted_total
        formatted_total=$(format_number "${total_data}")
        data_output+="$(printf " %12s" "${formatted_total}")"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${data_output}"
    done

    # Calculate overall data variance across all databases and iterations
    local overall_min_data=999999999
    local overall_max_data=0

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        for ((i=1; i<=PERF_ITERATIONS; i++)); do
            local data_key="${db_engine}_${i}"
            local data_bytes="${PERF_DATA_TRANSFERRED[${data_key}]:-0}"

            if [[ ${data_bytes} -lt ${overall_min_data} ]]; then
                overall_min_data=${data_bytes}
            fi
            if [[ ${data_bytes} -gt ${overall_max_data} ]]; then
                overall_max_data=${data_bytes}
            fi
        done
    done

    local overall_variance=$((overall_max_data - overall_min_data))
    local formatted_variance
    formatted_variance=$(format_number "${overall_variance}")
    local formatted_min
    formatted_min=$(format_number "${overall_min_data}")
    local formatted_max
    formatted_max=$(format_number "${overall_max_data}")

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Data Transfer Variance (min = ${formatted_min}  max = ${formatted_max}) = ${formatted_variance} bytes"

    # Print error summary
    local total_errors=0
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        for ((i=1; i<=PERF_ITERATIONS; i++)); do
            local error_key="${db_engine}_${i}"
            local error_count="${PERF_ERROR_COUNTS[${error_key}]:-0}"
            # shellcheck disable=SC2004 # Arithmetic expansion with variable requires $
            total_errors=$((total_errors + error_count))
        done
    done

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Error Summary"
    if [[ ${total_errors} -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No errors detected across all iterations"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${total_errors} errors detected across all iterations"
    fi

}

# Function to run performance tests
run_performance_tests() {
    local config_file="$1"
    local log_suffix="$2"
    local description="$3"

    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Start the unified server
    local server_info
    server_info=$(run_conduit_server "${config_file}" "${log_suffix}" "${description}" "${result_file}")

    # Check if server startup failed
    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server startup failed"
        return 1
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server started successfully: ${server_info}"

    # Parse server info
    local base_url hydrogen_pid
    base_url=$(echo "${server_info}" | awk -F: '{print $1":"$2":"$3}')
    hydrogen_pid=$(echo "${server_info}" | awk -F: '{print $4}')

    print_message "60" "0" "Server log location: build/tests/logs/test_60_${TIMESTAMP}_${log_suffix}.log"

    # Run performance iterations
    local total_errors=0
    for ((iter=1; iter<=PERF_ITERATIONS; iter++)); do
        print_box "${TEST_NUMBER}" "${TEST_COUNTER}" "Performance Iteration ${iter}/${PERF_ITERATIONS}"

        # shellcheck disable=SC2310 # We want to continue even if the iteration has errors
        if ! run_performance_iteration "${base_url}" "${result_file}" "${iter}"; then
            # shellcheck disable=SC2004 # Arithmetic expansion with special variable requires $
            total_errors=$((total_errors + $?))
        fi
    done

    echo "PERF_TEST_COMPLETE" >> "${result_file}"

    # Print summary
    print_performance_summary

    # If no database was actually measured (e.g. none became ready in the suite),
    # the run is not a success even if no explicit per-query errors were counted.
    if [[ "${PERF_MEASURED_COUNT}" -eq 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No database timings were captured - performance test cannot report results"
        shutdown_conduit_server "${hydrogen_pid}" "${result_file}"
        return 1
    fi

    # Shutdown the server
    shutdown_conduit_server "${hydrogen_pid}" "${result_file}"

    return "${total_errors}"
}

# Main test execution
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

# Validate required environment variables
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment Variables"
env_vars_valid=true
if [[ -z "${HYDROGEN_DEMO_USER_NAME}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_NAME is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_USER_PASS}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_PASS is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_API_KEY}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_API_KEY is not set"
    env_vars_valid=false
fi

if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
    EXIT_CODE=1
fi

# Validate configuration file
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${PERF_CONFIG_FILE}"; then
    port=$(get_webserver_port "${PERF_CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${PERF_DESCRIPTION} configuration will use port: ${port}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration file validated successfully"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting performance test server (${PERF_DESCRIPTION})"

    # Run performance tests
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if run_performance_tests "${PERF_CONFIG_FILE}" "${PERF_LOG_SUFFIX}" "${PERF_DESCRIPTION}"; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Process results
    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"

    # Add links to log and result files
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${PERF_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${PERF_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Performance Log: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Performance Results: ${DIAG_TEST_DIR}/${result_file##*/}"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping performance tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
