#!/usr/bin/env bash

# Database transaction verification helpers for blackbox tests.
# Confirms multi-statement DML commit/rollback behavior that migration LOAD/APPLY rely on.

# CHANGELOG
# 1.0.1 - 2026-07-23 - Shellcheck cleanups (SC2154/SC2155/SC2116/SC2312)
# 1.0.0 - 2026-07-23 - Initial commit/rollback probe across 7 engines

# Guard against multiple sourcing
[[ -n "${TRANSACTION_UTILS_GUARD:-}" ]] && return 0
readonly TRANSACTION_UTILS_GUARD=1

# Verify DML transactions for one engine.
# Args: engine_key (postgresql|mysql|sqlite|db2|mariadb|cockroachdb|yugabytedb)
#        [schema] optional schema/qualifier (default engine-specific demo schema)
#        [sqlite_path] required when engine_key=sqlite
# Returns 0 on success, 1 on failure. Prints brief diagnostics to stdout.
verify_database_transactions() {
    local engine_key="${1:-}"
    local schema="${2:-}"
    local sqlite_path="${3:-}"
    local table_base="hydro_tx_probe"
    local marker
    local table_u
    local qualified=""
    local out=""
    local rc=0

    marker="hydro-tx-$(date +%s)-$$"

    if [[ -z "${engine_key}" ]]; then
        echo "transaction_utils: engine_key required"
        return 1
    fi

    case "${engine_key}" in
        postgresql)
            schema="${schema:-demo}"
            qualified="${schema}.${table_base}"
            out=$(verify_tx_postgresql "${qualified}" "${marker}") || rc=$?
            ;;
        yugabytedb)
            schema="${schema:-demo}"
            qualified="${schema}.${table_base}"
            out=$(verify_tx_yugabytedb "${qualified}" "${marker}") || rc=$?
            ;;
        cockroachdb)
            schema="${schema:-democrdb}"
            qualified="${schema}.${table_base}"
            out=$(verify_tx_cockroachdb "${qualified}" "${marker}") || rc=$?
            ;;
        mysql)
            schema="${schema:-demo}"
            qualified="${schema}.${table_base}"
            out=$(verify_tx_mysql_family "${qualified}" "${marker}" \
                "${CANVAS_DB_HOST:-}" "${CANVAS_DB_PORT:-}" \
                "${CANVAS_DB_USER:-}" "${CANVAS_DB_PASS:-}" \
                "${CANVAS_DB_NAME:-}") || rc=$?
            ;;
        mariadb)
            schema="${schema:-demomrdb}"
            qualified="${schema}.${table_base}"
            out=$(verify_tx_mysql_family "${qualified}" "${marker}" \
                "${CANVAS_DB_HOST:-}" "${CANVAS_DB_PORT:-}" \
                "${CANVAS_DB_USER:-}" "${CANVAS_DB_PASS:-}" \
                "${CANVAS_DB_NAME:-}") || rc=$?
            ;;
        db2)
            schema="${schema:-DEMO}"
            schema=$(printf '%s' "${schema}" | tr '[:lower:]' '[:upper:]')
            table_u=$(printf '%s' "${table_base}" | tr '[:lower:]' '[:upper:]')
            qualified="${schema}.${table_u}"
            out=$(verify_tx_db2 "${qualified}" "${marker}") || rc=$?
            ;;
        sqlite)
            if [[ -z "${sqlite_path}" ]]; then
                echo "transaction_utils: sqlite_path required for sqlite"
                return 1
            fi
            qualified="${table_base}"
            out=$(verify_tx_sqlite "${sqlite_path}" "${qualified}" "${marker}") || rc=$?
            ;;
        *)
            echo "transaction_utils: unsupported engine_key=${engine_key}"
            return 1
            ;;
    esac

    if [[ "${rc}" -ne 0 ]]; then
        echo "transaction_utils: FAIL ${engine_key} ${out}"
        return 1
    fi
    echo "transaction_utils: PASS ${engine_key} rollback+commit DML OK (${out})"
    return 0
}

# --- engine runners ---

# True when counts string is exactly "0\n2"
_tx_counts_ok() {
    local counts="$1"
    [[ "${counts}" == $'0\n2' ]]
}

# Run PG-family multi-statement probe. COMMIT after DDL so CREATE survives ROLLBACK
# of the DML block (psql -c runs the whole string in one implicit transaction otherwise).
verify_tx_pg_family() {
    local host="$1"
    local port="$2"
    local user="$3"
    local pass="$4"
    local db="$5"
    local qualified="$6"
    local marker="$7"
    local val_type="${8:-TEXT}"
    local sql
    local result
    local counts
    export PGPASSWORD="${pass}"
    sql=$(cat <<EOF
DROP TABLE IF EXISTS ${qualified};
CREATE TABLE ${qualified} (id INTEGER PRIMARY KEY, val ${val_type});
COMMIT;
BEGIN;
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
ROLLBACK;
SELECT COUNT(*) AS c FROM ${qualified};
BEGIN;
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
COMMIT;
SELECT COUNT(*) AS c FROM ${qualified};
DROP TABLE ${qualified};
COMMIT;
EOF
)
    result=$(psql -h "${host}" -p "${port}" -U "${user}" -d "${db}" \
        -v ON_ERROR_STOP=1 -t -A -c "${sql}" 2>&1) || {
        echo "psql_error:${result}"
        return 1
    }
    counts=$(printf '%s\n' "${result}" | tr -d ' ' | grep -E '^[0-9]+$' || true)
    if _tx_counts_ok "${counts}"; then
        echo "counts=0,2"
        return 0
    fi
    echo "unexpected_counts:${counts//$'\n'/,}"
    return 1
}

verify_tx_postgresql() {
    verify_tx_pg_family \
        "${ACURANZO_DB_HOST:-}" "${ACURANZO_DB_PORT:-}" \
        "${ACURANZO_DB_USER:-}" "${ACURANZO_DB_PASS:-}" \
        "${ACURANZO_DB_NAME:-}" "$1" "$2" "TEXT"
}

verify_tx_yugabytedb() {
    verify_tx_pg_family \
        "${YUGABYTE_DB_HOST:-}" "${YUGABYTE_DB_PORT:-}" \
        "${YUGABYTE_DB_USER:-}" "${YUGABYTE_DB_PASS:-}" \
        "${YUGABYTE_DB_NAME:-}" "$1" "$2" "TEXT"
}

verify_tx_cockroachdb() {
    # Cockroach configs reuse ACURANZO_* credentials (PG wire).
    # Use TEXT (not STRING): TEXT works on both Cockroach and PostgreSQL; STRING does not.
    verify_tx_pg_family \
        "${ACURANZO_DB_HOST:-}" "${ACURANZO_DB_PORT:-}" \
        "${ACURANZO_DB_USER:-}" "${ACURANZO_DB_PASS:-}" \
        "${ACURANZO_DB_NAME:-}" "$1" "$2" "TEXT"
}

verify_tx_mysql_family() {
    local qualified="$1"
    local marker="$2"
    local host="$3"
    local port="$4"
    local user="$5"
    local pass="$6"
    local db="$7"
    local sql
    local result
    local counts
    # DDL often implicit-commits on MySQL/MariaDB; DML is what migration LOAD needs.
    sql=$(cat <<EOF
DROP TABLE IF EXISTS ${qualified};
CREATE TABLE ${qualified} (id INT PRIMARY KEY, val VARCHAR(128));
START TRANSACTION;
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
ROLLBACK;
SELECT COUNT(*) AS c FROM ${qualified};
START TRANSACTION;
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
COMMIT;
SELECT COUNT(*) AS c FROM ${qualified};
DROP TABLE ${qualified};
EOF
)
    result=$(mysql -h "${host}" -P "${port}" -u "${user}" -p"${pass}" "${db}" \
        -N -B -e "${sql}" 2>&1) || {
        echo "mysql_error:${result}"
        return 1
    }
    counts=$(printf '%s\n' "${result}" | tr -d ' ' | grep -E '^[0-9]+$' || true)
    if _tx_counts_ok "${counts}"; then
        echo "counts=0,2"
        return 0
    fi
    echo "unexpected_counts:${counts//$'\n'/,}"
    return 1
}

verify_tx_sqlite() {
    local db_path="$1"
    local table="$2"
    local marker="$3"
    local sql
    local result
    local counts
    sql=$(cat <<EOF
DROP TABLE IF EXISTS ${table};
CREATE TABLE ${table} (id INTEGER PRIMARY KEY, val TEXT);
BEGIN;
INSERT INTO ${table} (id, val) VALUES (1, '${marker}');
INSERT INTO ${table} (id, val) VALUES (2, '${marker}');
ROLLBACK;
SELECT COUNT(*) FROM ${table};
BEGIN;
INSERT INTO ${table} (id, val) VALUES (1, '${marker}');
INSERT INTO ${table} (id, val) VALUES (2, '${marker}');
COMMIT;
SELECT COUNT(*) FROM ${table};
DROP TABLE ${table};
EOF
)
    result=$(sqlite3 "${db_path}" "${sql}" 2>&1) || {
        echo "sqlite_error:${result}"
        return 1
    }
    counts=$(printf '%s\n' "${result}" | tr -d ' ' | grep -E '^[0-9]+$' || true)
    if _tx_counts_ok "${counts}"; then
        echo "counts=0,2"
        return 0
    fi
    echo "unexpected_counts:${counts//$'\n'/,}"
    return 1
}

verify_tx_db2() {
    local qualified="$1"
    local marker="$2"
    local name="${HYDROTST_DB_NAME:-}"
    local user="${HYDROTST_DB_USER:-}"
    local pass="${HYDROTST_DB_PASS:-}"
    local script
    local result
    local counts
    local has_zero=0
    local has_two=0
    local err_snip=""
    local tail_snip=""
    script=$(mktemp /tmp/kilo/db2_tx_XXXXXX.sql)

    # Autocommit OFF is required — same requirement as Hydrogen's migration path.
    # UPDATE COMMAND OPTIONS USING c OFF disables CLP autocommit for the session.
    cat > "${script}" <<EOF
CONNECT TO ${name} USER ${user} USING '${pass//\'/\'\'}';
UPDATE COMMAND OPTIONS USING c OFF;
DROP TABLE ${qualified};
CREATE TABLE ${qualified} (id INTEGER NOT NULL PRIMARY KEY, val VARCHAR(128));
COMMIT;
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
ROLLBACK;
SELECT COUNT(*) AS c FROM ${qualified};
INSERT INTO ${qualified} (id, val) VALUES (1, '${marker}');
INSERT INTO ${qualified} (id, val) VALUES (2, '${marker}');
COMMIT;
SELECT COUNT(*) AS c FROM ${qualified};
DROP TABLE ${qualified};
COMMIT;
CONNECT RESET;
EOF

    if [[ -f /home/db2inst1/sqllib/db2profile ]]; then
        # shellcheck disable=SC1091 # DB2 profile path is host-specific, not resolvable statically
        . /home/db2inst1/sqllib/db2profile
    fi
    result=$(db2 -tvf "${script}" 2>&1) || true
    rm -f "${script}"

    # Extract numeric count results from SELECT output lines
    counts=$(printf '%s\n' "${result}" | awk '
        /^[[:space:]]*[0-9]+[[:space:]]*$/ { gsub(/ /,"",$0); print $0 }
    ')
    counts=$(printf '%s\n' "${counts}" | head -5)

    if printf '%s\n' "${result}" | grep -q "SQLSTATE="; then
        # Allow drop-missing noise; fail on hard errors during inserts
        if printf '%s\n' "${result}" | grep -qE "SQL0[89][0-9]{2}N|SQLSTATE=23505|SQLSTATE=08003"; then
            err_snip=$(printf '%s\n' "${result}" | tr '\n' ' ')
            err_snip=$(printf '%s' "${err_snip}" | cut -c1-200)
            echo "db2_error:${err_snip}"
            return 1
        fi
    fi

    # Expect 0 then 2 among selected counts
    if printf '%s\n' "${counts}" | grep -q '^0$'; then
        has_zero=1
    fi
    if printf '%s\n' "${counts}" | grep -q '^2$'; then
        has_two=1
    fi
    if [[ "${has_zero}" -eq 1 && "${has_two}" -eq 1 ]]; then
        echo "counts=0,2"
        return 0
    fi
    tail_snip=$(printf '%s\n' "${result}" | tail -5)
    tail_snip=$(printf '%s' "${tail_snip}" | tr '\n' ';')
    echo "unexpected_counts:${counts//$'\n'/,};tail=${tail_snip}"
    return 1
}
