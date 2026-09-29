#!/usr/bin/env bash
# MSSQL Server stop — stops the SQL Server Podman container if running.
#
# Usage:
#   ./stop.sh
#
# CHANGELOG
# 1.0.0 - 2026-09-29 - Initial version

set -euo pipefail

CONTAINER_NAME="philement-mssql"

die() {
    echo "Error: $*" >&2
    exit 1
}

# Check Podman
if ! command -v podman >/dev/null 2>&1; then
    die "Podman is not installed. Install with: sudo dnf install podman"
fi

# Check if container exists
if ! podman ps -a --format '{{.Names}}' 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    echo "SQL Server container '${CONTAINER_NAME}' does not exist."
    exit 0
fi

# Check if running
if ! podman ps --format '{{.Names}}' 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    echo "SQL Server container '${CONTAINER_NAME}' is not running."
    exit 0
fi

echo "Stopping SQL Server container '${CONTAINER_NAME}'..."
podman stop "${CONTAINER_NAME}"

echo "SQL Server stopped."
