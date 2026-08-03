#!/usr/bin/env bash
# SchemaTool wrapper — IBM Db2 (Test 40: hydrogen_test_40_db2.json)
#
# Sets connection env vars from the Test 40 Db2 config and calls schematool.sh.
# Engine-specific env: HYDROTST_DB_{USER,NAME,PASS}
# Host/Port:           localhost:55555 (hardcoded in Test 40 config)
# Schema:             demo
# Design:             acuranzo
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; HYDROTST_DB_* from .zshrc
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${HELIUM_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HELIUM_ROOT}/acuranzo/migrations"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
fi

export SCHEMATOOL_DB_HOST="localhost"
export SCHEMATOOL_DB_PORT="55555"
export SCHEMATOOL_DB_SCHEMA="demo"

exec "${SCRIPT_DIR}/schematool.sh" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine db2 \
    "$@"
