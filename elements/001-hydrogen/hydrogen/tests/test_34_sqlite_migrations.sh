#!/usr/bin/env bash

# Test: SQLite Migrations
# Tests the database migration functionality for SQLite
# Launches hydrogen with migration config and waits for completion

# FUNCTIONS
# run_migration_test()

# CHANGELOG
# 1.0.0 - 2025-09-26 - Initial implementation for SQLite migration testing

set -euo pipefail

# Test Configuration
TEST_NAME="SQLite Migration"
TEST_ABBR="SQL"
TEST_NUMBER="34"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Migration test parameters (customize for different engines)
ENGINE_NAME="SQLite"
ENGINE_REF="sqlite"
CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_${ENGINE_REF}.json"
LOG_LINE_PATTERN="Migration test completed in"

# Test timeouts
TIMEOUT=30
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to run migration test
run_migration_test() {
    local config_file="$1"
    local engine_name="$2"
    local log_line_pattern="$3"
    local timeout="$4"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${engine_name}.log"
    local result_file="${LOG_PREFIX}_${engine_name}.result"

    # Clear result file
    true > "${result_file}"

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup
    local startup_success=false
    local start_time=${SECONDS}

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

        # Wait for migration completion or timeout
        local migration_complete=false
        local migration_time=""
        local test_start=${SECONDS}

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for migration completion (timeout: ${timeout}s)..."

        while true; do
            if [[ $((SECONDS - test_start)) -ge "${timeout}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Migration test timed out after ${timeout}s"
                break
            fi

            # Check for migration completion line
            if "${GREP}" -q "${log_line_pattern}" "${log_file}" 2>/dev/null; then
                migration_time=$("${GREP}" -o -E "${log_line_pattern} ([0-9.]+)s" "${log_file}" 2>/dev/null | head -1 | cut -d' ' -f5 | "${SED}" 's/s$//')
                if [[ -n "${migration_time}" ]]; then
                    migration_complete=true
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_line_pattern} ${migration_time}s"
                    break
                fi
            fi
            sleep 0.1
        done

        if [[ "${migration_complete}" = true ]]; then
            echo "MIGRATION_COMPLETED" >> "${result_file}"
            echo "MIGRATION_TIME=${migration_time}" >> "${result_file}"
        else
            echo "MIGRATION_TIMEOUT" >> "${result_file}"
        fi

        # Stop the server
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            # Wait for graceful shutdown
            local shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.1
            done
        fi

        echo "TEST_COMPLETE" >> "${result_file}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
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

# Validate configuration file
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_FILE}"; then
    port=$(get_webserver_port "${CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${ENGINE_NAME} migration configuration will use port: ${port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with migration test if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running ${ENGINE_NAME} Migration Test"

    # Run the migration test
    run_migration_test "${CONFIG_FILE}" "${ENGINE_REF}" "${LOG_LINE_PATTERN}" "${TIMEOUT}"

    # Analyze results
    result_file="${LOG_PREFIX}_${ENGINE_REF}.result"
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${ENGINE_REF}.log"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${ENGINE_NAME} Server Log: ..${log_file}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${ENGINE_NAME} Result File: ..${result_file}"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${ENGINE_NAME} migration test"
        EXIT_CODE=1
    else
        # Check startup
        if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${ENGINE_NAME} migration test"
            EXIT_CODE=1
        elif ! "${GREP}" -q "MIGRATION_COMPLETED" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${ENGINE_NAME} migration test did not complete within ${TIMEOUT}s timeout"
            EXIT_CODE=1
        else
            # Extract migration time and update test name
            migration_time=$("${GREP}" "MIGRATION_TIME=" "${result_file}" 2>/dev/null | cut -d'=' -f2)
            if [[ -n "${migration_time}" ]]; then
                TEST_NAME="${TEST_NAME} {BLUE}(cycle: ${migration_time}s){RESET}"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${ENGINE_NAME} migration test completed successfully in ${migration_time}s"
                PASS_COUNT=$(( PASS_COUNT + 1 ))

                # Extract total running time from log
                total_time=$("${GREP}" "Total running time:" "${log_file}" 2>/dev/null | "${SED}" -E 's/.*Total running time:\s*([0-9.]+)s.*/\1/' | head -1)
                if [[ -n "${total_time}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server shutdown complete"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server total running time: ${total_time}s"
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${ENGINE_NAME} migration test completed but time not captured"
                EXIT_CODE=1
            fi
        fi
    fi

else
    # Skip migration test if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping ${ENGINE_NAME} migration test due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"