#!/usr/bin/env bash

# Test: Database Migration Validation
# Performs SQL validation on database migration files using sqruff

# FUNCTIONS
# get_file_hash()
# validate_migration()

# CHANGELOG
# 1.2.0 - 2025-10-29 - Added check for unsubstituted variables in generated SQL
# 1.1.0 - 2025-09-30 - Updated location of Helium migration files
# 1.0.0 - 2025-09-14 - Initial creation for database migration validation

set -euo pipefail

# Test configuration
TEST_NAME="Migrations"
TEST_ABBR="MGR"
TEST_NUMBER="31"
TEST_COUNTER=0
TEST_VERSION="1.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test Setup
CACHE_DIR="${HOME}/.cache/sqruff"
mkdir -p "${CACHE_DIR}"

# Helium project path (relative to workspace root) - points to parent to allow appending design names
HELIUM_DIR="../../../elements/002-helium"

# List of designs to validate
DESIGNS=("helium" "acuranzo")

# Supported database engines
ENGINES=("postgresql" "sqlite" "mysql" "db2")

# Schema mapping per design per engine (corresponding to ENGINES array order)
# ENGINES=("postgresql" "sqlite" "mysql" "db2")
declare -A DESIGN_SCHEMAS
DESIGN_SCHEMAS["helium"]="helium::helium:HELIUM"
DESIGN_SCHEMAS["acuranzo"]="app::acuranzo:ACURANZO"

# Function to get file hash (using md5sum or equivalent)
get_file_hash() {
    # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
    "${MD5SUM}" "$1" | "${AWK}" '{print $1}' || true
}

# Function to validate a single migration
validate_migration() {
    local design="$1"
    local engine="$2"
    local migration="$3"
    local migration_file="$4"

    # Get schema_name from design mapping based on engine index
    local design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    local schema_name=""
    if [[ -n "${design_schemas}" ]]; then
        # Split by colon and get the schema for this engine
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        # Find engine index
        for i in "${!ENGINES[@]}"; do
            if [[ "${ENGINES[${i}]}" == "${engine}" ]]; then
                schema_name="${SCHEMA_ARRAY[${i}]:-}"
                break
            fi
        done
    fi

    # Generate cache key based on design, engine, schema, migration, and file hash
    local file_hash
    file_hash=$(get_file_hash "${migration_file}")
    local cache_key="${design}_${engine}_${schema_name}_${migration}_${file_hash}"
    local cache_file="${CACHE_DIR}/${cache_key}"

    # Check if we have cached result
    if [[ -f "${cache_file}" ]]; then
        # Read cached result but don't output it
        local cached_result
        cached_result=$(cat "${cache_file}")
        if [[ "${cached_result}" == *"PASSED"* ]]; then
            # Return cache status via stdout (0 = cached, 1 = not cached)
            echo "cached|"
            return 0
        else
            echo "cached|"
            return 1
        fi
    fi

    # Generate SQL using our Lua script (silently)
    local sql_output
    # Set LUA_PATH to include both the lib directory and design directory
    local design_dir="${HELIUM_DIR}/${design}/migrations"
    # local lua_path="${SCRIPT_DIR}/lib/?.lua;${design_dir}/?.lua"

    if ! sql_output=$("${SCRIPT_DIR}/lib/get_migration.sh" "${engine}" "${design}" "${schema_name}" "${migration}" 2>&1); then
        # SQL generation failed, save error to diags_file
        local diags_file="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${design}_${engine}_${schema_name}_${migration_num}.txt"
        mkdir -p "$(dirname "${diags_file}")"
        echo "SQL generation failed:" > "${diags_file}"
        echo "${sql_output}" >> "${diags_file}"
        echo "❌ ${design} ${engine} ${migration} FAILED (SQL generation)" > "${cache_file}"
        echo "fresh|${diags_file}"
        return 1
    fi

    # Check for unsubstituted variables in the generated SQL
    if echo "${sql_output}" | grep -q "\${[^}]*}" || echo "${sql_output}" | grep -q "{[^}]*}"; then
        # Found unsubstituted variables or braces, save error to diags_file
        local diags_file="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${design}_${engine}_${schema_name}_${migration_num}.txt"
        mkdir -p "$(dirname "${diags_file}")"
        echo "Unsubstituted variables found in generated SQL:" > "${diags_file}"
        echo "${sql_output}" >> "${diags_file}"
        echo "❌ ${design} ${engine} ${migration} FAILED (unsubstituted variables)" > "${cache_file}"
        echo "fresh|${diags_file}"
        return 1
    fi

    # Check if sqruff is available
    if ! command -v sqruff >/dev/null 2>&1; then
        echo "✅ ${design} ${engine} ${migration} SKIPPED (sqruff not found)" > "${cache_file}"
        return 0
    fi

    # Use sqruff config file for the specific engine (copied to hydrogen root)
    local config_file=".sqruff_${engine}"
    if [[ ! -f "${config_file}" ]]; then
        echo "✅ ${design} ${engine} ${migration} SKIPPED (config missing)" > "${cache_file}"
        return 0
    fi

    # Create temporary SQL file
    local temp_sql
    temp_sql=$(mktemp)
    echo "${sql_output}" > "${temp_sql}"

    # Create diagnostics file for sqruff output
    local diags_file="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${design}_${engine}_${schema_name}_${migration_num}.txt"
    mkdir -p "$(dirname "${diags_file}")"

    # Run sqruff validation and capture output
    local lint_exit_code=0
    if ! sqruff lint --format github-annotation-native --config "${config_file}" "${temp_sql}" > "${diags_file}" 2>&1; then
        lint_exit_code=1
    fi

    # Clean up temp file
    rm -f "${temp_sql}"

    if [[ ${lint_exit_code} -eq 0 ]]; then
        echo "✅ ${design} ${engine} ${migration} PASSED" > "${cache_file}"
        echo "fresh|${diags_file}"
        return 0
    else
        echo "❌ ${design} ${engine} ${migration} FAILED" > "${cache_file}"
        echo "fresh|${diags_file}"
        return 1
    fi
}

# Initialize counters
TOTAL_MIGRATIONS=0
PASSED_VALIDATIONS=0
FAILED_VALIDATIONS=0
CACHED_VALIDATIONS=0

# Arrays to collect results for final reporting
VALIDATION_RESULTS=()

# Calculate total combos
total_combos=0
for design in "${DESIGNS[@]}"; do
    design_dir="${HELIUM_DIR}/${design}/migrations"
    migration_files=()
    if [[ -d "${design_dir}" ]]; then
        while IFS= read -r -d '' file; do
            migration_files+=("${file}")
        done < <("${FIND}" "${design_dir}" -name "${design}_????.lua" -type f -print0 2>/dev/null | sort -z || true)
    fi
    if [[ ${#migration_files[@]} -eq 0 ]]; then
        continue
    fi
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        for migration_file in "${migration_files[@]}"; do
            migration_name=$(basename "${migration_file}" .lua)
            migration_num="${migration_name#"${design}"_}"
            for i in "${!ENGINES[@]}"; do
                schema="${SCHEMA_ARRAY[${i}]:-}"
                total_combos=$((total_combos + 1))
            done
        done
    fi
done

# Process each design
for design in "${DESIGNS[@]}"; do
    # Find migration files for this design (sorted)
    design_dir="${HELIUM_DIR}/${design}/migrations"
    migration_files=()
    if [[ -d "${design_dir}" ]]; then
        while IFS= read -r -d '' file; do
            migration_files+=("${file}")
        done < <("${FIND}" "${design_dir}" -name "${design}_????.lua" -type f -print0 2>/dev/null | sort -z || true)
    fi

    if [[ ${#migration_files[@]} -eq 0 ]]; then
        continue
    fi

    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"

        # Process each migration file (already sorted)
        for migration_file in "${migration_files[@]}"; do
            migration_name=$(basename "${migration_file}" .lua)
            migration_num="${migration_name#"${design}"_}"

            # Validate against each database engine
            for i in "${!ENGINES[@]}"; do
                engine="${ENGINES[${i}]}"
                schema="${SCHEMA_ARRAY[${i}]:-}"

                # Call function and capture both status and result
                cache_info=$(validate_migration "${design}" "${engine}" "${migration_num}" "${migration_file}"; echo "exit:$?")
                validation_result=${cache_info##*exit:}
                cache_info=${cache_info%exit:*}
                IFS='|' read -r cache_status diags_file <<< "${cache_info}"

                if [[ -z "${schema}" ]]; then
                    subtest_name="${engine} ${design} (no schema) ${migration_num}"
                else
                    subtest_name="${engine} ${design} ${schema} ${migration_num}"
                fi
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${subtest_name}"
                if [[ ${validation_result} -eq 0 ]]; then
                    PASSED_VALIDATIONS=$(( PASSED_VALIDATIONS + 1 ))
                    cache_indicator=""
                    if [[ "${cache_status}" == "cached" ]]; then
                        cache_indicator=" (cached)"
                        CACHED_VALIDATIONS=$(( CACHED_VALIDATIONS + 1 ))
                    fi
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Migration validation passed${cache_indicator}"
                    VALIDATION_RESULTS+=("✅ ${subtest_name} PASSED${cache_indicator}")
                else
                    FAILED_VALIDATIONS=$(( FAILED_VALIDATIONS + 1 ))
                    cache_indicator=""
                    if [[ "${cache_status}" == "cached" ]]; then
                        cache_indicator=" (cached)"
                        CACHED_VALIDATIONS=$(( CACHED_VALIDATIONS + 1 ))
                    fi
                    if [[ "${cache_status}" == "fresh" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Diagnostics: ..${diags_file}"
                    fi
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Migration validation failed${cache_indicator}"
                    VALIDATION_RESULTS+=("❌ ${subtest_name} FAILED${cache_indicator}")
                    EXIT_CODE=1
                fi
                TOTAL_MIGRATIONS=$(( TOTAL_MIGRATIONS + 1 ))
            done
        done
    fi
done

# Update test name with final counts
TOTAL_DESIGNS=${#DESIGNS[@]}
if [[ ${total_combos} -gt 0 ]]; then
    # Calculate total number of migration files across all designs
    total_migration_files=$(( total_combos / ${#ENGINES[@]} ))
    TEST_NAME="${TEST_NAME} {BLUE}(${TOTAL_DESIGNS} designs, ${total_migration_files} migrations, ${TOTAL_MIGRATIONS} combos){RESET}"
fi


# Summary
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validation Summary:"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Combos processed: ${total_combos}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Total validations: ${TOTAL_MIGRATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Passed validations: ${PASSED_VALIDATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Failed validations: ${FAILED_VALIDATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Cached validations: ${CACHED_VALIDATIONS} of ${TOTAL_MIGRATIONS}"

if [[ ${FAILED_VALIDATIONS} -eq 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All database migration validations passed"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${FAILED_VALIDATIONS} database migration validations failed"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"