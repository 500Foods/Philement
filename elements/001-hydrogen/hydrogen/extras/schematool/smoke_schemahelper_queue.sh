#!/usr/bin/env bash
# SchemaHelper — Headless smoke test for the queue + packet modules
#
# Runs schemahelper_smoke_queue.lua against checked-in fixture JSON in
# test/fixtures/sample_project/ (no live database required). Verifies:
#   - Queue build totals (total=4, perfect=1, accepted=1, subject=4)
#   - Field-level finding ID (meta:drift:1148:1003:name, not :code)
#   - next_ref = 1291 (max disk ref 1290 + 1)
#   - --ref 1148 collides (design_1148.lua on disk)
#   - --ref 2000 writes a packet directory
#
# Usage:
#   smoke_schemahelper_queue.sh
#   smoke_schemahelper_queue.sh --packet-dir /tmp/custom
#
# CHANGELOG
# 0.5.6 - 2026-08-24 - Initial version: queue totals, finding IDs, next_ref,
#   collision, and packet write verification against sample_project fixtures.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LUA_DIR="${SCRIPT_DIR}/lua"
LUA_SCRIPT="${LUA_DIR}/schemahelper_smoke_queue.lua"
FIXTURE_DIR="${SCRIPT_DIR}/test/fixtures/sample_project"
MIGRATIONS_DIR="${FIXTURE_DIR}/migrations"
STATE_FILE="${FIXTURE_DIR}/schemahelper_acuranzo_sqlite.json"

PACKET_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --packet-dir)
            PACKET_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--packet-dir DIR]"
            echo "  --packet-dir DIR  Where to write test packets (default: mktemp -d)"
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [[ ! -f "${LUA_SCRIPT}" ]]; then
    echo "Error: ${LUA_SCRIPT} not found" >&2
    exit 1
fi

if [[ ! -d "${FIXTURE_DIR}" ]]; then
    echo "Error: fixture directory not found: ${FIXTURE_DIR}" >&2
    exit 1
fi

if [[ ! -f "${STATE_FILE}" ]]; then
    echo "Error: state file not found: ${STATE_FILE}" >&2
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required (schemahelper_queue uses jq to parse findings.json)" >&2
    exit 1
fi

if ! command -v lua >/dev/null 2>&1; then
    echo "Error: lua not found on PATH" >&2
    exit 1
fi

LUA_VER="$(lua -e 'print(_VERSION)' 2>/dev/null || true)"
if [[ "${LUA_VER}" != "Lua 5.5"* ]]; then
    echo "Error: SchemaHelper requires Lua 5.5 (found: ${LUA_VER:-none})" >&2
    exit 1
fi

if [[ -z "${PACKET_DIR}" ]]; then
    PACKET_DIR="$(mktemp -d /tmp/schemahelper_smoke_XXXXXX)"
    CLEANUP_PACKET=1
else
    CLEANUP_PACKET=0
fi

trap 'if [[ "${CLEANUP_PACKET}" -eq 1 ]]; then rm -rf "${PACKET_DIR}"; fi' EXIT

export LUA_PATH="${LUA_DIR}/?.lua;${LUA_PATH:-}"

echo "=== SchemaHelper queue smoke test ==="
echo "  fixtures:   ${FIXTURE_DIR}"
echo "  migrations: ${MIGRATIONS_DIR}"
echo "  state:      ${STATE_FILE}"
echo "  packet dir: ${PACKET_DIR}"
echo ""

set +e
lua "${LUA_SCRIPT}" \
    --out-dir "${FIXTURE_DIR}" \
    --work-dir "${FIXTURE_DIR}" \
    --migrations "${MIGRATIONS_DIR}" \
    --packet-dir "${PACKET_DIR}"
SMOKE_RC=$?
set -e

if [[ "${SMOKE_RC}" -eq 0 ]]; then
    echo ""
    echo "SMOKE PASS"
else
    echo ""
    echo "SMOKE FAIL (exit ${SMOKE_RC})"
fi

exit "${SMOKE_RC}"
