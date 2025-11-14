#!/usr/bin/env bash

# Test: Conduit Query
# Tests the Conduit Service query endpoint that executes pre-defined database queries by reference
# Launches all 5 database engines from test_30, then tests the /api/conduit/query endpoint
# with various query references and parameter combinations

# FUNCTIONS
# validate_conduit_query()
# run_conduit_test_parallel()
# analyze_conduit_test_results()
# test_conduit_query()
# test_conduit_endpoints()

# CHANGELOG
# 1.0.0 - 2025-10-20 - Initial implementation based on test_21_system_endpoints.sh and test_30_database.sh
#                    - Launches all 5 database engines from test_30_multi.json (renamed to test_40_conduit.json)
#                    - Tests /api/conduit/query endpoint with various query references
#                    - Validates query execution, parameter handling, and result formatting

set -euo pipefail

# Test Configuration
TEST_NAME="Conduit Query {BLUE}(QRY){RESET}"
TEST_ABBR="QRY"
TEST_NUMBER="41"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
CONFIG_PATH="${SCRIPT_DIR}/configs/hydrogen_test_41_conduit.json"

# Conduit test configuration - format: "query_ref:database:expected_success:description"
declare -A CONDUIT_TEST_CONFIGS

# Conduit query test configuration
CONDUIT_TEST_CONFIGS=(
    ["USER_PROFILE"]="1234:ACZ:true:User Profile Query"
    ["PRODUCT_LIST"]="5678:CNV:true:Product List Query"
    ["SYSTEM_INFO"]="9999:Helium:false:Invalid Query Reference"
    ["ORDER_HISTORY"]="4321:HTST:true:Order History Query"
)

# Function to test a single conduit query
validate_conduit_query() {
    local query_ref="$1"
    local database="$2"
    local expected_success="$3"
    local description="$4"
    local base_url="$5"
    local response_file="$6"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing conduit query: ${query_ref} on ${database}"

    # Prepare JSON payload
    local payload
    payload=$(cat <<EOF
{
  "query_ref": ${query_ref},
  "database": "${database}",
  "params": {
    "INTEGER": {
      "userId": 123,
      "limit": 10
    },
    "STRING": {
      "username": "testuser",
      "status": "active"
    }
  }
}
EOF
)

    # Make the request
    local curl_output
    curl_output=$(curl -s -X POST "${base_url}/api/conduit/query" \
        -H "Content-Type: application/json" \
        -d "${payload}" \
        -w "HTTP_CODE:%{http_code}" \
        -o "${response_file}" 2>/dev/null)

    local http_code
    http_code=$(echo "${curl_output}" | grep "HTTP_CODE:" | cut -d: -f2)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_code}"

    # Validate response based on expected success
    if [[ "${expected_success}" == "true" ]]; then
        if [[ "${http_code}" -eq 200 ]]; then
            # Check for success in JSON response
            if "${GREP}" -q '"success":true' "${response_file}"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Query executed successfully"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Query failed (success=false in response)"
                return 1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Expected HTTP 200, got ${http_code}"
            return 1
        fi
    else
        if [[ "${http_code}" -eq 404 ]]; then
            # Check for error in JSON response
            if "${GREP}" -q '"success":false' "${response_file}"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Correctly rejected invalid query"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Invalid query not properly rejected"
                return 1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Expected HTTP 404 for invalid query, got ${http_code}"
            return 1
        fi
    fi
}

# Function to test conduit endpoints with parallel database setup
run_conduit_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local description="$4"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Clear result file
    true > "${result_file}"

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup and database initialization
    local startup_success=false
    local database_ready=false
    local start_time
    start_time=${SECONDS}

    # Wait for startup
    while true; do
        if [[ $((SECONDS - start_time)) -ge 15 ]]; then  # 15 second timeout for startup
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

        # Wait for DQM initialization (similar to test_30)
        local dqm_launch_count
        dqm_launch_count=$(grep -c "DQM launched successfully" "${log_file}" 2>/dev/null || echo "0")

        if [[ "${dqm_launch_count}" -gt 0 ]]; then
            echo "DQM_LAUNCH_COUNT=${dqm_launch_count}" >> "${result_file}"

            # Wait for DQM initialization to complete
            local dqm_init_timeout=20
            local init_start
            init_start=${SECONDS}
            local init_complete=false

            while [[ $((SECONDS - init_start)) -lt "${dqm_init_timeout}" ]]; do
                local current_init_count
                current_init_count=$(grep -c "Lead DQM initialization is complete" "${log_file}" 2>/dev/null || echo "0")

                if [[ "${current_init_count}" -ge "${dqm_launch_count}" ]]; then
                    init_complete=true
                    break
                fi
                sleep 0.1
            done

            if [[ "${init_complete}" = true ]]; then
                echo "DQM_INITIALIZATION_COMPLETED" >> "${result_file}"
                database_ready=true

                # Now test conduit queries
                local base_url="http://localhost:5341"
                local conduit_tests_passed=0
                local total_conduit_tests=0

                for test_config in "${!CONDUIT_TEST_CONFIGS[@]}"; do
                    IFS=':' read -r query_ref database expected_success test_description <<< "${CONDUIT_TEST_CONFIGS[${test_config}]}"

                    local conduit_response_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_${test_config}.json"

                    # shellcheck disable=SC2310 # We want to continue even if the test fails
                    if validate_conduit_query "${query_ref}" "${database}" "${expected_success}" "${test_description}" "${base_url}" "${conduit_response_file}"; then
                        conduit_tests_passed=$(( conduit_tests_passed + 1 ))
                    fi
                    total_conduit_tests=$(( total_conduit_tests + 1 ))
                done

                echo "CONDUIT_TESTS_PASSED=${conduit_tests_passed}" >> "${result_file}"
                echo "CONDUIT_TESTS_TOTAL=${total_conduit_tests}" >> "${result_file}"

                if [[ "${conduit_tests_passed}" -eq "${total_conduit_tests}" ]]; then
                    echo "CONDUIT_TEST_PASSED" >> "${result_file}"
                else
                    echo "CONDUIT_TEST_FAILED" >> "${result_file}"
                fi
            else
                echo "DQM_INITIALIZATION_TIMEOUT" >> "${result_file}"
                database_ready=false
            fi
        else
            echo "NO_DQM_LAUNCHES" >> "${result_file}"
            database_ready=false
        fi
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "CONDUIT_TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi

    if [[ "${startup_success}" = true ]] && [[ "${database_ready}" = true ]]; then
        # Test completed successfully
        echo "DATABASE_TEST_PASSED" >> "${result_file}"
        echo "TEST_COMPLETE" >> "${result_file}"

        # Stop the server gracefully
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            # Wait for graceful shutdown
            local shutdown_start
            shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge 10 ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.1
            done
        fi
    else
        echo "DATABASE_TEST_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel conduit test execution
analyze_conduit_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local description="$3"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi

    # Check DQM launch count
    if ! "${GREP}" -q "DQM_LAUNCH_COUNT=" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "DQM launch count not found for ${description} test"
        return 1
    fi

    # Check DQM initialization completed
    if ! "${GREP}" -q "DQM_INITIALIZATION_COMPLETED" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "DQM initialization not completed for ${description} test"
        return 1
    fi

    # Check conduit test results
    if "${GREP}" -q "CONDUIT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        local conduit_passed
        local conduit_total
        conduit_passed=$("${GREP}" "CONDUIT_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        conduit_total=$("${GREP}" "CONDUIT_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Conduit Query Test (${conduit_passed}/${conduit_total} queries passed)"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Conduit Query Test failed"
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
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_PATH}"; then
    # Extract and display port
    SERVER_PORT=$(get_webserver_port "${CONFIG_PATH}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration will use port: ${SERVER_PORT}"
else
    EXIT_CODE=1
fi

# Only proceed with conduit tests if binary and config are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Test conduit query functionality
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit Query Test"

    run_conduit_test_parallel "conduit_query" "${CONFIG_PATH}" "conduit" "Conduit Query"

    # Analyze results
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_conduit.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_conduit.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Server Log: ..${log_file}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Result Log: ..${result_file}"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if analyze_conduit_test_results "conduit_query" "conduit" "Conduit Query"; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit Query tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"