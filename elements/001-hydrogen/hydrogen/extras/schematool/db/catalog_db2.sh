#!/usr/bin/env bash
# SchemaTool DB2 live catalog probe — SYSCAT.COLUMNS (filtered)
#
# CHANGELOG
# 1.1.0 - 2026-08-20 - Tab/HEX export + jq assemble (drop python3)
# 1.0.0 - 2026-08-02 - Phase 7a DB2 catalog probe

set -euo pipefail

# shellcheck source=extras/schematool/db/common.sh # HEX decode helper
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

HOST=""
PORT=""
USER_NAME=""
DATABASE=""
SCHEMA=""
PASSWORD_ENV=""
TABLES_CSV=""

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

: "${HOST}" "${PORT}"

if ! command -v db2 >/dev/null 2>&1; then
    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        # shellcheck disable=SC1091 # host-specific DB2 profile path not in repo
        . /home/db2inst1/sqllib/db2profile
    fi
fi
if ! command -v db2 >/dev/null 2>&1; then
    echo "Error: db2 client not found" >&2
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi
# shellcheck disable=SC2310 # missing xxd is a hard adapter error
if ! schematool_require_xxd; then
    exit 1
fi

if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for db2" >&2
    exit 1
fi
if [[ -z "${SCHEMA}" || "${SCHEMA}" == "." ]]; then
    echo "Error: --schema is required for db2 catalog probe" >&2
    exit 1
fi

PASS=""
if [[ -n "${PASSWORD_ENV}" ]]; then
    if [[ -z "${!PASSWORD_ENV+x}" ]]; then
        echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
        exit 1
    fi
    PASS="${!PASSWORD_ENV}"
fi

SCHEMA_U="${SCHEMA^^}"
SCHEMA_SQL="${SCHEMA_U//\'/\'\'}"

TABLE_FILTER=""
if [[ -n "${TABLES_CSV}" ]]; then
    in_list=""
    IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
    for raw in "${TABLE_ARR[@]}"; do
        t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [[ -z "${t}" ]] && continue
        if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
            echo "Error: invalid table name: ${t}" >&2
            exit 1
        fi
        tu="${t^^}"
        if [[ -n "${in_list}" ]]; then
            in_list="${in_list},"
        fi
        in_list="${in_list}'${tu}'"
    done
    if [[ -z "${in_list}" ]]; then
        echo "{\"schema\":\"${SCHEMA_U}\",\"tables\":[]}"
        exit 0
    fi
    TABLE_FILTER="AND TABNAME IN (${in_list})"
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_cat_db2.XXXXXX")
# shellcheck disable=SC2064 # expand WORK now so EXIT trap removes this temp dir
trap "rm -rf \"${WORK}\"" EXIT

CONN_SCRIPT="${WORK}/connect.sql"
PASS_SQL="${PASS//\'/\'\'}"
{
    echo "CONNECT TO ${DATABASE} USER ${USER_NAME} USING '${PASS_SQL}';"
} > "${CONN_SCRIPT}"

# Tab-delimited HEX default so commas/quotes in DEFAULT cannot break fields
EXPORT_FILE="${WORK}/cols.del"
SQL_EXPORT="EXPORT TO '${EXPORT_FILE}' OF DEL MODIFIED BY COLDEL0x09 NOCHARDEL
SELECT TABNAME, COLNAME, TYPENAME, NULLS,
       HEX(CAST(COALESCE(DEFAULT, '') AS VARCHAR(512))),
       COALESCE(KEYSEQ, 0), COLNO
FROM SYSCAT.COLUMNS
WHERE TABSCHEMA = '${SCHEMA_SQL}'
  ${TABLE_FILTER}
ORDER BY TABNAME, COLNO;"

set +e
db2 -tvf "${CONN_SCRIPT}" >"${WORK}/conn.out" 2>&1
db2 -tvf /dev/stdin >"${WORK}/export.out" 2>&1 <<EOF
${SQL_EXPORT}
EOF
RC=$?
db2 connect reset >/dev/null 2>&1 || true
rm -f "${CONN_SCRIPT}"
set -e

if [[ "${RC}" -ne 0 && ! -f "${EXPORT_FILE}" ]]; then
    SAFE=$(sed "s/${PASS}/***/g" "${WORK}/export.out" 2>/dev/null || true)
    echo "Error: db2 catalog export failed" >&2
    echo "${SAFE}" >&2
    exit 1
fi

if [[ ! -f "${EXPORT_FILE}" || ! -s "${EXPORT_FILE}" ]]; then
    jq -nc --arg schema "${SCHEMA_U}" '{schema:$schema, tables:[]}'
    exit 0
fi

NDJSON="${WORK}/cols.ndjson"
: > "${NDJSON}"
idx=0
while IFS=$'\t' read -r tab col typ nulls def_h keyseq colno || [[ -n "${tab:-}" ]]; do
    [[ -z "${tab:-}" ]] && continue
    colno="${colno//$'\r'/}"
    pk_n="${keyseq:-0}"
    pk_n="${pk_n// /}"
    if [[ ! "${pk_n}" =~ ^[0-9]+$ ]]; then
        pk_n=0
    fi
    ord_n="${colno:-0}"
    ord_n="${ord_n// /}"
    if [[ ! "${ord_n}" =~ ^[0-9]+$ ]]; then
        ord_n=0
    fi
    idx=$((idx + 1))
    df="${WORK}/d.${idx}"
    schematool_unhex_to_file "${def_h:-}" "${df}"
    if [[ -s "${df}" ]]; then
        jq -nc \
            --arg tab "${tab}" --arg col "${col}" --arg typ "${typ:-}" \
            --arg nulls "${nulls:-}" --argjson pk "${pk_n}" --argjson ord "${ord_n}" \
            --rawfile dflt "${df}" \
            '{
              tab: ($tab | ascii_downcase),
              col: ($col | ascii_downcase),
              dtype: ($typ | ascii_downcase),
              nullable: (($nulls | ascii_upcase) == "Y"),
              dflt: $dflt,
              pk: $pk,
              ord: $ord
            }' >> "${NDJSON}"
    else
        jq -nc \
            --arg tab "${tab}" --arg col "${col}" --arg typ "${typ:-}" \
            --arg nulls "${nulls:-}" --argjson pk "${pk_n}" --argjson ord "${ord_n}" \
            '{
              tab: ($tab | ascii_downcase),
              col: ($col | ascii_downcase),
              dtype: ($typ | ascii_downcase),
              nullable: (($nulls | ascii_upcase) == "Y"),
              dflt: null,
              pk: $pk,
              ord: $ord
            }' >> "${NDJSON}"
    fi
done < "${EXPORT_FILE}"

if [[ ! -s "${NDJSON}" ]]; then
    jq -nc --arg schema "${SCHEMA_U}" '{schema:$schema, tables:[]}'
    exit 0
fi

# shellcheck disable=SC2016 # jq program is single-quoted on purpose
jq -s -c --arg schema "${SCHEMA_U}" '
  group_by(.tab)
  | map({
      table: .[0].tab,
      columns: (sort_by(.ord) | map({
        name: .col,
        data_type: .dtype,
        nullable: .nullable,
        default: .dflt
      })),
      primary_key: ([.[] | select(.pk > 0) | {o: .pk, n: .col}] | sort_by(.o) | map(.n)),
      indexes: []
    })
  | {schema: $schema, tables: .}
' "${NDJSON}"

exit 0
