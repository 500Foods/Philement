#!/usr/bin/env bash

# MCP Streamable HTTP blackbox helpers for tests/test_47_mcp.sh.
#
# Local variable prefix in callers must be lowercase so the test_03
# env-var scanner does not treat them as required environment variables.

# shellcheck disable=SC2154 # Globals (TEST_NUMBER, GREP, DATE, HTTP_TIMEOUT, READY_TIMEOUT, BASELINE_SQLITE, …) come from the test script / framework.sh
# shellcheck disable=SC2312 # Diagnostic substitutions swallow inner status; callers use || true

# CHANGELOG
# 1.0.0 - 2026-08-27 - Split from test_47 to stay under the 1000-line cap

[[ -n "${MCP_HELPERS_GUARD:-}" ]] && return 0
export MCP_HELPERS_GUARD="true"

MCP_HELPERS_NAME="MCP Test Helpers"
MCP_HELPERS_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${MCP_HELPERS_NAME} ${MCP_HELPERS_VERSION}" "info"

api_request() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local jwt_token="${5:-}"
    local max_time="${6:-${HTTP_TIMEOUT}}"
    local max_retries=5
    local retry=1
    local http_status="000"

    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
            --connect-timeout 10 --max-time "${max_time}" -w "%{http_code}" -o "${output_file}")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ "${method}" != "GET" && "${method}" != "DELETE" && -n "${data}" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=("${url}")
        # shellcheck disable=SC2312 # Intentionally swallow curl exit code; we use the HTTP status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status="${http_status:-000}"

        if [[ "${http_status}" == "000" || \
              "${http_status}" == 5* || \
              "${http_status}" == "408" || \
              "${http_status}" == "429" ]]; then
            if [[ "${retry}" -eq "${max_retries}" ]]; then
                break
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP ${http_status} from ${method} ${url}, retrying (${retry}/${max_retries})..."
            sleep "${retry}"
            retry=$(( retry + 1 ))
            continue
        fi
        break
    done

    echo "${http_status}"
}

mcp_http() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local header_file="$5"
    local jwt_token="${6:-}"
    local session_id="${7:-}"
    local origin="${8:-}"
    local max_time="${9:-${HTTP_TIMEOUT}}"
    local max_retries=5
    local retry=1
    local http_status="000"

    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
            --connect-timeout 10 --max-time "${max_time}"
            -D "${header_file}" -o "${output_file}" -w "%{http_code}")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ -n "${session_id}" ]]; then
            curl_cmd+=(-H "Mcp-Session-Id: ${session_id}")
        fi
        if [[ -n "${origin}" ]]; then
            curl_cmd+=(-H "Origin: ${origin}")
        fi
        if [[ "${method}" != "GET" && "${method}" != "DELETE" && -n "${data}" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=("${url}")
        # shellcheck disable=SC2312 # Intentionally swallow curl exit code; we use the HTTP status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status="${http_status:-000}"

        if [[ "${http_status}" == "000" || \
              "${http_status}" == 5* || \
              "${http_status}" == "408" || \
              "${http_status}" == "429" ]]; then
            if [[ "${retry}" -eq "${max_retries}" ]]; then
                break
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP ${http_status} from ${method} ${url}, retrying (${retry}/${max_retries})..."
            sleep "${retry}"
            retry=$(( retry + 1 ))
            continue
        fi
        break
    done

    echo "${http_status}"
}

# Retry JSON-RPC until HTTP 200 and jq filter match (QueryRef 152 can miss once).
mcp_expect_jq() {
    local url="$1"
    local jwt_token="$2"
    local session_id="$3"
    local payload="$4"
    local output_file="$5"
    local header_file="$6"
    local jq_filter="$7"
    local max_try="${8:-3}"
    local try=1
    local http_status="000"

    while [[ "${try}" -le "${max_try}" ]]; do
        http_status=$(mcp_http "POST" "${url}" "${payload}" "${output_file}" \
            "${header_file}" "${jwt_token}" "${session_id}" "" 20)
        if [[ "${http_status}" == "200" ]] \
            && jq -e "${jq_filter}" "${output_file}" >/dev/null 2>&1; then
            echo "${http_status}"
            return 0
        fi
        if [[ "${try}" -eq "${max_try}" ]]; then
            break
        fi
        sleep "${try}"
        try=$(( try + 1 ))
    done
    echo "${http_status}"
    return 1
}

extract_jwt() {
    local response_file="$1"
    if [[ -f "${response_file}" ]]; then
        jq -r '.token // empty' "${response_file}" 2>/dev/null || true
    fi
}

header_value() {
    local header_file="$1"
    local name="$2"
    if [[ -f "${header_file}" ]]; then
        "${GREP}" -i "^${name}:" "${header_file}" 2>/dev/null | head -1 | cut -d: -f2- | tr -d '\r' | sed 's/^[[:space:]]*//' || true
    fi
}

wait_ready() {
    local log_file="$1"
    local timeout="${2:-${READY_TIMEOUT}}"
    local start_time
    local now_secs
    start_time=$("${DATE}" +%s)
    while true; do
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            return 0
        fi
        now_secs=$("${DATE}" +%s)
        if (( now_secs - start_time >= timeout )); then
            return 1
        fi
        sleep 0.2
    done
}

b64url_encode() {
    openssl base64 -e -A | tr '+/' '-_' | tr -d '='
}

b64url_decode() {
    local s="$1"
    local mod=$(( ${#s} % 4 ))
    if [[ "${mod}" -eq 2 ]]; then
        s="${s}=="
    elif [[ "${mod}" -eq 3 ]]; then
        s="${s}="
    fi
    printf '%s' "${s}" | tr -- '-_' '+/' | openssl base64 -d -A 2>/dev/null || true
}

token_hash() {
    local token="$1"
    printf '%s' "${token}" | openssl dgst -sha256 -binary | b64url_encode
}

# Rebuild JWT A with a different sub so session bind rejects it (sqlite only).
mint_hijack_jwt() {
    local src_jwt="$1"
    local secret="$2"
    local new_sub="$3"
    local payload_b64 header_b64 payload_json new_payload unsigned sig
    header_b64=$(printf '%s' "${src_jwt}" | cut -d. -f1)
    payload_b64=$(printf '%s' "${src_jwt}" | cut -d. -f2)
    payload_json=$(b64url_decode "${payload_b64}")
    if [[ -z "${payload_json}" ]]; then
        return 1
    fi
    new_payload=$(printf '%s' "${payload_json}" | jq -c --arg sub "${new_sub}" '.sub = $sub')
    payload_b64=$(printf '%s' "${new_payload}" | b64url_encode)
    unsigned="${header_b64}.${payload_b64}"
    sig=$(printf '%s' "${unsigned}" | openssl dgst -sha256 -hmac "${secret}" -binary | b64url_encode)
    printf '%s' "${unsigned}.${sig}"
}

prepare_sqlite_config() {
    local src_config="$1"
    local work_dir="$2"
    local out_config="${work_dir}/config.json"
    local db_copy="${work_dir}/hydrodemo.sqlite"
    mkdir -p "${work_dir}"
    cp -f "${BASELINE_SQLITE}" "${db_copy}"
    if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
        cp -f "${BASELINE_SQLITE}-wal" "${db_copy}-wal" 2>/dev/null || true
    fi
    if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
        cp -f "${BASELINE_SQLITE}-shm" "${db_copy}-shm" 2>/dev/null || true
    fi
    local seed_n
    seed_n=$(sqlite3 "${db_copy}" \
        "SELECT COUNT(*) FROM scripts WHERE group_name='Mcp' AND script_name='Server' AND mcp_access<>0;" \
        2>/dev/null || echo 0)
    if [[ "${seed_n}" -lt 1 ]]; then
        return 1
    fi
    jq --arg db "${db_copy}" '
        .Databases.Connections |= map(
            if ((.Engine // "") | ascii_downcase) == "sqlite" then
                .Database = $db | .AutoMigration = false
            else . end
        )
    ' "${src_config}" > "${out_config}"
    echo "${out_config}"
}

record_case() {
    local result_file="$1"
    local name="$2"
    local ok="$3"
    if [[ "${ok}" == "1" ]]; then
        echo "CASE_PASS=${name}" >> "${result_file}"
    else
        echo "CASE_FAIL=${name}" >> "${result_file}"
    fi
}
