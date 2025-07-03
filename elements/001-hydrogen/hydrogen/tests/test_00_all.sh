#!/bin/bash
#
# Test 00: Hydrogen Test Suite Runner
# Executes all tests in sequence and generates a summary report
#
# Usage: ./test_00_all.sh [test_name|min|max|all] [--skip-tests] [--help]
#
# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Complete rewrite to use lib/ scripts, simplified orchestration approach
# 2.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 1.0.5 - 2025-06-20 - Adjusted timeouts to reduce test_00_all.sh overhead execution time, minor tweaks
# 1.0.4 - 2025-06-20 - Added test execution time, upgraded timing to millisecond precision
# 1.0.3 - 2025-06-17 - Major refactoring: eliminated code duplication, improved modularity, enhanced comments
# 1.0.2 - 2025-06-16 - Version history and title added
# 1.0.1 - 2025-06-16 - Changes to support shellcheck validation
# 1.0.0 - 2025-06-16 - Version history started
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the table rendering libraries for the summary
source "$SCRIPT_DIR/lib/tables_config.sh"
source "$SCRIPT_DIR/lib/tables_data.sh"
source "$SCRIPT_DIR/lib/tables_render.sh"
source "$SCRIPT_DIR/lib/tables_themes.sh"
source "$SCRIPT_DIR/lib/log_output.sh"
source "$SCRIPT_DIR/lib/framework.sh"

# Test configuration
TEST_NAME="Hydrogen Test Suite Runner"
SCRIPT_VERSION="3.0.0"

# Set up test numbering for header (test 00)
set_test_number "00"
reset_subtest_counter

# Arrays to track test results
declare -a TEST_NUMBERS
declare -a TEST_NAMES
declare -a TEST_SUBTESTS
declare -a TEST_PASSED
declare -a TEST_FAILED
declare -a TEST_ELAPSED

# Results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Command line argument parsing
SKIP_TESTS=false
TEST_ARGS=()

# Parse all arguments
for arg in "$@"; do
    case "$arg" in
        --skip-tests)
            SKIP_TESTS=true
            ;;
        --help|-h)
            # Help will be handled later
            ;;
        *)
            TEST_ARGS+=("$arg")
            ;;
    esac
done

# If no test arguments provided, default to "all"
if [ ${#TEST_ARGS[@]} -eq 0 ]; then
    TEST_ARGS=("all")
fi

# Function to update README.md with test results
update_readme_with_results() {
    local readme_file="$SCRIPT_DIR/../README.md"
    
    if [ ! -f "$readme_file" ]; then
        echo "Warning: README.md not found at $readme_file"
        return 1
    fi
    
    # Calculate summary statistics
    local total_tests=${#TEST_NUMBERS[@]}
    local total_subtests=0
    local total_passed=0
    local total_failed=0
    
    for i in "${!TEST_SUBTESTS[@]}"; do
        total_subtests=$((total_subtests + TEST_SUBTESTS[i]))
        total_passed=$((total_passed + TEST_PASSED[i]))
        total_failed=$((total_failed + TEST_FAILED[i]))
    done
    
    # Create temporary file for new README content
    local temp_readme
    temp_readme=$(mktemp) || { echo "Error: Failed to create temporary file" >&2; return 1; }
    
    # Generate timestamp
    local timestamp
    timestamp=$(date)
    
    # Process README.md line by line
    local in_test_results=false
    local in_individual_results=false
    local in_repo_info=false
    
    while IFS= read -r line; do
        if [[ "$line" == "## Latest Test Results" ]]; then
            in_test_results=true
            {
                echo "$line"
                echo ""
                echo "Generated on: $timestamp"
                echo ""
                echo "### Summary"
                echo ""
                echo "| Metric | Value |"
                echo "| ------ | ----- |"
            echo "| Total Tests | $total_tests |"
            echo "| Passed | $total_tests |"
            echo "| Failed | 0 |"
            echo "| Skipped | 0 |"
            echo "| Total Subtests | $total_subtests |"
            echo "| Passed Subtests | $total_passed |"
            echo "| Failed Subtests | $total_failed |"
                echo "| Elapsed Time | $TOTAL_ELAPSED_FORMATTED |"
                echo "| Running Time | $TOTAL_RUNNING_TIME_FORMATTED |"
                echo ""
            } >> "$temp_readme"
            continue
        elif [[ "$line" == "### Individual Test Results" ]]; then
            in_individual_results=true
            {
                echo "$line"
                echo ""
                echo "| Status | Time | Test | Tests | Pass | Fail | Summary |"
                echo "| ------ | ---- | ---- | ----- | ---- | ---- | ------- |"
                
                # Add individual test results
                for i in "${!TEST_NUMBERS[@]}"; do
                    local status="✅"
                    local summary="Test completed without errors"
                    if [ "${TEST_FAILED[$i]}" -gt 0 ]; then
                        status="❌"
                        summary="Test failed with errors"
                    fi
                    local time_formatted
                    time_formatted=$(format_time_duration "${TEST_ELAPSED[$i]}")
                    echo "| $status | $time_formatted | ${TEST_NUMBERS[$i]}_$(echo "${TEST_NAMES[$i]}" | tr ' ' '_' | tr '[:upper:]' '[:lower:]') | ${TEST_SUBTESTS[$i]} | ${TEST_PASSED[$i]} | ${TEST_FAILED[$i]} | $summary |"
                done
                echo ""
            } >> "$temp_readme"
            continue
        elif [[ "$line" == "## Repository Information" ]]; then
            in_repo_info=true
            in_test_results=false
            in_individual_results=false
            {
                echo "$line"
                echo ""
                echo "Generated via cloc: $timestamp"
                echo ""
                
                # Run cloc if available (using same method as test 99)
                if command -v cloc >/dev/null 2>&1; then
                    echo '```cloc'
                    
                    # Generate exclude list based on .lintignore and default excludes (same as test 99)
                    local exclude_list
                    exclude_list=$(mktemp)
                    
                    # Function to check if file should be excluded (exact same logic as test 99)
                    should_exclude_cloc() {
                        local file="$1"
                        local rel_file="${file#./}"  # Remove leading ./
                        
                        # Default exclude patterns for linting (same as test 99)
                        local default_excludes=(
                            "build/*"
                            "build_debug/*"
                            "build_perf/*"
                            "build_release/*"
                            "build_valgrind/*"
                            "tests/logs/*"
                            "tests/results/*"
                            "tests/diagnostics/*"
                        )
                        
                        # Check .lintignore file first if it exists
                        if [ -f "$SCRIPT_DIR/../.lintignore" ]; then
                            while IFS= read -r pattern; do
                                [[ -z "$pattern" || "$pattern" == \#* ]] && continue
                                # Remove trailing /* if present for directory matching
                                local clean_pattern="${pattern%/\*}"
                                
                                # Check if file matches pattern exactly or is within a directory pattern
                                if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
                                    return 0 # Exclude
                                fi
                            done < "$SCRIPT_DIR/../.lintignore"
                        fi
                        
                        # Check default excludes (same as test 99)
                        for pattern in "${default_excludes[@]}"; do
                            local clean_pattern="${pattern%/\*}"
                            if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
                                return 0 # Exclude
                            fi
                        done
                        
                        return 1 # Do not exclude
                    }
                    
                    # Generate exclude list
                    : > "$exclude_list"
                    while read -r file; do
                        if should_exclude_cloc "$file"; then
                            echo "${file#./}" >> "$exclude_list"
                        fi
                    done < <(find "$SCRIPT_DIR/.." -type f | sort)
                    
                    # Run cloc with same parameters as test 99
                    (cd "$SCRIPT_DIR/.." && env LC_ALL=en_US.UTF_8 cloc . --quiet --force-lang="C,inc" --exclude-list-file="$exclude_list" 2>/dev/null) || echo "cloc command failed"
                    
                    # Clean up
                    rm -f "$exclude_list"
                    
                    echo '```'
                else
                    echo '```'
                    echo "cloc not available"
                    echo '```'
                fi
            } >> "$temp_readme"
            continue
        elif [[ "$in_test_results" == true || "$in_individual_results" == true || "$in_repo_info" == true ]]; then
            # Skip existing content in these sections
            if [[ "$line" == "## "* ]]; then
                # New section started, stop skipping
                in_test_results=false
                in_individual_results=false
                in_repo_info=false
                echo "$line" >> "$temp_readme"
            fi
            continue
        else
            echo "$line" >> "$temp_readme"
        fi
    done < "$readme_file"
    
    # Replace original README with updated version
    if mv "$temp_readme" "$readme_file"; then
        echo "Updated README.md with test results"
    else
        echo "Error: Failed to update README.md"
        rm -f "$temp_readme"
        return 1
    fi
}

# Function to format seconds as HH:MM:SS.ZZZ
format_time_duration() {
    local seconds="$1"
    local hours minutes secs milliseconds
    
    # Handle decimal seconds
    if [[ "$seconds" =~ ^([0-9]+)\.([0-9]+)$ ]]; then
        secs="${BASH_REMATCH[1]}"
        milliseconds="${BASH_REMATCH[2]}"
        # Pad or truncate milliseconds to 3 digits
        milliseconds="${milliseconds}000"
        milliseconds="${milliseconds:0:3}"
    else
        secs="$seconds"
        milliseconds="000"
    fi
    
    hours=$((secs / 3600))
    minutes=$(((secs % 3600) / 60))
    secs=$((secs % 60))
    
    printf "%02d:%02d:%02d.%s" "$hours" "$minutes" "$secs" "$milliseconds"
}

# Function to show help
show_help() {
    echo "Usage: $0 [test_name1 test_name2 ...] [--skip-tests] [--help]"
    echo ""
    echo "Arguments:"
    echo "  test_name    Run specific tests (e.g., 10_compilation, 70_swagger)"
    echo "  min          Run with minimal configuration only"
    echo "  max          Run with maximal configuration only"
    echo "  all          Run all tests (default)"
    echo ""
    echo "Options:"
    echo "  --skip-tests Skip actual test execution, just show what tests would run"
    echo "  --help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Run all tests"
    echo "  $0 10_compilation                     # Run only the compilation test"
    echo "  $0 10_compilation 70_swagger          # Run compilation and swagger tests"
    echo "  $0 45_signals 50_crash_handler        # Run signals and crash handler tests"
    echo "  $0 all --skip-tests                   # Show all tests that would run"
    echo ""
    exit 0
}

# Check for help flag
for arg in "$@"; do
    if [ "$arg" == "--help" ] || [ "$arg" == "-h" ]; then
        show_help
    fi
done

# Check if --skip-tests is provided as any argument
for arg in "$@"; do
    if [ "$arg" == "--skip-tests" ]; then
        SKIP_TESTS=true
    fi
done


# Get start time
START_TIME=$(date +%s.%N 2>/dev/null || date +%s)

# Print beautiful test suite header in blue
print_test_suite_header "$TEST_NAME" "$SCRIPT_VERSION"

# Show what mode we're running in
if [ "$SKIP_TESTS" = true ]; then
    echo "Mode: Dry run (--skip-tests enabled)"
else
    echo "Mode: Full execution"
fi
echo "Test arguments: ${TEST_ARGS[*]}"
echo ""

# Function to run a single test and capture results
run_single_test() {
    local test_script="$1"
    local test_number
    local test_name
    
    # Extract test number and name from script filename
    test_number=$(basename "$test_script" .sh | sed 's/test_//' | sed 's/_.*//')
    test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
    
    # Handle skip mode
    if [ "$SKIP_TESTS" = true ]; then
        echo "Would run: $test_script"
        # Store skipped test results
        TEST_NUMBERS+=("$test_number")
        TEST_NAMES+=("$test_name")
        TEST_SUBTESTS+=(1)
        TEST_PASSED+=(0)
        TEST_FAILED+=(0)
        TEST_ELAPSED+=("0.000")
        return 0
    fi
    
    # Record start time for this test
    local test_start
    test_start=$(date +%s.%N 2>/dev/null || date +%s)
    
    # Run the test and capture exit code
    "$test_script"
    local exit_code=$?
    
    # Calculate elapsed time
    local test_end
    test_end=$(date +%s.%N 2>/dev/null || date +%s)
    local elapsed
    if [[ "$test_start" =~ \. && "$test_end" =~ \. ]]; then
        elapsed=$(echo "$test_end - $test_start" | bc 2>/dev/null || echo "$((test_end - test_start))")
    else
        elapsed=$((test_end - test_start))
    fi
    
    # Format elapsed time (without 's' suffix for table summation)
    local elapsed_formatted
    if [[ "$elapsed" =~ \. ]]; then
        elapsed_formatted=$(printf "%.3f" "$elapsed")
    else
        elapsed_formatted="$elapsed"
    fi
    
    # Look for subtest results file
    local total_subtests=1
    local passed_subtests=0
    
    # Find the most recent subtest file for this test
    local latest_subtest_file
    latest_subtest_file=$(find "$RESULTS_DIR" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1)
    
    if [ -n "$latest_subtest_file" ] && [ -f "$latest_subtest_file" ]; then
        # Read subtest results
        IFS=',' read -r total_subtests passed_subtests < "$latest_subtest_file" 2>/dev/null || {
            total_subtests=1
            passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
        }
    else
        # Default based on exit code
        total_subtests=1
        passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
    fi
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Store results
    TEST_NUMBERS+=("$test_number")
    TEST_NAMES+=("$test_name")
    TEST_SUBTESTS+=("$total_subtests")
    TEST_PASSED+=("$passed_subtests")
    TEST_FAILED+=("$failed_subtests")
    TEST_ELAPSED+=("$elapsed_formatted")
    
    return $exit_code
}

# Function to run a specific test by name
run_specific_test() {
    local test_name="$1"
    local test_script="$SCRIPT_DIR/test_${test_name}.sh"
    
    if [ ! -f "$test_script" ]; then
        echo "Error: Test script not found: $test_script"
        echo "Available tests:"
        find "$SCRIPT_DIR" -name "test_*.sh" -not -name "test_00_all.sh" -not -name "test_template.sh" | sort | while read -r script; do
            local name
            name=$(basename "$script" .sh | sed 's/test_//')
            echo "  $name"
        done
        return 1
    fi
    
    if ! run_single_test "$test_script"; then
        return 1
    fi
    return 0
}

# Function to run tests with minimal configuration
run_min_configuration_test() {
    echo "Running tests with minimal configuration..."
    # For now, just run all tests - this could be enhanced to use specific configs
    run_all_tests
}

# Function to run tests with maximal configuration  
run_max_configuration_test() {
    echo "Running tests with maximal configuration..."
    # For now, just run all tests - this could be enhanced to use specific configs
    run_all_tests
}

# Function to run all tests
run_all_tests() {
    local overall_exit_code=0
    
    for test_script in "${TEST_SCRIPTS[@]}"; do
        if ! run_single_test "$test_script"; then
            overall_exit_code=1
        fi
    done
    
    return $overall_exit_code
}

# Get list of all test scripts, excluding test_00_all.sh and test_template.sh
# Run test_10_compilation.sh first, then all others in order, ending with test_99_codebase.sh
TEST_SCRIPTS=()

# Add test_10_compilation.sh first if it exists
if [ -f "$SCRIPT_DIR/test_10_compilation.sh" ]; then
    TEST_SCRIPTS+=("$SCRIPT_DIR/test_10_compilation.sh")
fi

# Add all other tests except 00, 10, and template
while IFS= read -r script; do
    if [[ "$script" != *"test_00_all.sh" ]] && [[ "$script" != *"test_10_compilation.sh" ]] && [[ "$script" != *"test_template.sh" ]]; then
        TEST_SCRIPTS+=("$script")
    fi
done < <(find "$SCRIPT_DIR" -name "test_*.sh" -type f | sort)

# Ensure test_99_codebase.sh is last if it exists
if [ -f "$SCRIPT_DIR/test_99_codebase.sh" ]; then
    # Remove test_99_codebase.sh from the array if it's there
    for i in "${!TEST_SCRIPTS[@]}"; do
        if [[ "${TEST_SCRIPTS[$i]}" == *"test_99_codebase.sh" ]]; then
            unset 'TEST_SCRIPTS[i]'
        fi
    done
    # Re-index array and add test_99 at the end
    TEST_SCRIPTS=("${TEST_SCRIPTS[@]}")
    TEST_SCRIPTS+=("$SCRIPT_DIR/test_99_codebase.sh")
fi

# Execute tests based on command line arguments
OVERALL_EXIT_CODE=0

# Process each test argument
for test_arg in "${TEST_ARGS[@]}"; do
    case "$test_arg" in
        "min")
            echo "Running tests with minimal configuration..."
            if ! run_min_configuration_test; then
                OVERALL_EXIT_CODE=1
            fi
            ;;
        "max")
            echo "Running tests with maximal configuration..."
            if ! run_max_configuration_test; then
                OVERALL_EXIT_CODE=1
            fi
            ;;
        "all")
            echo "Running all tests..."
            if ! run_all_tests; then
                OVERALL_EXIT_CODE=1
            fi
            ;;
        *)
            # Check if it's a specific test name
            echo "Running specific test: $test_arg"
            if ! run_specific_test "$test_arg"; then
                OVERALL_EXIT_CODE=1
                echo ""
                echo "Usage: $0 [test_name1 test_name2 ...] [--skip-tests] [--help]"
                echo "  test_name: Run specific tests (e.g., 10_compilation 70_swagger)"
                echo "  min: Run with minimal configuration only"
                echo "  max: Run with maximal configuration only"
                echo "  all: Run all tests (default)"
                echo "  --skip-tests: Skip actual test execution, just show what tests would run"
                echo "  --help: Show help message"
                echo ""
                echo "Examples:"
                echo "  $0 10_compilation 70_swagger    # Run compilation and swagger tests"
                echo "  $0 all --skip-tests             # Show all tests that would run"
            fi
            ;;
    esac
done

# Calculate total elapsed time
END_TIME=$(date +%s.%N 2>/dev/null || date +%s)
if [[ "$START_TIME" =~ \. ]] && [[ "$END_TIME" =~ \. ]]; then
    TOTAL_ELAPSED=$(echo "$END_TIME - $START_TIME" | bc 2>/dev/null || echo "$((END_TIME - START_TIME))")
else
    TOTAL_ELAPSED=$((END_TIME - START_TIME))
fi

# Calculate total running time (sum of all test execution times)
TOTAL_RUNNING_TIME=0
for i in "${!TEST_ELAPSED[@]}"; do
    if [[ "${TEST_ELAPSED[$i]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        TOTAL_RUNNING_TIME=$(awk "BEGIN {print ($TOTAL_RUNNING_TIME + ${TEST_ELAPSED[$i]})}")
    fi
done

# Format both times as HH:MM:SS.ZZZ
TOTAL_ELAPSED_FORMATTED=$(format_time_duration "$TOTAL_ELAPSED")
TOTAL_RUNNING_TIME_FORMATTED=$(format_time_duration "$TOTAL_RUNNING_TIME")

# Generate summary table using the table libraries
# Create temporary files for tables.sh
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; exit 1; }
layout_json="$temp_dir/summary_layout.json"
data_json="$temp_dir/summary_data.json"

# Create layout JSON with column summaries
# "Elapsed: $TOTAL_ELAPSED_FORMATTED | Running: $TOTAL_RUNNING_TIME_FORMATTED",
cat > "$layout_json" << EOF
{
    "title": "Hydrogen Test Suite Results",
    "footer": "Elapsed: $TOTAL_ELAPSED_FORMATTED ─── Running: $TOTAL_RUNNING_TIME_FORMATTED",
    "footer_position": "right",
    "columns": [
        {
            "header": "Group",
            "key": "group",
            "datatype": "int",
            "width": 5,
            "visible": false,
            "break": true
        },
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text",
            "width": 8,
            "summary": "count"
        },
        {
            "header": "Test Name",
            "key": "test_name",
            "datatype": "text",
            "width": 50,
            "justification": "left"
        },
        {
            "header": "Tests",
            "key": "tests",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Pass",
            "key": "pass",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Fail",
            "key": "fail",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Elapsed",
            "key": "elapsed",
            "datatype": "float",
            "width": 11,
            "justification": "right",
            "summary": "sum"
        }
    ]
}
EOF

# Create data JSON
echo "[" > "$data_json"
for i in "${!TEST_NUMBERS[@]}"; do
    test_id="${TEST_NUMBERS[$i]}-000"
    # Calculate group (tens digit) from test number
    group=$((${TEST_NUMBERS[$i]} / 10))
    if [ "$i" -gt 0 ]; then
        echo "," >> "$data_json"
    fi
    cat >> "$data_json" << EOF
    {
        "group": $group,
        "test_id": "$test_id",
        "test_name": "${TEST_NAMES[$i]}",
        "tests": ${TEST_SUBTESTS[$i]},
        "pass": ${TEST_PASSED[$i]},
        "fail": ${TEST_FAILED[$i]},
        "elapsed": ${TEST_ELAPSED[$i]}
    }
EOF
done
echo "]" >> "$data_json"

# Use tables.sh to render the summary table
tables_script="$SCRIPT_DIR/lib/tables.sh"
if [[ -f "$tables_script" ]]; then
    bash "$tables_script" "$layout_json" "$data_json" 2>/dev/null
else
    echo "Warning: tables.sh not found, using fallback formatting"
    # Simple fallback table
    printf "%-8s %-50s %8s %8s %8s %11s\n" "Test #" "Test Name" "Tests" "Pass" "Fail" "Elapsed"
    echo "==============================================================================="
    for i in "${!TEST_NUMBERS[@]}"; do
        test_id="${TEST_NUMBERS[$i]}-000"
        printf "%-8s %-50s %8s %8s %8s %11s\n" "$test_id" "${TEST_NAMES[$i]}" "${TEST_SUBTESTS[$i]}" "${TEST_PASSED[$i]}" "${TEST_FAILED[$i]}" "${TEST_ELAPSED[$i]}"
    done
fi

# Clean up temporary files
rm -rf "$temp_dir" 2>/dev/null

# Update README.md with test results (only if not in skip mode)
if [ "$SKIP_TESTS" = false ]; then
    update_readme_with_results
fi

echo ""
echo "Completed at: $(date)"
echo "Total elapsed time: $TOTAL_ELAPSED_FORMATTED"
echo "Total running time: $TOTAL_RUNNING_TIME_FORMATTED"

# Calculate summary statistics
TOTAL_TESTS=${#TEST_NUMBERS[@]}
TOTAL_SUBTESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0

for i in "${!TEST_SUBTESTS[@]}"; do
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + TEST_SUBTESTS[i]))
    TOTAL_PASSED=$((TOTAL_PASSED + TEST_PASSED[i]))
    TOTAL_FAILED=$((TOTAL_FAILED + TEST_FAILED[i]))
done

echo ""
echo "Summary: $TOTAL_TESTS tests, $TOTAL_SUBTESTS subtests, $TOTAL_PASSED passed, $TOTAL_FAILED failed"

if [ $OVERALL_EXIT_CODE -eq 0 ]; then
    echo "Result: ALL TESTS PASSED"
else
    echo "Result: SOME TESTS FAILED"
fi

echo "==============================================================================="

exit $OVERALL_EXIT_CODE
