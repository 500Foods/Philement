#!/usr/bin/env bash

# Test: Database Diagram Generation
# Generates SVG database diagrams for all engine/design/schema combinations

# FUNCTIONS
# generate_database_diagram()

# CHANGELOG
# 2.0.0 - 2025-11-17 - Fixed diagram numbering bug, added before/after highlighting, generates diagrams for ALL migrations (not just latest)
# 1.2.0 - 2025-11-15 - Fixed filename generation to use last_diagram_migration from metadata
# 1.1.0 - 2025-09-30 - Added metadata, starting with 'Tables included' to output
# 1.0.0 - 2025-09-29 - Initial creation for database diagram generation

set -euo pipefail

# Test configuration
TEST_NAME="Database Diagrams"
TEST_ABBR="ERD"
TEST_NUMBER="71"
TEST_COUNTER=0
TEST_VERSION="2.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS

# Find global project root reliably
find_project_root() {
    # Try to get the script directory using a simple approach
    local script_dir
    # Use readlink -f if available, otherwise use pwd
    if command -v readlink >/dev/null 2>&1; then
        script_dir="$(dirname "$(readlink -f "$0" || true)")"
    else
        script_dir="$(cd "$(dirname "$0")" && pwd)"
    fi

    # Go up directories until we find the global project root (which contains elements/)
    local current_dir="${script_dir}"
    while [[ "${current_dir}" != "/" ]]; do
        # Check if this is the global project root (has elements/ subdirectory)
        if [[ -d "${current_dir}/elements" ]]; then
            echo "${current_dir}"
            return 0
        fi
        current_dir="$(dirname "${current_dir}")"
    done

    # Fallback: go up four levels from tests directory to reach project root
    # tests -> hydrogen -> 001-hydrogen -> elements -> project root
    current_dir="$(cd "${script_dir}/../../.." && pwd)"
    echo "${current_dir}"
    return 0
}

# Calculate absolute path to helium directory
PROJECT_DIR="$(find_project_root)"
HELIUM_DIR="${PROJECT_DIR}/elements/002-helium"
HELIUM_DIR="$(cd "${HELIUM_DIR}" && pwd 2>/dev/null || echo "${HELIUM_DIR}")"

# List of designs to process
# DESIGNS=( "acuranzo" "helium" "glm" "gaius")
DESIGNS=( "acuranzo" )

# Supported database engines
ENGINES=("postgresql" "sqlite" "mysql" "db2")

declare -A DESIGN_SCHEMAS
DESIGN_SCHEMAS["acuranzo"]="app::acuranzo:ACURANZO"
# DESIGN_SCHEMAS["helium"]="helium::helium:HELIUM"
# DESIGN_SCHEMAS["glm"]="glm2:glm2:glm2:GLM2"
# DESIGN_SCHEMAS["gaius"]="gaius:gaius:gaius:GAIUS"

# Function to generate database diagram for a specific migration number
generate_database_diagram() {
    local design="$1"
    local engine="$2"
    local schema="$3"
    local migration_num="$4"

    # Create output directory in hydrogen project (not global project root)
    local hydrogen_dir
    hydrogen_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)"
    local output_dir="${hydrogen_dir}/images/${design}"
    mkdir -p "${output_dir}"

    # Generate filename using requested migration number
    local schema_part=""
    if [[ -n "${schema}" ]]; then
        schema_part="-${schema,,}"  # ,, converts to lowercase
    fi
    
    # Always use the requested migration number for the filename
    local output_file="${output_dir}/${design,,}-${engine,,}${schema_part}-${migration_num}.svg"
    local metadata_file="${output_file%.svg}.metadata"

    # Check if diagram already exists - skip if it does
    if [[ -f "${output_file}" ]]; then
        echo "SKIPPED=Diagram already exists"
        return 0
    fi

    # Run get_diagram.sh and capture both SVG output and metadata
    local error_file
    error_file=$(mktemp)
    local error_output=""
    local metadata=""

    # Capture both stdout (SVG) and stderr (metadata + errors)
    if "${SCRIPT_DIR}/lib/get_diagram.sh" "${engine}" "${design}" "${schema}" "${migration_num}" > "${output_file}" 2> "${error_file}"; then
        # Success - check for metadata in error output
        error_output=$(cat "${error_file}")
        rm -f "${error_file}"

        # Extract metadata if present and save it
        metadata_line=$(echo "${error_output}" | grep "^DIAGRAM_METADATA:" || true)
        
        if [[ -n "${metadata_line}" ]]; then
            metadata="${metadata_line#DIAGRAM_METADATA: }"
            echo "${metadata}" > "${metadata_file}"
        fi

        return 0
    else
        # Failure - read error output
        error_output=$(cat "${error_file}")
        rm -f "${error_file}"

        # Extract metadata if present (even on failure, for debugging)
        metadata_line=$(echo "${error_output}" | grep "^DIAGRAM_METADATA:" || true)
        
        if [[ -n "${metadata_line}" ]]; then
            metadata="${metadata_line#DIAGRAM_METADATA: }"
            echo "${metadata}" > "${metadata_file}"
        fi

        # Determine failure reason from error output
        if echo "${error_output}" | grep -q "No diagram data"; then
            echo "ERROR=No diagram data found in migration files"
        elif [[ -n "${error_output}" ]]; then
            # Extract the last error line for cleaner output
            error_line=$(echo "${error_output}" | grep "^Error:" | tail -1 | sed 's/^Error: //')
            if [[ -n "${error_line}" ]]; then
                echo "ERROR=${error_line}"
            else
                echo "ERROR=Diagram generation failed"
            fi
        else
            echo "ERROR=Diagram generation failed"
        fi
        return 1
    fi
}

# Initialize counters
TOTAL_COMBINATIONS=0
SUCCESSFUL_GENERATIONS=0
FAILED_GENERATIONS=0

# Calculate total combinations (matches execution logic)
for design in "${DESIGNS[@]}"; do
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        TOTAL_COMBINATIONS=$((TOTAL_COMBINATIONS + ${#ENGINES[@]}))
    fi
done

# Update test name with total count and parallel indicator
TEST_NAME="${TEST_NAME} {BLUE}(combos: ${TOTAL_COMBINATIONS}){RESET}"

# Function to run diagram generation in parallel
run_diagram_generation_parallel() {
    local design="$1"
    local engine="$2"
    local schema="$3"
    local migration_num="$4"
    local result_file="$5"

    # Clear result file
    true > "${result_file}"

    # Create metadata file for this combination
    local metadata_file="${result_file%.result}.metadata"

    # Run diagram generation and capture output
    local output
    output=$(generate_database_diagram "${design}" "${engine}" "${schema}" "${migration_num}")
    local exit_code=$?

    if [[ ${exit_code} -eq 0 ]]; then
        if [[ "${output}" == "SKIPPED=Diagram already exists" ]]; then
            echo "${output}" > "${result_file}"
        else
            echo "SUCCESS" > "${result_file}"
        fi
    else
        # Check if output starts with ERROR=
        if [[ "${output}" == ERROR=* ]]; then
            echo "${output}" > "${result_file}"
        else
            echo "ERROR=Unknown failure" > "${result_file}"
        fi
    fi
}

# Function to get the actual migration number for a design
get_migration_number() {
    local design="$1"

    # Find the highest numbered migration file for this design
    local design_dir="${HELIUM_DIR}/${design}/migrations"
    local migration_files=()
    if [[ -d "${design_dir}" ]]; then
        while IFS= read -r -d '' file; do
            migration_files+=("${file}")
        done < <("${FIND}" "${design_dir}" -name "${design}_*.lua" -type f -print0 2>/dev/null | sort -z -V || true)
    fi

    if [[ ${#migration_files[@]} -eq 0 ]]; then
        echo ""
        return 1
    fi

    # Get the highest numbered migration (last in sorted array)
    local highest_migration_file="${migration_files[-1]}"
    local migration_name
    migration_name=$(basename "${highest_migration_file}" .lua)
    local migration_num="${migration_name#"${design}"_}"

    echo "${migration_num}"
}

# Find all migration numbers for each design
declare -A DESIGN_MIGRATIONS
for design in "${DESIGNS[@]}"; do
    design_dir="${HELIUM_DIR}/${design}/migrations"
    if [[ -d "${design_dir}" ]]; then
        migration_nums=()
        while IFS= read -r -d '' file; do
            filename=$(basename "${file}" .lua)
            migration_num=${filename#"${design}_"}
            if [[ "${migration_num}" =~ ^[0-9]+$ ]]; then
                migration_nums+=("${migration_num}")
            fi
        done < <("${FIND}" "${design_dir}" -name "${design}_*.lua" -type f -print0 2>/dev/null | sort -z -V || true)

        # Sort numerically and store
        if [[ ${#migration_nums[@]} -gt 0 ]]; then
            mapfile -t sorted_nums < <(printf '%s\n' "${migration_nums[@]}" | sort -n || true)
            DESIGN_MIGRATIONS["${design}"]="${sorted_nums[*]}"
        fi
    fi
done

# Calculate total combinations (all migrations for all designs/engines)
TOTAL_COMBINATIONS=0
for design in "${DESIGNS[@]}"; do
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        migration_nums_str="${DESIGN_MIGRATIONS[${design}]:-}"
        if [[ -n "${migration_nums_str}" ]]; then
            IFS=' ' read -ra MIGRATION_ARRAY <<< "${migration_nums_str}"
            TOTAL_COMBINATIONS=$((TOTAL_COMBINATIONS + (${#ENGINES[@]} * ${#MIGRATION_ARRAY[@]})))
        fi
    fi
done

# Update test name with total count
TEST_NAME="${TEST_NAME} {BLUE}(combos: ${TOTAL_COMBINATIONS}){RESET}"

# Start parallel diagram generation
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Diagram Generation in Parallel"

# Start all diagram generations in parallel with job limiting
for design in "${DESIGNS[@]}"; do
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        migration_nums_str="${DESIGN_MIGRATIONS[${design}]:-}"

        if [[ -n "${migration_nums_str}" ]]; then
            IFS=' ' read -ra MIGRATION_ARRAY <<< "${migration_nums_str}"
            
            # Process each migration/engine/schema combination
            for migration_num in "${MIGRATION_ARRAY[@]}"; do
                for i in "${!ENGINES[@]}"; do
                    engine="${ENGINES[${i}]}"
                    # Get schema for this engine index, or empty if index is out of bounds
                    if [[ ${i} -lt ${#SCHEMA_ARRAY[@]} ]]; then
                        schema="${SCHEMA_ARRAY[${i}]}"
                    else
                        schema=""
                    fi

                    # Job limiting - wait if we have too many parallel jobs
                    # shellcheck disable=SC2312 # Job control with wc -l is standard practice
                    while (( $(jobs -r | wc -l) >= CORES )); do
                        wait -n  # Wait for any job to finish
                    done

                    # Create result file for this combination
                    result_file="${LOG_PREFIX}_${design}_${engine}_${migration_num}.result"

                    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel generation: ${design} ${engine} ${schema} ${migration_num}"

                    # Run diagram generation in background
                    run_diagram_generation_parallel "${design}" "${engine}" "${schema}" "${migration_num}" "${result_file}" &
                    PARALLEL_PIDS+=($!)
                done
            done
        fi
    fi
done

# Wait for all parallel generations to complete
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#PARALLEL_PIDS[@]} parallel diagram generations to complete"
for pid in "${PARALLEL_PIDS[@]}"; do
    wait "${pid}"
done
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel generations completed"

# Analyze results sequentially for clean output
for design in "${DESIGNS[@]}"; do
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"
        migration_nums_str="${DESIGN_MIGRATIONS[${design}]:-}"

        if [[ -n "${migration_nums_str}" ]]; then
            IFS=' ' read -ra MIGRATION_ARRAY <<< "${migration_nums_str}"
            
            # Process each migration/engine/schema combination
            for migration_num in "${MIGRATION_ARRAY[@]}"; do
                for i in "${!ENGINES[@]}"; do
                    engine="${ENGINES[${i}]}"
                    # Get schema for this engine index, or empty if index is out of bounds
                    if [[ ${i} -lt ${#SCHEMA_ARRAY[@]} ]]; then
                        schema="${SCHEMA_ARRAY[${i}]}"
                    else
                        schema=""
                    fi

                    # Check result file first to see if we should skip output
                    result_file="${LOG_PREFIX}_${design}_${engine}_${migration_num}.result"

                    if [[ -f "${result_file}" ]]; then
                        result_content=$(cat "${result_file}")
                        if [[ "${result_content}" == "SKIPPED=Diagram already exists" ]]; then
                            # Skip all output for existing diagrams
                            SUCCESSFUL_GENERATIONS=$((SUCCESSFUL_GENERATIONS + 1))
                            continue
                        fi
                    fi

                    if [[ -z "${schema}" ]]; then
                        subtest_name="${engine} ${design} ${migration_num} (no schema)"
                    else
                        subtest_name="${engine} ${design} ${schema} ${migration_num}"
                    fi

                    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${subtest_name}"

                    # Generate the correct filename with proper schema handling
                    display_schema=""
                    if [[ ${i} -lt ${#SCHEMA_ARRAY[@]} ]]; then
                        if [[ -n "${SCHEMA_ARRAY[${i}]}" ]]; then
                            display_schema="-${SCHEMA_ARRAY[${i}],,}"
                        fi
                    fi

                    # Calculate output directory
                    hydrogen_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)"
                    output_dir="${hydrogen_dir}/images/${design}"

                    # Metadata file uses requested migration number
                    metadata_file="${output_dir}/${design,,}-${engine,,}${display_schema}-${migration_num}.metadata"

                    # Show table count and highlighting statistics from metadata
                    if [[ -f "${metadata_file}" ]]; then
                        metadata=$(cat "${metadata_file}")

                        table_count=$(echo "${metadata}" | jq -r '.table_count // empty' 2>/dev/null || true)
                        if [[ -n "${table_count}" ]] && [[ "${table_count}" != "null" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Tables included: ${table_count}"
                        fi

                        highlighted_tables=$(echo "${metadata}" | jq -r '.highlighted_tables // empty' 2>/dev/null || true)
                        highlighted_columns=$(echo "${metadata}" | jq -r '.highlighted_columns // empty' 2>/dev/null || true)

                        if [[ -n "${highlighted_tables}" ]] && [[ "${highlighted_tables}" != "null" ]] && [[ "${highlighted_tables}" != "0" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Highlighted: ${highlighted_tables} tables, ${highlighted_columns} columns"
                        fi

                        # Clean up metadata file after use
                        rm -f "${metadata_file}"
                    fi

                    # Extract relative path from hydrogen directory
                    relative_result_file="${result_file#*hydrogen/hydrogen/}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results in ${relative_result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Diagram in images/${design}/${design}-${engine}${display_schema}-${migration_num}.svg"

                    if [[ -f "${result_file}" ]]; then
                        result_content=$(cat "${result_file}")
                        if [[ "${result_content}" == "SUCCESS" ]]; then
                            SUCCESSFUL_GENERATIONS=$((SUCCESSFUL_GENERATIONS + 1))
                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Diagram generation successful"
                        elif [[ "${result_content}" == ERROR=* ]]; then
                            FAILED_GENERATIONS=$((FAILED_GENERATIONS + 1))
                            error_msg="${result_content#ERROR=}"
                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Diagram generation failed: ${error_msg}"
                            EXIT_CODE=1
                        else
                            FAILED_GENERATIONS=$((FAILED_GENERATIONS + 1))
                            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Diagram generation failed: ${result_content}"
                            EXIT_CODE=1
                        fi
                    else
                        FAILED_GENERATIONS=$((FAILED_GENERATIONS + 1))
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Diagram generation failed: no result file"
                        EXIT_CODE=1
                    fi
                done
            done
        fi
    fi
done

# Summary
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Diagram Generation Summary:"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Total combinations: ${TOTAL_COMBINATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Successful generations: ${SUCCESSFUL_GENERATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Failed generations: ${FAILED_GENERATIONS}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Diagram Generation Complete."

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"