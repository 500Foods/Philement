#!/usr/bin/env bash
# SchemaTool DB2 live catalog probe — SYSCAT.COLUMNS (filtered)
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Phase 7a DB2 catalog probe

set -euo pipefail

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

: "${HOST}" "${PORT}"

if ! command -v db2 >/dev/null 2>&1; then
    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        # shellcheck disable=SC1091 # host-specific DB2 profile path not in repo
        . /home/db2inst1/sqllib/db2profile
    fi
fi
if ! command -v db2 >/dev/null 2>&1; then
    echo "Error: db2 client not found" >&2
    exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 not found" >&2
    exit 1
fi

if [[ -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
    echo "Error: --user and --database are required for db2" >&2
    exit 1
fi
if [[ -z "${SCHEMA}" || "${SCHEMA}" == "." ]]; then
    echo "Error: --schema is required for db2 catalog probe" >&2
    exit 1
fi

PASS=""
if [[ -n "${PASSWORD_ENV}" ]]; then
    if [[ -z "${!PASSWORD_ENV+x}" ]]; then
        echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
        exit 1
    fi
    PASS="${!PASSWORD_ENV}"
fi

SCHEMA_U="${SCHEMA^^}"
SCHEMA_SQL="${SCHEMA_U//\'/\'\'}"

TABLE_FILTER=""
if [[ -n "${TABLES_CSV}" ]]; then
    in_list=""
    IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
    for raw in "${TABLE_ARR[@]}"; do
        t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [[ -z "${t}" ]] && continue
        if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
            echo "Error: invalid table name: ${t}" >&2
            exit 1
        fi
        tu="${t^^}"
        if [[ -n "${in_list}" ]]; then
            in_list="${in_list},"
        fi
        in_list="${in_list}'${tu}'"
    done
    if [[ -z "${in_list}" ]]; then
        echo "{\"schema\":\"${SCHEMA_U}\",\"tables\":[]}"
        exit 0
    fi
    TABLE_FILTER="AND TABNAME IN (${in_list})"
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_cat_db2.XXXXXX")
# shellcheck disable=SC2064 # expand WORK now so EXIT trap removes this temp dir
trap "rm -rf \"${WORK}\"" EXIT

CONN_SCRIPT="${WORK}/connect.sql"
PASS_SQL="${PASS//\'/\'\'}"
{
    echo "CONNECT TO ${DATABASE} USER ${USER_NAME} USING '${PASS_SQL}';"
} > "${CONN_SCRIPT}"

# Export column catalog as DEL
EXPORT_FILE="${WORK}/cols.del"
SQL_EXPORT="EXPORT TO '${EXPORT_FILE}' OF DEL
SELECT TABNAME, COLNAME, TYPENAME, NULLS, DEFAULT, KEYSEQ, COLNO
FROM SYSCAT.COLUMNS
WHERE TABSCHEMA = '${SCHEMA_SQL}'
  ${TABLE_FILTER}
ORDER BY TABNAME, COLNO;"

set +e
db2 -tvf "${CONN_SCRIPT}" >"${WORK}/conn.out" 2>&1
db2 -tvf /dev/stdin >"${WORK}/export.out" 2>&1 <<EOF
${SQL_EXPORT}
EOF
RC=$?
db2 connect reset >/dev/null 2>&1 || true
rm -f "${CONN_SCRIPT}"
set -e

if [[ "${RC}" -ne 0 && ! -f "${EXPORT_FILE}" ]]; then
    SAFE=$(sed "s/${PASS}/***/g" "${WORK}/export.out" 2>/dev/null || true)
    echo "Error: db2 catalog export failed" >&2
    echo "${SAFE}" >&2
    exit 1
fi

python3 - "${EXPORT_FILE}" "${SCHEMA_U}" <<'PY'
import csv, json, sys
path, schema = sys.argv[1], sys.argv[2]
tables = {}
order = []
try:
    with open(path, newline="") as f:
        reader = csv.reader(f)
        for row in reader:
            if len(row) < 7:
                continue
            tab, col, typ, nulls, default, keyseq, colno = row[:7]
            tab = tab.strip()
            col = col.strip()
            typ = (typ or "").strip().lower()
            nullable = (nulls or "").strip().upper() == "Y"
            try:
                ks = int(keyseq) if keyseq not in ("", None) else 0
            except ValueError:
                ks = 0
            try:
                cno = int(colno) if colno not in ("", None) else 0
            except ValueError:
                cno = 0
            if tab not in tables:
                tables[tab] = {"table": tab.lower(), "columns": [], "primary_key": [], "indexes": [], "_pk": []}
                order.append(tab)
            tables[tab]["columns"].append({
                "name": col.lower(),
                "data_type": typ.lower(),
                "nullable": nullable,
                "default": default if default not in ("", None) else None,
                "_colno": cno,
            })
            if ks and ks > 0:
                tables[tab]["_pk"].append((ks, col.lower()))
except FileNotFoundError:
    pass

out_tables = []
for tab in order:
    t = tables[tab]
    t["columns"].sort(key=lambda c: c.get("_colno", 0))
    for c in t["columns"]:
        c.pop("_colno", None)
    t["_pk"].sort(key=lambda x: x[0])
    t["primary_key"] = [n for _, n in t["_pk"]]
    t.pop("_pk", None)
    out_tables.append(t)

print(json.dumps({"schema": schema, "tables": out_tables}, separators=(",", ":")))
PY

exit 0
