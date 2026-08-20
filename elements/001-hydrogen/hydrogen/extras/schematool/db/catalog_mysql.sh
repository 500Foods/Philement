#!/usr/bin/env bash
# SchemaTool MySQL/MariaDB live catalog probe — information_schema (filtered)
#
# Prints JSON: { "schema": "...", "tables": [ { table, columns[], primary_key[] } ] }
#
# CHANGELOG
# 1.2.0 - 2026-08-20 - Assemble TSV with jq (drop python3)
# 1.1.0 - 2026-08-02 - Flat HEX export + Python assemble (avoid nested JSON_ARRAYAGG)
# 1.0.0 - 2026-08-02 - Phase 7a MySQL catalog probe

set -euo pipefail

HOST=""
PORT="3306"
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

if ! command -v mysql >/dev/null 2>&1; then
    echo "Error: mysql client not found" >&2
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi

if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --host, --user, and --database are required for mysql" >&2
    exit 1
fi

SCHEMA_USE="${SCHEMA}"
if [[ -z "${SCHEMA_USE}" || "${SCHEMA_USE}" == "." ]]; then
    SCHEMA_USE="${DATABASE}"
fi
DB_USE="${SCHEMA_USE}"

PASS=""
if [[ -n "${PASSWORD_ENV}" ]]; then
    if [[ -z "${!PASSWORD_ENV+x}" ]]; then
        echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
        exit 1
    fi
    PASS="${!PASSWORD_ENV}"
fi

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
        if [[ -n "${in_list}" ]]; then
            in_list="${in_list},"
        fi
        in_list="${in_list}'${t}'"
    done
    if [[ -z "${in_list}" ]]; then
        echo "{\"schema\":\"${SCHEMA_USE}\",\"tables\":[]}"
        exit 0
    fi
    TABLE_FILTER="AND c.TABLE_NAME IN (${in_list})"
fi

SCHEMA_SQL="${SCHEMA_USE//\'/\'\'}"

# TSV: table, column, data_type, is_nullable(YES/NO), ordinal, pk_ord(0 if not pk)
SQL=$(cat <<EOF
SELECT
    c.TABLE_NAME,
    c.COLUMN_NAME,
    LOWER(c.DATA_TYPE),
    c.IS_NULLABLE,
    c.ORDINAL_POSITION,
    COALESCE((
        SELECT k.ORDINAL_POSITION
        FROM information_schema.KEY_COLUMN_USAGE k
        WHERE k.TABLE_SCHEMA = c.TABLE_SCHEMA
          AND k.TABLE_NAME = c.TABLE_NAME
          AND k.COLUMN_NAME = c.COLUMN_NAME
          AND k.CONSTRAINT_NAME = 'PRIMARY'
        LIMIT 1
    ), 0) AS pk_ord
FROM information_schema.COLUMNS c
JOIN information_schema.TABLES t
  ON t.TABLE_SCHEMA = c.TABLE_SCHEMA
 AND t.TABLE_NAME = c.TABLE_NAME
 AND t.TABLE_TYPE = 'BASE TABLE'
WHERE c.TABLE_SCHEMA = '${SCHEMA_SQL}'
  ${TABLE_FILTER}
ORDER BY c.TABLE_NAME, c.ORDINAL_POSITION;
EOF
)

WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_cat_mysql.XXXXXX")
# shellcheck disable=SC2064 # expand WORK now for EXIT trap
trap "rm -rf \"${WORK}\"" EXIT
RAW="${WORK}/cols.tsv"
ERR="${WORK}/err.txt"

set +e
mysql -h "${HOST}" -P "${PORT}" -u "${USER_NAME}" -p"${PASS}" "${DB_USE}" \
    -N -B --raw -e "SET SESSION TRANSACTION READ ONLY; ${SQL}" >"${RAW}" 2>"${ERR}"
RC=$?
set -e

if [[ "${RC}" -ne 0 ]]; then
    SAFE=$(cat "${ERR}")
    SAFE="${SAFE//${PASS}/***}"
    SAFE=$(printf '%s\n' "${SAFE}" | grep -v 'Using a password on the command line' || true)
    echo "Error: mysql catalog probe failed (host=${HOST} db=${DB_USE} schema=${SCHEMA_USE})" >&2
    echo "${SAFE}" >&2
    exit 1
fi

# shellcheck disable=SC2016 # jq program is single-quoted on purpose
jq -n -c --arg schema "${SCHEMA_USE}" --rawfile raw "${RAW}" '
  def trim: gsub("^[[:space:]]+|[[:space:]]+$"; "");
  def as_int: tonumber? // 0;
  [
    $raw
    | gsub("\r"; "")
    | split("\n")
    | map(select(length > 0) | split("\t"))
    | map(select(length >= 6))
    | .[]
    | {
        tab: (.[0] | trim),
        col: (.[1] | trim),
        dtype: ((.[2] // "") | trim | ascii_downcase),
        nullable: ((.[3] // "") | trim | ascii_upcase == "YES"),
        ord: (.[4] | as_int),
        pk: (.[5] | as_int)
      }
  ]
  | group_by(.tab)
  | map({
      table: .[0].tab,
      columns: (sort_by(.ord) | map({
        name: .col,
        data_type: .dtype,
        nullable: .nullable,
        default: null
      })),
      primary_key: ([.[] | select(.pk > 0) | {o: .pk, n: .col}] | sort_by(.o) | map(.n)),
      indexes: []
    })
  | {schema: $schema, tables: .}
'

exit 0
