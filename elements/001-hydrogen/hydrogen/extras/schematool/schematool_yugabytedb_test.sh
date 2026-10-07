#!/usr/bin/env bash
# SchemaTool wrapper — YugabyteDB (Test 38: hydrogen_test_38_yugabytedb.json)
#
# Sets connection from YUGABYTE_DB_* (NOT ACURANZO_DB_* — different host/port).
# Dialect adapter: postgresql (psql). Schema: test. Design: acuranzo+argent.
#
# CHANGELOG
# 1.1.0 - 2026-10-07 - Payload acuranzo+argent in one run
# 1.0.0 - 2026-10-07 - Test 38 wrapper; schema test

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; YUGABYTE_DB_* from .zshrc
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

for _arg in "$@"; do
    if [[ "${_arg}" == "--help" || "${_arg}" == "-h" ]]; then
        exec "${SCHEMATOOL}" --help
    fi
done

if [[ -z "${YUGABYTE_DB_HOST:-}" || -z "${YUGABYTE_DB_USER:-}" || -z "${YUGABYTE_DB_NAME:-}" ]]; then
    echo "Error: YUGABYTE_DB_{HOST,USER,NAME} (and PASS) must be set for YugabyteDB" >&2
    exit 1
fi

export SCHEMATOOL_DB_SCHEMA="test"

exec "${SCHEMATOOL}" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo+argent \
    --engine yugabytedb \
    --schema test \
    --host "${YUGABYTE_DB_HOST}" \
    --port "${YUGABYTE_DB_PORT:-5433}" \
    --user "${YUGABYTE_DB_USER}" \
    --database "${YUGABYTE_DB_NAME}" \
    --password-env YUGABYTE_DB_PASS \
    "$@"
