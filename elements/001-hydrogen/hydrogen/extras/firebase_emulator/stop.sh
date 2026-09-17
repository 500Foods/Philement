#!/usr/bin/env bash
# shellcheck disable=SC2310 # predicate helpers (our_pid, pid_running, wait_gone) are used in if/||
#
# Stop the Cloud Firestore emulator only if this extra started it.
#
# CHANGELOG
# 1.0.0 - 2026-09-16 - Initial Firestore emulator stop (FIREBASE Phase 1)
#
# TEST_VERSION
# 1.0.0

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_FILE="${SCRIPT_DIR}/.emulator.pid"
FIREBASE_JSON="${SCRIPT_DIR}/firebase.json"
STOP_WAIT_SECS="${FIREBASE_EMULATOR_STOP_WAIT:-20}"

die() {
    echo "Error: $*" >&2
    exit 1
}

read_config() {
    if [[ -z "${FIREBASE_EMULATOR_HOST:-}" ]]; then
        if command -v jq >/dev/null 2>&1 && [[ -f "${FIREBASE_JSON}" ]]; then
            FIREBASE_EMULATOR_HOST="$(jq -r '.emulators.firestore.host // "127.0.0.1"' "${FIREBASE_JSON}")"
        else
            FIREBASE_EMULATOR_HOST="127.0.0.1"
        fi
    fi
    if [[ -z "${FIREBASE_EMULATOR_PORT:-}" ]]; then
        if command -v jq >/dev/null 2>&1 && [[ -f "${FIREBASE_JSON}" ]]; then
            FIREBASE_EMULATOR_PORT="$(jq -r '.emulators.firestore.port // 8080' "${FIREBASE_JSON}")"
        else
            FIREBASE_EMULATOR_PORT="8080"
        fi
    fi
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

kill_tree() {
    local signal="$1"
    local pid="$2"
    local child
    while IFS= read -r child; do
        [[ -n "${child}" ]] || continue
        kill_tree "${signal}" "${child}"
    done < <(pgrep -P "${pid}" 2>/dev/null || true)
    kill "-${signal}" "${pid}" 2>/dev/null || true
}

wait_gone() {
    local pid="$1"
    local elapsed=0
    while [[ "${elapsed}" -lt "${STOP_WAIT_SECS}" ]]; do
        if ! pid_running "${pid}"; then
            return 0
        fi
        sleep 1
        elapsed=$(( elapsed + 1 ))
    done
    return 1
}

read_config

if ! pid="$(our_pid)"; then
    echo "No ${PID_FILE}; this extra did not start the emulator. Leaving ${HOST}:${PORT} alone."
    exit 0
fi

if ! pid_running "${pid}"; then
    echo "Stale pid ${pid}; removing ${PID_FILE}"
    rm -f "${PID_FILE}"
    exit 0
fi

echo "Stopping Firestore emulator pid ${pid} (process tree)"
kill_tree TERM "${pid}"
if ! wait_gone "${pid}"; then
    echo "SIGTERM wait exceeded; sending SIGKILL to pid ${pid} tree"
    kill_tree KILL "${pid}"
    wait_gone "${pid}" || true
fi

rm -f "${PID_FILE}"
echo "Stopped Firestore emulator (was ${HOST}:${PORT})"
