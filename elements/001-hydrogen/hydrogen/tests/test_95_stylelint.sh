#!/bin/bash

# Test: CSS Linting
# Performs CSS validation using stylelint

# Test configuration
TEST_NAME="CSS Linting (stylelint)"
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
    "tests/unity/framework/*"
    "node_modules/*"
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

# Subtest 1: Lint CSS files
next_subtest
print_subtest "CSS Linting (stylelint)"

if command -v stylelint >/dev/null 2>&1; then
    CSS_FILES=()
    while read -r file; do
        if ! should_exclude_file "$file"; then
            CSS_FILES+=("$file")
        fi
    done < <(find . -type f -name "*.css")
    
    CSS_COUNT=${#CSS_FILES[@]}
    
    if [ "$CSS_COUNT" -gt 0 ]; then
        TEMP_LOG=$(mktemp)
        
        print_message "Running stylelint on $CSS_COUNT CSS files..."
        # Use basic stylelint rules if no config exists
        if [ -f ".stylelintrc.json" ] || [ -f ".stylelintrc.js" ] || [ -f "stylelint.config.js" ]; then
            stylelint "${CSS_FILES[@]}" > "$TEMP_LOG" 2>&1 || true
        else
            # Use recommended rules if no config
            stylelint --config '{"extends": ["stylelint-config-standard"]}' "${CSS_FILES[@]}" > "$TEMP_LOG" 2>&1 || true
        fi
        
        # Count actual issues (stylelint usually reports errors/warnings)
        ISSUE_COUNT=$(grep -c "✖" "$TEMP_LOG" 2>/dev/null || wc -l < "$TEMP_LOG")
        
        if [ "$ISSUE_COUNT" -gt 0 ]; then
            print_message "stylelint found $ISSUE_COUNT issues:"
            head -n "$LINT_OUTPUT_LIMIT" "$TEMP_LOG" | while IFS= read -r line; do
                print_output "$line"
            done
            if [ "$ISSUE_COUNT" -gt "$LINT_OUTPUT_LIMIT" ]; then
                print_message "Output truncated. Showing $LINT_OUTPUT_LIMIT of $ISSUE_COUNT lines."
            fi
            print_result 1 "Found $ISSUE_COUNT issues in $CSS_COUNT CSS files"
            EXIT_CODE=1
        else
            print_result 0 "No issues in $CSS_COUNT CSS files"
            ((PASS_COUNT++))
        fi
        
        rm -f "$TEMP_LOG"
    else
        print_result 0 "No CSS files to check"
        ((PASS_COUNT++))
    fi
else
    print_result 0 "stylelint not available (skipped)"
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