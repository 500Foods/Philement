#!/usr/bin/env bash

# generate-diagram.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 1.0.0 - 2025-09-28 - Initial creation

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}"
HELIUM_DIR="${SCRIPT_DIR}/../../../../../elements/002-helium"

# Function to display usage
usage() {
    echo "Usage: $0 <engine> <design> <prefix> <migration>"
    echo "  engine: database engine (db2, postgresql, mysql, sqlite)"
    echo "  design: design name (acuranzo, helium)"
    echo "  prefix: schema prefix (ACURANZO, HELIUM, etc.)"
    echo "  migration: migration number (1000, etc.)"
    echo ""
    echo "Example: $0 db2 acuranzo ACURANZO 1000"
    exit 1
}

# Check arguments
if [[ $# -ne 4 ]]; then
    usage
fi

ENGINE="$1"
DESIGN="$2"
PREFIX="$3"
MIGRATION="$4"

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

# Validate migration file exists
MIGRATION_FILE="${DESIGN_DIR}/${DESIGN}_${MIGRATION}.lua"
if [[ ! -f "${MIGRATION_FILE}" ]]; then
    echo "Error: Migration file not found: ${MIGRATION_FILE}" >&2
    exit 1
fi

# Create temporary file for JSON
TEMP_JSON=$(mktemp)
trap 'rm -f "${TEMP_JSON}"' EXIT

# Run Lua script to generate SQL
echo "Generating SQL for ${DESIGN} ${ENGINE} migration ${MIGRATION}..." >&2
SQL_OUTPUT=$(LUA_PATH="${LIB_DIR}/?.lua;${DESIGN_DIR}/?.lua" lua "${LIB_DIR}/get_migration_wrapper.lua" "${ENGINE}" "${DESIGN}" "${PREFIX}" "${MIGRATION}")

# Extract JSON from the diagram migration using DIAGRAM_START and DIAGRAM_END markers
RAW_BLOCK=$(echo "${SQL_OUTPUT}" | sed -n '/-- DIAGRAM_START/,/-- DIAGRAM_END/p' | sed '1d;$d')

if [[ ${#RAW_BLOCK} -eq 0 ]]; then
    echo "Error: No diagram data found in migration output" >&2
    exit 1
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
    echo "Error: No JSON data extracted from migration" >&2
    exit 1
fi

# Generate diagram
echo "Generating SVG diagram..." >&2
echo "${JSON_DATA}" | node "${LIB_DIR}/generate_diagram.js"

echo "Diagram generation complete." >&2