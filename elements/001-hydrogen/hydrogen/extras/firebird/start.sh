#!/usr/bin/env bash
# Firebird SuperServer start — idempotently starts the service and waits for port 3050
#
# Usage:
#   ./start.sh
#
# Requires: FIREBIRD_SYSDBA_PASSWORD (not used here, but expected by create_test_db.sh)
#
# CHANGELOG
# 1.0.0 - 2026-09-18 - Initial version for Firebird 4.0 on Fedora 43

set -euo pipefail

FIREBIRD_SERVICE="firebird"
FIREBIRD_PORT="3050"
FIREBIRD_HOST="127.0.0.1"
MAX_WAIT=30
WAITED=0

echo "Starting Firebird SuperServer..."

# Check if Firebird is already running
already_running=0
if systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    already_running=1
else
    if ss -tlnp 2>/dev/null | grep -q ":${FIREBIRD_PORT} "; then
        already_running=1
    fi
fi

if [[ "${already_running}" -eq 1 ]]; then
    echo "Firebird is already running on port ${FIREBIRD_PORT}."
else
    if command -v systemctl >/dev/null 2>&1; then
        if systemctl list-unit-files "${FIREBIRD_SERVICE}.service" >/dev/null 2>&1; then
            sudo systemctl start "${FIREBIRD_SERVICE}" 2>/dev/null || systemctl start "${FIREBIRD_SERVICE}" 2>/dev/null || true
        fi
    fi

    # Verify it started, fallback to fbguard if needed
    still_running=0
    if systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
        still_running=1
    else
        if ss -tlnp 2>/dev/null | grep -q ":${FIREBIRD_PORT} "; then
            still_running=1
        fi
    fi

    if [[ "${still_running}" -eq 0 ]]; then
        if command -v fbguard >/dev/null 2>&1; then
            sudo fbguard -daemon 2>/dev/null || fbguard -daemon 2>/dev/null || true
        fi
    fi
fi

echo "Waiting for Firebird to accept connections on ${FIREBIRD_HOST}:${FIREBIRD_PORT}..."

while [[ "${WAITED}" -lt "${MAX_WAIT}" ]]; do
    if ss -tln 2>/dev/null | grep -q ":${FIREBIRD_PORT} "; then
        echo "Firebird is ready on port ${FIREBIRD_PORT}."
        exit 0
    fi
    WAITED=$((WAITED + 1))
    sleep 1
done

echo "Error: Firebird did not start within ${MAX_WAIT}s." >&2
exit 1
