#!/usr/bin/env bash

# generate-diagram.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 1.0.0 - 2025-09-28 - Initial creation

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}"

# Calculate absolute path to helium directory from project root
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
HELIUM_DIR="${PROJECT_DIR}/../../../elements/002-helium"
HELIUM_DIR="$(cd "${HELIUM_DIR}" && pwd)"  # Resolve any .. in path

# Function to display usage
usage() {
    echo "Usage: $0 <engine> <design> <prefix> [migration]"
    echo "  engine: database engine (db2, postgresql, mysql, sqlite)"
    echo "  design: design name (acuranzo, helium)"
    echo "  prefix: schema prefix (ACURANZO, HELIUM, etc.)"
    echo "  migration: optional migration number (1000, etc.) - if not provided, process all migrations"
    echo ""
    echo "Examples:"
    echo "  $0 db2 acuranzo ACURANZO 1000    # Process migration 1000 only"
    echo "  $0 db2 acuranzo ACURANZO         # Process all migrations up to latest"
    exit 1
}

# Check arguments (3 or 4 parameters)
if [[ $# -lt 3 ]] || [[ $# -gt 4 ]]; then
    usage
fi

ENGINE="$1"
DESIGN="$2"
PREFIX="$3"
MIGRATION="${4:-}"  # Optional migration parameter

# Validate engine
case "${ENGINE}" in
    db2|postgresql|mysql|sqlite) ;;
    *) echo "Error: Invalid engine '${ENGINE}'" >&2; usage ;;
esac

# Validate design directory exists
DESIGN_DIR="${HELIUM_DIR}/${DESIGN}"
if [[ ! -d "${DESIGN_DIR}" ]]; then
    echo "Error: Design directory not found: ${DESIGN_DIR}" >&2
    exit 1
fi

# Find all migration files for this design
MIGRATION_FILES=()
while IFS= read -r -d '' file; do
    MIGRATION_FILES+=("${file}")
done < <(find "${DESIGN_DIR}" -name "${DESIGN}_*.lua" -type f -print0 2>/dev/null | sort -z -V || true)

if [[ ${#MIGRATION_FILES[@]} -eq 0 ]]; then
    echo "Error: No migration files found for design '${DESIGN}' in ${DESIGN_DIR}" >&2
    exit 1
fi

# Filter migration files based on optional migration parameter
FILTERED_MIGRATION_FILES=()
if [[ -n "${MIGRATION}" ]]; then
    # Process migrations up to and including the specified number
    for file in "${MIGRATION_FILES[@]}"; do
        # Extract migration number from filename (e.g., acuranzo_1000.lua -> 1000)
        filename=$(basename "${file}" .lua)
        migration_num=${filename#"${DESIGN}_"}

        # Only include if migration number <= specified migration
        if [[ "${migration_num}" =~ ^[0-9]+$ ]] && (( migration_num <= MIGRATION )); then
            FILTERED_MIGRATION_FILES+=("${file}")
        fi
    done

    if [[ ${#FILTERED_MIGRATION_FILES[@]} -eq 0 ]]; then
        echo "Error: No migration files found up to migration ${MIGRATION}" >&2
        exit 1
    fi
else
    # No migration specified, use all files
    FILTERED_MIGRATION_FILES=("${MIGRATION_FILES[@]}")
fi

# Create temporary file for combined JSON
TEMP_JSON=$(mktemp)
trap 'rm -f "${TEMP_JSON}"' EXIT

# Initialize combined JSON as empty array
echo "[]" > "${TEMP_JSON}"

# Process each migration file
for MIGRATION_FILE in "${FILTERED_MIGRATION_FILES[@]}"; do
    # Extract migration number from filename
    filename=$(basename "${MIGRATION_FILE}" .lua)
    migration_num=${filename#"${DESIGN}_"}

    echo "Processing ${DESIGN}_${migration_num}.lua..." >&2

    # Run Lua script to generate SQL for this migration (from lib directory for consistent paths)
    pushd "${LIB_DIR}" >/dev/null
    SQL_OUTPUT=$(LUA_PATH="./?.lua;${DESIGN_DIR}/?.lua" lua "./get_migration_wrapper.lua" "${ENGINE}" "${DESIGN}" "${PREFIX}" "${migration_num}")
    popd >/dev/null

    # Extract JSON from the diagram migration using DIAGRAM_START and DIAGRAM_END markers
    RAW_BLOCK=$(echo "${SQL_OUTPUT}" | sed -n '/-- DIAGRAM_START/,/-- DIAGRAM_END/p' | sed '1d;$d')

    if [[ ${#RAW_BLOCK} -eq 0 ]]; then
        echo "Warning: No diagram data found in ${DESIGN}_${migration_num}.lua, skipping..." >&2
        continue
    fi

    # Clean up the extracted JSON
    # Step 1: Remove DIAGRAM_START, DIAGRAM_END, and outer single quotes
    JSON_DATA=$(printf '%s' "${RAW_BLOCK}" | \
        sed '/DIAGRAM_START/d; /DIAGRAM_END/d; s/^'\''//; s/'\''$//')

    # Step 2: Escape newlines and tabs only within JSON strings
    JSON_DATA=$(printf '%s' "${JSON_DATA}" | awk '
    BEGIN {
        in_string = 0
        escaped = 0
        result = ""
    }

    {
        line = $0

        # Process each character in the line
        for (i = 1; i <= length(line); i++) {
            char = substr(line, i, 1)

            if (escaped) {
                # Previous character was a backslash, so this character is escaped
                result = result char
                escaped = 0
            } else if (char == "\\") {
                # This is an escape character
                result = result char
                escaped = 1
            } else if (char == "\"") {
                # Quote character - toggle string state
                result = result char
                in_string = !in_string
            } else if (in_string && char == "\t") {
                # Tab inside a string - escape it
                result = result "\\t"
            } else {
                # Regular character
                result = result char
            }
        }

        # Handle end of line
        if (in_string) {
            # We are inside a string, so escape the newline
            result = result "\\n"
        } else {
            # We are outside a string, preserve the newline
            result = result "\n"
        }
    }

    END {
        # Remove the trailing newline we added in the last iteration
        if (length(result) > 0 && substr(result, length(result), 1) == "\n") {
            result = substr(result, 1, length(result) - 1)
        }
        printf "%s", result
    }
    ')

    # Basic validation
    if [[ "${JSON_DATA}" == "" ]]; then
        echo "Warning: No JSON data extracted from ${DESIGN}_${migration_num}.lua, skipping..." >&2
        continue
    fi

    # Combine this migration's JSON with the accumulated JSON using jq
    jq -s '.[0] + .[1]' "${TEMP_JSON}" <(echo "${JSON_DATA}") > "${TEMP_JSON}.tmp" && mv "${TEMP_JSON}.tmp" "${TEMP_JSON}"
done

# Check if we have any data
json_length=$(jq '. | length' "${TEMP_JSON}")
if [[ "${json_length}" -eq 0 ]]; then
    echo "Error: No JSON data extracted from any migration files" >&2
    exit 1
fi

# Read the combined JSON
JSON_DATA=$(cat "${TEMP_JSON}")

# Generate diagram
migration_count=${#FILTERED_MIGRATION_FILES[@]}
if [[ ${migration_count} -eq 1 ]]; then
    echo "Generating SVG diagram from 1 migration..." >&2
else
    echo "Generating SVG diagram from ${migration_count} migrations..." >&2
fi
echo "${JSON_DATA}" | node "${LIB_DIR}/generate_diagram.js"

echo "Diagram generation complete." >&2