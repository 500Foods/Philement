#!/usr/bin/env bash

# Hydrogen Flush Utility
# Flushes all test and demo database schemas by dropping all tables
# Used to reset databases before major migration changes

# CHANGELOG
# 1.0.3 - 2026-01-17 - Fix DB2 password handling with special characters by disabling history expansion

set -euo pipefail
set +H  # Disable history expansion to handle passwords with special characters

# Parse command line arguments
DEBUG=false
if [[ "${1:-}" == "--debug" ]]; then
    DEBUG=true
fi

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
    PROJECT_DIR="${HYDROGEN_ROOT}"
else
    PROJECT_DIR="$(dirname "$(dirname "${SCRIPT_DIR}")")"
fi
TESTS_DIR="${PROJECT_DIR}/tests"
CONFIGS_DIR="${TESTS_DIR}/configs"

# Function to get database connection details from config
get_db_config() {
    local config_file="$1"
    local engine
    local host
    local port
    local database
    local user
    local pass
    local schema

    engine=$(jq -r '.Databases.Connections[0].Engine // empty' "${config_file}" 2>/dev/null || echo "")
    host=$(jq -r '.Databases.Connections[0].Host // empty' "${config_file}" 2>/dev/null || echo "")
    port=$(jq -r '.Databases.Connections[0].Port // empty' "${config_file}" 2>/dev/null || echo "")
    database=$(jq -r '.Databases.Connections[0].Database // empty' "${config_file}" 2>/dev/null || echo "")
    user=$(jq -r '.Databases.Connections[0].User // empty' "${config_file}" 2>/dev/null || echo "")
    pass=$(jq -r '.Databases.Connections[0].Pass // empty' "${config_file}" 2>/dev/null || echo "")
    schema=$(jq -r '.Databases.Connections[0].Schema // empty' "${config_file}" 2>/dev/null || echo "")

    # Expand environment variables (replace ${env.VAR} with ${VAR})
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    engine=$(echo "${engine}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    engine=$(eval echo "${engine}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    host=$(echo "${host}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    host=$(eval echo "${host}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    port=$(echo "${port}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    port=$(eval echo "${port}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    database=$(echo "${database}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    database=$(eval echo "${database}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    user=$(echo "${user}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    user=$(eval echo "${user}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    pass=$(echo "${pass}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    pass=$(eval echo "${pass}")
    # shellcheck disable=SC2001 # Sed is needed for regex pattern matching
    schema=$(echo "${schema}" | sed 's/\${env\.\([^}]*\)}/${\1}/g')
    schema=$(eval echo "${schema}")

    echo "${engine}|${host}|${port}|${database}|${user}|${pass}|${schema}"
}

# Function to count objects in schema/database
count_objects() {
    local engine="$1"
    local host="$2"
    local port="$3"
    local database="$4"
    local user="$5"
    local pass="$6"
    local schema="$7"

    local count
    case "${engine}" in
        postgresql|postgres)
            if [[ -n "${schema}" ]]; then
                count=$(PGPASSWORD="${pass}" psql -h "${host}" -p "${port}" -U "${user}" -d "${database}" -t -c "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${schema}';" 2>/dev/null | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
            else
                count="0"
            fi
            ;;
        mysql|mariadb)
            if [[ -n "${schema}" ]]; then
                count=$(mysql -h "${host}" -P "${port}" -u "${user}" -p"${pass}" "${database}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${schema}';" 2>/dev/null | tail -1 | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
            else
                count="0"
            fi
            ;;
        db2)
            if [[ -n "${schema}" ]]; then
                db2 connect to "${database}" user "${user}" using "${pass}" > /dev/null 2>&1 || true
                count=$(db2 "SELECT COUNT(*) FROM syscat.tables WHERE tabschema = '${schema}'" 2>/dev/null | tail -1 | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
                db2 connect reset > /dev/null 2>&1 || true
            else
                count="0"
            fi
            ;;
        sqlite)
            if [[ -f "${database}" ]]; then
                count=$(sqlite3 "${database}" "SELECT COUNT(*) FROM sqlite_master WHERE type='table';" 2>/dev/null | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
            else
                count="0"
            fi
            ;;
        cockroachdb)
            if [[ -n "${schema}" ]]; then
                count=$(cockroach sql --host="${host}:${port}" --user="${user}" --database="${database}" --execute="SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${schema}';" 2>/dev/null | tail -1 | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
            else
                count="0"
            fi
            ;;
        yugabytedb)
            if [[ -n "${schema}" ]]; then
                count=$(ysqlsh -h "${host}" -p "${port}" -U "${user}" -d "${database}" -c "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${schema}';" 2>/dev/null | tail -1 | tr -d ' ' | grep -E '^[0-9]+$' || echo "0")
            else
                count="0"
            fi
            ;;
        *)
            count="0"
            ;;
    esac
    echo "${count}"
}

# Function to drop schema for different database engines
drop_schema() {
    local engine="$1"
    local host="$2"
    local port="$3"
    local database="$4"
    local user="$5"
    local pass="$6"
    local schema="$7"

    case "${engine}" in
        postgresql|postgres)
            if [[ -n "${schema}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    PGPASSWORD="${pass}" psql -h "${host}" -p "${port}" -U "${user}" -d "${database}" -c "DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" || true
                else
                    PGPASSWORD="${pass}" psql -h "${host}" -p "${port}" -U "${user}" -d "${database}" -c "DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" > /dev/null 2>&1 || true
                fi
            fi
            ;;
        mysql|mariadb)
            if [[ -n "${schema}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    mysql -h "${host}" -P "${port}" -u "${user}" -p"${pass}" "${database}" -e "DROP SCHEMA IF EXISTS \`${schema}\`; CREATE SCHEMA \`${schema}\`;" || true
                else
                    mysql -h "${host}" -P "${port}" -u "${user}" -p"${pass}" "${database}" -e "DROP SCHEMA IF EXISTS \`${schema}\`; CREATE SCHEMA \`${schema}\`;" > /dev/null 2>&1 || true
                fi
            fi
            ;;
        db2)
            if [[ -n "${schema}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    db2 connect to "${database}" user "${user}" using "${pass}" || true
                    db2 "CALL SYSPROC.ADMIN_DROP_SCHEMA('${schema}', NULL, '${user}', 'CASCADE')" || true
                    db2 connect reset || true
                else
                    db2 connect to "${database}" user "${user}" using "${pass}" > /dev/null 2>&1 || true
                    db2 "CALL SYSPROC.ADMIN_DROP_SCHEMA('${schema}', NULL, '${user}', 'CASCADE')" > /dev/null 2>&1 || true
                    db2 connect reset > /dev/null 2>&1 || true
                fi
            fi
            ;;
        sqlite)
            if [[ -f "${database}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    for table in $(sqlite3 "${database}" "SELECT name FROM sqlite_master WHERE type='table';" || true); do
                        sqlite3 "${database}" "DROP TABLE IF EXISTS ${table};" || true
                    done
                else
                    for table in $(sqlite3 "${database}" "SELECT name FROM sqlite_master WHERE type='table';" 2>/dev/null || true); do
                        sqlite3 "${database}" "DROP TABLE IF EXISTS ${table};" > /dev/null 2>&1 || true
                    done
                fi
            fi
            ;;
        cockroachdb)
            if [[ -n "${schema}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    cockroach sql --host="${host}:${port}" --user="${user}" --database="${database}" --execute="DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" || true
                else
                    cockroach sql --host="${host}:${port}" --user="${user}" --database="${database}" --execute="DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" > /dev/null 2>&1 || true
                fi
            fi
            ;;
        yugabytedb)
            if [[ -n "${schema}" ]]; then
                if [[ "${DEBUG}" == true ]]; then
                    ysqlsh -h "${host}" -p "${port}" -U "${user}" -d "${database}" -c "DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" || true
                else
                    ysqlsh -h "${host}" -p "${port}" -U "${user}" -d "${database}" -c "DROP SCHEMA IF EXISTS \"${schema}\" CASCADE; CREATE SCHEMA \"${schema}\";" > /dev/null 2>&1 || true
                fi
            fi
            ;;
        *)
            echo "Unsupported database engine: ${engine}"
            return 1
            ;;
    esac
}

# Collect test databases (tests 32-38)
echo "TESTS:"
test_configs=(
    "32:PostgreSQL:hydrogen_test_32_postgres.json"
    "33:MySQL:hydrogen_test_33_mysql.json"
    "34:SQLite:hydrogen_test_34_sqlite.json"
    "35:DB2:hydrogen_test_35_db2.json"
    "36:MariaDB:hydrogen_test_36_mariadb.json"
    "37:CockroachDB:hydrogen_test_37_cockroachdb.json"
    "38:YugabyteDB:hydrogen_test_38_yugabytedb.json"
)

for config in "${test_configs[@]}"; do
    IFS=':' read -r test_num engine config_file <<< "${config}"
    config_path="${CONFIGS_DIR}/${config_file}"
    if [[ -f "${config_path}" ]]; then
        # shellcheck disable=SC2310 # Function invoked in || condition intentionally
        IFS='|' read -r db_engine host port database user pass schema <<< "$(get_db_config "${config_path}" || true)"
        name=$(jq -r '.Databases.Connections[0].Name // empty' "${config_path}" 2>/dev/null || true)
        name="${name//\\\$env./\$}"
        name=$(eval echo "${name}")
        engine_padded=$(printf '%-12s' "${engine}")
        if [[ -n "${schema}" ]]; then
            echo "- Test ${test_num} - ${engine_padded} - ${name}/${schema}"
        else
            if [[ "${engine,,}" == "sqlite" ]]; then
                echo "- Test ${test_num} - ${engine_padded} - ${database}"
            else
                echo "- Test ${test_num} - ${engine_padded} - ${name}/"
            fi
        fi
    fi
done

# Collect demo databases (test 40)
echo ""
echo "DEMOS:"
demo_configs=(
    "40:PostgreSQL:hydrogen_test_40_postgres.json"
    "40:MySQL:hydrogen_test_40_mysql.json"
    "40:SQLite:hydrogen_test_40_sqlite.json"
    "40:DB2:hydrogen_test_40_db2.json"
    "40:MariaDB:hydrogen_test_40_mariadb.json"
    "40:CockroachDB:hydrogen_test_40_cockroachdb.json"
    "40:YugabyteDB:hydrogen_test_40_yugabytedb.json"
)

for config in "${demo_configs[@]}"; do
    IFS=':' read -r test_num engine config_file <<< "${config}"
    config_path="${CONFIGS_DIR}/${config_file}"
    if [[ -f "${config_path}" ]]; then
        # shellcheck disable=SC2310 # Function invoked in || condition intentionally
        IFS='|' read -r db_engine host port database user pass schema <<< "$(get_db_config "${config_path}" || true)"
        name=$(jq -r '.Databases.Connections[0].Name // empty' "${config_path}" 2>/dev/null || true)
        name="${name//\\\$env./\$}"
        name=$(eval echo "${name}")
        engine_padded=$(printf '%-12s' "${engine}")
        if [[ -n "${schema}" ]]; then
            echo "- Test ${test_num} - ${engine_padded} - ${name}/${schema}"
        else
            if [[ "${engine,,}" == "sqlite" ]]; then
                echo "- Test ${test_num} - ${engine_padded} - ${database}"
            else
                echo "- Test ${test_num} - ${engine_padded} - ${name}/"
            fi
        fi
    fi
done

# Confirmation
echo ""
read -r -p "Enter HYDROGEN in all caps to continue: " confirmation
if [[ "${confirmation}" != "HYDROGEN" ]]; then
    echo "Operation cancelled."
    exit 0
fi

# Flush test databases
echo ""
echo "Flushing TEST databases..."
for config in "${test_configs[@]}"; do
    IFS=':' read -r test_num engine config_file <<< "${config}"
    config_path="${CONFIGS_DIR}/${config_file}"
    if [[ -f "${config_path}" ]]; then
        # shellcheck disable=SC2310 # Function invoked in || condition intentionally
        IFS='|' read -r db_engine host port database user pass schema <<< "$(get_db_config "${config_path}" || true)"
        echo "— Flushing Test ${test_num} - ${engine}..."
        before_count=$(count_objects "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}")
        echo "——— Start: ${before_count} objects found"
        drop_schema "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}"
        after_count=$(count_objects "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}")
        echo "——— After: ${after_count} objects found"
        if [[ ${before_count} -gt 0 && ${after_count} -eq 0 ]]; then
            echo "——— Successfully flushed Test ${test_num} - ${engine}"
        elif [[ ${before_count} -eq 0 ]]; then
            echo "——— No objects to flush for Test ${test_num} - ${engine}"
        else
            echo "——— Failed to flush Test ${test_num} - ${engine}"
        fi
    fi
done

# Flush demo databases
echo ""
echo "Flushing DEMO databases..."
for config in "${demo_configs[@]}"; do
    IFS=':' read -r test_num engine config_file <<< "${config}"
    config_path="${CONFIGS_DIR}/${config_file}"
    if [[ -f "${config_path}" ]]; then
        # shellcheck disable=SC2310 # Function invoked in || condition intentionally
        IFS='|' read -r db_engine host port database user pass schema <<< "$(get_db_config "${config_path}" || true)"
        echo "— Flushing Test ${test_num} - ${engine}..."
        before_count=$(count_objects "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}")
        echo "——— Start: ${before_count} objects found"
        drop_schema "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}"
        after_count=$(count_objects "${db_engine}" "${host}" "${port}" "${database}" "${user}" "${pass}" "${schema}")
        echo "——— After: ${after_count} objects found"
        if [[ ${before_count} -gt 0 && ${after_count} -eq 0 ]]; then
            echo "——— Successfully flushed Test ${test_num} - ${engine}"
        elif [[ ${before_count} -eq 0 ]]; then
            echo "——— No objects to flush for Test ${test_num} - ${engine}"
        else
            echo "——— Failed to flush Test ${test_num} - ${engine}"
        fi
    fi
done

echo ""
echo "Flush operation completed."