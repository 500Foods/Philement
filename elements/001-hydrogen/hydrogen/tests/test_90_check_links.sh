#!/bin/bash
#
# Test: Markdown Links Check
# Runs github-sitemap.sh to check markdown links and evaluates results with subtests
#
# VERSION HISTORY
# 2.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 1.0.0 - 2025-06-20 - Initial version with subtests for markdown link checking
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the new modular test libraries with guard clauses
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
# shellcheck source=tests/lib/tables.sh
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
# shellcheck source=tests/lib/log_output.sh
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh
source "$SCRIPT_DIR/lib/framework.sh"

# Test configuration
TEST_NAME="Markdown Links Check"
SCRIPT_VERSION="2.0.0"
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Test configuration
SITEMAP_SCRIPT="tests/lib/github-sitemap.sh"
TARGET_README="README.md"

# These are expected to exist, but are missing mid-test_00_all.sh
# We're just ensuring that they exist so sitemap doesn't complain
mkdir -p tests/results
touch tests/results/repository_info.md
touch tests/results/latest_test_results.md

# Subtest 1: Validate sitemap script availability
next_subtest
print_subtest "Validate GitHub Sitemap Script"

if [ ! -f "$SITEMAP_SCRIPT" ]; then
    print_result 1 "GitHub sitemap script not found: $SITEMAP_SCRIPT"
    EXIT_CODE=1
else
    if [ ! -f "$TARGET_README" ]; then
        print_result 1 "Target README file not found: $TARGET_README"
        EXIT_CODE=1
    else
        print_result 0 "GitHub sitemap script and target README found"
        ((PASS_COUNT++))
    fi
fi

# Skip remaining tests if prerequisites failed
if [ $EXIT_CODE -ne 0 ]; then
    print_warning "Prerequisites failed - skipping markdown link check"
    
    # Export results for test_all.sh integration
    export_subtest_results "98_check_links" $TOTAL_SUBTESTS $PASS_COUNT > /dev/null
    
    # Print completion table
    print_test_completion "$TEST_NAME"
    
    exit $EXIT_CODE
fi

# Subtest 2: Run markdown link check
next_subtest
print_subtest "Execute Markdown Link Check"

# Create temporary file to capture output
TEMP_OUTPUT=$(mktemp)
MARKDOWN_RESULT_LOG="$RESULTS_DIR/markdown_links_check_${TIMESTAMP}.log"

print_message "Running markdown link check on $TARGET_README..."
print_command "bash $SITEMAP_SCRIPT $TARGET_README --noreport --quiet"

# Run github-sitemap.sh with --noreport and --quiet to minimize output
bash "$SITEMAP_SCRIPT" "$TARGET_README" --noreport --quiet > "$TEMP_OUTPUT" 2>&1
SITEMAP_EXIT_CODE=$?

# Copy output to result log
cp "$TEMP_OUTPUT" "$MARKDOWN_RESULT_LOG"

# Display the output
print_message "Sitemap check output:"
while IFS= read -r line; do
    print_output "$line"
done < "$TEMP_OUTPUT"

if [ $SITEMAP_EXIT_CODE -eq 0 ]; then
    print_result 0 "Markdown link check executed successfully with no issues"
    ((PASS_COUNT++))
else
    print_result 1 "Markdown link check found issues (exit code: $SITEMAP_EXIT_CODE)"
    EXIT_CODE=1
fi

# Subtest 3: Parse and validate missing links count
next_subtest
print_subtest "Validate Missing Links Count"

# Parse the output to extract counts from the tables
# Look for "Issues found:" line to get the total issue count
ISSUES_FOUND=$(grep "Issues found:" "$TEMP_OUTPUT" | sed 's/Issues found: //' || echo "0")
if [ -z "$ISSUES_FOUND" ]; then
    ISSUES_FOUND="0"
fi

# Extract missing links count from the summary row at the bottom of the table
# Look for the summary row that shows totals
MISSING_LINKS_COUNT=0
if grep -q "│.*│.*│.*│.*│" "$TEMP_OUTPUT"; then
    # Get the last data row (summary row) and extract the missing links column (5th column)
    SUMMARY_ROW=$(grep "│.*│.*│.*│.*│" "$TEMP_OUTPUT" | tail -1)
    if [ -n "$SUMMARY_ROW" ]; then
        # Extract the 5th column (Missing Links) from the summary row
        MISSING_LINKS_COUNT=$(echo "$SUMMARY_ROW" | awk -F'│' '{gsub(/[[:space:]]/, "", $6); print $6}' | grep -o '[0-9]*' || echo "0")
        if [ -z "$MISSING_LINKS_COUNT" ]; then
            MISSING_LINKS_COUNT=0
        fi
    fi
fi

print_message "Missing links found: $MISSING_LINKS_COUNT"

if [ "$MISSING_LINKS_COUNT" -eq 0 ]; then
    print_result 0 "No missing links found"
    ((PASS_COUNT++))
else
    print_result 1 "Found $MISSING_LINKS_COUNT missing links"
    EXIT_CODE=1
fi

# Subtest 4: Parse and validate orphaned files count
next_subtest
print_subtest "Validate Orphaned Files Count"

# Try to extract orphaned files count from table data
ORPHANED_FILES_COUNT=0
if grep -q "Orphaned Markdown Files" "$TEMP_OUTPUT"; then
    # Count the number of data rows in the orphaned files table (excluding headers and borders)
    ORPHANED_FILES_COUNT=$(grep -A 100 "Orphaned Markdown Files" "$TEMP_OUTPUT" | grep -c "│.*│" | head -1 || echo "0")
    # Subtract 1 for the header row if present
    if [ "$ORPHANED_FILES_COUNT" -gt 0 ]; then
        ORPHANED_FILES_COUNT=$((ORPHANED_FILES_COUNT - 1))
    fi
    # Ensure non-negative
    if [ "$ORPHANED_FILES_COUNT" -lt 0 ]; then
        ORPHANED_FILES_COUNT=0
    fi
fi

print_message "Orphaned files found: $ORPHANED_FILES_COUNT"

if [ "$ORPHANED_FILES_COUNT" -eq 0 ]; then
    print_result 0 "No orphaned markdown files found"
    ((PASS_COUNT++))
else
    print_result 1 "Found $ORPHANED_FILES_COUNT orphaned markdown files"
    EXIT_CODE=1
fi

# Clean up temporary file
rm -f "$TEMP_OUTPUT"

# Display summary information
print_message "Link check summary:"
print_output "Total issues found: $ISSUES_FOUND"
print_output "Missing links: $MISSING_LINKS_COUNT"
print_output "Orphaned files: $ORPHANED_FILES_COUNT"
print_output "Detailed report saved to: $MARKDOWN_RESULT_LOG"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
