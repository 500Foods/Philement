#!/bin/bash

# Test: Code Size Analysis
# Performs comprehensive code size analysis including:
# - Source code analysis and line counting
# - Large file detection
# - Code line count analysis with cloc
# - File count summary

# Test configuration
TEST_NAME="Code Size Analysis"
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
# shellcheck source=tests/lib/cloc.sh # Resolve path statically
source "$SCRIPT_DIR/lib/cloc.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=5
PASS_COUNT=0
RESULT_LOG=""

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
BUILD_DIR="$SCRIPT_DIR/../build"
RESULTS_DIR="$BUILD_DIR/tests/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Test configuration constants
readonly MAX_SOURCE_LINES=1000
readonly LARGE_FILE_THRESHOLD="25k"

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

# Create temporary files
SOURCE_FILES_LIST=$(mktemp)
LARGE_FILES_LIST=$(mktemp)
LINE_COUNT_FILE=$(mktemp)

# Cleanup function
cleanup_temp_files() {
    rm -f "$SOURCE_FILES_LIST" "$LARGE_FILES_LIST" "$LINE_COUNT_FILE"
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

# Subtest 1: Linting Configuration Information
next_subtest
print_subtest "Linting Configuration Information"

print_message "Checking linting configuration files and displaying exclusion patterns..."

exclusion_files=(".lintignore" ".lintignore-c" ".lintignore-markdown" ".lintignore-bash")
found_files=0
missing_files=()

for file in "${exclusion_files[@]}"; do
    if [ -f "$file" ]; then
        ((found_files++))
        if [ "$file" = ".lintignore-bash" ]; then
            # Special handling for .lintignore-bash which doesn't have a SUMMARY section
            print_message "${file}: Used by shellcheck for bash script linting exclusions"
        else
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
    else
        missing_files+=("$file")
    fi
done

print_message "For details, see tests/README.md and .lintignore files."

if [ ${#missing_files[@]} -eq 0 ]; then
    print_result 0 "All $found_files linting configuration files found"
    ((PASS_COUNT++))
else
    print_result 1 "Missing linting files: ${missing_files[*]}"
    EXIT_CODE=1
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

# Subtest 4: Code line count analysis with cloc
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

# Subtest 5: File count summary
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

# Save analysis results
cp "$LINE_COUNT_FILE" "$RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
cp "$LARGE_FILES_LIST" "$RESULTS_DIR/large_files_${TIMESTAMP}.txt"

print_message "Analysis files saved to results directory:"
print_output "Line counts: $RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
print_output "Large files: $RESULTS_DIR/large_files_${TIMESTAMP}.txt"

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
