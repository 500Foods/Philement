#!/usr/bin/env bash
# SchemaTool PostgreSQL metadata adapter — read-only SELECT on queries
#
# Usage:
#   query_pg.sh --host H --port P --user U --database D --schema S \
#     [--password-env VAR] [--from N] [--to N] [--qualified TABLE]
#
# Prints JSON array of {query_ref,query_type,name,summary,code} to stdout.
# Never prints password.
#
# CHANGELOG
# 1.0.0 - 2026-07-29 - Phase 3 PostgreSQL adapter

set -euo pipefail

HOST=""
PORT="5432"
USER_NAME=""
DATABASE=""
SCHEMA=""
PASSWORD_ENV=""
FROM_REF=""
TO_REF=""
QUALIFIED=""

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

if [[ -z "${QUALIFIED}" ]]; then
    if [[ -n "${SCHEMA}" && "${SCHEMA}" != "." ]]; then
        QUALIFIED="${SCHEMA}.queries"
    else
        QUALIFIED="queries"
    fi
fi

PASS=""
if [[ -n "${PASSWORD_ENV}" ]]; then
    if [[ -z "${!PASSWORD_ENV+x}" ]]; then
        echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
        exit 1
    fi
    PASS="${!PASSWORD_ENV}"
fi

WHERE="query_type_a28 BETWEEN 1000 AND 1003"
if [[ -n "${FROM_REF}" ]]; then
    WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
fi
if [[ -n "${TO_REF}" ]]; then
    WHERE="${WHERE} AND query_ref <= ${TO_REF}"
fi

# json_build_object + json_agg — code/name/summary as text; null summary → empty string
SQL=$(cat <<EOF
SELECT COALESCE(json_agg(row_json ORDER BY query_ref, query_type), '[]'::json)
FROM (
    SELECT json_build_object(
        'query_ref', query_ref,
        'query_type', query_type_a28,
        'name', COALESCE(name, ''),
        'summary', COALESCE(summary, ''),
        'code', COALESCE(code, '')
    ) AS row_json,
    query_ref,
    query_type_a28 AS query_type
    FROM ${QUALIFIED}
    WHERE ${WHERE}
) sub;
EOF
)

export PGPASSWORD="${PASS}"
set +e
OUT=$(psql -h "${HOST}" -p "${PORT}" -U "${USER_NAME}" -d "${DATABASE}" \
    -v ON_ERROR_STOP=1 -t -A -c "${SQL}" 2>&1)
RC=$?
set -e
unset PGPASSWORD

if [[ "${RC}" -ne 0 ]]; then
    # Scrub accidental password echoes (should not appear)
    SAFE="${OUT//${PASS}/***}"
    echo "Error: postgresql query failed (host=${HOST} port=${PORT} db=${DATABASE} table=${QUALIFIED})" >&2
    echo "${SAFE}" >&2
    exit 1
fi

# psql may return empty
if [[ -z "${OUT}" ]]; then
    echo "[]"
    exit 0
fi

if ! printf '%s' "${OUT}" | jq -e 'type == "array"' >/dev/null 2>&1; then
    echo "Error: postgresql did not return a JSON array" >&2
    echo "${OUT}" >&2
    exit 1
fi

printf '%s\n' "${OUT}"
exit 0
