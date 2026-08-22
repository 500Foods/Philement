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
# 2.6.0 - 2026-08-21 - Record ORCH_SYSTEM_PROBE / ORCH_API_ERROR_PROBE
# 2.5.0 - 2026-08-20 - Drop python3: jq config extract/rewrite, sqlite3 readfile seed
# 2.4.0 - 2026-07-30 - ORCH_MAIL_REPO_PROBE + MAILRELAY_REPO_PROBE_OK (H.mail repo helpers)
# 2.3.0 - 2026-07-29 - Scoreboard/LLM orchestrator probes, mock LLM rewrite for
#                      SQLite fixtures, ORCH_SCOREBOARD_PROBE / ORCH_LLM_PROBE.
# 2.2.0 - 2026-07-28 - Record ORCH_MAIL_PROBE / ORCH_NOTIFY_PROBE markers from
#                      Orchestrator H.mail / H.notify blackbox probes.
# 2.1.0 - 2026-07-23 - Seed Orchestrator Lua from src before SQLite runs so
#                      H.query/H.wait blackbox coverage stays in sync.
# 2.0.0 - 2026-07-02 - Phase 11i: added scripting_run_engine_parallel and
#                      scripting_wait_for_ready_or_fail for parallel 7-engine
#                      execution with fail-fast log tracking.
# 1.0.0 - 2026-07-01 - Initial creation for LUA_PLAN Phase 11h (first scripting blackbox test).

# Guard clause to prevent multiple sourcing
[[ -n "${SCRIPTING_HELPERS_GUARD:-}" ]] && return 0
export SCRIPTING_HELPERS_GUARD="true"

SCRIPTING_HELPERS_NAME="Scripting Test Helpers"
SCRIPTING_HELPERS_VERSION="2.6.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${SCRIPTING_HELPERS_NAME} ${SCRIPTING_HELPERS_VERSION}" "info"

# Optional mock LLM (set by test_43 before parallel runs). Empty = skip rewrite.
SCRIPTING_MOCK_LLM_URL="${SCRIPTING_MOCK_LLM_URL:-}"
SCRIPTING_LLM_PROBE_MODEL="${SCRIPTING_LLM_PROBE_MODEL:-}"

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
    local count
    # Single integer only: grep -c may print "0" with exit 1 (no match); avoid
    # multiline/pipefail glitches that break [[ tick_count -ge 1 ]].
    count=$("${GREP}" -c -F "${needle}" "${log_file}" 2>/dev/null || true)
    count=$(printf '%s' "${count}" | tr -d '\r' | head -n 1)
    if [[ -z "${count}" || ! "${count}" =~ ^[0-9]+$ ]]; then
        count=0
    fi
    printf '%s\n' "${count}"
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
#   ORCH_QUERY_PROBE        - Lua-side "Orchestrator: query_probe ok" (data plane)
#   ORCH_MAIL_PROBE         - Lua-side "Orchestrator: mail_probe ok" (H.mail freeform)
#   ORCH_MAIL_REPO_PROBE    - Lua-side "Orchestrator: mail_repo_probe ok" (H.mail template/route/cleanup/event)
#   MAIL_REPO_PROBE_LAUNCH  - Launch seam "MAILRELAY_REPO_PROBE_OK" (MailRepoProbeOnLaunch)
#   ORCH_NOTIFY_PROBE       - Lua-side "Orchestrator: notify_probe ok" (H.notify deferred)
#   ORCH_HTTP_PROBE         - Lua-side "Orchestrator: http_probe ok" (H.http get/post)
#   ORCH_SCOREBOARD_PROBE   - Lua-side "Orchestrator: scoreboard_probe ok"
#   ORCH_LLM_PROBE          - Lua-side "Orchestrator: llm_probe ok"
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
    local sqlite_db
    local web_port
    local prev_http_probe_base="${HYDROGEN_HTTP_PROBE_BASE-}"
    local prev_llm_probe_model="${HYDROGEN_LLM_PROBE_MODEL-}"

    true > "${result_file}"
    true > "${log_file}"

    if [[ ! -f "${hydrogen_bin}" ]] || [[ ! -f "${config_file}" ]]; then
        echo "STARTUP_FAILED" >> "${result_file}"
        return 0
    fi

    # Extract WebServer.Port + optional SQLite path; seed Orchestrator Lua;
    # export HYDROGEN_HTTP_PROBE_BASE for H.http self-call (inherited at exec).
    web_port=""
    sqlite_db=""
    web_port=$(jq -r '.WebServer.Port // empty' "${config_file}" 2>/dev/null || true)
    sqlite_db=$(jq -r '[.Databases.Connections[]? | select((.Engine // "") | ascii_downcase == "sqlite") | .Database // empty][0] // empty' "${config_file}" 2>/dev/null || true)
    if [[ -n "${web_port}" ]]; then
        export HYDROGEN_HTTP_PROBE_BASE="http://127.0.0.1:${web_port}"
    else
        unset HYDROGEN_HTTP_PROBE_BASE || true
    fi
    if [[ -n "${SCRIPTING_LLM_PROBE_MODEL}" ]]; then
        export HYDROGEN_LLM_PROBE_MODEL="${SCRIPTING_LLM_PROBE_MODEL}"
    elif [[ -n "${prev_llm_probe_model}" ]]; then
        export HYDROGEN_LLM_PROBE_MODEL="${prev_llm_probe_model}"
    else
        unset HYDROGEN_LLM_PROBE_MODEL || true
    fi
    local run_config="${config_file}"
    if [[ -n "${sqlite_db}" ]]; then
        # Resolve relative to project root when needed
        if [[ ! -f "${sqlite_db}" && -n "${PROJECT_DIR:-}" && -f "${PROJECT_DIR}/${sqlite_db}" ]]; then
            sqlite_db="${PROJECT_DIR}/${sqlite_db}"
        fi
        # Disposable copy when mock LLM rewrite is needed so parallel runs and
        # the shared hydrodemo fixture are not mutated.
        if [[ -n "${SCRIPTING_MOCK_LLM_URL}" && -f "${sqlite_db}" ]]; then
            local temp_db="${log_file%.log}_orch.sqlite"
            local temp_cfg="${log_file%.log}_orch.json"
            cp "${sqlite_db}" "${temp_db}" 2>/dev/null || true
            if [[ -f "${sqlite_db}-wal" ]]; then
                cp "${sqlite_db}-wal" "${temp_db}-wal" 2>/dev/null || true
            fi
            if [[ -f "${sqlite_db}-shm" ]]; then
                cp "${sqlite_db}-shm" "${temp_db}-shm" 2>/dev/null || true
            fi
            if [[ -f "${temp_db}" ]]; then
                scripting_seed_orchestrator_from_source "${temp_db}" || true
                scripting_point_sqlite_engines_at_mock "${temp_db}" "${SCRIPTING_MOCK_LLM_URL}" || true
                if jq --arg db "${temp_db}" '
                    .Databases.Connections |= map(
                        if ((.Engine // "") | ascii_downcase) == "sqlite" then
                            .Database = $db | .Chat = true
                        else . end
                    )
                ' "${config_file}" > "${temp_cfg}" 2>/dev/null
                then
                    run_config="${temp_cfg}"
                    sqlite_db="${temp_db}"
                fi
            fi
        else
            scripting_seed_orchestrator_from_source "${sqlite_db}" || true
        fi
    fi

    "${hydrogen_bin}" "${run_config}" > "${log_file}" 2>&1 &
    hydrogen_pid=$!
    disown "${hydrogen_pid}" 2>/dev/null || true
    if declare -f register_hydrogen_pid >/dev/null 2>&1; then
        register_hydrogen_pid "${hydrogen_pid}"
    fi
    echo "PID=${hydrogen_pid}" >> "${result_file}"
    if [[ -n "${prev_http_probe_base}" ]]; then
        export HYDROGEN_HTTP_PROBE_BASE="${prev_http_probe_base}"
    else
        unset HYDROGEN_HTTP_PROBE_BASE || true
    fi
    if [[ -n "${prev_llm_probe_model}" ]]; then
        export HYDROGEN_LLM_PROBE_MODEL="${prev_llm_probe_model}"
    else
        unset HYDROGEN_LLM_PROBE_MODEL || true
    fi

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
    if scripting_assert_log_contains "${log_file}" "Orchestrator: query_probe ok"; then
        echo "ORCH_QUERY_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: mail_probe ok"; then
        echo "ORCH_MAIL_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: mail_repo_probe ok"; then
        echo "ORCH_MAIL_REPO_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "MAILRELAY_REPO_PROBE_OK"; then
        echo "MAIL_REPO_PROBE_LAUNCH" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: notify_probe ok"; then
        echo "ORCH_NOTIFY_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: http_probe ok"; then
        echo "ORCH_HTTP_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: scoreboard_probe ok"; then
        echo "ORCH_SCOREBOARD_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: llm_probe ok"; then
        echo "ORCH_LLM_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: system_probe ok"; then
        echo "ORCH_SYSTEM_PROBE" >> "${result_file}"
    fi
    if scripting_assert_log_contains "${log_file}" "Orchestrator: api_error_probe ok"; then
        echo "ORCH_API_ERROR_PROBE" >> "${result_file}"
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

# Point SQLite lookup engine endpoints at a local mock OpenAI-compatible URL
# (same rewrite pattern as test_59). Safe no-op when sqlite3/db missing.
# Usage: scripting_point_sqlite_engines_at_mock <sqlite_db_path> <mock_url>
scripting_point_sqlite_engines_at_mock() {
    local db_path="$1"
    local mock_url="$2"
    if [[ -z "${db_path}" || -z "${mock_url}" || ! -f "${db_path}" ]]; then
        return 1
    fi
    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi
    # shellcheck disable=SC2016 # SQL uses single-quoted JSON paths for sqlite3
    sqlite3 "${db_path}" <<SQL
UPDATE lookups
SET collection = json_set(
        json_set(
            json_set(collection, '$.endpoint', '${mock_url}'),
            '$.engine', 'openai'
        ),
        '$.api key', 'mock-key'
    )
WHERE collection IS NOT NULL
  AND json_valid(collection)
  AND json_extract(collection, '$.endpoint') IS NOT NULL;
SQL
}

# First engine display name from a SQLite fixture (for HYDROGEN_LLM_PROBE_MODEL).
# Usage: scripting_first_engine_name <sqlite_db_path>
scripting_first_engine_name() {
    local db_path="$1"
    if [[ -z "${db_path}" || ! -f "${db_path}" ]]; then
        return 1
    fi
    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi
    sqlite3 "${db_path}" \
        "SELECT json_extract(collection,'\$.name') FROM lookups WHERE collection IS NOT NULL AND json_valid(collection) AND json_extract(collection,'\$.endpoint') IS NOT NULL LIMIT 1;" \
        2>/dev/null
}

# Refresh the Orchestrator Lua source in a SQLite fixture from the in-tree
# reference file (src/scripting/orchestrator.lua). Keeps blackbox DBs aligned
# with the data-plane query probe without requiring a migration re-run.
# Usage: scripting_seed_orchestrator_from_source <sqlite_db_path> [lua_path]
scripting_seed_orchestrator_from_source() {
    local db_path="$1"
    local lua_path="${2:-}"
    local project_dir="${PROJECT_DIR:-}"

    if [[ -z "${lua_path}" ]]; then
        if [[ -n "${project_dir}" && -f "${project_dir}/src/scripting/orchestrator.lua" ]]; then
            lua_path="${project_dir}/src/scripting/orchestrator.lua"
        else
            return 1
        fi
    fi
    if [[ ! -f "${db_path}" || ! -f "${lua_path}" ]]; then
        return 1
    fi
    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    sqlite3 "${db_path}" \
        "UPDATE scripts SET code = readfile('${lua_path}') WHERE group_name = 'Orchestrators' AND script_name = 'Orchestrator';"
}
