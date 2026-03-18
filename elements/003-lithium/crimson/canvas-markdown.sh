#!/bin/bash
#
# canvas-markdown.sh
#
# Converts Canvas LMS .imscc export files to markdown format.
# The .imscc file is technically a zip file containing HTML content.
#
# Usage:
#   ./canvas-markdown.sh <input.imscc> <output-prefix>
#
# Example:
#   ./canvas-markdown.sh /path/to/course.imscc canvas
#   # Produces: canvas-module-1.md, canvas-module-2.md, etc.
#

set -e

# Check arguments
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <input.imscc> <output-prefix>"
    echo "Example: $0 /path/to/course.imscc canvas"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_PREFIX="$2"

# Check if input file exists
if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "Error: Input file not found: ${INPUT_FILE}"
    exit 1
fi

# Check if input file is a valid zip/imscc
if ! unzip -t "${INPUT_FILE}" > /dev/null 2>&1; then
    echo "Error: Input file is not a valid zip/imscc file: ${INPUT_FILE}"
    exit 1
fi

# Check for required tools
if ! command -v pandoc > /dev/null 2>&1; then
    echo "Error: pandoc is required but not installed."
    echo "Install with: apt-get install pandoc  (or equivalent for your system)"
    exit 1
fi

# Create temporary directory for extraction
TEMP_DIR=$(mktemp -d)
# shellcheck disable=SC2064
trap "rm -rf '${TEMP_DIR}'" EXIT

# Remove old files with the same prefix
# shellcheck disable=SC2312
printf '%s\n' "Removing old files with prefix: ${OUTPUT_PREFIX}-"
rm -f "${OUTPUT_PREFIX}"-*.md

printf '%s\n' "Extracting ${INPUT_FILE}..."
unzip -q "${INPUT_FILE}" -d "${TEMP_DIR}"

# Find all HTML files and convert them
printf '%s\n' "Converting HTML files to markdown..."

# Find all HTML files (case insensitive)
HTML_FILES=$(find "${TEMP_DIR}" -type f \( -iname "*.html" -o -iname "*.htm" \))
# shellcheck disable=SC2312
printf '%s\n' "${HTML_FILES}" | while IFS= read -r html_file; do
    # Get the basename without extension
    basename=$(basename "${html_file}")
    name_no_ext="${basename%.*}"
    
    # Sanitize filename: replace spaces, underscores, periods with hyphens
    # Convert to lowercase for consistency
    sanitized=$(printf '%s' "${name_no_ext}" | tr '[:upper:]' '[:lower:]' | tr ' _.' '-')
    
    # Remove multiple consecutive hyphens using bash parameter expansion
    while [[ "${sanitized}" == *--* ]]; do
        sanitized="${sanitized//--/-}"
    done
    
    # Remove leading/trailing hyphens using bash parameter expansion
    sanitized="${sanitized#-}"
    sanitized="${sanitized%-}"
    
    # Construct output filename
    output_file="${OUTPUT_PREFIX}-${sanitized}.md"
    
    # Convert HTML to markdown using pandoc
    # shellcheck disable=SC2312
    if pandoc -f html -t markdown --wrap=none "${html_file}" -o "${output_file}" 2>/dev/null; then
        printf '  Created: %s\n' "${output_file}"
    else
        printf '  Warning: Failed to convert %s\n' "${basename}"
    fi
done

printf '\n'
printf '%s\n' "Conversion complete. Files created with prefix: ${OUTPUT_PREFIX}-"
