#!/usr/bin/env bash
# SchemaTool PostgreSQL live catalog probe — information_schema (filtered)
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Phase 7a PostgreSQL catalog probe

set -euo pipefail

HOST=""
PORT="5432"
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

if ! command -v psql >/dev/null 2>&1; then
    echo "Error: psql not found" >&2
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi

if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --host, --user, and --database are required for postgresql" >&2
    exit 1
fi
if [[ -z "${SCHEMA}" || "${SCHEMA}" == "." ]]; then
    echo "Error: --schema is required for postgresql catalog probe" >&2
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

TABLE_FILTER=""
if [[ -n "${TABLES_CSV}" ]]; then
    arr_inner=""
    IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
    for raw in "${TABLE_ARR[@]}"; do
        t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [[ -z "${t}" ]] && continue
        if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
            echo "Error: invalid table name: ${t}" >&2
            exit 1
        fi
        if [[ -n "${arr_inner}" ]]; then
            arr_inner="${arr_inner},"
        fi
        arr_inner="${arr_inner}'${t}'"
    done
    if [[ -z "${arr_inner}" ]]; then
        jq -nc --arg schema "${SCHEMA}" '{schema:$schema, tables:[]}'
        exit 0
    fi
    TABLE_FILTER="AND c.table_name = ANY(ARRAY[${arr_inner}]::text[])"
fi

SCHEMA_SQL="${SCHEMA//\'/\'\'}"

SQL=$(cat <<EOF
SELECT COALESCE(json_agg(tbl ORDER BY tbl->>'table'), '[]'::json)
FROM (
    SELECT json_build_object(
        'table', t.table_name,
        'columns', COALESCE((
            SELECT json_agg(
                json_build_object(
                    'name', c.column_name,
                    'data_type', lower(c.data_type),
                    'nullable', (c.is_nullable = 'YES'),
                    'default', c.column_default
                ) ORDER BY c.ordinal_position
            )
            FROM information_schema.columns c
            WHERE c.table_schema = t.table_schema
              AND c.table_name = t.table_name
        ), '[]'::json),
        'primary_key', COALESCE((
            SELECT json_agg(kcu.column_name ORDER BY kcu.ordinal_position)
            FROM information_schema.table_constraints tc
            JOIN information_schema.key_column_usage kcu
              ON tc.constraint_name = kcu.constraint_name
             AND tc.table_schema = kcu.table_schema
             AND tc.table_name = kcu.table_name
            WHERE tc.constraint_type = 'PRIMARY KEY'
              AND tc.table_schema = t.table_schema
              AND tc.table_name = t.table_name
        ), '[]'::json),
        'indexes', '[]'::json
    ) AS tbl
    FROM information_schema.tables t
    WHERE t.table_schema = '${SCHEMA_SQL}'
      AND t.table_type = 'BASE TABLE'
      ${TABLE_FILTER//c.table_name/t.table_name}
) sub;
EOF
)

export PGPASSWORD="${PASS}"
export PGOPTIONS="${PGOPTIONS:-} -c default_transaction_read_only=on"
set +e
OUT=$(psql -h "${HOST}" -p "${PORT}" -U "${USER_NAME}" -d "${DATABASE}" \
    -v ON_ERROR_STOP=1 -t -A -c "${SQL}" 2>&1)
RC=$?
set -e
unset PGPASSWORD
unset PGOPTIONS

if [[ "${RC}" -ne 0 ]]; then
    SAFE="${OUT//${PASS}/***}"
    echo "Error: postgresql catalog probe failed (host=${HOST} db=${DATABASE} schema=${SCHEMA})" >&2
    echo "${SAFE}" >&2
    exit 1
fi

if [[ -z "${OUT}" ]]; then
    OUT="[]"
fi

if ! printf '%s' "${OUT}" | jq -e 'type == "array"' >/dev/null 2>&1; then
    echo "Error: postgresql catalog did not return a JSON array" >&2
    echo "${OUT}" >&2
    exit 1
fi

jq -nc --arg schema "${SCHEMA}" --argjson tables "${OUT}" \
    '{schema:$schema, tables:$tables}'
exit 0
