#!/usr/bin/env bash
# SchemaTool wrapper — MSSQL (Test 40: hydrogen_test_40_mssql.json)
#
# Sets connection env vars from the Test 40 MSSQL config and calls schematool.sh.
# Engine-specific env: MSSQL_DB_{HOST,PORT,NAME,USER} and MSSQL_SA_PASSWORD
# Schema:             demoms (inside database MSSQL_DB_NAME, default hydrotst)
# Design:             acuranzo+argent
#
# CHANGELOG
# 1.2.0 - 2026-10-07 - Payload acuranzo+argent in one run
# 1.1.0 - 2026-10-07 - Renamed from schematool_mssql.sh
# 1.0.0 - 2026-10-07 - Test 40 MSSQL convenience wrapper (schema demoms)

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; MSSQL_DB_* from .zshrc
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
if [[ -n "${HELIUM_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HELIUM_ROOT}/acuranzo/migrations"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
fi

export SCHEMATOOL_DB_SCHEMA="demoms"

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo+argent \
    --engine mssql \
    --password-env MSSQL_SA_PASSWORD \
    "$@"
