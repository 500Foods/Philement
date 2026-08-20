#!/usr/bin/env bash
# SchemaTool MySQL/MariaDB metadata adapter — read-only SELECT on queries
#
# Emits JSON array of {query_ref,query_type,name,summary,code}.
# Large text fields are transferred as HEX and decoded with xxd so the
# mysql client cannot truncate mid-JSON (seen with long migration code).
#
# CHANGELOG
# 1.4.0 - 2026-08-20 - HEX decode via xxd+jq (drop python3)
# 1.3.0 - 2026-08-06 - SET SESSION TRANSACTION READ ONLY before SELECT
# 1.2.0 - 2026-08-02 - HEX+Python decode (avoid client line truncation)
# 1.1.0 - 2026-08-02 - NDJSON rows + schema-as-DB
# 1.0.0 - 2026-07-29 - Phase 3 MySQL adapter

set -euo pipefail

# shellcheck source=extras/schematool/db/common.sh # HEX decode helper
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

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
# shellcheck disable=SC2310 # missing xxd is a hard adapter error
if ! schematool_require_xxd; then
    exit 1
fi

if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --host, --user, and --database are required for mysql" >&2
    exit 1
fi

# MySQL: schema name == database name for Acuranzo layouts (demo / demomrdb).
DB_USE="${DATABASE}"
if [[ -n "${SCHEMA}" && "${SCHEMA}" != "." ]]; then
    DB_USE="${SCHEMA}"
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

# TSV: ref, type, hex(name), hex(summary), hex(code)
SQL=$(cat <<EOF
SELECT
    query_ref,
    query_type_a28,
    HEX(CAST(COALESCE(name, '') AS CHAR)),
    HEX(CAST(COALESCE(summary, '') AS CHAR)),
    HEX(CAST(COALESCE(code, '') AS CHAR))
FROM ${QUALIFIED}
WHERE ${WHERE}
ORDER BY query_ref, query_type_a28;
EOF
)

WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_mysql.XXXXXX")
# shellcheck disable=SC2064 # expand WORK now for EXIT trap
trap "rm -rf \"${WORK}\"" EXIT
RAW="${WORK}/rows.tsv"
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
    echo "Error: mysql query failed (host=${HOST} port=${PORT} db=${DB_USE} table=${QUALIFIED})" >&2
    echo "${SAFE}" >&2
    exit 1
fi

if [[ ! -s "${RAW}" ]]; then
    echo "[]"
    exit 0
fi

NDJSON="${WORK}/rows.ndjson"
: > "${NDJSON}"
idx=0
while IFS=$'\t' read -r ref_s typ_s name_h sum_h code_h || [[ -n "${ref_s:-}" ]]; do
    [[ -z "${ref_s:-}" ]] && continue
    if [[ ! "${ref_s}" =~ ^[0-9]+$ || ! "${typ_s}" =~ ^[0-9]+$ ]]; then
        continue
    fi
    idx=$((idx + 1))
    nf="${WORK}/n.${idx}"
    sf="${WORK}/s.${idx}"
    cf="${WORK}/c.${idx}"
    schematool_unhex_to_file "${name_h}" "${nf}"
    schematool_unhex_to_file "${sum_h}" "${sf}"
    schematool_unhex_to_file "${code_h}" "${cf}"
    jq -nc --argjson query_ref "${ref_s}" --argjson query_type "${typ_s}" \
        --rawfile name "${nf}" --rawfile summary "${sf}" --rawfile code "${cf}" \
        '{query_ref:$query_ref, query_type:$query_type, name:$name, summary:$summary, code:$code}' \
        >> "${NDJSON}"
done < "${RAW}"

if [[ ! -s "${NDJSON}" ]]; then
    echo "[]"
    exit 0
fi
jq -s -c '.' "${NDJSON}"

exit 0
