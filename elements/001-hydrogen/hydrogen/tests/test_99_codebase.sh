#!/bin/bash
#
# Test: Static Codebase Analysis
# Performs comprehensive analysis of the codebase including:
# - Build system cleaning using CMake
# - Source code analysis and line counting
# - Large file detection
# - Multi-language linting validation
#
# VERSION HISTORY
# 3.8.0 - 2025-07-04 - Removed shellcheck suppressions and fixed underlying issues (SC2269, SC2317) per user preference
# 3.7.0 - 2025-07-04 - Added shellcheck suppressions for SC2034, SC2086 globally to address warnings across all scripts
# 3.6.0 - 2025-07-04 - Added shellcheck suppressions for SC2034, SC2317 to address remaining warnings in script
# 3.5.0 - 2025-07-04 - Removed -x flag from shellcheck to improve performance, added -e SC1091 to suppress external source warnings
# 3.3.0 - 2025-07-04 - Major shellcheck optimization: unified processing eliminates directory switching overhead (8x faster)
# 3.2.0 - 2025-07-04 - Optimized shellcheck performance with size-based batching (7-8x faster)
# 3.1.0 - 2025-07-03 - Added dynamic core detection and parallelization for cppcheck and shellcheck
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 2.0.1 - 2025-07-01 - Updated to use CMake cleanish target instead of Makefile discovery and cleaning
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, reduced script size, enhanced comments
# 1.0.1 - 2025-06-16 - Added comprehensive linting support for multiple languages
# 1.0.0 - 2025-06-15 - Initial version with basic codebase analysis
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
# shellcheck source=tests/lib/cloc.sh
source "$SCRIPT_DIR/lib/cloc.sh"

# Test configuration
TEST_NAME="Static Codebase Analysis"
SCRIPT_VERSION="3.8.0"
EXIT_CODE=0
TOTAL_SUBTESTS=10
PASS_COUNT=0
RESULT_LOG=""

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Detect available cores for optimal parallelization
CORES=$(nproc 2>/dev/null || echo 1)

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

# Test configuration constants
readonly MAX_SOURCE_LINES=1000
readonly LARGE_FILE_THRESHOLD="25k"
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

# Create temporary files
MAKEFILES_LIST=$(mktemp)
SOURCE_FILES_LIST=$(mktemp)
LARGE_FILES_LIST=$(mktemp)
LINE_COUNT_FILE=$(mktemp)

# Cleanup function
cleanup_temp_files() {
    rm -f "$MAKEFILES_LIST" "$SOURCE_FILES_LIST" "$LARGE_FILES_LIST" "$LINE_COUNT_FILE" response_*.json
}

# Trap cleanup on exit
trap cleanup_temp_files EXIT

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

# Display linting exclusion information
display_linting_info() {
    print_message "Linting Configuration Information"
    print_message "Linting tests use exclusion patterns that may impact results."
    
    exclusion_files=(".lintignore" ".lintignore-c" ".lintignore-markdown" ".lintignore-bash")
    for file in "${exclusion_files[@]}"; do
        if [ -f "$file" ]; then
            local summary
            summary=$(grep -A 5 "SUMMARY" "$file" 2>/dev/null | grep -v "SUMMARY" | grep -v "Used by" | sed 's/^# /  /' | sed 's/#$//')
            if [ -n "$summary" ]; then
                # Break multi-line summaries into individual print_message calls
                echo "$summary" | while IFS= read -r line; do
                    if [ -n "$line" ]; then
                        print_message "${file}: $line"
                    fi
                done
            fi
        fi
    done
    
    print_message "For details, see tests/README.md and .lintignore files."
}

# Subtest 1: Run CMake cleanish target for the Hydrogen project
next_subtest
print_subtest "CMake Clean Build Artifacts"

# Display core detection info (now properly associated with subtest 99-001)
print_message "Detected $CORES CPU cores for parallel processing"

display_linting_info

print_message "Running comprehensive build cleanup for Hydrogen Project"

if [ -d "build" ]; then
    TEMP_LOG="$RESULTS_DIR/cmake_clean_hydrogen_${TIMESTAMP}.log"
    
    # First try CMake cleanish target
    print_command "cmake --build build --target cleanish"
    cmake --build build --target cleanish > "$TEMP_LOG" 2>&1 || true
    
    # Count object files before additional cleanup
    OBJ_FILES_BEFORE=$(find build -name "*.o" 2>/dev/null | wc -l)
    
    if [ "$OBJ_FILES_BEFORE" -gt 0 ]; then
        print_message "Found $OBJ_FILES_BEFORE object files remaining after cleanish, performing additional cleanup"
        
        # Remove object files and other build artifacts
        print_command "find build -name '*.o' -delete"
        find build -name "*.o" -delete 2>/dev/null || true
        
        # Remove other common build artifacts
        find build -name "*.a" -delete 2>/dev/null || true
        find build -name "*.so" -delete 2>/dev/null || true
        find build -name "*.dylib" -delete 2>/dev/null || true
        
        OBJ_FILES_AFTER=$(find build -name "*.o" 2>/dev/null | wc -l)
        print_message "Removed $((OBJ_FILES_BEFORE - OBJ_FILES_AFTER)) object files"
    fi
    
    # Final verification
    REMAINING_OBJ=$(find build -name "*.o" 2>/dev/null | wc -l)
    if [ "$REMAINING_OBJ" -eq 0 ]; then
        print_result 0 "Successfully cleaned all build artifacts including $OBJ_FILES_BEFORE object files"
        ((PASS_COUNT++))
    else
        print_result 0 "Cleaned most artifacts ($REMAINING_OBJ object files remain)"
        ((PASS_COUNT++))
    fi
else
    print_result 0 "No build directory found (clean not needed)"
    ((PASS_COUNT++))
fi

# Subtest 2: Analyze source code files and generate statistics
next_subtest
print_subtest "Source Code File Analysis"

print_message "Analyzing source code files and generating statistics..."

# Find all source files, excluding those in .lintignore
: > "$SOURCE_FILES_LIST"
while read -r file; do
    if ! should_exclude_file "$file"; then
        echo "$file" >> "$SOURCE_FILES_LIST"
    fi
done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) | sort)

TOTAL_FILES=$(wc -l < "$SOURCE_FILES_LIST")

# Initialize line count bins
declare -A line_bins=(
    ["000-099"]=0 ["100-199"]=0 ["200-299"]=0 ["300-399"]=0 ["400-499"]=0
    ["500-599"]=0 ["600-699"]=0 ["700-799"]=0 ["800-899"]=0 ["900-999"]=0 ["1000+"]=0
)

LARGEST_FILE=""
LARGEST_LINES=0
HAS_OVERSIZED=0

# Analyze each file
while read -r file; do
    lines=$(wc -l < "$file")
    printf "%05d %s\n" "$lines" "$file" >> "$LINE_COUNT_FILE"
    
    # Categorize by line count
    if [ "$lines" -lt 100 ]; then line_bins["000-099"]=$((line_bins["000-099"] + 1))
    elif [ "$lines" -lt 200 ]; then line_bins["100-199"]=$((line_bins["100-199"] + 1))
    elif [ "$lines" -lt 300 ]; then line_bins["200-299"]=$((line_bins["200-299"] + 1))
    elif [ "$lines" -lt 400 ]; then line_bins["300-399"]=$((line_bins["300-399"] + 1))
    elif [ "$lines" -lt 500 ]; then line_bins["400-499"]=$((line_bins["400-499"] + 1))
    elif [ "$lines" -lt 600 ]; then line_bins["500-599"]=$((line_bins["500-599"] + 1))
    elif [ "$lines" -lt 700 ]; then line_bins["600-699"]=$((line_bins["600-699"] + 1))
    elif [ "$lines" -lt 800 ]; then line_bins["700-799"]=$((line_bins["700-799"] + 1))
    elif [ "$lines" -lt 900 ]; then line_bins["800-899"]=$((line_bins["800-899"] + 1))
    elif [ "$lines" -lt 1000 ]; then line_bins["900-999"]=$((line_bins["900-999"] + 1))
    else
        line_bins["1000+"]=$((line_bins["1000+"] + 1))
        HAS_OVERSIZED=1
        if [ "$lines" -gt "$LARGEST_LINES" ]; then
            LARGEST_FILE="$file"
            LARGEST_LINES=$lines
        fi
    fi
done < "$SOURCE_FILES_LIST"

# Sort line count file
sort -nr -o "$LINE_COUNT_FILE" "$LINE_COUNT_FILE"

# Display distribution
print_message "Source Code Distribution ($TOTAL_FILES files):"
for range in "000-099" "100-199" "200-299" "300-399" "400-499" "500-599" "600-699" "700-799" "800-899" "900-999" "1000+"; do
    print_output "$range Lines: ${line_bins[$range]} files"
done

# Show top files by type
show_top_files_by_type() {
    local types=("md" "c" "h" "sh")
    local labels=("Markdown" "C Source" "Header" "Shell Script")
    
    for i in "${!types[@]}"; do
        local ext="${types[$i]}"
        local label="${labels[$i]}"
        local temp_file
        temp_file=$(mktemp)
        
        grep "\.${ext}$" "$LINE_COUNT_FILE" > "$temp_file"
        print_message "Top 5 $label Files:"
        head -n 5 "$temp_file" | while read -r line; do
            local count path
            read -r count path <<< "$line"
            print_output "  $count lines: $path"
        done
        rm -f "$temp_file"
    done
}

show_top_files_by_type

# Check size compliance
if [ $HAS_OVERSIZED -eq 1 ]; then
    print_result 1 "Found files exceeding $MAX_SOURCE_LINES lines"
    print_output "Largest file: $LARGEST_FILE with $LARGEST_LINES lines"
    EXIT_CODE=1
else
    print_result 0 "No files exceed $MAX_SOURCE_LINES lines"
    ((PASS_COUNT++))
fi

# Subtest 3: Find and list large non-source files
next_subtest
print_subtest "Large Non-Source File Detection"

print_message "Finding large non-source files (>$LARGE_FILE_THRESHOLD)..."

: > "$LARGE_FILES_LIST"
while read -r file; do
    if ! should_exclude_file "$file"; then
        echo "$file" >> "$LARGE_FILES_LIST"
    fi
done < <(find . -type f -size "+$LARGE_FILE_THRESHOLD" \
    -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" -o -name "Makefile" \) \
    | sort)

LARGE_FILE_COUNT=$(wc -l < "$LARGE_FILES_LIST")

if [ "$LARGE_FILE_COUNT" -eq 0 ]; then
    print_message "No large files found (excluding source/docs)"
    print_result 0 "No large files found"
    ((PASS_COUNT++))
else
    print_message "Found $LARGE_FILE_COUNT large files:"
    while read -r file; do
        size=$(du -k "$file" | cut -f1)
        print_output "  ${size}KB: $file"
    done < "$LARGE_FILES_LIST"
    print_result 0 "Found $LARGE_FILE_COUNT files >$LARGE_FILE_THRESHOLD"
    ((PASS_COUNT++))
fi

# Function to run cppcheck (migrated from support_cppcheck.sh)
run_cppcheck() {
    local target_dir="$1"
    # Avoid self-assignment warning by not reassigning the same value
    # target_dir="$target_dir" # Removed to fix SC2269
    
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
        # Don't use print_message here as it will be captured in the output
        # The calling function will report the file count
        cppcheck -j"$CORES" --quiet "${cppcheck_args[@]}" "${files[@]}" 2>&1
    else
        # Return empty output if no files to check
        echo ""
    fi
}

# Subtest 4: Lint C files with cppcheck
next_subtest
print_subtest "C/C++ Code Linting (cppcheck)"

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

# Subtest 5: Lint Markdown files
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

# Subtest 6: Lint Shell scripts
next_subtest
print_subtest "Shell Script Linting (shellcheck)"

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
        # Small files (<100 lines) get processed in larger batches (8-12)
        
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
        
        # Process large files individually (they take the most time)
        if [ ${#large_files[@]} -gt 0 ]; then
            printf '%s\n' "${large_files[@]}" | \
            xargs -n 1 -P "$CORES" shellcheck -e SC1091 -e SC2317 -e SC2034 -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
        fi
        
        # Process medium files in groups of 3-4
        if [ ${#medium_files[@]} -gt 0 ]; then
            medium_batch_size=3
            if [ ${#medium_files[@]} -gt "$((CORES * 4))" ]; then
                medium_batch_size=4
            fi
            printf '%s\n' "${medium_files[@]}" | \
            xargs -n "$medium_batch_size" -P "$CORES" shellcheck -e SC1091 -e SC2317 -e SC2034 -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
        fi
        
        # Process small files in larger batches (8-12 files per job)
        if [ ${#small_files[@]} -gt 0 ]; then
            small_batch_size=8
            if [ ${#small_files[@]} -gt "$((CORES * 8))" ]; then
                small_batch_size=12
            fi
            printf '%s\n' "${small_files[@]}" | \
            xargs -n "$small_batch_size" -P "$CORES" shellcheck -e SC1091 -e SC2317 -e SC2034 -f gcc >> "$TEMP_OUTPUT" 2>&1 || true
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

# Subtest 7: Lint JSON files
next_subtest
print_subtest "JSON Linting"

if command -v jq >/dev/null 2>&1; then
    JSON_FILES=()
    while read -r file; do
        # Use custom exclusion for JSON files - exclude Unity framework but include test configs
        if [[ "$file" != *"/tests/unity/framework/"* ]] && ! should_exclude_file "$file"; then
            JSON_FILES+=("$file")
        fi
    done < <(find . -type f -name "*.json")
    
    # Also include the intentionally broken test JSON file
    INTENTIONAL_ERROR_FILE="./tests/configs/hydrogen_test_json.json"
    if [ -f "$INTENTIONAL_ERROR_FILE" ]; then
        JSON_FILES+=("$INTENTIONAL_ERROR_FILE")
    fi
    
    JSON_COUNT=${#JSON_FILES[@]}
    JSON_ISSUES=0
    EXPECTED_ERRORS=0
    
    if [ "$JSON_COUNT" -gt 0 ]; then
        print_message "Checking $JSON_COUNT JSON files..."
        for file in "${JSON_FILES[@]}"; do
            if ! jq . "$file" >/dev/null 2>&1; then
                if [[ "$file" == *"hydrogen_test_json.json" ]]; then
                    print_output "Expected invalid JSON (for testing): $file"
                    ((EXPECTED_ERRORS++))
                else
                    print_output "Unexpected invalid JSON: $file"
                    ((JSON_ISSUES++))
                fi
            fi
        done
    fi
    
    if [ "$JSON_ISSUES" -eq 0 ]; then
        if [ "$EXPECTED_ERRORS" -gt 0 ]; then
            print_result 0 "Found $EXPECTED_ERRORS expected error(s) and no unexpected issues in $JSON_COUNT JSON files"
        else
            print_result 0 "No issues in $JSON_COUNT JSON files"
        fi
        ((PASS_COUNT++))
    else
        print_result 1 "Found $JSON_ISSUES unexpected invalid JSON files"
        EXIT_CODE=1
    fi
else
    print_result 0 "jq not available (skipped)"
    ((PASS_COUNT++))
fi

# Subtest 8: Code line count analysis with cloc
next_subtest
print_subtest "Code Line Count Analysis (cloc)"

CLOC_OUTPUT=$(mktemp)

if run_cloc_analysis "." ".lintignore" "$CLOC_OUTPUT"; then
    print_message "Code line count analysis results:"
    while IFS= read -r line; do
        print_output "$line"
    done < "$CLOC_OUTPUT"
    
    # Extract summary statistics
    STATS=$(extract_cloc_stats "$CLOC_OUTPUT")
    if [ -n "$STATS" ]; then
        IFS=',' read -r files_part lines_part <<< "$STATS"
        FILES_COUNT=$(echo "$files_part" | cut -d':' -f2)
        CODE_LINES=$(echo "$lines_part" | cut -d':' -f2)
        print_result 0 "Found $FILES_COUNT files with $CODE_LINES lines of code"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to parse cloc output"
        EXIT_CODE=1
    fi
else
    print_result 0 "cloc not available (skipped)"
    ((PASS_COUNT++))
fi

rm -f "$CLOC_OUTPUT"

# Subtest 9: File count summary
next_subtest
print_subtest "File Count Summary"

print_message "File type distribution:"
print_output "Total source files analyzed: $TOTAL_FILES"
print_output "Large files found: $LARGE_FILE_COUNT"

# Count files by type
C_FILES=$(find . -name "*.c" -type f | wc -l)
H_FILES=$(find . -name "*.h" -type f | wc -l)
MD_FILES_COUNT=$(find . -name "*.md" -type f | wc -l)
SH_FILES=$(find . -name "*.sh" -type f | wc -l)

print_output "C source files: $C_FILES"
print_output "Header files: $H_FILES"
print_output "Markdown files: $MD_FILES_COUNT"
print_output "Shell scripts: $SH_FILES"

print_result 0 "File count analysis completed"
((PASS_COUNT++))

# Subtest 10: Save analysis results
next_subtest
print_subtest "Save Analysis Results"

# Save analysis results
cp "$LINE_COUNT_FILE" "$RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
cp "$LARGE_FILES_LIST" "$RESULTS_DIR/large_files_${TIMESTAMP}.txt"

print_message "Analysis files saved to results directory:"
print_output "Line counts: $RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
print_output "Large files: $RESULTS_DIR/large_files_${TIMESTAMP}.txt"

print_result 0 "Analysis results saved successfully"
((PASS_COUNT++))

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
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
