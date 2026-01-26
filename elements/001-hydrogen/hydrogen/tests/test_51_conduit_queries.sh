#!/usr/bin/env bash

# Test: Conduit Multiple Queries Endpoint
# Tests the /api/conduit/queries endpoint for multiple parallel query execution
# Launches unified server with 7 database engines and tests multiple query functionality

# FUNCTIONS
# validate_conduit_request()
# test_conduit_status_public()
# test_conduit_status_authenticated()
# test_conduit_status_validation()
# test_conduit_multiple_queries()
# run_conduit_test_unified()

# CHANGELOG
# 1.1.0 - 2026-01-25 - Updated to follow Test 50 structure
#                    - Added comprehensive status endpoint testing
#                    - Added print_box separators for better readability
#                    - Updated to use same patterns as matured Test 50
#                    - Added test for too many queries (exceeding MAX_QUERIES_PER_REQUEST)

set -euo pipefail

# Test Configuration
TEST_NAME="Conduit Multiple Queries"
TEST_ABBR="CMQ"
TEST_NUMBER="51"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_51_conduit_queries.json"
CONDUIT_LOG_SUFFIX="conduit_queries"
CONDUIT_DESCRIPTION="CMQ"

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

# Function to test conduit status endpoint public access
test_conduit_status_public() {
    local base_url="$1"
    local result_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Status Endpoint (Public)"

    # Test status endpoint without JWT (public access)
    local response_file="${result_file}.status_public.json"

    local http_status
    http_status=$(curl -s -X GET "${base_url}/api/conduit/status" -w "%{http_code}" -o "${response_file}" 2>/dev/null)
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"
    if [[ "${http_status}" == "200" ]]; then
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
            local success_val
            success_val=$(jq -r '.success' "${response_file}" 2>/dev/null || echo "unknown")
            if [[ "${success_val}" == "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${response_file}"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Status endpoint public test passed"
                echo "STATUS_PUBLIC_TESTS_PASSED=1" >> "${result_file}"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status endpoint public test failed - success=${success_val}"
                echo "STATUS_PUBLIC_TESTS_PASSED=0" >> "${result_file}"
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status endpoint public test failed - jq not available"
            echo "STATUS_PUBLIC_TESTS_PASSED=0" >> "${result_file}"
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status endpoint public test failed - HTTP ${http_status}"
        echo "STATUS_PUBLIC_TESTS_PASSED=0" >> "${result_file}"
    fi
    echo "STATUS_PUBLIC_TESTS_TOTAL=1" >> "${result_file}"
}

# Function to test conduit status endpoint authenticated access
test_conduit_status_authenticated() {
    local base_url="$1"
    local result_file="$2"

    # Test status endpoint with JWT (authenticated access)
    local response_file="${result_file}.status_auth.json"

    # Get JWT for first database (PostgreSQL)
    local jwt_token=""
    if [[ -n "${JWT_TOKENS_RESULT_51[0]:-}" ]]; then
        jwt_token="${JWT_TOKENS_RESULT_51[0]}"
    fi

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_conduit_request "${base_url}/api/conduit/status" "GET" "" "200" "${response_file}" "${jwt_token}" "Authenticated status request" "true"; then
        # Show summary of authenticated response per database
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Authenticated status response summary"
            for db_engine in "${!DATABASE_NAMES[@]}"; do
                local db_name="${DATABASE_NAMES[${db_engine}]}"
                local db_name_pad="${DATABASE_NAMES[${db_engine}]}     "
                local db_status
                db_status=$(jq -r ".databases.\"${db_name}\"" "${response_file}" 2>/dev/null || echo "not_found")
                if [[ "${db_status}" != "null" && "${db_status}" != "not_found" ]]; then
                    local query_cache_entries
                    query_cache_entries=$(jq -r ".query_cache_entries" <<< "${db_status}" 2>/dev/null || echo "0")
                    local queue_stats=()
                    local expected_queues=("slow" "medium" "fast" "cache" "lead")
                    for queue_type in "${expected_queues[@]}"; do
                        local completed
                        completed=$(jq -r ".dqm_statistics.per_queue_stats[] | select(.queue_type == \"${queue_type}\") | .completed" <<< "${db_status}" 2>/dev/null || echo "Missing")
                        queue_stats+=(" ${queue_type}(${completed})")
                    done
                    local queue_stats_str
                    queue_stats_str=$(IFS=',' ; echo "${queue_stats[*]}")
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_name_pad::9}: QTC(${query_cache_entries}),${queue_stats_str}"
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${db_name}: not in response"
                fi
            done
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Status endpoint authenticated test passed"
        echo "STATUS_AUTH_TESTS_PASSED=1" >> "${result_file}"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status endpoint authenticated test failed"
        echo "STATUS_AUTH_TESTS_PASSED=0" >> "${result_file}"
    fi
    echo "STATUS_AUTH_TESTS_TOTAL=1" >> "${result_file}"
}

# Function to validate conduit status endpoint database readiness matches log monitoring
test_conduit_status_validation() {
    local base_url="$1"
    local result_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Status Endpoint Validation"

    local response_file="${result_file}.status_public.json"

    # Verify status response content
    if ! command -v jq >/dev/null 2>&1 || [[ ! -f "${response_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status validation failed - jq not available or response file missing"
        echo "STATUS_VALIDATION_TESTS_PASSED=0" >> "${result_file}"
        echo "STATUS_VALIDATION_TESTS_TOTAL=1" >> "${result_file}"
        return 1
    fi

    # Extract databases from public status response that are ready=true
    local status_databases=()
    while IFS= read -r db; do
        if [[ -n "${db}" ]]; then
            status_databases+=("${db}")
        fi
    done < <(jq -r '.databases | to_entries | map(select(.value.ready == true)) | map(.key) | .[]' "${response_file}" 2>/dev/null | sort || true)

    # Get databases confirmed ready by log monitoring
    local log_ready_databases=()
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        if "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            log_ready_databases+=("${DATABASE_NAMES[${db_engine}]}")
        fi
    done


    # Sort for comparison
    # shellcheck disable=SC2207 # Command substitution with array expansion requires subshell
    log_ready_databases=($(printf '%s\n' "${log_ready_databases[@]}" | sort))

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Log-confirmed ready databases: ${log_ready_databases[*]}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Status-reported ready databases: ${status_databases[*]}"

    # Check if status-reported databases match log-confirmed databases
    local status_test_passed=true
    local status_db_count=${#status_databases[@]}
    local log_db_count=${#log_ready_databases[@]}

    # Check each status-reported database
    for db in "${status_databases[@]}"; do
        # Check if this database was confirmed by log
        local found=false
        for log_db in "${log_ready_databases[@]}"; do
            if [[ "${db}" == "${log_db}" ]]; then
                found=true
                break
            fi
        done
        if [[ "${found}" != true ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: Status endpoint reports database ${db} not confirmed by log monitoring"
            status_test_passed=false
        fi
    done

    # Check if all log-confirmed databases are reported by status
    for log_db in "${log_ready_databases[@]}"; do
        local found=false
        for db in "${status_databases[@]}"; do
            if [[ "${log_db}" == "${db}" ]]; then
                found=true
                break
            fi
        done
        if [[ "${found}" != true ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: Log-confirmed database ${log_db} not reported by status endpoint"
            status_test_passed=false
        fi
    done

    if [[ ${status_db_count} -ne ${log_db_count} ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: Status endpoint reports ${status_db_count} databases, log confirmed ${log_db_count} databases"
        status_test_passed=false
    fi

    # Also verify that each reported database shows ready: true
    for db in "${status_databases[@]}"; do
        local ready_status
        ready_status=$(jq -r ".databases.\"${db}\".ready" "${response_file}" 2>/dev/null)
        if [[ "${ready_status}" != "true" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Database ${db} shows ready: ${ready_status} (expected: true)"
            status_test_passed=false
        fi
    done

    if [[ "${status_test_passed}" == true ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Status endpoint validation passed - databases ready and matching"
        echo "STATUS_VALIDATION_TESTS_PASSED=1" >> "${result_file}"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Status endpoint validation failed - database readiness issues detected"
        echo "STATUS_VALIDATION_TESTS_PASSED=0" >> "${result_file}"
    fi
    echo "STATUS_VALIDATION_TESTS_TOTAL=1" >> "${result_file}"
}

# Function to test conduit multiple queries endpoint across ready databases
test_conduit_multiple_queries() {
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

        print_box "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing against ${db_engine}"
        
        # Test 1: Normal query batch - Get Themes + Get Icons + Get Number Range
        local payload_normal
        payload_normal=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 53,
      "params": {}
    },
    {
      "query_ref": 54,
      "params": {}
    },
    {
      "query_ref": 55,
      "params": {
        "INTEGER": {"START": 500, "FINISH": 600}
      }
    }
  ]
}
EOF
)

        local response_file_normal="${result_file}.queries_normal_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_normal}" "200" "${response_file_normal}" "" "${db_engine} Normal Queries" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 2: Duplicate queries - should deduplicate
        local payload_duplicate
        payload_duplicate=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 53,
      "params": {}
    },
    {
      "query_ref": 53,
      "params": {}
    },
    {
      "query_ref": 54,
      "params": {}
    }
  ]
}
EOF
)

        local response_file_duplicate="${result_file}.queries_duplicate_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_duplicate}" "200" "${response_file_duplicate}" "" "${db_engine} Duplicate Queries" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 3: Mixed valid and invalid queries
        local payload_mixed
        payload_mixed=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 53,
      "params": {}
    },
    {
      "query_ref": -100,
      "params": {}
    },
    {
      "query_ref": 54,
      "params": {}
    }
  ]
}
EOF
)

        local response_file_mixed="${result_file}.queries_mixed_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_mixed}" "422" "${response_file_mixed}" "" "${db_engine} Mixed Queries" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 4: Too many queries (exceed MAX_QUERIES_PER_REQUEST)
        local too_many_queries=""
        for ((i=0; i<25; i++)); do
            local start=$((i * 10))
            local finish=$((start + 10))
            if [[ -n "${too_many_queries}" ]]; then
                too_many_queries="${too_many_queries},"
            fi
            too_many_queries="${too_many_queries}{\"query_ref\": 55, \"params\": {\"INTEGER\": {\"START\": ${start}, \"FINISH\": ${finish}}}}"
        done

        local payload_too_many
        payload_too_many=$(cat <<EOF
{
"database": "${db_name}",
"queries": [${too_many_queries}]
}
EOF
)

        local response_file_too_many="${result_file}.queries_too_many_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_too_many}" "429" "${response_file_too_many}" "" "${db_engine} Too Many Queries" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 5: Invalid JSON
        local payload_invalid_json
        payload_invalid_json=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 53,
      "params": {}
EOF
)

        local response_file_invalid_json="${result_file}.queries_invalid_json_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_invalid_json}" "400" "${response_file_invalid_json}" "" "${db_engine} Invalid JSON" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 6: Invalid database name
        local payload_invalid_db
        payload_invalid_db=$(cat <<EOF
{
  "database": "nonexistent_database",
  "queries": [
    {
      "query_ref": 53,
      "params": {}
    }
  ]
}
EOF
)

        local response_file_invalid_db="${result_file}.queries_invalid_db_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_invalid_db}" "400" "${response_file_invalid_db}" "" "${db_engine} Invalid Database" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 7: Query with invalid parameters
        local payload_invalid_params
        payload_invalid_params=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 55,
      "params": {
        "INTEGER": {"START": "invalid", "FINISH": 600}
      }
    }
  ]
}
EOF
)

        local response_file_invalid_params="${result_file}.queries_invalid_params_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_invalid_params}" "400" "${response_file_invalid_params}" "" "${db_engine} Invalid Parameters" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test 8: Empty queries array
        local payload_empty
        payload_empty=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": []
}
EOF
)

        local response_file_empty="${result_file}.queries_empty_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_empty}" "200" "${response_file_empty}" "" "${db_engine} Empty Queries" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
    done

    echo "MULTIPLE_QUERIES_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "MULTIPLE_QUERIES_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to run conduit multiple queries tests on unified server
run_conduit_test_unified() {
    local config_file="$1"
    local log_suffix="$2"
    local description="$3"

    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Start the unified server and get base_url and pid
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

    print_message "51" "0" "Server log location: build/tests/logs/test_51_${TIMESTAMP}_conduit_queries.log"

    # Acquire JWT tokens for authenticated status testing
    acquire_jwt_tokens "${base_url}" "${result_file}"

    # Run conduit status endpoint tests
    test_conduit_status_public "${base_url}" "${result_file}"
    test_conduit_status_validation "${base_url}" "${result_file}"
    test_conduit_status_authenticated "${base_url}" "${result_file}"

    # Run conduit multiple queries endpoint tests
    test_conduit_multiple_queries "${base_url}" "${result_file}"

    echo "CONDUIT_TEST_COMPLETE" >> "${result_file}"

    # Shutdown the server
    shutdown_conduit_server "${hydrogen_pid}" "${result_file}"
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
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONDUIT_CONFIG_FILE}"; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Process Configuration File"
    port=$(get_webserver_port "${CONDUIT_CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION} configuration will use port: ${port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unified configuration file validated successfully"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unified configuration file validation failed"
    EXIT_CODE=1
fi
TEST_NAME="Conduit Multiple Queries  {BLUE}databases: ${#DATABASE_NAMES[@]}{RESET}"

# Only proceed with conduit tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Run single server test
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting conduit test server (${CONDUIT_DESCRIPTION})"

    # Run the conduit test on the single unified server
    run_conduit_test_unified "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"

    # Process results
    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"

    # Add links to log and result files for troubleshooting
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Results: ${DIAG_TEST_DIR}/${result_file##*/}"
else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit multiple queries endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
