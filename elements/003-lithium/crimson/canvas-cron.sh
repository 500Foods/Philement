#!/bin/bash
#
# canvas-cron.sh
#
# Cron job wrapper for canvas-markdown.sh
# Finds the latest PH-003*.imscc file and converts it to markdown
#
# Usage: ./canvas-cron.sh
#

set -e

# Configuration
BACKUP_DIR="/fvl/tnt/t-500courses/backups/courses"
OUTPUT_PREFIX="knowledge/canvas"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cp /mnt/extra/Projects/Philement/docs/Li/*.md knowledge

# Find the latest PH-003*.imscc file
# shellcheck disable=SC2312
LATEST_FILE=$(find "${BACKUP_DIR}" -type f -name 'PH-003*.imscc' -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2-)

if [[ -z "${LATEST_FILE}" ]]; then
    printf 'Error: No PH-003*.imscc files found in %s\n' "${BACKUP_DIR}" >&2
    exit 1
fi

# Check if the file exists and is readable
if [[ ! -r "${LATEST_FILE}" ]]; then
    printf 'Error: Cannot read file: %s\n' "${LATEST_FILE}" >&2
    exit 1
fi

printf 'Processing latest file: %s\n' "${LATEST_FILE}"

# Change to script directory and run the conversion
cd "${SCRIPT_DIR}"
"${SCRIPT_DIR}/canvas-markdown.sh" "${LATEST_FILE}" "${OUTPUT_PREFIX}"
