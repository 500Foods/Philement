#!/usr/bin/env bash
# Firebird SuperServer start — starts the systemd service and waits for port 3050
#
# Usage:
#   sudo ./start.sh
#
# Requires: sudo, a systemd service named "firebird", and a "firebird" system user.
#           The firebird package from dnf/RPM creates both on Fedora 43.
#
# CHANGELOG
# 1.2.0 - 2026-09-20 - Added lock directory permission fix; security database check
# 1.1.0 - 2026-09-20 - Rewritten: requires sudo + systemd service, no fallbacks
# 1.0.0 - 2026-09-18 - Initial version

set -euo pipefail

FIREBIRD_SERVICE="firebird"
FIREBIRD_PORT="3050"
FIREBIRD_HOST="127.0.0.1"
MAX_WAIT=30
WAITED=0
LOCKDIR="/tmp/firebird"
SECPATH="/var/lib/firebird/secdb/security4.fdb"

die() {
    echo "Error: $*" >&2
    exit 1
}

# Require sudo (firebird service + lock dir need root)
if [[ "${EUID}" -ne 0 ]]; then
    echo "Firebird service management requires sudo privileges."
    echo "Run this script with: sudo $0"
    echo ""
    if sudo -v 2>/dev/null; then
        exec sudo "$0" "$@"
    else
        die "Cannot acquire sudo privileges in this environment.\nAsk your system administrator to run: sudo $0"
    fi
fi

echo "Starting Firebird SuperServer..."

# Check that the firebird user exists
if ! id -u firebird >/dev/null 2>&1; then
    die "The 'firebird' system user does not exist. Install Firebird packages first:\n  sudo dnf install firebird firebird-utils firebird-devel libfbclient2 libfbclient2-devel"
fi

# Check that the systemd service exists
if ! systemctl list-unit-files "${FIREBIRD_SERVICE}.service" >/dev/null 2>&1; then
    for svc in firebird firebird-superserver firebird.service firebird-superserver.service; do
        if systemctl list-unit-files "${svc}.service" >/dev/null 2>&1; then
            FIREBIRD_SERVICE="${svc}"
            break
        fi
    done
    if ! systemctl list-unit-files "${FIREBIRD_SERVICE}.service" >/dev/null 2>&1; then
        die "The '${FIREBIRD_SERVICE}' systemd service was not found. Ensure the firebird package is installed and provides a systemd unit."
    fi
fi

# Check if already running
if systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    echo "Firebird is already running (service: ${FIREBIRD_SERVICE})."
    exit 0
fi

# Fix lock directory permissions (created by fbguard as firebird:firebird 770)
# Other users (e.g. the test runner) need to access it for embedded isql-fb calls
if [[ -d "${LOCKDIR}" ]]; then
    chmod 755 "${LOCKDIR}" 2>/dev/null || true
fi

# Start the service
systemctl start "${FIREBIRD_SERVICE}"

# Verify it started
if ! systemctl is-active --quiet "${FIREBIRD_SERVICE}" 2>/dev/null; then
    die "Firebird service '${FIREBIRD_SERVICE}' failed to start.\nCheck 'journalctl -u ${FIREBIRD_SERVICE}' for details."
fi

# Fix lock directory permissions after service creates it
if [[ -d "${LOCKDIR}" ]]; then
    chmod 755 "${LOCKDIR}" 2>/dev/null || true
fi

# Check security database initialization
if [[ ! -f "${SECPATH}" ]] || [[ ! -s "${SECPATH}" ]]; then
    echo "Warning: Security database missing at ${SECPATH}."
    echo "Run 'sudo ./create_test_db.sh --init-security' to initialize it."
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

die "Firebird did not start listening on port ${FIREBIRD_PORT} within ${MAX_WAIT}s."
