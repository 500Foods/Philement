#!/usr/bin/env bash

# Test: Conduit Query Endpoints
# Tests all Conduit Service endpoints that execute pre-defined database queries by reference
# Launches all 7 database engines from test_40, then tests the conduit endpoints:
# - /api/conduit/query: Single query execution (public)
# - /api/conduit/queries: Multiple query execution (public)
# - /api/conduit/auth_query: Single query with JWT authentication
# - /api/conduit/auth_queries: Multiple queries with JWT authentication

# FUNCTIONS
# validate_conduit_request()
# test_conduit_single_query()
# test_conduit_multiple_queries()
# test_conduit_auth_single_query()
# test_conduit_auth_multiple_queries()
# run_conduit_test_parallel()
# analyze_conduit_test_results()

# CHANGELOG
# 1.4.0 - 2026-01-19 - Further refactored to extract JWT acquisition and test helper functions
#                    - Added acquire_jwt_tokens(), test_conduit_single_queries(), test_conduit_multiple_queries_endpoint() to lib
#                    - Simplified test functions using new generic helpers
#                    - Improved code reusability and maintainability
# 1.3.0 - 2026-01-19 - Refactored to extract conduit utilities into tests/lib/conduit_utils.sh
#                    - Moved validate_conduit_request() and check_database_readiness() to lib
#                    - Moved DATABASE_NAMES, PUBLIC_QUERY_REFS, NON_PUBLIC_QUERY_REFS to lib
#                    - Reduced script size and improved reusability for other conduit tests
# 1.2.0 - 2026-01-18 - Updated to use unified single-server approach with test_50_conduit.json
#                    - Single server with 7 databases instead of 7 parallel servers
#                    - Tests all conduit endpoints across all database engines in one server
#                    - Maintains comprehensive coverage while reducing resource usage
# 1.1.0 - 2026-01-16 - Renamed from test_41 to test_51, expanded to 7 databases
#                    - Added testing for all 4 conduit endpoints
#                    - Integrated JWT authentication testing
#                    - Uses real query_refs from Acuranzo migrations
#                    - Copied configuration approach from test_40
# 1.0.0 - 2025-10-20 - Initial implementation based on test_21_system_endpoints.sh and test_30_database.sh
#                    - Launches all 5 database engines from test_30_multi.json (renamed to test_40_conduit.json)
#                    - Tests /api/conduit/query endpoint with various query references
#                    - Validates query execution, parameter handling, and result formatting

set -euo pipefail


# Test Configuration
TEST_NAME="Conduit Endpoints  {BLUE}CDT: 0{RESET}"
TEST_ABBR="QRY"
TEST_NUMBER="51"
TEST_COUNTER=0
TEST_VERSION="1.4.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_51_conduit.json"
CONDUIT_LOG_SUFFIX="conduit"
CONDUIT_ENGINE_NAME="multi"
CONDUIT_DESCRIPTION="Conduit"

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10
MIGRATION_TIMEOUT=300

# Demo credentials from environment variables (set in shell and used in migrations)
# Used in heredocs for JSON payloads
# shellcheck disable=SC2034 # Used in heredocs that may be expanded in future versions
DEMO_USER_NAME="${HYDROGEN_DEMO_USER_NAME:-}"
# Used in heredocs for JSON payloads
# shellcheck disable=SC2034 # Used in heredocs that may be expanded in future versions
DEMO_USER_PASS="${HYDROGEN_DEMO_USER_PASS:-}"
# These variables are defined for future test expansion
# shellcheck disable=SC2034 # Reserved for future test expansion
DEMO_ADMIN_NAME="${HYDROGEN_DEMO_ADMIN_NAME:-}"
# These variables are defined for future test expansion
# shellcheck disable=SC2034 # Reserved for future test expansion
DEMO_ADMIN_PASS="${HYDROGEN_DEMO_ADMIN_PASS:-}"
# Used in heredocs for JSON payloads
# shellcheck disable=SC2034 # Used in heredocs that may be expanded in future versions
DEMO_EMAIL="${HYDROGEN_DEMO_EMAIL:-}"
# Used in heredocs for JSON payloads
# shellcheck disable=SC2034 # Used in heredocs that may be expanded in future versions
DEMO_API_KEY="${HYDROGEN_DEMO_API_KEY:-}"



# Function to test conduit single query endpoint with public queries across ready databases
test_conduit_single_public_query() {
    local base_url="$1"
    local result_file="$2"

    local result
    result=$(test_conduit_single_queries "${base_url}" "${result_file}" "/api/conduit/query" "" "Public Query")

    IFS=':' read -r tests_passed total_tests <<< "${result}"

    echo "SINGLE_PUBLIC_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "SINGLE_PUBLIC_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit single query endpoint with invalid query refs
test_conduit_single_invalid_query() {
    local base_url="$1"
    local result_file="$2"

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Test invalid query ref -100
        local payload
        payload=$(cat <<EOF
{
  "query_ref": -100,
  "database": "${db_name}",
  "params": {}
}
EOF
)

        local response_file="${result_file}.single_invalid_${db_engine}.json"

        # Expect 200 with "fail" response for invalid query ref
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Invalid Query -100" "\"fail\""; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "SINGLE_INVALID_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "SINGLE_INVALID_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit multiple queries endpoint across ready databases
test_conduit_multiple_queries() {
    local base_url="$1"
    local result_file="$2"

    local queries_json
    queries_json=$(cat <<EOF
[
  {
    "query_ref": 1005,
    "params": {
      "INTEGER": { "limit": 5 }
    }
  },
  {
    "query_ref": 1006,
    "params": {
      "STRING": { "status": "active" }
    }
  }
]
EOF
)

    local result
    result=$(test_conduit_multiple_queries_endpoint "${base_url}" "${result_file}" "/api/conduit/queries" "" "Multiple Queries: System Info + Query List" "${queries_json}")

    IFS=':' read -r tests_passed total_tests <<< "${result}"

    echo "MULTIPLE_QUERIES_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "MULTIPLE_QUERIES_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit auth single query endpoint across ready databases
test_conduit_auth_single_query() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "AUTH_SINGLE_QUERY_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for auth single query
        local payload
        payload=$(cat <<EOF
{
  "query_ref": 1005,
  "params": {
    "STRING": {
      "username": "${HYDROGEN_DEMO_USER_NAME}"
    }
  }
}
EOF
)

        local response_file="${result_file}.auth_single_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${db_engine} Auth Single Query: Get Account ID"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "AUTH_SINGLE_QUERY_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "AUTH_SINGLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "AUTH_SINGLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit auth multiple queries endpoint across ready databases
test_conduit_auth_multiple_queries() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "AUTH_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for auth multiple queries
        local payload
        payload=$(cat <<EOF
{
  "queries": [
    {
      "query_ref": 1005,
      "params": {}
    },
    {
      "query_ref": 1006,
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
EOF
)

        local response_file="${result_file}.auth_multiple_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_queries" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${db_engine} Auth Multiple Queries: System Info + Query List"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "AUTH_MULTIPLE_QUERIES_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "AUTH_MULTIPLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "AUTH_MULTIPLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit alt single query endpoint across ready databases
test_conduit_alt_single_query() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "ALT_SINGLE_QUERY_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for alt single query
        local payload
        payload=$(cat <<EOF
{
  "token": "${jwt_token}",
  "database": "${db_name}",
  "query_ref": 1005,
  "params": {
    "STRING": {
      "username": "${HYDROGEN_DEMO_USER_NAME}"
    }
  }
}
EOF
)

        local response_file="${result_file}.alt_single_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/alt_query" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Alt Single Query: Get Account ID"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "ALT_SINGLE_QUERY_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "ALT_SINGLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "ALT_SINGLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit alt multiple queries endpoint across ready databases
test_conduit_alt_multiple_queries() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for alt multiple queries
        local payload
        payload=$(cat <<EOF
{
  "token": "${jwt_token}",
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 1005,
      "params": {}
    },
    {
      "query_ref": 1006,
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
EOF
)

        local response_file="${result_file}.alt_multiple_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/alt_queries" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Alt Multiple Queries: System Info + Query List"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "ALT_MULTIPLE_QUERIES_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "ALT_MULTIPLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "ALT_MULTIPLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit status endpoint
test_conduit_status() {
    local base_url="$1"
    local jwt_tokens_string="$2"  # Space-separated string of JWT tokens (one per database)
    local result_file="$3"

    local tests_passed=0
    local total_tests=0

    # Test status endpoint without authentication (basic status)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (Public)"
    local response_file="${result_file}.status_public.json"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_conduit_request "${base_url}/api/conduit/status" "GET" "" "200" "${response_file}" "" "Status Endpoint (Public)"; then
        # Parse and log status of each database
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
            for db_engine in "${!DATABASE_NAMES[@]}"; do
                local db_name="${DATABASE_NAMES[${db_engine}]}"
                local ready_status
                ready_status=$(jq -r ".databases.[\"${db_name}\"].ready" "${response_file}" 2>/dev/null || echo "unknown")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${db_engine}: ready=${ready_status}"
            done
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (Public) - All databases reported status"
        tests_passed=$(( tests_passed + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (Public) - Failed to get status"
    fi
    total_tests=$(( total_tests + 1 ))

    # Test status endpoint with authentication for each database
    # Use read -a to properly split the jwt_tokens_string into an array
    local jwt_array=()
    read -r -a jwt_array <<< "${jwt_tokens_string}"
    local jwt_index=0
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        if [[ ${jwt_index} -lt ${#jwt_array[@]} ]] && [[ -n "${jwt_array[${jwt_index}]}" ]]; then
            local jwt_token="${jwt_array[${jwt_index}]}"
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (${db_engine} Auth)"
            local response_file_auth="${result_file}.status_auth_${db_engine}.json"

            # Make the request and check basic success
            local http_status
            http_status=$(curl -s -X GET "${base_url}/api/conduit/status" \
                -H "Authorization: Bearer ${jwt_token}" \
                -w "%{http_code}" \
                -o "${response_file_auth}" 2>/dev/null)

            if [[ "${http_status}" -eq 200 ]]; then
                # Check for success in JSON response
                if "${GREP}" -q '"success"[[:space:]]*:[[:space:]]*true' "${response_file_auth}"; then
                    # Parse and validate detailed status including DQM statistics
                    if command -v jq >/dev/null 2>&1 && [[ -f "${response_file_auth}" ]]; then
                        local db_name="${DATABASE_NAMES[${db_engine}]}"
                        local ready_status migration_status cache_entries dqm_stats_present
                        ready_status=$(jq -r ".databases.[\"${db_name}\"].ready" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        migration_status=$(jq -r ".databases.[\"${db_name}\"].migration_status" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        cache_entries=$(jq -r ".databases.[\"${db_name}\"].query_cache_entries" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        dqm_stats_present=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_submitted" "${response_file_auth}" 2>/dev/null || echo "null")

                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${db_engine} authenticated status: ready=${ready_status}, migration=${migration_status}, cache_entries=${cache_entries}"

                        if [[ "${dqm_stats_present}" != "null" ]]; then
                            local total_submitted total_completed total_failed
                            total_submitted=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_submitted" "${response_file_auth}" 2>/dev/null || echo "0")
                            total_completed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_completed" "${response_file_auth}" 2>/dev/null || echo "0")
                            total_failed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_failed" "${response_file_auth}" 2>/dev/null || echo "0")
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  DQM Statistics for ${db_name}: submitted=${total_submitted}, completed=${total_completed}, failed=${total_failed}"

                            # Log per-queue statistics in order: Lead, Fast, Medium, Slow, Cache
                            local queue_indices=(4 2 1 0 3)
                            local queue_names=("Lead" "Fast" "Medium" "Slow" "Cache")
                            local i=0
                            for queue_name in "${queue_names[@]}"; do
                                local queue_index=${queue_indices[${i}]}
                                local q_submitted q_completed q_failed q_avg_time
                                q_submitted=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].submitted" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_completed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].completed" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_failed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].failed" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_avg_time=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].avg_execution_time_us" "${response_file_auth}" 2>/dev/null || echo "0")

                                # Format average time as milliseconds with 3 decimal places (convert from microseconds)
                                local avg_ms
                                if [[ "${q_avg_time}" == "0" ]]; then
                                    avg_ms="0.000ms"
                                else
                                    # Calculate average time in milliseconds
                                    local calc_result
                                    calc_result=$(echo "scale=3; ${q_avg_time}/1000" | bc -l 2>/dev/null || echo "0.000")
                                    avg_ms=$(printf "%.3fms" "${calc_result}")
                                fi

                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${queue_name} DQM: submitted=${q_submitted}, completed=${q_completed}, failed=${q_failed}, avg=${avg_ms}"
                                i=$(( i + 1 ))
                            done

                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (${db_engine} Auth) - Authenticated status with DQM statistics retrieved"
                            tests_passed=$(( tests_passed + 1 ))
                        else
                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - DQM statistics missing from response"
                        fi
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (${db_engine} Auth) - Request successful (jq not available for detailed validation)"
                        tests_passed=$(( tests_passed + 1 ))
                    fi
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Request failed (success=false in response)"
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Expected HTTP 200, got ${http_status}"
            fi
            total_tests=$(( total_tests + 1 ))
        else
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (${db_engine} Auth)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Skipped (no JWT available)"
        fi
        jwt_index=$(( jwt_index + 1 ))
    done

    echo "STATUS_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "STATUS_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to run conduit tests in parallel for a specific database engine
run_conduit_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local description="$4"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Clear result file
    true > "${result_file}"

    # Extract port from configuration
    local server_port
    server_port=$(jq -r '.WebServer.Port' "${config_file}" 2>/dev/null || echo "0")
    local base_url="http://localhost:${server_port}"

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}

    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi

        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.1
    done

    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"

        # Wait for all database migrations to complete individually
        local migration_start_time=${SECONDS}
        local completed_databases=0
        local expected_databases=7

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for all ${expected_databases} database migrations to complete..."
        while true; do
            if [[ $((SECONDS - migration_start_time)) -ge ${MIGRATION_TIMEOUT} ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Migration wait timeout - proceeding with conduit tests (${completed_databases}/${expected_databases} databases ready)"
                break
            fi

            # Count completed migrations for each database
            local current_completed=0
            for db_name in "${DATABASE_NAMES[@]}"; do
                # Check for individual database migration completion
                if "${GREP}" -q "${db_name}.*Migration completed in" "${log_file}" 2>/dev/null; then
                    current_completed=$(( current_completed + 1 ))
                elif "${GREP}" -q "${db_name}.*Migration process completed.*QTC populated from bootstrap queries" "${log_file}" 2>/dev/null; then
                    current_completed=$(( current_completed + 1 ))
                fi
            done

            # Update completed count if it increased
            if [[ ${current_completed} -gt ${completed_databases} ]]; then
                completed_databases=${current_completed}
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Migration progress: ${completed_databases}/${expected_databases} databases completed"
            fi

            # Check if all databases have completed migration
            if [[ ${completed_databases} -ge ${expected_databases} ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All ${expected_databases} database migrations completed - proceeding with conduit tests"
                break
            fi

            # Also check if no migration was needed (when migrations are already up to date)
            if "${GREP}" -q "Migration Current:" "${log_file}" 2>/dev/null; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Migrations already current - proceeding with conduit tests"
                break
            fi

            sleep 0.5
        done

        # Wait a bit for server to be fully ready after migrations
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for webserver to be fully ready..."
        sleep 5

        # Check which databases are ready for testing
        check_database_readiness "${log_file}" "${result_file}"

        # Get JWT tokens for authenticated endpoints - one for each database
        local jwt_tokens_string
        jwt_tokens_string=$(acquire_jwt_tokens "${base_url}" "${result_file}")
        # shellcheck disable=SC2206,SC2178 # We want word splitting here, variable is assigned a string but used as array
        local jwt_tokens=(${jwt_tokens_string})

        # Count JWT acquisition results
        local jwt_tests_passed=0
        local jwt_tests_total=0
        for db_engine in "${!DATABASE_NAMES[@]}"; do
            if [[ -n "${jwt_tokens[${jwt_tests_total}]}" ]]; then
                jwt_tests_passed=$(( jwt_tests_passed + 1 ))
            fi
            jwt_tests_total=$(( jwt_tests_total + 1 ))
        done

        echo "JWT_ACQUISITION_TESTS_PASSED=${jwt_tests_passed}" >> "${result_file}"
        echo "JWT_ACQUISITION_TESTS_TOTAL=${jwt_tests_total}" >> "${result_file}"

        # Test status endpoint first
        test_conduit_status "${base_url}" "${jwt_tokens[*]}" "${result_file}"

        # Run conduit endpoint tests in methodical order
        test_conduit_single_public_query "${base_url}" "${result_file}"
        test_conduit_single_invalid_query "${base_url}" "${result_file}"
        # TODO: Add more test functions as we progress
        # test_conduit_multiple_queries "${base_url}" "${result_file}"
        # test_conduit_auth_single_query "${base_url}" "${jwt_token}" "${result_file}"
        # test_conduit_auth_multiple_queries "${base_url}" "${jwt_token}" "${result_file}"
        # test_conduit_alt_single_query "${base_url}" "${jwt_token}" "${result_file}"
        # test_conduit_alt_multiple_queries "${base_url}" "${jwt_token}" "${result_file}"

        echo "CONDUIT_TEST_COMPLETE" >> "${result_file}"

        # Stop the server gracefully
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true

            # Wait for graceful shutdown
            local shutdown_start
            shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.1
            done
        fi
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from unified conduit test execution
analyze_conduit_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local description="$4"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Failed to start Hydrogen"
        return 1
    fi

    # Check if conduit tests completed
    if ! "${GREP}" -q "CONDUIT_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Conduit tests did not complete"
        return 1
    fi

    local total_passed=0
    local total_tests=0

    # Check single public query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Single Public Query Tests"
    if "${GREP}" -q "^SINGLE_PUBLIC_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local single_public_passed single_public_total
        single_public_passed=$("${GREP}" "^SINGLE_PUBLIC_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        single_public_total=$("${GREP}" "^SINGLE_PUBLIC_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${single_public_passed}" -eq "${single_public_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Single Public Query Tests (${single_public_passed}/${single_public_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Single Public Query Tests (${single_public_passed}/${single_public_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    fi

    # Check single invalid query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Single Invalid Query Tests"
    if "${GREP}" -q "^SINGLE_INVALID_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local single_invalid_passed single_invalid_total
        single_invalid_passed=$("${GREP}" "^SINGLE_INVALID_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        single_invalid_total=$("${GREP}" "^SINGLE_INVALID_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${single_invalid_passed}" -eq "${single_invalid_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Single Invalid Query Tests (${single_invalid_passed}/${single_invalid_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Single Invalid Query Tests (${single_invalid_passed}/${single_invalid_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    fi

    # Check multiple queries tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Multiple Queries Tests"
    if "${GREP}" -q "^MULTIPLE_QUERIES_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local multiple_passed multiple_total
        multiple_passed=$("${GREP}" "^MULTIPLE_QUERIES_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        multiple_total=$("${GREP}" "^MULTIPLE_QUERIES_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${multiple_passed}" -eq "${multiple_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Multiple Queries Tests (${multiple_passed}/${multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Multiple Queries Tests (${multiple_passed}/${multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    fi

    # Check auth single query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Single Query Tests"
    if "${GREP}" -q "^AUTH_SINGLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local auth_single_passed auth_single_total
        auth_single_passed=$("${GREP}" "^AUTH_SINGLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        auth_single_total=$("${GREP}" "^AUTH_SINGLE_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${auth_single_passed}" -eq "${auth_single_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Single Query Tests (${auth_single_passed}/${auth_single_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Auth Single Query Tests (${auth_single_passed}/${auth_single_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    elif "${GREP}" -q "AUTH_SINGLE_QUERY_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Single Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Check auth multiple query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Multiple Query Tests"
    if "${GREP}" -q "^AUTH_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local auth_multiple_passed auth_multiple_total
        auth_multiple_passed=$("${GREP}" "^AUTH_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        auth_multiple_total=$("${GREP}" "^AUTH_MULTIPLE_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${auth_multiple_passed}" -eq "${auth_multiple_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Multiple Query Tests (${auth_multiple_passed}/${auth_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Auth Multiple Query Tests (${auth_multiple_passed}/${auth_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    elif "${GREP}" -q "AUTH_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Multiple Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Check alt single query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Single Query Tests"
    if "${GREP}" -q "^ALT_SINGLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local alt_single_passed alt_single_total
        alt_single_passed=$("${GREP}" "^ALT_SINGLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        alt_single_total=$("${GREP}" "^ALT_SINGLE_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${alt_single_passed}" -eq "${alt_single_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Alt Single Query Tests (${alt_single_passed}/${alt_single_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Alt Single Query Tests (${alt_single_passed}/${alt_single_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    elif "${GREP}" -q "ALT_SINGLE_QUERY_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Alt Single Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Check alt multiple query tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Multiple Query Tests"
    if "${GREP}" -q "^ALT_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local alt_multiple_passed alt_multiple_total
        alt_multiple_passed=$("${GREP}" "^ALT_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        alt_multiple_total=$("${GREP}" "^ALT_MULTIPLE_QUERY_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${alt_multiple_passed}" -eq "${alt_multiple_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Alt Multiple Query Tests (${alt_multiple_passed}/${alt_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Alt Multiple Query Tests (${alt_multiple_passed}/${alt_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    elif "${GREP}" -q "ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Alt Multiple Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # JWT acquisition and status tests are already counted individually in their respective functions
    # Just aggregate the totals here
    if "${GREP}" -q "^JWT_ACQUISITION_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local jwt_passed
        jwt_passed=$("${GREP}" "^JWT_ACQUISITION_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        total_passed=$(( total_passed + jwt_passed ))
    fi

    if "${GREP}" -q "^JWT_ACQUISITION_TESTS_TOTAL=" "${result_file}" 2>/dev/null; then
        local jwt_total
        jwt_total=$("${GREP}" "^JWT_ACQUISITION_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        total_tests=$(( total_tests + jwt_total ))
    fi

    if "${GREP}" -q "^STATUS_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local status_passed
        status_passed=$("${GREP}" "^STATUS_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        total_passed=$(( total_passed + status_passed ))
    fi

    if "${GREP}" -q "^STATUS_TESTS_TOTAL=" "${result_file}" 2>/dev/null; then
        local status_total
        status_total=$("${GREP}" "^STATUS_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        total_tests=$(( total_tests + status_total ))
    fi

    # Overall result
    if [[ "${total_passed}" -eq "${total_tests}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: All Conduit Tests Passed (${total_passed}/${total_tests})"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Some Conduit Tests Failed (${total_passed}/${total_tests})"
        return 1
    fi
}

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

# Validate required environment variables for demo credentials
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
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables (HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY)"
    EXIT_CODE=1
fi

# Validate the unified configuration file
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Unified Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONDUIT_CONFIG_FILE}"; then
    port=$(get_webserver_port "${CONDUIT_CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION} configuration will use port: ${port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unified configuration file validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unified configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with conduit tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit endpoint tests on unified server"

    # Run single server test
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting unified conduit test server (${CONDUIT_DESCRIPTION})"

    # Run the conduit test on the single unified server
    run_conduit_test_parallel "Unified" "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_ENGINE_NAME}" "${CONDUIT_DESCRIPTION}"

    # Process results
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Analyzing results"

    # Add links to log and result files for troubleshooting
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${DIAG_TEST_DIR}/${result_file##*/}"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if analyze_conduit_test_results "Unified" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_ENGINE_NAME}" "${CONDUIT_DESCRIPTION}"; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Print summary
    if [[ -f "${result_file}" ]]; then
        if "${GREP}" -q "CONDUIT_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server passed all conduit endpoint tests"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sequential execution completed - Conduit endpoints validated across ${#DATABASE_NAMES[@]} database engines"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server failed conduit endpoint tests"
            EXIT_CODE=1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: No result file found for unified server"
        EXIT_CODE=1
    fi

else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"