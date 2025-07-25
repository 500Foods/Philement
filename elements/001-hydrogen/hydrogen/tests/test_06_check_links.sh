#!/bin/bash

# Test: Markdown Links Check
# Runs github-sitemap.sh to check markdown links and evaluates results with subtests

# CHANGELOG
# 2.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.0.4 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.3 - 2025-07-07 - Fixed table detection to distinguish between column headers and dedicated issue tables
# 2.0.2 - 2025-07-07 - Fixed extraction logic for missing links and orphaned files counts with ANSI color code handling
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 1.0.0 - 2025-06-20 - Initial version with subtests for markdown link checking

# Test configuration
TEST_NAME="Markdown Links Check {BLUE}(github-sitemap){RESET}"
SCRIPT_VERSION="2.0.4"

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# CMAKE_DIR="${PROJECT_DIR}/cmake"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
[[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Always use build/tests/results directory
BUILD_DIR="${SCRIPT_DIR}/../build"
RESULTS_DIR="${BUILD_DIR}/tests/results"
mkdir -p "${RESULTS_DIR}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Test configuration
SITEMAP_SCRIPT="${LIB_DIR}/github-sitemap.sh"
TARGET_README="README.md"

# Subtest 1: Validate sitemap script availability
next_subtest
print_subtest "Validate GitHub Sitemap Script"

if [ ! -f "${SITEMAP_SCRIPT}" ]; then
    print_result 1 "GitHub sitemap script not found: ${SITEMAP_SCRIPT}"
    EXIT_CODE=1
else
    if [ ! -f "${TARGET_README}" ]; then
        print_result 1 "Target README file not found: ${TARGET_README}"
        EXIT_CODE=1
    else
        print_result 0 "GitHub sitemap script and target README found"
        ((PASS_COUNT++))
    fi
fi

# Skip remaining tests if prerequisites failed
if [ ${EXIT_CODE} -ne 0 ]; then
    print_warning "Prerequisites failed - skipping markdown link check"
    
    # Export results for test_all.sh integration
    export_subtest_results "98_check_links" ${TOTAL_SUBTESTS} ${PASS_COUNT} > /dev/null
    
    # Print completion table
    print_test_completion "${TEST_NAME}"
    
    exit ${EXIT_CODE}
fi

# Subtest 2: Run markdown link check
next_subtest
print_subtest "Execute Markdown Link Check"

# Create temporary file to capture output
TEMP_OUTPUT=$(mktemp)
MARKDOWN_RESULT_LOG="${RESULTS_DIR}/markdown_links_check_${TIMESTAMP}.log.ansi"

print_message "Running markdown link check on ${TARGET_README}..."
print_command "bash ${SITEMAP_SCRIPT} ${TARGET_README} --noreport --quiet"

# Run github-sitemap.sh with --noreport and --quiet to minimize output
bash "${SITEMAP_SCRIPT}" "${TARGET_README}" --noreport --quiet > "${TEMP_OUTPUT}" 2>&1
SITEMAP_EXIT_CODE=$?

# Copy output to result log
cp "${TEMP_OUTPUT}" "${MARKDOWN_RESULT_LOG}"

# Display the output
print_message "Results saved to ${MARKDOWN_RESULT_LOG}"
# print_message "Sitemap check output:"
# while IFS= read -r line; do
#     print_output "${line}"
# done < "${TEMP_OUTPUT}"

if [ ${SITEMAP_EXIT_CODE} -eq 0 ]; then
    print_result 0 "Markdown link check executed successfully with no issues"
    ((PASS_COUNT++))
else
    print_result 1 "Markdown link check found issues (exit code: ${SITEMAP_EXIT_CODE})"
    EXIT_CODE=1
fi

# Subtest 3: Parse and validate missing links count
next_subtest
print_subtest "Validate Missing Links Count"

# Parse the output to extract counts from the tables
# Look for "Issues found:" line to get the total issue count
ISSUES_FOUND=$(grep "Issues found:" "${TEMP_OUTPUT}" | sed 's/Issues found: //' || echo "0")
if [ -z "${ISSUES_FOUND}" ]; then
    ISSUES_FOUND="0"
fi

# Extract missing links count using corner-detection algorithm
MISSING_LINKS_COUNT=0
# Look for a dedicated "Missing Links" table (not just a column header)
# Check if there's a line that contains only "Missing Links" after cleaning
MISSING_LINKS_TABLE_FOUND=false
while IFS= read -r line; do
    # Clean the line of ANSI codes and delimiters
    CLEANED_LINE=$(echo "${line}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//')
    # Check if this line contains only "Missing Links" (dedicated table title)
    if [[ "${CLEANED_LINE}" == "Missing Links" ]]; then
        MISSING_LINKS_TABLE_FOUND=true
        break
    fi
done < "${TEMP_OUTPUT}"

if [ "${MISSING_LINKS_TABLE_FOUND}" = true ]; then
    # Find the line number of the dedicated "Missing Links" table title
    MISSING_LINKS_LINE=$(grep -n "Missing Links" "${TEMP_OUTPUT}" | while IFS=: read -r line_num line_content; do
        CLEANED_LINE=$(echo "${line_content}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//')
        if [[ "${CLEANED_LINE}" == "Missing Links" ]]; then
            echo "${line_num}"
            break
        fi
    done)
    
    # Extract content from the dedicated Missing Links table only
    if [ -n "${MISSING_LINKS_LINE}" ]; then
        # Get lines starting from the Missing Links table title
        LINE_BEFORE_CLOSE=$(tail -n +"${MISSING_LINKS_LINE}" "${TEMP_OUTPUT}" | grep -B 1 "╰" | head -1)
        # Extract the number by removing ANSI color codes, Unicode delimiters, trimming whitespace, and using grep for simplicity
        MISSING_LINKS_COUNT=$(echo "${LINE_BEFORE_CLOSE}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//' | grep -o '[0-9]\+' | head -1 || echo "0")
        if [ -z "${MISSING_LINKS_COUNT}" ]; then
            MISSING_LINKS_COUNT=0
        fi
    fi
fi

print_message "Missing links found: ${MISSING_LINKS_COUNT}"

if [ "${MISSING_LINKS_COUNT}" -eq 0 ]; then
    print_result 0 "No missing links found"
    ((PASS_COUNT++))
else
    print_result 1 "Found ${MISSING_LINKS_COUNT} missing links"
    EXIT_CODE=1
fi

# Subtest 4: Parse and validate orphaned files count
next_subtest
print_subtest "Validate Orphaned Files Count"

# Try to extract orphaned files count using corner-detection algorithm
ORPHANED_FILES_COUNT=0
# Look for a dedicated "Orphaned Markdown Files" table (not just a column header)
# Check if there's a line that contains only "Orphaned Markdown Files" after cleaning
ORPHANED_FILES_TABLE_FOUND=false
while IFS= read -r line; do
    # Clean the line of ANSI codes and delimiters
    CLEANED_LINE=$(echo "${line}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//')
    # Check if this line contains only "Orphaned Markdown Files" (dedicated table title)
    if [[ "${CLEANED_LINE}" == "Orphaned Markdown Files" ]]; then
        ORPHANED_FILES_TABLE_FOUND=true
        break
    fi
done < "${TEMP_OUTPUT}"

if [ "${ORPHANED_FILES_TABLE_FOUND}" = true ]; then
    # Find the line number of the dedicated "Orphaned Markdown Files" table title
    ORPHANED_FILES_LINE=$(grep -n "Orphaned Markdown Files" "${TEMP_OUTPUT}" | while IFS=: read -r line_num line_content; do
        CLEANED_LINE=$(echo "${line_content}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//')
        if [[ "${CLEANED_LINE}" == "Orphaned Markdown Files" ]]; then
            echo "${line_num}"
            break
        fi
    done)
    
    # Extract content from the dedicated Orphaned Files table only
    if [ -n "${ORPHANED_FILES_LINE}" ]; then
        # Get lines starting from the Orphaned Files table title
        LINE_BEFORE_CLOSE=$(tail -n +"${ORPHANED_FILES_LINE}" "${TEMP_OUTPUT}" | grep -B 1 "╰" | head -1)
        # Extract the number by removing ANSI color codes, Unicode delimiters, trimming whitespace, and using grep for simplicity
        ORPHANED_FILES_COUNT=$(echo "${LINE_BEFORE_CLOSE}" | sed 's/\x1B\[[0-9;]*[JKmsu]//g' | sed 's/│//g' | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//' | grep -o '[0-9]\+' | head -1 || echo "0")
        if [ -z "${ORPHANED_FILES_COUNT}" ]; then
            ORPHANED_FILES_COUNT=0
        fi
    fi
fi

print_message "Orphaned files found: ${ORPHANED_FILES_COUNT}"

if [ "${ORPHANED_FILES_COUNT}" -eq 0 ]; then
    print_result 0 "No orphaned markdown files found"
    ((PASS_COUNT++))
else
    print_result 1 "Found ${ORPHANED_FILES_COUNT} orphaned markdown files"
    EXIT_CODE=1
fi

# Clean up temporary file
rm -f "${TEMP_OUTPUT}"

# Display summary information
print_message "Link check summary:"
print_output "Total issues found: ${ISSUES_FOUND}"
print_output "Missing links: ${MISSING_LINKS_COUNT}"
print_output "Orphaned files: ${ORPHANED_FILES_COUNT}"

# Validate counts against exit code from github-sitemap.sh for debugging
# Ensure counts are numeric before performing arithmetic
if ! [[ "${MISSING_LINKS_COUNT}" =~ ^[0-9]+$ ]]; then
    MISSING_LINKS_COUNT=0
fi
if ! [[ "${ORPHANED_FILES_COUNT}" =~ ^[0-9]+$ ]]; then
    ORPHANED_FILES_COUNT=0
fi
TOTAL_EXTRACTED_ISSUES=$((MISSING_LINKS_COUNT + ORPHANED_FILES_COUNT))
if [ "${TOTAL_EXTRACTED_ISSUES}" -eq "${SITEMAP_EXIT_CODE}" ]; then
    print_message "Validation: Extracted counts match sitemap exit code (${TOTAL_EXTRACTED_ISSUES} issues)"
else
    print_warning "Validation: Extracted counts (${TOTAL_EXTRACTED_ISSUES}) do not match sitemap exit code (${SITEMAP_EXIT_CODE})"
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return ${EXIT_CODE}
else
    exit ${EXIT_CODE}
fi
