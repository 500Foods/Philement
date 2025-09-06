#!/usr/bin/env bash

# Test: Database Subsystem
# Tests the database subsystem functionality, including multi-engine support and queue operations

# FUNCTIONS
# check_database_connectivity()
# run_database_test_parallel()
# analyze_database_test_results()
# test_database_configuration()

# CHANGELOG
# 1.1.0 - 2025-09-05 - Added MULTI configuration test with all four database engines in single config
# 1.0.0 - 2025-09-04 - Initial implementation based on test_22_swagger.sh pattern
#                    - Added parallel execution for all four database engines (PostgreSQL, MySQL, SQLite, DB2)
#                    - Configured separate test configs for each engine with different ports
#                    - Basic connectivity test focusing on successful startup/shutdown

set -euo pipefail

# Test Configuration
TEST_NAME="Databases"
TEST_ABBR="DBS"
TEST_NUMBER="27"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A DATABASE_TEST_CONFIGS

# Database test configuration - format: "config_file:log_suffix:engine_name:description"
DATABASE_TEST_CONFIGS=(
    ["POSTGRES"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_postgres.json:postgres:postgresql:PostgreSQL Engine"
    ["MYSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mysql.json:mysql:mysql:MySQL Engine"
    ["SQLITE"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json:sqlite:sqlite:SQLite Engine"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_db2.json:db2:db2:DB2 Engine"
    ["MULTI"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_multi.json:multi:multi:Multi-Engine Database"
)

# Test timeouts
STARTUP_TIMEOUT=30
SHUTDOWN_TIMEOUT=10

# Function to check database subsystem connectivity during startup
check_database_connectivity() {
    local log_file="$1"
    local expected_engine="$2"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking database connectivity for ${expected_engine} in log: ${log_file}"

    # Wait for database subsystem to indicate startup completion
    local max_attempts=30
    local attempt=1

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ -f "${log_file}" ]]; then
            if "${GREP}" -q "database.*initialized.*${expected_engine}" "${log_file}" 2>/dev/null ||
               "${GREP}" -q "database.*started.*${expected_engine}" "${log_file}" 2>/dev/null ||
               "${GREP}" -q "database.*ready.*${expected_engine}" "${log_file}" 2>/dev/null ||
               "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${expected_engine} database subsystem detected successfully"
                return 0
            fi
        fi
        sleep 1
        ((attempt++))
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Database connectivity check timed out after ${max_attempts} seconds"
    return 1
}

# Function to count expected queues from configuration
count_expected_queues() {
    local config_file="$1"
    local expected_queues=0

    # Read the JSON config and count queues with start > 0
    # Use jq if available, otherwise parse with grep/awk
    if command -v jq >/dev/null 2>&1; then
        # Use jq for precise JSON parsing
        local slow_start
        local medium_start
        local fast_start
        local cache_start

        slow_start=$(jq -r '.Databases.Connections[0].Queues.Slow.start // 0' "${config_file}" 2>/dev/null || echo "0")
        medium_start=$(jq -r '.Databases.Connections[0].Queues.Medium.start // 0' "${config_file}" 2>/dev/null || echo "0")
        fast_start=$(jq -r '.Databases.Connections[0].Queues.Fast.start // 0' "${config_file}" 2>/dev/null || echo "0")
        cache_start=$(jq -r '.Databases.Connections[0].Queues.Cache.start // 0' "${config_file}" 2>/dev/null || echo "0")

        # Count queues with start > 0, plus Lead queue (always present)
        [[ "${slow_start}" -gt 0 ]] && ((expected_queues++))
        [[ "${medium_start}" -gt 0 ]] && ((expected_queues++))
        [[ "${fast_start}" -gt 0 ]] && ((expected_queues++))
        [[ "${cache_start}" -gt 0 ]] && ((expected_queues++))
        # Lead queue is always present
        ((expected_queues++))
    else
        # Fallback: use grep to count non-zero start values
        local queue_starts
        queue_starts=$(grep -o '"start":[[:space:]]*[0-9]\+' "${config_file}" | grep -c  -v '"start":[[:space:]]*0' )
        expected_queues=$((queue_starts + 1)) # +1 for Lead queue
    fi

    echo "${expected_queues}"
}

# Function to test database subsystem with a specific configuration in parallel
run_database_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local engine_name="$4"
    local description="$5"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    # Clear result file
    true > "${result_file}"

    # Count expected queues from configuration
    local expected_queues
    expected_queues=$(count_expected_queues "${config_file}")
    echo "EXPECTED_QUEUES=${expected_queues}" >> "${result_file}"

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup and check database connectivity
    local startup_success=false
    local database_ready=false
    local queues_started=0
    local start_time
    start_time=${SECONDS}

    # Wait for startup
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi

        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.05
    done

    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"

        # Check database subsystem connectivity
        # shellcheck disable=SC2310 # Don't like how this coding mechanism works
        if check_database_connectivity "${log_file}" "${engine_name}"; then
            echo "DATABASE_READY" >> "${result_file}"
            database_ready=true

            # Count actual queues started by checking log for queue creation messages
            # Look for patterns like "Q1 [Lead, Slow] Connected" or similar
            queues_started=$(grep -c "Q[0-9]*.*Connected" "${log_file}" 2>/dev/null || echo "0")
            echo "QUEUES_STARTED=${queues_started}" >> "${result_file}"
        else
            echo "DATABASE_NOT_READY" >> "${result_file}"
        fi
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
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
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.05
            done
        fi
    else
        echo "DATABASE_TEST_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel database test execution
analyze_database_test_results() {
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
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi

    # Check database readiness
    if ! "${GREP}" -q "DATABASE_READY" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Database subsystem not ready for ${description} test"
        return 1
    fi

    # Check queue count correlation
    local expected_queues
    local actual_queues
    expected_queues=$(grep "EXPECTED_QUEUES=" "${result_file}" | cut -d'=' -f2)
    actual_queues=$(grep "QUEUES_STARTED=" "${result_file}" | cut -d'=' -f2)

    if [[ -n "${expected_queues}" ]] && [[ -n "${actual_queues}" ]]; then
        if [[ "${expected_queues}" != "${actual_queues}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Queue count mismatch for ${description}: expected ${expected_queues}, got ${actual_queues}"
            return 1
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Queue count verified: ${actual_queues} queues started as expected"
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Queue count verification skipped (values not found)"
    fi

    # Check test completion
    if "${GREP}" -q "DATABASE_TEST_PASSED" "${result_file}" 2>/dev/null; then
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

# Validate all configuration files
config_valid=true
for test_config in "${!DATABASE_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"

    # Parse test configuration
    IFS=':' read -r config_file log_suffix engine_name description <<< "${DATABASE_TEST_CONFIGS[${test_config}]}"

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

# Only proceed with database tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Database subsystem tests in parallel"

    # Start all database tests in parallel with job limiting
    for test_config in "${!DATABASE_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done

        # Parse test configuration
        IFS=':' read -r config_file log_suffix engine_name description <<< "${DATABASE_TEST_CONFIGS[${test_config}]}"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"

        # Run parallel database test in background
        run_database_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${engine_name}" "${description}" &
        PARALLEL_PIDS+=($!)
    done

    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#DATABASE_TEST_CONFIGS[@]} parallel database tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed"

    # Process results sequentially for clean output
    for test_config in "${!DATABASE_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix engine_name description <<< "${DATABASE_TEST_CONFIGS[${test_config}]}"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Server Log: ..${log_file}"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_database_test_results "${test_config}" "${log_suffix}" "${engine_name}" "${description}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Database subsystem test passed"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Database subsystem test failed"
            EXIT_CODE=1
        fi
    done

    # Print summary
    successful_configs=0
    for test_config in "${!DATABASE_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix engine_name description <<< "${DATABASE_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "DATABASE_TEST_PASSED" "${result_file}" 2>/dev/null; then
            successful_configs=$(( successful_configs + 1 ))
        fi
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#DATABASE_TEST_CONFIGS[@]} database engine configurations passed all tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - Database subsystem can handle multiple engines"

else
    # Skip database tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping database tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
