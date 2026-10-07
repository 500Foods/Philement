#!/usr/bin/env bash
# SchemaTool wrapper — IBM Db2 (Test 35: hydrogen_test_35_db2.json)
#
# Engine-specific env: HYDROTST_DB_{USER,NAME,PASS}
# Host/Port:           localhost:55555 (hardcoded in the Test 35 config)
# Schema:             test
# Design:             acuranzo+argent
#
# CHANGELOG
# 1.1.0 - 2026-10-07 - Payload acuranzo+argent in one run
# 1.0.0 - 2026-10-07 - Test 35 wrapper; schema test

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; HYDROTST_DB_* from .zshrc
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

export SCHEMATOOL_DB_HOST="localhost"
export SCHEMATOOL_DB_PORT="55555"
export SCHEMATOOL_DB_SCHEMA="test"

for _arg in "$@"; do
    if [[ "${_arg}" == "--help" || "${_arg}" == "-h" ]]; then
        exec "${SCHEMATOOL}" --help
    fi
done

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo+argent \
    --engine db2 \
    --schema test \
    "$@"
