#!/usr/bin/env bash
# database_reset_lib/common.sh — shared helpers for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh. It provides logging,
# temporary-file management, text scrubbing, connection loading, target
# validation, and reporting utilities used by every engine backend.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2312 # scrub_file calls in command substitution; return values handled by callers.

# shellcheck disable=SC2034,SC2154 # globals set here, consumed by database_reset.sh and backend libs

warn() {
    printf '%s\n' "$*" >&2
}

make_tmp() {
    if [[ -z "${WORK_DIR}" ]]; then
        warn "Error: work directory is not ready"
        return 1
    fi
    mktemp "${WORK_DIR}/step.XXXXXX"
}

make_tmpdir() {
    if [[ -z "${WORK_DIR}" ]]; then
        warn "Error: work directory is not ready"
        return 1
    fi
    mktemp -d "${WORK_DIR}/dir.XXXXXX"
}

scrub_text() {
    local text="$1"
    local secret="$2"
    local out="" tmp
    if [[ -z "${secret}" || -z "${text}" ]]; then
        printf '%s\n' "${text}"
        return 0
    fi
    tmp="${text}"
    while [[ "${tmp}" == *"${secret}"* ]]; do
        out+="${tmp%%"${secret}"*}***"
        tmp="${tmp#*"${secret}"}"
    done
    out+="${tmp}"
    printf '%s\n' "${out}"
}

scrub_file() {
    local file="$1"
    local secret="$2"
    local text
    text="$(cat "${file}" 2>/dev/null || true)"
    scrub_text "${text}" "${secret}"
}

trim() {
    local s="$1"
    s="${s#"${s%%[![:space:]]*}"}"
    s="${s%"${s##*[![:space:]]}"}"
    printf '%s' "${s}"
}

ident_ok() {
    [[ "${1}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]
}

connection_name_ok() {
    [[ "${1}" =~ ^[A-Za-z_][A-Za-z0-9_-]*$ ]]
}

read_count() {
    local file="$1"
    local line n
    while IFS= read -r line || [[ -n "${line}" ]]; do
        n="$(trim "${line}")"
        [[ -z "${n}" ]] && continue
        if [[ "${n}" =~ ^[0-9]+$ ]]; then
            printf '%s\n' "${n}"
            return 0
        fi
        if [[ "${n}" == Msg\ * || "${n}" == *"SQLSTATE"* || "${n}" == Error:* || "${n}" == Sqlcmd:* ]]; then
            warn "Error: expected a numeric count"
            return 1
        fi
    done < "${file}"
    warn "Error: expected a numeric count"
    return 1
}

assert_no_drop_database() {
    local sql="$1"
    local upper="${sql^^}"
    if [[ "${upper}" == *"DROP DATABASE"* || "${upper}" == *"DROP DATABANK"* ]]; then
        warn "Error: refusing to drop a database"
        return 1
    fi
    return 0
}

expand_field() {
    local raw="$1"
    local name
    if [[ -z "${raw}" || "${raw}" == "null" ]]; then
        printf '%s' ""
        return 0
    fi
    if [[ "${raw}" =~ ^\$\{env\.([A-Za-z_][A-Za-z0-9_]*)\}$ ]]; then
        name="${BASH_REMATCH[1]}"
        if [[ -z "${!name+x}" ]]; then
            warn "Error: environment variable ${name} is not set"
            return 1
        fi
        printf '%s' "${!name}"
        return 0
    fi
    local env_token="\${env."
    if [[ "${raw}" == *"${env_token}"* ]]; then
        warn "Error: unsupported environment reference in a config value"
        return 1
    fi
    printf '%s' "${raw}"
}

normalize_engine() {
    local name="${1,,}"
    case "${name}" in
        postgres|postgresql) printf 'postgresql' ;;
        yugabyte|ydb|yugabytedb) printf 'yugabytedb' ;;
        mysql|mariadb|sqlite|db2|firebird|mssql) printf '%s' "${name}" ;;
        *)
            warn "Error: unknown engine '${1}'"
            return 1
            ;;
    esac
}

expected_schema() {
    local group="$1"
    local engine="$2"
    case "${engine}" in
        mssql)
            if [[ "${group}" == "test" ]]; then
                printf 'testms'
            else
                printf 'demoms'
            fi
            ;;
        sqlite|firebird)
            printf ''
            ;;
        *)
            if [[ "${group}" == "test" ]]; then
                printf 'test'
            else
                printf 'demo'
            fi
            ;;
    esac
}

engine_matches_config() {
    local engine="$1"
    local configured="$2"
    case "${engine}" in
        postgresql|yugabytedb)
            [[ "${configured}" == "postgresql" || "${configured}" == "postgres" ]]
            ;;
        *)
            [[ "${configured}" == "${engine}" ]]
            ;;
    esac
}

report() {
    local status="$1"
    local group="$2"
    local number="$3"
    local label="$4"
    local detail="$5"
    printf '%-8s %-4s %2s  %-12s %s\n' "${status}" "${group^^}" "${number}" "${label}" "${detail}"
}

load_connection() {
    local file="$1"
    local engine_raw host_raw port_raw database_raw user_raw pass_raw schema_raw
    if [[ ! -f "${file}" ]]; then
        warn "Error: missing config ${file}"
        return 1
    fi
    engine_raw="$(jq -r '.Databases.Connections[0].Engine // empty' "${file}")" || return 1
    host_raw="$(jq -r '.Databases.Connections[0].Host // empty' "${file}")" || return 1
    port_raw="$(jq -r '.Databases.Connections[0].Port // empty' "${file}")" || return 1
    database_raw="$(jq -r '.Databases.Connections[0].Database // empty' "${file}")" || return 1
    user_raw="$(jq -r '.Databases.Connections[0].User // empty' "${file}")" || return 1
    pass_raw="$(jq -r '.Databases.Connections[0].Pass // empty' "${file}")" || return 1
    schema_raw="$(jq -r '.Databases.Connections[0].Schema // empty' "${file}")" || return 1
    CONN_ENGINE="$(expand_field "${engine_raw}")" || return 1
    CONN_HOST="$(expand_field "${host_raw}")" || return 1
    CONN_PORT="$(expand_field "${port_raw}")" || return 1
    CONN_DATABASE="$(expand_field "${database_raw}")" || return 1
    CONN_USER="$(expand_field "${user_raw}")" || return 1
    CONN_PASS="$(expand_field "${pass_raw}")" || return 1
    CONN_SCHEMA="$(expand_field "${schema_raw}")" || return 1
    CONN_ENGINE="${CONN_ENGINE,,}"
    if [[ -n "${CONN_DATABASE}" && "${CONN_DATABASE}" != /* && "${CONN_DATABASE}" == tests/* ]]; then
        CONN_DATABASE="${HYDROGEN_ROOT}/${CONN_DATABASE}"
    fi
    return 0
}

location_text() {
    local engine="$1"
    case "${engine}" in
        sqlite|firebird)
            printf '%s' "${CONN_DATABASE}"
            ;;
        mysql|mariadb)
            printf '%s:%s database %s' "${CONN_HOST}" "${CONN_PORT}" "${CONN_SCHEMA}"
            ;;
        *)
            printf '%s:%s/%s schema %s' "${CONN_HOST}" "${CONN_PORT}" "${CONN_DATABASE}" "${CONN_SCHEMA}"
            ;;
    esac
}

validate_target() {
    local group="$1"
    local engine="$2"
    local expect base real_dir artifact_dir
    expect="$(expected_schema "${group}" "${engine}")"
    if [[ "${CONN_PASS}" == *$'\n'* ]]; then
        warn "Error: password contains a newline"
        return 1
    fi
    if [[ "${CONN_SCHEMA}" != "${expect}" ]]; then
        warn "Error: ${group} ${engine} schema is '${CONN_SCHEMA}', expected '${expect}'"
        return 1
    fi
    if ! engine_matches_config "${engine}" "${CONN_ENGINE}"; then
        warn "Error: config engine '${CONN_ENGINE}' does not match ${engine}"
        return 1
    fi
    case "${engine}" in
        sqlite)
            base="$(basename "${CONN_DATABASE}")"
            if [[ "${group}" == "test" && "${base}" != "hydrotst.sqlite" ]]; then
                warn "Error: test SQLite file must be hydrotst.sqlite"
                return 1
            fi
            if [[ "${group}" == "demo" && "${base}" != "hydrodemo.sqlite" ]]; then
                warn "Error: demo SQLite file must be hydrodemo.sqlite"
                return 1
            fi
            if [[ ! -f "${CONN_DATABASE}" ]]; then
                warn "Error: SQLite file is missing: ${CONN_DATABASE}"
                return 1
            fi
            real_dir="$(dirname "$(readlink -f "${CONN_DATABASE}")")" || return 1
            artifact_dir="$(readlink -f "${HYDROGEN_ROOT}/tests/artifacts/database/sqlite")" || return 1
            if [[ "${real_dir}" != "${artifact_dir}" ]]; then
                warn "Error: SQLite file is outside tests/artifacts/database/sqlite"
                return 1
            fi
            ;;
        firebird)
            base="$(basename "${CONN_DATABASE}")"
            if [[ "${group}" == "test" && "${base}" != "hydrogen_test.fdb" ]]; then
                warn "Error: test Firebird file must be hydrogen_test.fdb"
                return 1
            fi
            if [[ "${group}" == "demo" && "${base}" != "hydrogen_demo.fdb" ]]; then
                warn "Error: demo Firebird file must be hydrogen_demo.fdb"
                return 1
            fi
            if [[ ! -f "${CONN_DATABASE}" ]]; then
                warn "Error: Firebird file is missing: ${CONN_DATABASE}"
                return 1
            fi
            real_dir="$(dirname "$(readlink -f "${CONN_DATABASE}")")" || return 1
            artifact_dir="$(readlink -f "${HYDROGEN_ROOT}/tests/artifacts/database/firebird")" || return 1
            if [[ "${real_dir}" != "${artifact_dir}" ]]; then
                warn "Error: Firebird file is outside tests/artifacts/database/firebird"
                return 1
            fi
            if [[ -z "${CONN_USER}" || -z "${CONN_PASS}" ]]; then
                warn "Error: Firebird user and password are required"
                return 1
            fi
            ;;
        db2)
            if [[ -z "${CONN_DATABASE}" || -z "${CONN_USER}" || -z "${CONN_PASS}" ]]; then
                warn "Error: DB2 database, user, and password are required"
                return 1
            fi
            if ! ident_ok "${CONN_DATABASE}"; then
                warn "Error: DB2 database name is not a plain identifier"
                return 1
            fi
            ;;
        mysql|mariadb)
            if [[ -z "${CONN_HOST}" || -z "${CONN_PORT}" || -z "${CONN_USER}" || -z "${CONN_PASS}" ]]; then
                warn "Error: ${engine} host, port, user, and password are required"
                return 1
            fi
            if ! ident_ok "${CONN_SCHEMA}"; then
                warn "Error: ${engine} schema is not a plain identifier"
                return 1
            fi
            ;;
        postgresql|yugabytedb|mssql)
            if [[ -z "${CONN_HOST}" || -z "${CONN_PORT}" || -z "${CONN_DATABASE}" || -z "${CONN_USER}" || -z "${CONN_PASS}" ]]; then
                warn "Error: ${engine} host, port, database, user, and password are required"
                return 1
            fi
            if ! connection_name_ok "${CONN_DATABASE}" || ! ident_ok "${CONN_SCHEMA}"; then
                warn "Error: ${engine} database or schema is not a plain identifier"
                return 1
            fi
            ;;
        *)
            warn "Error: unsupported engine ${engine}"
            return 1
            ;;
    esac
    if [[ -n "${CONN_PORT}" && ! "${CONN_PORT}" =~ ^[0-9]+$ ]]; then
        warn "Error: port is not numeric"
        return 1
    fi
    return 0
}

selected_row() {
    local row="$1"
    local group engine
    IFS='|' read -r group _ engine _ _ <<< "${row}"
    if [[ -n "${GROUP_FILTER}" && "${group}" != "${GROUP_FILTER}" ]]; then
        return 1
    fi
    if [[ -n "${ENGINE_FILTER}" && "${engine}" != "${ENGINE_FILTER}" ]]; then
        return 1
    fi
    return 0
}
