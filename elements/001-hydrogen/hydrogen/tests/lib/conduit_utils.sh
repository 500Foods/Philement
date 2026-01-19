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
    local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")

    # Add Authorization header if JWT token is provided
    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi

    # Complete curl command
    curl_cmd+=(-d "${data}" -w "%{http_code}" -o "${output_file}" --compressed --max-time 60 "${endpoint}")

    # Run curl and capture HTTP status
    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null)

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

    if [[ "${http_status}" == "${expected_status}" ]]; then
        # Check for success in JSON response for successful requests
        if [[ "${expected_status}" -eq 200 ]]; then
            # shellcheck disable=SC2154 # GREP defined in framework.sh
        if "${GREP}" -q "\"success\"[[:space:]]*:[[:space:]]*${expected_success}" "${output_file}"; then
                # Additional validation for successful queries
                if command -v jq >/dev/null 2>&1 && [[ -f "${output_file}" ]]; then
                    local success_value rows_count file_size
                    success_value=$(jq -r '.success' "${output_file}" 2>/dev/null || echo "unknown")
                    rows_count=$(jq -r '.rows | length' "${output_file}" 2>/dev/null || echo "unknown")
                    file_size=$(stat -c %s "${output_file}" 2>/dev/null || echo "unknown")
                    # Format numbers with thousands separators using framework function
                    rows_formatted=$(format_number "${rows_count}")
                    file_size_formatted=$(format_number "${file_size}")
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Valid JSON returned: success=${success_value}, ${rows_formatted} rows, ${file_size_formatted} bytes"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${output_file}"
                fi
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Request successful"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - Request failed (expected success=${expected_success} in response)"
                return 1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - Request returned expected status ${expected_status}"
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

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready before attempting login
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWT Acquisition (${db_engine})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWT Acquisition (${db_engine}) - Database not ready, skipping"
            jwt_tokens+=("")  # Add empty token to maintain array alignment
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWT Acquisition (${db_engine})"

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

    # Return the JWT tokens array (this will be captured and used)
    # Note: bash functions can't return arrays directly, so we use a global variable approach
    # The caller should use: jwt_tokens_string=$(acquire_jwt_tokens ...); declare -a jwt_tokens="($jwt_tokens_string)"
    echo "(${jwt_tokens[*]})"
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
            IFS=':' read -r description _ <<< "${PUBLIC_QUERY_REFS[${query_ref}]}"
            local query_ref_num=$((10#${query_ref}))

            # Build payload for single query
            local payload
            payload=$(cat <<EOF
{
  "query_ref": ${query_ref_num},
  "database": "${db_name}",
  "params": {}
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