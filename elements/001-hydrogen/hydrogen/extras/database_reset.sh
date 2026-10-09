#!/usr/bin/env bash
# shellcheck disable=SC2310,SC2312 # Each engine reports its own status and the loop continues.
# database_reset.sh — empty the build schemas used by migration tests 32-39
# and auth test 40.
#
# The databases, schema names, and database files stay. The build expects
# them to exist. What goes away is the tables, views, and routines inside
# the migration schema, so the next AutoMigration starts from empty instead
# of resuming at the last applied migration.
#
#   tests 32-39   schema test   (MSSQL testms, SQLite hydrotst.sqlite,
#                 Firebird hydrogen_test.fdb)
#   test 40       schema demo   (MSSQL demoms, SQLite hydrodemo.sqlite,
#                 Firebird hydrogen_demo.fdb)
#
# Connection settings come from tests/configs/hydrogen_test_3*.json and
# hydrogen_test_40_*.json. Passwords stay in the environment those files
# already reference. Nothing here drops a database.
#
# Implementation note: the per-engine reset logic lives in extras/database_reset_lib/
# (common.sh, mysql.sh, psql.sh, db2.sh, sqlite.sh, firebird.sh, mssql.sh,
# and dispatch.sh), sourced below. This file is the entry point and orchestrator.
#
# CHANGELOG
# 1.0.1 - 2026-10-08 - Ignore ~/.my.cnf and ~/.sqliterc when counting and resetting
# 1.0.1 - 2026-10-09 - Refactored into extras/database_reset_lib/ modules
# TEST_VERSION: 1.0.1

set -euo pipefail
set +H
umask 077

GROUP_FILTER=""
ENGINE_FILTER=""
DRY_RUN=0
ASSUME_YES=0
FAILS=0
WORK_DIR=""
DB2_OPEN=0
DB2_COUNT=""
REACHABLE=()

TARGETS=(
    "test|32|postgresql|PostgreSQL|hydrogen_test_32_postgres.json"
    "test|33|mysql|MySQL|hydrogen_test_33_mysql.json"
    "test|34|sqlite|SQLite|hydrogen_test_34_sqlite.json"
    "test|35|db2|DB2|hydrogen_test_35_db2.json"
    "test|36|mariadb|MariaDB|hydrogen_test_36_mariadb.json"
    "test|37|firebird|Firebird|hydrogen_test_37_firebird.json"
    "test|38|yugabytedb|YugabyteDB|hydrogen_test_38_yugabytedb.json"
    "test|39|mssql|MSSQL|hydrogen_test_39_mssql.json"
    "demo|40|postgresql|PostgreSQL|hydrogen_test_40_postgres.json"
    "demo|40|mysql|MySQL|hydrogen_test_40_mysql.json"
    "demo|40|sqlite|SQLite|hydrogen_test_40_sqlite.json"
    "demo|40|db2|DB2|hydrogen_test_40_db2.json"
    "demo|40|mariadb|MariaDB|hydrogen_test_40_mariadb.json"
    "demo|40|firebird|Firebird|hydrogen_test_40_firebird.json"
    "demo|40|yugabytedb|YugabyteDB|hydrogen_test_40_yugabytedb.json"
    "demo|40|mssql|MSSQL|hydrogen_test_40_mssql.json"
)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source library modules
# shellcheck source=extras/database_reset_lib/common.sh # shared helpers
source "${SCRIPT_DIR}/database_reset_lib/common.sh"
# shellcheck source=extras/database_reset_lib/mysql.sh # MySQL/MariaDB backend
source "${SCRIPT_DIR}/database_reset_lib/mysql.sh"
# shellcheck source=extras/database_reset_lib/psql.sh # PostgreSQL/YugabyteDB backend
source "${SCRIPT_DIR}/database_reset_lib/psql.sh"
# shellcheck source=extras/database_reset_lib/db2.sh # DB2 backend
source "${SCRIPT_DIR}/database_reset_lib/db2.sh"
# shellcheck source=extras/database_reset_lib/sqlite.sh # SQLite backend
source "${SCRIPT_DIR}/database_reset_lib/sqlite.sh"
# shellcheck source=extras/database_reset_lib/firebird.sh # Firebird backend
source "${SCRIPT_DIR}/database_reset_lib/firebird.sh"
# shellcheck source=extras/database_reset_lib/mssql.sh # MSSQL backend
source "${SCRIPT_DIR}/database_reset_lib/mssql.sh"
# shellcheck source=extras/database_reset_lib/dispatch.sh # count/reset dispatch
source "${SCRIPT_DIR}/database_reset_lib/dispatch.sh"

usage() {
    cat <<'EOF'
Usage: database_reset.sh [--dry-run] [--yes] [--group test|demo] [--engine name]

Empty the build schemas used by tests 32-39 (test) and test 40 (demo).
Tables, views, and routines inside those schemas are removed. The
databases and database files are not dropped.

  --dry-run       Show each target and its table/view count. Change nothing.
  --yes           Skip the RESET confirmation.
  --group test    Only the eight migration-test schemas (tests 32-39).
  --group demo    Only the eight test 40 schemas.
  --engine name   One engine: postgresql, mysql, sqlite, db2, mariadb,
                  firebird, yugabytedb, mssql.

Credentials are the environment variables already referenced by the
test JSON configs. Stop Hydrogen before running this so open transactions
do not block the drops.
EOF
}

# shellcheck disable=SC2329 # EXIT trap uses cleanup() defined in common.sh
cleanup() {
    if [[ "${DB2_OPEN}" -eq 1 ]] && command -v db2 >/dev/null 2>&1; then
        db2 connect reset >/dev/null 2>&1 || true
        DB2_OPEN=0
    fi
    if [[ -n "${WORK_DIR}" && -d "${WORK_DIR}" ]]; then
        rm -rf "${WORK_DIR}"
        WORK_DIR=""
    fi
}
trap cleanup EXIT

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --help|-h)
                usage
                exit 0
                ;;
            --dry-run)
                DRY_RUN=1
                shift
                ;;
            --yes)
                ASSUME_YES=1
                shift
                ;;
            --group)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --group needs test or demo"
                    exit 2
                fi
                GROUP_FILTER="${2,,}"
                if [[ "${GROUP_FILTER}" != "test" && "${GROUP_FILTER}" != "demo" ]]; then
                    warn "Error: --group must be test or demo"
                    exit 2
                fi
                shift 2
                ;;
            --engine)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --engine needs a name"
                    exit 2
                fi
                ENGINE_FILTER="$(normalize_engine "${2}")" || exit 2
                shift 2
                ;;
            *)
                warn "Error: unknown argument ${1}"
                usage >&2
                exit 2
                ;;
        esac
    done
}

confirm_reset() {
    local answer
    if [[ "${ASSUME_YES}" -eq 1 ]]; then
        return 0
    fi
    printf '\nType RESET to remove objects from these build schemas: '
    if [[ -r /dev/tty ]]; then
        IFS= read -r answer < /dev/tty || true
    else
        warn "No terminal. Re-run with --yes to skip the prompt."
        return 1
    fi
    if [[ "${answer}" != "RESET" ]]; then
        printf 'Cancelled.\n'
        return 1
    fi
    return 0
}

main() {
    local row group number engine label config path where before after rc
    local selected=0
    parse_args "$@"
    if ! command -v jq >/dev/null 2>&1; then
        warn "Error: jq not found"
        exit 2
    fi
    WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/database_reset.XXXXXX")" || {
        warn "Error: could not create a work directory"
        exit 2
    }
    if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
        HYDROGEN_ROOT="$(cd "${HYDROGEN_ROOT}" && pwd)"
    else
        HYDROGEN_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
    fi
    printf 'Build schemas for tests 32-39 (test) and test 40 (demo).\n'
    printf 'Databases and database files are kept. Objects inside the schema are removed.\n\n'

    for row in "${TARGETS[@]}"; do
        if ! selected_row "${row}"; then
            continue
        fi
        IFS='|' read -r group number engine label config <<< "${row}"
        path="${HYDROGEN_ROOT}/tests/configs/${config}"
        if ! load_connection "${path}"; then
            report "FAILED" "${group}" "${number}" "${label}" "could not read ${config}"
            FAILS=$((FAILS + 1))
            continue
        fi
        if ! validate_target "${group}" "${engine}"; then
            report "FAILED" "${group}" "${number}" "${label}" "refusing this target"
            FAILS=$((FAILS + 1))
            continue
        fi
        where="$(location_text "${engine}")"
        selected=$((selected + 1))
        set +e
        before="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 || ! "${before}" =~ ^[0-9]+$ ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "COUNT" "${group}" "${number}" "${label}" "${where}  tables/views ${before}"
        if [[ "${DRY_RUN}" -eq 1 ]]; then
            continue
        fi
        REACHABLE+=("${row}")
    done

    if [[ "${selected}" -eq 0 ]]; then
        warn "Error: no targets matched"
        exit 2
    fi
    if [[ "${DRY_RUN}" -eq 1 ]]; then
        printf '\nDry run. No changes made.\n'
        if [[ "${FAILS}" -ne 0 ]]; then
            exit 1
        fi
        exit 0
    fi
    if [[ "${#REACHABLE[@]}" -eq 0 ]]; then
        warn "Nothing reachable to reset."
        exit 1
    fi
    if ! confirm_reset; then
        if [[ "${ASSUME_YES}" -eq 0 ]]; then
            exit 0
        fi
        exit 1
    fi

    printf '\n'
    for row in "${REACHABLE[@]}"; do
        IFS='|' read -r group number engine label config <<< "${row}"
        path="${HYDROGEN_ROOT}/tests/configs/${config}"
        if ! load_connection "${path}"; then
            report "FAILED" "${group}" "${number}" "${label}" "could not re-read ${config}"
            FAILS=$((FAILS + 1))
            continue
        fi
        if ! validate_target "${group}" "${engine}"; then
            report "FAILED" "${group}" "${number}" "${label}" "refusing this target"
            FAILS=$((FAILS + 1))
            continue
        fi
        if [[ "${engine}" == "db2" ]]; then
            if ! ensure_db2; then
                report "FAILED" "${group}" "${number}" "${label}" "db2 client not found"
                FAILS=$((FAILS + 1))
                continue
            fi
        fi
        where="$(location_text "${engine}")"
        set +e
        before="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "RESET" "${group}" "${number}" "${label}" "${where}  was ${before}"
        set +e
        reset_target "${engine}"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        set +e
        after="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 || "${after}" != "0" ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}  still ${after:-unknown}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "OK" "${group}" "${number}" "${label}" "${where}  ${before} -> 0"
    done

    printf '\n'
    if [[ "${FAILS}" -ne 0 ]]; then
        printf '%s target(s) failed.\n' "${FAILS}"
        exit 1
    fi
    printf 'Reset complete. Databases were left in place.\n'
    exit 0
}

main "$@"
