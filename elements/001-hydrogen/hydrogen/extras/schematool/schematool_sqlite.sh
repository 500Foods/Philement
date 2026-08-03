#!/usr/bin/env bash
# SchemaTool wrapper — SQLite (Test 40: hydrogen_test_40_sqlite.json)
#
# Sets connection params from the Test 40 SQLite config and calls schematool.sh.
# Database:           tests/artifacts/database/sqlite/hydrodemo.sqlite
# Schema:             (empty — SQLite has no schema prefix)
# Design:             acuranzo
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HYDROGEN_ROOT may be set by env
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HYDROGEN_ROOT}/../../002-helium/acuranzo/migrations"
    SQLITE_DB="${HYDROGEN_ROOT}/tests/artifacts/database/sqlite/hydrodemo.sqlite"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
    SQLITE_DB="${SCRIPT_DIR}/../../../tests/artifacts/database/sqlite/hydrodemo.sqlite"
fi

export SCHEMATOOL_DB_SCHEMA=""

exec "${SCRIPT_DIR}/schematool.sh" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine sqlite \
    --database "${SQLITE_DB}" \
    --schema "" \
    "$@"
