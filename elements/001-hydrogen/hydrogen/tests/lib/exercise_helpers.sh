#!/usr/bin/env bash

# Shared helpers for multi-engine auth exercise tests (test_41 ASAN, test_44 native).
# Callers must set TEST_NUMBER / TEST_COUNTER / GREP via framework before sourcing.

# shellcheck disable=SC2154 # Globals (TEST_NUMBER, TEST_COUNTER, GREP) set by framework before sourcing

# CHANGELOG
# 1.0.0 - 2026-07-09 - Extracted from test_41_exercise.sh for ASAN/native split

[[ -n "${EXERCISE_HELPERS_GUARD:-}" ]] && return 0
export EXERCISE_HELPERS_GUARD="true"

EXERCISE_HELPERS_NAME="Exercise Test Helpers"
EXERCISE_HELPERS_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${EXERCISE_HELPERS_NAME} ${EXERCISE_HELPERS_VERSION}" "info"

# Defaults (callers may override before first scrape)
SCRAPE_MAX_ATTEMPTS="${SCRAPE_MAX_ATTEMPTS:-5}"
SCRAPE_CURL_TIMEOUT="${SCRAPE_CURL_TIMEOUT:-15}"
SCRAPE_RETRY_DELAY="${SCRAPE_RETRY_DELAY:-2}"
METRICS_DELAY="${METRICS_DELAY:-0.25}"

# scrape_metrics prom_url [settle_delay]
# Retries until a body containing hydrogen_ metrics is returned, or attempts exhausted.
scrape_metrics() {
    local prom_url="$1"
    local settle_delay="${2:-${METRICS_DELAY}}"
    local attempt response

    if [[ -n "${settle_delay}" ]] && awk "BEGIN {exit !(${settle_delay} > 0)}" 2>/dev/null; then
        sleep "${settle_delay}"
    fi
    for ((attempt=1; attempt<=SCRAPE_MAX_ATTEMPTS; attempt++)); do
        response=$(curl -s --max-time "${SCRAPE_CURL_TIMEOUT}" "${prom_url}" 2>/dev/null || true)
        if [[ -n "${response}" ]] && echo "${response}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            echo "${response}"
            return 0
        fi
        if [[ "${attempt}" -lt "${SCRAPE_MAX_ATTEMPTS}" ]]; then
            sleep "${SCRAPE_RETRY_DELAY}"
        fi
    done
    echo ""
}

# get_metric metrics_text metric_name
get_metric() {
    local metrics="$1"
    local name="$2"
    local value
    value=$(echo "${metrics}" | "${GREP}" "^${name}" 2>/dev/null | head -1 | awk '{print $NF}' | tr -d '[:space:]' || true)
    if [[ -z "${value}" ]] || ! [[ "${value}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        echo "0"
    else
        echo "${value}"
    fi
}

# run_auth_request url db_name req_num
run_auth_request() {
    local url="$1"
    local db_name="$2"
    local req_num="$3"
    local login_data

    if (( req_num % 2 == 0 )); then
        login_data="{\"database\": \"${db_name}\", \"login_id\": \"${HYDROGEN_DEMO_USER_NAME}\", \"password\": \"${HYDROGEN_DEMO_USER_PASS}\", \"api_key\": \"${HYDROGEN_DEMO_API_KEY}\", \"tz\": \"America/Vancouver\"}"
    else
        login_data="{\"database\": \"${db_name}\", \"login_id\": \"${HYDROGEN_DEMO_USER_NAME}\", \"password\": \"WrongPassword123!\", \"api_key\": \"${HYDROGEN_DEMO_API_KEY}\", \"tz\": \"America/Vancouver\"}"
    fi

    curl -s -X POST -H "Content-Type: application/json" \
        -d "${login_data}" --max-time 5 --compressed \
        "${url}/api/auth/login" >/dev/null 2>&1 || true
}

# run_auth_batch url start_req batch_size db_array_name
# Concurrent curls; preserves RR DB assignment and even/odd credential mix.
run_auth_batch() {
    local url="$1"
    local start_req="$2"
    local batch_size="$3"
    local -n _auth_dbs="$4"
    local db_count=${#_auth_dbs[@]}
    local j req db_index db_name

    if [[ "${batch_size}" -le 0 || "${db_count}" -le 0 ]]; then
        return 0
    fi

    for ((j=0; j<batch_size; j++)); do
        req=$((start_req + j))
        db_index=$(( (req - 1) % db_count ))
        db_name="${_auth_dbs[${db_index}]}"
        run_auth_request "${url}" "${db_name}" "${req}" &
    done
    wait || true
}

# exercise_enabled_db_names config_file
# Echo space-separated connection Names that are Enabled:true in the config.
exercise_enabled_db_names() {
    local config_file="$1"
    local names
    names=$(jq -r '.Databases.Connections[] | select(.Enabled == true) | .Name' "${config_file}" 2>/dev/null) || true
    names=$(echo "${names}" | tr '\n' ' ')
    echo "${names%"${names##*[![:space:]]}"}"
}

# exercise_enabled_db_count config_file
exercise_enabled_db_count() {
    local config_file="$1"
    jq '[.Databases.Connections[] | select(.Enabled == true)] | length' "${config_file}" 2>/dev/null || echo "0"
}
