#!/usr/bin/env bash

# Shared helpers for multi-engine auth exercise tests (test_41 ASAN, test_44 native).
# Callers must set TEST_NUMBER / TEST_COUNTER / GREP via framework before sourcing.

# shellcheck disable=SC2154 # Globals (TEST_NUMBER, TEST_COUNTER, GREP) set by framework before sourcing

# CHANGELOG
# 1.0.4 - 2026-08-29 - scrape_metrics status written to SCRAPE_STATUS_FILE so
#                      callers in command-substitution subshells can read the
#                      real last HTTP code and attempt count (previously
#                      reported 000/0/3 due to subshell variable scoping).
# 1.0.3 - 2026-08-28 - scrape_metrics exposes SCRAPE_LAST_HTTP_CODE/SCRAPE_LAST_ATTEMPTS
#                      diagnostics on exhaustion instead of a bare empty string.
# 1.0.2 - 2026-08-27 - Scrape INFO delay; flood connect-timeout; source group40.
# 1.0.1 - 2026-08-27 - Auth flood max-time 5s -> 15s (wait, do not retry).
# 1.0.0 - 2026-07-09 - Extracted from test_41_exercise.sh for ASAN/native split

[[ -n "${EXERCISE_HELPERS_GUARD:-}" ]] && return 0
export EXERCISE_HELPERS_GUARD="true"

EXERCISE_HELPERS_NAME="Exercise Test Helpers"
EXERCISE_HELPERS_VERSION="1.0.4"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${EXERCISE_HELPERS_NAME} ${EXERCISE_HELPERS_VERSION}" "info"

# shellcheck source=tests/lib/group40_http.sh # Shared 40-series HTTP timing
[[ -n "${GROUP40_HTTP_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/group40_http.sh"

# Defaults (callers may override before first scrape).
# Force numeric defaults: suite-parallel sourcing can leave empty/non-numeric
# SCRAPE_* in the environment, which collapses the retry loop to a single pass.
if ! [[ "${SCRAPE_MAX_ATTEMPTS:-}" =~ ^[1-9][0-9]*$ ]]; then
    SCRAPE_MAX_ATTEMPTS=5
fi
if ! [[ "${SCRAPE_CURL_TIMEOUT:-}" =~ ^[1-9][0-9]*$ ]]; then
    SCRAPE_CURL_TIMEOUT=15
fi
if ! [[ "${SCRAPE_RETRY_DELAY:-}" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    SCRAPE_RETRY_DELAY=2
fi
if ! [[ "${METRICS_DELAY:-}" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    METRICS_DELAY=0.25
fi

# scrape_metrics prom_url [settle_delay]
# Retries until a body containing hydrogen_ metrics is returned, or attempts exhausted.
# Uses connect-timeout so a hung ASAN handler does not burn the full max-time on
# every attempt before retries can help (suite-parallel contention).
# On exhaustion (empty echo), diagnostics are left in SCRAPE_LAST_HTTP_CODE and
# SCRAPE_LAST_ATTEMPTS for the caller to report (e.g. "000" means every attempt
# never connected/completed - a stall - vs. a non-200 that did respond).
scrape_metrics() {
    local prom_url="$1"
    local settle_delay="${2:-${METRICS_DELAY}}"
    local attempt response http_code
    local max_attempts="${SCRAPE_MAX_ATTEMPTS}"
    local curl_timeout="${SCRAPE_CURL_TIMEOUT}"
    local retry_delay="${SCRAPE_RETRY_DELAY}"
    local tmp_body=""

    SCRAPE_LAST_HTTP_CODE=""
    SCRAPE_LAST_ATTEMPTS=0

    # Use a well-known temp file path so callers (even those in command
    # substitution subshells) can read the last HTTP code and attempt count.
    # Command substitution creates a subshell where globals don't propagate.
    local _status_dir="${SCRATCH_DIR:-/tmp/kilo}"
    mkdir -p "${_status_dir}" 2>/dev/null || true
    SCRAPE_STATUS_FILE="${_status_dir}/scrape_${TEST_NUMBER}_status"

    if ! [[ "${max_attempts}" =~ ^[1-9][0-9]*$ ]]; then
        max_attempts=5
    fi
    if ! [[ "${curl_timeout}" =~ ^[1-9][0-9]*$ ]]; then
        curl_timeout=15
    fi

    if [[ -n "${settle_delay}" ]] && awk "BEGIN {exit !(${settle_delay} > 0)}" 2>/dev/null; then
        sleep "${settle_delay}"
    fi

    tmp_body=$(mktemp 2>/dev/null) || tmp_body=""
    for ((attempt=1; attempt<=max_attempts; attempt++)); do
        http_code="000"
        response=""
        if [[ -n "${tmp_body}" ]]; then
            : > "${tmp_body}"
            # shellcheck disable=SC2034 # http_code used for 200 check below
            local t0 t1 elapsed
            t0="${EPOCHREALTIME:-0}"
            http_code=$(curl -sS -o "${tmp_body}" -w "%{http_code}" \
                --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" \
                --max-time "${curl_timeout}" \
                "${prom_url}" 2>/dev/null || echo "000")
            t1="${EPOCHREALTIME:-0}"
            elapsed=$(awk -v a="${t0}" -v b="${t1}" 'BEGIN {printf "%.3f", b-a}')
            group40_log_delay "${elapsed}" "scrape ${prom_url} HTTP ${http_code}"
            response=$(cat "${tmp_body}" 2>/dev/null || true)
        else
            response=$(curl -s --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" --max-time "${curl_timeout}" \
                "${prom_url}" 2>/dev/null || true)
            if [[ -n "${response}" ]]; then
                http_code="200"
            fi
        fi
        SCRAPE_LAST_HTTP_CODE="${http_code}"
        SCRAPE_LAST_ATTEMPTS="${attempt}"
        echo "${http_code}" > "${SCRAPE_STATUS_FILE}" 2>/dev/null || true
        echo "${attempt}" >> "${SCRAPE_STATUS_FILE}" 2>/dev/null || true
        if [[ "${http_code}" == "200" ]] && [[ -n "${response}" ]] && \
           echo "${response}" | "${GREP}" -q "hydrogen_" 2>/dev/null; then
            [[ -n "${tmp_body}" ]] && rm -f "${tmp_body}"
            echo "${response}"
            return 0
        fi
        if [[ "${attempt}" -lt "${max_attempts}" ]]; then
            sleep "${retry_delay}"
        fi
    done
    [[ -n "${tmp_body}" ]] && rm -f "${tmp_body}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "WARNING: scrape_metrics exhausted ${SCRAPE_LAST_ATTEMPTS}/${max_attempts} attempts against ${prom_url} (last HTTP ${SCRAPE_LAST_HTTP_CODE:-000})"
    # Ensure the status file reflects the final state (overwrite)
    echo "${SCRAPE_LAST_HTTP_CODE:-000}" > "${SCRAPE_STATUS_FILE}" 2>/dev/null || true
    echo "${SCRAPE_LAST_ATTEMPTS:-${max_attempts}}" >> "${SCRAPE_STATUS_FILE}" 2>/dev/null || true
    echo ""
}

# read_scrape_status [test_number]
# Reads the SCRAPE_STATUS_FILE written by scrape_metrics (called in a subshell).
# Sets globals SCRAPE_LAST_HTTP_CODE and SCRAPE_LAST_ATTEMPTS for the caller.
read_scrape_status() {
    local _tn="${1:-${TEST_NUMBER}}"
    local _sf="${SCRATCH_DIR:-/tmp/kilo}/scrape_${_tn}_status"
    SCRAPE_LAST_HTTP_CODE=""
    SCRAPE_LAST_ATTEMPTS=0
    if [[ -f "${_sf}" ]]; then
        SCRAPE_LAST_HTTP_CODE=$(head -1 "${_sf}" 2>/dev/null || echo "")
        SCRAPE_LAST_ATTEMPTS=$(sed -n '2p' "${_sf}" 2>/dev/null || echo "")
    fi
    [[ -z "${SCRAPE_LAST_HTTP_CODE}" ]] && SCRAPE_LAST_HTTP_CODE="000"
    [[ -z "${SCRAPE_LAST_ATTEMPTS}" ]] && SCRAPE_LAST_ATTEMPTS=0
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
        -d "${login_data}" --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" --max-time 15 --compressed \
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
