#!/usr/bin/env bash
# SchemaTool MySQL/MariaDB metadata adapter — read-only SELECT on queries
#
# Emits JSON array of {query_ref,query_type,name,summary,code}.
# Large text fields are transferred as HEX and decoded in Python so the
# mysql client cannot truncate mid-JSON (seen with long migration code).
#
# CHANGELOG
# 1.3.0 - 2026-08-06 - SET SESSION TRANSACTION READ ONLY before SELECT
# 1.2.0 - 2026-08-02 - HEX+Python decode (avoid client line truncation)
# 1.1.0 - 2026-08-02 - NDJSON rows + schema-as-DB
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
if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 not found" >&2
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

python3 - "${RAW}" <<'PY'
import json, sys
from pathlib import Path

path = Path(sys.argv[1])
rows = []
if path.stat().st_size == 0:
    print("[]")
    sys.exit(0)

def unhex(h: str) -> str:
    h = (h or "").strip()
    if not h or h.upper() == "NULL":
        return ""
    try:
        return bytes.fromhex(h).decode("utf-8", errors="replace")
    except ValueError:
        return ""

with path.open("r", encoding="utf-8", errors="replace") as f:
    for line in f:
        line = line.rstrip("\n")
        if not line:
            continue
        parts = line.split("\t")
        if len(parts) < 5:
            continue
        ref_s, typ_s, name_h, sum_h, code_h = parts[0], parts[1], parts[2], parts[3], parts[4]
        try:
            ref = int(ref_s)
            typ = int(typ_s)
        except ValueError:
            continue
        rows.append({
            "query_ref": ref,
            "query_type": typ,
            "name": unhex(name_h),
            "summary": unhex(sum_h),
            "code": unhex(code_h),
        })

print(json.dumps(rows, ensure_ascii=False, separators=(",", ":")))
PY

exit 0
