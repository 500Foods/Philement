#!/usr/bin/env bash
# Firebird SuperServer stop — stops the systemd service via sudo
#
# Usage:
#   sudo ./stop.sh
#
# Requires: sudo, the systemd service started by start.sh
#
# CHANGELOG
# 1.1.0 - 2026-09-20 - Rewritten: uses systemctl stop, requires sudo
# 1.0.0 - 2026-09-18 - Initial version

set -euo pipefail

FIREBIRD_SERVICE="firebird"
FIREBIRD_PORT="3050"

die() {
    echo "Error: $*" >&2
    exit 1
}

# Require sudo
if [[ "${EUID}" -ne 0 ]]; then
    exec sudo "$0" "$@"
fi

echo "Stopping Firebird SuperServer..."

# Resolve the actual service name (same logic as start.sh)
for svc in firebird firebird-superserver; do
    if systemctl list-unit-files "${svc}.service" >/dev/null 2>&1; then
        FIREBIRD_SERVICE="${svc}"
        break
    fi
done

# Check if running
if ! systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    if ss -tln 2>/dev/null | grep -q ":${FIREBIRD_PORT} "; then
        # Port still bound but service not active — try to stop anyway
        :
    else
        echo "Firebird is not running."
        exit 0
    fi
fi

systemctl stop "${FIREBIRD_SERVICE}"

# Verify it stopped
if systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    die "Firebird service '${FIREBIRD_SERVICE}' did not stop. Check 'sudo journalctl -u ${FIREBIRD_SERVICE}' for details."
fi

echo "Firebird stopped."
