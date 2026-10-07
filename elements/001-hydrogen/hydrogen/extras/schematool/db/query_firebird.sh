#!/usr/bin/env bash
# SchemaTool Firebird metadata adapter — read-only SELECT on queries.
#
# Prints a JSON array of {query_ref, query_type, name, summary, code}.
# isql-fb wraps long fields, so each value is HEX_ENCODE'd in short
# chunks (32 bytes, shrunk for multibyte characters) and decoded here.
# The SQL is a single read-only EXECUTE BLOCK. There is no DML.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Phase 1 Firebird metadata adapter

set -euo pipefail

# shellcheck source=extras/schematool/db/firebird_common.sh # isql runner
source "$(dirname "${BASH_SOURCE[0]}")/firebird_common.sh"

HOST=""
PORT=""
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
        --qualified) shift 2 ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

: "${HOST}" "${PORT}" "${SCHEMA}"

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi
if ! command -v xxd >/dev/null 2>&1; then
    echo "Error: xxd not found" >&2
    exit 1
fi
if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for firebird" >&2
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

WHERE="query_type_a28 BETWEEN 1000 AND 1003"
if [[ -n "${FROM_REF}" ]]; then
    WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
fi
if [[ -n "${TO_REF}" ]]; then
    WHERE="${WHERE} AND query_ref <= ${TO_REF}"
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/schematool_fb_query.XXXXXX")"
# shellcheck disable=SC2064 # expand WORK now for the EXIT trap
trap 'rm -rf "'"${WORK}"'"' EXIT
RAW="${WORK}/raw.txt"

# shellcheck disable=SC2310 # isql writes the error and returns 1
if ! schematool_fb_isql "${DATABASE}" "${USER_NAME}" "${PASSWORD_ENV}" "${RAW}" <<SQL
SET HEADING OFF;
SET COUNT OFF;
SET ECHO OFF;
SET PLAN OFF;
SET STATS OFF;
SET TRANSACTION READ ONLY;
SET TERM ^ ;
EXECUTE BLOCK RETURNS (line VARCHAR(72))
AS
  DECLARE ref_id INTEGER;
  DECLARE typ INTEGER;
  DECLARE nm BLOB SUB_TYPE TEXT;
  DECLARE sm BLOB SUB_TYPE TEXT;
  DECLARE cd BLOB SUB_TYPE TEXT;
  DECLARE pos INTEGER;
  DECLARE n INTEGER;
  DECLARE take INTEGER;
  DECLARE piece VARCHAR(32);
  DECLARE tag VARCHAR(1);
  DECLARE which INTEGER;
  DECLARE payload BLOB SUB_TYPE TEXT;
BEGIN
  FOR SELECT query_ref, query_type_a28,
             COALESCE(name, ''),
             COALESCE(summary, ''),
             COALESCE(code, '')
      FROM queries
      WHERE ${WHERE}
      ORDER BY query_ref, query_type_a28
      INTO :ref_id, :typ, :nm, :sm, :cd
  DO
  BEGIN
    line = 'H|' || ref_id || '|' || typ;
    SUSPEND;
    which = 1;
    WHILE (which <= 3) DO
    BEGIN
      IF (which = 1) THEN
      BEGIN
        tag = 'N';
        payload = nm;
      END
      ELSE IF (which = 2) THEN
      BEGIN
        tag = 'S';
        payload = sm;
      END
      ELSE
      BEGIN
        tag = 'C';
        payload = cd;
      END
      n = CHAR_LENGTH(payload);
      IF (n IS NULL OR n = 0) THEN
      BEGIN
        line = tag || '|';
        SUSPEND;
      END
      ELSE
      BEGIN
        pos = 1;
        WHILE (pos <= n) DO
        BEGIN
          take = 32;
          IF (pos + take - 1 > n) THEN
            take = n - pos + 1;
          piece = SUBSTRING(payload FROM pos FOR take);
          WHILE (OCTET_LENGTH(piece) > 32 AND take > 1) DO
          BEGIN
            take = take - 1;
            piece = SUBSTRING(payload FROM pos FOR take);
          END
          line = tag || '|' || HEX_ENCODE(piece);
          SUSPEND;
          pos = pos + take;
        END
      END
      which = which + 1;
    END
  END
END
^
SET TERM ; ^
SQL
then
    exit 1
fi

ndjson="${WORK}/rows.ndjson"
: > "${ndjson}"
ref=""
typ=""
name_hex=""
sum_hex=""
code_hex=""
have=0

flush_row() {
    local nf sf cf
    if [[ "${have}" -eq 0 ]]; then
        return 0
    fi
    nf="${WORK}/name.txt"
    sf="${WORK}/summary.txt"
    cf="${WORK}/code.txt"
    printf '%s' "${name_hex}" | xxd -r -p > "${nf}"
    printf '%s' "${sum_hex}" | xxd -r -p > "${sf}"
    printf '%s' "${code_hex}" | xxd -r -p > "${cf}"
    jq -nc \
        --argjson ref "${ref}" \
        --argjson typ "${typ}" \
        --rawfile name "${nf}" \
        --rawfile summary "${sf}" \
        --rawfile code "${cf}" \
        '{query_ref:$ref, query_type:$typ, name:$name, summary:$summary, code:$code}' \
        >> "${ndjson}"
}

while IFS= read -r line || [[ -n "${line}" ]]; do
    # shellcheck disable=SC2311 # trim is a pure string edit
    line="$(schematool_fb_trim "${line}")"
    [[ -z "${line}" ]] && continue
    case "${line}" in
        H\|*)
            flush_row
            have=1
            name_hex=""
            sum_hex=""
            code_hex=""
            IFS='|' read -r _ ref typ <<< "${line}"
            ;;
        N\|*) name_hex+="${line#N|}" ;;
        S\|*) sum_hex+="${line#S|}" ;;
        C\|*) code_hex+="${line#C|}" ;;
        *)
            echo "Error: unrecognised firebird metadata line" >&2
            exit 1
            ;;
    esac
done < "${RAW}"
flush_row

jq -s '.' "${ndjson}"
exit 0
