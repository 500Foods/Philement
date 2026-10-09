#!/usr/bin/env bash
# shellcheck disable=SC2310,SC2312 # Each engine reports its own status and the loop continues.
# database_reset.sh — empty the build schemas used by migration tests 32-39
# and auth test 40.
#
# The databases, schema names, and database files stay. The build expects
# them to exist. What goes away is the tables, views, and routines inside
# the migration schema, so the next AutoMigration starts from empty instead
# of resuming at the last applied migration.
#
#   tests 32-39   schema test   (MSSQL testms, SQLite hydrotst.sqlite,
#                 Firebird hydrogen_test.fdb)
#   test 40       schema demo   (MSSQL demoms, SQLite hydrodemo.sqlite,
#                 Firebird hydrogen_demo.fdb)
#
# Connection settings come from tests/configs/hydrogen_test_3*.json and
# hydrogen_test_40_*.json. Passwords stay in the environment those files
# already reference. Nothing here drops a database.
#
# CHANGELOG
# 1.0.1 - 2026-10-08 - Ignore ~/.my.cnf and ~/.sqliterc when counting and resetting
# 1.0.0 - 2026-10-07 - Empty test and demo build schemas for tests 32-40
# TEST_VERSION: 1.0.1

set -euo pipefail
set +H
umask 077

GROUP_FILTER=""
ENGINE_FILTER=""
DRY_RUN=0
ASSUME_YES=0
FAILS=0
WORK_DIR=""
DB2_OPEN=0
DB2_COUNT=""

TARGETS=(
    "test|32|postgresql|PostgreSQL|hydrogen_test_32_postgres.json"
    "test|33|mysql|MySQL|hydrogen_test_33_mysql.json"
    "test|34|sqlite|SQLite|hydrogen_test_34_sqlite.json"
    "test|35|db2|DB2|hydrogen_test_35_db2.json"
    "test|36|mariadb|MariaDB|hydrogen_test_36_mariadb.json"
    "test|37|firebird|Firebird|hydrogen_test_37_firebird.json"
    "test|38|yugabytedb|YugabyteDB|hydrogen_test_38_yugabytedb.json"
    "test|39|mssql|MSSQL|hydrogen_test_39_mssql.json"
    "demo|40|postgresql|PostgreSQL|hydrogen_test_40_postgres.json"
    "demo|40|mysql|MySQL|hydrogen_test_40_mysql.json"
    "demo|40|sqlite|SQLite|hydrogen_test_40_sqlite.json"
    "demo|40|db2|DB2|hydrogen_test_40_db2.json"
    "demo|40|mariadb|MariaDB|hydrogen_test_40_mariadb.json"
    "demo|40|firebird|Firebird|hydrogen_test_40_firebird.json"
    "demo|40|yugabytedb|YugabyteDB|hydrogen_test_40_yugabytedb.json"
    "demo|40|mssql|MSSQL|hydrogen_test_40_mssql.json"
)

usage() {
    cat <<'EOF'
Usage: database_reset.sh [--dry-run] [--yes] [--group test|demo] [--engine name]

Empty the build schemas used by tests 32-39 (test) and test 40 (demo).
Tables, views, and routines inside those schemas are removed. The
databases and database files are not dropped.

  --dry-run       Show each target and its table/view count. Change nothing.
  --yes           Skip the RESET confirmation.
  --group test    Only the eight migration-test schemas (tests 32-39).
  --group demo    Only the eight test 40 schemas.
  --engine name   One engine: postgresql, mysql, sqlite, db2, mariadb,
                  firebird, yugabytedb, mssql.

Credentials are the environment variables already referenced by the
test JSON configs. Stop Hydrogen before running this so open transactions
do not block the drops.
EOF
}

# shellcheck disable=SC2329 # EXIT trap removes the work directory and the DB2 connection
cleanup() {
    if [[ "${DB2_OPEN}" -eq 1 ]] && command -v db2 >/dev/null 2>&1; then
        db2 connect reset >/dev/null 2>&1 || true
        DB2_OPEN=0
    fi
    if [[ -n "${WORK_DIR}" && -d "${WORK_DIR}" ]]; then
        rm -rf "${WORK_DIR}"
        WORK_DIR=""
    fi
}
trap cleanup EXIT

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

# Database names are client arguments, not SQL identifiers. Yugabyte's
# build database contains hyphens (t-500nodes-db).
connection_name_ok() {
    [[ "${1}" =~ ^[A-Za-z_][A-Za-z0-9_-]*$ ]]
}

# First line that is only digits. Blank lines and client chatter are skipped.
# A line that looks like a server error fails the count.
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

# Load connection 0 from a test JSON file. Sets CONN_*.
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
    # --defaults-file must be first. It skips ~/.my.cnf, whose [client]
    # password is for localhost and otherwise replaces MYSQL_DB_PASS.
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
    # Stored routines live in this database. Global UDFs (SONAME) are left
    # alone so the other schema on the same server keeps brotli_decompress.
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

ensure_db2() {
    if command -v db2 >/dev/null 2>&1; then
        return 0
    fi
    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        # shellcheck disable=SC1091 # host DB2 profile is outside the repo
        set +u
        # shellcheck disable=SC1091 # host DB2 profile is outside the repo
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

# One statement on the open CLP connection. db2 -x prints data only.
# Call this from the same shell that connected. A command substitution
# is a new process and DB2 answers SQL1024N (rc 4, not connected).
# rc 1 with an empty file means the query returned no rows.
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

# Sets DB2_COUNT. Must run in the shell that holds the CLP connection.
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

# Build DROP statements from SYSCAT and run them. The schema itself stays.
# A failed drop is reported; the recount after both passes decides success.
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

# ~/.sqliterc loads a local extension and prints a banner on stdout.
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

firebird_isql() {
    local sqlfile="$1"
    local dest="$2"
    local lock err rc
    if ! command -v isql-fb >/dev/null 2>&1; then
        warn "Error: isql-fb not found"
        return 1
    fi
    lock="$(make_tmpdir)"
    err="$(make_tmp)"
    set +e
    FIREBIRD_LOCK="${lock}" FIREBIRD_TMP="${lock}" \
        ISC_USER="${CONN_USER}" ISC_PASSWORD="${CONN_PASS}" \
        isql-fb -q -b -pag 0 -ch UTF8 "${CONN_DATABASE}" -i "${sqlfile}" \
        >"${dest}" 2>"${err}"
    rc=$?
    set -e
    if [[ "${rc}" -ne 0 ]]; then
        scrub_file "${err}" "${CONN_PASS}" >&2
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    fi
    if grep -q -E "Statement failed|SQLSTATE|Dynamic SQL Error" "${dest}" "${err}"; then
        scrub_file "${dest}" "${CONN_PASS}" >&2
        scrub_file "${err}" "${CONN_PASS}" >&2
        return 1
    fi
    return 0
}

count_firebird() {
    local sqlfile dest n
    sqlfile="$(make_tmp)"
    dest="$(make_tmp)"
    cat > "${sqlfile}" <<'EOF'
SET HEADING OFF;
SELECT COUNT(*) FROM RDB$RELATIONS WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0;
EOF
    firebird_isql "${sqlfile}" "${dest}" || return 1
    n="$(read_count "${dest}")" || {
        scrub_file "${dest}" "${CONN_PASS}" >&2
        return 1
    }
    printf '%s\n' "${n}"
}

reset_firebird() {
    local sqlfile dest n
    sqlfile="$(make_tmp)"
    dest="$(make_tmp)"
    cat > "${sqlfile}" <<'EOF'
SET TERM ^ ;
EXECUTE BLOCK AS
    DECLARE i INTEGER;
    DECLARE stmt VARCHAR(500);
    DECLARE rname VARCHAR(63);
    DECLARE cname VARCHAR(63);
BEGIN
    FOR SELECT TRIM(rc.RDB$CONSTRAINT_NAME), TRIM(rc.RDB$RELATION_NAME)
        FROM RDB$RELATION_CONSTRAINTS rc
        JOIN RDB$RELATIONS rel ON rel.RDB$RELATION_NAME = rc.RDB$RELATION_NAME
        WHERE rc.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY'
          AND COALESCE(rel.RDB$SYSTEM_FLAG, 0) = 0
        INTO :cname, :rname
    DO
    BEGIN
        stmt = 'ALTER TABLE ' || :rname || ' DROP CONSTRAINT ' || :cname;
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    i = 0;
    WHILE (i < 6) DO
    BEGIN
        i = i + 1;

        FOR SELECT 'DROP TRIGGER ' || TRIM(RDB$TRIGGER_NAME)
            FROM RDB$TRIGGERS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND TRIM(RDB$TRIGGER_NAME) NOT STARTING WITH 'RDB$'
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP PROCEDURE ' || TRIM(RDB$PROCEDURE_NAME)
            FROM RDB$PROCEDURES
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP FUNCTION ' || TRIM(RDB$FUNCTION_NAME)
            FROM RDB$FUNCTIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP VIEW ' || TRIM(RDB$RELATION_NAME)
            FROM RDB$RELATIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND RDB$VIEW_BLR IS NOT NULL
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END

        FOR SELECT 'DROP TABLE ' || TRIM(RDB$RELATION_NAME)
            FROM RDB$RELATIONS
            WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
              AND RDB$VIEW_BLR IS NULL
              AND TRIM(RDB$RELATION_NAME) NOT STARTING WITH 'RDB$'
            INTO :stmt
        DO
        BEGIN
            EXECUTE STATEMENT stmt;
        WHEN ANY DO
        BEGIN
        END
        END
    END

    FOR SELECT 'DROP SEQUENCE ' || TRIM(RDB$GENERATOR_NAME)
        FROM RDB$GENERATORS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
          AND TRIM(RDB$GENERATOR_NAME) NOT STARTING WITH 'RDB$'
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    FOR SELECT 'DROP EXCEPTION ' || TRIM(RDB$EXCEPTION_NAME)
        FROM RDB$EXCEPTIONS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END

    FOR SELECT 'DROP DOMAIN ' || TRIM(RDB$FIELD_NAME)
        FROM RDB$FIELDS
        WHERE COALESCE(RDB$SYSTEM_FLAG, 0) = 0
          AND RDB$FIELD_NAME NOT STARTING WITH 'RDB$'
        INTO :stmt
    DO
    BEGIN
        EXECUTE STATEMENT stmt;
    WHEN ANY DO
    BEGIN
    END
    END
END ^
SET TERM ; ^
EOF
    assert_no_drop_database "$(cat "${sqlfile}")" || return 1
    firebird_isql "${sqlfile}" "${dest}" || return 1
    n="$(count_firebird)" || return 1
    [[ "${n}" -eq 0 ]]
}

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

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --help|-h)
                usage
                exit 0
                ;;
            --dry-run)
                DRY_RUN=1
                shift
                ;;
            --yes)
                ASSUME_YES=1
                shift
                ;;
            --group)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --group needs test or demo"
                    exit 2
                fi
                GROUP_FILTER="${2,,}"
                if [[ "${GROUP_FILTER}" != "test" && "${GROUP_FILTER}" != "demo" ]]; then
                    warn "Error: --group must be test or demo"
                    exit 2
                fi
                shift 2
                ;;
            --engine)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --engine needs a name"
                    exit 2
                fi
                ENGINE_FILTER="$(normalize_engine "${2}")" || exit 2
                shift 2
                ;;
            *)
                warn "Error: unknown argument ${1}"
                usage >&2
                exit 2
                ;;
        esac
    done
}

confirm_reset() {
    local answer
    if [[ "${ASSUME_YES}" -eq 1 ]]; then
        return 0
    fi
    printf '\nType RESET to remove objects from these build schemas: '
    if [[ -r /dev/tty ]]; then
        IFS= read -r answer < /dev/tty || true
    else
        warn "No terminal. Re-run with --yes to skip the prompt."
        return 1
    fi
    if [[ "${answer}" != "RESET" ]]; then
        printf 'Cancelled.\n'
        return 1
    fi
    return 0
}

main() {
    local row group number engine label config path where before after rc
    local selected=0
    parse_args "$@"
    if ! command -v jq >/dev/null 2>&1; then
        warn "Error: jq not found"
        exit 2
    fi
    WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/database_reset.XXXXXX")" || {
        warn "Error: could not create a work directory"
        exit 2
    }
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
        HYDROGEN_ROOT="$(cd "${HYDROGEN_ROOT}" && pwd)"
    else
        HYDROGEN_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
    fi
    printf 'Build schemas for tests 32-39 (test) and test 40 (demo).\n'
    printf 'Databases and database files are kept. Objects inside the schema are removed.\n\n'

    for row in "${TARGETS[@]}"; do
        # shellcheck disable=SC2310 # a skipped filter is not a failure
        if ! selected_row "${row}"; then
            continue
        fi
        IFS='|' read -r group number engine label config <<< "${row}"
        path="${HYDROGEN_ROOT}/tests/configs/${config}"
        # shellcheck disable=SC2310 # one bad config should not hide the rest of the list
        if ! load_connection "${path}"; then
            report "FAILED" "${group}" "${number}" "${label}" "could not read ${config}"
            FAILS=$((FAILS + 1))
            continue
        fi
        # shellcheck disable=SC2310 # validation failure is reported and the loop continues
        if ! validate_target "${group}" "${engine}"; then
            report "FAILED" "${group}" "${number}" "${label}" "refusing this target"
            FAILS=$((FAILS + 1))
            continue
        fi
        where="$(location_text "${engine}")"
        selected=$((selected + 1))
        set +e
        before="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 || ! "${before}" =~ ^[0-9]+$ ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "COUNT" "${group}" "${number}" "${label}" "${where}  tables/views ${before}"
        if [[ "${DRY_RUN}" -eq 1 ]]; then
            continue
        fi
        # Remember reachable targets by re-reading the config at reset time.
        # Stash the row only when the count succeeded.
        REACHABLE+=("${row}")
    done

    if [[ "${selected}" -eq 0 ]]; then
        warn "Error: no targets matched"
        exit 2
    fi
    if [[ "${DRY_RUN}" -eq 1 ]]; then
        printf '\nDry run. No changes made.\n'
        if [[ "${FAILS}" -ne 0 ]]; then
            exit 1
        fi
        exit 0
    fi
    if [[ "${#REACHABLE[@]}" -eq 0 ]]; then
        warn "Nothing reachable to reset."
        exit 1
    fi
    # shellcheck disable=SC2310 # decline is a clean cancel
    if ! confirm_reset; then
        if [[ "${ASSUME_YES}" -eq 0 ]]; then
            exit 0
        fi
        exit 1
    fi

    printf '\n'
    for row in "${REACHABLE[@]}"; do
        IFS='|' read -r group number engine label config <<< "${row}"
        path="${HYDROGEN_ROOT}/tests/configs/${config}"
        # shellcheck disable=SC2310 # skip a target whose config changed under us
        if ! load_connection "${path}"; then
            report "FAILED" "${group}" "${number}" "${label}" "could not re-read ${config}"
            FAILS=$((FAILS + 1))
            continue
        fi
        # shellcheck disable=SC2310 # do not empty a target that no longer matches the allowlist
        if ! validate_target "${group}" "${engine}"; then
            report "FAILED" "${group}" "${number}" "${label}" "refusing this target"
            FAILS=$((FAILS + 1))
            continue
        fi
        if [[ "${engine}" == "db2" ]]; then
            # shellcheck disable=SC2310 # missing client fails this engine only
            if ! ensure_db2; then
                report "FAILED" "${group}" "${number}" "${label}" "db2 client not found"
                FAILS=$((FAILS + 1))
                continue
            fi
        fi
        where="$(location_text "${engine}")"
        set +e
        before="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "RESET" "${group}" "${number}" "${label}" "${where}  was ${before}"
        set +e
        reset_target "${engine}"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}"
            FAILS=$((FAILS + 1))
            continue
        fi
        set +e
        after="$(count_target "${engine}")"
        rc=$?
        set -e
        if [[ "${rc}" -ne 0 || "${after}" != "0" ]]; then
            report "FAILED" "${group}" "${number}" "${label}" "${where}  still ${after:-unknown}"
            FAILS=$((FAILS + 1))
            continue
        fi
        report "OK" "${group}" "${number}" "${label}" "${where}  ${before} -> 0"
    done

    printf '\n'
    if [[ "${FAILS}" -ne 0 ]]; then
        printf '%s target(s) failed.\n' "${FAILS}"
        exit 1
    fi
    printf 'Reset complete. Databases were left in place.\n'
    exit 0
}

REACHABLE=()
main "$@"
