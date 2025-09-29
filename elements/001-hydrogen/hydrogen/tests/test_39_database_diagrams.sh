#!/usr/bin/env bash

# Test: Database Diagram Generation
# Generates SVG database diagrams for all engine/design/schema combinations

# FUNCTIONS
# generate_database_diagram()

# CHANGELOG
# 1.0.0 - 2025-09-29 - Initial creation for database diagram generation

set -euo pipefail

# Test configuration
TEST_NAME="Database Diagrams"
TEST_ABBR="ERD"
TEST_NUMBER="39"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS

# Helium project path (absolute path from project root)
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)"
HELIUM_DIR="${PROJECT_DIR}/../../../elements/002-helium"
HELIUM_DIR="$(cd "${HELIUM_DIR}" && pwd)"  # Resolve any .. in path

# List of designs to process
# DESIGNS=("helium" "acuranzo")
DESIGNS=( "acuranzo")

# Supported database engines
ENGINES=("postgresql" "sqlite" "mysql" "db2")

declare -A DESIGN_SCHEMAS
# DESIGN_SCHEMAS["helium"]="helium::helium:HELIUM"
DESIGN_SCHEMAS["acuranzo"]="app::acuranzo:ACURANZO"

# Function to generate database diagram for a specific combination
generate_database_diagram() {
    local design="$1"
    local engine="$2"
    local schema="$3"

    # Find the highest numbered migration file for this design
    local design_dir="${HELIUM_DIR}/${design}"
    local migration_files=()
    if [[ -d "${design_dir}" ]]; then
        while IFS= read -r -d '' file; do
            migration_files+=("${file}")
        done < <("${FIND}" "${design_dir}" -name "${design}_*.lua" -type f -print0 2>/dev/null | sort -z -V || true)
    fi

    if [[ ${#migration_files[@]} -eq 0 ]]; then
        echo "ERROR=No migration files found for ${design}"
        return 1
    fi

    # Get the highest numbered migration (last in sorted array)
    local highest_migration_file="${migration_files[-1]}"
    local migration_name
    migration_name=$(basename "${highest_migration_file}" .lua)
    local migration_num="${migration_name#"${design}"_}"

    # Create output directory
    local output_dir="${PROJECT_DIR}/images/${design}"
    mkdir -p "${output_dir}"

    # Generate filename (lowercase, handle empty schema)
    local schema_part=""
    if [[ -n "${schema}" ]]; then
        schema_part="-${schema,,}"  # ,, converts to lowercase
    fi
    local output_file="${output_dir}/${design,,}-${engine,,}${schema_part}-${migration_num}.svg"

    # Run generate_diagram.sh and capture error output
    local error_file
    error_file=$(mktemp)
    local error_output=""
    if "${SCRIPT_DIR}/lib/generate_diagram.sh" "${engine}" "${design}" "${schema}" "${migration_num}" > "${output_file}" 2> "${error_file}"; then
        # Success
        rm -f "${error_file}"
        return 0
    else
        # Failure - read error output
        error_output=$(cat "${error_file}")
        rm -f "${error_file}"

        # Determine failure reason from error output
        if echo "${error_output}" | grep -q "No JSON data extracted"; then
            echo "ERROR=No diagram data found in migration files"
        elif echo "${error_output}" | grep -q "No diagram data found"; then
            echo "ERROR=No diagram data found in processed migrations"
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
    local result_file="$4"

    # Clear result file
    true > "${result_file}"

    # Run diagram generation and capture output
    local output
    output=$(generate_database_diagram "${design}" "${engine}" "${schema}")
    local exit_code=$?

    if [[ ${exit_code} -eq 0 ]]; then
        echo "SUCCESS" > "${result_file}"
    else
        # Check if output starts with ERROR=
        if [[ "${output}" == ERROR=* ]]; then
            echo "${output}" > "${result_file}"
        else
            echo "ERROR=Unknown failure" > "${result_file}"
        fi
    fi
}

# Start parallel diagram generation
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Diagram Generation in Parallel"

# Start all diagram generations in parallel with job limiting
for design in "${DESIGNS[@]}"; do
    design_schemas="${DESIGN_SCHEMAS[${design}]:-}"
    if [[ -n "${design_schemas}" ]]; then
        IFS=':' read -ra SCHEMA_ARRAY <<< "${design_schemas}"

        # Process each engine/schema combination
        for i in "${!ENGINES[@]}"; do
            engine="${ENGINES[${i}]}"
            schema="${SCHEMA_ARRAY[${i}]:-}"

            # Job limiting - wait if we have too many parallel jobs
            # shellcheck disable=SC2312 # Job control with wc -l is standard practice
            while (( $(jobs -r | wc -l) >= CORES )); do
                wait -n  # Wait for any job to finish
            done

            # Create result file for this combination
            result_file="${LOG_PREFIX}${TIMESTAMP}_${design}_${engine}.result"

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel generation: ${design} ${engine} ${schema}"

            # Run diagram generation in background
            run_diagram_generation_parallel "${design}" "${engine}" "${schema}" "${result_file}" &
            PARALLEL_PIDS+=($!)
        done
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

        # Process each engine/schema combination
        for i in "${!ENGINES[@]}"; do
            engine="${ENGINES[${i}]}"
            schema="${SCHEMA_ARRAY[${i}]:-}"

            if [[ -z "${schema}" ]]; then
                subtest_name="${engine} ${design} (no schema)"
            else
                subtest_name="${engine} ${design} ${schema}"
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${subtest_name}"

            # Check result file
            result_file="${LOG_PREFIX}${TIMESTAMP}_${design}_${engine}.result"
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