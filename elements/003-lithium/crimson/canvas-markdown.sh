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
#   ./canvas-markdown.sh /path/to/PH-003-I2L.imscc canvas
#   # Produces: canvas-PH-003-I2L-introduction-to-lithium-about-your-instructor.md, etc.
#
# Filename format: <prefix>-<course>-<module>-<page>.md
# This allows reconstructing Canvas URLs for citations:
#   https://canvas.500courses.com/courses/<course_id>/pages/<page>
#

set -e

# Check arguments
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <input.imscc> <output-prefix>"
    echo "Example: $0 /path/to/PH-003-I2L.imscc canvas"
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

# Extract course code from filename (e.g., PH-003-I2L from PH-003-I2L-2026-03-31.imscc)
BASENAME=$(basename "${INPUT_FILE}" .imscc)
# Remove date suffix if present (e.g., -2026-03-31)
COURSE_CODE=$(printf '%s' "${BASENAME}" | sed -E 's/-[0-9]{4}-[0-9]{2}-[0-9]{2}$//')

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

# Build module mapping from module_meta.xml
# This creates a lookup: identifierref -> module position number
declare -A MODULE_MAP
declare -A IDENTIFIER_TO_FILE

MODULE_META="${TEMP_DIR}/course_settings/module_meta.xml"
IMS_MANIFEST="${TEMP_DIR}/imsmanifest.xml"

if [[ -f "${MODULE_META}" ]] && [[ -f "${IMS_MANIFEST}" ]]; then
    printf '%s\n' "Building module mappings..."
    
    # First pass: Build mapping of identifierref -> module position number
    # Parse module_meta.xml to extract module structure
    IN_MODULE=false
    CURRENT_MODULE_POS=""
    while IFS= read -r line; do
        # Look for module start
        if [[ "${line}" =~ '<module identifier=' ]]; then
            CURRENT_MODULE_POS=""
            IN_MODULE=true
            SEEN_MODULE_POSITION=false
        # Look for module end
        elif [[ "${line}" =~ '</module>' ]]; then
            IN_MODULE=false
            CURRENT_MODULE_POS=""
        # Only process lines inside a module, before <items> tag
        elif [[ "${IN_MODULE}" == true ]]; then
            # Once we hit <items>, we're in item definitions - stop capturing module position
            if [[ "${line}" =~ '<items>' ]]; then
                SEEN_MODULE_POSITION=true
            fi
            # Extract module position (only the first position tag before <items>)
            if [[ "${SEEN_MODULE_POSITION}" == false ]] && [[ -z "${CURRENT_MODULE_POS}" ]] && [[ "${line}" =~ '<position>' ]]; then
                MODULE_POS=$(printf '%s' "${line}" | sed -E 's/.*<position>([^<]+)<\/position>.*/\1/')
                if [[ -n "${MODULE_POS}" && "${MODULE_POS}" != "${line}" ]]; then
                    CURRENT_MODULE_POS="${MODULE_POS}"
                fi
            fi
            # Map identifierref to current module position
            if [[ "${line}" =~ '<identifierref>' ]]; then
                REF=$(printf '%s' "${line}" | sed -E 's/.*<identifierref>([^<]+)<\/identifierref>.*/\1/')
                if [[ -n "${CURRENT_MODULE_POS}" && -n "${REF}" && "${REF}" != "${line}" ]]; then
                    MODULE_MAP["${REF}"]="${CURRENT_MODULE_POS}"
                fi
            fi
        fi
    done < "${MODULE_META}"
    
    # Second pass: Build mapping of identifierref -> file href from imsmanifest.xml
    CURRENT_RESOURCE=""
    while IFS= read -r line; do
        if [[ "${line}" =~ '<resource' ]] && [[ "${line}" =~ identifier=\"([^\"]+)\" ]]; then
            CURRENT_RESOURCE="${BASH_REMATCH[1]}"
        elif [[ "${line}" =~ '</resource>' ]]; then
            CURRENT_RESOURCE=""
        elif [[ -n "${CURRENT_RESOURCE}" ]] && [[ "${line}" =~ href=\"([^\"]+\.html)\" ]]; then
            HREF="${BASH_REMATCH[1]}"
            # Extract just the filename from path like "wiki_content/file.html"
            FILENAME=$(basename "${HREF}")
            IDENTIFIER_TO_FILE["${CURRENT_RESOURCE}"]="${FILENAME}"
        fi
    done < "${IMS_MANIFEST}"
fi

# Find all HTML files and convert them
printf '%s\n' "Converting HTML files to markdown..."

# Find all HTML files (case insensitive)
HTML_FILES=$(find "${TEMP_DIR}" -type f \( -iname "*.html" -o -iname "*.htm" \))

# Process files
# shellcheck disable=SC2312
printf '%s\n' "${HTML_FILES}" | while IFS= read -r html_file; do
    # Get the basename
    basename=$(basename "${html_file}")
    name_no_ext="${basename%.*}"
    
    # Look up module number for this file
    MODULE_NUM=""
    for ref in "${!IDENTIFIER_TO_FILE[@]}"; do
        if [[ "${IDENTIFIER_TO_FILE[$ref]}" == "${basename}" ]]; then
            MODULE_NUM="${MODULE_MAP[$ref]:-}"
            break
        fi
    done
    
    # Sanitize filename components: replace spaces, underscores, periods with hyphens
    # Convert to lowercase for consistency
    sanitized_page=$(printf '%s' "${name_no_ext}" | tr '[:upper:]' '[:lower:]' | tr ' _.' '-')
    
    # Use module number (position) or '0' for uncategorized
    if [[ -n "${MODULE_NUM}" ]]; then
        module_part=$(printf 'mod-%s' "${MODULE_NUM}")
    else
        module_part="mod-0"
    fi
    
    # Remove multiple consecutive hyphens from page name
    while [[ "${sanitized_page}" == *--* ]]; do
        sanitized_page="${sanitized_page//--/-}"
    done
    
    # Remove leading/trailing hyphens from page name
    sanitized_page="${sanitized_page#-}"
    sanitized_page="${sanitized_page%-}"
    
    # Construct output filename: prefix-course-mod-#-page.md
    output_file="${OUTPUT_PREFIX}-${COURSE_CODE}-${module_part}-${sanitized_page}.md"
    
    # Add metadata header with URL construction info
    {
        printf -- '---\n'
        printf 'course_code: "%s"\n' "${COURSE_CODE}"
        printf 'module_num: "%s"\n' "${MODULE_NUM:-0}"
        printf 'page: "%s"\n' "${name_no_ext}"
        printf 'canvas_url: "courses/<COURSE_ID>/pages/%s"\n' "${name_no_ext}"
        printf -- '---\n\n'
        
        # Convert HTML to markdown using pandoc
        if ! pandoc -f html -t markdown --wrap=none "${html_file}" 2>/dev/null; then
            printf 'Warning: Failed to convert %s\n' "${basename}" >&2
        fi
    } > "${output_file}"
    
    if [[ -f "${output_file}" ]]; then
        printf '  Created: %s\n' "${output_file}"
    fi
done

printf '\n'
printf '%s\n' "Conversion complete. Files created with prefix: ${OUTPUT_PREFIX}-${COURSE_CODE}"
