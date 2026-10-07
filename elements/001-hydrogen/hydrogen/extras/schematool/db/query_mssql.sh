#!/usr/bin/env bash
# SchemaTool MSSQL metadata adapter — read-only SELECT on queries.
#
# Emits a JSON array of {query_ref, query_type, name, summary, code}.
# FOR JSON PATH comes back as one UTF-8 cell (sqlcmd -y 0).
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Phase 2 MSSQL metadata adapter

set -euo pipefail

# shellcheck source=extras/schematool/db/mssql_common.sh # shared sqlcmd runner
source "$(dirname "${BASH_SOURCE[0]}")/mssql_common.sh"

HOST=""
PORT="1433"
USER_NAME=""
DATABASE=""
SCHEMA=""
PASSWORD_ENV=""
FROM_REF=""
TO_REF=""

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
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

# Accepted so the shared runner can pass them. sqlcmd uses localhost.
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
if [[ -n "${FROM_REF}" && ! "${FROM_REF}" =~ ^[0-9]+$ ]]; then
    echo "Error: --from must be an integer" >&2
    exit 1
fi
if [[ -n "${TO_REF}" && ! "${TO_REF}" =~ ^[0-9]+$ ]]; then
    echo "Error: --to must be an integer" >&2
    exit 1
fi
# shellcheck disable=SC2310 # ready writes the error and returns 1
if ! schematool_mssql_ready "${PASSWORD_ENV}"; then
    exit 1
fi

WHERE="query_type_a28 BETWEEN 1000 AND 1003"
if [[ -n "${FROM_REF}" ]]; then
    WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
fi
if [[ -n "${TO_REF}" ]]; then
    WHERE="${WHERE} AND query_ref <= ${TO_REF}"
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/schematool_mssql_query.XXXXXX")"
# shellcheck disable=SC2064 # expand WORK now for the EXIT trap
trap "rm -rf \"${WORK}\"" EXIT
RAW="${WORK}/raw.txt"

schematool_mssql_run "${PASSWORD_ENV}" "${USER_NAME}" "${DATABASE}" "${RAW}" <<SQL
SET NOCOUNT ON;
SELECT COALESCE((
  SELECT query_ref,
         query_type_a28 AS query_type,
         COALESCE(name, N'') AS name,
         COALESCE(summary, N'') AS summary,
         COALESCE(code, N'') AS code
  FROM [${SCHEMA}].[queries]
  WHERE ${WHERE}
  ORDER BY query_ref, query_type_a28
  FOR JSON PATH, INCLUDE_NULL_VALUES
), N'[]');
SQL

schematool_mssql_emit_json "${RAW}"
exit 0
