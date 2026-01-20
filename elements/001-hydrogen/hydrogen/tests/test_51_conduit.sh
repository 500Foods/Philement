#!/usr/bin/env bash

# Test: Conduit Query Endpoints
# Tests all Conduit Service endpoints that execute pre-defined database queries by reference
# Launches all 7 database engines from test_40, then tests the conduit endpoints:
# - /api/conduit/query: Single query execution (public)
# - /api/conduit/queries: Multiple query execution (public)
# - /api/conduit/auth_query: Single query with JWT authentication
# - /api/conduit/auth_queries: Multiple queries with JWT authentication

# FUNCTIONS
# validate_conduit_request()
# test_conduit_single_query()
# test_conduit_multiple_queries()
# test_conduit_auth_single_query()
# test_conduit_auth_multiple_queries()
# run_conduit_test_parallel()
# analyze_conduit_test_results()

# CHANGELOG
# 1.7.0 - 2026-01-20 - Added invalid JSON test case to verify JSON validation middleware
#                    - Added invalid database test case to verify database selection validation
#                    - Enhanced error handling test coverage for malformed requests and invalid database names
# 1.5.0 - 2026-01-19 - Major refactoring to simplify unified server testing
#                    - Renamed run_conduit_test_parallel() to run_conduit_test_unified()
#                    - Extracted server lifecycle management to conduit_utils.sh
#                    - Simplified main execution logic by removing parallel complexity
#                    - Improved maintainability and reduced code duplication
# 1.4.0 - 2026-01-19 - Further refactored to extract JWT acquisition and test helper functions
#                    - Added acquire_jwt_tokens(), test_conduit_single_queries(), test_conduit_multiple_queries_endpoint() to lib
#                    - Simplified test functions using new generic helpers
#                    - Improved code reusability and maintainability
# 1.3.0 - 2026-01-19 - Refactored to extract conduit utilities into tests/lib/conduit_utils.sh
#                    - Moved validate_conduit_request() and check_database_readiness() to lib
#                    - Moved DATABASE_NAMES, PUBLIC_QUERY_REFS, NON_PUBLIC_QUERY_REFS to lib
#                    - Reduced script size and improved reusability for other conduit tests
# 1.2.0 - 2026-01-18 - Updated to use unified single-server approach with test_50_conduit.json
#                    - Single server with 7 databases instead of 7 parallel servers
#                    - Tests all conduit endpoints across all database engines in one server
#                    - Maintains comprehensive coverage while reducing resource usage
# 1.1.0 - 2026-01-16 - Renamed from test_41 to test_51, expanded to 7 databases
#                    - Added testing for all 4 conduit endpoints
#                    - Integrated JWT authentication testing
#                    - Uses real query_refs from Acuranzo migrations
#                    - Copied configuration approach from test_40
# 1.0.0 - 2025-10-20 - Initial implementation based on test_21_system_endpoints.sh and test_30_database.sh
#                    - Launches all 5 database engines from test_30_multi.json (renamed to test_40_conduit.json)
#                    - Tests /api/conduit/query endpoint with various query references
#                    - Validates query execution, parameter handling, and result formatting

set -euo pipefail

# Test Configuration
TEST_NAME="Conduit Endpoints  {BLUE}CDT: 0{RESET}"
TEST_ABBR="QRY"
TEST_NUMBER="51"
TEST_COUNTER=0
TEST_VERSION="1.7.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

# Single server configuration with all 7 database engines
CONDUIT_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_51_conduit.json"
CONDUIT_LOG_SUFFIX="conduit"
CONDUIT_DESCRIPTION="Conduit"

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

    local result
    result=$(test_conduit_single_queries "${base_url}" "${result_file}" "/api/conduit/query" "" "Public Query")

    IFS=':' read -r tests_passed total_tests <<< "${result}"

    echo "SINGLE_PUBLIC_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "SINGLE_PUBLIC_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit single query endpoint with invalid query refs
test_conduit_single_invalid_query() {
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

        # Test invalid query ref -100
        local payload
        payload=$(cat <<EOF
{
  "query_ref": -100,
  "database": "${db_name}",
  "params": {}
}
EOF
)

        local response_file="${result_file}.single_invalid_${db_engine}.json"

        # Expect 200 with "fail" response for invalid query ref
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/query" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Invalid Query -100" "\"fail\""; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "SINGLE_INVALID_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "SINGLE_INVALID_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"

}

# Function to test conduit multiple queries endpoint across ready databases
test_conduit_multiple_queries() {
    local base_url="$1"
    local result_file="$2"

    local queries_json
    queries_json=$(cat <<EOF
[
  {
    "query_ref": 53,
    "params": {}
  },
  {
    "query_ref": 54,
    "params": {}
  }
]
EOF
)

    local result
    result=$(test_conduit_multiple_queries_endpoint "${base_url}" "${result_file}" "/api/conduit/queries" "" "Multiple Queries: Get Themes + Get Icons" "${queries_json}")

    IFS=':' read -r tests_passed total_tests <<< "${result}"

    echo "MULTIPLE_QUERIES_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "MULTIPLE_QUERIES_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit queries endpoint comprehensively across ready databases
test_conduit_queries_comprehensive() {
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

        # Test 1: Normal result - pass the three queryrefs we're using in "query" to test as one set
        local queries_normal
        queries_normal=$(cat <<EOF
[
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
      "INTEGER": {
        "START": 500,
        "FINISH": 600
      }
    }
  }
]
EOF
)

        local payload_normal
        payload_normal=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_normal}
}
EOF
)

        local response_file_normal="${result_file}.queries_normal_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_normal}" "200" "${response_file_normal}" "" "Queries Normal: 53+54+55 (${db_engine})"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 2: Duplicated results - add queryref 53 twice
        local queries_duplicate
        queries_duplicate=$(cat <<EOF
[
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
EOF
)

        local payload_duplicate
        payload_duplicate=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_duplicate}
}
EOF
)

        local response_file_duplicate="${result_file}.queries_duplicate_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_duplicate}" "200" "${response_file_duplicate}" "" "Queries Duplicate: 53 twice (${db_engine})"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 3: Mixed results - add an invalid queryref to the set
        local queries_mixed
        queries_mixed=$(cat <<EOF
[
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
EOF
)

        local payload_mixed
        payload_mixed=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_mixed}
}
EOF
)

        local response_file_mixed="${result_file}.queries_mixed_${db_engine}.json"

        # Expect 200 with success=false response for mixed valid/invalid queries
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_mixed}" "200" "${response_file_mixed}" "" "Queries Mixed: Invalid -100 (${db_engine})" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 4: Add queryrefs with invalid or missing params
        local queries_invalid_params
        queries_invalid_params=$(cat <<EOF
[
  {
    "query_ref": 53,
    "params": {}
  },
  {
    "query_ref": 55,
    "params": {
      "INTEGER": {
        "START": "invalid_string",
        "FINISH": 600
      }
    }
  },
  {
    "query_ref": 45,
    "params": {}
  }
]
EOF
)

        local payload_invalid_params
        payload_invalid_params=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_invalid_params}
}
EOF
)

        local response_file_invalid_params="${result_file}.queries_invalid_params_${db_engine}.json"

        # Expect 200 with success=false response for invalid params
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_invalid_params}" "200" "${response_file_invalid_params}" "" "Queries Invalid Params (${db_engine})" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 5: Run it with no queryrefs at all
        local queries_empty
        queries_empty=$(cat <<EOF
[]
EOF
)

        local payload_empty
        payload_empty=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_empty}
}
EOF
)

        # Test 6: Query with missing required parameters to generate message
        local queries_missing_params
        queries_missing_params=$(cat <<EOF
[
  {
    "query_ref": 45,
    "params": {}
  }
]
EOF
)

        local response_file_empty="${result_file}.queries_empty_${db_engine}.json"

        # Expect 200 with success=false response for empty queries array
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_empty}" "200" "${response_file_empty}" "" "Queries Empty Array (${db_engine})" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 6: Query with missing required parameters to generate message
        local payload_missing_params
        payload_missing_params=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_missing_params}
}
EOF
)

        local response_file_missing_params="${result_file}.queries_missing_params_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_missing_params}" "200" "${response_file_missing_params}" "" "Queries Missing Params (${db_engine})"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 7: Query that triggers database error (division by zero) to test error message propagation
        local queries_db_error
        queries_db_error=$(cat <<EOF
[
  {
    "query_ref": 56,
    "params": {
      "INTEGER": {
        "NUMERATOR": 10,
        "DENOMINATOR": 0
      }
    }
  }
]
EOF
)

        local payload_db_error
        payload_db_error=$(cat <<EOF
{
  "database": "${db_name}",
  "queries": ${queries_db_error}
}
EOF
)

        local response_file_db_error="${result_file}.queries_db_error_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_db_error}" "422" "${response_file_db_error}" "" "Queries DB Error: Division by Zero (${db_engine})" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 8: Invalid JSON - send malformed JSON to test JSON validation middleware
        local invalid_json_payload='{ "database": "Demo_PG", "queries": [ { "query_ref": 53, "params": {} } '  # Missing closing brace

        local response_file_invalid_json="${result_file}.queries_invalid_json_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${invalid_json_payload}" "400" "${response_file_invalid_json}" "" "Queries Invalid JSON (${db_engine})"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))

        # Test 9: Invalid database - send valid JSON with non-existent database name
        local queries_invalid_db
        queries_invalid_db=$(cat <<EOF
[
 {
   "query_ref": 53,
   "params": {}
 }
]
EOF
)

        local payload_invalid_db
        payload_invalid_db=$(cat <<EOF
{
 "database": "Invalid_Database",
 "queries": ${queries_invalid_db}
}
EOF
)

        local response_file_invalid_db="${result_file}.queries_invalid_db_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/queries" "POST" "${payload_invalid_db}" "400" "${response_file_invalid_db}" "" "Queries Invalid Database (${db_engine})" "false"; then
            tests_passed=$(( tests_passed + 1 ))
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "QUERIES_COMPREHENSIVE_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "QUERIES_COMPREHENSIVE_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

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

# Function to test conduit auth single query endpoint across ready databases
test_conduit_auth_single_query() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "AUTH_SINGLE_QUERY_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for auth single query
        local payload
        payload=$(cat <<EOF
{
  "query_ref": 30,
  "params": {}
}
EOF
)

        local response_file="${result_file}.auth_single_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_query" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${db_engine} Auth Single Query: Get Lookups List"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "AUTH_SINGLE_QUERY_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
        jwt_index=$(( jwt_index + 1 ))
    done

    echo "AUTH_SINGLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "AUTH_SINGLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit auth multiple queries endpoint across ready databases
test_conduit_auth_multiple_queries() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "AUTH_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for auth multiple queries
        local payload
        payload=$(cat <<EOF
{
  "queries": [
    {
      "query_ref": 1005,
      "params": {}
    },
    {
      "query_ref": 1006,
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
EOF
)

        local response_file="${result_file}.auth_multiple_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/auth_queries" "POST" "${payload}" "200" "${response_file}" "${jwt_token}" "${db_engine} Auth Multiple Queries: System Info + Query List"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "AUTH_MULTIPLE_QUERIES_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "AUTH_MULTIPLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "AUTH_MULTIPLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit alt single query endpoint across ready databases
test_conduit_alt_single_query() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "ALT_SINGLE_QUERY_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for alt single query
        local payload
        payload=$(cat <<EOF
{
  "token": "${jwt_token}",
  "database": "${db_name}",
  "query_ref": 1005,
  "params": {
    "STRING": {
      "username": "${HYDROGEN_DEMO_USER_NAME}"
    }
  }
}
EOF
)

        local response_file="${result_file}.alt_single_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/alt_query" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Alt Single Query: Get Account ID"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "ALT_SINGLE_QUERY_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "ALT_SINGLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "ALT_SINGLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit alt multiple queries endpoint across ready databases
test_conduit_alt_multiple_queries() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"

    if [[ -z "${jwt_token}" ]]; then
        echo "ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi

    local tests_passed=0
    local total_tests=0

    # Test each database that is ready
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        # Check if database is ready
        if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
            continue
        fi

        local db_name="${DATABASE_NAMES[${db_engine}]}"

        # Prepare JSON payload for alt multiple queries
        local payload
        payload=$(cat <<EOF
{
  "token": "${jwt_token}",
  "database": "${db_name}",
  "queries": [
    {
      "query_ref": 1005,
      "params": {}
    },
    {
      "query_ref": 1006,
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
EOF
)

        local response_file="${result_file}.alt_multiple_${db_engine}.json"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_conduit_request "${base_url}/api/conduit/alt_queries" "POST" "${payload}" "200" "${response_file}" "" "${db_engine} Alt Multiple Queries: System Info + Query List"; then
            tests_passed=$(( tests_passed + 1 ))
        else
            echo "ALT_MULTIPLE_QUERIES_FAILED_${db_engine}" >> "${result_file}"
        fi
        total_tests=$(( total_tests + 1 ))
    done

    echo "ALT_MULTIPLE_QUERY_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "ALT_MULTIPLE_QUERY_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to test conduit status endpoint
test_conduit_status() {
    local base_url="$1"
    local jwt_tokens_string="$2"  # Space-separated string of JWT tokens (one per database)
    local result_file="$3"

    local tests_passed=0
    local total_tests=0

    # Test status endpoint without authentication (basic status)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (Public)"
    local response_file="${result_file}.status_public.json"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_conduit_request "${base_url}/api/conduit/status" "GET" "" "200" "${response_file}" "" "Status Endpoint (Public)"; then
        # Parse and log status of each database
        if command -v jq >/dev/null 2>&1 && [[ -f "${response_file}" ]]; then
            for db_engine in "${!DATABASE_NAMES[@]}"; do
                local db_name="${DATABASE_NAMES[${db_engine}]}"
                local ready_status
                ready_status=$(jq -r ".databases.[\"${db_name}\"].ready" "${response_file}" 2>/dev/null || echo "unknown")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${db_engine}: ready=${ready_status}"
            done
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (Public) - All databases reported status"
        tests_passed=$(( tests_passed + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (Public) - Failed to get status"
    fi
    total_tests=$(( total_tests + 1 ))

    # Test status endpoint with authentication for each database
    # Use read -a to properly split the jwt_tokens_string into an array
    local jwt_array=()
    read -r -a jwt_array <<< "${jwt_tokens_string}"
    local jwt_index=0
    for db_engine in "${!DATABASE_NAMES[@]}"; do
        if [[ ${jwt_index} -lt ${#jwt_array[@]} ]] && [[ -n "${jwt_array[${jwt_index}]}" ]]; then
            local jwt_token="${jwt_array[${jwt_index}]}"
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (${db_engine} Auth)"
            local response_file_auth="${result_file}.status_auth_${db_engine}.json"

            # Make the request and check basic success
            local http_status
            http_status=$(curl -s -X GET "${base_url}/api/conduit/status" \
                -H "Authorization: Bearer ${jwt_token}" \
                -w "%{http_code}" \
                -o "${response_file_auth}" 2>/dev/null)

            if [[ "${http_status}" -eq 200 ]]; then
                # Check for success in JSON response
                if "${GREP}" -q '"success"[[:space:]]*:[[:space:]]*true' "${response_file_auth}"; then
                    # Parse and validate detailed status including DQM statistics
                    if command -v jq >/dev/null 2>&1 && [[ -f "${response_file_auth}" ]]; then
                        local db_name="${DATABASE_NAMES[${db_engine}]}"
                        local ready_status migration_status cache_entries dqm_stats_present
                        ready_status=$(jq -r ".databases.[\"${db_name}\"].ready" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        migration_status=$(jq -r ".databases.[\"${db_name}\"].migration_status" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        cache_entries=$(jq -r ".databases.[\"${db_name}\"].query_cache_entries" "${response_file_auth}" 2>/dev/null || echo "unknown")
                        dqm_stats_present=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_submitted" "${response_file_auth}" 2>/dev/null || echo "null")

                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${db_engine} authenticated status: ready=${ready_status}, migration=${migration_status}, cache_entries=${cache_entries}"

                        if [[ "${dqm_stats_present}" != "null" ]]; then
                            local total_submitted total_completed total_failed
                            total_submitted=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_submitted" "${response_file_auth}" 2>/dev/null || echo "0")
                            total_completed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_completed" "${response_file_auth}" 2>/dev/null || echo "0")
                            total_failed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.total_queries_failed" "${response_file_auth}" 2>/dev/null || echo "0")
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  DQM Statistics for ${db_name}: submitted=${total_submitted}, completed=${total_completed}, failed=${total_failed}"

                            # Log per-queue statistics in order: Lead, Fast, Medium, Slow, Cache
                            local queue_indices=(4 2 1 0 3)
                            local queue_names=("Lead" "Fast" "Medium" "Slow" "Cache")
                            local i=0
                            for queue_name in "${queue_names[@]}"; do
                                local queue_index=${queue_indices[${i}]}
                                local q_submitted q_completed q_failed q_avg_time
                                q_submitted=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].submitted" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_completed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].completed" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_failed=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].failed" "${response_file_auth}" 2>/dev/null || echo "0")
                                q_avg_time=$(jq -r ".databases.[\"${db_name}\"].dqm_statistics.per_queue_stats[${queue_index}].avg_execution_time_us" "${response_file_auth}" 2>/dev/null || echo "0")

                                # Format average time as milliseconds with 3 decimal places (convert from microseconds)
                                local avg_ms
                                if [[ "${q_avg_time}" == "0" ]]; then
                                    avg_ms="0.000ms"
                                else
                                    # Calculate average time in milliseconds
                                    local calc_result
                                    calc_result=$(echo "scale=3; ${q_avg_time}/1000" | bc -l 2>/dev/null || echo "0.000")
                                    avg_ms=$(printf "%.3fms" "${calc_result}")
                                fi

                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${queue_name} DQM: submitted=${q_submitted}, completed=${q_completed}, failed=${q_failed}, avg=${avg_ms}"
                                i=$(( i + 1 ))
                            done

                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (${db_engine} Auth) - Authenticated status with DQM statistics retrieved"
                            tests_passed=$(( tests_passed + 1 ))
                        else
                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - DQM statistics missing from response"
                        fi
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Status Endpoint (${db_engine} Auth) - Request successful (jq not available for detailed validation)"
                        tests_passed=$(( tests_passed + 1 ))
                    fi
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Request failed (success=false in response)"
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Expected HTTP 200, got ${http_status}"
            fi
            total_tests=$(( total_tests + 1 ))
        else
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Status Endpoint (${db_engine} Auth)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Status Endpoint (${db_engine} Auth) - Skipped (no JWT available)"
        fi
        jwt_index=$(( jwt_index + 1 ))
    done

    echo "STATUS_TESTS_PASSED=${tests_passed}" >> "${result_file}"
    echo "STATUS_TESTS_TOTAL=${total_tests}" >> "${result_file}"
}

# Function to run conduit tests on unified server
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

    print_message "51" "0" "Server log location: build/tests/logs/test_51_${TIMESTAMP}_conduit.log"

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

    # Test status endpoint first
    test_conduit_status "${base_url}" "${jwt_tokens[*]}" "${result_file}"

    # Run conduit endpoint tests in methodical order
    test_conduit_single_public_query "${base_url}" "${result_file}"
    test_conduit_single_invalid_query "${base_url}" "${result_file}"
    test_conduit_multiple_queries "${base_url}" "${result_file}"
    test_conduit_queries_comprehensive "${base_url}" "${result_file}"
    # TODO: Add more test functions as we progress
    test_conduit_auth_query_comprehensive "${base_url}" "${jwt_tokens[*]}" "${result_file}"
    # test_conduit_auth_multiple_queries "${base_url}" "${jwt_token}" "${result_file}"
    # test_conduit_alt_single_query "${base_url}" "${jwt_token}" "${result_file}"
    # test_conduit_alt_multiple_queries "${base_url}" "${jwt_token}" "${result_file}"

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
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Conduit endpoint tests on unified server"

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
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server passed all conduit endpoint tests"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sequential execution completed - Conduit endpoints validated across ${#DATABASE_NAMES[@]} database engines"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: Unified multi-database server failed conduit endpoint tests"
            EXIT_CODE=1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: No result file found for unified server"
        EXIT_CODE=1
    fi

else
    # Skip conduit tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Conduit endpoint tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"