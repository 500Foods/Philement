#!/usr/bin/env bash
# SchemaTool SQLite metadata adapter — read-only SELECT on queries
#
# --database is the path to the .sqlite file. --schema ignored (no schemas).
#
# CHANGELOG
# 1.0.0 - 2026-07-29 - Phase 3 SQLite adapter

set -euo pipefail

DATABASE=""
FROM_REF=""
TO_REF=""
QUALIFIED="queries"
# Accept and ignore connection flags for uniform CLI
HOST=""
PORT=""
USER_NAME=""
SCHEMA=""
PASSWORD_ENV=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host) HOST="${2:-}"; shift 2 ;;
        --port) PORT="${2:-}"; shift 2 ;;
        --user) USER_NAME="${2:-}"; shift 2 ;;
        --database) DATABASE="${2:-}"; shift 2 ;;
        --schema) SCHEMA="${2:-}"; shift 2 ;;
        --password-env) PASSWORD_ENV="${2:-}"; shift 2 ;;
        --from) FROM_REF="${2:-}"; shift 2 ;;
        --to) TO_REF="${2:-}"; shift 2 ;;
        --qualified) QUALIFIED="${2:-}"; shift 2 ;;
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

WHERE="query_type_a28 BETWEEN 1000 AND 1003"
if [[ -n "${FROM_REF}" ]]; then
    WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
fi
if [[ -n "${TO_REF}" ]]; then
    WHERE="${WHERE} AND query_ref <= ${TO_REF}"
fi

# sqlite3 json_group_array / json_object (SQLite 3.38+)
# Build objects inline (subquery of json_object AS text double-encodes strings).
# CAST LOB/BLOB columns to TEXT — json_object rejects BLOB values.
SQL=$(cat <<EOF
SELECT COALESCE(
    json_group_array(
        json_object(
            'query_ref', query_ref,
            'query_type', query_type_a28,
            'name', COALESCE(CAST(name AS TEXT), ''),
            'summary', COALESCE(CAST(summary AS TEXT), ''),
            'code', COALESCE(CAST(code AS TEXT), '')
        )
    ),
    json_array()
)
FROM (
    SELECT query_ref, query_type_a28, name, summary, code
    FROM ${QUALIFIED}
    WHERE ${WHERE}
    ORDER BY query_ref, query_type_a28
);
EOF
)

set +e
OUT=$(sqlite3 -batch "${DATABASE}" "${SQL}" 2>&1)
RC=$?
set -e

if [[ "${RC}" -ne 0 ]]; then
    echo "Error: sqlite query failed (db=${DATABASE} table=${QUALIFIED})" >&2
    echo "${OUT}" >&2
    exit 1
fi

if [[ -z "${OUT}" ]]; then
    echo "[]"
    exit 0
fi

if ! printf '%s' "${OUT}" | jq -e 'type == "array"' >/dev/null 2>&1; then
    echo "Error: sqlite did not return a JSON array" >&2
    echo "${OUT}" >&2
    exit 1
fi

printf '%s\n' "${OUT}"
exit 0
