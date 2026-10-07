#!/usr/bin/env bash
# SchemaTool MSSQL helpers — container check and sqlcmd JSON fetch.
#
# sqlcmd runs inside container philement-mssql and talks to localhost
# there. Adapters accept --host and --port so SchemaTool readiness
# matches the other server engines. Those flags are not a remote
# SQL Server connection.
#
# The SA password is handed to sqlcmd as SQLCMDPASSWORD inside the
# container script on stdin. It is not a podman or sqlcmd argument.
# Do not print that script. Callers scrub the password from errors.
#
# -y 0 keeps a long JSON cell on one line. Do not combine it with
# -w, -W, or -h (this sqlcmd rejects those pairs, and -w wraps).
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Phase 2 shared sqlcmd runner (SELECT via podman)

# shellcheck disable=SC2034 # constants read by callers

MSSQL_CONTAINER="philement-mssql"
MSSQL_SQLCMD="/opt/mssql-tools18/bin/sqlcmd"

schematool_mssql_ident_ok() {
    [[ "${1}" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]
}

schematool_mssql_ready() {
    local password_env="$1"
    if ! command -v podman >/dev/null 2>&1; then
        echo "Error: podman not found" >&2
        return 1
    fi
    if ! command -v jq >/dev/null 2>&1; then
        echo "Error: jq not found" >&2
        return 1
    fi
    # shellcheck disable=SC2312 # container lookup is the adapter precondition
    if ! podman ps --format '{{.Names}}' 2>/dev/null | grep -qx "${MSSQL_CONTAINER}"; then
        echo "Error: container ${MSSQL_CONTAINER} is not running" >&2
        return 1
    fi
    if [[ -z "${password_env}" ]]; then
        echo "Error: --password-env is required for mssql" >&2
        return 1
    fi
    if [[ -z "${!password_env+x}" || -z "${!password_env}" ]]; then
        echo "Error: password env var '${password_env}' is not set" >&2
        return 1
    fi
    return 0
}

# SQL text on stdin. sqlcmd stdout is written to dest.
# sqlcmd_flags defaults to -y 0 (one wide JSON cell).
schematool_mssql_run() {
    local password_env="$1"
    local user="$2"
    local database="$3"
    local dest="$4"
    local sqlcmd_flags="${5:--y 0}"
    local pass="${!password_env}"
    local err sqlfile scriptfile rc safe
    err="$(mktemp "${TMPDIR:-/tmp}/schematool_mssql_err.XXXXXX")"
    sqlfile="$(mktemp "${TMPDIR:-/tmp}/schematool_mssql_sql.XXXXXX")"
    scriptfile="$(mktemp "${TMPDIR:-/tmp}/schematool_mssql_sh.XXXXXX")"
    cat > "${sqlfile}"
    {
        printf 'export SQLCMDPASSWORD=%q\n' "${pass}"
        printf "exec %s -S localhost -U %q -C -d %q %s -b -i /dev/stdin <<'SCHEMATOOL_MSSQL_SQL_END'\n" \
            "${MSSQL_SQLCMD}" "${user}" "${database}" "${sqlcmd_flags}"
        cat "${sqlfile}"
        printf '\nSCHEMATOOL_MSSQL_SQL_END\n'
    } > "${scriptfile}"
    set +e
    podman exec -i "${MSSQL_CONTAINER}" bash -s < "${scriptfile}" >"${dest}" 2>"${err}"
    rc=$?
    set -e
    rm -f "${sqlfile}" "${scriptfile}"
    if [[ "${rc}" -ne 0 ]]; then
        safe="$(cat "${err}" "${dest}" 2>/dev/null || true)"
        safe="${safe//${pass}/***}"
        echo "Error: sqlcmd failed (container=${MSSQL_CONTAINER} db=${database})" >&2
        echo "${safe}" >&2
        rm -f "${err}"
        return 1
    fi
    rm -f "${err}"
    return 0
}

# Print one JSON value from a sqlcmd -y 0 capture.
schematool_mssql_emit_json() {
    local src="$1"
    local stripped cleaned
    stripped="$(mktemp "${TMPDIR:-/tmp}/schematool_mssql_strip.XXXXXX")"
    cleaned="$(mktemp "${TMPDIR:-/tmp}/schematool_mssql_clean.XXXXXX")"
    tr -d '\r' < "${src}" > "${stripped}"
    sed -E '/^[[:space:]]*\([0-9]+ rows affected\)[[:space:]]*$/d' "${stripped}" > "${cleaned}"
    rm -f "${stripped}"
    if ! jq -Rs 'gsub("\uFEFF";"") | gsub("^\\s+|\\s+$";"") | fromjson' < "${cleaned}"; then
        rm -f "${cleaned}"
        echo "Error: sqlcmd output was not JSON" >&2
        return 1
    fi
    rm -f "${cleaned}"
}
