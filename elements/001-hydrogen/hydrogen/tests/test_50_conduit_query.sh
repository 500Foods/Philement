#!/usr/bin/env bash

# Test: Conduit Single Query Endpoint
# Tests the /api/conduit/query endpoint for single public query execution
# Launches unified server with 7 database engines and tests single query functionality

# FUNCTIONS
# validate_conduit_request()
# test_conduit_single_public_query()
# run_conduit_test_unified()

# CHANGELOG
# 1.7.0 - 2026-01-22 - Added QueryRef #57 parameter type testing
#                    - Test all 9 parameter datatypes (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP)
#                    - Cross-engine result comparison to verify identical values across all database engines
#                    - Comprehensive parameter validation testing
#                    - Missing required parameter test (Query #55 with only START, missing FINISH)
#                    - Unused parameter test (Query #55 with extra parameter)
#                    - Invalid database name test
#                    - Parameter type mismatch test (sending string for INTEGER)
#                    - Enhanced response format validation
# 1.5.0 - 2026-01-21 - Added parameter and query failure testing
#                    - Param Test: QueryRef 56 with NUMERATOR=50, DENOMINATOR=10 (expects success)
#                    - Param Test Fail: QueryRef 56 with TOP=50, BOTTOM=10 (expects failure with parameter mismatch message)
#                    - Query Fail: QueryRef 56 with NUMERATOR=50, DENOMINATOR=0 (expects failure with database error message)
# 1.4.0 - 2026-01-21 - Added invalid JSON testing for malformed request validation
#                    - Tests missing closing bracket JSON and expects HTTP 400 response
#                    - Validates API-level JSON validation middleware functionality

set -euo pipefail

# Test Configuration
TEST_NAME="Conduit Single Query"
TEST_ABBR="CSQ"
TEST_NUMBER="50"
TEST_COUNTER=0
TEST_VERSION="1.7.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_50_conduit_query.json"
CONDUIT_LOG_SUFFIX="conduit_query"
CONDUIT_DESCRIPTION="CSQ"

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

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing public queries (#53, #54, #55) across all ready databases"
    #print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing ${db_engine} database"
        #print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"

        # Test query #53 - Get Themes
        local payload53
        payload53=$(cat <<EOF
{
  "query_ref": 53,
  "database": "${db_name}",
  "params": {}
}
EOF
)

        local response_file53="${result_file}.single_${db_engine}_53.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload53}" "200" "${response_file53}" "" "${db_engine} Query #53" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test query #54 - Get Icons
        local payload54
        payload54=$(cat <<EOF
{
  "query_ref": 54,
  "database": "${db_name}",
  "params": {}
}
EOF
)

        local response_file54="${result_file}.single_${db_engine}_54.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload54}" "200" "${response_file54}" "" "${db_engine} Query #54" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test query #55 - Get Number Range
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

        local response_file55="${result_file}.single_${db_engine}_55.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload55}" "200" "${response_file55}" "" "${db_engine} Query #55" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test auth-required query #30 on public endpoint - should fail
        local payload30_public
        payload30_public=$(cat <<EOF
{
  "query_ref": 30,
  "database": "${db_name}",
  "params": {}
}
EOF
)
        local response_file30_public="${result_file}.single_${db_engine}_30_public.json"
        total_tests=$(( total_tests + 1 ))

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload30_public}" "200" "${response_file30_public}" "" "${db_engine} Query #30 (public)" "\"fail\""; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test invalid query ref -100
        local payload_invalid
        payload_invalid=$(cat <<EOF
{
  "query_ref": -100,
  "database": "${db_name}",
  "params": {}
}
EOF
)
        local response_file_invalid="${result_file}.single_invalid_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 200 with "fail" response for invalid query ref
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_invalid}" "200" "${response_file_invalid}" "" "${db_engine} Invalid Query -100" "\"fail\""; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test invalid JSON - missing closing bracket
        local payload_invalid_json
        payload_invalid_json=$(cat <<EOF
{
  "query_ref": 53,
  "database": "${db_name}",
  "params": {}
EOF
)
        local response_file_invalid_json="${result_file}.single_invalid_json_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 400 for invalid JSON
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_invalid_json}" "400" "${response_file_invalid_json}" "" "${db_engine} Invalid JSON" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test missing required parameter - Query #55 with only START, missing FINISH
        local payload_missing_param
        payload_missing_param=$(cat <<EOF
{
  "query_ref": 55,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"START": 500}
  }
}
EOF
)
        local response_file_missing_param="${result_file}.single_missing_param_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 400 for missing required parameter
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_missing_param}" "400" "${response_file_missing_param}" "" "${db_engine} Missing Required Parameter" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test unused parameter - Query #55 with extra parameter
        local payload_unused_param
        payload_unused_param=$(cat <<EOF
{
  "query_ref": 55,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"START": 500, "FINISH": 600, "EXTRA": 123}
  }
}
EOF
)
        local response_file_unused_param="${result_file}.single_unused_param_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 200 with warning in message for unused parameter
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_unused_param}" "200" "${response_file_unused_param}" "" "${db_engine} Unused Parameter" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test invalid database name
        local payload_invalid_db
        payload_invalid_db=$(cat <<EOF
{
  "query_ref": 53,
  "database": "nonexistent_database",
  "params": {}
}
EOF
)
        local response_file_invalid_db="${result_file}.single_invalid_db_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 400 for invalid database name
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_invalid_db}" "400" "${response_file_invalid_db}" "" "${db_engine} Invalid Database" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test parameter type mismatch - send string for INTEGER parameter
        local payload_type_mismatch
        payload_type_mismatch=$(cat <<EOF
{
  "query_ref": 55,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"START": "invalid_string", "FINISH": 600}
  }
}
EOF
)
        local response_file_type_mismatch="${result_file}.single_type_mismatch_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 400 for parameter type mismatch
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_type_mismatch}" "400" "${response_file_type_mismatch}" "" "${db_engine} Parameter Type Mismatch" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test Param Test - QueryRef 56 with correct parameters (NUMERATOR=50, DENOMINATOR=10)
        local payload_param_test
        payload_param_test=$(cat <<EOF
{
  "query_ref": 56,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"NUMERATOR": 50, "DENOMINATOR": 10}
  }
}
EOF
)
        local response_file_param_test="${result_file}.single_param_test_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 200 with success=true
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_param_test}" "200" "${response_file_param_test}" "" "${db_engine} Param Test" "true"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test Param Test Fail - QueryRef 56 with wrong parameter names (TOP=50, BOTTOM=10)
        local payload_param_fail
        payload_param_fail=$(cat <<EOF
{
  "query_ref": 56,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"TOP": 50, "BOTTOM": 10}
  }
}
EOF
)
        local response_file_param_fail="${result_file}.single_param_fail_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Expect 400 with parameter mismatch details
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload_param_fail}" "400" "${response_file_param_fail}" "" "${db_engine} Param Test Fail" "none"; then
            tests_passed=$(( tests_passed + 1 ))
        fi

        # Test Query Fail - QueryRef 56 with division by zero (NUMERATOR=50, DENOMINATOR=0)
        local payload_query_fail
        payload_query_fail=$(cat <<EOF
{
  "query_ref": 56,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"NUMERATOR": 50, "DENOMINATOR": 0}
  }
}
EOF
)
        local response_file_query_fail="${result_file}.single_query_fail_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        # Special handling for division by zero - some engines don't treat it as error
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} Query Fail"

        local http_status
        http_status=$(curl -s -X POST "${base_url}/api/conduit/query" \
            -H "Content-Type: application/json" \
            -d "${payload_query_fail}" \
            -w "%{http_code}" -o "${response_file_query_fail}" 2>/dev/null)

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

        # Show response details
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file_query_fail}" ]]; then
            # Build summary line
            local summary_parts=()

            # Show success field
            local success_val
            success_val=$(jq -r '.success' "${response_file_query_fail}" 2>/dev/null || echo "unknown")
            if [[ "${success_val}" != "null" ]]; then
                summary_parts+=("Success: ${success_val}")
            fi

            # Show row_count if present
            local row_count
            row_count=$(jq -r '.row_count' "${response_file_query_fail}" 2>/dev/null || echo "")
            if [[ -n "${row_count}" ]] && [[ "${row_count}" != "null" ]]; then
                row_count_formatted=$(format_number "${row_count}")
                summary_parts+=("Rows: ${row_count_formatted}")
            fi

            # Show error field if present
            local error_msg
            error_msg=$(jq -r '.error' "${response_file_query_fail}" 2>/dev/null || echo "")
            if [[ -n "${error_msg}" ]] && [[ "${error_msg}" != "null" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${error_msg}"
            fi

            # Show message field if present
            local message
            message=$(jq -r '.message' "${response_file_query_fail}" 2>/dev/null || echo "")
            if [[ -n "${message}" ]] && [[ "${message}" != "null" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Message: ${message}"
            fi

            # Show results file with size
            local file_size
            file_size=$(stat -c %s "${response_file_query_fail}" 2>/dev/null || echo "unknown")
            file_size_formatted=$(format_number "${file_size}")
            summary_parts+=("Response: ${file_size_formatted} bytes")

            # Print summary line
            if [[ ${#summary_parts[@]} -gt 0 ]]; then
                local summary=""
                for part in "${summary_parts[@]}"; do
                    if [[ -n "${summary}" ]]; then
                        summary="${summary}, ${part}"
                    else
                        summary="${part}"
                    fi
                done
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${summary}"
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${response_file_query_fail}"
        fi

        # Check if response is acceptable (422 for error, or 200 for engines that don't treat division by zero as error)
        if [[ "${http_status}" == "422" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${db_engine} Query Fail - Request completed"
            tests_passed=$(( tests_passed + 1 ))
        elif [[ "${http_status}" == "200" ]]; then
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} does not treat division by zero as an error"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${db_engine} Query Fail - Engine does not treat division by zero as error"
            tests_passed=$(( tests_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${db_engine} Query Fail - Unexpected HTTP status ${http_status}"
        fi

        # Test Query Params Test - QueryRef 57 with all parameter types
        local payload_params_test
        payload_params_test=$(cat <<EOF
{
  "query_ref": 57,
  "database": "${db_name}",
  "params": {
    "INTEGER": {"INTEGER": 42},
    "STRING": {"STRING": "test_value"},
    "BOOLEAN": {"BOOLEAN": true},
    "FLOAT": {"FLOAT": 3.14159},
    "TEXT": {"TEXT": "This is a longer text value for testing"},
    "DATE": {"DATE": "2023-12-25"},
    "TIME": {"TIME": "14:30:00"},
    "DATETIME": {"DATETIME": "2023-12-25 14:30:00"},
    "TIMESTAMP": {"TIMESTAMP": "2023-12-25 14:30:00.123"}
  }
}
EOF
)
        local response_file_params_test="${result_file}.single_params_test_${db_engine}.json"
        total_tests=$(( total_tests + 1 ))

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} Query Params Test #57"

        local http_status
        http_status=$(curl -s -X POST "${base_url}/api/conduit/query" \
            -H "Content-Type: application/json" \
            -d "${payload_params_test}" \
            -w "%{http_code}" -o "${response_file_params_test}" 2>/dev/null)

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

        # Show response details for troubleshooting
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file_params_test}" ]]; then
            # Build summary line
            local summary_parts=()

            # Show success field
            local success_val
            success_val=$(jq -r '.success' "${response_file_params_test}" 2>/dev/null || echo "unknown")
            if [[ "${success_val}" != "null" ]]; then
                summary_parts+=("Success: ${success_val}")
            fi

            # Show row_count if present
            local row_count
            row_count=$(jq -r '.row_count' "${response_file_params_test}" 2>/dev/null || echo "")
            if [[ -n "${row_count}" ]] && [[ "${row_count}" != "null" ]]; then
                row_count_formatted=$(format_number "${row_count}")
                summary_parts+=("Rows: ${row_count_formatted}")
            fi

            # Show error field if present
            local error_msg
            error_msg=$(jq -r '.error' "${response_file_params_test}" 2>/dev/null || echo "")
            if [[ -n "${error_msg}" ]] && [[ "${error_msg}" != "null" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${error_msg}"
            fi

            # Show message field if present
            local message
            message=$(jq -r '.message' "${response_file_params_test}" 2>/dev/null || echo "")
            if [[ -n "${message}" ]] && [[ "${message}" != "null" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Message: ${message}"
            fi

            # Show results file with size
            local file_size
            file_size=$(stat -c %s "${response_file_params_test}" 2>/dev/null || echo "unknown")
            file_size_formatted=$(format_number "${file_size}")
            summary_parts+=("Response: ${file_size_formatted} bytes")

            # Print summary line
            if [[ ${#summary_parts[@]} -gt 0 ]]; then
                local summary=""
                for part in "${summary_parts[@]}"; do
                    if [[ -n "${summary}" ]]; then
                        summary="${summary}, ${part}"
                    else
                        summary="${part}"
                    fi
                done
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${summary}"
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${response_file_params_test}"
        fi

        # Check if response is successful
        if [[ "${http_status}" == "200" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${db_engine} Query Params Test #57 - Request completed"
            tests_passed=$(( tests_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${db_engine} Query Params Test #57 - Expected HTTP 200, got ${http_status}"
        fi
    done

    # Cross-engine parameter test result comparison
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Cross-Engine Parameter Test Comparison"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"

    local comparison_tests_passed=0
    local comparison_tests_total=0

    # Define the parameters to test
    local parameters=("float_test" "boolean_test" "integer_test" "string_test" "datetime_test" "timestamp_test" "text_test" "time_test" "date_test")

    # Collect results from all ready databases for Query #57
    declare -A engine_results
    local ready_engines=()

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local response_file="${result_file}.single_params_test_${db_engine}.json"

        if [[ -f "${response_file}" ]]; then
            # Extract the data row from the response
            local data_result
            data_result=$(jq -r '.rows[0]' "${response_file}" 2>/dev/null || echo "")

            if [[ -n "${data_result}" ]] && [[ "${data_result}" != "null" ]]; then
                engine_results["${db_engine}"]="${data_result}"
                ready_engines+=("${db_engine}")
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} has no valid result data"
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} response file not found"
        fi
    done

    # Compare each parameter individually
    for param in "${parameters[@]}"; do
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Compare ${param} results"

        local param_values=()
        local param_engines=()
        local all_match=true
        local reference_value=""

        # Collect values for this parameter from all engines
        for db_engine in "${ready_engines[@]}"; do
            local json_data="${engine_results[${db_engine}]}"
            local param_value
            param_value=$(echo "${json_data}" | jq -r ".${param}" 2>/dev/null || echo "")

            if [[ -n "${param_value}" ]] && [[ "${param_value}" != "null" ]]; then
                param_values+=("${param_value}")
                param_engines+=("${db_engine}")

                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine}: \"${param_value}\""

                if [[ -z "${reference_value}" ]]; then
                    reference_value="${param_value}"
                elif [[ "${param_value}" != "${reference_value}" ]]; then
                    all_match=false
                fi
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine}: (no value)"
                all_match=false
            fi
        done

        comparison_tests_total=$(( comparison_tests_total + 1 ))

        if [[ "${all_match}" == true ]] && [[ ${#param_values[@]} -gt 1 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${param} results match"
            comparison_tests_passed=$(( comparison_tests_passed + 1 ))
        elif [[ ${#param_values[@]} -eq 0 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${param} - No valid results to compare"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${param} results differ between engines"
        fi
    done

    echo "SINGLE_PUBLIC_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "SINGLE_PUBLIC_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}



# Function to run conduit single query tests on unified server
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

    print_message "50" "0" "Server log location: build/tests/logs/test_50_${TIMESTAMP}_conduit_query.log"

    # Run conduit single query endpoint tests
    test_conduit_single_public_query "${base_url}" "${result_file}"

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
    # PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unified configuration file validation failed"
    EXIT_CODE=1
fi
TEST_NAME="Conduit Single Query  {BLUE}databases: ${#DATABASE_NAMES[@]}{RESET}"

# Only proceed with conduit tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit single query endpoint tests on unified server"

    # Run single server test
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting conduit test server (${CONDUIT_DESCRIPTION})"

    # Run the conduit test on the single unified server
    run_conduit_test_unified "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"

    # Process results
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "──────────────────────────────────────────────────"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CONDUIT_DESCRIPTION}: Analyzing results"

    # Add links to log and result files for troubleshooting
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Results: ${DIAG_TEST_DIR}/${result_file##*/}"
else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit single query endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"