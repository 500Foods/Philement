#!/usr/bin/env bash
# SchemaTool wrapper — Firebird (Test 40: hydrogen_test_40_firebird.json)
#
# Sets connection env vars from the Test 40 Firebird config and calls schematool.sh.
# Engine-specific env: FIREBIRD_DB_PATH_DEMO (preferred), FIREBIRD_DB_PATH_TEST,
#   deprecated FIREBIRD_DB_PATH, FIREBIRD_SYSDBA_PASSWORD
# Schema:             (empty - Firebird uses database file isolation)
# Design:             acuranzo
#
# CHANGELOG
# 1.1.2 - 2026-09-22 - Prefer FIREBIRD_DB_PATH_DEMO (fallback TEST, then deprecated singular)
# 1.1.1 - 2026-09-20 - Renamed from CockroachDB wrapper to Firebird; uses isql-fb
# 1.1.0 - 2026-08-22 - Resolve sibling schematool.sh, then HYDROGEN_ROOT
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; FIREBIRD_* from .zshrc
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

export SCHEMATOOL_DB_SCHEMA=""
export SCHEMATOOL_DB_DATABASE="${FIREBIRD_DB_PATH_DEMO:-${FIREBIRD_DB_PATH_TEST:-${FIREBIRD_DB_PATH:-}}}"
export SCHEMATOOL_DB_USER="SYSDBA"
export SCHEMATOOL_DB_PASSWORD_ENV="FIREBIRD_SYSDBA_PASSWORD"

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine firebird \
    "$@"
