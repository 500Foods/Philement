#!/bin/bash

# Test: Shell Script Analysis
# Performs shellcheck analysis and exception justification checking

# CHANGELOG
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for shell script analysis

# Test configuration
TEST_NAME="Shell Script Analysis (shellcheck)"
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
TOTAL_SUBTESTS=2
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Detect available cores for optimal parallelization
CORES=$(nproc 2>/dev/null || echo 1)

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
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

# Test configuration constants
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_OUTPUT_LIMIT:-}" ]]; then
    readonly LINT_OUTPUT_LIMIT=100
fi

# Default exclude patterns for linting (can be overridden by .lintignore)
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_EXCLUDES:-}" ]]; then
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
fi

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

# Subtest 1: Lint Shell scripts
next_subtest
print_subtest "Shell Script Linting (shellcheck)"

print_message "Detected $CORES CPU cores for parallel processing"

if command -v shellcheck >/dev/null 2>&1; then
    SHELL_FILES=()
    TEST_SHELL_FILES=()
    OTHER_SHELL_FILES=()
    
    while read -r file; do
        if ! should_exclude_file "$file"; then
            SHELL_FILES+=("$file")
            # Separate test scripts from other shell scripts
            if [[ "$file" == ./tests/* ]]; then
                TEST_SHELL_FILES+=("$file")
            else
                OTHER_SHELL_FILES+=("$file")
            fi
        fi
    done < <(find . -type f -name "*.sh")
    
    SHELL_COUNT=${#SHELL_FILES[@]}
    SHELL_ISSUES=0
    TEMP_OUTPUT=$(mktemp)
    
    if [ "$SHELL_COUNT" -gt 0 ]; then
        print_message "Running shellcheck on $SHELL_COUNT shell scripts..."
        
        # Optimized unified approach: Process all files together with size-based batching
        # This eliminates directory switching overhead while maintaining optimal load balancing
        # Large files (>400 lines) get processed individually
        # Medium files (100-400 lines) get processed in groups of 3-4  
        # Small files (<100 lines) get processed in larger batches (8-12 files per job)
        
        # Function to get file line count
        get_line_count() {
            wc -l < "$1" 2>/dev/null || echo 0
        }
        
        # Categorize all files by size for optimal batching
        large_files=()
        medium_files=()
        small_files=()
        
        for file in "${SHELL_FILES[@]}"; do
            lines=$(get_line_count "$file")
            if [ "$lines" -gt 400 ]; then
                large_files+=("$file")
            elif [ "$lines" -gt 100 ]; then
                medium_files+=("$file")
            else
                small_files+=("$file")
            fi
        done
        
        # Build shellcheck exclusion arguments from .lintignore-bash file
        SHELLCHECK_EXCLUDES=(-e SC1091 -e SC2317 -e SC2034)  # Default exclusions as array
        if [ -f ".lintignore-bash" ]; then
            while IFS= read -r line; do
                # Skip comments and empty lines
                [[ -z "$line" || "$line" == \#* ]] && continue
                # Add exclusion if it looks like a shellcheck code (SCxxxx)
                if [[ "$line" =~ ^SC[0-9]+$ ]]; then
                    SHELLCHECK_EXCLUDES+=(-e "$line")
                fi
            done < ".lintignore-bash"
        fi
        
        # Process large files individually (they take the most time)
        if [ ${#large_files[@]} -gt 0 ]; then
            printf '%s\n' "${large_files[@]}" | \
            xargs -n 1 -P "$CORES" shellcheck "${SHELLCHECK_EXCLUDES[@]}" -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
        fi
        
        # Process medium files in groups of 3-4
        if [ ${#medium_files[@]} -gt 0 ]; then
            medium_batch_size=3
            if [ ${#medium_files[@]} -gt "$((CORES * 4))" ]; then
                medium_batch_size=4
            fi
            printf '%s\n' "${medium_files[@]}" | \
            xargs -n "$medium_batch_size" -P "$CORES" shellcheck "${SHELLCHECK_EXCLUDES[@]}" -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
        fi
        
        # Process small files in larger batches (8-12 files per job)
        if [ ${#small_files[@]} -gt 0 ]; then
            small_batch_size=8
            if [ ${#small_files[@]} -gt "$((CORES * 8))" ]; then
                small_batch_size=12
            fi
            printf '%s\n' "${small_files[@]}" | \
            xargs -n "$small_batch_size" -P "$CORES" shellcheck "${SHELLCHECK_EXCLUDES[@]}" -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
        fi
        
        # Filter output to show clean relative paths
        sed "s|\"$(pwd)\"/||g; s|tests/||g" "$TEMP_OUTPUT" > "${TEMP_OUTPUT}.filtered"
        
        SHELL_ISSUES="$(wc -l < "${TEMP_OUTPUT}.filtered")"
        if [ "$SHELL_ISSUES" -gt 0 ]; then
            FILES_WITH_ISSUES="$(cut -d: -f1 "${TEMP_OUTPUT}.filtered" | sort -u | wc -l)"
            print_message "shellcheck found $SHELL_ISSUES issues in $FILES_WITH_ISSUES files:"
            head -n "$LINT_OUTPUT_LIMIT" "${TEMP_OUTPUT}.filtered" | while IFS= read -r line; do
                print_output "$line"
            done
            if [ "$SHELL_ISSUES" -gt "$LINT_OUTPUT_LIMIT" ]; then
                print_message "Output truncated. Showing $LINT_OUTPUT_LIMIT of $SHELL_ISSUES lines."
            fi
        fi
        
        rm -f "$TEMP_OUTPUT" "${TEMP_OUTPUT}.filtered"
    fi
    
    if [ "$SHELL_ISSUES" -eq 0 ]; then
        print_result 0 "No issues in $SHELL_COUNT shell files"
        ((PASS_COUNT++))
    else
        print_result 1 "Found $SHELL_ISSUES issues in shell files"
        EXIT_CODE=1
    fi
else
    print_result 0 "shellcheck not available (skipped)"
    ((PASS_COUNT++))
fi

# Subtest 2: Shellcheck Exception Justification Check
next_subtest
print_subtest "Shellcheck Exception Justification Check"

print_message "Checking for shellcheck directives in shell scripts..."

SHELLCHECK_DIRECTIVE_TOTAL=0
SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION=0
SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION=0

# Reuse the SHELL_FILES array from the previous subtest if available
if [ ${#SHELL_FILES[@]} -eq 0 ]; then
    SHELL_FILES=()
    while read -r file; do
        if ! should_exclude_file "$file"; then
            SHELL_FILES+=("$file")
        fi
    done < <(find . -type f -name "*.sh")
fi

SHELL_COUNT=${#SHELL_FILES[@]}

if [ "$SHELL_COUNT" -gt 0 ]; then
    print_message "Analyzing $SHELL_COUNT shell scripts for shellcheck directives..."
    for file in "${SHELL_FILES[@]}"; do
        # Find lines with shellcheck directives (allowing leading whitespace)
        while IFS= read -r line; do
            if [[ "$line" =~ ^[[:space:]]*"# shellcheck" ]]; then
                ((SHELLCHECK_DIRECTIVE_TOTAL++))
                # Check if there's a justification (additional comment after the directive)
                if [[ "$line" =~ ^[[:space:]]*"# shellcheck"[[:space:]]+[^[:space:]]+[[:space:]]+"#".* ]]; then
                    ((SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION++))
                else
                    ((SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION++))
                    print_output "No justification found in $file: $line"
                fi
            fi
        done < <(grep "^[[:space:]]*# shellcheck" "$file")
    done
    
    print_message "INFO: Total shellcheck directives: $SHELLCHECK_DIRECTIVE_TOTAL"
    print_message "INFO: Directives with justification: $SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION"
    print_message "INFO: Directives without justification: $SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION"
    
    if [ "$SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION" -eq 0 ]; then
        print_result 0 "All $SHELLCHECK_DIRECTIVE_TOTAL shellcheck directives have justifications"
        ((PASS_COUNT++))
    else
        print_result 1 "Found $SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION shellcheck directives without justifications"
        EXIT_CODE=1
    fi
else
    print_result 0 "No shell scripts found to check for shellcheck directives"
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
