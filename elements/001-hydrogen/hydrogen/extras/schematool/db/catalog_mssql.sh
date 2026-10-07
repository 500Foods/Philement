#!/usr/bin/env bash
# SchemaTool MSSQL live catalog probe — information_schema (filtered).
#
# Two FOR JSON PATH queries (columns, primary keys) are assembled into
# {schema, tables:[{table, columns, primary_key, indexes}]}.
# SELECT only. data_type is lowercased for the catalog fold.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Phase 2 MSSQL catalog probe

set -euo pipefail

# shellcheck source=extras/schematool/db/mssql_common.sh # shared sqlcmd runner
source "$(dirname "${BASH_SOURCE[0]}")/mssql_common.sh"

HOST=""
PORT="1433"
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

if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for mssql" >&2
    exit 1
fi
# shellcheck disable=SC2310 # bad schema is a usage error, not a crash
if ! schematool_mssql_ident_ok "${SCHEMA}"; then
    echo "Error: --schema must be a plain identifier" >&2
    exit 1
fi
# shellcheck disable=SC2310 # ready writes the error and returns 1
if ! schematool_mssql_ready "${PASSWORD_ENV}"; then
    exit 1
fi

TABLE_FILTER=""
if [[ -n "${TABLES_CSV}" ]]; then
    in_list=""
    IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
    for raw in "${TABLE_ARR[@]}"; do
        t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [[ -z "${t}" ]] && continue
        # shellcheck disable=SC2310 # bad table name is a usage error, not a crash
        if ! schematool_mssql_ident_ok "${t}"; then
            echo "Error: invalid table name: ${t}" >&2
            exit 1
        fi
        if [[ -n "${in_list}" ]]; then
            in_list="${in_list}, "
        fi
        in_list="${in_list}N'${t}'"
    done
    if [[ -z "${in_list}" ]]; then
        jq -nc --arg schema "${SCHEMA}" '{schema:$schema, tables:[]}'
        exit 0
    fi
    TABLE_FILTER="AND c.TABLE_NAME IN (${in_list})"
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/schematool_mssql_catalog.XXXXXX")"
# shellcheck disable=SC2064 # expand WORK now for the EXIT trap
trap "rm -rf \"${WORK}\"" EXIT
COLS_RAW="${WORK}/cols.raw"
PKS_RAW="${WORK}/pks.raw"
COLS_JSON="${WORK}/cols.json"
PKS_JSON="${WORK}/pks.json"

schematool_mssql_run "${PASSWORD_ENV}" "${USER_NAME}" "${DATABASE}" "${COLS_RAW}" <<SQL
SET NOCOUNT ON;
SELECT COALESCE((
  SELECT
    c.TABLE_NAME AS table_name,
    c.COLUMN_NAME AS column_name,
    LOWER(CASE
      WHEN c.CHARACTER_MAXIMUM_LENGTH = -1 THEN c.DATA_TYPE + '(max)'
      WHEN c.CHARACTER_MAXIMUM_LENGTH IS NOT NULL AND c.CHARACTER_MAXIMUM_LENGTH > 0
        THEN c.DATA_TYPE + '(' + CONVERT(varchar(10), c.CHARACTER_MAXIMUM_LENGTH) + ')'
      WHEN c.NUMERIC_PRECISION IS NOT NULL
           AND c.DATA_TYPE IN ('decimal', 'numeric')
        THEN c.DATA_TYPE + '(' + CONVERT(varchar(10), c.NUMERIC_PRECISION) + ','
             + CONVERT(varchar(10), ISNULL(c.NUMERIC_SCALE, 0)) + ')'
      ELSE c.DATA_TYPE
    END) AS data_type,
    c.IS_NULLABLE AS is_nullable,
    c.ORDINAL_POSITION AS ordinal_position,
    c.COLUMN_DEFAULT AS column_default
  FROM INFORMATION_SCHEMA.COLUMNS c
  WHERE c.TABLE_SCHEMA = N'${SCHEMA}'
    AND c.TABLE_NAME IN (
      SELECT t.TABLE_NAME
      FROM INFORMATION_SCHEMA.TABLES t
      WHERE t.TABLE_SCHEMA = c.TABLE_SCHEMA
        AND t.TABLE_TYPE = 'BASE TABLE'
    )
    ${TABLE_FILTER}
  ORDER BY c.TABLE_NAME, c.ORDINAL_POSITION
  FOR JSON PATH, INCLUDE_NULL_VALUES
), N'[]');
SQL

PK_FILTER="${TABLE_FILTER/c.TABLE_NAME/kcu.TABLE_NAME}"
schematool_mssql_run "${PASSWORD_ENV}" "${USER_NAME}" "${DATABASE}" "${PKS_RAW}" <<SQL
SET NOCOUNT ON;
SELECT COALESCE((
  SELECT
    kcu.TABLE_NAME AS table_name,
    kcu.COLUMN_NAME AS column_name,
    kcu.ORDINAL_POSITION AS ordinal_position
  FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc
  JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu
    ON tc.CONSTRAINT_NAME = kcu.CONSTRAINT_NAME
   AND tc.TABLE_SCHEMA = kcu.TABLE_SCHEMA
   AND tc.TABLE_NAME = kcu.TABLE_NAME
   AND tc.CONSTRAINT_CATALOG = kcu.CONSTRAINT_CATALOG
  WHERE tc.CONSTRAINT_TYPE = 'PRIMARY KEY'
    AND tc.TABLE_SCHEMA = N'${SCHEMA}'
    ${PK_FILTER}
  ORDER BY kcu.TABLE_NAME, kcu.ORDINAL_POSITION
  FOR JSON PATH, INCLUDE_NULL_VALUES
), N'[]');
SQL

schematool_mssql_emit_json "${COLS_RAW}" > "${COLS_JSON}"
schematool_mssql_emit_json "${PKS_RAW}" > "${PKS_JSON}"

jq -n --arg schema "${SCHEMA}" --slurpfile cols "${COLS_JSON}" --slurpfile pks "${PKS_JSON}" '
  ($cols[0] // []) as $c
  | ($pks[0] // []) as $p
  | ($c | map(.table_name) | unique | sort) as $names
  | {
      schema: $schema,
      tables: [
        $names[] as $t
        | {
            table: $t,
            columns: [
              $c[]
              | select(.table_name == $t)
              | {
                  name: .column_name,
                  data_type: .data_type,
                  nullable: (.is_nullable == "YES"),
                  default: .column_default
                }
            ],
            primary_key: [
              $p[]
              | select(.table_name == $t)
              | .column_name
            ],
            indexes: []
          }
      ]
    }
'
exit 0
