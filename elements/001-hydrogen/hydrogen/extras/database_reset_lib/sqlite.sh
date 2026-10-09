#!/usr/bin/env bash
# database_reset_lib/sqlite.sh — SQLite reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides counting and schema-reset operations for SQLite via sqlite3.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2312 # CONN_* globals set in database_reset.sh; scrub_file in command substitution

sqlite3_bin() {
    sqlite3 -batch -init /dev/null "$@"
}

count_sqlite() {
    local n
    if ! command -v sqlite3 >/dev/null 2>&1; then
        warn "Error: sqlite3 not found"
        return 1
    fi
    n="$(sqlite3_bin "${CONN_DATABASE}" "SELECT COUNT(*) FROM sqlite_master WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite_%';")" || return 1
    n="$(trim "${n}")"
    if [[ ! "${n}" =~ ^[0-9]+$ ]]; then
        warn "Error: sqlite3 did not return a count"
        return 1
    fi
    printf '%s\n' "${n}"
}

reset_sqlite() {
    local list sql_file line kind name ident pass_n n
    pass_n=0
    while [[ "${pass_n}" -lt 3 ]]; do
        n="$(count_sqlite)" || return 1
        if [[ "${n}" -eq 0 ]]; then
            break
        fi
        list="$(sqlite3_bin "${CONN_DATABASE}" "SELECT type || ' ' || name FROM sqlite_master WHERE type IN ('trigger', 'view', 'table') AND name NOT LIKE 'sqlite_%' ORDER BY CASE type WHEN 'trigger' THEN 0 WHEN 'view' THEN 1 ELSE 2 END, name;")" || return 1
        sql_file="$(make_tmp)"
        {
            printf 'PRAGMA busy_timeout = 5000;\n'
            printf 'PRAGMA foreign_keys = OFF;\n'
            printf 'BEGIN IMMEDIATE;\n'
            while IFS= read -r line || [[ -n "${line}" ]]; do
                [[ -z "${line}" ]] && continue
                kind="${line%% *}"
                name="${line#* }"
                if [[ "${name}" == *'"'* || "${name}" == *';'* ]]; then
                    warn "Error: refusing to drop SQLite object with an unsafe name"
                    return 1
                fi
                ident="\"${name}\""
                case "${kind}" in
                    trigger) printf 'DROP TRIGGER IF EXISTS %s;\n' "${ident}" ;;
                    view) printf 'DROP VIEW IF EXISTS %s;\n' "${ident}" ;;
                    table) printf 'DROP TABLE IF EXISTS %s;\n' "${ident}" ;;
                    *)
                        warn "Error: unexpected SQLite object kind ${kind}"
                        return 1
                        ;;
                esac
            done <<< "${list}"
            printf 'COMMIT;\n'
        } > "${sql_file}"
        assert_no_drop_database "$(cat "${sql_file}")" || return 1
        if ! sqlite3_bin "${CONN_DATABASE}" < "${sql_file}"; then
            warn "Error: sqlite3 drop failed for ${CONN_DATABASE}"
            warn "Stop Hydrogen instances using this file and run the reset again."
            return 1
        fi
        pass_n=$((pass_n + 1))
    done
    if ! sqlite3_bin "${CONN_DATABASE}" "VACUUM;"; then
        warn "Warning: VACUUM failed for ${CONN_DATABASE}"
    fi
    n="$(count_sqlite)" || return 1
    [[ "${n}" -eq 0 ]]
}
