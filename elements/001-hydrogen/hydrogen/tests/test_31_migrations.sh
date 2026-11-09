#!/usr/bin/env bash

# Test: Database Migration Validation
# Performs SQL validation on database migration files using sqruff

# FUNCTIONS
# get_file_hash()
# check_json_payloads()
# validate_migration()

# CHANGELOG
# 1.14.0 - 2026-11-09 - Add yugabytedb to ENGINES; emit fresh| when sqruff is absent; fix indent
# 1.13.0 - 2026-11-09 - Fail a migration whose JSON_INGEST_START, JSON_INGEST_SCHEMA_START, or JIS payload is not an object
# 1.12.0 - 2026-10-16 - Include courses in the suite; skip it until its migrations exist
# 1.11.0 - 2026-10-16 - Require the shared helium/database*.lua set before expanding
# 1.10.0 - 2026-10-16 - Drop the retired helium printing design from DESIGNS
# 1.9.0 - 2026-10-05 - Lint the argent design with the same schema map as acuranzo
# 1.8.0 - 2026-09-29 - Added mssql engine to ENGINES array and DESIGN_SCHEMAS; skip sqruff for mssql
# 1.7.0 - 2026-09-19 - Removed firebase engine (C-level Firebase fully removed)
# 1.6.0 - 2026-09-19 - Fixed cache check to recognize SKIPPED results as success
# 1.5.0 - 2026-09-18 - Added firebird engine to ENGINES array and DESIGN_SCHEMAS
# 1.4.0 - 2026-09-16 - Added firebase engine; skip sqruff (FB_* is not postgres SQL)
# 1.3.0 - 2025-11-16 - Modified unsubstituted variables check to only flag ${} patterns, not JSON braces
# 1.2.0 - 2025-10-29 - Added check for unsubstituted variables in generated SQL
# 1.1.0 - 2025-09-30 - Updated location of Helium migration files
# 1.0.0 - 2025-09-14 - Initial creation for database migration validation

set -euo pipefail

# Test configuration
TEST_NAME="Migrations"
TEST_ABBR="MGR"
TEST_NUMBER="31"
TEST_COUNTER=0
TEST_VERSION="1.14.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/engine_utils.sh # Resolve path statically
[[ -n "${ENGINE_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/engine_utils.sh"
setup_test_environment

# Test Setup
CACHE_DIR="${HOME}/.cache/hydrogen/sqruff"
mkdir -p "${CACHE_DIR}"

# Helium project path (relative to workspace root) - points to parent to allow appending design names
HELIUM_DIR="../../../elements/002-helium"

# Full suite order. courses is skipped below when that folder does not exist yet.
DESIGNS=("acuranzo" "courses" "argent")

# Supported database engines
ENGINES=("postgresql" "sqlite" "mysql" "db2" "mariadb" "firebird" "yugabytedb" "mssql")

# Schema mapping per design per engine (corresponding to ENGINES array order)
# ENGINES=("postgresql" "sqlite" "mysql" "db2" "mariadb" "firebird" "yugabytedb" "mssql")
declare -A DESIGN_SCHEMAS
DESIGN_SCHEMAS["acuranzo"]="app::acuranzo:ACURANZO:test:testfb:test:testms"
DESIGN_SCHEMAS["courses"]="app::acuranzo:ACURANZO:test:testfb:test:testms"
DESIGN_SCHEMAS["argent"]="app::acuranzo:ACURANZO:test:testfb:test:testms"

# HYDROGEN_ENGINES decides which engines are validated (indexes still follow ENGINES)
ENABLED_ENGINES=()
SKIPPED_ENGINES=()
declare -A ENGINE_IS_ENABLED
for engine in "${ENGINES[@]}"; do
    # shellcheck disable=SC2310 # engine_enabled is a test, its status is the point
    if engine_enabled "${engine}"; then
        ENABLED_ENGINES+=("${engine}")
        ENGINE_IS_ENABLED["${engine}"]=true
    else
        SKIPPED_ENGINES+=("${engine}")
    fi
done
warn_unknown_engines
if [[ ${#SKIPPED_ENGINES[@]} -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipped by HYDROGEN_ENGINES: ${SKIPPED_ENGINES[*]}"
fi

# Function to get file hash (using md5sum or equivalent)
get_file_hash() {
    # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
    "${MD5SUM}" "$1" | "${AWK}" '{print $1}' || true
}

# JSON payloads must be objects. DB2's JSON2BSON rejects a top-level array,
# and the payload is encoded in the generated SQL, so sqruff never sees it.
# Matches ${JSON_INGEST_START}, ${JSON_INGEST_SCHEMA_START}, or ${JIS} followed
# by a long bracket or a quote whose first non-space character is not '{'.
JSON_PAYLOAD_PATTERN='\$\{(JSON_INGEST_START|JSON_INGEST_SCHEMA_START|JIS)\}\s*(\[=*\[|'"'"')\s*(?!\{)\S'

# Function to check JSON payload shape in a migration source file.
# Runs on every pass and is not cached, so a cached PASSED result never hides it.
check_json_payloads() {
    local design="$1"
    local migration_num="$2"
    local migration_file="$3"

    if ! "${GREP}" -Pzq -- "${JSON_PAYLOAD_PATTERN}" "${migration_file}" 2>/dev/null; then
        return 0
    fi

    local diags_file="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${design}_${migration_num}_json_payload.txt"
    mkdir -p "$(dirname "${diags_file}")"
    {
        echo "JSON payload is not an object (DB2 JSON2BSON rejects top-level arrays):"
        "${GREP}" -nP -- "${JSON_PAYLOAD_PATTERN}" "${migration_file}" || true
    } > "${diags_file}"
    echo "${diags_file}"
    return 1
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
        if [[ "${cached_result}" == *"PASSED"* || "${cached_result}" == *"SKIPPED"* ]]; then
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
    if echo "${sql_output}" | grep -q "\${[^}]*}"; then
        # Found unsubstituted variables, save error to diags_file
        local diags_file="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${design}_${engine}_${schema_name}_${migration_num}.txt"
        mkdir -p "$(dirname "${diags_file}")"
        echo "Unsubstituted variables found in generated SQL:" > "${diags_file}"
        echo "${sql_output}" >> "${diags_file}"
        echo "❌ ${design} ${engine} ${migration} FAILED (unsubstituted variables)" > "${cache_file}"
        echo "fresh|${diags_file}"
        return 1
    fi

    # Firebird and MSSQL emit SQL that sqruff (postgres dialect) cannot lint
    if [[ "${engine}" == "firebird" || "${engine}" == "mssql" ]]; then
        echo "✅ ${design} ${engine} ${migration} SKIPPED (sqruff not applicable)" > "${cache_file}"
        echo "fresh|"
        return 0
    fi

    # Check if sqruff is available
    if ! command -v sqruff >/dev/null 2>&1; then
        echo "✅ ${design} ${engine} ${migration} SKIPPED (sqruff not found)" > "${cache_file}"
        echo "fresh|"
        return 0
    fi

    # Use sqruff config file for the specific engine (copied to hydrogen root)
    local config_file=".sqruff_${engine}"
    if [[ ! -f "${config_file}" ]]; then
        echo "✅ ${design} ${engine} ${migration} SKIPPED (config missing)" > "${cache_file}"
        echo "fresh|"
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

# The payload and the C loader share one helium/database.lua set.
SHARED_DATABASE="${HELIUM_DIR}/helium"
if [[ ! -f "${SHARED_DATABASE}/database.lua" ]]; then
    echo "Missing ${SHARED_DATABASE}/database.lua" >&2
    exit 1
fi
for engine in "${ENGINES[@]}"; do
    if [[ ! -f "${SHARED_DATABASE}/database_${engine}.lua" ]]; then
        echo "Missing ${SHARED_DATABASE}/database_${engine}.lua" >&2
        exit 1
    fi
done

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
                [[ -n "${ENGINE_IS_ENABLED[${ENGINES[${i}]}]:-}" ]] || continue
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

            # JSON payload shape: checked once per file, before any cached result
            json_diags=$(check_json_payloads "${design}" "${migration_num}" "${migration_file}"; echo "exit:$?")
            json_failed=false
            [[ "${json_diags##*exit:}" -eq 0 ]] || json_failed=true
            json_diags=${json_diags%exit:*}
            json_diags=${json_diags%$'\n'}

            # Validate against each database engine
            for i in "${!ENGINES[@]}"; do
                engine="${ENGINES[${i}]}"
                [[ -n "${ENGINE_IS_ENABLED[${engine}]:-}" ]] || continue
                schema="${SCHEMA_ARRAY[${i}]:-}"

                # Call function and capture both status and result
                if [[ "${json_failed}" == true ]]; then
                    validation_result=1
                    cache_status="fresh"
                    diags_file="${json_diags}"
                else
                    cache_info=$(validate_migration "${design}" "${engine}" "${migration_num}" "${migration_file}"; echo "exit:$?")
                    validation_result=${cache_info##*exit:}
                    cache_info=${cache_info%exit:*}
                    IFS='|' read -r cache_status diags_file <<< "${cache_info}"
                fi

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
    total_migration_files=$(( total_combos / ${#ENABLED_ENGINES[@]} ))
    TEST_NAME="${TEST_NAME}  {BLUE}${TOTAL_DESIGNS} designs, ${total_migration_files} migrations, ${TOTAL_MIGRATIONS} combos{RESET}"
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