#!/usr/bin/env bash

# Scripting Subsystem Blackbox Test Helpers
# Shared functions for tests/test_43_scripting.sh.
#
# Phase 11h of LUA_PLAN: first blackbox test for the Scripting subsystem.
# Phase 11i: expanded to run all 7 database engines in parallel (like
# test_40_auth.sh) with a "with DefaultDatabase" and a "without
# DefaultDatabase" variant per engine. These helpers cover the lifecycle
# of a single Hydrogen instance started with a scripting-enabled config,
# plus the log-content assertions that prove the Orchestrator actually
# started and shut down.
#
# Fail-fast: test_43 assumes the scripting migrations (1201+, the
# `scripts` table, and the Orchestrators.Orchestrator row) are already
# present in each engine's fixture. If the scripting subsystem cannot
# start the Orchestrator because those are missing, the failure is
# captured in the log (e.g. "no script row" / "continuing without one")
# and the run ends quickly instead of waiting out the full startup
# timeout.
#
# Local variable prefix in callers must be lowercase (subtest_*, not
# SCRIPTING_*) to avoid the test_03 env-var scanner picking them up.

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, HYDROGEN_BIN, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code; helpers either fall back gracefully or || true the outer call

# CHANGELOG
# 2.0.0 - 2026-07-02 - Phase 11i: added scripting_run_engine_parallel and
#                      scripting_wait_for_ready_or_fail for parallel 7-engine
#                      execution with fail-fast log tracking.
# 1.0.0 - 2026-07-01 - Initial creation for LUA_PLAN Phase 11h (first scripting blackbox test).

# Guard clause to prevent multiple sourcing
[[ -n "${SCRIPTING_HELPERS_GUARD:-}" ]] && return 0
export SCRIPTING_HELPERS_GUARD="true"

SCRIPTING_HELPERS_NAME="Scripting Test Helpers"
SCRIPTING_HELPERS_VERSION="2.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${SCRIPTING_HELPERS_NAME} ${SCRIPTING_HELPERS_VERSION}" "info"

# Log markers that mean "the Orchestrator will not start" (missing
# scripting migrations, missing scripts table, disabled row, or no DB).
# When any of these appear after READY, there is no point waiting for
# ticks — the run has failed fast.
SCRIPTING_FAIL_MARKERS=(
    "continuing without one"
    "no script row for"
    "failed to compile"
    "failed to parse query result JSON"
)

# Start a Hydrogen instance with the given config. Writes the PID to the
# named variable. Returns 0 on successful launch, 1 if the process did
# not stay alive for at least 0.5s (i.e. crashed immediately).
#
# Usage: scripting_start_instance <config_file> <log_file> <hydrogen_bin> <pid_var>
scripting_start_instance() {
    local config_file="$1"
    local log_file="$2"
    local hydrogen_bin="$3"
    local pid_var="$4"
    local hydrogen_pid

    eval "${pid_var}=''"

    true > "${log_file}"

    if [[ ! -f "${hydrogen_bin}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen binary not found: ${hydrogen_bin}"
        return 1
    fi
    if [[ ! -f "${config_file}" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Config not found: ${config_file}"
        return 1
    fi

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${hydrogen_bin}") $(basename "${config_file}")"

    "${hydrogen_bin}" "${config_file}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true
    if declare -f register_hydrogen_pid >/dev/null 2>&1; then
        register_hydrogen_pid "${hydrogen_pid}"
    fi

    # Brief settle so an immediate crash shows up before we return
    sleep 0.5

    if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen exited immediately (PID ${hydrogen_pid})"
        if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
            unregister_hydrogen_pid "${hydrogen_pid}"
        fi
        return 1
    fi

    eval "${pid_var}='${hydrogen_pid}'"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen started with PID ${hydrogen_pid}"
    return 0
}

# Wait for the canonical READY FOR REQUESTS signal (database.c / launch.c).
# Returns 0 on signal, 1 on timeout.
#
# Usage: scripting_wait_for_ready <log_file> <timeout_seconds>
scripting_wait_for_ready() {
    local log_file="$1"
    local timeout="$2"
    local deadline
    deadline=$(( $(date +%s) + timeout ))
    while true; do
        if [[ $(date +%s) -ge ${deadline} ]]; then
            return 1
        fi
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            # Brief settle so the Orchestrator's READY-time hook has a moment to fire
            sleep 1
            return 0
        fi
        sleep 0.2
    done
}

# Wait for READY FOR REQUESTS, but bail out early if a scripting failure
# marker appears first (fail-fast). Echoes one of: "ready", "failed",
# "timeout". This lets the parallel runner stop a broken engine quickly
# instead of blocking on the full startup timeout.
#
# Usage: scripting_wait_for_ready_or_fail <log_file> <timeout_seconds>
scripting_wait_for_ready_or_fail() {
    local log_file="$1"
    local timeout="$2"
    local deadline
    deadline=$(( $(date +%s) + timeout ))
    local marker
    while true; do
        if [[ $(date +%s) -ge ${deadline} ]]; then
            echo "timeout"
            return 0
        fi
        # Fail-fast: a scripting failure marker means the Orchestrator
        # will not start (missing migrations / scripts table / row).
        for marker in "${SCRIPTING_FAIL_MARKERS[@]}"; do
            if "${GREP}" -q -F "${marker}" "${log_file}" 2>/dev/null; then
                echo "failed"
                return 0
            fi
        done
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            sleep 1
            echo "ready"
            return 0
        fi
        sleep 0.2
    done
}

# Send SIGTERM and wait for the process to exit. Returns 0 on clean exit
# within timeout, 1 on timeout (process killed with SIGKILL).
#
# Usage: scripting_shutdown_instance <pid> <timeout_seconds>
scripting_shutdown_instance() {
    local pid="$1"
    local timeout="$2"
    local deadline

    kill -TERM "${pid}" 2>/dev/null || true

    deadline=$(( $(date +%s) + timeout ))
    while kill -0 "${pid}" 2>/dev/null; do
        if [[ $(date +%s) -ge ${deadline} ]]; then
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown timeout after ${timeout}s; sending SIGKILL"
            kill -9 "${pid}" 2>/dev/null || true
            return 1
        fi
        sleep 0.2
    done
    return 0
}

# Assert a literal string is present in the log file. Returns 0 on hit.
# Usage: scripting_assert_log_contains <log_file> <needle>
scripting_assert_log_contains() {
    local log_file="$1"
    local needle="$2"
    if "${GREP}" -q -F "${needle}" "${log_file}" 2>/dev/null; then
        return 0
    fi
    return 1
}

# Assert a literal string is absent from the log file. Returns 0 on miss.
# Usage: scripting_assert_log_not_contains <log_file> <needle>
scripting_assert_log_not_contains() {
    local log_file="$1"
    local needle="$2"
    if "${GREP}" -q -F "${needle}" "${log_file}" 2>/dev/null; then
        return 1
    fi
    return 0
}

# Count occurrences of a literal string in the log file. Echoes the count.
# Usage: scripting_count_log_matches <log_file> <needle>
scripting_count_log_matches() {
    local log_file="$1"
    local needle="$2"
    "${GREP}" -c -F "${needle}" "${log_file}" 2>/dev/null | head -1 || echo "0"
}

# Run the full Orchestrator lifecycle for one engine/config in the
# background and write outcome markers to a result file. Designed to be
# launched with & from the test's parallel loop (like test_40_auth.sh's
# run_auth_test_parallel). All output goes to files so parallel runs do
# not interleave console output.
#
# Markers written to result_file (one per line):
#   STARTUP_SUCCESS         - process launched and stayed alive
#   STARTUP_FAILED          - process crashed immediately
#   READY                   - READY FOR REQUESTS observed
#   FAILFAST                - scripting failure marker seen before READY
#   NOT_READY               - startup timeout without READY or failure
#   ORCH_C_STARTED          - C-side "Orchestrator: started <name>"
#   ORCH_LUA_STARTED        - Lua-side "Orchestrator: started"
#   ORCH_TICKS=<n>          - number of "Orchestrator: tick" lines
#   ORCH_NO_FAIL            - no "Orchestrator: failed" marker
#   SHUTDOWN_CLEAN          - process exited within the shutdown timeout
#   ORCH_C_DESTROYED        - C-side "Orchestrator: destroyed"
#   ORCH_LUA_SHUTDOWN       - Lua-side "Orchestrator: shutdown requested"
#   LIFECYCLE_COMPLETE      - full start->tick->clean-stop path succeeded
#
# Usage: scripting_run_engine_parallel <config_file> <log_file> \
#            <result_file> <hydrogen_bin> <startup_timeout> \
#            <shutdown_timeout> <tick_settle_seconds>
scripting_run_engine_parallel() {
    local config_file="$1"
    local log_file="$2"
    local result_file="$3"
    local hydrogen_bin="$4"
    local startup_timeout="$5"
    local shutdown_timeout="$6"
    local tick_settle="$7"

    local hydrogen_pid
    local ready_state

    true > "${result_file}"
    true > "${log_file}"

    if [[ ! -f "${hydrogen_bin}" ]] || [[ ! -f "${config_file}" ]]; then
        echo "STARTUP_FAILED" >> "${result_file}"
        return 0
    fi

    "${hydrogen_bin}" "${config_file}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true
    if declare -f register_hydrogen_pid >/dev/null 2>&1; then
        register_hydrogen_pid "${hydrogen_pid}"
    fi
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    sleep 0.5
    if ! kill -0 "${hydrogen_pid}" 2>/dev/null; then
        echo "STARTUP_FAILED" >> "${result_file}"
        if declare -f unregister_hydrogen_pid >/dev/null 2>&1; then
            unregister_hydrogen_pid "${hydrogen_pid}"
        fi
        return 0
    fi
    echo "STARTUP_SUCCESS" >> "${result_file}"

    ready_state=$(scripting_wait_for_ready_or_fail "${log_file}" "${startup_timeout}")

    case "${ready_state}" in
        ready)
            echo "READY" >> "${result_file}"
            ;;
        failed)
            echo "FAILFAST" >> "${result_file}"
            # Stop quickly; no point exercising the Orchestrator path.
            scripting_shutdown_instance "${hydrogen_pid}" "${shutdown_timeout}" || true
            return 0
            ;;
        *)
            echo "NOT_READY" >> "${result_file}"
            scripting_shutdown_instance "${hydrogen_pid}" "${shutdown_timeout}" || true
            return 0
            ;;
    esac

    # The Orchestrator loads asynchronously at the READY hook, and its
    # DB fetch has its own timeout (a live engine can take several
    # seconds to time out when the scripts table is missing). Poll a
    # bounded window for one of three outcomes: a tick (success signal),
    # a fail marker (missing migrations), or the C-side start line.
    # This keeps a healthy engine fast while giving a live-engine
    # failure enough time to surface as a clean fail-fast.
    local post_ready_deadline
    post_ready_deadline=$(( $(date +%s) + tick_settle + 10 ))
    local marker
    while true; do
        for marker in "${SCRIPTING_FAIL_MARKERS[@]}"; do
            if "${GREP}" -q -F "${marker}" "${log_file}" 2>/dev/null; then
                echo "FAILFAST" >> "${result_file}"
                scripting_shutdown_instance "${hydrogen_pid}" "${shutdown_timeout}" || true
                return 0
            fi
        done
        if "${GREP}" -q -F "Orchestrator: tick" "${log_file}" 2>/dev/null; then
            break
        fi
        if [[ $(date +%s) -ge ${post_ready_deadline} ]]; then
            break
        fi
        sleep 0.3
    done

    if scripting_assert_log_contains "${log_file}" "Orchestrator: started Orchestrators.Orchestrator"; then
        echo "ORCH_C_STARTED" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: started"; then
        echo "ORCH_LUA_STARTED" >> "${result_file}"
    fi

    local tick_count
    tick_count=$(scripting_count_log_matches "${log_file}" "Orchestrator: tick")
    echo "ORCH_TICKS=${tick_count}" >> "${result_file}"

    if scripting_assert_log_not_contains "${log_file}" "Orchestrator: failed"; then
        echo "ORCH_NO_FAIL" >> "${result_file}"
    fi

    # Clean shutdown and post-shutdown assertions.
    if scripting_shutdown_instance "${hydrogen_pid}" "${shutdown_timeout}"; then
        echo "SHUTDOWN_CLEAN" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: destroyed"; then
        echo "ORCH_C_DESTROYED" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: shutdown requested"; then
        echo "ORCH_LUA_SHUTDOWN" >> "${result_file}"
    fi

    # Full lifecycle succeeds only if every stage passed.
    if "${GREP}" -q "ORCH_C_STARTED" "${result_file}" 2>/dev/null && \
       "${GREP}" -q "ORCH_LUA_STARTED" "${result_file}" 2>/dev/null && \
       [[ "${tick_count}" -ge 1 ]] && \
       "${GREP}" -q "ORCH_NO_FAIL" "${result_file}" 2>/dev/null && \
       "${GREP}" -q "SHUTDOWN_CLEAN" "${result_file}" 2>/dev/null && \
       "${GREP}" -q "ORCH_C_DESTROYED" "${result_file}" 2>/dev/null && \
       "${GREP}" -q "ORCH_LUA_SHUTDOWN" "${result_file}" 2>/dev/null; then
        echo "LIFECYCLE_COMPLETE" >> "${result_file}"
    fi
    return 0
}

# Update the Orchestrator row's status in the SQLite scripts table.
# 0 = Disabled, 1 = Enabled (Lookup #062).
# Usage: scripting_set_orchestrator_status <sqlite_db_path> <status_int>
scripting_set_orchestrator_status() {
    local db_path="$1"
    local status="$2"
    sqlite3 "${db_path}" \
        "UPDATE scripts SET status = ${status} WHERE group_name = 'Orchestrators' AND script_name = 'Orchestrator';" \
        2>/dev/null
}
