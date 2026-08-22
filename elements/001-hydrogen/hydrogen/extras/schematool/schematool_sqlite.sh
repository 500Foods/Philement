#!/usr/bin/env bash
# SchemaTool wrapper — SQLite (Test 40: hydrogen_test_40_sqlite.json)
#
# Sets connection params from the Test 40 SQLite config and calls schematool.sh.
# Database:           tests/artifacts/database/sqlite/hydrodemo.sqlite
# Schema:             (empty — SQLite has no schema prefix)
# Design:             acuranzo
#
# CHANGELOG
# 1.1.0 - 2026-08-22 - Resolve sibling schematool.sh, then HYDROGEN_ROOT
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HYDROGEN_ROOT / HELIUM_ROOT may be set by env
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -x "${HERE}/schematool.sh" ]]; then
    SCHEMATOOL="${HERE}/schematool.sh"
elif [[ -x "${HYDROGEN_ROOT:-}/extras/schematool/schematool.sh" ]]; then
    SCHEMATOOL="${HYDROGEN_ROOT}/extras/schematool/schematool.sh"
else
    echo "Error: extras/schematool/schematool.sh not found (set HYDROGEN_ROOT)" >&2
    exit 1
fi
SCRIPT_DIR="$(cd "$(dirname "${SCHEMATOOL}")" && pwd)"
if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HYDROGEN_ROOT}/../../002-helium/acuranzo/migrations"
    SQLITE_DB="${HYDROGEN_ROOT}/tests/artifacts/database/sqlite/hydrodemo.sqlite"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
    SQLITE_DB="${SCRIPT_DIR}/../../../tests/artifacts/database/sqlite/hydrodemo.sqlite"
fi

export SCHEMATOOL_DB_SCHEMA=""

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine sqlite \
    --database "${SQLITE_DB}" \
    --schema "" \
    "$@"
