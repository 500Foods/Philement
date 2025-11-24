#!/bin/bash

# migration_index.sh
# Generates a Migrations table section for README files based on migration folder contents
# Usage: ./migration_index.sh <readme_file> <migrations_folder>

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

# Function to extract value from cfg.VARIABLE = "value" or cfg.VARIABLE = 'value'
extract_cfg_value() {
    local file="$1"
    local var="$2"
    grep -m 1 "^cfg\\.${var} =" "${file}" | sed -E 's/^cfg\.[^ ]+ = ["'"'"'](.*)["'"'"']/\1/' || echo ""
}

# Function to get latest version from CHANGELOG
get_latest_version() {
    local file="$1"
    grep -m 1 "^-- [0-9]" "${file}" | sed -E 's/^-- ([0-9]+\.[0-9]+\.[0-9]+).*/\1/' || echo ""
}

# Function to get latest release date from CHANGELOG
get_latest_release() {
    local file="$1"
    grep -m 1 "^-- [0-9]" "${file}" | sed -E 's/^-- [0-9]+\.[0-9]+\.[0-9]+ - ([0-9]{4}-[0-9]{2}-[0-9]{2}).*/\1/' || echo ""
}

# Function to count migrations (table.insert + QUERY_DELIMITER + SUBQUERY_DELIMITER)
count_migrations() {
    local file="$1"
    local insert_count
    local query_delim
    local subquery_delim
    
    insert_count=$(grep -c "table\.insert(queries" "${file}" || echo "0")
    query_delim=$(grep -c "QUERY_DELIMITER" "${file}" || echo "0")
    subquery_delim=$(grep -c "SUBQUERY_DELIMITER" "${file}" || echo "0")
    echo $((insert_count + query_delim + subquery_delim))
}

# Function to check if file has diagram migration
has_diagram() {
    local file="$1"
    if grep -q "TYPE_DIAGRAM_MIGRATION" "${file}"; then
        echo "✓"
    else
        echo ""
    fi
}

# Generate Migrations table
generate_migrations_table() {
    echo "## Migrations"
    echo ""
    echo "| M# | Table | Version | Released | Migrations | Diagram | Description |"
    echo "|----|-------|---------|----------|------------|---------|-------------|"
    
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
        description=$(sed -n '2p' "${file}" | sed 's/^-- //')
        mig_count=$(count_migrations "${file}")
        has_diag=$(has_diagram "${file}")
        
        # Update totals
        total_migrations=$((total_migrations + mig_count))
        file_count=$((file_count + 1))
        if [[ -n "${has_diag}" ]]; then
            total_diagrams=$((total_diagrams + 1))
        fi
        
        # Make migration number a clickable link
        echo "| [${migration_num}](${MIGRATIONS_FOLDER}/${basename}) | ${table_name} | ${version} | ${released} | ${mig_count} | ${has_diag} | ${description} |"
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