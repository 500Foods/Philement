#!/usr/bin/env bash
# Shared mysql-protocol client for the MySQL and MariaDB entry points.
# The entry point passes the client name. This file does not choose an engine.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Query and catalog bodies moved out of the MySQL scripts

# shellcheck source=extras/schematool/db/common.sh # HEX decode helper
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

schematool_mysql_family_dump_queries() {
    local client="$1"
    shift

    local HOST="" PORT="3306" USER_NAME="" DATABASE="" SCHEMA="" PASSWORD_ENV=""
    local FROM_REF="" TO_REF="" QUALIFIED=""

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

    if ! command -v "${client}" >/dev/null 2>&1; then
        echo "Error: ${client} client not found" >&2
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
        echo "Error: --host, --user, and --database are required for ${client}" >&2
        exit 1
    fi

    # Schema name is the database name (demo).
    local DB_USE="${DATABASE}"
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

    local PASS=""
    if [[ -n "${PASSWORD_ENV}" ]]; then
        if [[ -z "${!PASSWORD_ENV+x}" ]]; then
            echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
            exit 1
        fi
        PASS="${!PASSWORD_ENV}"
    fi

    local WHERE="query_type_a28 BETWEEN 1000 AND 1003"
    if [[ -n "${FROM_REF}" ]]; then
        WHERE="${WHERE} AND query_ref >= ${FROM_REF}"
    fi
    if [[ -n "${TO_REF}" ]]; then
        WHERE="${WHERE} AND query_ref <= ${TO_REF}"
    fi

    local SQL
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

    local WORK
    WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_${client}.XXXXXX")
    # shellcheck disable=SC2064 # expand WORK now for EXIT trap
    trap "rm -rf \"${WORK}\"" EXIT
    local RAW="${WORK}/rows.tsv"
    local ERR="${WORK}/err.txt"

    set +e
    "${client}" -h "${HOST}" -P "${PORT}" -u "${USER_NAME}" -p"${PASS}" "${DB_USE}" \
        -N -B --raw -e "SET SESSION TRANSACTION READ ONLY; ${SQL}" >"${RAW}" 2>"${ERR}"
    local RC=$?
    set -e

    if [[ "${RC}" -ne 0 ]]; then
        local SAFE
        SAFE=$(cat "${ERR}")
        SAFE="${SAFE//${PASS}/***}"
        SAFE=$(printf '%s\n' "${SAFE}" | grep -v 'Using a password on the command line' || true)
        echo "Error: ${client} query failed (host=${HOST} port=${PORT} db=${DB_USE} table=${QUALIFIED})" >&2
        echo "${SAFE}" >&2
        exit 1
    fi

    if [[ ! -s "${RAW}" ]]; then
        echo "[]"
        exit 0
    fi

    local NDJSON="${WORK}/rows.ndjson"
    : > "${NDJSON}"
    local idx=0
    local ref_s typ_s name_h sum_h code_h nf sf cf
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
}

schematool_mysql_family_dump_catalog() {
    local client="$1"
    shift

    local HOST="" PORT="3306" USER_NAME="" DATABASE="" SCHEMA="" PASSWORD_ENV=""
    local TABLES_CSV=""

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

    if ! command -v "${client}" >/dev/null 2>&1; then
        echo "Error: ${client} client not found" >&2
        exit 1
    fi
    if ! command -v jq >/dev/null 2>&1; then
        echo "Error: jq not found" >&2
        exit 1
    fi

    if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
        echo "Error: --host, --user, and --database are required for ${client}" >&2
        exit 1
    fi

    local SCHEMA_USE="${SCHEMA}"
    if [[ -z "${SCHEMA_USE}" || "${SCHEMA_USE}" == "." ]]; then
        SCHEMA_USE="${DATABASE}"
    fi
    local DB_USE="${SCHEMA_USE}"

    local PASS=""
    if [[ -n "${PASSWORD_ENV}" ]]; then
        if [[ -z "${!PASSWORD_ENV+x}" ]]; then
            echo "Error: password env var '${PASSWORD_ENV}' is not set" >&2
            exit 1
        fi
        PASS="${!PASSWORD_ENV}"
    fi

    local TABLE_FILTER=""
    if [[ -n "${TABLES_CSV}" ]]; then
        local in_list="" raw t
        local -a TABLE_ARR
        IFS=',' read -r -a TABLE_ARR <<< "${TABLES_CSV}"
        for raw in "${TABLE_ARR[@]}"; do
            t="$(echo "${raw}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
            [[ -z "${t}" ]] && continue
            if [[ ! "${t}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
                echo "Error: invalid table name: ${t}" >&2
                exit 1
            fi
            if [[ -n "${in_list}" ]]; then
                in_list="${in_list},"
            fi
            in_list="${in_list}'${t}'"
        done
        if [[ -z "${in_list}" ]]; then
            echo "{\"schema\":\"${SCHEMA_USE}\",\"tables\":[]}"
            exit 0
        fi
        TABLE_FILTER="AND c.TABLE_NAME IN (${in_list})"
    fi

    local SCHEMA_SQL="${SCHEMA_USE//\'/\'\'}"
    local SQL
    SQL=$(cat <<EOF
SELECT
    c.TABLE_NAME,
    c.COLUMN_NAME,
    LOWER(c.DATA_TYPE),
    c.IS_NULLABLE,
    c.ORDINAL_POSITION,
    COALESCE((
        SELECT k.ORDINAL_POSITION
        FROM information_schema.KEY_COLUMN_USAGE k
        WHERE k.TABLE_SCHEMA = c.TABLE_SCHEMA
          AND k.TABLE_NAME = c.TABLE_NAME
          AND k.COLUMN_NAME = c.COLUMN_NAME
          AND k.CONSTRAINT_NAME = 'PRIMARY'
        LIMIT 1
    ), 0) AS pk_ord
FROM information_schema.COLUMNS c
JOIN information_schema.TABLES t
  ON t.TABLE_SCHEMA = c.TABLE_SCHEMA
 AND t.TABLE_NAME = c.TABLE_NAME
 AND t.TABLE_TYPE = 'BASE TABLE'
WHERE c.TABLE_SCHEMA = '${SCHEMA_SQL}'
  ${TABLE_FILTER}
ORDER BY c.TABLE_NAME, c.ORDINAL_POSITION;
EOF
)

    local WORK
    WORK=$(mktemp -d "${TMPDIR:-/tmp}/schematool_cat_${client}.XXXXXX")
    # shellcheck disable=SC2064 # expand WORK now for EXIT trap
    trap "rm -rf \"${WORK}\"" EXIT
    local RAW="${WORK}/cols.tsv"
    local ERR="${WORK}/err.txt"

    set +e
    "${client}" -h "${HOST}" -P "${PORT}" -u "${USER_NAME}" -p"${PASS}" "${DB_USE}" \
        -N -B --raw -e "SET SESSION TRANSACTION READ ONLY; ${SQL}" >"${RAW}" 2>"${ERR}"
    local RC=$?
    set -e

    if [[ "${RC}" -ne 0 ]]; then
        local SAFE
        SAFE=$(cat "${ERR}")
        SAFE="${SAFE//${PASS}/***}"
        SAFE=$(printf '%s\n' "${SAFE}" | grep -v 'Using a password on the command line' || true)
        echo "Error: ${client} catalog probe failed (host=${HOST} db=${DB_USE} schema=${SCHEMA_USE})" >&2
        echo "${SAFE}" >&2
        exit 1
    fi

    # shellcheck disable=SC2016 # jq program is single-quoted on purpose
    jq -n -c --arg schema "${SCHEMA_USE}" --rawfile raw "${RAW}" '
      def trim: gsub("^[[:space:]]+|[[:space:]]+$"; "");
      def as_int: tonumber? // 0;
      [
        $raw
        | gsub("\r"; "")
        | split("\n")
        | map(select(length > 0) | split("\t"))
        | map(select(length >= 6))
        | .[]
        | {
            tab: (.[0] | trim),
            col: (.[1] | trim),
            dtype: ((.[2] // "") | trim | ascii_downcase),
            nullable: ((.[3] // "") | trim | ascii_upcase == "YES"),
            ord: (.[4] | as_int),
            pk: (.[5] | as_int)
          }
      ]
      | group_by(.tab)
      | map({
          table: .[0].tab,
          columns: (sort_by(.ord) | map({
            name: .col,
            data_type: .dtype,
            nullable: .nullable,
            default: null
          })),
          primary_key: ([.[] | select(.pk > 0) | {o: .pk, n: .col}] | sort_by(.o) | map(.n)),
          indexes: []
        })
      | {schema: $schema, tables: .}
    '
    exit 0
}
