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
# 1.0.0 - 2026-01-20 - Initial implementation based on test_51_conduit.sh
#                    - Focused on authenticated single query endpoint (/api/conduit/auth_query)
#                    - Tests valid auth queries, invalid JWT, missing auth, malformed JWT, and invalid query refs

set -euo pipefail

# Test Configuration
TEST_NAME="Secure Single Query"
TEST_ABBR="SSQ"
TEST_NUMBER="52"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_52_conduit_auth_query.json"
CONDUIT_LOG_SUFFIX="conduit_auth_query"
CONDUIT_DESCRIPTION="Conduit Auth Single Query"

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

# Function to test conduit auth query endpoint comprehensively across ready databases
test_conduit_auth_query_comprehensive() {
    local base_url="$1"
    local jwt_tokens_string="$2"  # Space-separated string of JWT tokens (one per database)
    local result_file="$3"

    # Parse JWT tokens
    local jwt_tokens=()
    read -r -a jwt_tokens <<< "${jwt_tokens_string}"

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
        local jwt_token="${jwt_tokens[${jwt_index}]}"

        if [[ -z "${jwt_token}" ]]; then
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Auth Query Comprehensive (${db_engine})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Auth Query Comprehensive (${db_engine}) - No JWT token available"
            continue
        fi

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
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload_invalid_ref}" "200" "${response_file_invalid_ref}" "${jwt_token}" "${db_engine} Auth Query Invalid Ref" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

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

    # Get JWT tokens for authenticated endpoints - one for each database
    local jwt_tokens_string
    jwt_tokens_string=$(acquire_jwt_tokens "${base_url}" "${result_file}")
    local jwt_tokens=()
    read -r -a jwt_tokens <<< "${jwt_tokens_string}"

    # Count JWT acquisition results
    local jwt_tests_passed=0
    local jwt_tests_total=0
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        if [[ ${jwt_tests_total} -lt ${#jwt_tokens[@]} ]] && [[ -n "${jwt_tokens[${jwt_tests_total}]}" ]]; then
            jwt_tests_passed=$(( jwt_tests_passed + 1 ))
        fi
        jwt_tests_total=$(( jwt_tests_total + 1 ))
    done

    echo "JWT_ACQUISITION_TESTS_PASSED=${jwt_tests_passed}" >> "${result_file}"
    echo "JWT_ACQUISITION_TESTS_TOTAL=${jwt_tests_total}" >> "${result_file}"

    # Run conduit auth single query endpoint tests
    test_conduit_auth_query_comprehensive "${base_url}" "${jwt_tokens[*]}" "${result_file}"

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
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit authenticated single query endpoint tests on unified server"

    # Run single server test
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting unified conduit test server (${CONDUIT_DESCRIPTION})"

    # Run the conduit test on the single unified server
    run_conduit_test_unified "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"

    # Process results
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Analyzing results"

    # Add links to log and result files for troubleshooting
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${DIAG_TEST_DIR}/${result_file##*/}"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if analyze_conduit_results "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Print summary
    if [[ -f "${result_file}" ]]; then
        if "${GREP}" -q "CONDUIT_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server passed all conduit authenticated single query endpoint tests"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sequential execution completed - Authenticated single query endpoint validated across ${#DATABASE_NAMES[@]} database engines"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server failed conduit authenticated single query endpoint tests"
            EXIT_CODE=1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: No result file found for unified server"
        EXIT_CODE=1
    fi

else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit authenticated single query endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"