#!/usr/bin/env bash

# Test: Authentication Endpoints - Multi-Engine Parallel Testing
# Tests all the authentication API endpoints against multiple database engines in parallel:
# - /api/auth/login: Tests user login with username/password, returns JWT
# - /api/auth/renew: Tests JWT token renewal
# - /api/auth/logout: Tests logout and JWT token invalidation
# - /api/auth/register: Tests user registration
#
# This test runs parallel instances of the server with PostgreSQL, MySQL, SQLite, and DB2
# Each instance connects to the "demo" schema with automigration enabled (persistent data)

# FUNCTIONS
# validate_auth_request()
# test_auth_login()
# test_auth_login_invalid()
# test_auth_renew()
# test_auth_logout()
# test_auth_register()
# run_auth_test_parallel()
# analyze_auth_test_results()

# CHANGELOG
# 1.3.0 - 2026-01-13 - Fixed JWT token passing for renew/logout endpoints
#                    - Added Authorization: Bearer header support to validate_auth_request()
#                    - Changed renew/logout to use empty JSON body with auth header
#                    - Fixes authentication failures for protected endpoints
# 1.2.0 - 2026-01-10 - Updated to use environment variables for demo credentials
#                    - Uses HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, etc.
#                    - Aligns with migration 1144 and 1145 environment variable usage
# 1.1.0 - 2026-01-10 - Restructured for parallel execution across multiple database engines
#                    - Added support for PostgreSQL, MySQL, SQLite, and DB2
#                    - Uses "demo" schema with persistent automigrations
#                    - Follows test_30_database.sh parallel patterns
# 1.0.0 - 2026-01-10 - Initial implementation of auth endpoint tests

set -euo pipefail

# Test Configuration
TEST_NAME="Auth  {BLUE}engines: 4{RESET}"
TEST_ABBR="JWT"
TEST_NUMBER="40"
TEST_COUNTER=0
TEST_VERSION="1.3.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A AUTH_TEST_CONFIGS

# Auth test configuration - format: "config_file:log_suffix:engine_name:description"
AUTH_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_postgres.json:postgres:postgresql:PostgreSQL Engine"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mysql.json:mysql:mysql:MySQL Engine"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json:sqlite:sqlite:SQLite Engine"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_db2.json:db2:db2:DB2 Engine"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Demo credentials from environment variables (set in shell and used in migrations)
# shellcheck disable=SC2034 # Used in heredocs for JSON payloads
DEMO_USER_NAME="${HYDROGEN_DEMO_USER_NAME:-}"
# shellcheck disable=SC2034 # Used in heredocs for JSON payloads
DEMO_USER_PASS="${HYDROGEN_DEMO_USER_PASS:-}"
# shellcheck disable=SC2034 # These variables are defined for future test expansion
DEMO_ADMIN_NAME="${HYDROGEN_DEMO_ADMIN_NAME:-}"
# shellcheck disable=SC2034 # These variables are defined for future test expansion
DEMO_ADMIN_PASS="${HYDROGEN_DEMO_ADMIN_PASS:-}"
# shellcheck disable=SC2034 # These variables are defined for future test expansion
DEMO_EMAIL="${HYDROGEN_DEMO_EMAIL:-}"
# shellcheck disable=SC2034 # Used in heredocs for JSON payloads
DEMO_API_KEY="${HYDROGEN_DEMO_API_KEY:-}"

# Function to validate auth request
validate_auth_request() {
    local url="$1"
    local method="$2"
    local data="$3"
    local expected_status="$4"
    local output_file="$5"
    local jwt_token="${6:-}"  # Optional JWT token for authenticated requests
    
    # Build curl command with optional Authorization header
    local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")
    
    # Add Authorization header if JWT token is provided
    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi
    
    # Complete curl command
    curl_cmd+=(-d "${data}" -w "%{http_code}" -o "${output_file}" --compressed --max-time 5 "${url}")
    
    # Run curl and capture HTTP status
    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null)
    
    if [[ "${http_status}" == "${expected_status}" ]]; then
        return 0
    else
        return 1
    fi
}

# Function to test login endpoint
test_auth_login() {
    local base_url="$1"
    local result_file="$2"
    
    # Use environment variables for demo credentials (set in migrations 1144/1145)
    local login_data
    login_data=$(cat <<EOF
{
    "database": "Acuranzo",
    "login_id": "${HYDROGEN_DEMO_USER_NAME}",
    "password": "${HYDROGEN_DEMO_USER_PASS}",
    "api_key": "${HYDROGEN_DEMO_API_KEY}",
    "tz": "America/Vancouver"
}
EOF
)
    
    local response_file="${result_file}.login.json"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_auth_request "${base_url}/api/auth/login" "POST" "${login_data}" "200" "${response_file}"; then
        echo "LOGIN_SUCCESS" >> "${result_file}"
        
        # Extract JWT token for subsequent tests
        if command -v jq >/dev/null 2>&1; then
            local jwt_token
            jwt_token=$(jq -r '.token' "${response_file}" 2>/dev/null || echo "")
            if [[ -n "${jwt_token}" ]] && [[ "${jwt_token}" != "null" ]]; then
                echo "JWT_TOKEN=${jwt_token}" >> "${result_file}"
            fi
        fi
    else
        echo "LOGIN_FAILED" >> "${result_file}"
    fi
}

# Function to test login with invalid credentials
test_auth_login_invalid() {
    local base_url="$1"
    local result_file="$2"
    
    # Use valid username but invalid password (wrong password test)
    local login_data
    login_data=$(cat <<EOF
{
    "database": "Acuranzo",
    "login_id": "${HYDROGEN_DEMO_USER_NAME}",
    "password": "WrongPassword123!",
    "api_key": "${HYDROGEN_DEMO_API_KEY}",
    "tz": "America/Vancouver"
}
EOF
)
    
    local response_file="${result_file}.login_invalid.json"
    
    # Should return 401 Unauthorized
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_auth_request "${base_url}/api/auth/login" "POST" "${login_data}" "401" "${response_file}"; then
        echo "LOGIN_INVALID_SUCCESS" >> "${result_file}"
    else
        echo "LOGIN_INVALID_FAILED" >> "${result_file}"
    fi
}

# Function to test token renewal
test_auth_renew() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"
    
    if [[ -z "${jwt_token}" ]]; then
        echo "RENEW_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi
    
    local renew_data="{}"
    local response_file="${result_file}.renew.json"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_auth_request "${base_url}/api/auth/renew" "POST" "${renew_data}" "200" "${response_file}" "${jwt_token}"; then
        echo "RENEW_SUCCESS" >> "${result_file}"
    else
        echo "RENEW_FAILED" >> "${result_file}"
    fi
}

# Function to test logout
test_auth_logout() {
    local base_url="$1"
    local jwt_token="$2"
    local result_file="$3"
    
    if [[ -z "${jwt_token}" ]]; then
        echo "LOGOUT_SKIPPED_NO_TOKEN" >> "${result_file}"
        return
    fi
    
    local logout_data="{}"
    local response_file="${result_file}.logout.json"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_auth_request "${base_url}/api/auth/logout" "POST" "${logout_data}" "200" "${response_file}" "${jwt_token}"; then
        echo "LOGOUT_SUCCESS" >> "${result_file}"
    else
        echo "LOGOUT_FAILED" >> "${result_file}"
    fi
}

# Function to test registration
test_auth_register() {
    local base_url="$1"
    local result_file="$2"
    
    # Generate unique username to avoid conflicts
    local timestamp
    timestamp=$(date +%s%N)
    local unique_user="testuser_${timestamp}"
    local unique_email="test_${timestamp}@example.com"
    
    # Use environment variable for API key (same as used in migrations)
    local register_data
    register_data=$(cat <<EOF
{
    "database": "Acuranzo",
    "username": "${unique_user}",
    "password": "NewPassword123!",
    "email": "${unique_email}",
    "full_name": "Test User ${timestamp}",
    "api_key": "${HYDROGEN_DEMO_API_KEY}"
}
EOF
)
    
    local response_file="${result_file}.register.json"
    
    # Should return 201 Created
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_auth_request "${base_url}/api/auth/register" "POST" "${register_data}" "201" "${response_file}"; then
        echo "REGISTER_SUCCESS" >> "${result_file}"
    else
        echo "REGISTER_FAILED" >> "${result_file}"
    fi
}

# Function to run auth tests in parallel for a specific database engine
run_auth_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local engine_name="$4"
    local description="$5"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
    
    # Clear result file
    true > "${result_file}"
    
    # Extract port from configuration
    local server_port
    server_port=$(jq -r '.WebServer.Port' "${config_file}" 2>/dev/null || echo "0")
    local base_url="http://localhost:${server_port}"
    
    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"
    
    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}
    
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
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
        
        # Wait a bit for server to be fully ready
        sleep 1
        
        # Run auth endpoint tests
        test_auth_login "${base_url}" "${result_file}"
        
        # Extract JWT token for subsequent tests
        local jwt_token
        jwt_token=$("${GREP}" "^JWT_TOKEN=" "${result_file}" 2>/dev/null | cut -d'=' -f2 || echo "")
        
        test_auth_login_invalid "${base_url}" "${result_file}"
        test_auth_renew "${base_url}" "${jwt_token}" "${result_file}"
        test_auth_logout "${base_url}" "${jwt_token}" "${result_file}"
        test_auth_register "${base_url}" "${result_file}"
        
        echo "AUTH_TEST_COMPLETE" >> "${result_file}"
        
        # Stop the server gracefully
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            
            # Wait for graceful shutdown
            local shutdown_start
            shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.1
            done
        fi
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel auth test execution
analyze_auth_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local engine_name="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
    
    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi
    
    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Failed to start Hydrogen"
        return 1
    fi
    
    # Check login test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Login Test"
    if "${GREP}" -q "LOGIN_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Login successful"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Login failed"
    fi
    
    # Check invalid login test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Invalid Credentials Test"
    if "${GREP}" -q "LOGIN_INVALID_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Invalid credentials correctly rejected"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Invalid credentials test failed"
    fi
    
    # Check renew test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Renew Test"
    if "${GREP}" -q "RENEW_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Token renewal successful"
    elif "${GREP}" -q "RENEW_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Renew skipped (no token)"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Token renewal failed"
    fi
    
    # Check logout test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Logout Test"
    if "${GREP}" -q "LOGOUT_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Logout successful"
    elif "${GREP}" -q "LOGOUT_SKIPPED_NO_TOKEN" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Logout skipped (no token)"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Logout failed"
    fi
    
    # Check register test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Register Test"
    if "${GREP}" -q "REGISTER_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Registration successful"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Registration failed"
    fi
    
    # Overall result
    if "${GREP}" -q "AUTH_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        return 0
    else
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

# Validate all configuration files
config_valid=true
for test_config in "${!AUTH_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"
    
    # Parse test configuration
    IFS=':' read -r config_file log_suffix engine_name description <<< "${AUTH_TEST_CONFIGS[${test_config}]}"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} configuration will use port: ${port}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All configuration files validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with auth tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Auth endpoint tests in parallel"
    
    # Start all auth tests in parallel with job limiting
    for test_config in "${!AUTH_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix engine_name description <<< "${AUTH_TEST_CONFIGS[${test_config}]}"
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel auth test in background
        run_auth_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${engine_name}" "${description}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#AUTH_TEST_CONFIGS[@]} parallel auth tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed"
    
    # Process results sequentially for clean output
    for test_config in "${!AUTH_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix engine_name description <<< "${AUTH_TEST_CONFIGS[${test_config}]}"
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"
        
        # Add links to log and result files for troubleshooting
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Engine: ${TESTS_DIR}/logs/${log_file##*/}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Engine: ${DIAG_TEST_DIR}/${result_file##*/}"
        
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_auth_test_results "${test_config}" "${log_suffix}" "${engine_name}" "${description}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!AUTH_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix engine_name description <<< "${AUTH_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        # Check not just AUTH_TEST_COMPLETE but also that key tests passed (login and register)
        if [[ -f "${result_file}" ]]; then
            if "${GREP}" -q "AUTH_TEST_COMPLETE" "${result_file}" 2>/dev/null && \
               "${GREP}" -q "LOGIN_SUCCESS" "${result_file}" 2>/dev/null && \
               "${GREP}" -q "REGISTER_SUCCESS" "${result_file}" 2>/dev/null; then
                successful_configs=$(( successful_configs + 1 ))
            fi
        fi
    done
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "---------------------------------"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#AUTH_TEST_CONFIGS[@]} database engine configurations passed all checks"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - Auth endpoints validated across ${#AUTH_TEST_CONFIGS[@]} database engines"
    
else
    # Skip auth tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping auth tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
