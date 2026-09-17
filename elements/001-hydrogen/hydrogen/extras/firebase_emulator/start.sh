#!/usr/bin/env bash
# shellcheck disable=SC2310 # predicate helpers (our_pid, pid_running, emulator_http_up, tcp_open) are used in if/||
#
# Start the Cloud Firestore emulator for Hydrogen (idempotent).
# Does not install UDFs; functions for firebase are in-process in Hydrogen.
#
# CHANGELOG
# 1.0.0 - 2026-09-16 - Initial Firestore emulator start (FIREBASE Phase 1)
#
# TEST_VERSION
# 1.0.0

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_FILE="${SCRIPT_DIR}/.emulator.pid"
LOG_FILE="${SCRIPT_DIR}/firestore-emulator.log"
FIREBASE_BIN="${SCRIPT_DIR}/node_modules/.bin/firebase"
FIREBASE_JSON="${SCRIPT_DIR}/firebase.json"
WAIT_SECS="${FIREBASE_EMULATOR_WAIT:-300}"

die() {
    echo "Error: $*" >&2
    exit 1
}

require_cmd() {
    local name="$1"
    if ! command -v "${name}" >/dev/null 2>&1; then
        die "${name} is not on PATH"
    fi
}

read_config() {
    require_cmd jq
    if [[ ! -f "${FIREBASE_JSON}" ]]; then
        die "missing ${FIREBASE_JSON}"
    fi
    if [[ -z "${FIREBASE_EMULATOR_HOST:-}" ]]; then
        FIREBASE_EMULATOR_HOST="$(jq -r '.emulators.firestore.host // "127.0.0.1"' "${FIREBASE_JSON}")"
    fi
    if [[ -z "${FIREBASE_EMULATOR_PORT:-}" ]]; then
        FIREBASE_EMULATOR_PORT="$(jq -r '.emulators.firestore.port // 8080' "${FIREBASE_JSON}")"
    fi
    PROJECT="${FIREBASE_PROJECT:-hydrodemo}"
    HOST="${FIREBASE_EMULATOR_HOST}"
    PORT="${FIREBASE_EMULATOR_PORT}"
}

our_pid() {
    if [[ ! -f "${PID_FILE}" ]]; then
        return 1
    fi
    local pid
    pid="$(tr -d '[:space:]' < "${PID_FILE}")"
    if [[ -z "${pid}" ]]; then
        return 1
    fi
    echo "${pid}"
}

pid_running() {
    local pid="$1"
    kill -0 "${pid}" 2>/dev/null
}

emulator_http_code() {
    local url="http://${HOST}:${PORT}/v1/projects/${PROJECT}/databases/(default)/documents"
    local code
    code="$(curl -sS -o /dev/null -w '%{http_code}' --connect-timeout 1 --max-time 2 "${url}" 2>/dev/null || true)"
    echo "${code}"
}

emulator_http_up() {
    local code
    code="$(emulator_http_code)"
    [[ "${code}" =~ ^[1-5][0-9][0-9]$ ]]
}

tcp_open() {
    # /dev/tcp is bash; failure is "not listening"
    bash -c "echo >/dev/tcp/${HOST}/${PORT}" 2>/dev/null
}

ensure_deps() {
    require_cmd java
    require_cmd node
    require_cmd npm
    require_cmd curl
    if [[ ! -x "${FIREBASE_BIN}" ]]; then
        echo "Installing pinned firebase-tools into ${SCRIPT_DIR} (once)..."
        (
            cd "${SCRIPT_DIR}"
            npm install --omit=dev --no-fund --no-audit
        )
    fi
    if [[ ! -x "${FIREBASE_BIN}" ]]; then
        die "firebase CLI missing after npm install (${FIREBASE_BIN})"
    fi
}

read_config

if existing_pid="$(our_pid 2>/dev/null)"; then
    if pid_running "${existing_pid}"; then
        if emulator_http_up; then
            echo "Firestore emulator already running (pid ${existing_pid}) on ${HOST}:${PORT}"
            exit 0
        fi
        echo "Emulator pid ${existing_pid} is running but HTTP on ${HOST}:${PORT} is not up. Run stop.sh and retry."
        exit 1
    fi
    echo "Removing stale pid file ${PID_FILE} (pid ${existing_pid} not running)"
    rm -f "${PID_FILE}"
fi

if emulator_http_up || tcp_open; then
    die "port ${HOST}:${PORT} is already in use by another process (not started by this extra)"
fi

ensure_deps
read_config

echo "Starting Firestore emulator project=${PROJECT} ${HOST}:${PORT}"
echo "Log: ${LOG_FILE}"
echo "(First start may download the emulator JAR into ~/.cache/firebase/emulators/)"

: > "${LOG_FILE}"
cd "${SCRIPT_DIR}"
# nohup so SIGHUP from the caller does not take the JVM down. stop.sh walks
# this pid's children (firebase-tools spawns the Java emulator).
nohup "${FIREBASE_BIN}" emulators:start --only firestore --project "${PROJECT}" --non-interactive </dev/null >>"${LOG_FILE}" 2>&1 &
emul_pid=$!
echo "${emul_pid}" > "${PID_FILE}"

elapsed=0
while [[ "${elapsed}" -lt "${WAIT_SECS}" ]]; do
    if emulator_http_up; then
        echo "Firestore emulator listening on ${HOST}:${PORT} (pid ${emul_pid})"
        echo "REST: http://${HOST}:${PORT}/v1/projects/${PROJECT}/databases/(default)/documents/"
        exit 0
    fi
    if ! pid_running "${emul_pid}"; then
        echo "Emulator process ${emul_pid} exited before ${HOST}:${PORT} answered." >&2
        echo "Last log lines:" >&2
        tail -n 40 "${LOG_FILE}" >&2 || true
        rm -f "${PID_FILE}"
        exit 1
    fi
    sleep 1
    elapsed=$(( elapsed + 1 ))
done

echo "Timed out after ${WAIT_SECS}s waiting for ${HOST}:${PORT}" >&2
echo "Last log lines:" >&2
tail -n 40 "${LOG_FILE}" >&2 || true
exit 1
