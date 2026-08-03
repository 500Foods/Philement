#!/usr/bin/env bash
# SchemaTool wrapper — CockroachDB (Test 40: hydrogen_test_40_cockroachdb.json)
#
# Sets connection env vars from the Test 40 CockroachDB config and calls schematool.sh.
# Engine-specific env: ACURANZO_DB_{HOST,PORT,NAME,USER,PASS}
# Schema:             democrdb
# Design:             acuranzo
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Created as Test 40 config convenience wrapper

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env; ACURANZO_DB_* from .zshrc
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${HELIUM_ROOT:-}" ]]; then
    MIGRATIONS_DIR="${HELIUM_ROOT}/acuranzo/migrations"
else
    MIGRATIONS_DIR="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
fi

export SCHEMATOOL_DB_SCHEMA="democrdb"

exec "${SCRIPT_DIR}/schematool.sh" \
    --migrations "${MIGRATIONS_DIR}" \
    --design acuranzo \
    --engine postgresql \
    "$@"
