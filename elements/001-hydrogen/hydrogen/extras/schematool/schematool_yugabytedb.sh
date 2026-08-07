#!/usr/bin/env bash
# SchemaTool wrapper — YugabyteDB (Test 40: hydrogen_test_40_yugabytedb.json)
#
# Sets connection from YUGABYTE_DB_* (NOT ACURANZO_DB_* — different host/port).
# Dialect adapter: postgresql (psql). Schema: demo. Design: acuranzo.
#
# CHANGELOG
# 1.1.0 - 2026-08-06 - Pass explicit --engine yugabytedb so env maps to YUGABYTE_DB_*
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; YUGABYTE_DB_* from .zshrc
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${HELIUM_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HELIUM_ROOT}/acuranzo/migrations"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
fi

if [[ -z "${YUGABYTE_DB_HOST:-}" || -z "${YUGABYTE_DB_USER:-}" || -z "${YUGABYTE_DB_NAME:-}" ]]; then
    echo "Error: YUGABYTE_DB_{HOST,USER,NAME} (and PASS) must be set for YugabyteDB" >&2
    exit 1
fi

export SCHEMATOOL_DB_SCHEMA="demo"

# --engine yugabytedb (before alias) selects YUGABYTE_DB_* env in schematool.sh
exec "${SCRIPT_DIR}/schematool.sh" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine yugabytedb \
    --schema demo \
    --host "${YUGABYTE_DB_HOST}" \
    --port "${YUGABYTE_DB_PORT:-5433}" \
    --user "${YUGABYTE_DB_USER}" \
    --database "${YUGABYTE_DB_NAME}" \
    --password-env YUGABYTE_DB_PASS \
    "$@"
