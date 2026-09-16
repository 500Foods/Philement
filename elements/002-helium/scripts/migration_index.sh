#!/bin/bash

# migration_index.sh
# Generates a Migrations table section for README files based on migration folder contents
# Usage: ./migration_index.sh <readme_file> <migrations_folder>

# CHANGEHISTORY
# 1.1.0 - 2026-09-16 - Version/date only from `-- N.N.N - YYYY-MM-DD` CHANGELOG
#              lines (ignore wrap lines that start with a digit). Description
#              is the single line after `-- Migration:`. Absolute Helium links.
# 1.0.0 - 2025-11-24 - Initial version

set -euo pipefail

# Check arguments
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <readme_file> <migrations_folder>"
    echo "Example: $0 README.md migrations"
    exit 1
fi

README_FILE="$1"
MIGRATIONS_FOLDER="$2"

if [[ ! -f "${README_FILE}" ]]; then
    echo "Error: README file not found: ${README_FILE}"
    exit 1
fi

if [[ ! -d "${MIGRATIONS_FOLDER}" ]]; then
    echo "Error: Migrations folder not found: ${MIGRATIONS_FOLDER}"
    exit 1
fi

# Repo-rooted links so markdownlint (absolute /elements/... paths) stays happy
# whether README_FILE is absolute or relative.
README_REAL="$(realpath "${README_FILE}")"
README_PARENT="$(dirname "${README_REAL}")"
SCHEMA_DIR="$(basename "${README_PARENT}")"
MIGRATIONS_ABS="/elements/002-helium/${SCHEMA_DIR}/migrations"

# Function to extract value from cfg.VARIABLE = "value" or cfg.VARIABLE = 'value'
extract_cfg_value() {
    local file="$1"
    local var="$2"
    grep -m 1 "^cfg\\.${var} =" "${file}" | sed -E 's/^cfg\.[^ ]+ = ["'"'"'](.*)["'"'"']/\1/' || echo ""
}

# CHANGELOG version lines are `-- N.N.N - YYYY-MM-DD - ...` (newest first by
# convention). Do not match wrap lines such as `-- 34 (Course Manager)` or
# `-- 1 is Auditor` — those start with a digit after `-- ` but are not semver.
CHANGELOG_VER_RE='^-- [0-9]+\.[0-9]+\.[0-9]+ - [0-9]{4}-[0-9]{2}-[0-9]{2}'

# Highest N.N.N among CHANGELOG version lines (works if oldest is listed first).
get_latest_version() {
    local file="$1"
    local ver=""
    ver="$(grep -E "${CHANGELOG_VER_RE}" "${file}" 2>/dev/null \
        | sed -E 's/^-- ([0-9]+\.[0-9]+\.[0-9]+).*/\1/' \
        | sort -t. -k1,1n -k2,2n -k3,3n \
        | tail -n 1 || true)"
    echo "${ver}"
}

# Date from the CHANGELOG line for that highest version (first match).
get_latest_release() {
    local file="$1"
    local ver
    ver="$(get_latest_version "${file}")"
    if [[ -z "${ver}" ]]; then
        echo ""
        return
    fi
    local ver_re="${ver//./\\.}"
    grep -E "^-- ${ver_re} - [0-9]{4}-[0-9]{2}-[0-9]{2}" "${file}" 2>/dev/null \
        | head -n 1 \
        | sed -E 's/^-- [0-9]+\.[0-9]+\.[0-9]+ - ([0-9]{4}-[0-9]{2}-[0-9]{2}).*/\1/' || true
}

# One-line README description: the comment immediately under `-- Migration:`.
# Never join wrapped CHANGELOG or body comments into the table cell.
get_description() {
    local file="$1"
    local line
    line="$(sed -n '2p' "${file}")"
    line="${line#"${line%%[![:space:]]*}"}"
    if [[ "${line}" == --* ]]; then
        line="${line#--}"
        line="${line#"${line%%[![:space:]]*}"}"
    fi
    printf '%s' "${line}" | tr '\n\r' '  ' | sed 's/[[:space:]]\+/ /g; s/[[:space:]]*$//'
}

# Function to count migrations (table.insert + QUERY_DELIMITER + SUBQUERY_DELIMITER)
count_migrations() {
    local file="$1"
    local insert_count
    local query_delim
    
    insert_count=$(grep -c "table\.insert(queries" "${file}" || echo "0")
    query_delim=$(grep -c "QUERY_DELIMITER" "${file}" || true)
    echo $((insert_count + query_delim))
}

# Function to check if file has diagram migration
has_diagram() {
    local file="$1"
    if grep -q "TYPE_DIAGRAM_MIGRATION" "${file}"; then
        echo "✓"
    else
        echo "✗"
    fi
}

# Generate Migrations table
generate_migrations_table() {
    echo "## Migrations"
    echo ""
    echo "| M# | Table | Version | Updated | Stmts | Diagram | Description |"
    echo "| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |"
    
    local total_migrations=0
    local total_diagrams=0
    local file_count=0
    
    # Process all migration files in order using find and sort
    
    while IFS= read -r -d '' file; do
        local basename
        local migration_num
        local table_name
        local version
        local released
        local description
        local mig_count
        local has_diag
        
        basename=$(basename "${file}")
        migration_num=$(echo "${basename}" | sed -E 's/.*_([0-9]+)\.lua/\1/')
        table_name=$(extract_cfg_value "${file}" "TABLE")
        version=$(get_latest_version "${file}")
        released=$(get_latest_release "${file}")
        description=$(get_description "${file}")
        mig_count=$(count_migrations "${file}")
        has_diag=$(has_diagram "${file}")
        
        # Update totals
        total_migrations=$((total_migrations + mig_count))
        file_count=$((file_count + 1))
        if [[ -n "${has_diag}" ]]; then
            total_diagrams=$((total_diagrams + 1))
        fi
        
        # Make migration number a clickable link
        echo "| [${migration_num}](${MIGRATIONS_ABS}/${basename}) | ${table_name} | ${version} | ${released} | ${mig_count} | ${has_diag} | ${description} |"
    done < <(find "${MIGRATIONS_FOLDER}" -maxdepth 1 -name '[a-z]*_[0-9]*.lua' -print0 | sort -zV || true)
    
    # Add totals row
    echo "| **${file_count}** | | | | **${total_migrations}** | **${total_diagrams}** | |"
}

# Create temporary file with new content
TEMP_FILE=$(mktemp)

# Check if README has a Migrations section and extract content before it
if grep -q "^## Migrations" "${README_FILE}"; then
    sed '/^## Migrations/,$d' "${README_FILE}" > "${TEMP_FILE}"
else
    cat "${README_FILE}" > "${TEMP_FILE}"
fi

# Remove trailing blank lines by reading file into array and finding last non-empty line
mapfile -t lines < "${TEMP_FILE}"
last_content_line=-1
for ((i=${#lines[@]}-1; i>=0; i--)); do
    if [[ -n "${lines[i]}" && "${lines[i]}" != *[[:space:]]* ]] || [[ "${lines[i]}" =~ [^[:space:]] ]]; then
        last_content_line=${i}
        break
    fi
done

# Write content up to last non-empty line
true > "${TEMP_FILE}"
for ((i=0; i<=last_content_line; i++)); do
    echo "${lines[i]}" >> "${TEMP_FILE}"
done

# Add exactly one blank line then the migrations table
echo "" >> "${TEMP_FILE}"
generate_migrations_table >> "${TEMP_FILE}"

# Replace the original file
mv "${TEMP_FILE}" "${README_FILE}"

echo "Migrations table has been updated in ${README_FILE}"