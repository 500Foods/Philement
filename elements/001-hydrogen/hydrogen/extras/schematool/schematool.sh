#!/usr/bin/env bash

# SchemaTool — Migration Drift Auditor
# Compares on-disk Lua migrations against a running database.
# Outputs: Hydrogen `tables` checklist + fully commented remediation .sql + orphan .mig

# CHANGELOG
# 1.4.0 - 2026-07-29 - Phase 5: SCHEMATOOL_DB_* env, tables polish, operator docs
# 1.3.0 - 2026-07-29 - Phase 4: full audit (compare, remediate .sql/.mig, exit 0/2/3)
# 1.2.0 - 2026-07-29 - Phase 3: --dump-db native client metadata SELECTs (pg/mysql/sqlite/db2)
# 1.1.0 - 2026-07-29 - Phase 2: --emit-expected via Lua get_migration-style extract
# 1.0.0 - 2026-07-29 - Phase 1 CLI skeleton: discover disk migrations, tables report, SQL stub

set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="$(cd "$(dirname "${SCRIPT_PATH}")" && pwd)"
LUA_DIR="${SCRIPT_DIR}/lua"
DB_DIR="${SCRIPT_DIR}/db"

VERSION="1.4.0"

# Showstoppers
if ! command -v tables >/dev/null 2>&1; then
    echo "Error: 'tables' command not found" >&2
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: 'jq' command not found" >&2
    exit 1
fi
if ! command -v lua >/dev/null 2>&1; then
    echo "Error: 'lua' command not found" >&2
    exit 1
fi

TABLES="$(command -v tables)"
JQ="$(command -v jq)"
LUA="$(command -v lua)"
DATE_CMD="$(command -v gdate 2>/dev/null || command -v date)"

print_help() {
    cat <<'EOF'
SchemaTool — Migration Drift Auditor

Usage:
  schematool.sh --migrations DIR --design NAME --engine ENGINE [options]

Required:
  --migrations DIR       Folder with database.lua and design_NNNN.lua
  --design NAME          Design prefix (e.g. acuranzo)
  --engine ENGINE        postgresql|mysql|sqlite|db2 (aliases: mariadb→mysql)

Connection (required for full audit / --dump-db; env fallbacks apply):
  --schema NAME          Schema prefix (empty OK for SQLite)
  --database NAME        Database name or SQLite file path
  --host HOST            DB host
  --port PORT            DB port
  --user USER            DB user
  --password-env VAR     Env var holding password (preferred; never printed)

  Env precedence when flags omitted (first non-empty wins per field):
    1) Engine-specific:
         postgresql → ACURANZO_DB_{HOST,PORT,USER,NAME,PASS}
         mysql      → CANVAS_DB_{HOST,PORT,USER,NAME,PASS}
         db2        → HYDROTST_DB_{USER,NAME,PASS}
    2) Generic SCHEMATOOL_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}
    3) sqlite → --database path (or SCHEMATOOL_DB_NAME as file path)

  Docs: /docs/H/tools/SCHEMATOOL.md

Range / filter:
  --from N               First migration ref (inclusive)
  --to N                 Last migration ref (inclusive)
  --only-failures        Filter checklist to failures
  --dry-disk             Disk discovery only (no DB, no compare)

Output:
  --format tables|json|both   Default: tables
  --out-dir DIR               Write artifacts (layout/data/sql/mig)
  --sql-out PATH              Remediation .sql path
  --no-sql                    Skip remediation .sql
  --mig-out PATH              Orphan capture .mig path (plain text blocks)
  --normalize loose|strict    Default: loose
  --include-ok-comments       Comment OK refs in .sql
  --include-reverse           Also compare reverse payloads (type 1001)
  --include-diagram           Also compare diagram payloads (type 1002)
  --emit-expected [PATH]      Write expected payloads JSON only
  --dump-db [PATH]            Fetch queries metadata JSON only
  -h, --help                  This help
  --version                   Print version

Default path: when connection is available and --dry-disk is not set, run full
audit (discover + expect + dump + compare + tables + .sql/.mig).

Exit codes:
  0  clean   1  hard error   2  audit drift/missing   3  anomalies (orphans etc.)
EOF
}

MIGRATIONS=""
DESIGN=""
ENGINE=""
SCHEMA=""
DATABASE=""
HOST=""
PORT=""
USER_NAME=""
PASSWORD_ENV=""
FROM_REF=""
TO_REF=""
ONLY_FAILURES=0
DRY_DISK=0
FORMAT="tables"
OUT_DIR=""
SQL_OUT=""
NO_SQL=0
MIG_OUT=""
NORMALIZE="loose"
INCLUDE_OK=0
INCLUDE_REVERSE=0
INCLUDE_DIAGRAM=0
EMIT_EXPECTED=0
EMIT_EXPECTED_PATH=""
DUMP_DB=0
DUMP_DB_PATH=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --migrations)
            MIGRATIONS="${2:-}"
            shift 2
            ;;
        --design)
            DESIGN="${2:-}"
            shift 2
            ;;
        --engine)
            ENGINE="${2:-}"
            shift 2
            ;;
        --schema)
            SCHEMA="${2:-}"
            shift 2
            ;;
        --database)
            DATABASE="${2:-}"
            shift 2
            ;;
        --host)
            HOST="${2:-}"
            shift 2
            ;;
        --port)
            PORT="${2:-}"
            shift 2
            ;;
        --user)
            USER_NAME="${2:-}"
            shift 2
            ;;
        --password-env)
            PASSWORD_ENV="${2:-}"
            shift 2
            ;;
        --from)
            FROM_REF="${2:-}"
            shift 2
            ;;
        --to)
            TO_REF="${2:-}"
            shift 2
            ;;
        --only-failures)
            ONLY_FAILURES=1
            shift
            ;;
        --dry-disk)
            DRY_DISK=1
            shift
            ;;
        --format)
            FORMAT="${2:-}"
            shift 2
            ;;
        --out-dir)
            OUT_DIR="${2:-}"
            shift 2
            ;;
        --sql-out)
            SQL_OUT="${2:-}"
            shift 2
            ;;
        --no-sql)
            NO_SQL=1
            shift
            ;;
        --mig-out)
            MIG_OUT="${2:-}"
            shift 2
            ;;
        --normalize)
            NORMALIZE="${2:-}"
            shift 2
            ;;
        --include-ok-comments)
            INCLUDE_OK=1
            shift
            ;;
        --include-reverse)
            INCLUDE_REVERSE=1
            shift
            ;;
        --include-diagram)
            INCLUDE_DIAGRAM=1
            shift
            ;;
        --emit-expected)
            EMIT_EXPECTED=1
            if [[ $# -ge 2 && "${2:-}" != -* ]]; then
                EMIT_EXPECTED_PATH="${2}"
                shift 2
            else
                shift
            fi
            ;;
        --dump-db)
            DUMP_DB=1
            if [[ $# -ge 2 && "${2:-}" != -* ]]; then
                DUMP_DB_PATH="${2}"
                shift 2
            else
                shift
            fi
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        --version)
            echo "schematool ${VERSION}"
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            echo "Try --help" >&2
            exit 1
            ;;
    esac
done

# Engine aliases
case "${ENGINE}" in
    mariadb) ENGINE="mysql" ;;
    cockroachdb|yugabytedb|postgres) ENGINE="postgresql" ;;
    *) ;;
esac

if [[ -z "${MIGRATIONS}" || -z "${DESIGN}" || -z "${ENGINE}" ]]; then
    echo "Error: --migrations, --design, and --engine are required" >&2
    exit 1
fi

if [[ ! -d "${MIGRATIONS}" ]]; then
    echo "Error: migrations directory not found: ${MIGRATIONS}" >&2
    exit 1
fi

if [[ ! -f "${MIGRATIONS}/database.lua" ]]; then
    echo "Error: database.lua not found in ${MIGRATIONS}" >&2
    exit 1
fi

case "${ENGINE}" in
    postgresql|mysql|sqlite|db2) ;;
    *)
        echo "Error: unsupported engine '${ENGINE}' (use postgresql|mysql|sqlite|db2)" >&2
        exit 1
        ;;
esac

case "${FORMAT}" in
    tables|json|both) ;;
    *)
        echo "Error: --format must be tables|json|both" >&2
        exit 1
        ;;
esac

case "${NORMALIZE}" in
    loose|strict) ;;
    *)
        echo "Error: --normalize must be loose|strict" >&2
        exit 1
        ;;
esac

if [[ -n "${FROM_REF}" && ! "${FROM_REF}" =~ ^[0-9]+$ ]]; then
    echo "Error: --from must be an integer" >&2
    exit 1
fi
if [[ -n "${TO_REF}" && ! "${TO_REF}" =~ ^[0-9]+$ ]]; then
    echo "Error: --to must be an integer" >&2
    exit 1
fi

# Env fallbacks for connection (flags win)
# Precedence per field: CLI flag → engine-specific → SCHEMATOOL_DB_*
case "${ENGINE}" in
    postgresql)
        [[ -z "${HOST}" ]] && HOST="${ACURANZO_DB_HOST:-}"
        [[ -z "${PORT}" ]] && PORT="${ACURANZO_DB_PORT:-}"
        [[ -z "${USER_NAME}" ]] && USER_NAME="${ACURANZO_DB_USER:-}"
        [[ -z "${DATABASE}" ]] && DATABASE="${ACURANZO_DB_NAME:-}"
        [[ -z "${PASSWORD_ENV}" && -n "${ACURANZO_DB_PASS:-}" ]] && PASSWORD_ENV="ACURANZO_DB_PASS"
        [[ -z "${SCHEMA}" && -n "${ACURANZO_DB_SCHEMA:-}" ]] && SCHEMA="${ACURANZO_DB_SCHEMA}"
        ;;
    mysql)
        [[ -z "${HOST}" ]] && HOST="${CANVAS_DB_HOST:-}"
        [[ -z "${PORT}" ]] && PORT="${CANVAS_DB_PORT:-}"
        [[ -z "${USER_NAME}" ]] && USER_NAME="${CANVAS_DB_USER:-}"
        [[ -z "${DATABASE}" ]] && DATABASE="${CANVAS_DB_NAME:-}"
        [[ -z "${PASSWORD_ENV}" && -n "${CANVAS_DB_PASS:-}" ]] && PASSWORD_ENV="CANVAS_DB_PASS"
        [[ -z "${SCHEMA}" && -n "${CANVAS_DB_SCHEMA:-}" ]] && SCHEMA="${CANVAS_DB_SCHEMA}"
        ;;
    db2)
        [[ -z "${USER_NAME}" ]] && USER_NAME="${HYDROTST_DB_USER:-}"
        [[ -z "${DATABASE}" ]] && DATABASE="${HYDROTST_DB_NAME:-}"
        [[ -z "${PASSWORD_ENV}" && -n "${HYDROTST_DB_PASS:-}" ]] && PASSWORD_ENV="HYDROTST_DB_PASS"
        [[ -z "${SCHEMA}" && -n "${HYDROTST_DB_SCHEMA:-}" ]] && SCHEMA="${HYDROTST_DB_SCHEMA}"
        ;;
    sqlite)
        [[ -z "${PORT}" ]] && PORT=""
        ;;
    *) ;;
esac

# Generic SCHEMATOOL_DB_* (fills remaining empty fields)
[[ -z "${HOST}" ]] && HOST="${SCHEMATOOL_DB_HOST:-}"
[[ -z "${PORT}" ]] && PORT="${SCHEMATOOL_DB_PORT:-}"
[[ -z "${USER_NAME}" ]] && USER_NAME="${SCHEMATOOL_DB_USER:-}"
[[ -z "${DATABASE}" ]] && DATABASE="${SCHEMATOOL_DB_NAME:-}"
[[ -z "${SCHEMA}" && -n "${SCHEMATOOL_DB_SCHEMA:-}" ]] && SCHEMA="${SCHEMATOOL_DB_SCHEMA}"
if [[ -z "${PASSWORD_ENV}" && -n "${SCHEMATOOL_DB_PASS:-}" ]]; then
    PASSWORD_ENV="SCHEMATOOL_DB_PASS"
fi

# Engine default ports only after all env probes
case "${ENGINE}" in
    postgresql)
        [[ -z "${PORT}" ]] && PORT="5432"
        ;;
    mysql)
        [[ -z "${PORT}" ]] && PORT="3306"
        ;;
    *) ;;
esac

CONN_OK=0
case "${ENGINE}" in
    sqlite)
        if [[ -n "${DATABASE}" && -f "${DATABASE}" ]]; then
            CONN_OK=1
        fi
        ;;
    db2)
        if [[ -n "${DATABASE}" && -n "${USER_NAME}" && -n "${SCHEMA}" ]]; then
            CONN_OK=1
        fi
        ;;
    *)
        if [[ -n "${HOST}" && -n "${USER_NAME}" && -n "${DATABASE}" && -n "${SCHEMA}" ]]; then
            CONN_OK=1
        fi
        ;;
esac

# Mode selection:
# - --emit-expected alone → expected only
# - --dump-db alone → dump only
# - --dry-disk → disk checklist only
# - else if connection ready → full audit
# - else → dry-disk with note
FULL_AUDIT=0
if [[ "${EMIT_EXPECTED}" -eq 1 && "${DUMP_DB}" -eq 0 && "${DRY_DISK}" -eq 0 ]]; then
    : # expected-only path
elif [[ "${DUMP_DB}" -eq 1 && "${EMIT_EXPECTED}" -eq 0 && "${DRY_DISK}" -eq 0 ]]; then
    : # dump-only path (unless combined later)
elif [[ "${DRY_DISK}" -eq 1 ]]; then
    : # disk only
elif [[ "${CONN_OK}" -eq 1 ]]; then
    FULL_AUDIT=1
else
    DRY_DISK=1
    echo "Note: disk discovery only (no usable connection). Pass --database/--schema/… or engine env for full audit." >&2
fi

UTC_STAMP="$("${DATE_CMD}" -u '+%Y%m%dT%H%M%SZ' 2>/dev/null || "${DATE_CMD}" -u '+%Y%m%dT%H%M%SZ')"
DISPLAY_STAMP="$("${DATE_CMD}" '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null || echo "${UTC_STAMP}")"

if [[ -n "${OUT_DIR}" ]]; then
    mkdir -p "${OUT_DIR}"
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/schematool.XXXXXX")"
# shellcheck disable=SC2064 # expand WORK_DIR now for EXIT trap
trap "rm -rf \"${WORK_DIR}\"" EXIT

DATA_JSON="${WORK_DIR}/checklist_data.json"
LAYOUT_JSON="${WORK_DIR}/checklist_layout.json"
EXPECTED_JSON="${WORK_DIR}/expected.json"
DB_JSON="${WORK_DIR}/db_metadata.json"
DISK_JSON="${WORK_DIR}/disk.json"
FINDINGS_JSON="${WORK_DIR}/findings.json"

run_dump_db() {
    local out_file="$1"
    local adapter
    case "${ENGINE}" in
        postgresql) adapter="${DB_DIR}/query_pg.sh" ;;
        mysql) adapter="${DB_DIR}/query_mysql.sh" ;;
        sqlite) adapter="${DB_DIR}/query_sqlite.sh" ;;
        db2) adapter="${DB_DIR}/query_db2.sh" ;;
        *)
            echo "Error: no dump adapter for engine ${ENGINE}" >&2
            exit 1
            ;;
    esac
    if [[ ! -x "${adapter}" && -f "${adapter}" ]]; then
        chmod +x "${adapter}" || true
    fi
    if [[ ! -f "${adapter}" ]]; then
        echo "Error: adapter not found: ${adapter}" >&2
        exit 1
    fi

    if [[ "${ENGINE}" == "sqlite" ]]; then
        if [[ -z "${DATABASE}" ]]; then
            echo "Error: --database (sqlite file path) is required" >&2
            exit 1
        fi
    elif [[ "${ENGINE}" == "db2" ]]; then
        if [[ -z "${DATABASE}" || -z "${USER_NAME}" ]]; then
            echo "Error: --database and --user required for db2 (or HYDROTST_DB_*)" >&2
            exit 1
        fi
        if [[ -z "${SCHEMA}" ]]; then
            echo "Error: --schema is required for db2 (e.g. DEMO)" >&2
            exit 1
        fi
    else
        if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
            echo "Error: --host/--user/--database required (or engine env fallbacks)" >&2
            exit 1
        fi
        if [[ -z "${SCHEMA}" ]]; then
            echo "Error: --schema is required (e.g. demo)" >&2
            exit 1
        fi
    fi

    local dump_args=(
        --host "${HOST}"
        --port "${PORT}"
        --user "${USER_NAME}"
        --database "${DATABASE}"
        --schema "${SCHEMA}"
        --password-env "${PASSWORD_ENV}"
    )
    if [[ -n "${FROM_REF}" ]]; then
        dump_args+=(--from "${FROM_REF}")
    fi
    if [[ -n "${TO_REF}" ]]; then
        dump_args+=(--to "${TO_REF}")
    fi

    set +e
    "${adapter}" "${dump_args[@]}" > "${out_file}"
    local dump_rc=$?
    set -e
    if [[ "${dump_rc}" -ne 0 ]]; then
        echo "Error: database metadata dump failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: dump did not produce a JSON array" >&2
        exit 1
    fi
}

run_expect() {
    local out_file="$1"
    if [[ -z "${SCHEMA}" && "${ENGINE}" != "sqlite" ]]; then
        echo "Error: --schema is required for expected extract (empty only for sqlite)" >&2
        exit 1
    fi
    local schema_arg="${SCHEMA}"
    set +e
    (
        cd "${MIGRATIONS}" || exit 1
        if [[ -n "${FROM_REF}" || -n "${TO_REF}" ]]; then
            "${LUA}" "${LUA_DIR}/schematool_expect.lua" \
                "${MIGRATIONS}" "${ENGINE}" "${DESIGN}" "${schema_arg}" \
                --all "${FROM_REF}" "${TO_REF}"
        else
            "${LUA}" "${LUA_DIR}/schematool_expect.lua" \
                "${MIGRATIONS}" "${ENGINE}" "${DESIGN}" "${schema_arg}" \
                --all
        fi
    ) > "${out_file}"
    local expect_rc=$?
    set -e
    if [[ "${expect_rc}" -ne 0 ]]; then
        echo "Error: expected payload extraction failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array" or type == "object"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: expected extraction did not produce JSON" >&2
        exit 1
    fi
}

run_discover() {
    local out_file="$1"
    if ! "${LUA}" "${LUA_DIR}/schematool_discover.lua" \
        "${MIGRATIONS}" "${DESIGN}" "${FROM_REF}" "${TO_REF}" \
        > "${out_file}"; then
        echo "Error: migration discovery failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: discovery did not produce a JSON array" >&2
        exit 1
    fi
    local row_count
    row_count="$("${JQ}" 'length' "${out_file}")"
    if [[ "${row_count}" -eq 0 ]]; then
        echo "Error: no migrations matched ${DESIGN}_NNNN.lua in ${MIGRATIONS}" >&2
        exit 1
    fi
}

# --- Phase 3 only: dump DB ---
if [[ "${DUMP_DB}" -eq 1 && "${FULL_AUDIT}" -eq 0 ]]; then
    run_dump_db "${DB_JSON}"
    DUMP_OUT="${DUMP_DB_PATH}"
    if [[ -z "${DUMP_OUT}" ]]; then
        if [[ -n "${OUT_DIR}" ]]; then
            DUMP_OUT="${OUT_DIR}/db_metadata.json"
        else
            DUMP_OUT=""
        fi
    fi
    ROW_N="$("${JQ}" 'length' "${DB_JSON}")"
    if [[ -n "${DUMP_OUT}" ]]; then
        dump_parent="$(dirname "${DUMP_OUT}")"
        mkdir -p "${dump_parent}"
        cp "${DB_JSON}" "${DUMP_OUT}"
        echo "DB metadata: ${DUMP_OUT} (${ROW_N} rows)" >&2
    else
        "${JQ}" '.' "${DB_JSON}"
        echo "DB metadata rows: ${ROW_N}" >&2
    fi
    if [[ "${EMIT_EXPECTED}" -eq 0 && "${DRY_DISK}" -eq 0 ]]; then
        exit 0
    fi
fi

# --- Phase 2 only: emit expected ---
if [[ "${EMIT_EXPECTED}" -eq 1 && "${FULL_AUDIT}" -eq 0 ]]; then
    run_expect "${EXPECTED_JSON}"
    EXPECT_OUT="${EMIT_EXPECTED_PATH}"
    if [[ -z "${EXPECT_OUT}" ]]; then
        if [[ -n "${OUT_DIR}" ]]; then
            EXPECT_OUT="${OUT_DIR}/expected_payloads.json"
        else
            EXPECT_OUT=""
        fi
    fi
    if [[ -n "${EXPECT_OUT}" ]]; then
        expect_parent="$(dirname "${EXPECT_OUT}")"
        mkdir -p "${expect_parent}"
        cp "${EXPECTED_JSON}" "${EXPECT_OUT}"
        echo "Expected payloads: ${EXPECT_OUT}" >&2
    else
        "${JQ}" '.' "${EXPECTED_JSON}"
    fi
    if [[ "${DRY_DISK}" -eq 0 && "${DUMP_DB}" -eq 0 ]]; then
        exit 0
    fi
fi

DB_LABEL="${DATABASE:-none}"
SCHEMA_LABEL="${SCHEMA:-.}"
AUDIT_EXIT=0

# --- Phase 4: full audit ---
if [[ "${FULL_AUDIT}" -eq 1 ]]; then
    # Ranged disk set for checklist; full disk set for orphan membership
    DISK_ALL_JSON="${WORK_DIR}/disk_all.json"
    run_discover "${DISK_JSON}"
    if [[ -n "${FROM_REF}" || -n "${TO_REF}" ]]; then
        # Full discovery without range (orphan = in DB, not on any disk file)
        if ! "${LUA}" "${LUA_DIR}/schematool_discover.lua" \
            "${MIGRATIONS}" "${DESIGN}" "" "" \
            > "${DISK_ALL_JSON}"; then
            echo "Error: full migration discovery failed" >&2
            exit 1
        fi
    else
        cp "${DISK_JSON}" "${DISK_ALL_JSON}"
    fi
    run_expect "${EXPECTED_JSON}"
    # Dump all migration metadata (no --from/--to) so orphans outside range are visible
    SAVED_FROM="${FROM_REF}"
    SAVED_TO="${TO_REF}"
    FROM_REF=""
    TO_REF=""
    run_dump_db "${DB_JSON}"
    FROM_REF="${SAVED_FROM}"
    TO_REF="${SAVED_TO}"

    COMPARE_ARGS=(
        --disk "${DISK_JSON}"
        --disk-all "${DISK_ALL_JSON}"
        --expected "${EXPECTED_JSON}"
        --db "${DB_JSON}"
        --normalize "${NORMALIZE}"
        --checklist-out "${DATA_JSON}"
        --findings-out "${FINDINGS_JSON}"
    )
    if [[ "${ONLY_FAILURES}" -eq 1 ]]; then
        COMPARE_ARGS+=(--only-failures)
    fi
    if [[ "${INCLUDE_REVERSE}" -eq 1 ]]; then
        COMPARE_ARGS+=(--include-reverse)
    fi
    if [[ "${INCLUDE_DIAGRAM}" -eq 1 ]]; then
        COMPARE_ARGS+=(--include-diagram)
    fi

    set +e
    "${LUA}" "${LUA_DIR}/schematool_compare.lua" "${COMPARE_ARGS[@]}"
    cmp_rc=$?
    set -e
    if [[ "${cmp_rc}" -ne 0 ]]; then
        echo "Error: compare failed" >&2
        exit 1
    fi

    AUDIT_EXIT="$("${JQ}" -r '.exit_code // 0' "${FINDINGS_JSON}")"
    MIN_REF="$("${JQ}" 'min_by(.ref).ref' "${DATA_JSON}")"
    MAX_REF="$("${JQ}" 'max_by(.ref).ref' "${DATA_JSON}")"
    ROW_COUNT="$("${JQ}" 'length' "${DATA_JSON}")"
    CNT_OK="$("${JQ}" -r '.counts.ok // 0' "${FINDINGS_JSON}")"
    CNT_DRIFT="$("${JQ}" -r '.counts.drift // 0' "${FINDINGS_JSON}")"
    CNT_MLOAD="$("${JQ}" -r '.counts.missing_load // 0' "${FINDINGS_JSON}")"
    CNT_MAPPLY="$("${JQ}" -r '.counts.missing_apply // 0' "${FINDINGS_JSON}")"
    CNT_ORPHAN="$("${JQ}" -r '.counts.orphans // 0' "${FINDINGS_JSON}")"
    DB_MAX_APPLY="$("${JQ}" '[.[] | select(.query_type==1003) | .query_ref] | if length>0 then max else 0 end' "${DB_JSON}")"
    DISK_MAX="$("${JQ}" 'max_by(.ref).ref' "${DISK_JSON}")"

    case "${AUDIT_EXIT}" in
        0) EXIT_LABEL="exit 0 clean" ;;
        2) EXIT_LABEL="exit 2 drift/missing" ;;
        3) EXIT_LABEL="exit 3 anomalies" ;;
        *) EXIT_LABEL="exit ${AUDIT_EXIT}" ;;
    esac

    SUBTITLE="{CYAN}Audit{WHITE} refs ${MIN_REF}-${MAX_REF} ({BOLD}${ROW_COUNT}{RESET}{CYAN} rows) · disk AVAIL ${DISK_MAX} · DB APPLY ${DB_MAX_APPLY}{RESET}"
    # Keep footer plain-ish: tables theme tokens vary; counts + exit are the operator signal
    FOOTER="{CYAN}${DISPLAY_STAMP}{RESET} {RED}———{RESET} ok=${CNT_OK} drift=${CNT_DRIFT} missL=${CNT_MLOAD} missA=${CNT_MAPPLY} orphan=${CNT_ORPHAN} · ${EXIT_LABEL} · norm=${NORMALIZE}"

    # Remediation artifacts
    if [[ "${NO_SQL}" -eq 0 ]]; then
        if [[ -z "${SQL_OUT}" ]]; then
            if [[ -n "${OUT_DIR}" ]]; then
                SQL_OUT="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            else
                SQL_OUT="./schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            fi
        fi
    fi

    MIG_PATH="${MIG_OUT}"
    if [[ -z "${MIG_PATH}" && -n "${OUT_DIR}" ]]; then
        orphan_n="$("${JQ}" '.counts.orphans // 0' "${FINDINGS_JSON}")"
        if [[ "${orphan_n}" -gt 0 ]]; then
            MIG_PATH="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.mig"
        fi
    fi

    if [[ "${NO_SQL}" -eq 0 || -n "${MIG_PATH}" ]]; then
        REM_ARGS=(
            --findings "${FINDINGS_JSON}"
            --design "${DESIGN}"
            --engine "${ENGINE}"
            --schema "${SCHEMA}"
            --database "${DATABASE}"
            --migrations "${MIGRATIONS}"
            --normalize "${NORMALIZE}"
            --version "${VERSION}"
            --stamp "${UTC_STAMP}"
        )
        if [[ "${NO_SQL}" -eq 0 ]]; then
            sql_parent="$(dirname "${SQL_OUT}")"
            mkdir -p "${sql_parent}"
            REM_ARGS+=(--sql-out "${SQL_OUT}")
        else
            # remediate requires --sql-out; write to work dir and discard
            REM_ARGS+=(--sql-out "${WORK_DIR}/discard.sql")
        fi
        if [[ -n "${MIG_PATH}" ]]; then
            mig_parent="$(dirname "${MIG_PATH}")"
            mkdir -p "${mig_parent}"
            REM_ARGS+=(--mig-out "${MIG_PATH}")
        fi
        if [[ "${INCLUDE_OK}" -eq 1 ]]; then
            REM_ARGS+=(--include-ok-comments)
        fi
        set +e
        "${LUA}" "${LUA_DIR}/schematool_remediate.lua" "${REM_ARGS[@]}"
        rem_rc=$?
        set -e
        if [[ "${rem_rc}" -ne 0 ]]; then
            echo "Error: remediation generation failed" >&2
            exit 1
        fi
    fi

    if [[ -n "${OUT_DIR}" ]]; then
        cp "${DATA_JSON}" "${OUT_DIR}/checklist_data.json"
        cp "${FINDINGS_JSON}" "${OUT_DIR}/findings.json"
        cp "${DB_JSON}" "${OUT_DIR}/db_metadata.json"
        cp "${EXPECTED_JSON}" "${OUT_DIR}/expected_payloads.json"
        cp "${DISK_JSON}" "${OUT_DIR}/disk.json"
    fi
else
    # Disk-only (Phase 1)
    run_discover "${DATA_JSON}"
    MIN_REF="$("${JQ}" 'min_by(.ref).ref' "${DATA_JSON}")"
    MAX_REF="$("${JQ}" 'max_by(.ref).ref' "${DATA_JSON}")"
    ROW_COUNT="$("${JQ}" 'length' "${DATA_JSON}")"
    SUBTITLE="{CYAN}Disk discovery{WHITE} refs ${MIN_REF}–${MAX_REF} ({BOLD}${ROW_COUNT}{RESET}{CYAN} files) · no DB audit{RESET}"
    FOOTER="{CYAN}Generated{WHITE} ${DISPLAY_STAMP}{RESET} {RED}———{RESET} {YELLOW}dry-disk{RESET}"

    if [[ "${NO_SQL}" -eq 0 ]]; then
        if [[ -z "${SQL_OUT}" ]]; then
            if [[ -n "${OUT_DIR}" ]]; then
                SQL_OUT="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            else
                SQL_OUT="./schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            fi
        fi
        sql_parent="$(dirname "${SQL_OUT}")"
        mkdir -p "${sql_parent}"
        {
            echo "-- ============================================================================="
            echo "-- SchemaTool remediation (NOT EXECUTED)"
            echo "-- schematool ${VERSION} · dry-disk stub"
            echo "-- design=${DESIGN} engine=${ENGINE} schema=${SCHEMA_LABEL} database=${DB_LABEL}"
            echo "-- migrations=${MIGRATIONS}"
            echo "-- refs=${MIN_REF}-${MAX_REF} count=${ROW_COUNT}"
            echo "-- normalize=${NORMALIZE}"
            echo "-- Generated: ${UTC_STAMP}"
            echo "-- Rule: Uncomment deliberately. Prefer Hydrogen LOAD/APPLY when possible."
            echo "-- Full audit requires DB connection (not --dry-disk)."
            echo "-- ============================================================================="
            echo "--"
        } > "${SQL_OUT}"
        echo "SQL: ${SQL_OUT} (all statements commented)" >&2
    fi

    if [[ -n "${OUT_DIR}" ]]; then
        cp "${DATA_JSON}" "${OUT_DIR}/checklist_data.json"
    fi
fi

# tables binary supports Red and Blue (others warn and fall back to Red)
TABLE_THEME="Red"
if [[ "${FULL_AUDIT}" -eq 1 && "${AUDIT_EXIT}" -eq 0 ]]; then
    TABLE_THEME="Blue"
fi

# Shorten long sqlite paths in title for readability
TITLE_DB="${DB_LABEL}"
if [[ "${ENGINE}" == "sqlite" && ${#TITLE_DB} -gt 48 ]]; then
    TITLE_DB="…/${TITLE_DB##*/}"
fi

cat > "${LAYOUT_JSON}" <<EOF
{
    "title": "{BOLD}{WHITE}SchemaTool{RESET} — ${DESIGN} / ${ENGINE} / ${SCHEMA_LABEL} @ ${TITLE_DB}",
    "subtitle": "${SUBTITLE}",
    "footer": "${FOOTER}",
    "footer_position": "right",
    "theme": "${TABLE_THEME}",
    "columns": [
        {
            "header": "Ref",
            "key": "ref",
            "datatype": "int",
            "justification": "right",
            "summary": "count"
        },
        {
            "header": "File",
            "key": "file",
            "datatype": "text",
            "justification": "left",
            "summary": "count"
        },
        {
            "header": "LOAD",
            "key": "load",
            "datatype": "text",
            "justification": "center"
        },
        {
            "header": "L.match",
            "key": "load_match",
            "datatype": "text",
            "justification": "center"
        },
        {
            "header": "APPLY",
            "key": "apply",
            "datatype": "text",
            "justification": "center"
        },
        {
            "header": "A.match",
            "key": "apply_match",
            "datatype": "text",
            "justification": "center"
        },
        {
            "header": "Notes",
            "key": "notes",
            "datatype": "text",
            "justification": "left"
        }
    ]
}
EOF

if [[ -n "${OUT_DIR}" ]]; then
    cp "${LAYOUT_JSON}" "${OUT_DIR}/checklist_layout.json"
fi

render_tables() {
    "${TABLES}" "${LAYOUT_JSON}" "${DATA_JSON}"
}

case "${FORMAT}" in
    tables)
        render_tables
        ;;
    json)
        "${JQ}" '.' "${DATA_JSON}"
        ;;
    both)
        render_tables
        if [[ -n "${OUT_DIR}" ]]; then
            echo "JSON: ${OUT_DIR}/checklist_data.json" >&2
        else
            echo "--- checklist JSON ---"
            "${JQ}" '.' "${DATA_JSON}"
        fi
        ;;
    *)
        echo "Error: internal format handling failed: ${FORMAT}" >&2
        exit 1
        ;;
esac

exit "${AUDIT_EXIT}"
