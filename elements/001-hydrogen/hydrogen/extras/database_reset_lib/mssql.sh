#!/usr/bin/env bash
# database_reset_lib/mssql.sh — MS SQL Server reset backend for database_reset.sh.
#
# This library is sourced by extras/database_reset.sh after common.sh.
# It provides counting and schema-reset operations for MS SQL Server
# via sqlcmd inside the philement-mssql podman container.
#
# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from database_reset.sh
# TEST_VERSION: 1.0.0

# shellcheck disable=SC2154,SC2034,SC2310,SC2311,SC2312 # CONN_* globals set in database_reset.sh; error handling in || and command substitution

mssql_can() {
    if ! command -v podman >/dev/null 2>&1; then
        warn "Error: podman not found"
        return 1
    fi
    if ! podman ps --format '{{.Names}}' 2>/dev/null | grep -qx philement-mssql; then
        warn "Error: container philement-mssql is not running"
        return 1
    fi
    return 0
}

mssql_sqlcmd() {
    local sql="$1"
    local dest="$2"
    local flags="$3"
    local err sqlfile scriptfile rc
    assert_no_drop_database "${sql}" || return 1
    err="$(make_tmp)"
    sqlfile="$(make_tmp)"
    scriptfile="$(make_tmp)"
    printf '%s\n' "${sql}" > "${sqlfile}"
    {
        printf 'export SQLCMDPASSWORD=%q\n' "${CONN_PASS}"
        printf "exec /opt/mssql-tools18/bin/sqlcmd -S localhost -U %q -C -d %q %s -b -i /dev/stdin <<'DATABASE_RESET_MSSQL_SQL'\n" \
            "${CONN_USER}" "${CONN_DATABASE}" "${flags}"
        cat "${sqlfile}"
        printf '\nDATABASE_RESET_MSSQL_SQL\n'
    } > "${scriptfile}"
    set +e
    podman exec -i philement-mssql bash -s < "${scriptfile}" >"${dest}" 2>"${err}"
    rc=$?
    set -e
    rm -f "${sqlfile}" "${scriptfile}"
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${err}" "${CONN_PASS}" >&2
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    return 0
}

count_mssql() {
    local dest n
    mssql_can || return 1
    dest="$(make_tmp)"
    mssql_sqlcmd "$(cat <<SQL
SET NOCOUNT ON;
SELECT
  (SELECT COUNT(*) FROM sys.tables AS t
    WHERE SCHEMA_NAME(t.schema_id) = N'${CONN_SCHEMA}' AND t.is_ms_shipped = 0)
  + (SELECT COUNT(*) FROM sys.views AS v
    WHERE SCHEMA_NAME(v.schema_id) = N'${CONN_SCHEMA}');
SQL
)" "${dest}" "-h-1 -W" || return 1
    n="$(read_count "${dest}")" || {
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    }
    printf '%s\n' "${n}"
}

reset_mssql() {
    local dest n
    mssql_can || return 1
    dest="$(make_tmp)"
    mssql_sqlcmd "$(cat <<SQL
SET NOCOUNT ON;
SET XACT_ABORT ON;
DECLARE @schema sysname = N'${CONN_SCHEMA}';
DECLARE @sql nvarchar(max) = N'';

IF SCHEMA_ID(@schema) IS NULL
BEGIN
    RAISERROR('schema is missing; refusing to create a database', 16, 1);
    RETURN;
END

SELECT @sql = @sql + N'ALTER TABLE '
    + QUOTENAME(SCHEMA_NAME(t.schema_id)) + N'.' + QUOTENAME(t.name)
    + N' DROP CONSTRAINT ' + QUOTENAME(fk.name) + N';'
FROM sys.foreign_keys AS fk
JOIN sys.tables AS t ON t.object_id = fk.parent_object_id
WHERE SCHEMA_NAME(t.schema_id) = @schema;

SELECT @sql = @sql + N'DROP VIEW '
    + QUOTENAME(SCHEMA_NAME(v.schema_id)) + N'.' + QUOTENAME(v.name) + N';'
FROM sys.views AS v
WHERE SCHEMA_NAME(v.schema_id) = @schema;

SELECT @sql = @sql + N'DROP PROCEDURE '
    + QUOTENAME(SCHEMA_NAME(p.schema_id)) + N'.' + QUOTENAME(p.name) + N';'
FROM sys.procedures AS p
WHERE SCHEMA_NAME(p.schema_id) = @schema AND p.is_ms_shipped = 0;

SELECT @sql = @sql + N'DROP FUNCTION '
    + QUOTENAME(SCHEMA_NAME(o.schema_id)) + N'.' + QUOTENAME(o.name) + N';'
FROM sys.objects AS o
WHERE SCHEMA_NAME(o.schema_id) = @schema
  AND o.type IN (N'FN', N'IF', N'TF', N'FS', N'FT');

SELECT @sql = @sql + N'DROP SYNONYM '
    + QUOTENAME(SCHEMA_NAME(s.schema_id)) + N'.' + QUOTENAME(s.name) + N';'
FROM sys.synonyms AS s
WHERE SCHEMA_NAME(s.schema_id) = @schema;

SELECT @sql = @sql + N'DROP TABLE '
    + QUOTENAME(SCHEMA_NAME(t.schema_id)) + N'.' + QUOTENAME(t.name) + N';'
FROM sys.tables AS t
WHERE SCHEMA_NAME(t.schema_id) = @schema AND t.is_ms_shipped = 0;

SELECT @sql = @sql + N'DROP SEQUENCE '
    + QUOTENAME(SCHEMA_NAME(s.schema_id)) + N'.' + QUOTENAME(s.name) + N';'
FROM sys.sequences AS s
WHERE SCHEMA_NAME(s.schema_id) = @schema;

SELECT @sql = @sql + N'DROP TYPE '
    + QUOTENAME(SCHEMA_NAME(t.schema_id)) + N'.' + QUOTENAME(t.name) + N';'
FROM sys.types AS t
WHERE t.is_user_defined = 1 AND SCHEMA_NAME(t.schema_id) = @schema;

IF @sql <> N''
    EXEC sp_executesql @sql;
SQL
)" "${dest}" "-b" || return 1
    n="$(count_mssql)" || return 1
    [[ "${n}" -eq 0 ]]
}
