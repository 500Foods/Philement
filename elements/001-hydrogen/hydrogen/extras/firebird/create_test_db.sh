#!/usr/bin/env bash
# Firebird test database creation — creates testfb.fdb (drops existing first)
#
# Usage:
#   ./create_test_db.sh [database.fdb]
#
# Requires: FIREBIRD_SYSDBA_PASSWORD, Firebird SuperServer running on 3050
#
# CHANGELOG
# 1.0.0 - 2026-09-18 - Initial version for Firebird 4.0 on Fedora 43

set -euo pipefail

DB_NAME="${1:-testfb.fdb}"
DB_PATH="/var/lib/firebird/data/${DB_NAME}"
HOST="127.0.0.1"
PORT="3050"
SYSDBA="${FIREBIRD_SYSDBA_PASSWORD:-masterkey}"
SERVER="${HOST}/${PORT}:${DB_PATH}"

if [[ -z "${FIREBIRD_SYSDBA_PASSWORD:-}" ]]; then
    echo "Warning: FIREBIRD_SYSDBA_PASSWORD is not set. Using default 'masterkey'." >&2
fi

echo "Creating Firebird database: ${DB_PATH}"

# Drop existing database if it exists
if isql-fb -user SYSDBA -password "${SYSDBA}" "${SERVER}" -z >/dev/null 2>&1; then
    echo "Database ${DB_NAME} exists. Dropping..."
    isql-fb -user SYSDBA -password "${SYSDBA}" "${SERVER}" -z "DROP DATABASE;" >/dev/null 2>&1 || true
fi

# Create the database
isql-fb -user SYSDBA -password "${SYSDBA}" "${SERVER}" -z "CREATE DATABASE '${DB_PATH}' PAGE_SIZE 4096 DEFAULT CHARACTER SET UTF8;" >/dev/null 2>&1

if isql-fb -user SYSDBA -password "${SYSDBA}" "${SERVER}" -z >/dev/null 2>&1; then
    echo "Database ${DB_NAME} created successfully at ${SERVER}"
else
    echo "Error: Failed to create database ${DB_NAME}" >&2
    exit 1
fi

echo "Database is ready for migrations."
