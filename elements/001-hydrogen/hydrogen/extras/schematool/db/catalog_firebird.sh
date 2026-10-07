#!/usr/bin/env bash
# SchemaTool Firebird live catalog probe — RDB$RELATIONS / RDB$RELATION_FIELDS.
#
# Prints JSON: { "schema": "", "tables": [ { table, columns[], primary_key[] } ] }
# Field types are mapped to the migration SQL spelling, lowercased.
# SELECT only. Password is ISC_PASSWORD, never a process argument.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Phase 1 Firebird catalog probe

set -euo pipefail

# shellcheck source=extras/schematool/db/firebird_common.sh # isql runner and type map
source "$(dirname "${BASH_SOURCE[0]}")/firebird_common.sh"

HOST=""
PORT=""
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

: "${HOST}" "${PORT}" "${SCHEMA}"

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found" >&2
    exit 1
fi
if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for firebird" >&2
    exit 1
fi

FILTER_SQL=""
REQUESTED=""
if [[ -n "${TABLES_CSV}" ]]; then
    in_list=""
    IFS=',' read -r -a table_arr <<< "${TABLES_CSV}"
    for raw in "${table_arr[@]+"${table_arr[@]}"}"; do
        # shellcheck disable=SC2311 # trim is a pure string edit
        t="$(schematool_fb_trim "${raw}")"
        [[ -z "${t}" ]] && continue
        if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
            echo "Error: invalid table name: ${t}" >&2
            exit 1
        fi
        if [[ -n "${REQUESTED}" ]]; then
            REQUESTED="${REQUESTED},"
        fi
        REQUESTED="${REQUESTED}${t}"
        if [[ -n "${in_list}" ]]; then
            in_list="${in_list},"
        fi
        in_list="${in_list}'${t^^}'"
    done
    if [[ -z "${in_list}" ]]; then
        jq -nc --arg schema "" '{schema:$schema, tables:[]}'
        exit 0
    fi
    FILTER_SQL="AND TRIM(rel.RDB\$RELATION_NAME) IN (${in_list})"
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/schematool_fb_catalog.XXXXXX")"
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
  DECLARE rel VARCHAR(63);
  DECLARE fld VARCHAR(63);
  DECLARE ftype INTEGER;
  DECLARE fsub INTEGER;
  DECLARE clen INTEGER;
  DECLARE prec INTEGER;
  DECLARE sc INTEGER;
  DECLARE nflag INTEGER;
  DECLARE ident INTEGER;
  DECLARE pkpos INTEGER;
BEGIN
  FOR SELECT TRIM(rel.RDB\$RELATION_NAME), TRIM(rf.RDB\$FIELD_NAME),
             f.RDB\$FIELD_TYPE,
             COALESCE(f.RDB\$FIELD_SUB_TYPE, 0),
             COALESCE(f.RDB\$CHARACTER_LENGTH, 0),
             COALESCE(f.RDB\$FIELD_PRECISION, 0),
             COALESCE(f.RDB\$FIELD_SCALE, 0),
             COALESCE(rf.RDB\$NULL_FLAG, 0),
             COALESCE(rf.RDB\$IDENTITY_TYPE, -1)
      FROM RDB\$RELATION_FIELDS rf
      JOIN RDB\$FIELDS f ON f.RDB\$FIELD_NAME = rf.RDB\$FIELD_SOURCE
      JOIN RDB\$RELATIONS rel ON rel.RDB\$RELATION_NAME = rf.RDB\$RELATION_NAME
      WHERE rel.RDB\$SYSTEM_FLAG = 0
        AND rel.RDB\$VIEW_BLR IS NULL
        ${FILTER_SQL}
      ORDER BY rel.RDB\$RELATION_NAME, rf.RDB\$FIELD_POSITION
      INTO :rel, :fld, :ftype, :fsub, :clen, :prec, :sc, :nflag, :ident
  DO
  BEGIN
    line = 'T|' || rel;
    SUSPEND;
    line = 'C|' || fld;
    SUSPEND;
    line = 'M|' || ftype || '|' || fsub || '|' || clen || '|' || prec
        || '|' || sc || '|' || nflag || '|' || ident;
    SUSPEND;
  END
  FOR SELECT TRIM(rc.RDB\$RELATION_NAME), TRIM(seg.RDB\$FIELD_NAME),
             seg.RDB\$FIELD_POSITION
      FROM RDB\$RELATION_CONSTRAINTS rc
      JOIN RDB\$INDEX_SEGMENTS seg ON seg.RDB\$INDEX_NAME = rc.RDB\$INDEX_NAME
      JOIN RDB\$RELATIONS rel ON rel.RDB\$RELATION_NAME = rc.RDB\$RELATION_NAME
      WHERE rc.RDB\$CONSTRAINT_TYPE = 'PRIMARY KEY'
        AND rel.RDB\$SYSTEM_FLAG = 0
        AND rel.RDB\$VIEW_BLR IS NULL
        ${FILTER_SQL}
      ORDER BY rc.RDB\$RELATION_NAME, seg.RDB\$FIELD_POSITION
      INTO :rel, :fld, :pkpos
  DO
  BEGIN
    line = 'T|' || rel;
    SUSPEND;
    line = 'K|' || fld || '|' || pkpos;
    SUSPEND;
  END
END
^
SET TERM ; ^
SQL
then
    exit 1
fi

schematool_fb_catalog_json "${REQUESTED}" < "${RAW}"
exit 0
