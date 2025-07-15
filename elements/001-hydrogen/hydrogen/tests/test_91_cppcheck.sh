#!/bin/bash

# Test: C/C++ Code Analysis
# Performs cppcheck analysis on C/C++ source files

# CHANGELOG
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for C/C++ code analysis

# Test configuration
TEST_NAME="C/C++ Code Analysis (cppcheck)"
SCRIPT_VERSION="2.0.0"

# Get the directory where this script is located
TEST_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$TEST_SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/framework.sh"

# Restore SCRIPT_DIR after sourcing libraries (they may override it)
SCRIPT_DIR="$TEST_SCRIPT_DIR"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=1
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Detect available cores for optimal parallelization
CORES=$(nproc 2>/dev/null || echo 1)

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory - always use build/tests/results
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

# Function to run cppcheck
run_cppcheck() {
    local target_dir="$1"
    
    # Check for required files
    if [ ! -f ".lintignore" ]; then
        print_error ".lintignore not found"
        return 1
    fi
    if [ ! -f ".lintignore-c" ]; then
        print_error ".lintignore-c not found"
        return 1
    fi
    
    # Cache exclude patterns
    local exclude_patterns=()
    while IFS= read -r pattern; do
        pattern=$(echo "$pattern" | sed 's/#.*//;s/^[[:space:]]*//;s/[[:space:]]*$//')
        [ -z "$pattern" ] && continue
        exclude_patterns+=("$pattern")
    done < ".lintignore"
    
    # Function to check if file should be excluded
    should_exclude_cppcheck() {
        local file="$1"
        local rel_file="${file#./}"  # Remove leading ./
        
        for pattern in "${exclude_patterns[@]}"; do
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            
            # Check if file matches pattern exactly or is within a directory pattern
            if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
                return 0 # Exclude
            fi
        done
        return 1 # Do not exclude
    }
    
    # Parse .lintignore-c
    local cppcheck_args=()
    while IFS='=' read -r key value; do
        case "$key" in
            "enable") cppcheck_args+=("--enable=$value") ;;
            "include") cppcheck_args+=("--include=$value") ;;
            "check-level") cppcheck_args+=("--check-level=$value") ;;
            "template") cppcheck_args+=("--template=$value") ;;
            "option") cppcheck_args+=("$value") ;;
            "suppress") cppcheck_args+=("--suppress=$value") ;;
            *) ;;
        esac
    done < <(grep -v '^#' ".lintignore-c" | grep '=')
    
    # Collect files with inline filtering
    local files=()
    while read -r file; do
        if ! should_exclude_cppcheck "$file"; then
            files+=("$file")
        fi
    done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \))
    
    if [ ${#files[@]} -gt 0 ]; then
        cppcheck -j"$CORES" --quiet "${cppcheck_args[@]}" "${files[@]}" 2>&1
    else
        echo ""
    fi
}

# Subtest 1: Lint C files with cppcheck
next_subtest
print_subtest "C/C++ Code Linting (cppcheck)"

print_message "Detected $CORES CPU cores for parallel processing"

if command -v cppcheck >/dev/null 2>&1; then
    # Count files that will be checked (excluding .lintignore patterns)
    C_FILES_TO_CHECK=()
    while read -r file; do
        if ! should_exclude_file "$file"; then
            C_FILES_TO_CHECK+=("$file")
        fi
    done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \))
    
    C_COUNT=${#C_FILES_TO_CHECK[@]}
    print_message "Running cppcheck on $C_COUNT files..."
    
    CPPCHECK_OUTPUT=$(run_cppcheck ".")
    
    # Check for expected vs unexpected issues
    EXPECTED_WARNING="warning: Possible null pointer dereference: ptr"
    HAS_EXPECTED=0
    OTHER_ISSUES=0
    
    while IFS= read -r line; do
        if [[ "$line" =~ ^🛈 ]]; then
            continue
        elif [[ "$line" == *"$EXPECTED_WARNING"* ]]; then
            HAS_EXPECTED=1
        elif [[ "$line" =~ /.*: ]]; then
            ((OTHER_ISSUES++))
        fi
    done <<< "$CPPCHECK_OUTPUT"
    
    # Display output
    if [ -n "$CPPCHECK_OUTPUT" ]; then
        print_message "cppcheck output:"
        echo "$CPPCHECK_OUTPUT" | while IFS= read -r line; do
            print_output "$line"
        done
    fi
    
    if [ $OTHER_ISSUES -gt 0 ]; then
        print_result 1 "Found $OTHER_ISSUES unexpected issues in $C_COUNT files"
        EXIT_CODE=1
    else
        if [ $HAS_EXPECTED -eq 1 ]; then
            print_result 0 "Found 1 expected warning in $C_COUNT files"
        else
            print_result 0 "No issues found in $C_COUNT files"
        fi
        ((PASS_COUNT++))
    fi
else
    print_result 0 "cppcheck not available (skipped)"
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
