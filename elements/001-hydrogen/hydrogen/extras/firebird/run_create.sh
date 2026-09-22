#!/usr/bin/env bash
# One-shot Firebird database creation runner (pkexec → create_test_db.sh).
# Forwards dual env vars and optional mode args (both|test|demo|/path.fdb).
# CHANGELOG
# 1.1.0 - 2026-09-22 - Forward FIREBIRD_DB_PATH_TEST/DEMO + CREATE_MODE; pass "$@"
# 1.0.0 - 2026-09-21 - Initial version.
set -euo pipefail
# shellcheck disable=SC2154 # env vars expected from calling shell
pkexec env \
  HYDROGEN_ROOT="${HYDROGEN_ROOT}" \
  FIREBIRD_DB_PATH_TEST="${FIREBIRD_DB_PATH_TEST:-}" \
  FIREBIRD_DB_PATH_DEMO="${FIREBIRD_DB_PATH_DEMO:-}" \
  FIREBIRD_DB_PATH="${FIREBIRD_DB_PATH:-}" \
  FIREBIRD_CREATE_MODE="${FIREBIRD_CREATE_MODE:-}" \
  FIREBIRD_SYSDBA_PASSWORD="${FIREBIRD_SYSDBA_PASSWORD:-}" \
  FIREBIRD=/tmp/firebird \
  bash "${HYDROGEN_ROOT}/extras/firebird/create_test_db.sh" "$@"
