#!/usr/bin/env bash
# database_reset_lib/dispatch.sh — engine dispatch for count_target/reset_target.
#
# This library is sourced by extras/database_reset.sh after common.sh and
# all engine backends. It routes count and reset operations to the correct
# engine implementation.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

count_target() {
    local engine="$1"
    case "${engine}" in
        postgresql|yugabytedb) count_psql ;;
        mysql|mariadb) count_mysql "${engine}" ;;
        db2) count_db2 ;;
        sqlite) count_sqlite ;;
        firebird) count_firebird ;;
        mssql) count_mssql ;;
        *)
            warn "Error: unsupported engine ${engine}"
            return 1
            ;;
    esac
}

reset_target() {
    local engine="$1"
    case "${engine}" in
        postgresql|yugabytedb) reset_psql ;;
        mysql|mariadb) reset_mysql "${engine}" ;;
        db2) reset_db2 ;;
        sqlite) reset_sqlite ;;
        firebird) reset_firebird ;;
        mssql) reset_mssql ;;
        *)
            warn "Error: unsupported engine ${engine}"
            return 1
            ;;
    esac
}
