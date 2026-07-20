#!/usr/bin/env bash

# Test: Authenticated Chat API with local mock LLM (no external credits)
#
# Starts a Node mock OpenAI-compatible server, points the SQLite demo DB chat
# engine endpoints at it, enables Chat on the connection, then exercises
# /api/conduit/auth_chat and /api/conduit/auth_chats enough to produce blackbox
# coverage of both handlers.
#
# Required environment variables:
#   HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY
#   HYDROGEN_DEMO_JWT_KEY, PAYLOAD_KEY

# FUNCTIONS
# start_mock_llm()
# stop_mock_llm()
# prepare_sqlite_with_mock_endpoint()
# api_request()
# extract_jwt()

# CHANGELOG
# 1.0.0 - 2026-07-19 - Initial SQLite + mock LLM blackbox for auth_chat
# 1.1.0 - 2026-07-19 - Reordered cleanup/stop_mock_llm for shellcheck; justified
#                       SC2310 disables; added auth_chats blackbox coverage
# 1.2.0 - 2026-07-19 - Added auth_chat/stream blackbox coverage (SSE stub path)

set -euo pipefail

TEST_NAME="Auth Chat"
TEST_ABBR="ACH"
TEST_NUMBER="59"
TEST_COUNTER=0
TEST_VERSION="1.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

WEB_PORT=15900
MOCK_LLM_PORT=15901
STARTUP_TIMEOUT=20
SHUTDOWN_TIMEOUT=30
SHUTDOWN_ACTIVITY_TIMEOUT=5
HTTP_READY_TIMEOUT=15
READY_TIMEOUT=40

BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"
BASE_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_59_auth_chat.json"
MOCK_LLM_SCRIPT="${SCRIPT_DIR}/lib/mock_llm/server.js"

MOCK_LLM_PID=""
HYDROGEN_PID=""
SQLITE_TEMP=""
CONFIG_TEMP=""
MOCK_LLM_LOG=""
HYDROGEN_LOG=""

HYDROGEN_BIN=""
# shellcheck disable=SC2310 # Failure is non-fatal; we report the missing binary later
find_hydrogen_binary "${PROJECT_DIR}" HYDROGEN_BIN || true

stop_mock_llm() {
    if [[ -z "${MOCK_LLM_PID}" ]]; then
        return 0
    fi
    if ps -p "${MOCK_LLM_PID}" >/dev/null 2>&1; then
        kill -TERM "${MOCK_LLM_PID}" 2>/dev/null || true
        local waited=0
        while (( waited < 20 )) && ps -p "${MOCK_LLM_PID}" >/dev/null 2>&1; do
            sleep 0.1
            waited=$((waited + 1))
        done
        if ps -p "${MOCK_LLM_PID}" >/dev/null 2>&1; then
            kill -KILL "${MOCK_LLM_PID}" 2>/dev/null || true
        fi
    fi
    wait "${MOCK_LLM_PID}" 2>/dev/null || true
    MOCK_LLM_PID=""
}

cleanup() {
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" >/dev/null 2>&1; then
        # shellcheck disable=SC2310 # Cleanup must continue even if shutdown fails
        stop_hydrogen "${HYDROGEN_PID}" "${HYDROGEN_LOG:-/dev/null}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR:-.}" || true
    fi
    # shellcheck disable=SC2310 # Cleanup must continue even if the mock LLM is already gone
    stop_mock_llm || true
    if [[ -n "${SQLITE_TEMP}" ]]; then
        rm -f "${SQLITE_TEMP}" "${SQLITE_TEMP}-wal" "${SQLITE_TEMP}-shm" 2>/dev/null || true
    fi
    if [[ -n "${CONFIG_TEMP}" ]]; then
        rm -f "${CONFIG_TEMP}" 2>/dev/null || true
    fi
}
trap cleanup EXIT

start_mock_llm() {
    if ! command -v node >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "node not available; cannot start mock LLM"
        return 1
    fi
    if [[ ! -f "${MOCK_LLM_SCRIPT}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock LLM script missing: ${MOCK_LLM_SCRIPT}"
        return 1
    fi

    MOCK_LLM_LOG="${LOG_PREFIX}${TIMESTAMP}_mock_llm.log"
    node "${MOCK_LLM_SCRIPT}" "${MOCK_LLM_PORT}" >"${MOCK_LLM_LOG}" 2>&1 &
    MOCK_LLM_PID=$!

    local waited=0
    while (( waited < 50 )); do
        if [[ -s "${MOCK_LLM_LOG}" ]] && "${GREP}" -q "^READY ${MOCK_LLM_PORT}\$" "${MOCK_LLM_LOG}"; then
            return 0
        fi
        if ! ps -p "${MOCK_LLM_PID}" >/dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock LLM exited early; log: ${MOCK_LLM_LOG}"
            MOCK_LLM_PID=""
            return 1
        fi
        sleep 0.1
        waited=$((waited + 1))
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock LLM not READY within 5s"
    return 1
}

prepare_sqlite_with_mock_endpoint() {
    local dest="$1"
    local mock_url="$2"

    if [[ ! -f "${BASELINE_SQLITE}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "baseline sqlite missing: ${BASELINE_SQLITE}"
        return 1
    fi
    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "sqlite3 not available"
        return 1
    fi

    cp "${BASELINE_SQLITE}" "${dest}"
    if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
        cp "${BASELINE_SQLITE}-wal" "${dest}-wal" 2>/dev/null || true
    fi
    if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
        cp "${BASELINE_SQLITE}-shm" "${dest}-shm" 2>/dev/null || true
    fi

    # Rewrite every engine collection endpoint to the local mock LLM.
    # Also normalize provider to openai so health/proxy paths use OpenAI format.
    sqlite3 "${dest}" <<SQL
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

api_request() {
    local method="$1"
    local url="$2"
    local data="${3:-}"
    local out_file="$4"
    local token="${5:-}"

    local args=(-s -X "${method}" -w "%{http_code}" -o "${out_file}" -H "Content-Type: application/json")
    if [[ -n "${token}" ]]; then
        args+=(-H "Authorization: Bearer ${token}")
    fi
    if [[ -n "${data}" ]]; then
        args+=(-d "${data}")
    fi
    curl "${args[@]}" "${url}" 2>/dev/null || echo "000"
}

extract_jwt() {
    local file="$1"
    jq -r '.token // empty' "${file}" 2>/dev/null || true
}

wait_for_http_ready() {
    local base_url="$1"
    local timeout_s="$2"
    local waited=0
    while (( waited < timeout_s )); do
        local code
        code=$(curl -s -o /dev/null -w "%{http_code}" "${base_url}/api/system/health" 2>/dev/null || echo "000")
        if [[ "${code}" == "200" ]]; then
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    return 1
}

wait_for_ready_for_requests() {
    local log_file="$1"
    local timeout_s="$2"
    local waited=0
    while (( waited < timeout_s )); do
        if [[ -f "${log_file}" ]] && "${GREP}" -q "READY FOR REQUESTS" "${log_file}"; then
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    return 1
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Prerequisites"
if [[ -z "${HYDROGEN_BIN}" || ! -x "${HYDROGEN_BIN}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen binary not found"
    exit 1
fi
if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" || -z "${HYDROGEN_DEMO_USER_PASS:-}" || -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Demo credentials not set in environment"
    exit 1
fi
if [[ ! -f "${BASE_CONFIG}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing config ${BASE_CONFIG}"
    exit 1
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Binary and credentials available (${HYDROGEN_BIN})"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start mock LLM"
# shellcheck disable=SC2310 # On failure we report and exit explicitly below
if ! start_mock_llm; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start mock LLM on port ${MOCK_LLM_PORT}"
    exit 1
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Mock LLM READY on ${MOCK_LLM_PORT}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Prepare SQLite + config"
SQLITE_TEMP="${DIAG_TEST_DIR}/auth_chat_demo.sqlite"
CONFIG_TEMP="${DIAG_TEST_DIR}/hydrogen_test_59_auth_chat.runtime.json"
MOCK_URL="http://127.0.0.1:${MOCK_LLM_PORT}/v1/chat/completions"

# shellcheck disable=SC2310 # On failure we report and exit explicitly below
if ! prepare_sqlite_with_mock_endpoint "${SQLITE_TEMP}" "${MOCK_URL}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to prepare SQLite with mock endpoint"
    exit 1
fi

# Materialize config with absolute sqlite path and Chat enabled.
python3 - "${BASE_CONFIG}" "${CONFIG_TEMP}" "${SQLITE_TEMP}" "${WEB_PORT}" <<'PY'
import json, sys
src, dst, sqlite_path, port = sys.argv[1:5]
cfg = json.load(open(src))
cfg["WebServer"]["Port"] = int(port)
for c in cfg.get("Databases", {}).get("Connections", []):
    c["Database"] = sqlite_path
    c["Chat"] = True
json.dump(cfg, open(dst, "w"), indent=2)
print(dst)
PY

print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "SQLite engines retargeted to ${MOCK_URL}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen"
HYDROGEN_LOG="${LOG_PREFIX}${TIMESTAMP}_hydrogen.log"
hydrogen_pid_var="AUTH_CHAT_HYDROGEN_PID"
eval "${hydrogen_pid_var}=''"
# shellcheck disable=SC2310 # On failure we report and exit explicitly below
if ! start_hydrogen_with_pid "${CONFIG_TEMP}" "${HYDROGEN_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${hydrogen_pid_var}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen failed to start"
    exit 1
fi
HYDROGEN_PID=$(eval "echo \${${hydrogen_pid_var}}")
if [[ -z "${HYDROGEN_PID}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen PID not captured"
    exit 1
fi

BASE_URL="http://127.0.0.1:${WEB_PORT}"
# shellcheck disable=SC2310 # On failure we report and exit explicitly below
if ! wait_for_http_ready "${BASE_URL}" "${HTTP_READY_TIMEOUT}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HTTP readiness failed"
    exit 1
fi
# shellcheck disable=SC2310 # On failure we report and exit explicitly below
if ! wait_for_ready_for_requests "${HYDROGEN_LOG}" "${READY_TIMEOUT}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "READY FOR REQUESTS not observed"
    exit 1
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen ready (pid ${HYDROGEN_PID})"

RESP_DIR="${DIAG_TEST_DIR}/responses"
mkdir -p "${RESP_DIR}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Login for JWT"
login_body=$(jq -n \
    --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
    --arg password "${HYDROGEN_DEMO_USER_PASS}" \
    --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
    '{database: "Acuranzo", login_id: $login_id, password: $password, api_key: $api_key, tz: "America/Vancouver"}')
login_file="${RESP_DIR}/login.json"
http_status=$(api_request "POST" "${BASE_URL}/api/auth/login" "${login_body}" "${login_file}")
JWT_TOKEN=$(extract_jwt "${login_file}")
if [[ "${http_status}" != "200" || -z "${JWT_TOKEN}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Login failed HTTP ${http_status}"
    exit 1
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWT acquired"

passed=0
failed=0
record() {
    local ok="$1"
    local msg="$2"
    if [[ "${ok}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${msg}"
        passed=$((passed + 1))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${msg}"
        failed=$((failed + 1))
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat no auth -> 401"
out="${RESP_DIR}/noauth.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" \
    '{"messages":[{"role":"user","content":"hi"}]}' "${out}")
if [[ "${code}" == "401" ]]; then record 0 "401 without Authorization"; else record 1 "expected 401 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat wrong method -> 405"
out="${RESP_DIR}/get.json"
code=$(api_request "GET" "${BASE_URL}/api/conduit/auth_chat" "" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "405" ]]; then record 0 "405 on GET"; else record 1 "expected 405 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat invalid JSON -> 400"
out="${RESP_DIR}/badjson.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" "not-json" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on invalid JSON"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat missing messages -> 400"
out="${RESP_DIR}/nomsg.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" '{"engine":"x"}' "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on missing messages"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat stream not implemented -> 501"
out="${RESP_DIR}/stream.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" \
    '{"messages":[{"role":"user","content":"hi"}],"stream":true}' "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "501" ]]; then record 0 "501 on stream=true"; else record 1 "expected 501 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat success via mock LLM -> 200"
out="${RESP_DIR}/success.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" \
    '{"messages":[{"role":"user","content":"hello blackbox"}],"temperature":0.2,"max_tokens":64}' \
    "${out}" "${JWT_TOKEN}")
success=$(jq -r '.success // false' "${out}" 2>/dev/null || echo false)
content=$(jq -r '.content // empty' "${out}" 2>/dev/null || true)
if [[ "${code}" == "200" && "${success}" == "true" && -n "${content}" ]]; then
    record 0 "200 success content=${content:0:60}"
else
    record 1 "success path failed HTTP ${code} success=${success} body=$(head -c 200 "${out}" 2>/dev/null || true)"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat named engine missing -> 400"
out="${RESP_DIR}/missing_engine.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat" \
    '{"messages":[{"role":"user","content":"hi"}],"engine":"does-not-exist-xyz"}' \
    "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on unknown engine"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream no auth -> 401"
out="${RESP_DIR}/stream_noauth.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat/stream" \
    '{"messages":[{"role":"user","content":"hi"}]}' "${out}")
if [[ "${code}" == "401" ]]; then record 0 "401 without Authorization"; else record 1 "expected 401 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream wrong method -> 405"
out="${RESP_DIR}/stream_get.json"
code=$(api_request "GET" "${BASE_URL}/api/conduit/auth_chat/stream" "" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "405" ]]; then record 0 "405 on GET"; else record 1 "expected 405 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream invalid JSON -> 400"
out="${RESP_DIR}/stream_badjson.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat/stream" "not-json" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on invalid JSON"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream missing messages -> 400"
out="${RESP_DIR}/stream_nomsg.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat/stream" '{"engine":"x"}' "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on missing messages"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream named engine missing -> 400"
out="${RESP_DIR}/stream_missing_engine.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat/stream" \
    '{"messages":[{"role":"user","content":"hi"}],"engine":"does-not-exist-xyz"}' \
    "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on unknown engine"; else record 1 "expected 400 got ${code}"; fi

# Streaming is stubbed: a successful auth+parse+engine path returns 200 with an
# SSE body containing the "not yet implemented" error event.
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat/stream stub SSE -> 200"
out="${RESP_DIR}/stream_stub.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chat/stream" \
    '{"messages":[{"role":"user","content":"hello stream"}],"temperature":0.2,"max_tokens":64}' \
    "${out}" "${JWT_TOKEN}")
body_snip=$(head -c 200 "${out}" 2>/dev/null || true)
if [[ "${code}" == "200" ]] && echo "${body_snip}" | "${GREP}" -q "Streaming not yet implemented"; then
    record 0 "200 SSE stub with not-implemented event"
else
    record 1 "stream stub failed HTTP ${code} body=${body_snip}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats no auth -> 401"
out="${RESP_DIR}/chats_noauth.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    '{"engines":["ChatGPT 4o"],"messages":[{"role":"user","content":"hi"}]}' "${out}")
if [[ "${code}" == "401" ]]; then record 0 "401 without Authorization"; else record 1 "expected 401 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats wrong method -> 405"
out="${RESP_DIR}/chats_get.json"
code=$(api_request "GET" "${BASE_URL}/api/conduit/auth_chats" "" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "405" ]]; then record 0 "405 on GET"; else record 1 "expected 405 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats invalid JSON -> 400"
out="${RESP_DIR}/chats_badjson.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" "not-json" "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on invalid JSON"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats missing engines -> 400"
out="${RESP_DIR}/chats_noengines.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    '{"messages":[{"role":"user","content":"hi"}]}' "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on missing engines"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats empty engines -> 400"
out="${RESP_DIR}/chats_empty_engines.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    '{"engines":[],"messages":[{"role":"user","content":"hi"}]}' "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on empty engines"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats too many engines -> 400"
out="${RESP_DIR}/chats_toomany.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    '{"engines":["a","b","c","d","e","f","g","h","i","j","k"],"messages":[{"role":"user","content":"hi"}]}' \
    "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 on >10 engines"; else record 1 "expected 400 got ${code}"; fi

# Resolve a valid engine name from the prepared demo DB so the fan-out has at
# least one healthy target. Falls back to "ChatGPT 4o" if the lookup fails.
VALID_ENGINE=$(sqlite3 "${SQLITE_TEMP}" \
    "SELECT json_extract(collection,'\$.name') FROM lookups WHERE collection IS NOT NULL AND json_valid(collection) AND json_extract(collection,'\$.endpoint') IS NOT NULL LIMIT 1;" \
    2>/dev/null || true)
VALID_ENGINE="${VALID_ENGINE:-ChatGPT 4o}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats success via mock LLM -> 200"
out="${RESP_DIR}/chats_success.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    "{\"engines\":[\"${VALID_ENGINE}\"],\"messages\":[{\"role\":\"user\",\"content\":\"hello blackbox\"}],\"temperature\":0.2,\"max_tokens\":64}" \
    "${out}" "${JWT_TOKEN}")
success=$(jq -r '.success // false' "${out}" 2>/dev/null || echo false)
req_count=$(jq -r '.engines_requested // 0' "${out}" 2>/dev/null || echo 0)
ok_count=$(jq -r '.engines_succeeded // 0' "${out}" 2>/dev/null || echo 0)
if [[ "${code}" == "200" && "${success}" == "true" && "${req_count}" -ge 1 && "${ok_count}" -ge 1 ]]; then
    record 0 "200 success engines_requested=${req_count} succeeded=${ok_count}"
else
    record 1 "success path failed HTTP ${code} success=${success} body=$(head -c 200 "${out}" 2>/dev/null || true)"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats all unknown engines -> 400"
out="${RESP_DIR}/chats_all_unknown.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    '{"engines":["nope-1","nope-2"],"messages":[{"role":"user","content":"hi"}]}' \
    "${out}" "${JWT_TOKEN}")
if [[ "${code}" == "400" ]]; then record 0 "400 when no valid engines"; else record 1 "expected 400 got ${code}"; fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chats valid+unknown engine -> 200 (unknown dropped)"
out="${RESP_DIR}/chats_partial.json"
code=$(api_request "POST" "${BASE_URL}/api/conduit/auth_chats" \
    "{\"engines\":[\"${VALID_ENGINE}\",\"does-not-exist-xyz\"],\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}" \
    "${out}" "${JWT_TOKEN}")
req_count=$(jq -r '.engines_requested // 0' "${out}" 2>/dev/null || echo 0)
ok_count=$(jq -r '.engines_succeeded // 0' "${out}" 2>/dev/null || echo 0)
# Unknown engine names are silently skipped, so only the valid engine runs.
if [[ "${code}" == "200" && "${req_count}" -eq 2 && "${ok_count}" -eq 1 ]]; then
    record 0 "200 with unknown engine dropped (requested=${req_count} succeeded=${ok_count})"
else
    record 1 "expected 200 with dropped unknown engine got ${code} body=$(head -c 200 "${out}" 2>/dev/null || true)"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary"
if [[ "${failed}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Auth chat blackbox passed (${passed} checks)"
    TEST_RESULT=0
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Auth chat blackbox failed (${failed} failed, ${passed} passed)"
    TEST_RESULT=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${TEST_RESULT}" || exit "${TEST_RESULT}"
