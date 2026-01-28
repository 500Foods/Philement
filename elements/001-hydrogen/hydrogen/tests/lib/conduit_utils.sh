#!/usr/bin/env bash

# Conduit Utilities Library
# Provides conduit endpoint testing utilities for test scripts

# LIBRARY FUNCTIONS
# validate_conduit_request()
# check_database_readiness()
# acquire_jwt_tokens()
# test_conduit_endpoint()
# test_conduit_single_queries()
# test_conduit_multiple_queries_endpoint()

# CHANGELOG
# 1.5.0 - 2026-01-24 - Modified validate_conduit_request() to conditionally send request body only for non-GET methods
#                    - GET requests no longer send Content-Type header or empty data body
# 1.4.0 - 2026-01-21 - Enhanced validate_conduit_request() for non-200 responses
#                    - Expected non-200 status codes now show response details and messages
#                    - Includes file size, message field, and error field for better debugging
#                    - Enables detailed validation of error responses (e.g., HTTP 400 for invalid JSON)
# 1.2.0 - 2026-01-19 - Added server lifecycle and result analysis functions
#                    - Added run_conduit_server() for unified server startup/shutdown management
#                    - Added shutdown_conduit_server() for graceful server termination
#                    - Added analyze_conduit_results() for comprehensive test result analysis
#                    - Improved modularity for conduit testing infrastructure
# 1.1.0 - 2026-01-19 - Added JWT acquisition and test helper functions
#                    - Added acquire_jwt_tokens() for automated JWT token acquisition
#                    - Added test_conduit_single_queries() for testing single query endpoints
#                    - Added test_conduit_multiple_queries_endpoint() for testing batch query endpoints
#                    - Improved reusability for conduit endpoint testing
# 1.0.0 - 2026-01-19 - Initial creation from test_51_conduit.sh refactoring
#                    - Extracted validate_conduit_request() and check_database_readiness()
#                    - Added database name mappings for conduit testing

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] && return 0
export CONDUIT_UTILS_GUARD="true"

# Database names mapping for conduit testing
# Used in unified single-server configuration
declare -A DATABASE_NAMES
DATABASE_NAMES=(
    ["PostgreSQL"]="Demo_PG"
    ["MySQL"]="Demo_MY"
    ["SQLite"]="Demo_SQL"
    ["DB2"]="Demo_DB2"
    ["MariaDB"]="Demo_MR"
    ["CockroachDB"]="Demo_CR"
    ["YugabyteDB"]="Demo_YB"
)

# Query references for conduit testing
# Format: "query_ref:description:requires_auth:params_json"
# Query references for conduit testing
# Format: "query_ref:description:requires_auth:params_json"
declare -A PUBLIC_QUERY_REFS
PUBLIC_QUERY_REFS=(
    ["53"]="Get Themes:false:{}"
    ["54"]="Get Icons:false:{}"
    ["55"]="Get Number Range:false:{\"INTEGER\":{\"START\":500,\"FINISH\":600}}"
)

declare -A NON_PUBLIC_QUERY_REFS
NON_PUBLIC_QUERY_REFS=(
    ["30"]="Get Lookups List:true"
    ["45"]="Get Lookup:true"
)

# Export variables for use in scripts that source this library
export PUBLIC_QUERY_REFS
export NON_PUBLIC_QUERY_REFS

# Function to validate conduit request with optional JWT authentication
validate_conduit_request() {
    local endpoint="$1"
    local method="$2"
    local data="$3"
    local expected_status="$4"
    local output_file="$5"
    local jwt_token="${6:-}"  # Optional JWT token for authenticated requests
    local description="$7"
    local expected_success="${8:-true}"  # Optional expected success value (default: true)

    # shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"

    # Build curl command with optional Authorization header
    local curl_cmd=(curl -s -X "${method}")

    # Add Content-Type and data only for methods that typically send a body
    if [[ "${method}" != "GET" && "${method}" != "HEAD" ]]; then
        curl_cmd+=(-H "Content-Type: application/json" -d "${data}")
    fi

    # Add Authorization header if JWT token is provided
    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi

    # Complete curl command
    curl_cmd+=(-w "%{http_code}" -o "${output_file}" --compressed --max-time 60 "${endpoint}")

    # Run curl and capture HTTP status
    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

    # For all HTTP requests, show key response details
    if command -v jq >/dev/null 2>&1 && [[ -f "${output_file}" ]]; then
        # Build summary line
        local summary_parts=()

        # Show success field
        local success_val
        success_val=$(jq -r '.success' "${output_file}" 2>/dev/null || echo "unknown")
        if [[ "${success_val}" != "null" ]]; then
            summary_parts+=("Success: ${success_val}")
        fi

        # Show row_count if present
        local row_count
        row_count=$(jq -r '.row_count' "${output_file}" 2>/dev/null || echo "")
        if [[ -n "${row_count}" ]] && [[ "${row_count}" != "null" ]]; then
            row_count_formatted=$(format_number "${row_count}")
            summary_parts+=("Rows: ${row_count_formatted}")
        fi

        # Show error field if present
        local error_msg
        error_msg=$(jq -r '.error' "${output_file}" 2>/dev/null || echo "")
        if [[ -n "${error_msg}" ]] && [[ "${error_msg}" != "null" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${error_msg}"
        fi

        # Show message field if present
        local message
        message=$(jq -r '.message' "${output_file}" 2>/dev/null || echo "")
        if [[ -n "${message}" ]] && [[ "${message}" != "null" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Message: ${message}"
        fi

        # Show results file with size
        local file_size
        file_size=$(stat -c %s "${output_file}" 2>/dev/null || echo "unknown")
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

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${output_file}"
    fi

    # Handle HTTP 000 (server didn't return valid response) as a special case
    if [[ "${http_status}" == "000" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server did not return a valid HTTP response"
        if [[ -f "${output_file}" ]]; then
            local file_size
            file_size=$(stat -c %s "${output_file}" 2>/dev/null || echo "unknown")
            if [[ "${file_size}" -gt 0 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response file contains ${file_size} bytes of data"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response file is empty"
            fi
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Server did not return valid HTTP response (HTTP 000)"
        return 1
    fi

    if [[ "${http_status}" == "${expected_status}" ]]; then
        # Check success field if expected_success is specified
        if [[ -n "${expected_success}" && "${expected_success}" != "none" ]]; then
            # shellcheck disable=SC2154 # GREP provided via tests/lib/framework.sh
            if "${GREP}" -q "\"success\"[[:space:]]*:[[:space:]]*${expected_success}" "${output_file}"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Request successful"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Request failed (expected success=${expected_success} in response)"
                return 1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Request completed"
            return 0
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Expected HTTP ${expected_status}, got ${http_status}"
        return 1
    fi
}

# Function to check which databases are ready for testing
check_database_readiness() {
    local log_file="$1"
    local result_file="$2"

    # Check each database for migration completion
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local db_name="${DATABASE_NAMES[${db_engine}]}"
        local db_ready=false

        # Check for successful migration completion
        if "${GREP}" -q "${db_name}.*Migration process completed.*QTC populated from bootstrap queries" "${log_file}" 2>/dev/null; then
            db_ready=true
        fi

        if [[ "${db_ready}" = true ]]; then
            echo "DATABASE_READY_${db_engine}=true" >> "${result_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} database ready for testing"
        else
            echo "DATABASE_READY_${db_engine}=false" >> "${result_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} database not ready - skipping tests"
        fi
    done
}

# Function to acquire JWT tokens for all ready databases
acquire_jwt_tokens() {
    local base_url="$1"
    local result_file="$2"
    local jwt_tokens=()

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting JWT acquisition for ${#DATABASE_NAMES[@]} databases"

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWT Acquisition (${db_engine})"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking readiness for ${db_engine}..."

        # Check if database is ready before attempting login
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} not ready, skipping JWT acquisition"
            #print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWT Acquisition (${db_engine})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWT Acquisition (${db_engine}) - Database not ready, skipping"
            jwt_tokens+=("")  # Add empty token to maintain array alignment
            continue
        fi

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} is ready, attempting JWT acquisition"

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        local login_data
        # shellcheck disable=SC2154 # Environment variables set externally
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

        local login_response_file="${result_file}.login_${db_engine}.json"
        local login_curl_output
        login_curl_output=$(curl -s -X POST "${base_url}/api/auth/login" \
            -H "Content-Type: application/json" \
            -d "${login_data}" \
            -w "HTTP_CODE:%{http_code}" \
            -o "${login_response_file}" 2>/dev/null)

        local login_http_code
        login_http_code=$(echo "${login_curl_output}" | grep "HTTP_CODE:" | cut -d: -f2)

        local jwt_token=""
        if [[ "${login_http_code}" -eq 200 ]] && command -v jq >/dev/null 2>&1; then
            jwt_token=$(jq -r '.token' "${login_response_file}" 2>/dev/null || echo "")
            if [[ -n "${jwt_token}" ]] && [[ "${jwt_token}" != "null" ]]; then
                jwt_tokens+=("${jwt_token}")
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWT Acquisition (${db_engine}) - Successfully obtained JWT"
            else
                jwt_tokens+=("")  # Add empty token to maintain array alignment
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWT Acquisition (${db_engine}) - Failed to extract JWT token"
            fi
        else
            jwt_tokens+=("")  # Add empty token to maintain array alignment
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWT Acquisition (${db_engine}) - Login failed (HTTP ${login_http_code})"
        fi
    done

    # Store the JWT tokens in a test-specific global variable to avoid subshell issues
    # and prevent conflicts between concurrent test executions
    local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
    eval "${global_var_name}=(${jwt_tokens[*]})"

}

# Function to test conduit endpoint across all ready databases
test_conduit_endpoint() {
    local base_url="$1"
    local result_file="$2"
    local endpoint="$3"
    local payload_template="$4"
    local jwt_token="${5:-}"  # Optional JWT token
    local description_prefix="$6"
    local expected_success="${7:-true}"  # Optional expected success value

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Replace placeholders in payload template
        local payload="${payload_template//\{\{DB_NAME\}\}/${db_name}}"
        payload="${payload//\{\{JWT_TOKEN\}\}/${jwt_token}}"
        payload="${payload//\{\{USER_NAME\}\}/${HYDROGEN_DEMO_USER_NAME}}"
        payload="${payload//\{\{USER_PASS\}\}/${HYDROGEN_DEMO_USER_PASS}}"
        payload="${payload//\{\{API_KEY\}\}/${HYDROGEN_DEMO_API_KEY}}"

        local response_file="${result_file}.endpoint_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}${endpoint}" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${description_prefix} (${db_engine})" "${expected_success}"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))
    done

    # Return results for caller to handle
    echo "${tests_passed}:${total_tests}"
}

# Function to test single conduit queries with different query references
test_conduit_single_queries() {
    local base_url="$1"
    local result_file="$2"
    local endpoint="$3"
    local jwt_token="${4:-}"  # Optional JWT token
    local description_prefix="$5"

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Test each public query reference for this database
        for query_ref in "${!PUBLIC_QUERY_REFS[@]}"; do
            # Parse the format: "description:requires_auth:params_json"
            local query_info
            query_info="${PUBLIC_QUERY_REFS[${query_ref}]}"
            local description
            description=$(echo "${query_info}" | cut -d: -f1)
            local requires_auth
            requires_auth=$(echo "${query_info}" | cut -d: -f2)
            local params_json
            params_json=$(echo "${query_info}" | cut -d: -f3-)
            local query_ref_num
            query_ref_num=$((10#${query_ref}))
        
            # Check if requires_auth is used in this context
            if [[ "${requires_auth}" == "true" ]]; then
                # If authentication is required, ensure JWT token is available
                if [[ -z "${jwt_token:-}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping query ${query_ref} - authentication required but no JWT token available"
                    return 1
                fi
            fi

            # Build payload for single query
            local payload
            payload=$(cat <<EOF
{
  "query_ref": ${query_ref_num},
  "database": "${db_name}",
  "params": ${params_json}
}
EOF
)

            local response_file="${result_file}.single_${db_engine}_${query_ref}.json"

            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if validate_conduit_request "${base_url}${endpoint}" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${description_prefix} ${query_ref}: ${description} (${db_engine})"; then
                tests_passed=$(( tests_passed + 1 ))
            fi
            total_tests=$(( total_tests + 1 ))
        done
    done

    # Return results for caller to handle
    echo "${tests_passed}:${total_tests}"
}

# Function to test conduit multiple queries endpoint
test_conduit_multiple_queries_endpoint() {
    local base_url="$1"
    local result_file="$2"
    local endpoint="$3"
    local jwt_token="${4:-}"  # Optional JWT token
    local description_prefix="$5"
    local queries_json="$6"  # JSON array of queries

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Build payload for multiple queries
        local payload
        payload=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_json}
}
EOF
)

        local response_file="${result_file}.multiple_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}${endpoint}" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${description_prefix} (${db_engine})"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))
    done

    # Return results for caller to handle
    echo "${tests_passed}:${total_tests}"
}

# Function to run conduit server lifecycle (startup, migration wait, shutdown)
run_conduit_server() {
    local config_file="$1"
    local log_suffix="$2"
    local description="$3"
    local result_file="$4"

    # shellcheck disable=SC2154 # LOGS_DIR and TIMESTAMP defined in framework.sh
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"

    # Clear result file
    true > "${result_file}"

    # Extract port from configuration
    local server_port
    server_port=$(jq -r '.WebServer.Port' "${config_file}" 2>/dev/null || echo "0")
    local base_url="http://localhost:${server_port}"

    # Start hydrogen server
    # shellcheck disable=SC2154 # HYDORGEN_BIN defined in framework.sh
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}

    while true; do
        if [[ $((SECONDS - start_time)) -ge 15 ]]; then  # STARTUP_TIMEOUT
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
            if [[ $((SECONDS - migration_start_time)) -ge 300 ]]; then  # MIGRATION_TIMEOUT
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

            sleep 0.1
        done

        # Wait a bit for server to be fully ready after migrations
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for webserver to be fully ready..."
        sleep 1

        # Check which databases are ready for testing
        check_database_readiness "${log_file}" "${result_file}"

        # Wait for webserver to be ready for HTTP requests
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for webserver to be ready for HTTP requests..."
        local server_ready=false
        local server_ready_start=${SECONDS}
        while true; do
            if [[ $((SECONDS - server_ready_start)) -ge 30 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Webserver readiness timeout"
                break
            fi
            local http_code
            http_code=$(curl -s -w "%{http_code}" -o /dev/null --max-time 5 "${base_url}/api/version" 2>/dev/null)
            if [[ "${http_code}" == "200" ]]; then
                server_ready=true
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Webserver is ready for HTTP requests"
                break
            fi
            sleep 0.1
        done

        if [[ "${server_ready}" = false ]]; then
            echo "STARTUP_FAILED" >> "${result_file}"
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
            echo "FAILED:0"
            return 1
        fi

        # Return server info for caller to use
        echo "${base_url}:${hydrogen_pid}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        echo "FAILED:0"
    fi
}

# Function to shutdown conduit server
shutdown_conduit_server() {
    local hydrogen_pid="$1"
    local result_file="$2"

    # Stop the server gracefully
    if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true

        # Wait for graceful shutdown
        local shutdown_start
        shutdown_start=${SECONDS}
        while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
            if [[ $((SECONDS - shutdown_start)) -ge 10 ]]; then  # SHUTDOWN_TIMEOUT
                kill -9 "${hydrogen_pid}" 2>/dev/null || true
                break
            fi
            sleep 0.1
        done
    fi
}

# Function to analyze conduit test results
analyze_conduit_results() {
    local log_suffix="$1"
    local description="$2"         
    # shellcheck disable=SC2154 # LOG_PREFIX defined in framework.sh
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for conduit test"
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

    # Check comprehensive queries tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Comprehensive Queries Tests"
    if "${GREP}" -q "^QUERIES_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local queries_comprehensive_passed queries_comprehensive_total
        queries_comprehensive_passed=$("${GREP}" "^QUERIES_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        queries_comprehensive_total=$("${GREP}" "^QUERIES_COMPREHENSIVE_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${queries_comprehensive_passed}" -eq "${queries_comprehensive_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Comprehensive Queries Tests (${queries_comprehensive_passed}/${queries_comprehensive_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Comprehensive Queries Tests (${queries_comprehensive_passed}/${queries_comprehensive_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
        total_tests=$(( total_tests + 1 ))
    fi

    # Check auth query comprehensive tests across all databases
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Query Comprehensive Tests"
    if "${GREP}" -q "^AUTH_QUERY_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        local auth_comprehensive_passed auth_comprehensive_total
        auth_comprehensive_passed=$("${GREP}" "^AUTH_QUERY_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
        auth_comprehensive_total=$("${GREP}" "^AUTH_QUERY_COMPREHENSIVE_TESTS_TOTAL=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

        if [[ "${auth_comprehensive_passed}" -eq "${auth_comprehensive_total}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Query Comprehensive Tests (${auth_comprehensive_passed}/${auth_comprehensive_total} passed across ${#DATABASE_NAMES[@]} databases)"
            total_passed=$(( total_passed + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Auth Query Comprehensive Tests (${auth_comprehensive_passed}/${auth_comprehensive_total} passed across ${#DATABASE_NAMES[@]} databases)"
        fi
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