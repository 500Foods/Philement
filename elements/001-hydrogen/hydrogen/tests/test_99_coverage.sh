#!/bin/bash

# Test: Coverage Collection and Analysis
# Collects and analyzes coverage data from Unity and blackbox tests

# CHANGELOG
# 2.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.0 - Initial version with comprehensive coverage analysis

# Test configuration
TEST_NAME="Test Suite Coverage (coverage_tables)"
SCRIPT_VERSION="2.0.1"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$SCRIPT_DIR/lib/coverage.sh"
# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
source "$SCRIPT_DIR/lib/coverage-common.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Use build/tests/results directory for consistency
BUILD_DIR="$SCRIPT_DIR/../build"
RESULTS_DIR="$BUILD_DIR/tests/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Subtest 1: Recall Unity Test Coverage
next_subtest
print_subtest "Recall Unity Test Coverage"

print_message "Recalling coverage data from Unity tests (Test 11)..."

# Find the Unity build directory
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
UNITY_BUILD_DIR="$HYDROGEN_DIR/build/unity"

# Read Unity coverage data from Test 11's stored results instead of recalculating
if [ -f "${UNITY_COVERAGE_FILE}" ] && [ -f "${UNITY_COVERAGE_FILE}.detailed" ]; then
    unity_coverage_percentage=$(cat "${UNITY_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    
    # Parse detailed coverage data to get file counts
    unity_instrumented_files=0
    unity_covered_files=0
    unity_covered_lines=0
    unity_total_lines=0
    IFS=',' read -r _ _ unity_covered_lines unity_total_lines unity_instrumented_files unity_covered_files < "${UNITY_COVERAGE_FILE}.detailed"
    
    # Format numbers with thousands separators
    formatted_unity_covered_files=$(printf "%'d" "$unity_covered_files")
    formatted_unity_instrumented_files=$(printf "%'d" "$unity_instrumented_files")
    formatted_unity_covered_lines=$(printf "%'d" "$unity_covered_lines")
    formatted_unity_total_lines=$(printf "%'d" "$unity_total_lines")
    
    print_message "Files instrumented: $formatted_unity_covered_files of $formatted_unity_instrumented_files source files have coverage"
    print_message "Lines instrumented: $formatted_unity_covered_lines of $formatted_unity_total_lines executable lines covered"
    print_result 0 "Unity test coverage recalled: $unity_coverage_percentage% - $formatted_unity_covered_files of $formatted_unity_instrumented_files files covered"
    ((PASS_COUNT++))
else
    print_result 1 "Unity coverage data not found. Run Test 11 first to generate Unity coverage data."
    EXIT_CODE=1
fi

# Subtest 2: Collect Blackbox Test Coverage
next_subtest
print_subtest "Collect Blackbox Test Coverage"

print_message "Collecting coverage data from blackbox tests..."

# Check for blackbox coverage data in build/coverage directory only
BLACKBOX_COVERAGE_DIR="$HYDROGEN_DIR/build/coverage"

if [ -d "$BLACKBOX_COVERAGE_DIR" ]; then
    # Collect blackbox coverage data strictly from build/coverage
    coverage_percentage=$(collect_blackbox_coverage_from_dir "$BLACKBOX_COVERAGE_DIR" "$TIMESTAMP")
    result=$?
    
    if [ $result -eq 0 ]; then
        # Parse detailed coverage data to get file counts
        instrumented_files=0
        covered_files=0
        covered_lines=0
        total_lines=0
        if [ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]; then
            IFS=',' read -r _ _ covered_lines total_lines instrumented_files covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
        fi
        
        # Format numbers with thousands separators
        formatted_covered_files=$(printf "%'d" "$covered_files")
        formatted_instrumented_files=$(printf "%'d" "$instrumented_files")
        formatted_covered_lines=$(printf "%'d" "$covered_lines")
        formatted_total_lines=$(printf "%'d" "$total_lines")
        
        print_message "Files instrumented: $formatted_covered_files of $formatted_instrumented_files source files have coverage"
        print_message "Lines instrumented: $formatted_covered_lines of $formatted_total_lines executable lines covered"
        print_result 0 "Blackbox test coverage collected: $coverage_percentage% - $formatted_covered_files of $formatted_instrumented_files files covered"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to collect blackbox test coverage"
        EXIT_CODE=1
    fi
else
    print_result 1 "Blackbox coverage directory not found at: $BLACKBOX_COVERAGE_DIR"
    EXIT_CODE=1
fi

# Subtest 3: Calculate Combined Coverage
next_subtest
print_subtest "Calculate Combined Coverage"

print_message "Calculating combined coverage from Unity and blackbox tests..."

# Use the same logic as our successful coverage_table.sh implementation
if [ -f "${UNITY_COVERAGE_FILE}.detailed" ] && [ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]; then
    # Read Unity coverage data
    IFS=',' read -r _ _ unity_covered_lines unity_total_lines unity_instrumented_files unity_covered_files < "${UNITY_COVERAGE_FILE}.detailed"
    
    # Read Blackbox coverage data
    IFS=',' read -r _ _ blackbox_covered_lines blackbox_total_lines blackbox_instrumented_files blackbox_covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    # Calculate combined coverage using max logic (union approach)
    # For total lines: use the maximum total lines from either test suite
    combined_total_lines=$((unity_total_lines > blackbox_total_lines ? unity_total_lines : blackbox_total_lines))
    
    # For covered lines: use the maximum covered lines (represents minimum bound of union)
    combined_covered_lines=$((unity_covered_lines > blackbox_covered_lines ? unity_covered_lines : blackbox_covered_lines))
    
    # Calculate combined percentage
    if [ "$combined_total_lines" -gt 0 ]; then
        combined_coverage=$(awk "BEGIN {printf \"%.3f\", ($combined_covered_lines / $combined_total_lines) * 100}")
        
        # Format numbers with thousands separators
        formatted_covered=$(printf "%'d" "$combined_covered_lines")
        formatted_total=$(printf "%'d" "$combined_total_lines")
        
        # Store the combined coverage value for other scripts to use
        echo "$combined_coverage" > "$COMBINED_COVERAGE_FILE"
        
        # Calculate combined file counts for the detailed file
        # For files covered: count unique files that have coverage in either test suite
        combined_covered_files=$((unity_covered_files > blackbox_covered_files ? unity_covered_files : blackbox_covered_files))
        combined_instrumented_files=$((unity_instrumented_files > blackbox_instrumented_files ? unity_instrumented_files : blackbox_instrumented_files))
        
        # Store detailed combined coverage data for Test 00 to use in README
        # Format: timestamp,percentage,covered_lines,total_lines,instrumented_files,covered_files
        echo "$(date +%Y%m%d_%H%M%S),$combined_coverage,$combined_covered_lines,$combined_total_lines,$combined_instrumented_files,$combined_covered_files" > "${COMBINED_COVERAGE_FILE}.detailed"
        
        print_result 0 "Combined coverage calculated: $combined_coverage% - $formatted_covered of $formatted_total lines covered"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to calculate combined coverage: no instrumented lines found"
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to calculate combined coverage: detailed coverage data not available"
    EXIT_CODE=1
fi

# Subtest 4: Identify Uncovered Source Files
next_subtest
print_subtest "Identify Uncovered Source Files"

print_message "Identifying source files not covered by blackbox tests..."

# Get project root for file analysis
PROJECT_ROOT="$HYDROGEN_DIR"
uncovered_analysis=$(identify_uncovered_files "$PROJECT_ROOT")

# Use the already-calculated values from blackbox coverage instead of recalculating
if [ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]; then
    # Read the values from blackbox coverage calculation
    IFS=',' read -r _ _ _ _ instrumented_files covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    # Parse just the uncovered files list for display
    while IFS= read -r line; do
        if [[ "$line" == "UNCOVERED_FILES:" ]]; then
            # Start of uncovered files list
            print_message "Uncovered source files:"
            continue
        elif [[ "$line" =~ ^/.*/src/.* ]]; then
            # This is an uncovered file path
            relative_path=${line#"$PROJECT_ROOT"/}
            print_output "  $relative_path"
        fi
    done <<< "$uncovered_analysis"
    
    # Calculate uncovered count from the known values
    uncovered_count=$((instrumented_files - covered_files))
    
    # Format numbers with thousands separators
    formatted_covered_files=$(printf "%'d" "$covered_files")
    formatted_instrumented_files=$(printf "%'d" "$instrumented_files")
    formatted_uncovered_count=$(printf "%'d" "$uncovered_count")
    
    print_result 0 "Coverage analysis: $formatted_covered_files of $formatted_instrumented_files source files covered, $formatted_uncovered_count uncovered"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to analyze source file coverage - blackbox detailed data not found"
    EXIT_CODE=1
fi

# Export results for test_all.sh integration
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
