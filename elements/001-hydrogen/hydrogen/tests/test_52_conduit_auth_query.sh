#!/usr/bin/env bash

# Test: Conduit Authenticated Single Query Endpoint
# Tests the /api/conduit/auth_query endpoint for single authenticated query execution
# Launches unified server with 7 database engines and tests authenticated single query functionality

# FUNCTIONS
# validate_conduit_request()
# test_conduit_auth_query_comprehensive()
# run_conduit_test_unified()
# analyze_conduit_results()

# CHANGELOG
# 1.1.0 - 2026-01-27 - Updated to follow Test 50 structure
#                    - Added comprehensive status endpoint testing
#                    - Added print_box separators for better readability
#                    - Updated to use same patterns as matured Test 50
#                    - Added test for expired JWT
#                    - Added test for revoked JWT
#                    - Added test for missing database in JWT claims
# 1.0.0 - 2026-01-20 - Initial implementation based on test_51_conduit.sh
#                    - Focused on authenticated single query endpoint (/api/conduit/auth_query)
#                    - Tests valid auth queries, invalid JWT, missing auth, malformed JWT, and invalid query refs

set -euo pipefail

# Test Configuration
TEST_NAME="Secure Single Query"
TEST_ABBR="SSQ"
TEST_NUMBER="52"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_52_conduit_auth_query.json"
CONDUIT_LOG_SUFFIX="conduit_auth_query"
CONDUIT_DESCRIPTION="SSQ"

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
    if [[ -n "${JWT_TOKENS_RESULT_52[0]:-}" ]]; then
        jwt_token="${JWT_TOKENS_RESULT_52[0]}"
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

# Function to test conduit auth query endpoint comprehensively across ready databases
test_conduit_auth_query_comprehensive() {
    local base_url="$1"
    local result_file="$2"

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    local jwt_index=0
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            jwt_index=$(( jwt_index + 1 ))
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"
        # Access the global JWT tokens array for this test
        local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
        local jwt_tokens=()
        eval "jwt_tokens=(\${${global_var_name}[@]})"
        local jwt_token="${jwt_tokens[${jwt_index}]:-}"

        if [[ -z "${jwt_token}" ]]; then
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Auth Query Comprehensive (${db_engine})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Auth Query Comprehensive (${db_engine}) - No JWT token available"
            continue
        fi

        print_box "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing against ${db_engine}"
        
        # Test 1: Valid authenticated query
        local payload_valid
        payload_valid=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_valid="${result_file}.auth_comprehensive_valid_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_valid}" "200" "${response_file_valid}" "${jwt_token}" "${db_engine} Auth Query Valid: Get Lookups List"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 2: Invalid JWT token (malformed)
        local invalid_jwt="invalid.jwt.token"
        local payload_invalid_jwt
        payload_invalid_jwt=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_invalid_jwt="${result_file}.auth_comprehensive_invalid_jwt_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_invalid_jwt}" "401" "${response_file_invalid_jwt}" "${invalid_jwt}" "${db_engine} Auth Query Invalid JWT" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 3: Missing authentication (no JWT token)
        local payload_no_auth
        payload_no_auth=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_no_auth="${result_file}.auth_comprehensive_no_auth_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_no_auth}" "401" "${response_file_no_auth}" "" "${db_engine} Auth Query No Auth" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 4: Expired JWT token
        local expired_jwt="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE1MTYyMzkwMjJ9.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"
        local payload_expired_jwt
        payload_expired_jwt=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_expired_jwt="${result_file}.auth_comprehensive_expired_jwt_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_expired_jwt}" "401" "${response_file_expired_jwt}" "${expired_jwt}" "${db_engine} Auth Query Expired JWT" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 5: Malformed JWT (valid base64 but invalid JSON)
        local malformed_jwt="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.payload.signature"
        local payload_malformed_jwt
        payload_malformed_jwt=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_malformed_jwt="${result_file}.auth_comprehensive_malformed_jwt_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_malformed_jwt}" "401" "${response_file_malformed_jwt}" "${malformed_jwt}" "${db_engine} Auth Query Malformed JWT" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 6: Invalid query reference (should fail even with valid JWT)
        local payload_invalid_ref
        payload_invalid_ref=$(cat <<EOF
{
  "query_ref": -100,
  "params": {}
}
EOF
)

        local response_file_invalid_ref="${result_file}.auth_comprehensive_invalid_ref_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_invalid_ref}" "200" "${response_file_invalid_ref}" "${jwt_token}" "${db_engine} Auth Query Invalid Ref" "\"fail\""; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 7: Missing database in JWT claims
        local no_db_jwt="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"
        local payload_no_db_jwt
        payload_no_db_jwt=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_no_db_jwt="${result_file}.auth_comprehensive_no_db_jwt_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_no_db_jwt}" "401" "${response_file_no_db_jwt}" "${no_db_jwt}" "${db_engine} Auth Query Missing DB Claim" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 8: Revoked JWT token
        local revoked_jwt="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyLCJkYXRhYmFzZSI6ImRlbW9fYXBwIiwicmV2b2tlZCI6dHJ1ZX0.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"
        local payload_revoked_jwt
        payload_revoked_jwt=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file_revoked_jwt="${result_file}.auth_comprehensive_revoked_jwt_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_revoked_jwt}" "401" "${response_file_revoked_jwt}" "${revoked_jwt}" "${db_engine} Auth Query Revoked JWT" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        jwt_index=$(( jwt_index + 1 ))
    done

    echo "AUTH_QUERY_COMPREHENSIVE_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "AUTH_QUERY_COMPREHENSIVE_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to run conduit auth single query tests on unified server
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
        return 1
    fi

    # Parse server info
    local base_url hydrogen_pid
    base_url=$(echo "${server_info}" | awk -F: '{print $1":"$2":"$3}')
    hydrogen_pid=$(echo "${server_info}" | awk -F: '{print $4}')

    print_message "52" "0" "Server log location: build/tests/logs/test_52_${TIMESTAMP}_conduit_auth_query.log"

    # Acquire JWT tokens for authenticated testing
    acquire_jwt_tokens "${base_url}" "${result_file}"

    # Run conduit status endpoint tests
    test_conduit_status_public "${base_url}" "${result_file}"
    test_conduit_status_validation "${base_url}" "${result_file}"
    test_conduit_status_authenticated "${base_url}" "${result_file}"

    # Count JWT acquisition results
    local jwt_tests_passed=0
    local jwt_tests_total=0
    local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
    local jwt_tokens=()
    eval "jwt_tokens=(\${${global_var_name}[@]})"
    for ((i=0; i<${#DATABASE_NAMES[@]}; i++)); do
        if [[ -n "${jwt_tokens[${i}]:-}" ]]; then
            jwt_tests_passed=$(( jwt_tests_passed + 1 ))
        fi
        jwt_tests_total=$(( jwt_tests_total + 1 ))
    done

    echo "JWT_ACQUISITION_TESTS_PASSED=${jwt_tests_passed}" >> "${result_file}"
    echo "JWT_ACQUISITION_TESTS_TOTAL=${jwt_tests_total}" >> "${result_file}"

    # Run conduit auth single query endpoint tests
    test_conduit_auth_query_comprehensive "${base_url}" "${result_file}"

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
    port=$(get_webserver_port "${CONDUIT_CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION} configuration will use port: ${port}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified configuration file validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
fi               

TEST_NAME="Secure Single Query  {BLUE}databases: ${#DATABASE_NAMES[@]}{RESET}"

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
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit authenticated single query endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"