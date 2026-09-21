#!/usr/bin/env bash
# One-shot Firebird database creation runner.
# CHANGELOG
# 1.0.0 - 2026-09-21 - Initial version.
set -euo pipefail
# shellcheck disable=SC2154 # env vars expected from calling shell
pkexec env HYDROGEN_ROOT="${HYDROGEN_ROOT}" FIREBIRD_DB_PATH="${FIREBIRD_DB_PATH}" FIREBIRD_SYSDBA_PASSWORD="${FIREBIRD_SYSDBA_PASSWORD}" FIREBIRD=/tmp/firebird bash "${HYDROGEN_ROOT}/extras/firebird/create_test_db.sh"
