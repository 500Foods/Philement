#!/usr/bin/env bash
# MSSQL Server create test database — creates the hydrotst database and
# the testms (Test 39) and demoms (matrix) schemas.
#
# Usage:
#   ./create_test_db.sh [--drop]
#
# Requires: Running SQL Server container, MSSQL_SA_PASSWORD env var, sqlcmd.
#
# shellcheck shell=bash # Scripts are bash-specific (process substitution, arrays)
# CHANGELOG
# 1.1.0 - 2026-09-30 - Also create schema demoms for Tests 40/43/45/46/47/58
# 1.0.0 - 2026-09-29 - Initial version

set -euo pipefail

CONTAINER_NAME="philement-mssql"
TEST_DB="${MSSQL_TEST_DB:-hydrotst}"
TEST_SCHEMAS=(testms demoms)

die() {
    echo "Error: $*" >&2
    exit 1
}

# shellcheck disable=SC2154 # SA_PASSWORD is validated below
SA_PASSWORD="${MSSQL_SA_PASSWORD:-}"
if [[ -z "${SA_PASSWORD}" ]]; then
    die "MSSQL_SA_PASSWORD env var is required"
fi

DROP_EXISTING=0
if [[ "${1:-}" == "--drop" ]]; then
    DROP_EXISTING=1
fi

# Check container is running
if ! podman ps --format '{{.Names}}' 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    die "SQL Server container '${CONTAINER_NAME}' is not running. Run start.sh first."
fi

# Use sqlcmd inside the container
run_sql() {
    podman exec -i "${CONTAINER_NAME}" /opt/mssql-tools18/bin/sqlcmd \
        -S localhost \
        -U sa \
        -P "${SA_PASSWORD}" \
        -C \
        "$@"
}

echo "Creating database '${TEST_DB}'..."

if [[ "${DROP_EXISTING}" -eq 1 ]]; then
    echo "Dropping existing database '${TEST_DB}' (if it exists)..."
    # shellcheck disable=SC2310 # Error handling intentional
    run_sql -d master -Q "IF DB_ID('${TEST_DB}') IS NOT NULL DROP DATABASE [${TEST_DB}];" 2>/dev/null || true
fi

run_sql -d master -Q "
IF DB_ID('${TEST_DB}') IS NULL
BEGIN
    CREATE DATABASE [${TEST_DB}];
    PRINT 'Database created: ${TEST_DB}';
END
ELSE
BEGIN
    PRINT 'Database already exists: ${TEST_DB}';
END"

for TEST_SCHEMA in "${TEST_SCHEMAS[@]}"; do
    echo "Creating schema '${TEST_SCHEMA}'..."

    run_sql -d "${TEST_DB}" -Q "
IF NOT EXISTS (SELECT * FROM sys.schemas WHERE name = '${TEST_SCHEMA}')
BEGIN
    EXEC('CREATE SCHEMA [${TEST_SCHEMA}]');
    PRINT 'Schema created: ${TEST_SCHEMA}';
END
ELSE
BEGIN
    PRINT 'Schema already exists: ${TEST_SCHEMA}';
END"
done

echo ""
echo "Test database '${TEST_DB}' and schemas ${TEST_SCHEMAS[*]} are ready."
echo "Configure Hydrogen with:"
echo "  Engine: mssql"
echo "  Host: 127.0.0.1"
echo "  Port: 1433"
echo "  Database: ${TEST_DB}"
echo "  User: sa"
echo "  Password: <from MSSQL_SA_PASSWORD env>"
echo "  Schemas: ${TEST_SCHEMAS[*]} (testms is Test 39; demoms is the matrix)"
