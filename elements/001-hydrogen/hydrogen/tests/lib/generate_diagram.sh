#!/usr/bin/env bash

# generate-diagram.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 1.2.0 - 2025-11-15 - Fixed Base64 decoding to handle BASE64DECODE wrapper functions
# 1.1.0 - 2025-09-30 - Passing metadata
# 1.0.0 - 2025-09-28 - Initial creation

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}"

# Find global project root reliably
find_project_root() {
    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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

    # Fallback: go up four levels from lib directory to reach project root
    # lib -> tests -> hydrogen -> 001-hydrogen -> elements -> project root
    current_dir="$(cd "${script_dir}/../../.." && pwd)"
    echo "${current_dir}"
    return 0
}

# Calculate absolute path to helium directory
PROJECT_DIR="$(find_project_root)"
HELIUM_DIR="${PROJECT_DIR}/elements/002-helium"
HELIUM_DIR="$(cd "${HELIUM_DIR}" && pwd 2>/dev/null || echo "${HELIUM_DIR}")"

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
DESIGN_DIR="${HELIUM_DIR}/${DESIGN}/migrations"
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
# trap 'rm -f "${TEMP_JSON}"' EXIT

# Initialize combined JSON as empty array
echo "[]" > "${TEMP_JSON}"

# Track the last migration that actually had diagram data
LAST_DIAGRAM_MIGRATION=""

# Process each migration file
for MIGRATION_FILE in "${FILTERED_MIGRATION_FILES[@]}"; do
    # Extract migration number from filename
    filename=$(basename "${MIGRATION_FILE}" .lua)
    migration_num=${filename#"${DESIGN}_"}

    echo "Processing ${DESIGN}_${migration_num}.lua..." >&2

    # Run Lua script to generate SQL for this migration (from lib directory for consistent paths)
    pushd "${LIB_DIR}" >/dev/null
    SQL_OUTPUT=$(LUA_PATH="./?.lua;${DESIGN_DIR}/?.lua" lua "./get_migration.lua" "${ENGINE}" "${DESIGN}" "${PREFIX}" "${migration_num}")
    popd >/dev/null

    # Extract JSON from the diagram migration using DIAGRAM_START and DIAGRAM_END markers
    RAW_BLOCK=$(echo "${SQL_OUTPUT}" | sed -n '/-- DIAGRAM_START/,/-- DIAGRAM_END/p' | sed '1,2d;$d')

    if [[ ${#RAW_BLOCK} -eq 0 ]]; then
        # No diagram data in this migration - skip it silently
        continue
    fi
    
    # This migration has diagram data - track it
    LAST_DIAGRAM_MIGRATION="${migration_num}"

    # Clean up the extracted JSON
    # Step 1: Remove DIAGRAM_START, DIAGRAM_END, and outer single quotes
    JSON_DATA=$(printf '%s' "${RAW_BLOCK}" | \
        sed '/DIAGRAM_START/d; /DIAGRAM_END/d; s/^'\''//; s/'\''$//' | head -n -1)

    # Step 2: Remove JSON_INGEST_START and JSON_INGEST_END wrappers if present
    case "${ENGINE}" in
        sqlite)
            JSON_INGEST_START="("
            JSON_INGEST_END=")"
            ;;
        postgresql|mysql|db2)
            # For these engines, JSON_INGEST_START includes schema prefix like "ACURANZOjson_ingest ("
            # We need to match the pattern dynamically
            JSON_INGEST_START="json_ingest[ ]*("
            JSON_INGEST_END=")"
            ;;
        *)
            JSON_INGEST_START=""
            JSON_INGEST_END=""
            ;;
    esac

    if [[ -n "${JSON_INGEST_START}" && -n "${JSON_INGEST_END}" ]]; then
        # Remove the JSON_INGEST wrappers
        JSON_DATA=$(printf '%s' "${JSON_DATA}" | sed "s/^${JSON_INGEST_START}//; s/${JSON_INGEST_END}$//")
    fi

    # Step 3: Handle BASE64DECODE wrapper if present
    ESCAPED_ALREADY=false
    
    # Check if the data contains BASE64DECODE or BASE64_START pattern
    if printf '%s' "${JSON_DATA}" | grep -qiE 'BASE64DECODE'; then
        
        # Extract Base64 content from within BASE64DECODE('content')
        # This handles multiline Base64 strings by extracting everything between the first and last single quote
        BASE64_CONTENT=$(printf '%s' "${JSON_DATA}" | awk '
            BEGIN { content = ""; in_quote = 0; ORS = ""; sq = "\047" }
            {
                for (i = 1; i <= length($0); i++) {
                    char = substr($0, i, 1)
                    if (char == sq && !in_quote) {
                        in_quote = 1
                    } else if (char == sq && in_quote) {
                        # Found closing quote - we have our content
                        exit
                    } else if (in_quote) {
                        content = content char
                    }
                }
                if (in_quote) {
                    content = content "\n"
                }
            }
            END { print content }
        ')
        
        if [[ -n "${BASE64_CONTENT}" ]]; then
            # Remove any whitespace/newlines from Base64 string before decoding
            BASE64_CLEAN=$(printf '%s' "${BASE64_CONTENT}" | tr -d '\n\r\t ')
            
            # Decode the Base64 string
            DECODED_DATA=$(printf '%s' "${BASE64_CLEAN}" | base64 -d 2>/dev/null || true)
            DECODE_STATUS=$?
            
            if [[ ${DECODE_STATUS} -eq 0 ]] && [[ -n "${DECODED_DATA}" ]]; then
                
                # Apply escape processing to decoded data before validation
                DECODED_DATA=$(printf '%s' "${DECODED_DATA}" | awk '
                BEGIN {
                    in_string = 0
                    escaped = 0
                    result = ""
                }
                {
                    line = $0
                    for (i = 1; i <= length(line); i++) {
                        char = substr(line, i, 1)
                        if (escaped) {
                            result = result char
                            escaped = 0
                        } else if (char == "\\") {
                            result = result char
                            escaped = 1
                        } else if (char == "\"") {
                            result = result char
                            in_string = !in_string
                        } else if (in_string && char == "\t") {
                            result = result "\\t"
                        } else {
                            result = result char
                        }
                    }
                    if (in_string) {
                        result = result "\\n"
                    } else {
                        result = result "\n"
                    }
                }
                END {
                    if (length(result) > 0 && substr(result, length(result), 1) == "\n") {
                        result = substr(result, 1, length(result) - 1)
                    }
                    printf "%s", result
                }
                ')
                
                # Now verify the processed data is valid JSON
                if printf '%s' "${DECODED_DATA}" | jq empty >/dev/null 2>&1; then
                    JSON_DATA="${DECODED_DATA}"
                    ESCAPED_ALREADY=true
                fi
            fi
        fi
    else
        # No BASE64DECODE wrapper, check if content itself is Base64
        # Remove whitespace and check if it looks like Base64
        BASE64_TEST=$(printf '%s' "${JSON_DATA}" | tr -d '\n\r\t ')
        
        if printf '%s' "${BASE64_TEST}" | grep -qE '^[A-Za-z0-9+/]{20,}={0,2}$'; then
            if DECODED_DATA=$(printf '%s' "${BASE64_TEST}" | base64 -d 2>/dev/null) && [[ -n "${DECODED_DATA}" ]] && printf '%s' "${DECODED_DATA}" | jq empty >/dev/null 2>&1; then
                JSON_DATA="${DECODED_DATA}"
            fi
        fi
    fi

    # Step 4: Escape newlines and tabs only within JSON strings (skip if already done)
    if [[ "${ESCAPED_ALREADY}" != "true" ]]; then
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
    fi

    # Basic validation
    if [[ "${JSON_DATA}" == "" ]]; then
        echo "Warning: No JSON data extracted from ${DESIGN}_${migration_num}.lua, skipping..." >&2
        continue
    fi

    # Combine this migration's JSON with the accumulated JSON using jq
    jq -s '.[0] + .[1].diagram' "${TEMP_JSON}" <(echo "${JSON_DATA}") > "${TEMP_JSON}.tmp" && mv "${TEMP_JSON}.tmp" "${TEMP_JSON}"
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

# Generate diagram and capture both stdout (SVG) and stderr (metadata)
# Use unique temporary file to avoid conflicts in parallel execution
METADATA_TMP=$(mktemp)
SVG_OUTPUT=$(echo "${JSON_DATA}" | node "${LIB_DIR}/generate_diagram.js" "${ENGINE}" 2>"${METADATA_TMP}")

# Extract metadata from stderr if available
METADATA=""
if [[ -f "${METADATA_TMP}" ]] && [[ -s "${METADATA_TMP}" ]]; then
    # Find the METADATA line in stderr output
    METADATA_LINE=$(grep "^METADATA:" "${METADATA_TMP}" 2>/dev/null || true)

    if [[ -n "${METADATA_LINE}" ]] && [[ -n "${LAST_DIAGRAM_MIGRATION}" ]]; then
        # Extract JSON part after "METADATA:"
        METADATA_JSON="${METADATA_LINE#METADATA: }"

        # Validate that it's valid JSON and add last_diagram_migration
        if jq empty <<< "${METADATA_JSON}" 2>/dev/null; then
            # Add the last diagram migration number to the metadata
            METADATA=$(jq --arg lastmig "${LAST_DIAGRAM_MIGRATION}" '. + {last_diagram_migration: $lastmig}' <<< "${METADATA_JSON}")
        fi
    fi

    # Clean up temporary file
    rm -f "${METADATA_TMP}"
fi

# Output SVG to stdout (main purpose)
echo "${SVG_OUTPUT}"

# Output metadata to stderr in a format the calling script can easily parse
if [[ -n "${METADATA}" ]]; then
    echo "DIAGRAM_METADATA: ${METADATA}" >&2
fi

echo "Diagram generation complete." >&2
