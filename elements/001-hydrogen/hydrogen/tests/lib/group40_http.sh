#!/usr/bin/env bash

# Shared HTTP timing for 40-series blackbox tests.
# Prefer one long wait over abort-and-retry. Retry only if curl never connected.

# CHANGELOG
# 1.0.1 - 2026-08-27 - group40_curl_to for form/OIDC extra curl args
# 1.0.0 - 2026-08-27 - Long max-time, 000-only retry, INFO delay lines

[[ -n "${GROUP40_HTTP_GUARD:-}" ]] && return 0
export GROUP40_HTTP_GUARD="true"

GROUP40_HTTP_NAME="Group40 HTTP"
GROUP40_HTTP_VERSION="1.0.1"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER set by framework before sourcing
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${GROUP40_HTTP_NAME} ${GROUP40_HTTP_VERSION}" "info"

: "${GROUP40_HTTP_MAX_TIME:=90}"
: "${GROUP40_CONNECT_TIMEOUT:=10}"
: "${GROUP40_STARTUP_TIMEOUT:=90}"
: "${GROUP40_READY_TIMEOUT:=120}"
: "${GROUP40_SHUTDOWN_TIMEOUT:=30}"
: "${GROUP40_SLOW_SECS:=2}"
: "${GROUP40_CONNECT_RETRIES:=2}"

group40_log_delay() {
    local elapsed="$1"
    local detail="$2"
    if awk -v e="${elapsed}" -v t="${GROUP40_SLOW_SECS}" 'BEGIN {exit !(e+0 >= t+0)}' 2>/dev/null; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "INFO delay ${elapsed}s: ${detail}"
    fi
}

# One curl; logs elapsed when slow. Echoes HTTP status (000 if no connect).
# Args: method url output_file [data] [jwt] [max_time]
group40_curl_once() {
    local method="$1"
    local url="$2"
    local output_file="$3"
    local data="${4:-}"
    local jwt_token="${5:-}"
    local max_time="${6:-${GROUP40_HTTP_MAX_TIME}}"
    local t0 t1 elapsed http_status
    local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
        --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" --max-time "${max_time}"
        -w "%{http_code}" -o "${output_file}")

    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi
    if [[ "${method}" != "GET" && "${method}" != "DELETE" && -n "${data}" ]]; then
        curl_cmd+=(-d "${data}")
    fi
    curl_cmd+=("${url}")

    t0="${EPOCHREALTIME:-}"
    # shellcheck disable=SC2312 # curl exit ignored; HTTP status is the signal
    http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
    http_status="${http_status:-000}"
    t1="${EPOCHREALTIME:-}"
    if [[ -n "${t0}" && -n "${t1}" ]]; then
        elapsed=$(awk -v a="${t0}" -v b="${t1}" 'BEGIN {printf "%.3f", b-a}')
    else
        elapsed="0"
    fi
    group40_log_delay "${elapsed}" "${method} ${url} HTTP ${http_status}"
    echo "${http_status}"
}

# Retry only when status is 000 (never connected). No retry on 408/5xx/401.
group40_curl() {
    local method="$1"
    local url="$2"
    local output_file="$3"
    local data="${4:-}"
    local jwt_token="${5:-}"
    local max_time="${6:-${GROUP40_HTTP_MAX_TIME}}"
    local try=1
    local http_status="000"

    while [[ "${try}" -le "${GROUP40_CONNECT_RETRIES}" ]]; do
        http_status=$(group40_curl_once "${method}" "${url}" "${output_file}" \
            "${data}" "${jwt_token}" "${max_time}")
        if [[ "${http_status}" != "000" ]]; then
            echo "${http_status}"
            return 0
        fi
        if [[ "${try}" -eq "${GROUP40_CONNECT_RETRIES}" ]]; then
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay connect-fail ${method} ${url}, retry ${try}/${GROUP40_CONNECT_RETRIES}"
        sleep 1
        try=$(( try + 1 ))
    done
    echo "${http_status}"
}

# Extra curl args (form POST, -D headers). Retry 000 only.
# Usage: group40_curl_to output_file [curl args…] URL
group40_curl_to() {
    local output_file="$1"
    shift
    local try=1
    local http_status="000"
    local t0 t1 elapsed

    while [[ "${try}" -le "${GROUP40_CONNECT_RETRIES}" ]]; do
        t0="${EPOCHREALTIME:-0}"
        # shellcheck disable=SC2312 # curl exit ignored; HTTP status is the signal
        http_status=$(curl -sS -o "${output_file}" -w '%{http_code}' \
            --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" \
            --max-time "${GROUP40_HTTP_MAX_TIME}" \
            "$@" 2>/dev/null || echo "000")
        http_status="${http_status:-000}"
        t1="${EPOCHREALTIME:-0}"
        elapsed=$(awk -v a="${t0}" -v b="${t1}" 'BEGIN {printf "%.3f", b-a}')
        group40_log_delay "${elapsed}" "curl HTTP ${http_status}"
        if [[ "${http_status}" != "000" ]]; then
            echo "${http_status}"
            return 0
        fi
        if [[ "${try}" -eq "${GROUP40_CONNECT_RETRIES}" ]]; then
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay connect-fail curl retry ${try}/${GROUP40_CONNECT_RETRIES}"
        sleep 1
        try=$(( try + 1 ))
    done
    echo "${http_status}"
}
