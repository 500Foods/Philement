#!/usr/bin/env bash

# Test: Conduit Cap-Protected Query Endpoint
# Tests the /api/conduit/cap_query endpoint negative paths and the protected
# INSERT positive path. Positive tests use the Cap fallback path by pointing
# CHACHA_SERVER at an unreachable URL, so no real browser-solved token is
# required in CI.

# CHANGELOG
# 1.2.0 - 2026-06-28 - Added queue_used == "slow" assertion for protected INSERT success cases
# 1.1.0 - 2026-06-24 - Added positive fallback-path tests for QueryRefs #085 and #086
# 1.0.0 - 2026-06-22 - Initial negative-path tests for cap_query

set -euo pipefail

# Test Configuration
TEST_NAME="Conduit Cap Query"
TEST_ABBR="CCQ"
TEST_NUMBER="56"
TEST_COUNTER=0
TEST_VERSION="1.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Reuse the unified 7-engine config from test_50; it already includes ChaCha fields.
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_50_conduit_query.json"
CONDUIT_LOG_SUFFIX="cap_query"
CONDUIT_DESCRIPTION="CCQ"

# Test helper: send a cap_query request and verify error code
# Usage: test_cap_query_error <base_url> <result_file> <description> <payload> <expected_error>
test_cap_query_error() {
    local base_url="$1"
    local result_file="$2"
    local description="$3"
    local payload="$4"
    local expected_error="$5"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"

    local response_file="${result_file}.${description// /_}.json"
    local http_status
    http_status=$(curl -s -X POST "${base_url}/api/conduit/cap_query" \
        -H "Content-Type: application/json" \
        -d "${payload}" \
        -w "%{http_code}" \
        -o "${response_file}" \
        --max-time 30 2>/dev/null)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

    local actual_error=""
    if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
        actual_error=$(jq -r '.error // empty' "${response_file}" 2>/dev/null || true)
        local actual_message
        actual_message=$(jq -r '.message // empty' "${response_file}" 2>/dev/null || true)
        [[ -n "${actual_error}" ]] && print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${actual_error}"
        [[ -n "${actual_message}" ]] && print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Message: ${actual_message}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${response_file}"
    fi

    if [[ "${http_status}" != "400" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected HTTP 400, got ${http_status}"
        return 1
    fi

    if [[ "${actual_error}" != "${expected_error}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected error '${expected_error}', got '${actual_error}'"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} passed"
    return 0
}

# Test helper: send a cap_query request and verify successful execution
# Usage: test_cap_query_success <base_url> <result_file> <description> <query_ref> <database> <params_json> [expect_fallback]
test_cap_query_success() {
    local base_url="$1"
    local result_file="$2"
    local description="$3"
    local query_ref="$4"
    local database="$5"
    local params_json="$6"
    local expect_fallback="${7:-false}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"

    local payload
    payload=$(jq -n \
        --arg query_ref "${query_ref}" \
        --arg database "${database}" \
        --argjson params "${params_json}" \
        --arg chachaToken "test-token" \
        --arg chachaSite "a9b8369d3b" \
        '{query_ref: ($query_ref | tonumber), database: $database, params: $params, chachaToken: $chachaToken, chachaSite: $chachaSite}')

    local response_file="${result_file}.${description// /_}.json"
    local http_status
    http_status=$(curl -s -X POST "${base_url}/api/conduit/cap_query" \
        -H "Content-Type: application/json" \
        -d "${payload}" \
        -w "%{http_code}" \
        -o "${response_file}" \
        --max-time 30 2>/dev/null)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

    local success_val="false"
    local actual_error=""
    local returned_id=""
    local cap_fallback="false"
    local queue_used=""
    if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
        success_val=$(jq -r '.success // "false"' "${response_file}" 2>/dev/null || echo "false")
        actual_error=$(jq -r '.error // empty' "${response_file}" 2>/dev/null || true)
        returned_id=$(jq -r '.rows[0].suggestion_id // .rows[0].submission_id // empty' "${response_file}" 2>/dev/null || true)
        cap_fallback=$(jq -r '.cap_fallback // "false"' "${response_file}" 2>/dev/null || echo "false")
        queue_used=$(jq -r '.queue_used // empty' "${response_file}" 2>/dev/null || true)
        [[ -n "${actual_error}" ]] && print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${actual_error}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${response_file}"
    fi

    if [[ "${http_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected HTTP 200, got ${http_status}"
        return 1
    fi

    if [[ "${success_val}" != "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected success=true, got ${success_val}"
        return 1
    fi

    if [[ -z "${returned_id}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - no returned ID in response"
        return 1
    fi

    if [[ "${expect_fallback}" == "true" && "${cap_fallback}" != "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected cap_fallback=true in response"
        return 1
    fi

    if [[ "${queue_used}" != "slow" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected queue_used='slow', got '${queue_used}'"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} passed (returned_id=${returned_id})"
    return 0
}

# Main test runner
run_cap_query_test_unified() {
    local config_file="$1"
    local log_suffix="$2"
    local description="$3"

    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Start the unified server
    local server_info
    server_info=$(run_conduit_server "${config_file}" "${log_suffix}" "${description}" "${result_file}")

    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server startup failed"
        return 1
    fi

    local base_url hydrogen_pid
    base_url=$(echo "${server_info}" | awk -F: '{print $1":"$2":"$3}')
    hydrogen_pid=$(echo "${server_info}" | awk -F: '{print $4}')

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server started successfully: ${server_info}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log location: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"

    local test_exit_code=0

    # Negative case 1: missing chachaToken
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! test_cap_query_error "${base_url}" "${result_file}" "Missing Token" \
        '{"query_ref":1,"database":"Demo_PG"}' \
        "CAP_TOKEN_MISSING"; then
        test_exit_code=1
    fi

    # Negative case 2: invalid chachaToken (verifier will reject it)
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! test_cap_query_error "${base_url}" "${result_file}" "Invalid Token" \
        '{"query_ref":1,"database":"Demo_PG","chachaToken":"invalid-token"}' \
        "CAP_VERIFY_FAILED"; then
        test_exit_code=1
    fi

    echo "CONDUIT_TEST_COMPLETE" >> "${result_file}"

    shutdown_conduit_server "${hydrogen_pid}" "${result_file}"

    # Positive cases: run with CHACHA_SERVER pointed at an unreachable URL so
    # the siteverify call fails with a transient error and the fallback path
    # executes the protected INSERT queries.
    local original_chacha_server="${CHACHA_SERVER:-}"
    export CHACHA_SERVER="http://127.0.0.1:1"

    local positive_result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_fallback.result"
    local positive_server_info
    positive_server_info=$(run_conduit_server "${config_file}" "${log_suffix}_fallback" "${description} Fallback" "${positive_result_file}")

    if [[ "${positive_server_info}" == "FAILED:0" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Fallback server startup failed"
        export CHACHA_SERVER="${original_chacha_server}"
        return 1
    fi

    local positive_base_url positive_hydrogen_pid
    positive_base_url=$(echo "${positive_server_info}" | awk -F: '{print $1":"$2":"$3}')
    positive_hydrogen_pid=$(echo "${positive_server_info}" | awk -F: '{print $4}')

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Fallback server started successfully: ${positive_server_info}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Fallback server log location: ${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}_fallback.log"

    # Test each protected query reference
    # shellcheck disable=SC2312 # Invoke commands separately to avoid masking return values
    for query_ref in "${!PROTECTED_CAP_QUERY_REFS[@]}"; do
        local query_info
        query_info="${PROTECTED_CAP_QUERY_REFS[${query_ref}]}"
        local query_description
        query_description=$(echo "${query_info}" | cut -d: -f1)
        local params_json
        params_json=$(echo "${query_info}" | cut -d: -f2-)

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! test_cap_query_success "${positive_base_url}" "${positive_result_file}" "${query_description}" \
            "${query_ref}" "Demo_PG" "${params_json}" "true"; then
            test_exit_code=1
        fi
    done

    echo "CONDUIT_TEST_COMPLETE" >> "${positive_result_file}"

    shutdown_conduit_server "${positive_hydrogen_pid}" "${positive_result_file}"

    export CHACHA_SERVER="${original_chacha_server}"

    return "${test_exit_code}"
}

# Locate Hydrogen binary
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
if [[ -z "${CHACHA_SERVER:-}" || -z "${CHACHA_SITEID:-}" || -z "${CHACHA_SECRET:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: CHACHA_* environment variables are not all set; negative tests still work, positive tests will fail"
fi

if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required demo environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
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

# Run the blackbox tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting cap_query test server (${CONDUIT_DESCRIPTION})"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if run_cap_query_test_unified "${CONDUIT_CONFIG_FILE}" "${CONDUIT_LOG_SUFFIX}" "${CONDUIT_DESCRIPTION}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "cap_query negative and positive path tests completed"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "cap_query tests reported failures"
        EXIT_CODE=1
    fi

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"

    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.log"
    result_file="${LOG_PREFIX}${TIMESTAMP}_${CONDUIT_LOG_SUFFIX}.result"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Server: ${TESTS_DIR}/logs/${log_file##*/}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Conduit Results: ${DIAG_TEST_DIR}/${result_file##*/}"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping cap_query tests due to prerequisite failures"
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
