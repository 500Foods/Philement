#!/usr/bin/env bash
# SchemaTool wrapper — Firebird (Test 37: hydrogen_test_37_firebird.json)
#
# Engine-specific env: FIREBIRD_DB_PATH_TEST, FIREBIRD_SYSDBA_PASSWORD
# Schema:             (empty — Firebird uses the database file)
# Design:             acuranzo+argent
#
# CHANGELOG
# 1.1.0 - 2026-10-07 - Payload acuranzo+argent in one run
# 1.0.0 - 2026-10-07 - Test 37 wrapper; FIREBIRD_DB_PATH_TEST only

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
export SCHEMATOOL_DB_USER="SYSDBA"
export SCHEMATOOL_DB_PASSWORD_ENV="FIREBIRD_SYSDBA_PASSWORD"

for _arg in "$@"; do
    if [[ "${_arg}" == "--help" || "${_arg}" == "-h" ]]; then
        exec "${SCHEMATOOL}" --help
    fi
done

if [[ -z "${FIREBIRD_DB_PATH_TEST:-}" ]]; then
    echo "Error: FIREBIRD_DB_PATH_TEST must be set" >&2
    exit 1
fi

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo+argent \
    --engine firebird \
    --database "${FIREBIRD_DB_PATH_TEST}" \
    --schema "" \
    "$@"
