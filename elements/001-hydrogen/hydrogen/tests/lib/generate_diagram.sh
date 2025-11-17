#!/usr/bin/env bash

# generate-diagram.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 2.0.0 - 2025-11-17 - Implemented before/after comparison with automatic highlighting, fixed diagram numbering bug, improved base64 extraction to handle all database engine wrappers
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

# Create temporary files for before and after states
TEMP_BEFORE_JSON=$(mktemp)
TEMP_AFTER_JSON=$(mktemp)
# trap 'rm -f "${TEMP_BEFORE_JSON}" "${TEMP_AFTER_JSON}"' EXIT

# Initialize both as empty arrays
echo "[]" > "${TEMP_BEFORE_JSON}"
echo "[]" > "${TEMP_AFTER_JSON}"

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

    # Step 3: Handle BASE64 wrapper if present
    ESCAPED_ALREADY=false
    
    # Generic base64 detection: Look for any function that suggests base64 encoding/decoding
    # Patterns: BASE64DECODE, DECODE, from_base64, CRYPTO_DECODE, CONVERT_FROM
    if printf '%s' "${JSON_DATA}" | grep -qiE '(BASE64|DECODE|from_base64|CRYPTO_DECODE|CONVERT_FROM)'; then
        
        # Strategy: Extract the FIRST quoted string (single or double quotes)
        # This works for all our database wrapper patterns:
        # - DB2: BASE64DECODE('base64data')
        # - PostgreSQL: CONVERT_FROM(DECODE('base64data', 'base64'), 'UTF8')
        # - MySQL: cast(from_base64('base64data') as char...)
        # - SQLite: CRYPTO_DECODE('base64data','base64')
        #
        # The base64 data is always the innermost/first quoted string
        BASE64_CONTENT=$(printf '%s' "${JSON_DATA}" | awk '
            BEGIN {
                content = "";
                in_quote = 0;
                ORS = "";
                sq = "\047";  # Single quote
                quote_char = "";
            }
            {
                line = $0
                for (i = 1; i <= length(line); i++) {
                    char = substr(line, i, 1)
                    
                    if (!in_quote) {
                        # Looking for opening single quote only (our data uses single quotes)
                        if (char == sq) {
                            in_quote = 1
                            quote_char = sq
                        }
                    } else {
                        # In quote - check for closing
                        if (char == quote_char) {
                            # Found closing quote - done with first string
                            exit
                        } else {
                            content = content char
                        }
                    }
                }
                # If still in quote at EOL, add newline to content
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

    # Add to AFTER state (always, since we're processing migrations up to the requested one)
    jq -s '.[0] + .[1].diagram' "${TEMP_AFTER_JSON}" <(echo "${JSON_DATA}") > "${TEMP_AFTER_JSON}.tmp" && mv "${TEMP_AFTER_JSON}.tmp" "${TEMP_AFTER_JSON}"
    
    # Add to BEFORE state (only if this migration is BEFORE the requested migration)
    if [[ -n "${MIGRATION}" ]] && (( migration_num < MIGRATION )); then
        jq -s '.[0] + .[1].diagram' "${TEMP_BEFORE_JSON}" <(echo "${JSON_DATA}") > "${TEMP_BEFORE_JSON}.tmp" && mv "${TEMP_BEFORE_JSON}.tmp" "${TEMP_BEFORE_JSON}"
    fi
done

# Check if we have any data in AFTER state
after_length=$(jq '. | length' "${TEMP_AFTER_JSON}")
if [[ "${after_length}" -eq 0 ]]; then
    echo "Error: No diagram data found in any migration up to ${MIGRATION:-latest}" >&2
    exit 1
fi

# Create combined before/after JSON structure
BEFORE_DATA=$(cat "${TEMP_BEFORE_JSON}")
AFTER_DATA=$(cat "${TEMP_AFTER_JSON}")

COMBINED_JSON=$(jq -n \
    --argjson before "${BEFORE_DATA}" \
    --argjson after "${AFTER_DATA}" \
    '{before: {diagram: $before}, after: {diagram: $after}}')

# Report state counts
before_count=$(jq '. | length' "${TEMP_BEFORE_JSON}" || true)
after_count=$(jq '. | length' "${TEMP_AFTER_JSON}" || true)
echo "Before state: ${before_count} tables, After state: ${after_count} tables" >&2

# Generate diagram with before/after comparison
migration_count=${#FILTERED_MIGRATION_FILES[@]}
if [[ ${migration_count} -eq 1 ]]; then
    echo "Generating SVG diagram from 1 migration..." >&2
else
    echo "Generating SVG diagram from ${migration_count} migrations..." >&2
fi

# Generate diagram and capture both stdout (SVG) and stderr (metadata)
# Use unique temporary file to avoid conflicts in parallel execution
METADATA_TMP=$(mktemp)
SVG_OUTPUT=$(echo "${COMBINED_JSON}" | node "${LIB_DIR}/generate_diagram.js" "${ENGINE}" 2>"${METADATA_TMP}")

# Extract metadata from stderr if available
METADATA=""
if [[ -f "${METADATA_TMP}" ]] && [[ -s "${METADATA_TMP}" ]]; then
    # Find the METADATA line in stderr output
    METADATA_LINE=$(grep "^METADATA:" "${METADATA_TMP}" 2>/dev/null || true)

    if [[ -n "${METADATA_LINE}" ]]; then
        # Extract JSON part after "METADATA:"
        METADATA_JSON="${METADATA_LINE#METADATA: }"

        # Validate that it's valid JSON and add requested_migration
        if jq empty <<< "${METADATA_JSON}" 2>/dev/null; then
            # Add the requested migration number to the metadata
            if [[ -n "${MIGRATION}" ]]; then
                METADATA=$(jq --arg reqmig "${MIGRATION}" '. + {requested_migration: $reqmig}' <<< "${METADATA_JSON}")
            else
                METADATA="${METADATA_JSON}"
            fi
        fi
    fi

    # Clean up temporary file
    rm -f "${METADATA_TMP}"
fi

# Clean up temporary JSON files
rm -f "${TEMP_BEFORE_JSON}" "${TEMP_AFTER_JSON}"

# Output SVG to stdout (main purpose)
echo "${SVG_OUTPUT}"

# Output metadata to stderr in a format the calling script can easily parse
if [[ -n "${METADATA}" ]]; then
    echo "DIAGRAM_METADATA: ${METADATA}" >&2
fi

echo "Diagram generation complete." >&2
