#!/usr/bin/env bash

# Test: Authenticated Chat API with local mock LLM (no external credits)
#
# Starts a Node mock OpenAI-compatible server (with SSE streaming support),
# points the SQLite demo DB chat engine endpoints at it, enables Chat on the
# connection, then exercises the REST /api/conduit/auth_chat and
# /api/conduit/auth_chats handlers plus the chat WebSocket (media upload and
# streaming chat) so that the runtime app touches the wschat helper sources:
#   - storage_media.c / storage_hex.c  (media store/retrieve + hex conversion)
#   - proxy_mq.c / proxy_mc.c          (multi-stream chunk queue + CURL callbacks)
#
# Required environment variables:
#   HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY
#   HYDROGEN_DEMO_JWT_KEY, PAYLOAD_KEY, WEBSOCKET_KEY

# FUNCTIONS
# start_mock_llm()
# stop_mock_llm()
# prepare_sqlite_with_mock_endpoint()
# api_request()
# extract_jwt()
# chat_ws_send()
# chat_ws_upload_media()

# CHANGELOG
# 1.0.0 - 2026-07-19 - Initial SQLite + mock LLM blackbox for auth_chat
# 1.1.0 - 2026-07-19 - Reordered cleanup/stop_mock_llm for shellcheck; justified
#                       SC2310 disables; added auth_chats blackbox coverage
# 1.2.0 - 2026-07-19 - Added auth_chat/stream blackbox coverage (SSE stub path)
# 1.3.0 - 2026-07-20 - Added WebSocketServer + chat media-upload and streaming
#                       chat to blackbox-cover storage_media.c, storage_hex.c,
#                       proxy_mq.c and proxy_mc.c; mock LLM gains SSE streaming
# 1.4.0 - 2026-07-23 - Non-stream chat_done + chat_error paths for chat_send.c

set -euo pipefail

TEST_NAME="Auth Chat"
TEST_ABBR="ACH"
TEST_NUMBER="59"
TEST_COUNTER=0
TEST_VERSION="1.4.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

WEB_PORT=15900
MOCK_LLM_PORT=15901
WS_PORT=15902
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

# Build a chat WebSocket URL and a websocat auth header for the hydrogen protocol.
ws_chat_url() {
    echo "ws://127.0.0.1:${WS_PORT}/"
}

# Send a single JSON message to the chat WebSocket and capture the server's
# response frames (up to a timeout). Returns the response on stdout via the
# provided output file. Exits nonzero (not fatal) if websocat is missing or the
# connection fails.
chat_ws_send() {
    local message="$1"
    local out_file="$2"
    local timeout_s="${3:-10}"
    # shellcheck disable=SC2310 # Connection failure is non-fatal; we report below
    if ! command -v websocat >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "websocat not available; skipping WS chat step"
        return 1
    fi
    local ws_url
    ws_url=$(ws_chat_url)
    # websocat line mode requires a trailing newline to flush the frame.
    # --no-close keeps the socket open after stdin EOF so the server's JSON
    # response is captured before the timeout reaps the process.
    printf '%s\n' "${message}" | "${TIMEOUT}" "${timeout_s}" websocat \
        --protocol=hydrogen \
        -H="Authorization: Key ${WEBSOCKET_KEY}" \
        --ping-interval=30 \
        --no-close \
        "${ws_url}" >"${out_file}" 2>&1
    return 0
}

# Upload a small media asset over the chat WebSocket so the runtime exercises
# chat_storage_store_media (storage_media.c) -> chat_storage_binary_to_hex
# (storage_hex.c). Echoes the returned media hash on stdout (empty if failed).
chat_ws_upload_media() {
    local out_file="$1"
    local mime_type="${2:-image/png}"
    # 1x1 transparent PNG, base64url encoded (the server decodes base64url).
    local b64="iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk-M8AAAMCAQDJ_3pUAAAAAElFTkSuQmCC"
    local msg
    # shellcheck disable=SC2310 # Build failure is non-fatal; we report empty hash
    msg=$(jq -cn \
        --arg jwt "Bearer ${JWT_TOKEN}" \
        --arg data "${b64}" \
        --arg mime "${mime_type}" \
        '{type:"media_upload", payload:{jwt:$jwt, data:$data, mime_type:$mime}}') || true
    # shellcheck disable=SC2310 # Send failure is non-fatal; caller checks hash
    chat_ws_send "${msg}" "${out_file}" 10 || true
    jq -r '.media_hash // empty' "${out_file}" 2>/dev/null || true
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
if [[ -z "${WEBSOCKET_KEY:-}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "WEBSOCKET_KEY not set in environment"
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

# Materialize config with absolute sqlite path, Chat enabled, and WebSocketServer port.
python3 - "${BASE_CONFIG}" "${CONFIG_TEMP}" "${SQLITE_TEMP}" "${WEB_PORT}" "${WS_PORT}" <<'PY'
import json, sys
src, dst, sqlite_path, web_port, ws_port = sys.argv[1:6]
cfg = json.load(open(src))
cfg["WebServer"]["Port"] = int(web_port)
cfg["WebSocketServer"]["Port"] = int(ws_port)
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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WS chat media upload -> store_media + binary_to_hex"
# Upload a tiny media asset over the chat WebSocket. The runtime must execute
# chat_storage_store_media (storage_media.c) which calls chat_storage_binary_to_hex
# (storage_hex.c) before issuing its DB query. We assert the runtime actually
# touched the code by grepping the server log for storage_media.c's own marker.
MEDIA_OUT="${RESP_DIR}/ws_media_upload.json"
MEDIA_HASH=$(chat_ws_upload_media "${MEDIA_OUT}")
# shellcheck disable=SC2310 # grep returning 1 (no marker) is handled below
if [[ -f "${HYDROGEN_LOG}" ]] && "${GREP}" -q "Storing media hash" "${HYDROGEN_LOG}"; then
    record 0 "store_media + binary_to_hex executed (hash=${MEDIA_HASH:0:16})"
else
    record 1 "store_media not reached; upload body=$(head -c 200 "${MEDIA_OUT}" 2>/dev/null || true)"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "auth_chat media reference -> retrieve_media + hex_to_binary"
# Send a chat request whose content references a media: asset. The runtime calls
# chat_storage_resolve_media_in_content (storage_media.c) -> chat_storage_retrieve_media
# (storage_media.c) -> chat_storage_hex_to_binary (storage_hex.c). Whether or not
# the asset exists, the helper code is touched; we confirm via the server log.
MEDIA_REF=$(jq -cn --arg hash "${MEDIA_HASH}" '{"messages":[{"role":"user","content":[{"type":"image_url","image_url":{"url":("media:" + $hash)}}]}]}' 2>/dev/null || true)
out="${RESP_DIR}/media_ref.json"
if [[ -n "${MEDIA_REF}" ]]; then
    # shellcheck disable=SC2310 # Request failure is non-fatal; we check the log
    api_request "POST" "${BASE_URL}/api/conduit/auth_chat" "${MEDIA_REF}" "${out}" "${JWT_TOKEN}" >/dev/null 2>&1 || true
    # shellcheck disable=SC2310 # grep returning 1 (no marker) is handled below
    if [[ -f "${HYDROGEN_LOG}" ]] && "${GREP}" -q "QueryRef #072" "${HYDROGEN_LOG}"; then
        record 0 "retrieve_media + hex_to_binary executed"
    else
        record 1 "retrieve_media not reached; body=$(head -c 200 "${out}" 2>/dev/null || true)"
    fi
else
    record 1 "failed to build media reference message"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WS streaming chat -> proxy_mq + proxy_mc"
# A streaming chat request drives chat_proxy_multi_stream_start, which
# initializes the chunk queue (proxy_mq.c: chunk_queue_init) and registers the
# CURL write/debug callbacks (proxy_mc.c: multi_stream_write_callback,
# multi_stream_debug_callback). The mock LLM answers with SSE so the write
# callback actually receives bytes. We assert the runtime touched the code by
# grepping the server log for the multi-stream start marker.
STREAM_OUT="${RESP_DIR}/ws_stream.json"
# shellcheck disable=SC2310 # Build failure is non-fatal; we report below
STREAM_MSG=$(jq -cn \
    --arg jwt "Bearer ${JWT_TOKEN}" \
    --arg engine "${VALID_ENGINE}" \
    '{type:"chat", payload:{jwt:$jwt, engine:$engine, stream:true, messages:[{role:"user",content:"stream me"}]}}' 2>/dev/null) || true
if [[ -n "${STREAM_MSG}" ]]; then
    # shellcheck disable=SC2310 # Send failure is non-fatal; path still runs server-side
    chat_ws_send "${STREAM_MSG}" "${STREAM_OUT}" 12 || true
    # shellcheck disable=SC2310 # grep returning 1 (no marker) is handled below
    if [[ -f "${HYDROGEN_LOG}" ]] && "${GREP}" -q "Multi-stream started" "${HYDROGEN_LOG}"; then
        record 0 "multi-stream proxy executed (proxy_mq + proxy_mc touched)"
    else
        record 1 "multi-stream proxy not reached; body=$(head -c 200 "${STREAM_OUT}" 2>/dev/null || true)"
    fi
else
    record 1 "failed to build streaming chat message"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WS chat_error path -> send_chat_error"
# Missing JWT / empty payload should hit websocket_server_chat_send.c:send_chat_error.
ERR_OUT="${RESP_DIR}/ws_chat_error.json"
# shellcheck disable=SC2310 # Build failure is non-fatal; we report below
ERR_MSG=$(jq -cn '{type:"chat", id:"bb-err-1", payload:{}}' 2>/dev/null) || true
if [[ -n "${ERR_MSG}" ]]; then
    # shellcheck disable=SC2310 # Send failure is non-fatal; response body is checked
    chat_ws_send "${ERR_MSG}" "${ERR_OUT}" 8 || true
    if [[ -f "${ERR_OUT}" ]] && "${GREP}" -q "chat_error" "${ERR_OUT}"; then
        record 0 "WS chat_error response received (send_chat_error)"
    else
        record 1 "chat_error not received; body=$(head -c 200 "${ERR_OUT}" 2>/dev/null || true)"
    fi
else
    record 1 "failed to build chat_error message"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WS non-stream chat -> send_chat_done"
# stream:false uses the legacy proxy path that finishes via send_chat_done
# in websocket_server_chat_send.c (streaming uses multi_curl helpers instead).
DONE_OUT="${RESP_DIR}/ws_chat_done.json"
# shellcheck disable=SC2310 # Build failure is non-fatal; we report below
DONE_MSG=$(jq -cn \
    --arg jwt "Bearer ${JWT_TOKEN}" \
    --arg engine "${VALID_ENGINE}" \
    '{type:"chat", id:"bb-done-1", payload:{jwt:$jwt, engine:$engine, stream:false, messages:[{role:"user",content:"hello nonstream"}]}}' 2>/dev/null) || true
if [[ -n "${DONE_MSG}" ]]; then
    # shellcheck disable=SC2310 # Send failure is non-fatal; response body is checked
    chat_ws_send "${DONE_MSG}" "${DONE_OUT}" 12 || true
    if [[ -f "${DONE_OUT}" ]] && "${GREP}" -q "chat_done" "${DONE_OUT}"; then
        record 0 "WS chat_done response received (send_chat_done)"
    else
        record 1 "chat_done not received; body=$(head -c 200 "${DONE_OUT}" 2>/dev/null || true)"
    fi
else
    record 1 "failed to build non-stream chat message"
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
