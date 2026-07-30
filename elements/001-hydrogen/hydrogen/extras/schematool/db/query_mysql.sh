#!/usr/bin/env bash
# SchemaTool MySQL/MariaDB metadata adapter — read-only SELECT on queries
#
# CHANGELOG
# 1.0.0 - 2026-07-29 - Phase 3 MySQL adapter

set -euo pipefail

HOST=""
PORT="3306"
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

# Schema may be a separate MySQL database name; prefer --schema when set
DB_USE="${DATABASE}"
if [[ -n "${SCHEMA}" && "${SCHEMA}" != "." ]]; then
    # When schema is set, table is schema.queries and connection DB can stay DATABASE
    if [[ -z "${QUALIFIED}" ]]; then
        QUALIFIED="\`${SCHEMA}\`.queries"
    fi
else
    if [[ -z "${QUALIFIED}" ]]; then
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

# JSON_ARRAYAGG available MySQL 5.7.22+ / MariaDB 10.5+
SQL=$(cat <<EOF
SELECT COALESCE(JSON_ARRAYAGG(row_json), JSON_ARRAY())
FROM (
    SELECT JSON_OBJECT(
        'query_ref', query_ref,
        'query_type', query_type_a28,
        'name', COALESCE(name, ''),
        'summary', COALESCE(summary, ''),
        'code', COALESCE(code, '')
    ) AS row_json
    FROM ${QUALIFIED}
    WHERE ${WHERE}
    ORDER BY query_ref, query_type_a28
) sub;
EOF
)

set +e
OUT=$(mysql -h "${HOST}" -P "${PORT}" -u "${USER_NAME}" -p"${PASS}" "${DB_USE}" \
    -N -B -e "${SQL}" 2>&1)
RC=$?
set -e

if [[ "${RC}" -ne 0 ]]; then
    SAFE="${OUT//${PASS}/***}"
    # mysql often prefixes "Warning: Using a password on the command line interface can be insecure."
    SAFE=$(printf '%s\n' "${SAFE}" | grep -v 'Using a password on the command line' || true)
    echo "Error: mysql query failed (host=${HOST} port=${PORT} db=${DB_USE} table=${QUALIFIED})" >&2
    echo "${SAFE}" >&2
    exit 1
fi

# Strip password warning lines if mixed with result
OUT=$(printf '%s\n' "${OUT}" | grep -v 'Using a password on the command line' || true)
OUT=$(printf '%s' "${OUT}" | tr -d '\r')

if [[ -z "${OUT}" || "${OUT}" == "NULL" ]]; then
    echo "[]"
    exit 0
fi

if ! printf '%s' "${OUT}" | jq -e 'type == "array"' >/dev/null 2>&1; then
    echo "Error: mysql did not return a JSON array" >&2
    echo "${OUT}" >&2
    exit 1
fi

printf '%s\n' "${OUT}"
exit 0
