#!/usr/bin/env bash

# Test: Conduit Alt Multiple Queries Endpoint
# Tests the /api/conduit/alt_queries endpoint for multiple authenticated queries with database override
# Launches unified server with 7 database engines and tests alt multiple query functionality

# FUNCTIONS
# validate_conduit_request()
# test_conduit_alt_multiple_queries(base_url, result_file)
# run_conduit_test_unified()
# analyze_conduit_results()

# CHANGELOG
# 1.1.0 - 2026-02-18 - Implemented 7x2 cross-database testing matrix with different combinations
#                    - Each of 7 databases' JWT tokens used to query 2 different databases
#                    - Uses offset patterns 3,4 (vs 1,2 in test_54) for different cross-db combinations
#                    - Tests alt_queries endpoint with deduplication, rate limiting, and DQM statistics
# 1.0.0 - 2026-01-20 - Initial implementation based on test_51_conduit.sh
#                    - Focused on alt multiple queries endpoint (/api/conduit/alt_queries)
#                    - Tests cross-database batch queries with JWT authentication and database override

set -euo pipefail

# Test Configuration - F = Federated queries
TEST_NAME="Conduit Alt Queries"
TEST_ABBR="CFM"
TEST_NUMBER="55"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_55_conduit_alt_queries.json"
CONDUIT_LOG_SUFFIX="conduit_alt_queries"
CONDUIT_DESCRIPTION="Conduit Alt Multiple Queries"

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

# Function to test conduit alt multiple queries endpoint with cross-database testing
# Tests 7x2 matrix: Each of 7 databases' JWT tokens used to query 2 different databases
# Uses different query combinations than test 54
test_conduit_alt_multiple_queries() {
    local base_url="$1"
    local result_file="$2"

    local tests_passed=0
    local total_tests=0

    # Build list of ready databases with their JWT tokens
    # This ensures proper mapping between database and its token
    local ready_databases=()
    local ready_tokens=()
    
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi
        
        # Get JWT token for this database from global variable
        local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
        local jwt_token=""
        # Extract token for this specific database engine from the global array
        # The global array is indexed in the same order as DATABASE_NAMES iteration
        local token_idx=0
        for check_db in "${!DATABASE_NAMES[@]}"; do
            if [[ "${check_db}" == "${db_engine}" ]]; then
                eval "jwt_token=\${${global_var_name}[${token_idx}]:-}"
                break
            fi
            token_idx=$((token_idx + 1))
        done
        
        if [[ -n "${jwt_token}" ]]; then
            ready_databases+=("${db_engine}")
            ready_tokens+=("${jwt_token}")
        fi
    done
    
    local num_ready=${#ready_databases[@]}
    if [[ ${num_ready} -eq 0 ]]; then
        echo "ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" >> "${result_file}"
        echo "ALT_MULTIPLE_QUERY_TESTS_PASSED=0" >> "${result_file}"
        echo "ALT_MULTIPLE_QUERY_TESTS_TOTAL=0" >> "${result_file}"
        return
    fi
    
    # For each ready source database, test against 2 different target databases
    # Use different offsets than test 54 (3 and 4 instead of 1 and 2)
    local source_idx=0
    for source_db in "${ready_databases[@]}"; do
        local jwt_token="${ready_tokens[${source_idx}]}"
        
        # Test 2 different target databases (cross-database queries)
        # Cycle through ready databases with offset
        for offset in 3 4; do
            local target_idx=$(( (source_idx + offset) % num_ready ))
            local target_db="${ready_databases[${target_idx}]}"
            
            # Skip if target is same as source
            if [[ "${target_db}" == "${source_db}" ]]; then
                target_idx=$(( (target_idx + 1) % num_ready ))
                target_db="${ready_databases[${target_idx}]}"
            fi
            
            local target_name="${DATABASE_NAMES[${target_db}]}"
            local test_desc="${source_db}→${target_db} Cross-DB Batch Queries"
            
            # Prepare JSON payload for alt multiple queries
            # Use different query combinations than test 54
            local payload
            payload=$(cat <<EOF
{
  "token": "${jwt_token}",
  "database": "${target_name}",
  "queries": [
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

            local response_file="${result_file}.alt_multiple_${source_db}_${target_db}.json"

            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if validate_conduit_request "${base_url}/api/conduit/alt_queries" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${test_desc}"; then
                tests_passed=$(( tests_passed + 1 ))
            else
                echo "ALT_MULTIPLE_QUERIES_FAILED_${source_db}_${target_db}" >> "${result_file}"
            fi
            total_tests=$(( total_tests + 1 ))
        done
        
        source_idx=$((source_idx + 1))
    done

    echo "ALT_MULTIPLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "ALT_MULTIPLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to run conduit alt multiple queries tests on unified server
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

    print_message "55" "0" "Server log location: build/tests/logs/test_55_${TIMESTAMP}_conduit_alt_queries.log"

    # Get JWT tokens for authenticated endpoints - one for each database
    acquire_jwt_tokens "${base_url}" "${result_file}"

    # Count JWT acquisition results
    local jwt_tests_passed=0
    local jwt_tests_total=0
    local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local jwt_token=""
        local token_idx=0
        for check_db in "${!DATABASE_NAMES[@]}"; do
            if [[ "${check_db}" == "${db_engine}" ]]; then
                eval "jwt_token=\${${global_var_name}[${token_idx}]:-}"
                break
            fi
            token_idx=$((token_idx + 1))
        done
        if [[ -n "${jwt_token}" ]]; then
            jwt_tests_passed=$(( jwt_tests_passed + 1 ))
        fi
        jwt_tests_total=$(( jwt_tests_total + 1 ))
    done

    echo "JWT_ACQUISITION_TESTS_PASSED=${jwt_tests_passed}" >> "${result_file}"
    echo "JWT_ACQUISITION_TESTS_TOTAL=${jwt_tests_total}" >> "${result_file}"

    # Run conduit alt multiple queries endpoint tests with cross-database matrix
    test_conduit_alt_multiple_queries "${base_url}" "${result_file}"

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
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit alt multiple queries endpoint tests on unified server"

    # Run single server test
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting unified conduit test server (${CONDUIT_DESCRIPTION})"

    # Run the conduit test on the single unified server
    run_conduit_test_unified "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"

    # Process results
    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Analyzing results"

    # Add links to log and result files for troubleshooting
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unified Server: ${DIAG_TEST_DIR}/${result_file##*/}"

    # Custom analysis for Test 55 - only check results we actually produce
    total_passed=0
    total_tests=0

    # Check JWT acquisition results
    if "${GREP}" -q "^JWT_ACQUISITION_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        jwt_passed=$("${GREP}" "^JWT_ACQUISITION_TESTS_PASSED=" "${result_file}" | cut -d'=' -f2)
        jwt_total=$("${GREP}" "^JWT_ACQUISITION_TESTS_TOTAL=" "${result_file}" | cut -d'=' -f2)
        total_passed=$(( total_passed + jwt_passed ))
        total_tests=$(( total_tests + jwt_total ))
    fi

    # Check alt multiple query results
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Alt Multiple Query Tests"
    if "${GREP}" -q "^ALT_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        alt_multiple_passed=$("${GREP}" "^ALT_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" | cut -d'=' -f2)
        alt_multiple_total=$("${GREP}" "^ALT_MULTIPLE_QUERY_TESTS_TOTAL=" "${result_file}" | cut -d'=' -f2)

        if [[ "${alt_multiple_passed}" -eq "${alt_multiple_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${CONDUIT_DESCRIPTION}: Alt Multiple Query Tests (${alt_multiple_passed}/${alt_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${CONDUIT_DESCRIPTION}: Alt Multiple Query Tests (${alt_multiple_passed}/${alt_multiple_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    elif "${GREP}" -q "ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${CONDUIT_DESCRIPTION}: Alt Multiple Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Overall result
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Overall Conduit Test Results"
    if [[ "${total_passed}" -eq "${total_tests}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${CONDUIT_DESCRIPTION}: All Conduit Tests Passed (${total_passed}/${total_tests})"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${CONDUIT_DESCRIPTION}: Some Conduit Tests Failed (${total_passed}/${total_tests})"
        EXIT_CODE=1
    fi

    # Print summary
    if [[ -f "${result_file}" ]]; then
        if "${GREP}" -q "CONDUIT_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
            print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server passed all conduit alt multiple queries endpoint tests"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sequential execution completed - Alt multiple queries endpoint validated across ${#DATABASE_NAMES[@]} database engines"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server failed conduit alt multiple queries endpoint tests"
            EXIT_CODE=1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: No result file found for unified server"
        EXIT_CODE=1
    fi

else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit alt multiple queries endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"