#!/usr/bin/env bash
# SchemaTool wrapper — SQLite (Test 34: hydrogen_test_34_sqlite.json)
#
# Database:           tests/artifacts/database/sqlite/hydrotst.sqlite
# Schema:             (empty — SQLite has no schema prefix)
# Design:             acuranzo+argent
#
# CHANGELOG
# 1.1.0 - 2026-10-07 - Payload acuranzo+argent in one run
# 1.0.0 - 2026-10-07 - Test 34 wrapper; hydrotst.sqlite

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
    SQLITE_DB="${HYDROGEN_ROOT}/tests/artifacts/database/sqlite/hydrotst.sqlite"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
    SQLITE_DB="${SCRIPT_DIR}/../../../tests/artifacts/database/sqlite/hydrotst.sqlite"
fi

export SCHEMATOOL_DB_SCHEMA=""

for _arg in "$@"; do
    if [[ "${_arg}" == "--help" || "${_arg}" == "-h" ]]; then
        exec "${SCHEMATOOL}" --help
    fi
done

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo+argent \
    --engine sqlite \
    --database "${SQLITE_DB}" \
    --schema "" \
    "$@"
