#!/usr/bin/env bash
# MSSQL Server start — starts the SQL Server 2022 Linux container via Podman
# and waits for port 1433 to accept connections.
#
# Usage:
#   ./start.sh
#
# Requires: Podman, the MSSQL_SA_PASSWORD env var, and at least 2 GiB RAM.
#
# shellcheck shell=bash # Scripts are bash-specific (process substitution, arrays)
# CHANGELOG
# 1.0.0 - 2026-09-29 - Initial version

set -euo pipefail

CONTAINER_NAME="philement-mssql"
IMAGE_NAME="mcr.microsoft.com/mssql/server:2022-latest"
MSSQL_PORT="1433"
MSSQL_HOST="127.0.0.1"
MAX_WAIT=60
WAITED=0

die() {
    echo "Error: $*" >&2
    exit 1
}

# Require MSSQL_SA_PASSWORD
SA_PASSWORD="${MSSQL_SA_PASSWORD:-}"
if [[ -z "${SA_PASSWORD}" ]]; then
    die "MSSQL_SA_PASSWORD env var is required"
fi
export SA_PASSWORD

# Check Podman
if ! command -v podman >/dev/null 2>&1; then
    die "Podman is not installed. Install with: sudo dnf install podman"
fi

# Check memory (SQL Server needs at least 2 GiB)
TOTAL_MEM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
TOTAL_MEM_GB=$((TOTAL_MEM_KB / 1024 / 1024))
if [[ "${TOTAL_MEM_GB}" -lt 4 ]]; then
    echo "Warning: System has ${TOTAL_MEM_GB} GiB RAM. SQL Server requires at least 2 GiB; 4 GiB recommended."
fi

# Check if container already running
if podman ps --format '{{.Names}}' 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    echo "SQL Server container '${CONTAINER_NAME}' is already running."
    exit 0
fi

# Check if container exists (stopped)
if podman ps -a --format '{{.Names}}' 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    echo "Starting existing SQL Server container '${CONTAINER_NAME}'..."
    podman start "${CONTAINER_NAME}"
else
    # Pull the image if not present
    if ! podman image exists "${IMAGE_NAME}" 2>/dev/null; then
        echo "Pulling SQL Server image: ${IMAGE_NAME}"
        podman pull "${IMAGE_NAME}"
    fi

    echo "Creating and starting SQL Server container '${CONTAINER_NAME}'..."
    podman run -d --name "${CONTAINER_NAME}" \
        -e "ACCEPT_EULA=Y" \
        -e "MSSQL_PID=Developer" \
        -e "MSSQL_SA_PASSWORD=${SA_PASSWORD}" \
        -p "${MSSQL_PORT}:1433" \
        --memory=4g \
        --restart=unless-stopped \
        "${IMAGE_NAME}"
fi

echo "Waiting for SQL Server to accept connections on ${MSSQL_HOST}:${MSSQL_PORT}..."

# Wait for the container to be ready using sqlcmd
while [[ "${WAITED}" -lt "${MAX_WAIT}" ]]; do
    if ss -tln 2>/dev/null | grep -q ":${MSSQL_PORT} "; then
        # Verify SQL Server is actually ready (not just port open)
        # shellcheck disable=SC2310 # Error handling intentional
        if podman exec "${CONTAINER_NAME}" /opt/mssql-tools18/bin/sqlcmd \
            -S localhost \
            -U sa \
            -P "${SA_PASSWORD}" \
            -C \
            -Q "SELECT 1" >/dev/null 2>&1; then
            echo "SQL Server is ready on port ${MSSQL_PORT}."
            exit 0
        fi
    fi
    WAITED=$((WAITED + 1))
    sleep 1
done

die "SQL Server did not become ready on port ${MSSQL_PORT} within ${MAX_WAIT}s."
