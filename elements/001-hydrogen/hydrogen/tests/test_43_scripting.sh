#!/usr/bin/env bash

# Test: Scripting Subsystem — Orchestrator Lifecycle (All Engines, Parallel)
#
# Boots Hydrogen with the Scripting subsystem enabled and the reference
# Orchestrator configured, verifies the Orchestrator actually starts
# (per the 11f sync DB load), produces ticks, and shuts down cleanly —
# then confirms the C-side and Lua-side teardown log lines fire.
#
# Like test_40_auth.sh, this runs one Hydrogen instance per database
# engine in parallel (PostgreSQL, MySQL, SQLite, DB2, MariaDB,
# CockroachDB, YugabyteDB), each with its own configuration file and
# WebServer port. Two variants are exercised per engine:
#   - "with DefaultDatabase"    (Scripting.DefaultDatabase = "Acuranzo")
#   - "without DefaultDatabase" (field omitted)
# With a single configured database the two must behave identically:
# the Orchestrator resolves the sole database automatically, so the
# lifecycle is the same with or without an explicit DefaultDatabase.
#
# This test ASSUMES the scripting migrations (1201+, the `scripts`
# table, and the Orchestrators.Orchestrator row) are already present in
# each engine's fixture. If the scripting subsystem cannot start the
# Orchestrator because those are missing, the failure is captured in the
# log (a "no script row" / "continuing without one" marker) and that
# engine's run ends quickly (fail-fast) instead of blocking on the full
# startup timeout.
#
# This first phase of the test only tracks logs: it confirms the
# scripting subsystem starts the main orchestration script and ends it
# cleanly. Future phases (12+) will add data-plane subtests (H.wait,
# H.query, task claims, etc.) per the LUA_PLAN.
#
# Helpers live in tests/lib/scripting_helpers.sh.

# FUNCTIONS
# (Helper functions live in tests/lib/scripting_helpers.sh)

# CHANGELOG
# 2.1.0 - 2026-07-15 - Moved WebServer listeners from Linux ephemeral range 55430-55446 to dedicated 15430-15446 ports.
# 2.0.0 - 2026-07-02 - Phase 11i: run all 7 database engines in parallel (like test_40),
#                      one config per engine, with and without DefaultDatabase, and
#                      fail-fast when the scripting migrations are unavailable.
# 1.0.0 - 2026-07-01 - Initial implementation for LUA_PLAN Phase 11h (first scripting blackbox test)

set -euo pipefail

# Test Configuration
TEST_NAME="Scripting  {BLUE}engines: 7{RESET}"
TEST_ABBR="SCR"
TEST_NUMBER="43"
TEST_COUNTER=0
TEST_VERSION="2.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/scripting_helpers.sh # Phase 11h split for code-size cap
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A SCRIPTING_TEST_CONFIGS

# One entry per engine/variant. Format:
#   "config_file:log_suffix:engine_name:description"
# Both a "with DefaultDatabase" and a "without DefaultDatabase" variant
# are included for every engine so requirement (3) is exercised across
# all engines simultaneously.
SCRIPTING_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_postgres.json:postgres:postgresql:PostgreSQL (default DB)"
    ["PostgreSQL-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_postgres_no_default.json:postgres_nd:postgresql:PostgreSQL (no default DB)"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_mysql.json:mysql:mysql:MySQL (default DB)"
    ["MySQL-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_mysql_no_default.json:mysql_nd:mysql:MySQL (no default DB)"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_sqlite.json:sqlite:sqlite:SQLite (default DB)"
    ["SQLite-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_sqlite_no_default.json:sqlite_nd:sqlite:SQLite (no default DB)"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_db2.json:db2:db2:DB2 (default DB)"
    ["DB2-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_db2_no_default.json:db2_nd:db2:DB2 (no default DB)"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_mariadb.json:mariadb:mariadb:MariaDB (default DB)"
    ["MariaDB-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_mariadb_no_default.json:mariadb_nd:mariadb:MariaDB (no default DB)"
    ["CockroachDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_cockroachdb.json:cockroachdb:cockroachdb:CockroachDB (default DB)"
    ["CockroachDB-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_cockroachdb_no_default.json:cockroachdb_nd:cockroachdb:CockroachDB (no default DB)"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB (default DB)"
    ["YugabyteDB-ND"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_scripting_yugabytedb_no_default.json:yugabytedb_nd:yugabytedb:YugabyteDB (no default DB)"
)

# Timeouts (seconds). Fixtures are assumed pre-migrated, so READY is
# quick; the fail-fast path keeps a broken engine from blocking here.
STARTUP_TIMEOUT=60
SHUTDOWN_TIMEOUT=15
TICK_SETTLE_SECONDS=3

# ---------------------------------------------------------------------------
# Pre-flight
# ---------------------------------------------------------------------------

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
for test_config in "${!SCRIPTING_TEST_CONFIGS[@]}"; do
    IFS=':' read -r config_file log_suffix _ description <<< "${SCRIPTING_TEST_CONFIGS[${test_config}]}"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} will use port: ${port}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#SCRIPTING_TEST_CONFIGS[@]} configuration files validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Parallel Orchestrator lifecycle across all engines/variants
# ---------------------------------------------------------------------------

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running scripting Orchestrator lifecycle in parallel"

    for test_config in "${!SCRIPTING_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n
        done

        IFS=':' read -r config_file log_suffix _ description <<< "${SCRIPTING_TEST_CONFIGS[${test_config}]}"

        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel run: ${test_config} (${description})"

        scripting_run_engine_parallel \
            "${config_file}" "${log_file}" "${result_file}" "${HYDROGEN_BIN}" \
            "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${TICK_SETTLE_SECONDS}" &
        PARALLEL_PIDS+=($!)
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#SCRIPTING_TEST_CONFIGS[@]} parallel runs to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel runs completed"

    # Analyze results sequentially for clean output.
    successful_configs=0
    for test_config in "${!SCRIPTING_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix _ description <<< "${SCRIPTING_TEST_CONFIGS[${test_config}]}"

        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

        print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: log ..${log_file##*/}"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Orchestrator lifecycle"

        if [[ ! -f "${result_file}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: no result file"
            EXIT_CODE=1
            continue
        fi

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if scripting_assert_log_contains "${result_file}" "FAILFAST"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: scripting migrations unavailable (fail-fast)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Orchestrator could not start (missing scripts table/migrations)"
            EXIT_CODE=1
            continue
        fi

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if scripting_assert_log_contains "${result_file}" "LIFECYCLE_COMPLETE"; then
            tick_line=$("${GREP}" "^ORCH_TICKS=" "${result_file}" 2>/dev/null | head -1 || echo "ORCH_TICKS=0")
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: start -> ${tick_line#ORCH_TICKS=} tick(s) -> clean shutdown"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
            successful_configs=$(( successful_configs + 1 ))
        else
            reason="incomplete lifecycle"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            scripting_assert_log_contains "${result_file}" "NOT_READY" && reason="did not reach READY within ${STARTUP_TIMEOUT}s"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            scripting_assert_log_contains "${result_file}" "STARTUP_FAILED" && reason="process crashed at startup"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: ${reason}"
            EXIT_CODE=1
        fi
    done

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#SCRIPTING_TEST_CONFIGS[@]} engine/variant runs completed the full Orchestrator lifecycle"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping scripting lifecycle runs due to prerequisite failures"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Completion
# ---------------------------------------------------------------------------

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
