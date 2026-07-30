#!/usr/bin/env bash
# SchemaTool DB2 metadata adapter — read-only EXPORT of queries rows
#
# Uses db2 EXPORT … LOBS TO for CLOB code/summary (avoids VARCHAR truncation).
# Prints JSON array of {query_ref,query_type,name,summary,code}.
# Never prints password.
#
# CHANGELOG
# 1.0.0 - 2026-07-29 - Phase 3 DB2 adapter (EXPORT + LOBSINFILE)

set -euo pipefail

HOST=""
PORT=""
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

: "${HOST}" "${PORT}"

if ! command -v db2 >/dev/null 2>&1; then
    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        # shellcheck disable=SC1091 # host-specific DB2 profile
        . /home/db2inst1/sqllib/db2profile
    fi
fi
if ! command -v db2 >/dev/null 2>&1; then
    echo "Error: db2 client not found" >&2
    exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 not found (required to parse DB2 EXPORT)" >&2
    exit 1
fi

if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for db2" >&2
    exit 1
fi

if [[ -z "${QUALIFIED}" ]]; then
    if [[ -n "${SCHEMA}" && "${SCHEMA}" != "." ]]; then
        QUALIFIED="${SCHEMA^^}.QUERIES"
    else
        QUALIFIED="QUERIES"
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

PASS_SQL="${PASS//\'/\'\'}"

WHERE="query_type_a28 BETWEEN 1000 AND 1003"
if [[ -n "${FROM_REF}" ]]; then
    WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
fi
if [[ -n "${TO_REF}" ]]; then
    WHERE="${WHERE} AND query_ref <= ${TO_REF}"
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/schematool_db2.XXXXXX")"
# shellcheck disable=SC2064 # expand WORK now for EXIT trap cleanup
trap "rm -rf \"${WORK}\"" EXIT

SCRIPT="${WORK}/export.sql"
DEL_FILE="${WORK}/rows.del"

cat > "${SCRIPT}" <<EOF
CONNECT TO ${DATABASE} USER ${USER_NAME} USING '${PASS_SQL}';
EXPORT TO ${DEL_FILE} OF DEL LOBS TO ${WORK}/ LOBFILE stlob
MODIFIED BY LOBSINFILE
SELECT query_ref, query_type_a28, name, summary, code
FROM ${QUALIFIED}
WHERE ${WHERE}
ORDER BY query_ref, query_type_a28;
CONNECT RESET;
EOF

set +e
RAW=$(db2 -tvf "${SCRIPT}" 2>&1)
set -e

# Drop SQL script (contains password) immediately
rm -f "${SCRIPT}"

if ! printf '%s' "${RAW}" | grep -q "Number of rows exported"; then
    SAFE="${RAW//${PASS}/***}"
    SAFE=$(printf '%s\n' "${SAFE}" | grep -v "USING '" || true)
    echo "Error: db2 EXPORT failed (db=${DATABASE} table=${QUALIFIED})" >&2
    echo "${SAFE}" | head -40 >&2
    exit 1
fi

if [[ ! -f "${DEL_FILE}" ]]; then
    echo "Error: db2 EXPORT produced no data file" >&2
    exit 1
fi

# Parse DEL + LOB files → JSON
export SCHEMATOOL_DB2_WORK="${WORK}"
export SCHEMATOOL_DB2_DEL="${DEL_FILE}"
python3 - <<'PY'
import csv
import json
import os
import re
import sys

work = os.environ["SCHEMATOOL_DB2_WORK"]
del_path = os.environ["SCHEMATOOL_DB2_DEL"]

# LOB pointer: filename.start.length/  e.g. stlob.001.lob.0.175/
lob_re = re.compile(r"^(.+\.lob)\.(\d+)\.(\d+)/$")


def resolve_field(raw: str) -> str:
    if raw is None:
        return ""
    raw = raw.strip()
    if not raw:
        return ""
    m = lob_re.match(raw)
    if not m:
        return raw
    fname, start_s, length_s = m.group(1), m.group(2), m.group(3)
    path = os.path.join(work, fname)
    if not os.path.isfile(path):
        # sometimes basename only differs
        alt = os.path.join(work, os.path.basename(fname))
        path = alt if os.path.isfile(alt) else path
    if not os.path.isfile(path):
        sys.stderr.write(f"Error: missing LOB file {fname}\n")
        sys.exit(1)
    start = int(start_s)
    length = int(length_s)
    with open(path, "rb") as fh:
        fh.seek(start)
        data = fh.read(length)
    return data.decode("utf-8", errors="replace")


rows = []
with open(del_path, "r", encoding="utf-8", errors="replace", newline="") as fh:
    reader = csv.reader(fh)
    for parts in reader:
        if len(parts) < 5:
            continue
        try:
            ref = int(parts[0])
            qtype = int(parts[1])
        except ValueError:
            continue
        name = resolve_field(parts[2])
        summary = resolve_field(parts[3])
        code = resolve_field(parts[4])
        rows.append(
            {
                "query_ref": ref,
                "query_type": qtype,
                "name": name,
                "summary": summary,
                "code": code,
            }
        )

json.dump(rows, sys.stdout, ensure_ascii=False)
sys.stdout.write("\n")
PY

exit 0
