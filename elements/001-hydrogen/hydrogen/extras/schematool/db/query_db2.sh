#!/usr/bin/env bash
# SchemaTool DB2 metadata adapter — read-only EXPORT of queries rows
#
# Uses db2 EXPORT … LOBS TO for CLOB code/summary (avoids VARCHAR truncation).
# Prints JSON array of {query_ref,query_type,name,summary,code}.
# Never prints password.
#
# CHANGELOG
# 1.1.0 - 2026-08-20 - Tab DEL + dd LOB assemble (drop python3)
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
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
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
MODIFIED BY LOBSINFILE COLDEL0x09 NOCHARDEL
SELECT query_ref, query_type_a28, COALESCE(name, ''), summary, code
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

# LOB pointer: filename.start.length/  e.g. stlob.001.lob.0.175/
resolve_db2_field() {
    local raw="$1"
    local dest="$2"
    local fname start length path
    raw="${raw#"${raw%%[![:space:]]*}"}"
    raw="${raw%"${raw##*[![:space:]]}"}"
    if [[ -z "${raw}" ]]; then
        : > "${dest}"
        return 0
    fi
    if [[ "${raw}" =~ ^(.+\.lob)\.([0-9]+)\.([0-9]+)/$ ]]; then
        fname="${BASH_REMATCH[1]}"
        start="${BASH_REMATCH[2]}"
        length="${BASH_REMATCH[3]}"
        path="${WORK}/${fname}"
        if [[ ! -f "${path}" ]]; then
            path="${WORK}/${fname##*/}"
        fi
        if [[ ! -f "${path}" ]]; then
            echo "Error: missing LOB file ${fname}" >&2
            return 1
        fi
        dd if="${path}" of="${dest}" iflag=skip_bytes,count_bytes \
            skip="${start}" count="${length}" status=none
        return 0
    fi
    printf '%s' "${raw}" > "${dest}"
}

if [[ ! -s "${DEL_FILE}" ]]; then
    echo "[]"
    exit 0
fi

NDJSON="${WORK}/rows.ndjson"
: > "${NDJSON}"
idx=0
while IFS=$'\t' read -r ref_s typ_s name_h sum_raw code_raw || [[ -n "${ref_s:-}" ]]; do
    [[ -z "${ref_s:-}" ]] && continue
    if [[ ! "${ref_s}" =~ ^[0-9]+$ || ! "${typ_s}" =~ ^[0-9]+$ ]]; then
        continue
    fi
    idx=$((idx + 1))
    nf="${WORK}/n.${idx}"
    sf="${WORK}/s.${idx}"
    cf="${WORK}/c.${idx}"
    resolve_db2_field "${name_h:-}" "${nf}"
    resolve_db2_field "${sum_raw:-}" "${sf}"
    resolve_db2_field "${code_raw:-}" "${cf}"
    jq -nc --argjson query_ref "${ref_s}" --argjson query_type "${typ_s}" \
        --rawfile name "${nf}" --rawfile summary "${sf}" --rawfile code "${cf}" \
        '{query_ref:$query_ref, query_type:$query_type, name:$name, summary:$summary, code:$code}' \
        >> "${NDJSON}"
done < "${DEL_FILE}"

if [[ ! -s "${NDJSON}" ]]; then
    echo "[]"
    exit 0
fi
jq -s -c '.' "${NDJSON}"

exit 0
