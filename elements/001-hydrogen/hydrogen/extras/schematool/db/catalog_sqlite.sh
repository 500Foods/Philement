#!/usr/bin/env bash
# SchemaTool SQLite live catalog probe — PRAGMA table_info (targeted)
#
# Usage:
#   catalog_sqlite.sh --database FILE [--tables t1,t2] [--schema ignored]
#
# Prints JSON: { "schema": "", "tables": [ { table, columns[], primary_key[] } ] }
# Never scans product row data.
#
# CHANGELOG
# 1.1.0 - 2026-08-06 - Open with sqlite3 -readonly
# 1.0.0 - 2026-08-02 - Phase 7a SQLite catalog probe

set -euo pipefail

DATABASE=""
TABLES_CSV=""
SCHEMA=""
HOST=""
PORT=""
USER_NAME=""
PASSWORD_ENV=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host) HOST="${2:-}"; shift 2 ;;
        --port) PORT="${2:-}"; shift 2 ;;
        --user) USER_NAME="${2:-}"; shift 2 ;;
        --database) DATABASE="${2:-}"; shift 2 ;;
        --schema) SCHEMA="${2:-}"; shift 2 ;;
        --password-env) PASSWORD_ENV="${2:-}"; shift 2 ;;
        --tables) TABLES_CSV="${2:-}"; shift 2 ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

: "${HOST}" "${PORT}" "${USER_NAME}" "${SCHEMA}" "${PASSWORD_ENV}"

if ! command -v sqlite3 >/dev/null 2>&1; then
    echo "Error: sqlite3 not found" >&2
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi

if [[ -z "${DATABASE}" ]]; then
    echo "Error: --database (sqlite file path) is required" >&2
    exit 1
fi
if [[ ! -f "${DATABASE}" ]]; then
    echo "Error: sqlite database file not found: ${DATABASE}" >&2
    exit 1
fi

# Resolve table list
if [[ -n "${TABLES_CSV}" ]]; then
    IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
else
    # Capture table list then mapfile (avoid SC2312 masking sqlite3 status in process subst)
    local_list="$(sqlite3 -batch -readonly "${DATABASE}" \
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name;")"
    mapfile -t TABLE_ARR <<< "${local_list}"
fi

TABLES_JSON="[]"
for raw in "${TABLE_ARR[@]+"${TABLE_ARR[@]}"}"; do
    t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [[ -z "${t}" ]] && continue
    # Reject odd identifiers
    if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
        echo "Error: invalid table name: ${t}" >&2
        exit 1
    fi

    # PRAGMA table_info: cid|name|type|notnull|dflt_value|pk
    set +e
    PRAGMA_OUT=$(sqlite3 -batch -readonly -separator '|' "${DATABASE}" "PRAGMA table_info('${t}');" 2>&1)
    PRC=$?
    set -e
    if [[ "${PRC}" -ne 0 ]]; then
        echo "Error: PRAGMA table_info failed for ${t}" >&2
        echo "${PRAGMA_OUT}" >&2
        exit 1
    fi

    # Empty pragma → table missing; still emit empty columns so compare can flag
    COLS_JSON="[]"
    PK_JSON="[]"
    if [[ -n "${PRAGMA_OUT}" ]]; then
        while IFS='|' read -r _cid cname ctype cnotnull cdflt cpk; do
            [[ -z "${cname}" ]] && continue
            nullable=true
            if [[ "${cnotnull}" == "1" ]]; then
                nullable=false
            fi
            dtype="$(printf '%s' "${ctype}" | tr '[:upper:]' '[:lower:]')"
            dflt_json="null"
            if [[ -n "${cdflt}" && "${cdflt}" != "" ]]; then
                dflt_json=$(printf '%s' "${cdflt}" | jq -Rs '.')
            fi
            col_obj=$(jq -nc \
                --arg name "${cname}" \
                --arg dt "${dtype}" \
                --argjson nullable "${nullable}" \
                --argjson dflt "${dflt_json}" \
                '{name:$name, data_type:$dt, nullable:$nullable, default:$dflt}')
            COLS_JSON=$(jq -c --argjson c "${col_obj}" '. + [$c]' <<< "${COLS_JSON}")
            if [[ "${cpk}" != "0" && -n "${cpk}" ]]; then
                PK_JSON=$(jq -c --arg n "${cname}" --argjson ord "${cpk}" \
                    '. + [{n:$n, o:($ord|tonumber)}]' <<< "${PK_JSON}")
            fi
        done <<< "${PRAGMA_OUT}"
        PK_JSON=$(jq -c 'sort_by(.o) | map(.n)' <<< "${PK_JSON}")
    fi

    tbl_obj=$(jq -nc \
        --arg table "${t}" \
        --argjson columns "${COLS_JSON}" \
        --argjson pk "${PK_JSON}" \
        '{table:$table, columns:$columns, primary_key:$pk, indexes:[]}')
    TABLES_JSON=$(jq -c --argjson t "${tbl_obj}" '. + [$t]' <<< "${TABLES_JSON}")
done

jq -nc --arg schema "" --argjson tables "${TABLES_JSON}" \
    '{schema:$schema, tables:$tables}'
exit 0
