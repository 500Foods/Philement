#!/bin/bash
# add_coverage.sh - Identify lines with missing coverage in both gcov files.
#
# Usage: ./add_coverage.sh <relative_source_path>
#
# Example: ./add_coverage.sh api/conduit/alt_queries/alt_queries.c
#
# This script parses two .gcov files (unit tests and coverage tests) for the
# same source file, finds lines that are uncovered (##### or -----) in *both*
# files, and prints those line numbers along with the corresponding source
# code snippet.

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <relative_source_path>" >&2
    echo "  Example: $0 api/conduit/alt_queries/alt_queries.c" >&2
    exit 1
fi

# Remove any .c extension if present
src_path="${1%.c}"

# Construct the two gcov paths
a="build/unity/src/${src_path}.c.gcov"
b="build/coverage/src/${src_path}.c.gcov"

[[ -f "${a}" ]] || { echo "Error: File '${a}' not found." >&2; exit 1; }
[[ -f "${b}" ]] || { echo "Error: File '${b}' not found." >&2; exit 1; }

# Function to extract uncovered lines: line_num:source
extract_uncovered() {
    local file="$1"
    local output="$2"
    while IFS= read -r line || [[ -n "${line}" ]]; do
        if [[ ${line} =~ ^[[:space:]]*(#####|-----):[[:space:]]*([0-9]+):(.*)$ ]]; then
            local ln="${BASH_REMATCH[2]}"
            local source="${BASH_REMATCH[3]}"
            echo "${ln}:${source}"
        fi
    done < "${file}" > "${output}"
}

extract_uncovered "${a}" uncovered_a.txt
extract_uncovered "${b}" uncovered_b.txt

# Find common line numbers
sort uncovered_a.txt > sorted_a.txt
sort uncovered_b.txt > sorted_b.txt
common_lines=$(comm -12 sorted_a.txt sorted_b.txt | cut -d: -f1 | sort -n)

if [[ -z "${common_lines}" ]]; then
    echo "No lines uncovered in both files—all lines are covered by at least one test suite."
else
    echo "Lines not covered in both files:"
    echo "--------------------------------------------------"
    echo "${common_lines}" | while IFS= read -r ln; do
        source=$(grep "^${ln}:" uncovered_a.txt | cut -d: -f2-)
        printf "%5d: %s\n" "${ln}" "${source}"
    done
fi

# Cleanup
rm -f uncovered_*.txt sorted_*.txt