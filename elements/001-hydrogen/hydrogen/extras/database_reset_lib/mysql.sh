#!/usr/bin/env bash
# database_reset_lib/mysql.sh — MySQL and MariaDB reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides connection, counting, and schema-emptying operations for
# MySQL and MariaDB via the mysql/mariadb client.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2310,SC2311,SC2312 # CONN_* and DB2_* globals set in database_reset.sh; error handling in || and command substitution

write_mysql_defaults() {
    local dest="$1"
    local pass="$2"
    local escaped
    if [[ "${pass}" == *$'\n'* ]]; then
        warn "Error: password contains a newline"
        return 1
    fi
    escaped="${pass//\\/\\\\}"
    escaped="${escaped//\"/\\\"}"
    {
        printf '[client]\n'
        printf 'password="%s"\n' "${escaped}"
    } > "${dest}"
    chmod 600 "${dest}"
}

mysql_bin() {
    local engine="$1"
    if [[ "${engine}" == "mariadb" ]] && command -v mariadb >/dev/null 2>&1; then
        printf 'mariadb'
        return 0
    fi
    if command -v mysql >/dev/null 2>&1; then
        printf 'mysql'
        return 0
    fi
    if command -v mariadb >/dev/null 2>&1; then
        printf 'mariadb'
        return 0
    fi
    warn "Error: mysql client not found"
    return 1
}

mysql_run() {
    local engine="$1"
    local sql_file="$2"
    local dest="$3"
    local client cnf err rc
    client="$(mysql_bin "${engine}")" || return 1
    cnf="$(make_tmp)"
    err="$(make_tmp)"
    write_mysql_defaults "${cnf}" "${CONN_PASS}" || return 1
    assert_no_drop_database "$(cat "${sql_file}")" || return 1
    set +e
    "${client}" --defaults-file="${cnf}" --protocol=TCP --connect-timeout=10 \
        -h "${CONN_HOST}" -P "${CONN_PORT}" -u "${CONN_USER}" \
        --batch --raw --skip-column-names --silent \
        < "${sql_file}" >"${dest}" 2>"${err}"
    rc=$?
    set -e
    rm -f "${cnf}"
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${err}" "${CONN_PASS}" >&2
        if [[ "${engine}" == "mysql" || "${engine}" == "mariadb" ]]; then
            local err_text
            err_text="$(scrub_file "${err}" "${CONN_PASS}")"
            if [[ "${err_text}" == *[Ll]ock* || "${err_text}" == *[Tt]imeout* ]]; then
                warn "Stop Hydrogen instances using this database and run the reset again."
            fi
        fi
        return 1
    fi
    return 0
}

mysql_scalar() {
    local engine="$1"
    local sql="$2"
    local sql_file dest value
    sql_file="$(make_tmp)"
    dest="$(make_tmp)"
    printf '%s\n' "${sql}" > "${sql_file}"
    mysql_run "${engine}" "${sql_file}" "${dest}" || return 1
    value="$(trim "$(cat "${dest}")")"
    if [[ ! "${value}" =~ ^[0-9]+$ ]]; then
        warn "Error: ${engine} did not return a count"
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    printf '%s\n' "${value}"
}

mysql_database_exists() {
    local engine="$1"
    local n
    n="$(mysql_scalar "${engine}" "SELECT COUNT(*) FROM information_schema.schemata WHERE schema_name = '${CONN_SCHEMA}';")" || return 1
    if [[ "${n}" -eq 0 ]]; then
        warn "Error: ${engine} database ${CONN_SCHEMA} does not exist. Refusing to create it."
        return 1
    fi
    return 0
}

count_mysql() {
    local engine="$1"
    mysql_database_exists "${engine}" || return 1
    mysql_scalar "${engine}" "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${CONN_SCHEMA}' AND table_type IN ('BASE TABLE', 'VIEW');"
}

reset_mysql() {
    local engine="$1"
    local pass_n=0
    local n sql_file list dest kind name qname qschema
    qschema="${CONN_SCHEMA//\`/\`\`}"
    mysql_database_exists "${engine}" || return 1
    while [[ "${pass_n}" -lt 3 ]]; do
        n="$(count_mysql "${engine}")" || return 1
        if [[ "${n}" -eq 0 ]]; then
            break
        fi
        sql_file="$(make_tmp)"
        list="$(make_tmp)"
        dest="$(make_tmp)"
        {
            printf 'SELECT CASE table_type WHEN '\''VIEW'\'' THEN '\''VIEW'\'' WHEN '\''SEQUENCE'\'' THEN '\''SEQUENCE'\'' ELSE '\''TABLE'\'' END, table_name\n'
            printf 'FROM information_schema.tables\n'
            printf 'WHERE table_schema = '\''%s'\''\n' "${CONN_SCHEMA}"
            printf '  AND table_type IN ('\''BASE TABLE'\'', '\''VIEW'\'', '\''SEQUENCE'\'')\n'
            printf 'ORDER BY CASE table_type WHEN '\''VIEW'\'' THEN 0 WHEN '\''SEQUENCE'\'' THEN 1 ELSE 2 END, table_name;\n'
        } > "${list}"
        mysql_run "${engine}" "${list}" "${dest}" || return 1
        {
            printf 'SET SESSION lock_wait_timeout = 30;\n'
            printf 'SET SESSION innodb_lock_wait_timeout = 30;\n'
            printf 'SET SESSION foreign_key_checks = 0;\n'
            while IFS=$'\t' read -r kind name || [[ -n "${kind:-}" ]]; do
                kind="$(trim "${kind}")"
                name="$(trim "${name}")"
                [[ -z "${kind}" || -z "${name}" ]] && continue
                if [[ ! "${kind}" =~ ^(VIEW|TABLE|SEQUENCE)$ ]]; then
                    warn "Error: unexpected ${engine} object kind ${kind}"
                    return 1
                fi
                if [[ "${name}" == *\`* || "${name}" == *';'* || "${name}" == *$'\n'* ]]; then
                    warn "Error: refusing to drop ${engine} object with an unsafe name"
                    return 1
                fi
                qname="${name//\`/\`\`}"
                printf 'DROP %s IF EXISTS %s%s%s.%s%s%s;\n' "${kind}" '`' "${qschema}" '`' '`' "${qname}" '`'
            done < "${dest}"
            printf 'SET SESSION foreign_key_checks = 1;\n'
        } > "${sql_file}"
        assert_no_drop_database "$(cat "${sql_file}")" || return 1
        mysql_run "${engine}" "${sql_file}" "$(make_tmp)" || return 1
        pass_n=$((pass_n + 1))
    done
    sql_file="$(make_tmp)"
    list="$(make_tmp)"
    dest="$(make_tmp)"
    printf 'SELECT routine_type, routine_name FROM information_schema.routines WHERE routine_schema = '\''%s'\'';\n' "${CONN_SCHEMA}" > "${list}"
    mysql_run "${engine}" "${list}" "${dest}" || return 1
    {
        while IFS=$'\t' read -r kind name || [[ -n "${kind:-}" ]]; do
            kind="$(trim "${kind}")"
            name="$(trim "${name}")"
            [[ -z "${kind}" || -z "${name}" ]] && continue
            if [[ "${kind}" != "FUNCTION" && "${kind}" != "PROCEDURE" ]]; then
                continue
            fi
            if [[ "${name}" == *\`* || "${name}" == *';'* ]]; then
                warn "Error: refusing to drop ${engine} routine with an unsafe name"
                return 1
            fi
            qname="${name//\`/\`\`}"
            printf 'DROP %s IF EXISTS %s%s%s.%s%s%s;\n' "${kind}" '`' "${qschema}" '`' '`' "${qname}" '`'
        done < "${dest}"
    } > "${sql_file}"
    if [[ -s "${sql_file}" ]]; then
        mysql_run "${engine}" "${sql_file}" "$(make_tmp)" || return 1
    fi
    n="$(count_mysql "${engine}")" || return 1
    [[ "${n}" -eq 0 ]]
}
