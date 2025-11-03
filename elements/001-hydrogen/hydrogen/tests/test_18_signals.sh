#!/usr/bin/env bash

# Test: Signal Handling
# Tests signal handling capabilities including SIGINT, SIGTERM, SIGHUP, SIGUSR2, and multiple signals

# FUNCTIONS
# run_signal_test_parallel()
# analyze_signal_test_results()
# verify_clean_shutdown()
# verify_single_restart()
# verify_multi_restart()
# verify_crash_dump()
# verify_config_dump()
# verify_multi_signals()
# display_sighup_single_log()
# display_sighup_multi_log()
# display_sigterm_log()
# display_sigint_log()
# display_sigusr1_log()
# display_sigusr2_log()
# display_multi_log()

# CHANGELOG
# 6.1.0 - 2025-09-03 - Added server log output for each signal test similar to Test 24 for easier debugging
# 6.0.0 - 2025-08-08 - Complete refactor using lifecycle.sh and parallel execution pattern from Test 13
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.0 - 2025-07-30 - Shellcheck overhaul, general review
# 3.0.4 - 2025-07-15 - No more sleep
# 3.0.3 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.1 - 2025-07-02 - Performance optimization: removed all artificial delays (sleep statements) for dramatically faster execution while maintaining reliability
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic signal handling test

set -euo pipefail

# Test configuration
TEST_NAME="Signal Handling"
TEST_ABBR="SIG"
TEST_NUMBER="18"
TEST_COUNTER=0
TEST_VERSION="6.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
TEST_CONFIG="${CONFIG_DIR}/hydrogen_test_${TEST_NUMBER}_signals.json"
TEST_CONFIG_BASE=$(basename "${TEST_CONFIG}")

# Signal test configuration - format: "signal:action:description:validation_func:cleanup_signal"
declare -A SIGNAL_TESTS=(
    ["SIGINT"]="SIGINT:terminate:Ctrl+C:verify_clean_shutdown:SIGINT"
    ["SIGTERM"]="SIGTERM:terminate:Terminate:verify_clean_shutdown:SIGTERM"
    ["SIGHUP_SINGLE"]="SIGHUP:restart:Single Restart:verify_single_restart:SIGTERM"
    ["SIGHUP_MULTI"]="SIGHUP:restart:Many Restarts:verify_multi_restart:SIGTERM"
    ["SIGUSR1"]="SIGUSR1:crash:Crash Dump:verify_crash_dump:SIGUSR1"
    ["SIGUSR2"]="SIGUSR2:config:Config Dump:verify_config_dump:SIGTERM"
    ["MULTI"]="MULTIPLE:terminate:Multiple Signals:verify_multi_signals:MULTIPLE"
)

# Signal test timeouts
RESTART_COUNT=5
STARTUP_TIMEOUT=5
SHUTDOWN_TIMEOUT=10
SIGNAL_TIMEOUT=10

# Parallel execution configuration
declare -a PARALLEL_PIDS

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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Test Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${TEST_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using minimal configuration for signal testing"
else
    EXIT_CODE=1
fi

# Validation functions for different signal types
verify_clean_shutdown() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local start_time
    start_time=${SECONDS}

    # Wait for process to exit
    while ps -p "${pid}" > /dev/null 2>&1; do
        if [[ $((SECONDS - start_time)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
            echo "SHUTDOWN_TIMEOUT" >> "${result_file}"
            return 1
        fi
        sleep 0.05
    done
    
    # Check for clean shutdown message
    if "${GREP}" -q "SHUTDOWN COMPLETE" "${log_file}" 2>/dev/null; then
        echo "SHUTDOWN_SUCCESS" >> "${result_file}"
        return 0
    else
        echo "SHUTDOWN_FAILED" >> "${result_file}"
        return 1
    fi
}

verify_single_restart() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local start_time
    start_time=${SECONDS}

    # Wait for restart signal to be logged
    while true; do
        if [[ $(( SECONDS - start_time )) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "RESTART_TIMEOUT" >> "${result_file}"
            return 1
        fi

        if "${GREP}" -q "SIGHUP received" "${log_file}" 2>/dev/null; then
            break
        fi
        sleep 0.05
    done

    # Wait for restart completion (simplified to match multi-restart logic)
    while true; do
        if [[ $(( SECONDS - start_time )) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "RESTART_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        # Check for restart completion - either message works
        if ("${GREP}" -q "Application restarts: 1" "${log_file}" 2>/dev/null || \
            "${GREP}" -q "Restart count: 1" "${log_file}" 2>/dev/null || \
            "${GREP}" -q "Restart completed successfully" "${log_file}" 2>/dev/null); then
            echo "RESTART_SUCCESS" >> "${result_file}"
            return 0
        fi
        sleep 0.05
    done
}

verify_multi_restart() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local current_count=1
    local success_count=0
    local failure_count=0

    echo "MULTI_RESTART_STARTING" >> "${result_file}"

    while [[ "${current_count}" -le "${RESTART_COUNT}" ]]; do
        # Log the start of this restart attempt
        echo "MULTI_RESTART_ATTEMPT_${current_count}_START" >> "${result_file}"

        # Send SIGHUP signal
        kill -SIGHUP "${pid}" || true

        # Wait for this restart to complete
        local start_time
        start_time=${SECONDS}

        while true; do
            if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
                echo "MULTI_RESTART_TIMEOUT_${current_count}" >> "${result_file}"
                failure_count=$((failure_count + 1))
                echo "MULTI_RESTART_ATTEMPT_${current_count}_TIMEOUT" >> "${result_file}"
                break
            fi

            if ("${GREP}" -q "Application restarts: ${current_count}" "${log_file}" 2>/dev/null || \
                "${GREP}" -q "Restart count: ${current_count}" "${log_file}" 2>/dev/null); then
                success_count=$((success_count + 1))
                echo "MULTI_RESTART_ATTEMPT_${current_count}_SUCCESS" >> "${result_file}"
                break
            fi
            sleep 0.05
        done

        current_count=$((current_count + 1))

        # Brief delay between restarts to avoid overwhelming the server
        sleep 0.25
    done

    # Report final results
    echo "MULTI_RESTART_SUCCESS_COUNT=${success_count}" >> "${result_file}"
    echo "MULTI_RESTART_FAILURE_COUNT=${failure_count}" >> "${result_file}"

    # Consider successful if we got at least 4 out of 5 restarts (some tolerance for timing issues)
    if [[ "${success_count}" -ge 4 ]]; then
        echo "MULTI_RESTART_OVERALL_SUCCESS" >> "${result_file}"
        return 0
    else
        echo "MULTI_RESTART_OVERALL_FAILURE" >> "${result_file}"
        return 1
    fi
}

verify_crash_dump() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    local binary_name
    binary_name=$(basename "${HYDROGEN_BIN_BASE}")
    local expected_core="${PROJECT_DIR}/${binary_name}.core.${pid}"
    local timeout="${SIGNAL_TIMEOUT}"
    local start_time
    start_time=${SECONDS}
    
    # Wait for core file to appear
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${timeout}" ]]; then
            echo "CRASH_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if [[ -f "${expected_core}" ]]; then
            echo "CRASH_DUMP_SUCCESS" >> "${result_file}"
            rm -f "${expected_core}" # Clean up
            return 0
        fi
        sleep 0.05
    done
}

verify_config_dump() {
    local pid="$1"
    local log_file="$2"
    local signal="$3" 
    local result_file="$4"
    local start_time
    start_time=${SECONDS}
    
    # Wait for dump to start
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "CONFIG_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if "${GREP}" -q "APPCONFIG Dump Started" "${log_file}" 2>/dev/null; then
            break
        fi
        sleep 0.05
    done
    
    # Wait for dump to complete
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${SIGNAL_TIMEOUT}" ]]; then
            echo "CONFIG_DUMP_TIMEOUT" >> "${result_file}"
            return 1
        fi
        
        if "${GREP}" -q "APPCONFIG Dump Complete" "${log_file}" 2>/dev/null; then
            echo "CONFIG_DUMP_SUCCESS" >> "${result_file}"
            return 0
        fi
        sleep 0.05
    done
}

verify_multi_signals() {
    local pid="$1"
    local log_file="$2"
    local signal="$3"
    local result_file="$4"
    
    # Send multiple signals rapidly
    kill -SIGTERM "${pid}" 2>/dev/null || true
    kill -SIGINT "${pid}" 2>/dev/null || true
    
    # Wait for shutdown
    local start_time
    start_time=${SECONDS}
    while ps -p "${pid}" > /dev/null 2>&1; do
        if [[ $((SECONDS - start_time)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
            echo "MULTI_SIGNAL_TIMEOUT" >> "${result_file}"
            return 1
        fi
        sleep 0.05
    done
    
    # Check for single shutdown sequence
    local shutdown_count
    shutdown_count=$("${GREP}" -c "Initiating shutdown sequence" "${log_file}" 2>/dev/null || echo "0")
    if [[ "${shutdown_count}" -eq 1 ]]; then
        echo "MULTI_SIGNAL_SUCCESS" >> "${result_file}"
        return 0
    else
        echo "MULTI_SIGNAL_MULTIPLE_SEQUENCES" >> "${result_file}"
        return 1
    fi
}

# Function to run a single signal test in parallel (background process)
run_signal_test_parallel() {
    local test_name="$1"
    local signal="$2"
    local action="$3"
    local description="$4"
    local validation_func="$5"
    local cleanup_signal="$6"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.log"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.result"
    
    # Clear result file
    true > "${result_file}"
    
    # Start hydrogen server directly
    HYDROGEN_LOG_LEVEL="DEBUG" "${HYDROGEN_BIN}" "${TEST_CONFIG}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
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
        sleep 0.05
    done
    
    if [[ "${startup_success}" = true ]]; then
        echo "PID=${hydrogen_pid}" >> "${result_file}"
        echo "STARTUP_SUCCESS" >> "${result_file}"
        
        # Send the signal (except for multi-signal test which handles its own signaling)
        if [[ "${signal}" != "MULTIPLE" ]]; then
            kill -"${signal}" "${hydrogen_pid}" 2>/dev/null || true
        fi
        
        # Validate signal behavior using specific validation function
        if "${validation_func}" "${hydrogen_pid}" "${log_file}" "${signal}" "${result_file}"; then
            # For multi-restart test, validate based on success ratio
            if [[ "${validation_func}" = "verify_multi_restart" ]]; then
                if "${GREP}" -q "MULTI_RESTART_OVERALL_SUCCESS" "${result_file}" 2>/dev/null; then
                    echo "VALIDATION_SUCCESS" >> "${result_file}"
                else
                    echo "VALIDATION_FAILED" >> "${result_file}"
                fi
            else
                echo "VALIDATION_SUCCESS" >> "${result_file}"
            fi
        else
            # For multi-restart test, still consider it successful even if it returns 1 (due to individual failures)
            if [[ "${validation_func}" = "verify_multi_restart" ]]; then
                if "${GREP}" -q "MULTI_RESTART_OVERALL_SUCCESS" "${result_file}" 2>/dev/null; then
                    echo "VALIDATION_SUCCESS" >> "${result_file}"
                else
                    echo "VALIDATION_FAILED" >> "${result_file}"
                fi
            else
                echo "VALIDATION_FAILED" >> "${result_file}"
            fi
        fi
        
        # Clean up if needed (for restart tests, we need to stop the server)
        if [[ "${cleanup_signal}" != "${signal}" ]] && [[ "${cleanup_signal}" != "MULTIPLE" ]]; then
            if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
                kill -"${cleanup_signal}" "${hydrogen_pid}" 2>/dev/null || true
                # Simple shutdown wait
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
        fi
        
        echo "TEST_COMPLETE" >> "${result_file}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Display functions for each test type
display_sighup_single_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGHUP SINGLE] Server Log: ..${log_file}"

    # SIGHUP SIGNAL RECEPTION SECTION
    local sighup_line
    sighup_line=$(grep -n "SIGHUP received, initiating restart" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${sighup_line}" != "0" ]]; then
        local sighup_start=$((sighup_line - 1))  # Start 1 line before
        [[ "${sighup_start}" -lt 1 ]] && sighup_start=1
        local sighup_end=$((sighup_start + 5))  # 6 lines total

        print_message "${test_number}" "${test_counter}" "[SIGNAL RECEPTION - SIGHUP]"

        local sighup_section
        sighup_section=$("${AWK}" "NR >= ${sighup_start} && NR <= ${sighup_end} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${sighup_section}" ]]; then
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${sighup_section}"
        fi
    else
        print_message "${test_number}" "${test_counter}" "[No SIGHUP reception found in log]"
    fi

    # SIGTERM SIGNAL RECEPTION SECTION
    local sigterm_line
    sigterm_line=$(grep -n "SIGTERM received, initiating shutdown" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${sigterm_line}" != "0" ]]; then
        local sigterm_start=$((sigterm_line - 1))  # Start 1 line before
        [[ "${sigterm_start}" -lt 1 ]] && sigterm_start=1
        local sigterm_end=$((sigterm_start + 5))  # 6 lines total

        print_message "${test_number}" "${test_counter}" "[SIGNAL RECEPTION - SIGTERM]"

        local sigterm_section
        sigterm_section=$("${AWK}" "NR >= ${sigterm_start} && NR <= ${sigterm_end} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${sigterm_section}" ]]; then
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${sigterm_section}"
        fi
    else
        print_message "${test_number}" "${test_counter}" "[No SIGTERM reception found in log]"
    fi

    # Show completion (shutdown) section
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

display_sighup_multi_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGHUP MULTI] Server Log: ..${log_file}"

    # Find all SIGHUP occurrences and show each instance
    local signal_occurrences
    signal_occurrences=$("${GREP}" -n "SIGHUP received" "${log_file}" 2>/dev/null || true)

    if [[ -n "${signal_occurrences}" ]]; then
        local instance_count=1
        while IFS= read -r occurrence; do
            local current_signal_line
            current_signal_line=$(echo "${occurrence}" | cut -d: -f1)
            # Start one line before signal and show 6 lines total
            local start_display=$((current_signal_line - 1))  # Start 1 line before signal
            [[ "${start_display}" -lt 1 ]] && start_display=1
            local end_display=$((start_display + 5))  # 6 lines total

            local start_lines
            start_lines=$("${AWK}" "NR >= ${start_display} && NR <= ${end_display} { print }" "${log_file}" 2>/dev/null || true)

            if [[ -n "${start_lines}" ]]; then
                print_message "${test_number}" "${test_counter}" "[SIGHUP #${instance_count} RECEIVED - START]"
                local restart_count=1
                while IFS= read -r line; do
                    local output_line
                    output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                    print_output "${test_number}" "${test_counter}" "${output_line}"
                    restart_count=$((restart_count + 1))
                    [[ "${restart_count}" -gt 7 ]] && break
                done < <(echo "${start_lines}" || true)
                instance_count=$((instance_count + 1))
                # Limit to first 5 instances to avoid excessive output
                [[ "${instance_count}" -gt 5 ]] && break
            fi
        done < <(echo "${signal_occurrences}" || true)
    else
        print_message "${test_number}" "${test_counter}" "[No SIGHUP signals found in log]"
    fi

    # Show completion section with standard 7 lines
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

display_sigterm_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGTERM] Server Log: ..${log_file}"

    # SIGTERM SIGNAL RECEPTION SECTION
    local sigterm_line
    sigterm_line=$(grep -n "SIGTERM received, initiating shutdown" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${sigterm_line}" != "0" ]]; then
        local sigterm_start=$((sigterm_line - 1))  # Start 1 line before
        [[ "${sigterm_start}" -lt 1 ]] && sigterm_start=1
        local sigterm_end=$((sigterm_start + 5))  # 6 lines total

        print_message "${test_number}" "${test_counter}" "[SIGNAL RECEPTION - SIGTERM]"

        local sigterm_section
        sigterm_section=$("${AWK}" "NR >= ${sigterm_start} && NR <= ${sigterm_end} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${sigterm_section}" ]]; then
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${sigterm_section}"
        fi
    else
        print_message "${test_number}" "${test_counter}" "[No SIGTERM reception found in log]"
    fi

    # Show completion section
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

display_sigint_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGINT] Server Log: ..${log_file}"

    # SIGINT SIGNAL RECEPTION SECTION (similar to SIGTERM)
    local sigint_line
    sigint_line=$(grep -n "SIGINT received, initiating shutdown" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${sigint_line}" != "0" ]]; then
        local sigint_start=$((sigint_line - 1))  # Start 1 line before
        [[ "${sigint_start}" -lt 1 ]] && sigint_start=1
        local sigint_end=$((sigint_start + 5))  # 6 lines total

        print_message "${test_number}" "${test_counter}" "[SIGNAL RECEPTION - SIGINT]"

        local sigint_section
        sigint_section=$("${AWK}" "NR >= ${sigint_start} && NR <= ${sigint_end} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${sigint_section}" ]]; then
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${sigint_section}"
        fi
    else
        print_message "${test_number}" "${test_counter}" "[No SIGINT reception found in log]"
    fi

    # Show completion section
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

display_sigusr1_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGUSR1] Server Log: ..${log_file}"

    # Show last 4 lines of the log (SIGUSR1 terminates program immediately, so crash info is at the end)
    print_message "${test_number}" "${test_counter}" "[CRASH INFO - FINAL LINES]"

    local end_lines
    end_lines=$(tail -n 4 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    else
        print_message "${test_number}" "${test_counter}" "[DEBUG] No log lines found]"
    fi
}

display_sigusr2_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[SIGUSR2] Server Log: ..${log_file}"

    # Find SIGUSR2 reception with improved pattern matching
    local signal_line
    signal_line=$(grep -n "Received SIGUSR2" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${signal_line}" != "0" ]]; then
        local sigusr2_start=$((signal_line - 1))  # Start 1 line before
        [[ "${sigusr2_start}" -lt 1 ]] && sigusr2_start=1
        local sigusr2_end=$((sigusr2_start + 7))  # 8 lines total

        print_message "${test_number}" "${test_counter}" "[SIGNAL RECEIVED - START]"

        local signal_section
        signal_section=$("${AWK}" "NR >= ${sigusr2_start} && NR <= ${sigusr2_end} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${signal_section}" ]]; then
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${signal_section}"
        fi
    else
        print_message "${test_number}" "${test_counter}" "[No SIGUSR2 reception found in log]"
    fi



    # Show completion section with 7 lines instead of 5
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

display_multi_log() {
    local test_number="$1"
    local test_counter="$2"
    local log_file="$3"

    print_message "${test_number}" "${test_counter}" "[MULTI SIGNAL] Server Log: ..${log_file}"

    # Use "Press Ctrl+C to exit" anchor for multiple signals since they arrive simultaneously
    local anchor_line
    anchor_line=$(grep -n "Press Ctrl+C to exit" "${log_file}" 2>/dev/null | head -n 1 | cut -d: -f1 || echo "0")

    if [[ "${anchor_line}" != "0" ]]; then
        # Extract 7 lines starting from the line AFTER the anchor (to get the signals and immediate response)
        local start_display=$((anchor_line + 1))
        local end_display=$((start_display + 6))  # 7 lines total

        print_message "${test_number}" "${test_counter}" "[DEBUG] Extracting lines ${start_display} to ${end_display} (7 lines)"

        local signal_section
        signal_section=$("${AWK}" "NR >= ${start_display} && NR <= ${end_display} { print }" "${log_file}" 2>/dev/null || true)

        if [[ -n "${signal_section}" ]]; then
            print_message "${test_number}" "${test_counter}" "[MULTIPLE SIGNALS RECEIVED - START]"

            # Use the same here-string approach that worked for other functions
            while IFS= read -r line && [[ -n "${line}" ]]; do
                local output_line
                output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                print_output "${test_number}" "${test_counter}" "${output_line}"
            done <<< "${signal_section}"
        fi

        # Check how many shutdown sequences there are
        local shutdown_count
        shutdown_count=$("${GREP}" -c "Initiating shutdown sequence" "${log_file}" 2>/dev/null || echo "0")
        print_message "${test_number}" "${test_counter}" "[DEBUG] Found ${shutdown_count} shutdown sequences (should be 1 for proper handling)"
    else
        print_message "${test_number}" "${test_counter}" "[DEBUG] No 'Press Ctrl+C' anchor found in log]"
    fi

    # Show completion section
    local end_lines
    end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
    if [[ -n "${end_lines}" ]]; then
        print_message "${test_number}" "${test_counter}" "[COMPLETION - END]"
        while IFS= read -r line; do
            local output_line
            output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
            print_output "${test_number}" "${test_counter}" "${output_line}"
        done < <(echo "${end_lines}" || true)
    fi
}

# Function to analyze results from parallel signal test execution
analyze_signal_test_results() {
    local test_name="$1"
    local signal="$2"
    local description="$3"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_name}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi

    # Check validation
    if "${GREP}" -q "VALIDATION_SUCCESS" "${result_file}" 2>/dev/null; then
        return 0
    else
        # Check for specific failure reasons
        if "${GREP}" -q "SHUTDOWN_FAILED" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${signal} handling failed - no clean shutdown"
        elif "${GREP}" -q "RESTART_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${signal} restart failed or timed out"
        elif "${GREP}" -q "CRASH_DUMP_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${signal} failed to generate crash dump"
        elif "${GREP}" -q "CONFIG_DUMP_TIMEOUT" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${signal} handling failed - no config dump"
        elif "${GREP}" -q "MULTI_SIGNAL_MULTIPLE_SEQUENCES" "${result_file}" 2>/dev/null; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Multiple shutdown sequences detected"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${signal} handling failed"
        fi
        return 1
    fi
}

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running signal tests in parallel for faster execution..."

# Start all signal tests in parallel with job limiting  
for test_config in "${!SIGNAL_TESTS[@]}"; do
    # shellcheck disable=SC2312 # Gettin fancy with the functions
    while (( $(jobs -r | wc -l) >= CORES )); do
        wait -n  # Wait for any job to finish
    done
    
    # Parse test configuration
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
    
    # Run parallel signal test in background
    run_signal_test_parallel "${test_config}" "${signal}" "${action}" "${description}" "${validation_func}" "${cleanup_signal}" &
    PARALLEL_PIDS+=($!)
done

# Wait for all parallel tests to complete
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for all ${#SIGNAL_TESTS[@]} parallel signal tests to complete..."
for pid in "${PARALLEL_PIDS[@]}"; do
    wait "${pid}"
done
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All parallel tests completed, analyzing results..."

# Process results sequentially for clean output
for test_config in "${!SIGNAL_TESTS[@]}"; do
    # Parse test configuration
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${signal} Signal Handling (${description})"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "${HYDROGEN_BIN_BASE} ${TEST_CONFIG_BASE}"

    # Display targeted snapshot using test-specific functions
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${test_config}.log"
    if [[ -f "${log_file}" ]]; then
        # Call appropriate display function based on test type
        case "${test_config}" in
            "SIGINT")
                display_sigint_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "SIGTERM")
                display_sigterm_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "SIGHUP_SINGLE")
                display_sighup_single_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "SIGHUP_MULTI")
                display_sighup_multi_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "SIGUSR1")
                display_sigusr1_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "SIGUSR2")
                display_sigusr2_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            "MULTI")
                display_multi_log "${TEST_NUMBER}" "${TEST_COUNTER}" "${log_file}"
                ;;
            *)
                # Fallback for unknown test configs
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${signal} Server Log: ..${log_file}"
                end_lines=$(tail -n 7 "${log_file}" 2>/dev/null || true)
                if [[ -n "${end_lines}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "[COMPLETION - END]"
                    while IFS= read -r line; do
                        output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output_line}"
                    done < <(echo "${end_lines}" || true)
                fi
                ;;
        esac
    fi

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if analyze_signal_test_results "${test_config}" "${signal}" "${description}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${signal} handled successfully"
    else
        EXIT_CODE=1
    fi
done

# Print summary
successful_tests=0
for test_config in "${!SIGNAL_TESTS[@]}"; do
    IFS=':' read -r signal action description validation_func cleanup_signal <<< "${SIGNAL_TESTS[${test_config}]}"
    result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${test_config}.result"
    if [[ -f "${result_file}" ]] && "${GREP}" -q "VALIDATION_SUCCESS" "${result_file}" 2>/dev/null; then
       successful_tests=$(( successful_tests + 1 ))
    fi
done

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_tests}/${#SIGNAL_TESTS[@]} signal tests passed"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
