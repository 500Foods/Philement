#!/usr/bin/env bash
# SchemaTool DB common helpers — schema qualification and shared contracts
#
# CHANGELOG
# 1.1.0 - 2026-08-20 - HEX decode helper (xxd) for MySQL/DB2 adapters
# 1.0.0 - 2026-07-29 - Phase 3 shared helpers

# shellcheck disable=SC2034 # sourced variables used by callers

# Qualify queries table name for engine/schema.
# Empty schema → bare "queries" (SQLite). DB2 uppercases schema.
schematool_qualify_queries() {
    local engine="$1"
    local schema="$2"
    local table="queries"

    if [[ -z "${schema}" || "${schema}" == "." ]]; then
        printf '%s\n' "${table}"
        return 0
    fi

    case "${engine}" in
        db2)
            # DB2 identifiers often uppercase
            printf '%s.%s\n' "${schema^^}" "${table^^}"
            ;;
        *)
            printf '%s.%s\n' "${schema}" "${table}"
            ;;
    esac
}

# Resolve password from --password-env without printing it
schematool_resolve_password() {
    local password_env="$1"
    if [[ -z "${password_env}" ]]; then
        printf '%s\n' ""
        return 0
    fi
    if [[ -z "${!password_env+x}" ]]; then
        echo "Error: password env var '${password_env}' is not set" >&2
        return 1
    fi
    printf '%s\n' "${!password_env}"
}

# Apply ACURANZO_/CANVAS_/HYDROTST_ style fallbacks when flags empty
# Sets caller variables via namerefs when bash 4.3+; otherwise echo KEY=VAL lines
schematool_apply_env_fallbacks() {
    local engine="$1"
    case "${engine}" in
        postgresql)
            : "${SCHEMATOOL_FB_HOST:=${ACURANZO_DB_HOST:-}}"
            : "${SCHEMATOOL_FB_PORT:=${ACURANZO_DB_PORT:-}}"
            : "${SCHEMATOOL_FB_USER:=${ACURANZO_DB_USER:-}}"
            : "${SCHEMATOOL_FB_DATABASE:=${ACURANZO_DB_NAME:-}}"
            : "${SCHEMATOOL_FB_PASSWORD_ENV:=ACURANZO_DB_PASS}"
            ;;
        mysql)
            : "${SCHEMATOOL_FB_HOST:=${CANVAS_DB_HOST:-}}"
            : "${SCHEMATOOL_FB_PORT:=${CANVAS_DB_PORT:-}}"
            : "${SCHEMATOOL_FB_USER:=${CANVAS_DB_USER:-}}"
            : "${SCHEMATOOL_FB_DATABASE:=${CANVAS_DB_NAME:-}}"
            : "${SCHEMATOOL_FB_PASSWORD_ENV:=CANVAS_DB_PASS}"
            ;;
        db2)
            : "${SCHEMATOOL_FB_HOST:=localhost}"
            : "${SCHEMATOOL_FB_PORT:=}"
            : "${SCHEMATOOL_FB_USER:=${HYDROTST_DB_USER:-}}"
            : "${SCHEMATOOL_FB_DATABASE:=${HYDROTST_DB_NAME:-}}"
            : "${SCHEMATOOL_FB_PASSWORD_ENV:=HYDROTST_DB_PASS}"
            ;;
        sqlite)
            : "${SCHEMATOOL_FB_HOST:=}"
            : "${SCHEMATOOL_FB_PORT:=}"
            : "${SCHEMATOOL_FB_USER:=}"
            : "${SCHEMATOOL_FB_DATABASE:=}"
            : "${SCHEMATOOL_FB_PASSWORD_ENV:=}"
            ;;
        *)
            : "${SCHEMATOOL_FB_HOST:=}"
            : "${SCHEMATOOL_FB_PORT:=}"
            : "${SCHEMATOOL_FB_USER:=}"
            : "${SCHEMATOOL_FB_DATABASE:=}"
            : "${SCHEMATOOL_FB_PASSWORD_ENV:=}"
            ;;
    esac
}

schematool_require_xxd() {
    if ! command -v xxd >/dev/null 2>&1; then
        echo "Error: xxd not found (needed to decode HEX exports)" >&2
        return 1
    fi
}

# Decode a HEX string to dest file. Empty / NULL / invalid → empty file.
schematool_unhex_to_file() {
    local hex="$1"
    local dest="$2"
    hex="${hex//[$'\r\n']/}"
    hex="${hex// /}"
    if [[ -z "${hex}" || "${hex^^}" == "NULL" ]]; then
        : > "${dest}"
        return 0
    fi
    if [[ $(( ${#hex} % 2 )) -ne 0 ]]; then
        : > "${dest}"
        return 0
    fi
    if ! printf '%s' "${hex}" | xxd -r -p > "${dest}" 2>/dev/null; then
        : > "${dest}"
    fi
}
