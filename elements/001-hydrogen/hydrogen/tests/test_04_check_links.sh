#!/bin/bash
# Test: Markdown Links Check
# Runs github-sitemap.sh to check markdown links and evaluates results with subtests

# FUNCTIONS

# CHANGELOG
# 3.4.0 - 2025-12-09 - Added subtest to check for relative links in markdown files under HYDROGEN_ROOT and HYDROGEN_DOCS_ROOT
# 3.3.0 - 2025-09-25 - Added total links count extraction from github-sitemap table and display in test title (e.g., "links: 1,163")
# 3.2.0 - 2025-08-08 - Optimized parsing using single-pass batch processing (md5sum batching pattern)
#                    - Applied md5sum optimization principles: minimize external process forks
#                    - Replaced 6+ file reads and grep|sed|awk pipelines with single awk command
#                    - Parsing performance improved from ~1.26s to ~0.04s (97% improvement)
#                    - Overall Test 06 runtime improved from 3.852s to ~2.245s (42% improvement)
#                    - Fixed typo
# 3.1.0 - 2025-08-06 - Bit of temp file handling management and cleanup
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.0.4 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.3 - 2025-07-07 - Fixed table detection to distinguish between column headers and dedicated issue tables
# 2.0.2 - 2025-07-07 - Fixed extraction logic for missing links and orphaned files counts with ANSI color code handling
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 1.0.0 - 2025-06-20 - Initial version with subtests for markdown link checking

set -euo pipefail

# Test configuration
TEST_NAME="Markdown Links {BLUE}(github-sitemap){RESET}"
TEST_ABBR="LNK"
TEST_NUMBER="04"
TEST_COUNTER=0
TEST_VERSION="3.4.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/file_utils.sh # Reference file utilities directly
[[ -n "${FILE_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/file_utils.sh"

# Test configuration
TARGET_README="README.md"

# Set paths for webroot and include directories
PHILEMENT_ROOT="$(realpath ../../../)"
export PHILEMENT_ROOT
HYDROGEN_ROOT="$(pwd)"
export HYDROGEN_ROOT
HYDROGEN_DOCS_ROOT="$(realpath ../../../docs/H)"
export HYDROGEN_DOCS_ROOT

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Execute Markdown Link Check"

# Create temporary file to capture output
MARKDOWN_CHECK="${LOG_PREFIX}_markdown_links_check.ansi"

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running markdown link check on ${TARGET_README}..."
# shellcheck disable=SC2153,SC2154 # SITEMAP_EXTERNAL defined in framework.sh
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" ".${SITEMAP} ${TARGET_README} --webroot '${PHILEMENT_ROOT}' --include '${HYDROGEN_ROOT}' --include '${HYDROGEN_DOCS_ROOT}' --noreport --quiet"

# Run github-sitemap.sh with webroot and include options
SITEMAP_EXIT_CODE=0
if ! "${SITEMAP}" "${TARGET_README}" --webroot "${PHILEMENT_ROOT}" --include "${HYDROGEN_ROOT}" --include "${HYDROGEN_DOCS_ROOT}" --noreport --quiet > "${MARKDOWN_CHECK}" 2>&1; then
    SITEMAP_EXIT_CODE=$?
else
    SITEMAP_EXIT_CODE=0
fi

# Display the output
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Results saved to ${MARKDOWN_CHECK}"

if [[ "${SITEMAP_EXIT_CODE}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Markdown link check executed successfully with no issues"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Markdown link check found issues (exit code: ${SITEMAP_EXIT_CODE})"
    EXIT_CODE=1
fi

# Batch parse all counts in single pass (md5sum batching pattern optimization)
parse_sitemap_output() {
    local file="$1"
    local -n issues_ref="$2"
    local -n missing_ref="$3"
    local -n orphaned_ref="$4"
    local -n total_ref="$5"

    # Initialize defaults
    issues_ref="0"
    missing_ref="0"
    orphaned_ref="0"
    total_ref="0"

    # Single pass parsing with awk (following md5sum batching pattern)
    local results
    results=$(awk '
    BEGIN {
        issues_found = "0"
        missing_count = "0"
        orphaned_count = "0"
        total_links = "0"
        in_missing_table = 0
        in_orphaned_table = 0
        in_reviewed_table = 0
    }

    # Clean ANSI codes and table delimiters once per line
    {
        # Remove ANSI escape sequences
        gsub(/\x1B\[[0-9;]*[JKmsu]/, "")
        # Remove table delimiters and trim
        gsub(/│/, "")
        gsub(/^[[:space:]]+|[[:space:]]+$/, "")
    }

    # Extract issues found count
    /Issues found:/ {
        if (match($0, /Issues found: *([0-9]+)/, arr)) {
            issues_found = arr[1]
        }
    }

    # Detect Reviewed Files Summary table
    $0 == "Reviewed Files Summary" {
        in_reviewed_table = 1
        reviewed_table_start = NR
        next
    }

    # Detect Missing Links table
    $0 == "Missing Links" {
        in_missing_table = 1
        missing_table_start = NR
        next
    }

    # Detect Orphaned Markdown Files table
    $0 == "Orphaned Markdown Files" {
        in_orphaned_table = 1
        orphaned_table_start = NR
        next
    }

    # Extract counts from table footers (line before ╰)
    /╰/ {
        if (in_reviewed_table && prev_line) {
            # Parse totals line: fields separated by spaces, get second field (Total Links)
            split(prev_line, fields)
            if (length(fields) >= 2) {
                total_links = fields[2]
                # Remove commas from number
                gsub(/,/, "", total_links)
            }
            in_reviewed_table = 0
        }
        if (in_missing_table && prev_line && match(prev_line, /([0-9]+)/)) {
            missing_count = substr(prev_line, RSTART, RLENGTH)
            in_missing_table = 0
        }
        if (in_orphaned_table && prev_line && match(prev_line, /([0-9]+)/)) {
            orphaned_count = substr(prev_line, RSTART, RLENGTH)
            in_orphaned_table = 0
        }
    }

    # Store previous line for table parsing
    { prev_line = $0 }

    END {
        # Calculate total issues as sum of missing and orphaned
        issues_found = (missing_count + 0) + (orphaned_count + 0)
        print issues_found ":" missing_count ":" orphaned_count ":" total_links
    }
    ' "${file}")

    # Parse results from single awk call
    IFS=':' read -r issues_ref missing_ref orphaned_ref total_ref <<< "${results}"

    # Ensure numeric values
    [[ "${issues_ref}" =~ ^[0-9]+$ ]] || issues_ref="0"
    [[ "${missing_ref}" =~ ^[0-9]+$ ]] || missing_ref="0"
    [[ "${orphaned_ref}" =~ ^[0-9]+$ ]] || orphaned_ref="0"
    [[ "${total_ref}" =~ ^[0-9]+$ ]] || total_ref="0"
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Missing Links Count"

# Parse all values in single batch operation (md5sum pattern)
ISSUES_FOUND=""
MISSING_LINKS_COUNT=0
ORPHANED_FILES_COUNT=0
TOTAL_LINKS=0
parse_sitemap_output "${MARKDOWN_CHECK}" ISSUES_FOUND MISSING_LINKS_COUNT ORPHANED_FILES_COUNT TOTAL_LINKS

# Update test name to include total links count
if [[ "${TOTAL_LINKS}" -gt 0 ]]; then
    TEST_NAME="Markdown Links {BLUE}(github-sitemap links: $("${PRINTF}" "%'d" "${TOTAL_LINKS}" || true)){RESET}"
fi

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing links found: ${MISSING_LINKS_COUNT}"
if [[ "${MISSING_LINKS_COUNT}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No missing links found"
    # ((PASS_COUNT++))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${MISSING_LINKS_COUNT} missing links"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Orphaned Files Count"

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Orphaned files found: ${ORPHANED_FILES_COUNT}"

if [[ "${ORPHANED_FILES_COUNT}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No orphaned markdown files found"
    # ((PASS_COUNT++))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ORPHANED_FILES_COUNT} orphaned markdown files"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Relative Links"

# Find all .md files under HYDROGEN_ROOT and HYDROGEN_DOCS_ROOT
RELATIVE_LINK_FILES=()
while IFS= read -r -d '' file; do
    RELATIVE_LINK_FILES+=("${file}")
done < <(find "${HYDROGEN_ROOT}" "${HYDROGEN_DOCS_ROOT}" -name "*.md" -type f -print0 2>/dev/null || true)

RELATIVE_LINK_COUNT=0
RELATIVE_LINK_VIOLATIONS=()

for file in "${RELATIVE_LINK_FILES[@]}"; do
    # Convert to relative path for .lintignore checking
    rel_file="${file#"${HYDROGEN_ROOT}"/}"
    if [[ "${rel_file}" == "${file}" ]]; then
        rel_file="${file#"${HYDROGEN_DOCS_ROOT}"/}"
    fi

    # Skip files excluded by .lintignore
    # shellcheck disable=SC2310 # We want to continue even if the test fails    
    if should_exclude_file "${rel_file}"; then
        continue
    fi

    # Use grep with perl regex to find lines with [label](link) where link doesn't start with /, http, or #
    violations=$(grep -P -n '\[.*\]\((?!http|/|#)[^)]*\)' "${file}" || true)
    if [[ -n "${violations}" ]]; then
        RELATIVE_LINK_COUNT=$((RELATIVE_LINK_COUNT + $(echo "${violations}" | wc -l)))
        while IFS= read -r line; do
            RELATIVE_LINK_VIOLATIONS+=("${file}:${line}")
        done <<< "${violations}"
    fi
done

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Relative links found: ${RELATIVE_LINK_COUNT}"

if [[ "${RELATIVE_LINK_COUNT}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No relative links found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${RELATIVE_LINK_COUNT} relative links"
    for violation in "${RELATIVE_LINK_VIOLATIONS[@]}"; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Violation: ${violation}"
    done
    EXIT_CODE=1
fi

# Display summary information
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Link check summary:"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Total issues found: ${ISSUES_FOUND}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing links: ${MISSING_LINKS_COUNT}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Orphaned files: ${ORPHANED_FILES_COUNT}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Relative links: ${RELATIVE_LINK_COUNT}"

# Validate counts against exit code from github-sitemap.sh for debugging
# Ensure counts are numeric before performing arithmetic
if ! [[ "${MISSING_LINKS_COUNT}" =~ ^[0-9]+$ ]]; then
    MISSING_LINKS_COUNT=0
fi
if ! [[ "${ORPHANED_FILES_COUNT}" =~ ^[0-9]+$ ]]; then
    ORPHANED_FILES_COUNT=0
fi
TOTAL_EXTRACTED_ISSUES=$((MISSING_LINKS_COUNT + ORPHANED_FILES_COUNT))
if [[ "${TOTAL_EXTRACTED_ISSUES}" -eq "${SITEMAP_EXIT_CODE}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validation: Extracted counts match sitemap exit code (${TOTAL_EXTRACTED_ISSUES} issues)"
else
    print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Validation: Extracted counts (${TOTAL_EXTRACTED_ISSUES}) do not match sitemap exit code (${SITEMAP_EXIT_CODE})"
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
