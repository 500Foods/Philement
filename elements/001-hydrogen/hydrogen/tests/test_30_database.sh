#!/usr/bin/env bash

# Test: Database Subsystem - DQM Startup Verification
# Tests that the Database Queue Manager (DQM) starts correctly
# Specifically checks for the "DQM-{DatabaseName}-00-{TagLetters} Worker thread started" log message
# This verifies that the DQM architecture is working as intended with the new logging format

# FUNCTIONS
# check_database_connectivity()
# run_database_test_parallel()
# analyze_database_test_results()
# test_database_configuration()
# check_dqm_startup()
# count_dqm_launches()
# wait_for_dqm_initialization()

# CHANGELOG
# 1.4.2 - 2025-09-22 - Fixed bash syntax errors in arithmetic comparisons
#                    - Fixed: [[ ${var} -lt ${var} ]] syntax errors at lines 117 and 123
#                    - Added proper variable quoting to prevent expansion issues
#                    - Added DQM_INIT_TIMEOUT variable for configurable timeout management
# 1.4.1 - 2025-09-22 - Fixed DQM initialization timing logic for PostgreSQL and DB2 tests
#                    - Fixed: Test was killing server immediately after "STARTUP COMPLETE" without waiting for DQM initialization
#                    - DQM initialization completion happens AFTER "STARTUP COMPLETE", not before
#                    - Updated test logic to wait for DQM initialization completion after startup is complete
#                    - Clarified log message to indicate DQM launches happen "during startup" not "after startup"
# 1.4.0 - 2025-09-22 - Updated to handle asynchronous Lead DQM startup process
#                    - Added count_dqm_launches() to count "DQM launched successfully" messages before "STARTUP COMPLETE"
#                    - Added wait_for_dqm_initialization() to wait for matching "Lead DQM initialization is complete" messages
#                    - Updated test logic to handle 0-5 asynchronous DQM launches with proper initialization verification
#                    - Added DQM Initialization Check sub-test to verify launch and initialization counts match
#                    - Enhanced result analysis to work with new asynchronous startup behavior
#                    - Fixed: DQM launches occur BEFORE "STARTUP COMPLETE", not after
#                    - Optimized timeouts: STARTUP_TIMEOUT=5s, DQM initialization wait=5s, faster sleep intervals (0.05s)
# 1.3.0 - 2025-09-21 - Added waiting for "Initial connection attempt completed" before shutdown
#                    - Added bootstrap query analysis reporting time and row counts as subtest results
#                    - Enhanced test to wait for full database connection completion
# 1.2.1 - 2025-09-10 - Updated to check for new DQM logging format with queue numbers and tags
#                    - Changed from "DQM-{DatabaseName} Lead queue worker thread started" to
#                      "DQM-{DatabaseName}-00-{TagLetters} Worker thread started" format
# 1.2.0 - 2025-09-08 - Updated to check for DQM Lead queue worker thread startup
#                    - Changed from generic database initialization checks to specific DQM startup verification
#                    - Now looks for "[ DQM-{DatabaseName} ] Lead queue worker thread started" message
# 1.1.0 - 2025-09-05 - Added MULTI configuration test with all four database engines in single config
# 1.0.0 - 2025-09-04 - Initial implementation based on test_22_swagger.sh pattern
#                    - Added parallel execution for all four database engines (PostgreSQL, MySQL, SQLite, DB2)
#                    - Configured separate test configs for each engine with different ports
#                    - Basic connectivity test focusing on successful startup/shutdown

set -euo pipefail

# Test Configuration
TEST_NAME="Databases  {BLUE}engines: 4{RESET}"
TEST_ABBR="DBS"
TEST_NUMBER="30"
TEST_COUNTER=0
TEST_VERSION="1.4.2"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A DATABASE_TEST_CONFIGS

# Database test configuration - format: "config_file:log_suffix:engine_name:description"
DATABASE_TEST_CONFIGS=(
    ["PostGreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_postgres.json:postgres:postgresql:PostgreSQL Engine"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mysql.json:mysql:mysql:MySQL Engine"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json:sqlite:sqlite:SQLite Engine"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_db2.json:db2:db2:DB2 Engine"
    ["Multi"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_multi.json:multi:multi:Multi Engine"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=15
DQM_INIT_TIMEOUT=15

# Function to check for DQM startup log message
check_dqm_startup() {
    local log_file="$1"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for DQM startup message in: ${log_file}"

    # Look for any DQM worker thread startup message with the new format
    # New format: DQM-<Database>-<QueueNumber>-<TagLetters>
    if "${GREP}" -q "DQM-.*-00-.*Worker thread started" "${log_file}" 2>/dev/null; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ DQM Lead queue (00) worker thread started"
        return 0
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ DQM startup message not found"
        return 1
    fi
}

# Function to count DQM launches before startup complete
count_dqm_launches() {
    local log_file="$1"
    local count

    # Count "DQM launched successfully" messages that appear before "STARTUP COMPLETE"
    # The DQM launches happen during startup, before the final "STARTUP COMPLETE" message
    count=$("${GREP}" -B 10000 "STARTUP COMPLETE" "${log_file}" 2>/dev/null | "${GREP}" -c "DQM launched successfully" || echo "0")

    echo "${count}"
}

# Function to wait for DQM initialization completion
wait_for_dqm_initialization() {
    local log_file="$1"
    local expected_count="$2"
    local timeout="${3}"

    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${expected_count} DQM initialization completion message(s)"

    local start_time
    start_time=${SECONDS}
    local current_count=0

    while [[ "${current_count}" -lt "${expected_count}" ]] && [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        # Count "Lead DQM initialization is complete" messages
        current_count=$("${GREP}" -c "Lead DQM initialization is complete" "${log_file}" 2>/dev/null || true)
        # echo "count = $current_count log = $log_file"
        sleep 0.1
    done

    if [[ "${current_count}" -ge "${expected_count}" ]]; then
        #print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✅ All ${expected_count} DQM initialization(s) completed"
        # echo "yay"
        return 0
    else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "❌ Timeout waiting for DQM initialization after ${timeout}s: ${current_count}/${expected_count} completed"
        # echo "nay"
        return 1
    fi
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
        expected_queues=$(( expected_queues + 1 ))
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

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup and check database connectivity
    local startup_success=false
    local database_ready=false
    # local queues_started=0
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
        sleep 0.1
    done

    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"

        # Count DQM launches that happened during startup (before STARTUP COMPLETE)
        local dqm_launch_count
        dqm_launch_count=$(count_dqm_launches "${log_file}")

        if [[ "${dqm_launch_count}" -gt 0 ]]; then
            echo "DQM_LAUNCH_COUNT=${dqm_launch_count}" >> "${result_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${dqm_launch_count} DQM launch(es) during startup"

            # Wait for DQM initialization to complete (happens after STARTUP COMPLETE)
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if wait_for_dqm_initialization "${log_file}" "${dqm_launch_count}" "${DQM_INIT_TIMEOUT}"; then
                echo "DQM_INITIALIZATION_COMPLETED" >> "${result_file}"
                database_ready=true

                # Parse bootstrap query results with DQM context
                local bootstrap_lines
                bootstrap_lines=$(grep -B 1 "Bootstrap query completed" "${log_file}" 2>/dev/null | grep -E "(DQM-.*|Bootstrap query completed)" || echo "")
                if [[ -n "${bootstrap_lines}" ]]; then
                    echo "BOOTSTRAP_QUERIES_FOUND" >> "${result_file}"
                    echo "${bootstrap_lines}" >> "${result_file}"
                else
                    echo "BOOTSTRAP_QUERIES_MISSING" >> "${result_file}"
                fi
            else
                echo "DQM_INITIALIZATION_TIMEOUT" >> "${result_file}"
                database_ready=false
            fi
        else
            echo "NO_DQM_LAUNCHES" >> "${result_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No DQM launches found during startup"
            database_ready=false
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
                sleep 0.1
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

    # Check DQM launch count
    if ! "${GREP}" -q "DQM_LAUNCH_COUNT=" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "DQM launch count not found for ${description} test"
        return 1
    fi

    # Extract DQM launch count
    local dqm_launch_count
    dqm_launch_count=$("${GREP}" "DQM_LAUNCH_COUNT=" "${result_file}" 2>/dev/null | cut -d'=' -f2)

    # Check DQM initialization completed
    if ! "${GREP}" -q "DQM_INITIALIZATION_COMPLETED" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "DQM initialization not completed for ${description} test"
        return 1
    fi

    # Perform DQM Launch Check sub-test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: DQM Launch Check"

    # Check for DQM launch messages in the log
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local dqm_launches
    dqm_launches=$("${GREP}" "DQM launched successfully" "${log_file}" 2>/dev/null | wc -l)

    if [[ "${dqm_launches}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Found ${dqm_launches} DQM launch message(s)"

        # Show individual DQM launches
        local launch_messages
        launch_messages=$("${GREP}" "DQM launched successfully" "${log_file}" 2>/dev/null)
        local launch_count=0
        while IFS= read -r launch_msg; do
            if [[ -n "${launch_msg}" ]]; then
        #        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: ${launch_msg}"
                launch_count=$(( launch_count + 1 ))
            fi
        done <<< "${launch_messages}"

        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: DQM Launch Check (${launch_count} confirmed)"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: No DQM Launches Found"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: DQM Launch Check"
        return 1
    fi

    # Perform DQM Initialization Check sub-test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: DQM Initialization Check"

    # Check for DQM initialization completion messages
    local dqm_initializations
    dqm_initializations=$("${GREP}" -c "Lead DQM initialization is complete" "${log_file}" 2>/dev/null || echo "0")

    if [[ "${dqm_initializations}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Found ${dqm_initializations} DQM initialization completion message(s)"

        # Show individual DQM initializations
        local init_messages
        init_messages=$("${GREP}" "Lead DQM initialization is complete" "${log_file}" 2>/dev/null)
        local init_count=0
        while IFS= read -r init_msg; do
            if [[ -n "${init_msg}" ]]; then
                # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: ${init_msg}"
                init_count=$(( init_count + 1 ))
            fi
        done <<< "${init_messages}"

        # Verify counts match
        if [[ "${dqm_initializations}" -eq "${dqm_launches}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: DQM Initialization Check (${init_count} completed, matches launch count)"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Warning - Initialization count (${dqm_initializations}) doesn't match launch count (${dqm_launches})"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: DQM Initialization Check (count mismatch)"
            return 1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: No DQM Initialization Messages Found"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: DQM Initialization Check"
        return 1
    fi

    # Perform Bootstrap Query Analysis sub-test
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Bootstrap Query Analysis"

    if "${GREP}" -q "BOOTSTRAP_QUERIES_FOUND" "${result_file}" 2>/dev/null; then
        # Parse bootstrap query results from result file
        local bootstrap_data
        bootstrap_data=$("${GREP}" -A 100 "BOOTSTRAP_QUERIES_FOUND" "${result_file}" 2>/dev/null | tail -n +2)

        if [[ -n "${bootstrap_data}" ]]; then
            local total_queries=0
            local total_time=0
            local total_rows=0

            while IFS= read -r line; do
                if [[ "${line}" =~ Bootstrap\ query\ completed\ in\ ([0-9.]+)s:\ returned\ ([0-9]+)\ rows ]]; then
                    local query_time="${BASH_REMATCH[1]}"
                    local query_rows="${BASH_REMATCH[2]}"
                    total_queries=$(( total_queries + 1))
                    total_time=$(echo "${total_time} + ${query_time}" | bc -l 2>/dev/null || echo "${total_time}")
                    total_rows=$((total_rows + query_rows))

                    # Extract database name from the DQM in the same line
                    local db_name=""
                    if echo "${line}" | grep -q "\[ DQM-.*-00-"; then
                        db_name="${line#*\[ DQM-}"
                        db_name="${db_name%%-*}"
                    fi

                    if [[ "${test_name}" == "MULTI" && -n "${db_name}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: ${db_name} - ${query_time}s, ${query_rows} rows"
                    else
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Bootstrap Query ${total_queries} - ${query_time}s, ${query_rows} rows"
                    fi
                fi
            done <<< "${bootstrap_data}"

            if [[ "${total_queries}" -gt 0 ]]; then
                # print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bootstrap Query Analysis (${total_queries} queries, ${total_time}s total, ${total_rows} rows)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bootstrap Query Analysis Complete"
                return 0
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Bootstrap Query Analysis - No valid queries found (non-fatal)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bootstrap Query Analysis - No valid queries found (non-fatal)"
                return 0
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Bootstrap Query Analysis - No data found (non-fatal)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bootstrap Query Analysis - No data found (non-fatal)"
            return 0
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Bootstrap Query Analysis - Queries not found (non-fatal)"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bootstrap Query Analysis - Queries not found (non-fatal)"
        return 0
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

        print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Engine: ..${log_file}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Engine: ..${result_file}"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_database_test_results "${test_config}" "${log_suffix}" "${engine_name}" "${description}"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
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

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}" 
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#DATABASE_TEST_CONFIGS[@]} database engine configurations passed all checks"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - DQM architecture validated across all database engines"

else
    # Skip database tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping database tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
