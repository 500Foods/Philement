#!/usr/bin/env bash

# Conduit Server Lifecycle
# Startup, shutdown, and result analysis for conduit blackbox tests.
# Sourced by tests/lib/conduit_utils.sh so existing callers keep one entry point.
# Split out so conduit_utils.sh stays under the 1000-line cap.

# LIBRARY FUNCTIONS
# hydrogen_process_crashed()
# conduit_fail_startup()
# run_conduit_server()
# shutdown_conduit_server()
# analyze_conduit_results()

# CHANGELOG
# 1.0.0 - 2026-10-30 - Moved server lifecycle out of conduit_utils.sh 1.8.1
#                    - Same functions: hydrogen_process_crashed, conduit_fail_startup,
#                      run_conduit_server, shutdown_conduit_server, analyze_conduit_results

# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, GREP, LOGS_DIR, TIMESTAMP, HYDROGEN_BIN, LOG_PREFIX, and DATABASE_NAMES come from the caller
# shellcheck disable=SC2034 # EXIT_CODE is read by the calling test script

# The parent library sets this before sourcing. Repeat it here so shellcheck
# sees pipefail on the pipelines below when this file is checked on its own.
set -euo pipefail

[[ -n "${CONDUIT_SERVER_GUARD:-}" ]] && return 0
export CONDUIT_SERVER_GUARD="true"

# True if hydrogen pid is gone, log shows SEGV, or a port-bind failure was logged.
# Used by run_conduit_server to fail fast instead of waiting full readiness timeouts.
hydrogen_process_crashed() {
    local pid="$1"
    local log="$2"
    if ! kill -0 "${pid}" 2>/dev/null; then
        return 0
    fi
    if "${GREP}" -q "Signal 11 received" "${log}" 2>/dev/null; then
        return 0
    fi
    if "${GREP}" -qE "Port [0-9]+ is already in use|Port [0-9]+ is not available|failed to bind to port" "${log}" 2>/dev/null; then
        return 0
    fi
    return 1
}

# Score a run_conduit_server failure. Call this from the test shell, after the
# command substitution returns. A failure opens one subtest and one FAIL, and
# sets EXIT_CODE. Success leaves the subtest counts unchanged.
# Returns 1 when startup failed, 0 when server_info is a live server.
# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, and GREP come from the test shell
conduit_fail_startup() {
    local server_info="$1"
    local result_file="$2"
    local label="${3:-Start conduit server}"

    # Command substitution keeps every stdout line. The token is the last one.
    local last_line="${server_info##*$'\n'}"
    if [[ "${last_line}" != "FAILED:0" ]]; then
        return 0
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}"

    local detail="Server startup failed"
    if [[ -f "${result_file}" ]]; then
        if "${GREP}" -q "PORT_IN_USE=true" "${result_file}" 2>/dev/null; then
            detail="Server startup failed: port in use"
        elif "${GREP}" -q "CRASH_DETECTED=true" "${result_file}" 2>/dev/null; then
            detail="Server crashed (SEGV) during startup"
        elif "${GREP}" -q "PROCESS_EXITED=true" "${result_file}" 2>/dev/null; then
            detail="Server process exited during startup"
        elif "${GREP}" -q "STARTUP_TIMEOUT=true" "${result_file}" 2>/dev/null; then
            detail="Server did not finish startup"
        elif "${GREP}" -q "READINESS_TIMEOUT=true" "${result_file}" 2>/dev/null; then
            detail="Server did not become ready"
        fi
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${detail}"
    # shellcheck disable=SC2034 # EXIT_CODE is read by the calling test script
    EXIT_CODE=1
    return 1
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
    disown "${hydrogen_pid}" 2>/dev/null || true
    if declare -f register_hydrogen_pid >/dev/null 2>&1; then
        register_hydrogen_pid "${hydrogen_pid}"
    fi

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup
    local startup_success=false
    local startup_timed_out=false
    local process_exited=false
    local start_time
    start_time=${SECONDS}

    # Startup timeout: 15s is often enough for a fast regular binary with 1 DB.
    # For multi-DB configs (e.g. test_41 with 7 engines + migrations + Slow queues)
    # or when launching an ASAN-instrumented debug build (hydrogen_debug), startup
    # can legitimately take 30-120s. Use a generous default here; the migration
    # wait below is already 300s. Callers that need even more can set
    # CONDUIT_STARTUP_TIMEOUT before calling.
    local startup_timeout=120
    if [[ -n "${CONDUIT_STARTUP_TIMEOUT:-}" ]]; then
        startup_timeout="${CONDUIT_STARTUP_TIMEOUT}"
    fi

    while true; do
        if [[ $((SECONDS - start_time)) -ge ${startup_timeout} ]]; then
            startup_timed_out=true
            break
        fi

        # shellcheck disable=SC2310 # Poll crash/alive status; non-zero means still running
        if hydrogen_process_crashed "${hydrogen_pid}" "${log_file}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen exited or crashed during startup (see log for Signal 11 / port bind)"
            if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
                process_exited=true
            fi
            break
        fi

        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.1
    done

    # shellcheck disable=SC2310 # Poll crash/alive status after STARTUP COMPLETE race
    if [[ "${startup_success}" = true ]] && hydrogen_process_crashed "${hydrogen_pid}" "${log_file}"; then
        # STARTUP COMPLETE can race with a worker-thread SEGV; do not wait 300s+
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server logged STARTUP COMPLETE but process crashed (SEGV/port) — aborting"
        startup_success=false
        if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
            process_exited=true
        fi
    fi

    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"

        # Wait for the server to become Ready For Requests. The Hydrogen server emits a
        # single canonical "READY FOR REQUESTS" log line only once EVERY enabled database's
        # Lead DQM has completed its full conductor sequence (connect -> bootstrap ->
        # migration -> additional queues). Keying off this one signal replaces the previous
        # brittle approach (hardcoded expected_databases=7 + per-engine migration-string
        # greps + a fixed sleep), and works for any number of configured databases.
        local migration_start_time=${SECONDS}
        local migration_timeout=300
        if [[ -n "${CONDUIT_MIGRATION_TIMEOUT:-}" ]]; then
            migration_timeout="${CONDUIT_MIGRATION_TIMEOUT}"
        fi
        local ready_signalled=false
        local crash_during_ready=false

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for server to be Ready For Requests (all database migrations complete)..."
        while true; do
            if [[ $((SECONDS - migration_start_time)) -ge ${migration_timeout} ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Readiness wait timeout after ${migration_timeout}s - proceeding with conduit tests"
                break
            fi

            # shellcheck disable=SC2310 # Poll crash/alive status; non-zero means still running
            if hydrogen_process_crashed "${hydrogen_pid}" "${log_file}"; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen crashed before READY FOR REQUESTS (SEGV or port failure)"
                crash_during_ready=true
                break
            fi

            if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
                ready_signalled=true
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server signalled READY FOR REQUESTS - proceeding with conduit tests"
                break
            fi

            sleep 0.1
        done

        if [[ "${crash_during_ready}" = true ]]; then
            echo "STARTUP_FAILED" >> "${result_file}"
            if "${GREP}" -q "Signal 11 received" "${log_file}" 2>/dev/null; then
                echo "CRASH_DETECTED=true" >> "${result_file}"
            fi
            if "${GREP}" -qE "Port [0-9]+ is already in use|Port [0-9]+ is not available|failed to bind to port" "${log_file}" 2>/dev/null; then
                echo "PORT_IN_USE=true" >> "${result_file}"
            fi
            if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
                echo "PROCESS_EXITED=true" >> "${result_file}"
            fi
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
            if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
                unregister_hydrogen_pid "${hydrogen_pid}"
            fi
            echo "FAILED:0"
            return 1
        fi

        # Confirm the webserver is serving requests by polling the authoritative readiness
        # endpoint. It returns HTTP 200 only once the server is Ready For Requests, and 503
        # while still starting. Support override for slow starts (ASAN debug builds + multi-DB).
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Confirming webserver readiness via /api/system/readiness..."
        local server_ready=false
        local server_ready_start=${SECONDS}
        local webserver_ready_timeout=120
        if [[ -n "${CONDUIT_WEBSERVER_READY_TIMEOUT:-}" ]]; then
            webserver_ready_timeout="${CONDUIT_WEBSERVER_READY_TIMEOUT}"
        fi
        while true; do
            if [[ $((SECONDS - server_ready_start)) -ge ${webserver_ready_timeout} ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Webserver readiness timeout"
                echo "READINESS_TIMEOUT=true" >> "${result_file}"
                break
            fi
            # shellcheck disable=SC2310 # Poll crash/alive status; non-zero means still running
            if hydrogen_process_crashed "${hydrogen_pid}" "${log_file}"; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen crashed while waiting for readiness endpoint"
                if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
                    echo "PROCESS_EXITED=true" >> "${result_file}"
                fi
                break
            fi
            local http_code
            http_code=$(curl -s -w "%{http_code}" -o /dev/null --max-time 5 "${base_url}/api/system/readiness" 2>/dev/null)
            if [[ "${http_code}" == "200" ]]; then
                server_ready=true
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Webserver is ready for HTTP requests (readiness endpoint returned 200)"
                break
            fi
            sleep 0.1
        done

        if [[ "${server_ready}" = false ]]; then
            if [[ "${ready_signalled}" = true ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "READY FOR REQUESTS was logged but readiness endpoint did not return 200"
            fi
            if "${GREP}" -q "Signal 11 received" "${log_file}" 2>/dev/null; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "SEGV detected in server log — preserve core dump if present"
                echo "CRASH_DETECTED=true" >> "${result_file}"
            fi
            if "${GREP}" -qE "Port [0-9]+ is already in use|failed to bind to port" "${log_file}" 2>/dev/null; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port bind failure detected (another process likely holds the port)"
                echo "PORT_IN_USE=true" >> "${result_file}"
            fi
            echo "STARTUP_FAILED" >> "${result_file}"
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
            if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
                unregister_hydrogen_pid "${hydrogen_pid}"
            fi
            echo "FAILED:0"
            return 1
        fi

        # Record per-database readiness only after HTTP readiness is confirmed. The
        # conduit status endpoint excludes databases whose migration/query setup failed.
        check_database_readiness "${base_url}" "${result_file}"

        # Return server info for caller to use
        echo "${base_url}:${hydrogen_pid}"
    else
        if "${GREP}" -q "Signal 11 received" "${log_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "SEGV during startup — check core dump and server log"
            echo "CRASH_DETECTED=true" >> "${result_file}"
        fi
        if "${GREP}" -qE "Port [0-9]+ is already in use|Port [0-9]+ is not available|failed to bind to port" "${log_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Port unavailable — refuse startup instead of hanging"
            echo "PORT_IN_USE=true" >> "${result_file}"
        fi
        if [[ "${process_exited}" = true ]]; then
            echo "PROCESS_EXITED=true" >> "${result_file}"
        fi
        if [[ "${startup_timed_out}" = true ]]; then
            echo "STARTUP_TIMEOUT=true" >> "${result_file}"
        fi
        echo "STARTUP_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
            unregister_hydrogen_pid "${hydrogen_pid}"
        fi
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
    if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
        unregister_hydrogen_pid "${hydrogen_pid}"
    fi
}

# Function to analyze conduit test results
analyze_conduit_results() {
    local log_suffix="$1"
    local description="$2"         
    # shellcheck disable=SC2154 # LOG_PREFIX defined in framework.sh
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Result File"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for conduit test"
        return 1
    fi

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Startup"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Failed to start Hydrogen"
        return 1
    fi

    # Check if conduit tests completed
    if ! "${GREP}" -q "CONDUIT_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Completion"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Conduit tests did not complete"
        return 1
    fi

    local total_passed=0
    local total_tests=0

    # Check single public query tests across all databases
    if "${GREP}" -q "^SINGLE_PUBLIC_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Single Public Query Tests"
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
    if "${GREP}" -q "^SINGLE_INVALID_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Single Invalid Query Tests"
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
    if "${GREP}" -q "^MULTIPLE_QUERIES_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Multiple Queries Tests"
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
    if "${GREP}" -q "^QUERIES_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Comprehensive Queries Tests"
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
    if "${GREP}" -q "^AUTH_QUERY_COMPREHENSIVE_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Query Comprehensive Tests"
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
    if "${GREP}" -q "^AUTH_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Multiple Query Tests"
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
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Auth Multiple Query Tests"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Auth Multiple Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Check alt single query tests across all databases
    if "${GREP}" -q "^ALT_SINGLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Single Query Tests"
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
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Single Query Tests"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Alt Single Query Tests skipped (no JWT token)"
        total_passed=$(( total_passed + 1 ))
        total_tests=$(( total_tests + 1 ))
    fi

    # Check alt multiple query tests across all databases
    if "${GREP}" -q "^ALT_MULTIPLE_QUERY_TESTS_PASSED=" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Multiple Query Tests"
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
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Alt Multiple Query Tests"
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
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Overall Conduit Test Results"
    if [[ "${total_passed}" -eq "${total_tests}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: All Conduit Tests Passed (${total_passed}/${total_tests})"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Some Conduit Tests Failed (${total_passed}/${total_tests})"
        return 1
    fi
}
