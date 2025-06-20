#!/bin/bash
#
# Hydrogen Markdown Links Check Test
# Runs support_sitemap.sh to check markdown links and evaluates results with subtests.
#
# Usage: ./test_98_check_links.sh
#
# This test checks for missing links and orphaned markdown files in the documentation.
# Subtests:
# - Overall execution (pass if support_sitemap.sh returns 0)
# - Number of markdown files (always pass, informational)
# - Number of missing links (pass if zero)
# - Number of orphaned files (pass if zero)
#

# Script metadata
NAME_TEST="Markdown Links Check Test"
VERS_TEST="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-06-20 - Initial version with subtests for markdown link checking

# Display script name and version
echo "=== $NAME_TEST v$VERS_TEST ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# These are expected to exist, but are missing mid-test_00_all.sh
# We're just ensuring that they exist so sitemap doesn't complain
mkdir -p results
touch results/repository_info.md
touch results/latest_test_results.md

# ====================================================================
# STEP 1: Set up the test environment
# ====================================================================

# Set up test environment with descriptive name
TEST_NAME="Markdown Links Check"
RESULT_LOG="results/markdown_links_check_$(date +%Y%m%d_%H%M%S).log"
mkdir -p "results"
touch "$RESULT_LOG"

# ====================================================================
# STEP 2: Initialize subtest tracking
# ====================================================================

# Define total number of subtests
TOTAL_SUBTESTS=4  # Overall, Markdown Files, Missing Links, Orphaned Files
PASS_COUNT=0
missing_links=0
orphaned_files=0

# ====================================================================
# STEP 3: Run support_sitemap.sh and evaluate results
# ====================================================================

run_sitemap_check() {
    print_header "Running Markdown Link Check" | tee -a "$RESULT_LOG"
    
    # Create a temporary file to capture output
    local temp_output
    temp_output=$(mktemp)
    
    # Run support_sitemap.sh with --noreport and --quiet to minimize output
    bash "$SCRIPT_DIR/support_sitemap.sh" "../README.md" --noreport --quiet > "$temp_output" 2>&1
    local exit_code=$?
    
    # Initialize counts
    local md_files=0
    
    # Parse output for counts (if needed, though with --quiet we rely on exit code primarily)
    # From support_sitemap.sh code, exit code is total issues (missing + orphaned)
    if [ $exit_code -eq 0 ]; then
        print_result 0 "support_sitemap.sh executed successfully with no issues" | tee -a "$RESULT_LOG"
        ((PASS_COUNT++))
    else
        print_result 1 "support_sitemap.sh found issues (exit code: $exit_code)" | tee -a "$RESULT_LOG"
    fi
    
    # Parse the output to extract counts from the tables using the corner characters as markers
    # Main table for total markdown files (always present)
    md_files_line=$(grep -B 1 "╰" "$temp_output" | grep "│" | head -n 1 | cut -c23-30 | tr -d ' ')
    if [ -n "$md_files_line" ]; then
        md_files=$md_files_line
    fi
    if [ -z "$md_files" ]; then
        md_files="unknown"
    fi
    
    # Check for presence of Missing Links and Orphaned Files tables using headers
    missing_links_present=$(grep -A 2 "╭" "$temp_output" | grep -c "Missing")
    orphaned_files_present=$(grep -A 2 "╭" "$temp_output" | grep -c "Orphaned")
    
    # Initialize counts
    missing_links=0
    orphaned_files=0
    
    # Extract counts from tables based on their order after the main table
    # Get all lines before bottom corners that contain numbers, excluding the markdown files count
    count_lines=$(grep -B 1 "╰" "$temp_output" | grep "│" | grep -v "$md_files" | cut -c23-30 | tr -d ' ')
    if [ -n "$count_lines" ]; then
        IFS=$'\n' read -d '' -r -a counts <<< "$count_lines"
        if [ "$missing_links_present" -gt 0 ] && [ ${#counts[@]} -gt 0 ]; then
            missing_links=${counts[0]}
        fi
        if [ "$orphaned_files_present" -gt 0 ] && [ ${#counts[@]} -gt 1 ]; then
            orphaned_files=${counts[1]}
        elif [ "$orphaned_files_present" -gt 0 ] && [ ${#counts[@]} -eq 1 ]; then
            orphaned_files=${counts[0]}
        fi
    fi
    
    # Subtest 1: Markdown Files Count (always pass)
    print_header "Subtest 1: Markdown Files Count" | tee -a "$RESULT_LOG"
    print_result 0 "Found $md_files markdown files" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
    
    # Subtest 2: Missing Links Check
    print_header "Subtest 2: Missing Links Check" | tee -a "$RESULT_LOG"
    if [ "$missing_links" -eq 0 ]; then
        print_result 0 "No missing links found" | tee -a "$RESULT_LOG"
        ((PASS_COUNT++))
    else
        print_result 1 "Found $missing_links missing links" | tee -a "$RESULT_LOG"
    fi
    
    # Subtest 3: Orphaned Files Check
    print_header "Subtest 3: Orphaned Files Check" | tee -a "$RESULT_LOG"
    if [ "$orphaned_files" -eq 0 ]; then
        print_result 0 "No orphaned markdown files found" | tee -a "$RESULT_LOG"
        ((PASS_COUNT++))
    else
        print_result 1 "Found $orphaned_files orphaned markdown files" | tee -a "$RESULT_LOG"
    fi
    
    # Append full output to log for reference (even with --quiet, it's informative)
    print_header "Full Output of support_sitemap.sh" | tee -a "$RESULT_LOG"
    tee -a "$RESULT_LOG" < "$temp_output"
    print_header "End of Full Output" | tee -a "$RESULT_LOG"
    
    # Clean up
    rm -f "$temp_output"
    
    return $exit_code
}

# ====================================================================
# STEP 4: Execute Tests and Report Results
# ====================================================================

# Run all test cases
run_tests() {
    local test_result=0
    run_sitemap_check
    test_result=$?
    return $test_result
}

run_tests
TEST_RESULT=$?

# Collect test results for reporting
declare -a TEST_NAMES=("Overall Execution" "Markdown Files" "Missing Links" "Orphaned Files")
declare -a TEST_RESULTS
declare -a TEST_DETAILS

# Populate results (0=pass, 1=fail)
TEST_RESULTS=(0 0 0 0)  # Default to pass
TEST_DETAILS=("support_sitemap.sh execution" "Count of markdown files" "Check for missing links" "Check for orphaned files")

# Adjust based on actual results
if [ $TEST_RESULT -ne 0 ]; then
    TEST_RESULTS[0]=1
fi
# Set subtest results based on actual counts from run_sitemap_check
if [ "$missing_links" -ne 0 ]; then
    TEST_RESULTS[2]=1  # Missing Links fail if count is non-zero
fi
if [ "$orphaned_files" -ne 0 ]; then
    TEST_RESULTS[3]=1  # Orphaned Files fail if count is non-zero
fi

# Calculate statistics
TOTAL_PASS=0
TOTAL_FAIL=0
for result in "${TEST_RESULTS[@]}"; do
    if [ "$result" -eq 0 ]; then
        ((TOTAL_PASS++))
    else
        ((TOTAL_FAIL++))
    fi
done

# Print individual test results
echo -e "
${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$RESULT_LOG"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item "${TEST_RESULTS[$i]}" "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}" | tee -a "$RESULT_LOG"
done

# Print summary statistics
print_test_summary $((TOTAL_PASS + TOTAL_FAIL)) $TOTAL_PASS $TOTAL_FAIL | tee -a "$RESULT_LOG"

# Get test name from script name for subtest export
SCRIPT_TEST_NAME=$(basename "$0" .sh)
SCRIPT_TEST_NAME="${SCRIPT_TEST_NAME#test_}"

# Export subtest results for test_all.sh to pick up, including markdown count in details
export_subtest_results "$SCRIPT_TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT "${TEST_DETAILS[1]}"

# Log subtest results
print_info "Markdown Links Check: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# End test with final result
end_test $TEST_RESULT "$TEST_NAME" | tee -a "$RESULT_LOG"

exit $TEST_RESULT
