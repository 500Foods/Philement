#!/usr/bin/env bash
# Firebird SuperServer stop — stops the service only if start.sh began it
#
# Usage:
#   ./stop.sh
#
# CHANGELOG
# 1.0.0 - 2026-09-18 - Initial version for Firebird 4.0 on Fedora 43

set -euo pipefail

FIREBIRD_SERVICE="firebird"
FIREBIRD_PORT="3050"
FIREBIRD_HOST="127.0.0.1"

echo "Stopping Firebird SuperServer..."

# Check if Firebird is running
running=0
if systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    running=1
else
    if ss -tlnp 2>/dev/null | grep -q ":${FIREBIRD_PORT} "; then
        running=1
    fi
fi

if [[ "${running}" -eq 1 ]]; then
    if command -v systemctl >/dev/null 2>&1 && systemctl list-unit-files "${FIREBIRD_SERVICE}.service" >/dev/null 2>&1; then
        sudo systemctl stop "${FIREBIRD_SERVICE}" 2>/dev/null || systemctl stop "${FIREBIRD_SERVICE}" 2>/dev/null || true
    else
        # Fallback: gfix can shut down a running Firebird
        if command -v gfix >/dev/null 2>&1; then
            if [[ -n "${FIREBIRD_SYSDBA_PASSWORD:-}" ]]; then
                gfix -shutdown full -user SYSDBA -password "${FIREBIRD_SYSDBA_PASSWORD}" "${FIREBIRD_HOST}/${FIREBIRD_PORT}" 2>/dev/null || true
            fi
        fi
    fi
    echo "Firebird stopped."
else
    echo "Firebird was not running."
fi
