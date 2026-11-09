#!/usr/bin/env bash

# Engine Utilities Library
# Makes HYDROGEN_ENGINES the authoritative list of database engines that tests run against
# HYDROGEN_ENGINES is a comma-separated list, eg: "postgresql,sqlite,db2,mariadb,firebird,mssql"
# Case and whitespace are ignored. Unset or empty means every engine is enabled.
# PostgreSQL and YugabyteDB are separate engines: postgres -> postgresql, yugabyte -> yugabytedb

# LIBRARY FUNCTIONS
# normalize_engine_name()
# is_known_engine()
# engine_enabled()
# enabled_engine_count()
# unknown_engine_tokens()
# warn_unknown_engines()
# filter_engine_map()
# skip_engine_test()

# CHANGELOG
# 1.0.0 - 2026-11-09 - Initial creation: HYDROGEN_ENGINES filtering for the 30-series tests

set -euo pipefail

# Guard clause to prevent multiple sourcing
# Not exported: the functions only exist in shells that sourced this file
[[ -n "${ENGINE_UTILS_GUARD:-}" ]] && return 0
ENGINE_UTILS_GUARD="true"

# Library metadata
# Sourced before setup_test_environment (so a test title can say "(skipped)"),
# which is before print_message exists, so there is no load announcement here
ENGINE_UTILS_NAME="Engine Utilities Library"
ENGINE_UTILS_VERSION="1.0.0"
export ENGINE_UTILS_NAME ENGINE_UTILS_VERSION

# Canonical engine names
HYDROGEN_ENGINE_NAMES=("postgresql" "sqlite" "mysql" "db2" "mariadb" "firebird" "yugabytedb" "mssql")

# Engines removed by the last filter_engine_map call (map keys)
SKIPPED_ENGINE_KEYS=()

# Unknown tokens are only reported once per shell
ENGINE_UTILS_WARNED=false

# Function: Convert an engine name or alias to its canonical name
# Parameters: $1 - engine name (any case, surrounding whitespace ignored)
# Output: canonical name, or the cleaned-up input if it is not a known engine
normalize_engine_name() {
    local name="${1,,}"
    name="${name//[[:space:]]/}"
    case "${name}" in
        postgres)   name="postgresql" ;;
        yugabyte)   name="yugabytedb" ;;
        *)          ;;
    esac
    echo "${name}"
}

# Function: Check whether a name (after normalizing) is one of the canonical engines
# Parameters: $1 - engine name
# Returns: 0 if known, 1 otherwise
is_known_engine() {
    local name known
    name=$(normalize_engine_name "$1")
    for known in "${HYDROGEN_ENGINE_NAMES[@]}"; do
        [[ "${known}" == "${name}" ]] && return 0
    done
    return 1
}

# Function: Check whether an engine is enabled by HYDROGEN_ENGINES
# Parameters: $1 - engine name or alias
# Returns: 0 if enabled (or HYDROGEN_ENGINES is unset/empty), 1 otherwise
engine_enabled() {
    local list="${HYDROGEN_ENGINES:-}"
    [[ -z "${list//[[:space:],]/}" ]] && return 0

    local wanted token normalized
    wanted=$(normalize_engine_name "$1")
    local -a tokens=()
    IFS=',' read -ra tokens <<< "${list}"
    for token in "${tokens[@]}"; do
        normalized=$(normalize_engine_name "${token}")
        [[ "${normalized}" == "${wanted}" ]] && return 0
    done
    return 1
}

# Function: Count the canonical engines that are enabled
# Output: number of enabled engines (0-8)
enabled_engine_count() {
    local engine count=0
    for engine in "${HYDROGEN_ENGINE_NAMES[@]}"; do
        # shellcheck disable=SC2310 # engine_enabled is a test, its status is the point
        if engine_enabled "${engine}"; then
            count=$(( count + 1 ))
        fi
    done
    echo "${count}"
}

# Function: List HYDROGEN_ENGINES entries that are not known engines
# Output: space-separated unknown tokens (empty if none)
unknown_engine_tokens() {
    local list="${HYDROGEN_ENGINES:-}"
    local token cleaned
    local -a tokens=()
    local -a unknown=()
    IFS=',' read -ra tokens <<< "${list}"
    for token in "${tokens[@]}"; do
        cleaned="${token//[[:space:]]/}"
        [[ -z "${cleaned}" ]] && continue
        # shellcheck disable=SC2310 # is_known_engine is a test, its status is the point
        if ! is_known_engine "${cleaned}"; then
            unknown+=("${cleaned}")
        fi
    done
    echo "${unknown[*]}"
}

# Function: Warn (once per shell) about HYDROGEN_ENGINES entries that match no engine
# Never fails the test - an unknown entry simply enables nothing
warn_unknown_engines() {
    [[ "${ENGINE_UTILS_WARNED}" == true ]] && return 0
    ENGINE_UTILS_WARNED=true
    local unknown
    unknown=$(unknown_engine_tokens)
    if [[ -n "${unknown}" ]]; then
        # shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "HYDROGEN_ENGINES has unknown engine(s), ignored: ${unknown} (known: ${HYDROGEN_ENGINE_NAMES[*]})"
    fi
    return 0
}

# Function: Remove entries for disabled engines from an associative array
# Parameters: $1 - name of the associative array
#             $2 - 1-based colon-separated field in each value holding the engine name
# Entries whose engine field is not a known engine (eg: "multi") are kept
# Sets SKIPPED_ENGINE_KEYS to the removed keys (sorted)
filter_engine_map() {
    local -n engine_map="$1"
    local field="$2"
    local key engine
    local -a fields=()
    SKIPPED_ENGINE_KEYS=()
    for key in "${!engine_map[@]}"; do
        IFS=':' read -ra fields <<< "${engine_map[${key}]}"
        engine="${fields[$(( field - 1 ))]:-}"
        # shellcheck disable=SC2310 # is_known_engine and engine_enabled are tests, their status is the point
        if is_known_engine "${engine}" && ! engine_enabled "${engine}"; then
            SKIPPED_ENGINE_KEYS+=("${key}")
            unset "engine_map[${key}]"
        fi
    done
    if [[ ${#SKIPPED_ENGINE_KEYS[@]} -gt 0 ]]; then
        mapfile -t SKIPPED_ENGINE_KEYS < <("${PRINTF:-printf}" '%s\n' "${SKIPPED_ENGINE_KEYS[@]}" | sort || true)
    fi
}

# Function: Finish a single-engine test whose engine is not in HYDROGEN_ENGINES
# Parameters: $1 - engine display name (eg: "MySQL")
# Call after setup_test_environment, with TEST_NAME already ending in "(skipped)",
# then return/exit 0 from the test script itself
# The test reports 0 tests, 0 passed, 0 failed
skip_engine_test() {
    local engine_label="$1"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${engine_label} is not in HYDROGEN_ENGINES (${HYDROGEN_ENGINES:-}) - test skipped"
    # Standalone setup records a PASS for loading the framework; drop it so the summary is 0/0
    # shellcheck disable=SC2034 # Counters are read by log_output.sh and the calling test
    TEST_COUNTER=0 TEST_PASSED_COUNT=0 TEST_FAILED_COUNT=0 PASS_COUNT=0 EXIT_CODE=0
    # shellcheck disable=SC2154 # TEST_NAME, TEST_ABBR, TEST_NUMBER, TEST_VERSION defined by caller
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
}
