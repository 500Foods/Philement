#!/usr/bin/env bash

# Test: Lithium Auth Integration Tests
# Runs the Vitest integration tests against a real Hydrogen server
#
# These tests verify:
# - Login with valid/invalid credentials
# - Token renewal
# - Logout
# - JWT claims structure
#
# CHANGELOG
# 1.0.0 - 2026-03-06 - Initial implementation

set -euo pipefail

# Test Configuration
TEST_NAME="Lithium Auth Integration"
TEST_ABBR="AUTH"
TEST_NUMBER="50"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Locate Lithium directory
LITHIUM_DIR="${PROJECT_DIR}/elements/003-lithium"

# Validate environment variables
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment"
env_vars_valid=true

if [[ -z "${HYDROGEN_SERVER_URL:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: HYDROGEN_SERVER_URL not set, using default http://localhost:8080"
    export HYDROGEN_SERVER_URL="http://localhost:8080"
fi

if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: HYDROGEN_DEMO_USER_NAME not set"
    env_vars_valid=false
fi

if [[ -z "${HYDROGEN_DEMO_USER_PASS:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: HYDROGEN_DEMO_USER_PASS not set"
    env_vars_valid=false
fi

if [[ -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: HYDROGEN_DEMO_API_KEY not set"
    env_vars_valid=false
fi

if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Environment variables configured"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Tests will be skipped if credentials are missing"
fi

# Check if Lithium directory exists
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Lithium Project"
if [[ -d "${LITHIUM_DIR}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found Lithium at: ${LITHIUM_DIR}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Lithium directory found"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Lithium directory not found at ${LITHIUM_DIR}"
    EXIT_CODE=1
fi

# Check if node_modules exists
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Dependencies"
if [[ -d "${LITHIUM_DIR}/node_modules" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Dependencies installed"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Installing dependencies..."
    cd "${LITHIUM_DIR}"
    if npm install --silent 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Dependencies installed successfully"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to install dependencies"
        EXIT_CODE=1
    fi
fi

# Test server connectivity
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Hydrogen Server"
if curl -s "${HYDROGEN_SERVER_URL}/api/system/health" --max-time 5 -o /dev/null; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen server is reachable at ${HYDROGEN_SERVER_URL}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server is online"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARNING: Hydrogen server not reachable at ${HYDROGEN_SERVER_URL}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server is offline or unreachable"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Integration tests will be skipped"
    EXIT_CODE=1
fi

# Run Vitest integration tests
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Run Integration Tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Vitest integration tests..."

    cd "${LITHIUM_DIR}"

    # Create a log file for test output
    test_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_vitest.log"

    # Run the tests and capture output
    if npm test -- --run tests/integration/ 2>&1 | tee "${test_log}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All integration tests passed"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Some integration tests failed"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "See log: ${test_log}"
        EXIT_CODE=1
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping integration tests due to prerequisite failures"
fi

# Print summary
print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All Lithium auth integration tests completed successfully"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Some tests failed or were skipped - check output above"
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
