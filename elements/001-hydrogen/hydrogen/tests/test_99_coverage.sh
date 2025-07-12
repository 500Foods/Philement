#!/bin/bash

# Test: Coverage Collection and Analysis
# Collects and analyzes coverage data from Unity and blackbox tests

# Test configuration
TEST_NAME="Coverage Collection and Analysis"
SCRIPT_VERSION="2.0.0"

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

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=5
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

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

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
    
    print_message "Files instrumented: $unity_covered_files of $unity_instrumented_files source files have coverage"
    print_message "Lines instrumented: $unity_covered_lines of $unity_total_lines executable lines covered"
    print_result 0 "Unity test coverage recalled: $unity_coverage_percentage% - $unity_covered_files of $unity_instrumented_files files covered"
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
        
        print_message "Files instrumented: $covered_files of $instrumented_files source files have coverage"
        print_message "Lines instrumented: $covered_lines of $total_lines executable lines covered"
        print_result 0 "Blackbox test coverage collected: $coverage_percentage% - $covered_files of $instrumented_files files covered"
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

combined_coverage=$(calculate_combined_coverage)
result=$?

if [ $result -eq 0 ]; then
    print_result 0 "Combined coverage calculated: $combined_coverage%"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to calculate combined coverage"
    EXIT_CODE=1
fi

# Subtest 4: Calculate Coverage Overlap
next_subtest
print_subtest "Calculate Coverage Overlap"

print_message "Calculating coverage overlap between Unity and blackbox tests..."

overlap_percentage=$(calculate_coverage_overlap)
result=$?

if [ $result -eq 0 ]; then
    print_result 0 "Coverage overlap calculated: $overlap_percentage%"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to calculate coverage overlap"
    EXIT_CODE=1
fi

# Subtest 5: Identify Uncovered Source Files
next_subtest
print_subtest "Identify Uncovered Source Files"

print_message "Identifying source files not covered by blackbox tests..."

# Get project root for file analysis
PROJECT_ROOT="$HYDROGEN_DIR"
uncovered_analysis=$(identify_uncovered_files "$PROJECT_ROOT")

# Parse the results
covered_count=0
uncovered_count=0
total_count=0

while IFS= read -r line; do
    if [[ "$line" == "COVERED_FILES_COUNT:"* ]]; then
        covered_count=${line#*:}
    elif [[ "$line" == "UNCOVERED_FILES_COUNT:"* ]]; then
        uncovered_count=${line#*:}
    elif [[ "$line" == "TOTAL_SOURCE_FILES:"* ]]; then
        total_count=${line#*:}
    elif [[ "$line" == "UNCOVERED_FILES:" ]]; then
        # Start of uncovered files list
        print_message "Uncovered source files:"
        continue
    elif [[ "$line" =~ ^/.*/src/.* ]]; then
        # This is an uncovered file path
        relative_path=${line#"$PROJECT_ROOT"/}
        print_output "  $relative_path"
    fi
done <<< "$uncovered_analysis"

if [[ $total_count -gt 0 ]]; then
    print_result 0 "Coverage analysis: $covered_count of $total_count source files covered, $uncovered_count uncovered"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to analyze source file coverage"
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
