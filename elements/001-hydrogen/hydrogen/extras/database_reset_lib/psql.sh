#!/usr/bin/env bash
# database_reset_lib/psql.sh — PostgreSQL and YugabyteDB reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides connection, counting, and schema-reset operations for
# PostgreSQL and YugabyteDB via psql.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2310,SC2311,SC2312 # CONN_* globals set in database_reset.sh; error handling in || and command substitution

psql_value() {
    local sql="$1"
    local dest value
    dest="$(make_tmp)"
    assert_no_drop_database "${sql}" || return 1
    set +e
    PGCONNECT_TIMEOUT=10 PGPASSWORD="${CONN_PASS}" \
        psql -X -q -h "${CONN_HOST}" -p "${CONN_PORT}" -U "${CONN_USER}" -d "${CONN_DATABASE}" \
        -v ON_ERROR_STOP=1 -t -A -c "${sql}" >"${dest}" 2>"${dest}.err"
    local rc=$?
    set -e
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${dest}.err" "${CONN_PASS}" >&2
        local err_text
        err_text="$(scrub_file "${dest}.err" "${CONN_PASS}")"
        if [[ "${err_text}" == *[Ll]ock* || "${err_text}" == *[Tt]imeout* ]]; then
            warn "Stop Hydrogen instances using this database and run the reset again."
        fi
        return 1
    fi
    value="$(trim "$(cat "${dest}")")"
    printf '%s\n' "${value}"
}

psql_do() {
    local sql="$1"
    local dest
    dest="$(make_tmp)"
    assert_no_drop_database "${sql}" || return 1
    set +e
    PGCONNECT_TIMEOUT=10 PGPASSWORD="${CONN_PASS}" \
        psql -X -q -h "${CONN_HOST}" -p "${CONN_PORT}" -U "${CONN_USER}" -d "${CONN_DATABASE}" \
        -v ON_ERROR_STOP=1 -c "${sql}" >"${dest}" 2>&1
    local rc=$?
    set -e
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    return 0
}

count_psql() {
    local n
    n="$(psql_value "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${CONN_SCHEMA}' AND table_type IN ('BASE TABLE', 'VIEW');")" || return 1
    if [[ ! "${n}" =~ ^[0-9]+$ ]]; then
        warn "Error: psql did not return a count"
        return 1
    fi
    printf '%s\n' "${n}"
}

reset_psql() {
    local ident sql
    ident="\"${CONN_SCHEMA}\""
    sql="$(cat <<SQL
SET lock_timeout = '20s';
SET statement_timeout = '180s';
DROP SCHEMA IF EXISTS ${ident} CASCADE;
CREATE SCHEMA ${ident};
SQL
)"
    psql_do "${sql}" || return 1
    local n
    n="$(count_psql)" || return 1
    [[ "${n}" -eq 0 ]]
}
