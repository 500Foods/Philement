#!/usr/bin/env bash

# Test: Performance Testing
# Tests API performance across 7 database engines with timing measurements
# Runs 5 iterations of query sequences to measure response times and caching effectiveness

# FUNCTIONS
# run_performance_test_iteration()
# get_jwt_token()
# run_query_sequence()
# print_performance_summary()

# CHANGELOG
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
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
PERF_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_60_performance.json"
PERF_LOG_SUFFIX="performance"
PERF_DESCRIPTION="PERF"

# Number of iterations for performance testing
PERF_ITERATIONS=5

# Arrays to store timing results per database per iteration
declare -A PERF_TIMINGS
declare -A PERF_DATA_TRANSFERRED
declare -A PERF_ERROR_COUNTS

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

# Function to run a batched queries request and measure timing
# Returns: "elapsed_time_ms:data_transferred_bytes:error"
run_batched_queries_timed() {
    local base_url="$1"
    local db_name="$2"
    local jwt_token="$3"
    local response_file="$4"

    local payload
    payload=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {"query_ref": 53, "params": {}},
    {"query_ref": 54, "params": {}},
    {"query_ref": 55, "params": {"INTEGER": {"START": 500, "FINISH": 600}}}
  ]
}
EOF
)

    local start_time end_time elapsed_ms
    start_time=$(date +%s%N)

    local curl_cmd=(curl -s -X POST "${base_url}/api/conduit/queries")

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
    local db_name="$3"
    local jwt_token="$4"
    local result_file="$5"
    local iteration="$6"

    local total_time=0
    local total_data=0
    local total_errors=0

    # Create a dedicated directory for this iteration's responses
    local responses_dir="${DIAG_TEST_DIR}/responses/iter${iteration}/${db_engine}"
    mkdir -p "${responses_dir}"

    # Query 53: Get Themes (public query)
    local payload53
    payload53=$(cat <<EOF
{
  "query_ref": 53,
  "database": "${db_name}",
  "params": {}
}
EOF
)
    local response53="${responses_dir}/q53_themes.json"
    local result53
    result53=$(run_single_query_timed "${base_url}" "/api/conduit/query" "${payload53}" "" "${response53}")
    total_time=$((total_time + $(echo "${result53}" | cut -d: -f1)))
    total_data=$((total_data + $(echo "${result53}" | cut -d: -f2)))
    # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
    total_errors=$((total_errors + $(echo "${result53}" | cut -d: -f3)))

    # Query 54: Get Icons (public query)
    local payload54
    payload54=$(cat <<EOF
{
  "query_ref": 54,
  "database": "${db_name}",
  "params": {}
}
EOF
)
    local response54="${responses_dir}/q54_icons.json"
    local result54
    result54=$(run_single_query_timed "${base_url}" "/api/conduit/query" "${payload54}" "" "${response54}")
    total_time=$((total_time + $(echo "${result54}" | cut -d: -f1)))
    total_data=$((total_data + $(echo "${result54}" | cut -d: -f2)))
    # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
    total_errors=$((total_errors + $(echo "${result54}" | cut -d: -f3)))

    # Query 55: Get Number Range (public query with params)
    local payload55
    payload55=$(cat <<EOF
{
  "query_ref": 55,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"START": 500, "FINISH": 600}
  }
}
EOF
)
    local response55="${responses_dir}/q55_numbers.json"
    local result55
    result55=$(run_single_query_timed "${base_url}" "/api/conduit/query" "${payload55}" "" "${response55}")
    total_time=$((total_time + $(echo "${result55}" | cut -d: -f1)))
    total_data=$((total_data + $(echo "${result55}" | cut -d: -f2)))
    # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
    total_errors=$((total_errors + $(echo "${result55}" | cut -d: -f3)))

    # Batched queries (53, 54, 55 in one request)
    local response_batch="${responses_dir}/batch_queries.json"
    local result_batch
    result_batch=$(run_batched_queries_timed "${base_url}" "${db_name}" "" "${response_batch}")
    total_time=$((total_time + $(echo "${result_batch}" | cut -d: -f1)))
    total_data=$((total_data + $(echo "${result_batch}" | cut -d: -f2)))
    # shellcheck disable=SC2004 # Arithmetic expansion with command substitution requires $
    total_errors=$((total_errors + $(echo "${result_batch}" | cut -d: -f3)))

    # Query 30: Get Lookups List (authenticated query)
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
        local jwt_data
        jwt_data=$(echo "${jwt_result}" | cut -d: -f3)
        local jwt_error
        jwt_error=$(echo "${jwt_result}" | cut -d: -f4)

        if [[ -z "${jwt_token}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to get JWT for ${db_engine}"
            iter_errors=$((iter_errors + 1))
            continue
        fi

        # Run query sequence with timing
        local sequence_result
        sequence_result=$(run_query_sequence "${base_url}" "${db_engine}" "${db_name}" "${jwt_token}" "${result_file}" "${iteration}")

        local seq_time
        seq_time=$(echo "${sequence_result}" | cut -d: -f1)
        local seq_data
        seq_data=$(echo "${sequence_result}" | cut -d: -f2)
        local seq_errors
        seq_errors=$(echo "${sequence_result}" | cut -d: -f3)

        # Calculate totals
        local total_time=$((jwt_time + seq_time))
        local total_data=$((jwt_data + seq_data))
        local total_errors=$((jwt_error + seq_errors))

        # Store results
        PERF_TIMINGS["${db_engine}_${iteration}"]="${total_time}"
        PERF_DATA_TRANSFERRED["${db_engine}_${iteration}"]="${total_data}"
        PERF_ERROR_COUNTS["${db_engine}_${iteration}"]="${total_errors}"

        # Convert to seconds for display with leading zero
        local total_time_sec
        total_time_sec=$(printf "0.%03d" "${total_time}")

        local formatted_data
        formatted_data=$(format_number "${total_data}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${total_errors}" "${db_engine} Iter ${iteration}: ${total_time_sec}s, ${formatted_data} bytes, ${total_errors} errors"

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
    timing_output+="$(printf " %10s" "Best")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"

    # Print separator
    timing_output=""
    timing_output+="$(printf "%-12s" "────────────")"
    for ((i=1; i<=PERF_ITERATIONS; i++)); do
        timing_output+="$(printf " %10s" "──────────")"
    done
    timing_output+="$(printf " %10s" "──────────")"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"

    # Track overall winner
    local best_overall_db=""
    local best_overall_time=999999999

    # Print results for each database
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local db_name="${DATABASE_NAMES[${db_engine}]}"
        local timings=()
        local min_time=999999999

        # Collect timings for this database
        for ((i=1; i<=PERF_ITERATIONS; i++)); do
            local timing_key="${db_engine}_${i}"
            local timing_ms="${PERF_TIMINGS[${timing_key}]:-0}"
            timings+=("${timing_ms}")

            # Track minimum for this database
            if [[ ${timing_ms} -lt ${min_time} ]]; then
                min_time=${timing_ms}
            fi
        done

        # Track overall best
        if [[ ${min_time} -lt ${best_overall_time} ]]; then
            best_overall_time=${min_time}
            best_overall_db="${db_name}"
        fi

        # Build output line
        timing_output=""
        timing_output+="$(printf "%-12s" "${db_name}:")"

        # Print all timings with leading zeros
        for timing_ms in "${timings[@]}"; do
            local timing_sec
            timing_sec=$(printf "0.%03d" "${timing_ms}")
            timing_output+="$(printf " %9ss" "${timing_sec}")"
        done

        # Print best time with leading zero
        local min_sec
        min_sec=$(printf "0.%03d" "${min_time}")
        timing_output+="$(printf " %9ss" "${min_sec}")"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${timing_output}"
    done

    # Announce the winner
    local best_overall_sec
    best_overall_sec=$(printf "0.%03d" "${best_overall_time}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Winner: ${best_overall_db} with ${best_overall_sec}s"
    TEST_NAME=$(echo "Performance Test  {BLUE}winner:  ${best_overall_db} with ${best_overall_sec}s{RESET}" || true)

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
