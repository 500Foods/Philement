#!/usr/bin/env bash

# SchemaTool — Migration Drift Auditor
# Compares on-disk Lua migrations against a running database.
# Outputs: Hydrogen `tables` checklist + fully commented remediation .sql + orphan .mig
#
# This is the entry point: help text and parameter handling live here.
# Database/Lua operations, audit orchestration, and rendering are in lib/.
#
# CHANGELOG
# 1.7.1 - 2026-08-06 - Post-table finding details (field diffs + commented remediation SQL)
# 1.7.0 - 2026-08-06 - Engine-request env (YUGABYTE_DB_*); read-only client guards; Test 40 smoke
# 1.6.0 - 2026-08-02 - Row grouping: --group-size N inserts separators every N rows (default 20; 0 = disabled)
# 1.5.2 - 2026-08-02 - Refactored into lib/ modules; no behavioral change
# 1.5.1 - 2026-08-02 - MySQL HEX metadata dump + flat catalog probe (all Test 40 engines)
# 1.5.0 - 2026-08-02 - Phase 7: --catalog live object shape (hybrid C, targeted probes)
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
LIB_DIR="${SCRIPT_DIR}/lib"

VERSION="1.7.1"

# Source library modules (helpers, audit orchestration, rendering)
# shellcheck source=extras/schematool/lib/schematool_init.sh # dependency checks + command lookups
source "${LIB_DIR}/schematool_init.sh"
# shellcheck source=extras/schematool/lib/schematool_runners.sh # db/Lua adapter wrappers
source "${LIB_DIR}/schematool_runners.sh"
# shellcheck source=extras/schematool/lib/schematool_audit.sh # audit mode dispatch + orchestration
source "${LIB_DIR}/schematool_audit.sh"
# shellcheck source=extras/schematool/lib/schematool_render.sh # tables rendering + dispatch
source "${LIB_DIR}/schematool_render.sh"

# Verify required tools are present and resolve command paths
schematool_check_deps
schematool_resolve_commands

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
    1) Requested engine name (before alias) → primary env:
         postgresql|postgres|cockroachdb → ACURANZO_DB_{HOST,PORT,USER,NAME,PASS}
         yugabytedb                      → YUGABYTE_DB_{HOST,PORT,USER,NAME,PASS}
         mysql|mariadb                   → CANVAS_DB_{HOST,PORT,USER,NAME,PASS}
         db2                             → HYDROTST_DB_{USER,NAME,PASS}
    2) Generic SCHEMATOOL_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}
    3) sqlite → --database path (or SCHEMATOOL_DB_NAME as file path)

  IMPORTANT: --engine yugabytedb must NOT fall through to ACURANZO_DB_* (wrong host).
  Prefer Test 40 wrappers (schematool_*.sh) or explicit --host/--password-env for prod.

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
  --catalog                   Live catalog audit (object shape vs applied DDL fold)
  --dump-catalog [PATH]       Fetch live catalog JSON only (optional --only-tables)
  --only-tables a,b           Catalog: probe/compare only these tables (cheap path)
  --group-size N              Insert table separator after every N rows (default: 20; 0 = disabled)
  --no-detail                 Skip post-table finding details (diffs + commented SQL)
  --detail-max-lines N        Max diff lines per field in detail section (default: 80)
  -h, --help                  This help
  --version                   Print version

Default path: when connection is available and --dry-disk is not set, run full
metadata audit (discover + expect + dump + compare + tables + .sql/.mig).
With --catalog, also (or instead if --dump-catalog) run live catalog track:
  fold applied type-1003 forward code → expected shape → targeted probes → compare.
Exit when both tracks run: worst of metadata/catalog (0/2/3); bitfield later.

Exit codes:
  0  clean   1  hard error   2  audit drift/missing   3  anomalies (orphans etc.)
EOF
}

# --- Argument defaults ---
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
CATALOG=0
DUMP_CATALOG=0
DUMP_CATALOG_PATH=""
ONLY_TABLES=""
ROW_GROUP_SIZE=20
NO_DETAIL=0
DETAIL_MAX_LINES=80

# --- Argument parsing ---
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
        --catalog)
            CATALOG=1
            shift
            ;;
        --dump-catalog)
            DUMP_CATALOG=1
            CATALOG=1
            if [[ $# -ge 2 && "${2:-}" != -* ]]; then
                DUMP_CATALOG_PATH="${2}"
                shift 2
            else
                shift
            fi
            ;;
        --only-tables)
            ONLY_TABLES="${2:-}"
            shift 2
            ;;
        --group-size)
            ROW_GROUP_SIZE="${2:-}"
            shift 2
            ;;
        --no-detail)
            NO_DETAIL=1
            shift
            ;;
        --detail-max-lines)
            DETAIL_MAX_LINES="${2:-}"
            shift 2
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

# Preserve requested engine for env selection (aliases collapse dialect only)
ENGINE_REQUESTED="${ENGINE}"

# Engine aliases (dialect adapters)
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
if [[ -n "${ROW_GROUP_SIZE}" && ! "${ROW_GROUP_SIZE}" =~ ^[0-9]+$ ]]; then
    echo "Error: --group-size must be a non-negative integer" >&2
    exit 1
fi
if [[ -n "${DETAIL_MAX_LINES}" && ! "${DETAIL_MAX_LINES}" =~ ^[0-9]+$ ]]; then
    echo "Error: --detail-max-lines must be a non-negative integer" >&2
    exit 1
fi

# Env fallbacks for connection (flags win)
# Precedence per field: CLI flag → requested-engine env → SCHEMATOOL_DB_*
# ENGINE_REQUESTED keeps yugabytedb on YUGABYTE_DB_* (not ACURANZO after alias).
case "${ENGINE_REQUESTED}" in
    yugabytedb)
        [[ -z "${HOST}" ]] && HOST="${YUGABYTE_DB_HOST:-}"
        [[ -z "${PORT}" ]] && PORT="${YUGABYTE_DB_PORT:-}"
        [[ -z "${USER_NAME}" ]] && USER_NAME="${YUGABYTE_DB_USER:-}"
        [[ -z "${DATABASE}" ]] && DATABASE="${YUGABYTE_DB_NAME:-}"
        [[ -z "${PASSWORD_ENV}" && -n "${YUGABYTE_DB_PASS:-}" ]] && PASSWORD_ENV="YUGABYTE_DB_PASS"
        [[ -z "${SCHEMA}" && -n "${YUGABYTE_DB_SCHEMA:-}" ]] && SCHEMA="${YUGABYTE_DB_SCHEMA}"
        ;;
    postgresql|postgres|cockroachdb)
        [[ -z "${HOST}" ]] && HOST="${ACURANZO_DB_HOST:-}"
        [[ -z "${PORT}" ]] && PORT="${ACURANZO_DB_PORT:-}"
        [[ -z "${USER_NAME}" ]] && USER_NAME="${ACURANZO_DB_USER:-}"
        [[ -z "${DATABASE}" ]] && DATABASE="${ACURANZO_DB_NAME:-}"
        [[ -z "${PASSWORD_ENV}" && -n "${ACURANZO_DB_PASS:-}" ]] && PASSWORD_ENV="ACURANZO_DB_PASS"
        [[ -z "${SCHEMA}" && -n "${ACURANZO_DB_SCHEMA:-}" ]] && SCHEMA="${ACURANZO_DB_SCHEMA}"
        ;;
    mysql|mariadb)
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
# - --dump-catalog alone → catalog dump only
# - --catalog → catalog audit (optionally with metadata full audit)
# - --dry-disk → disk checklist only
# - else if connection ready → full metadata audit
# - else → dry-disk with note
FULL_AUDIT=0
CATALOG_AUDIT=0
if [[ "${DUMP_CATALOG}" -eq 1 && "${DUMP_DB}" -eq 0 && "${EMIT_EXPECTED}" -eq 0 && "${DRY_DISK}" -eq 0 ]]; then
    : # catalog-dump-only (handled below)
elif [[ "${EMIT_EXPECTED}" -eq 1 && "${DUMP_DB}" -eq 0 && "${DRY_DISK}" -eq 0 && "${CATALOG}" -eq 0 ]]; then
    : # expected-only path
elif [[ "${DUMP_DB}" -eq 1 && "${EMIT_EXPECTED}" -eq 0 && "${DRY_DISK}" -eq 0 && "${CATALOG}" -eq 0 ]]; then
    : # dump-only path
elif [[ "${DRY_DISK}" -eq 1 ]]; then
    : # disk only
elif [[ "${CONN_OK}" -eq 1 ]]; then
    if [[ "${CATALOG}" -eq 1 ]]; then
        CATALOG_AUDIT=1
        # Also run metadata unless catalog-dump-only was requested
        if [[ "${DUMP_CATALOG}" -eq 0 ]]; then
            FULL_AUDIT=1
        fi
    else
        FULL_AUDIT=1
    fi
else
    if [[ "${CATALOG}" -eq 1 || "${DUMP_CATALOG}" -eq 1 ]]; then
        echo "Error: --catalog requires a usable DB connection" >&2
        exit 1
    fi
    DRY_DISK=1
    echo "Note: disk discovery only (no usable connection). Pass --database/--schema/… or engine env for full audit." >&2
fi

# --catalog without wanting full metadata: allow CATALOG_AUDIT alone when
# ONLY_TABLES is set (cheap one-table path) — skip FULL_AUDIT for speed.
if [[ "${CATALOG_AUDIT}" -eq 1 && -n "${ONLY_TABLES}" && "${DUMP_DB}" -eq 0 && "${EMIT_EXPECTED}" -eq 0 ]]; then
    FULL_AUDIT=0
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
CAT_EXPECTED_JSON="${WORK_DIR}/catalog_expected.json"
CAT_LIVE_JSON="${WORK_DIR}/catalog_live.json"
CAT_DATA_JSON="${WORK_DIR}/catalog_checklist.json"
CAT_FINDINGS_JSON="${WORK_DIR}/catalog_findings.json"

DB_LABEL="${DATABASE:-none}"
SCHEMA_LABEL="${SCHEMA:-.}"
AUDIT_EXIT=0
CATALOG_EXIT=0
RENDER_MODE="metadata"

# --- Early-exit modes (standalone dump/emit; may exit 0 internally) ---
if [[ "${DUMP_DB}" -eq 1 && "${FULL_AUDIT}" -eq 0 && "${CATALOG_AUDIT}" -eq 0 ]]; then
    schematool_run_dump_db_only
fi
if [[ "${DUMP_CATALOG}" -eq 1 && "${CATALOG_AUDIT}" -eq 0 && "${FULL_AUDIT}" -eq 0 ]]; then
    schematool_run_dump_catalog_only
fi
if [[ "${EMIT_EXPECTED}" -eq 1 && "${FULL_AUDIT}" -eq 0 ]]; then
    schematool_run_emit_expected_only
fi

# --- Audit orchestration ---
if [[ "${FULL_AUDIT}" -eq 1 ]]; then
    schematool_run_metadata_audit
elif [[ "${CATALOG_AUDIT}" -eq 0 ]]; then
    schematool_run_dry_disk
fi

if [[ "${CATALOG_AUDIT}" -eq 1 ]]; then
    schematool_run_catalog_audit
fi

# Worst-wins exit across tracks
FINAL_EXIT="${AUDIT_EXIT}"
if [[ "${CATALOG_EXIT}" -gt "${FINAL_EXIT}" ]]; then
    FINAL_EXIT="${CATALOG_EXIT}"
fi

# Render checklist
schematool_render

exit "${FINAL_EXIT}"
