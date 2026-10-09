#!/usr/bin/env bash
# database_reset_lib/db2.sh — DB2 reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides connection management, counting, and schema-reset operations
# for DB2 via the db2 CLP.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2310,SC2312 # CONN_* and DB2_* globals set in database_reset.sh; error handling in || and loops

ensure_db2() {
    if command -v db2 >/dev/null 2>&1; then
        return 0
    fi
    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        set +u
        . /home/db2inst1/sqllib/db2profile
        set -u
    fi
    if ! command -v db2 >/dev/null 2>&1; then
        warn "Error: db2 client not found"
        return 1
    fi
    return 0
}

db2_connect() {
    local script dest rc pass_sql
    ensure_db2 || return 1
    if [[ "${DB2_OPEN}" -eq 1 ]]; then
        return 0
    fi
    if [[ "${CONN_PASS}" == *$'\n'* ]]; then
        warn "Error: DB2 password contains a newline"
        return 1
    fi
    script="$(make_tmp)"
    dest="$(make_tmp)"
    pass_sql="${CONN_PASS//\'/\'\'}"
    printf "CONNECT TO %s USER %s USING '%s';\n" "${CONN_DATABASE}" "${CONN_USER}" "${pass_sql}" > "${script}"
    assert_no_drop_database "$(cat "${script}")" || return 1
    set +e
    db2 -tvf "${script}" >"${dest}" 2>&1
    rc=$?
    set -e
    rm -f "${script}"
    if [[ "${rc}" -ge 8 ]]; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    DB2_OPEN=1
    return 0
}

db2_disconnect() {
    local rc
    if [[ "${DB2_OPEN}" -ne 1 ]]; then
        return 0
    fi
    set +e
    db2 connect reset >/dev/null 2>&1
    rc=$?
    set -e
    if [[ "${rc}" -ge 8 ]]; then
        warn "Error: DB2 connect reset failed"
        return 1
    fi
    DB2_OPEN=0
    return 0
}

db2_x() {
    local sql="$1"
    local dest="$2"
    local rc
    assert_no_drop_database "${sql}" || return 1
    set +e
    db2 -x "${sql}" >"${dest}" 2>&1
    rc=$?
    set -e
    if [[ "${rc}" -ge 8 ]] || grep -q -E '^SQL[0-9]{4}[NW] ' "${dest}"; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    return 0
}

count_db2_open() {
    local schema_u dest
    schema_u="${CONN_SCHEMA^^}"
    dest="$(make_tmp)"
    db2_x "SELECT COUNT(*) FROM SYSCAT.TABLES WHERE TABSCHEMA = '${schema_u}' AND TYPE IN ('T', 'V')" "${dest}" || return 1
    DB2_COUNT="$(read_count "${dest}")" || return 1
    return 0
}

count_db2() {
    local rc
    rc=0
    db2_connect || return 1
    count_db2_open || rc=1
    db2_disconnect || rc=1
    if [[ "${rc}" -ne 0 ]]; then
        return 1
    fi
    printf '%s\n' "${DB2_COUNT}"
}

db2_run_generated() {
    local select_sql="$1"
    local list script dest line stmt any rc hit
    list="$(make_tmp)"
    db2_x "${select_sql}" "${list}" || return 1
    script="$(make_tmp)"
    : > "${script}"
    any=0
    while IFS= read -r line || [[ -n "${line}" ]]; do
        stmt="$(trim "${line}")"
        [[ -z "${stmt}" ]] && continue
        if [[ "${stmt}" == SQL[0-9]* || "${stmt}" == *"SQLSTATE="* ]]; then
            warn "${stmt}"
            return 1
        fi
        assert_no_drop_database "${stmt}" || return 1
        printf '%s;\n' "${stmt}" >> "${script}"
        any=1
    done < "${list}"
    if [[ "${any}" -eq 0 ]]; then
        return 0
    fi
    dest="$(make_tmp)"
    set +e
    db2 -tvf "${script}" >"${dest}" 2>&1
    rc=$?
    set -e
    if grep -q -E "SQL1224N|SQL30081N|SQL1024N" "${dest}"; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        DB2_OPEN=0
        return 1
    fi
    if [[ "${rc}" -ge 8 ]]; then
        hit="$(grep "SQLSTATE=" "${dest}" | head -n 5 || true)"
        if [[ -n "${hit}" ]]; then
            scrub_text "${hit}" "${CONN_PASS}" >&2
        else
            scrub_file "${dest}" "${CONN_PASS}" >&2
        fi
    fi
    return 0
}

reset_db2_connected() {
    local schema_u pass_n n
    schema_u="${CONN_SCHEMA^^}"
    if ! ident_ok "${schema_u}"; then
        warn "Error: DB2 schema is not a plain identifier"
        return 1
    fi
    pass_n=0
    while [[ "${pass_n}" -lt 2 ]]; do
        count_db2_open || return 1
        n="${DB2_COUNT}"
        if [[ "${n}" -eq 0 ]]; then
            return 0
        fi
        db2_run_generated "SELECT 'ALTER TABLE \"' || RTRIM(TABSCHEMA) || '\".\"' || REPLACE(RTRIM(TABNAME), '\"', '\"\"') || '\" DROP CONSTRAINT \"' || REPLACE(RTRIM(CONSTNAME), '\"', '\"\"') || '\"' FROM SYSCAT.TABCONST WHERE TABSCHEMA = '${schema_u}' AND TYPE = 'F'" || return 1
        db2_run_generated "SELECT 'DROP SPECIFIC ' || CASE ROUTINETYPE WHEN 'P' THEN 'PROCEDURE ' ELSE 'FUNCTION ' END || '\"' || RTRIM(ROUTINESCHEMA) || '\".\"' || REPLACE(RTRIM(SPECIFICNAME), '\"', '\"\"') || '\"' FROM SYSCAT.ROUTINES WHERE ROUTINESCHEMA = '${schema_u}'" || return 1
        db2_run_generated "SELECT 'DROP VIEW \"' || RTRIM(TABSCHEMA) || '\".\"' || REPLACE(RTRIM(TABNAME), '\"', '\"\"') || '\"' FROM SYSCAT.TABLES WHERE TABSCHEMA = '${schema_u}' AND TYPE IN ('V', 'W')" || return 1
        db2_run_generated "SELECT 'DROP TABLE \"' || RTRIM(TABSCHEMA) || '\".\"' || REPLACE(RTRIM(TABNAME), '\"', '\"\"') || '\"' FROM SYSCAT.TABLES WHERE TABSCHEMA = '${schema_u}' AND TYPE IN ('T', 'U', 'S')" || return 1
        db2_run_generated "SELECT 'DROP SEQUENCE \"' || RTRIM(SEQSCHEMA) || '\".\"' || REPLACE(RTRIM(SEQNAME), '\"', '\"\"') || '\"' FROM SYSCAT.SEQUENCES WHERE SEQSCHEMA = '${schema_u}'" || return 1
        pass_n=$((pass_n + 1))
    done
    count_db2_open || return 1
    [[ "${DB2_COUNT}" -eq 0 ]]
}

reset_db2() {
    local rc
    rc=0
    db2_connect || return 1
    reset_db2_connected || rc=1
    db2_disconnect || rc=1
    return "${rc}"
}
