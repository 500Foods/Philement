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
# Server lifecycle (hydrogen_process_crashed, conduit_fail_startup,
# run_conduit_server, shutdown_conduit_server, analyze_conduit_results)
# lives in tests/lib/conduit_server.sh, sourced at the bottom of this file.

# CHANGELOG
# 1.9.0 - 2026-10-30 - Move server lifecycle into tests/lib/conduit_server.sh
#                    - Keep sourcing it here so callers stay on this file
# 1.8.1 - 2026-10-30 - Score a conduit server startup failure as a FAIL
#                    - Record process exit, startup timeout, and readiness timeout
#                      on the result file so the FAIL line names the cause
# 1.8.0 - 2026-10-16 - Course suggestion cap query is QueryRef 4000
# 1.7.9 - 2026-10-01 - Eighth engine: MSSQL maps to Demo_MS (schema demoms)
# 1.7.8 - 2026-09-04 - Pair JWT skip with print_result (not-ready engines)
# 1.7.7 - 2026-09-04 - Pair TEST/PASS/FAIL in analyze_conduit_results
# 1.7.6 - 2026-09-04 - disown hydrogen started by run_conduit_server (same as
#                    start_hydrogen) so command-substitution return does not
#                    reap the server with SIGHUP/SIGINT
# 1.7.5 - 2026-07-27 - Fail fast on Signal 11 / process death during startup waits
#                    - Avoid 300s+ hangs when hydrogen crashes after STARTUP COMPLETE
# 1.7.4 - 2026-07-15 - Store acquired JWTs by database engine instead of position
#                    - Keeps tokens aligned when a non-ready database is skipped
# 1.7.3 - 2026-07-15 - Use the conduit status endpoint for per-database readiness
#                    - Prevents failed migrations with a completion timer from being
#                      treated as ready and exercised by downstream conduit tests
# 1.7.2 - 2026-07-15 - Keep shared query maps global when sourced by the suite runner
#                    - Prevents the exported guard from outliving function-local maps
# 1.7.1 - 2026-07-09 - QueryRef #30 is TYPE_PUBLIC (acuranzo_1121); keep #45 as non-public auth-only sample
# 1.7.0 - 2026-06-20 - Switched run_conduit_server() readiness to the canonical "READY FOR REQUESTS" signal
#                    - Replaced hardcoded expected_databases=7 + per-engine migration-string polling +
#                      fixed sleep with a single grep for "READY FOR REQUESTS" (emitted once all DBs are ready)
#                    - Webserver readiness now polls /api/system/readiness (200 when ready, 503 while starting)
#                      instead of /api/version; removed the fixed 1s sleep
#                    - Added CONDUIT_MIGRATION_TIMEOUT override for the readiness wait
#                    - check_database_readiness() now also matches the "Migration process completed ... QTC
#                      populated from bootstrap queries" marker for per-database ready flags
# 1.6.0 - 2026-06-20 - Fixed JWT token array index misalignment in acquire_jwt_tokens()
#                    - eval "name=(${jwt_tokens[*]})" word-split and dropped empty
#                      placeholder elements, collapsing the array and breaking the
#                      index->database mapping relied on by cross-database tests (53/54/55)
#                    - Now serializes each element single-quoted so empty placeholders are preserved
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
declare -gA DATABASE_NAMES
DATABASE_NAMES=(
    ["PostgreSQL"]="Demo_PG"
    ["MySQL"]="Demo_MY"
    ["SQLite"]="Demo_SQL"
    ["DB2"]="Demo_DB2"
    ["MariaDB"]="Demo_MR"
    ["Firebird"]="Demo_FB"
    ["YugabyteDB"]="Demo_YB"
    ["MSSQL"]="Demo_MS"
)

# Query references for conduit testing
# Format: "query_ref:description:requires_auth:params_json"
# Query references for conduit testing
# Format: "query_ref:description:requires_auth:params_json"
declare -gA PUBLIC_QUERY_REFS
PUBLIC_QUERY_REFS=(
    ["30"]="Get Lookups List:false:{}"
    ["53"]="Get Themes:false:{}"
    ["54"]="Get Icons:false:{}"
    ["55"]="Get Number Range:false:{\"INTEGER\":{\"START\":500,\"FINISH\":600}}"
)

# Auth-only system SQL (query_type_a28 != 10). Public /query must reject these.
declare -gA NON_PUBLIC_QUERY_REFS
NON_PUBLIC_QUERY_REFS=(
    ["45"]="Get Version History:true"
)

# Protected Cap query references for conduit cap_query testing
# Format: "query_ref:description:params_json"
# Populated with real QueryRefs once Helium Phase 2b registers the protected INSERT queries.
declare -gA PROTECTED_CAP_QUERY_REFS
PROTECTED_CAP_QUERY_REFS=(
    # These require Helium migrations 1196-1200 to be applied to the target database.
    # The test environment sets CHACHA_SERVER to an unreachable URL to force the
    # FALLBACK path, so no real Cap token is needed for automated testing.
    # Parameters are grouped by type (STRING, INTEGER, etc.) as expected by parse_typed_parameters.
    ["4000"]="Insert Course Suggestion:{\"STRING\":{\"COURSE_NAME\":\"Test Course\",\"SUMMARY\":\"Test summary\",\"DETAIL\":\"<p>Test detail</p>\",\"INTENDED_AUDIENCE\":\"Beginners\",\"ADDITIONAL_FEATURES\":\"None\",\"SUBMITTER_NAME\":\"Test User\",\"SUBMITTER_EMAIL\":\"test@example.com\",\"CHACHA_TOKEN\":\"test-token\",\"CHACHA_SITE\":\"a9b8369d3b\",\"IP_ADDRESS\":\"127.0.0.1\"}}"
    ["86"]="Insert Contact Submission:{\"STRING\":{\"NAME\":\"Test User\",\"EMAIL\":\"test@example.com\",\"SUBJECT\":\"Test Subject\",\"MESSAGE\":\"<p>Test message</p>\",\"CHACHA_TOKEN\":\"test-token\",\"CHACHA_SITE\":\"a9b8369d3b\",\"IP_ADDRESS\":\"127.0.0.1\"}}"
)

# Export variables for use in scripts that source this library
export PUBLIC_QUERY_REFS
export NON_PUBLIC_QUERY_REFS
export PROTECTED_CAP_QUERY_REFS

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
    local base_url="$1"
    local result_file="$2"

    local status_file="${result_file}.status_readiness.json"
    local status_http_code
    status_http_code=$(curl -s -X GET "${base_url}/api/conduit/status" \
        -w "%{http_code}" -o "${status_file}" 2>/dev/null)

    # The status endpoint reflects whether each database can actually serve conduit
    # queries. A migration timer line alone is insufficient because it is emitted
    # after both successful and failed migrations.
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        local db_name="${DATABASE_NAMES[${db_engine}]}"
        local db_ready=false
        local status_ready=false

        if [[ "${status_http_code}" == "200" ]]; then
            status_ready=$(jq -r --arg database "${db_name}" '.databases[$database].ready // false' "${status_file}" 2>/dev/null || true)
        fi

        if [[ "${status_ready}" == "true" ]]; then
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
    local token_map_name="JWT_TOKENS_BY_DATABASE_${TEST_NUMBER}"
    eval "declare -gA ${token_map_name}=()"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting JWT acquisition for ${#DATABASE_NAMES[@]} databases"

    for db_engine in "${!DATABASE_NAMES[@]}"; do
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWT Acquisition (${db_engine})"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking readiness for ${db_engine}..."

        # Check if database is ready before attempting login
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} not ready, skipping JWT acquisition"
            jwt_tokens+=("")  # Add empty token to maintain array alignment
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWT Acquisition (${db_engine}) - Skipped (database not ready)"
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
                eval "${token_map_name}[\"${db_engine}\"]=\"${jwt_token}\""
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
    # and prevent conflicts between concurrent test executions.
    #
    # IMPORTANT: The jwt_tokens array intentionally contains empty-string placeholders
    # for databases that were not ready / failed login, so that array indices stay
    # aligned with the DATABASE_NAMES iteration order. Cross-database tests (53, 54, 55)
    # rely on this positional alignment. A naive `eval "${name}=(${jwt_tokens[*]})"`
    # word-splits the joined string and silently drops empty elements, collapsing the
    # array and breaking the index→database mapping. Build a quoted element list so
    # empty placeholders are preserved exactly.
    local global_var_name="JWT_TOKENS_RESULT_${TEST_NUMBER}"
    local jwt_tokens_quoted=""
    local jwt_token_elem
    for jwt_token_elem in "${jwt_tokens[@]}"; do
        # Escape any single quotes within the token, then single-quote the element
        # so empty strings remain as distinct '' positional entries.
        jwt_tokens_quoted+=" '${jwt_token_elem//\'/\'\\\'\'}'"
    done
    eval "${global_var_name}=(${jwt_tokens_quoted})"

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

# Server lifecycle stays in this library's public surface. The body lives in
# conduit_server.sh so this file stays under the 1000-line cap.
# shellcheck source=tests/lib/conduit_server.sh # Startup, shutdown, and result analysis
[[ -n "${CONDUIT_SERVER_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/conduit_server.sh"
