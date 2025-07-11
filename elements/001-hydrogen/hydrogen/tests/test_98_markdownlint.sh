#!/bin/bash

# Test: Markdown Linting
# Performs markdown linting validation

# Test configuration
TEST_NAME="Markdown Linting (markdownlint)"
SCRIPT_VERSION="1.0.0"

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

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=1
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

# Test configuration constants
readonly LINT_OUTPUT_LIMIT=100

# Default exclude patterns for linting (can be overridden by .lintignore)
readonly LINT_EXCLUDES=(
    "build/*"
    "build_debug/*"
    "build_perf/*"
    "build_release/*"
    "build_valgrind/*"
    "tests/logs/*"
    "tests/results/*"
    "tests/diagnostics/*"
)

# Check if a file should be excluded from linting
should_exclude_file() {
    local file="$1"
    local lint_ignore=".lintignore"
    local rel_file="${file#./}"  # Remove leading ./
    
    # Check .lintignore file first if it exists
    if [ -f "$lint_ignore" ]; then
        while IFS= read -r pattern; do
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            
            # Check if file matches pattern exactly or is within a directory pattern
            if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
                return 0 # Exclude
            fi
        done < "$lint_ignore"
    fi
    
    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
            return 0 # Exclude
        fi
    done
    
    return 1 # Do not exclude
}

# Subtest 1: Lint Markdown files
next_subtest
print_subtest "Markdown Linting"

if command -v markdownlint >/dev/null 2>&1; then
    MD_FILES=()
    while read -r file; do
        if ! should_exclude_file "$file"; then
            MD_FILES+=("$file")
        fi
    done < <(find . -type f -name "*.md")
    
    MD_COUNT=${#MD_FILES[@]}
    
    if [ "$MD_COUNT" -gt 0 ]; then
        TEMP_LOG=$(mktemp)
        FILTERED_LOG=$(mktemp)
        
        # Run markdownlint on all files at once for better performance
        print_message "Running markdownlint on $MD_COUNT files..."
        if ! markdownlint --config ".lintignore-markdown" "${MD_FILES[@]}" 2> "$TEMP_LOG"; then
            # markdownlint failed, but we'll check the output for actual issues
            :
        fi
        
        if [ -s "$TEMP_LOG" ]; then
            # Filter out Node.js deprecation warnings and other noise
            grep -v "DeprecationWarning" "$TEMP_LOG" | \
            grep -v "Use \`node --trace-deprecation" | \
            grep -v "^(node:" | \
            grep -v "^$" > "$FILTERED_LOG"
            
            ISSUE_COUNT=$(wc -l < "$FILTERED_LOG")
            if [ "$ISSUE_COUNT" -gt 0 ]; then
                print_message "markdownlint found $ISSUE_COUNT actual issues:"
                head -n "$LINT_OUTPUT_LIMIT" "$FILTERED_LOG" | while IFS= read -r line; do
                    print_output "$line"
                done
                if [ "$ISSUE_COUNT" -gt "$LINT_OUTPUT_LIMIT" ]; then
                    print_message "Output truncated. Showing $LINT_OUTPUT_LIMIT of $ISSUE_COUNT lines."
                fi
                print_result 1 "Found $ISSUE_COUNT issues in $MD_COUNT markdown files"
                EXIT_CODE=1
            else
                print_message "markdownlint completed (Node.js deprecation warnings filtered out)"
                print_result 0 "No issues in $MD_COUNT markdown files"
                ((PASS_COUNT++))
            fi
        else
            print_result 0 "No issues in $MD_COUNT markdown files"
            ((PASS_COUNT++))
        fi
        
        rm -f "$TEMP_LOG" "$FILTERED_LOG"
    else
        print_result 0 "No markdown files to check"
        ((PASS_COUNT++))
    fi
else
    print_result 0 "markdownlint not available (skipped)"
    ((PASS_COUNT++))
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