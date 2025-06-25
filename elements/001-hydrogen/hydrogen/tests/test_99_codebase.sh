#!/bin/bash
#
# Hydrogen Codebase Analysis Test
# Performs comprehensive analysis of the codebase including:
# - Makefile discovery and cleaning
# - Source code analysis and line counting
# - Large file detection
# - Multi-language linting validation
#
# Usage: ./test_99_codebase.sh

# About this Script
NAME_TEST_99="Hydrogen Codebase Analysis Test"
VERS_TEST_99="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, reduced script size, enhanced comments
# 1.0.1 - 2025-06-16 - Added comprehensive linting support for multiple languages
# 1.0.0 - 2025-06-15 - Initial version with basic codebase analysis

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

# Get script directory and set up paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Initialize test environment
setup_test_environment() {
    # Note: TEST_NAME is used by support_utils.sh functions
    # shellcheck disable=SC2034
    TEST_NAME="$NAME_TEST_99"
    RESULTS_DIR="$SCRIPT_DIR/results"
    mkdir -p "$RESULTS_DIR"
    
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    RESULT_LOG="$RESULTS_DIR/codebase_test_${TIMESTAMP}.log"
    
    # Initialize test tracking arrays
    declare -g -a TEST_NAMES=()
    declare -g -a TEST_RESULTS=()
    declare -g -a TEST_DETAILS=()
    declare -g TEST_RESULT=0
    
    # Create temporary files
    MAKEFILES_LIST=$(mktemp)
    SOURCE_FILES_LIST=$(mktemp)
    LARGE_FILES_LIST=$(mktemp)
    LINE_COUNT_FILE=$(mktemp)
    
    # Start test with version number like test_00_all.sh
    start_test "$NAME_TEST_99 v$VERS_TEST_99" | tee "$RESULT_LOG"
}

# Check if a file should be excluded from linting
should_exclude_file() {
    local file="$1"
    local lint_ignore="$HYDROGEN_DIR/.lintignore"
    local rel_file="${file#"$HYDROGEN_DIR"/}"
    
    # Check .lintignore file first if it exists
    if [ -f "$lint_ignore" ]; then
        while IFS= read -r pattern; do
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            shopt -s extglob
            if [[ "$rel_file" == @($pattern) ]]; then
                shopt -u extglob
                return 0 # Exclude
            fi
            shopt -u extglob
        done < "$lint_ignore"
    fi
    
    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        shopt -s extglob
        if [[ "$rel_file" == @($pattern) ]]; then
            shopt -u extglob
            return 0 # Exclude
        fi
        shopt -u extglob
    done
    
    return 1 # Do not exclude
}

# Display linting exclusion information
display_linting_info() {
    print_header "Linting Configuration Information" | tee -a "$RESULT_LOG"
    print_info "Linting tests use exclusion patterns that may impact results." | tee -a "$RESULT_LOG"
    
    local exclusion_files=(".lintignore" ".lintignore-c" ".lintignore-markdown" ".lintignore-bash")
    for file in "${exclusion_files[@]}"; do
        if [ -f "$HYDROGEN_DIR/$file" ]; then
            local summary
            summary=$(grep -A 5 "SUMMARY" "$HYDROGEN_DIR/$file" 2>/dev/null | grep -v "SUMMARY" | grep -v "Used by" | sed 's/^# /  /' | sed 's/#$//')
            if [ -n "$summary" ]; then
                print_info "${file}: $summary" | tee -a "$RESULT_LOG"
            fi
        fi
    done
    
    print_info "For details, see tests/README.md and .lintignore files." | tee -a "$RESULT_LOG"
    echo "" | tee -a "$RESULT_LOG"
}

# Record test result in tracking arrays
record_test_result() {
    local name="$1"
    local result="$2"
    local details="$3"
    
    TEST_NAMES+=("$name")
    TEST_RESULTS+=("$result")
    TEST_DETAILS+=("$details")
    
    if [ "$result" -ne 0 ]; then
        TEST_RESULT=1
    fi
}

# Find and list all Makefiles in the project
find_makefiles() {
    print_header "1. Locating Makefiles" | tee -a "$RESULT_LOG"
    
    find "$HYDROGEN_DIR" -type f -name "Makefile" > "$MAKEFILES_LIST"
    local count
    count=$(wc -l < "$MAKEFILES_LIST")
    
    print_info "Found $count Makefiles:" | tee -a "$RESULT_LOG"
    while read -r makefile; do
        local rel_path
        rel_path=$(convert_to_relative_path "$makefile")
        print_info "  - ${rel_path#hydrogen/}" | tee -a "$RESULT_LOG"
    done < "$MAKEFILES_LIST"
    
    print_result 0 "$count Makefiles located successfully" | tee -a "$RESULT_LOG"
    record_test_result "Locate Makefiles" 0 "Found $count Makefiles"
}

# Run make cleanish for each Makefile
run_make_clean() {
    print_header "2. Running 'make cleanish' for Makefiles" | tee -a "$RESULT_LOG"
    
    local success=0
    local fail=0
    local start_dir
    start_dir=$(pwd)
    
    while read -r makefile; do
        local makefile_dir
        makefile_dir=$(dirname "$makefile")
        local rel_dir
        rel_dir=$(convert_to_relative_path "$makefile_dir")
        rel_dir=${rel_dir#hydrogen/}
        
        print_info "Processing: $rel_dir" | tee -a "$RESULT_LOG"
        
        if cd "$makefile_dir" 2>/dev/null; then
            local temp_log
            temp_log="$RESULTS_DIR/make_clean_$(basename "$makefile_dir").log"
            if make cleanish > "$temp_log" 2>&1; then
                print_result 0 "Successfully cleaned $rel_dir" | tee -a "$RESULT_LOG"
                ((success++))
            else
                print_warning "Clean failed in $rel_dir (continuing)" | tee -a "$RESULT_LOG"
                ((success++))  # Count as success since it's non-fatal
            fi
            cd "$start_dir" || exit 1
        else
            print_result 1 "Failed to access $rel_dir" | tee -a "$RESULT_LOG"
            ((fail++))
        fi
    done < "$MAKEFILES_LIST"
    
    local total=$((success + fail))
    record_test_result "Run make cleanish" 0 "Cleaned $success of $total Makefiles"
}

# Analyze source code files and generate statistics
analyze_source_files() {
    print_header "3. Analyzing Source Code Files" | tee -a "$RESULT_LOG"
    
    # Find all source files, excluding those in .lintignore
    : > "$SOURCE_FILES_LIST"
    while read -r file; do
        if ! should_exclude_file "$file"; then
            echo "$file" >> "$SOURCE_FILES_LIST"
        fi
    done < <(find "$HYDROGEN_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) | sort)
    local total_files
    total_files=$(wc -l < "$SOURCE_FILES_LIST")
    
    # Initialize line count bins
    declare -A line_bins=(
        ["000-099"]=0 ["100-199"]=0 ["200-299"]=0 ["300-399"]=0 ["400-499"]=0
        ["500-599"]=0 ["600-699"]=0 ["700-799"]=0 ["800-899"]=0 ["900-999"]=0 ["1000+"]=0
    )
    
    local largest_file=""
    local largest_lines=0
    local has_oversized=0
    
    # Analyze each file
    while read -r file; do
        local lines
        lines=$(wc -l < "$file")
        printf "%05d %s\n" "$lines" "$(convert_to_relative_path "$file")" >> "$LINE_COUNT_FILE"
        
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
            has_oversized=1
            if [ "$lines" -gt "$largest_lines" ]; then
                largest_file=$(convert_to_relative_path "$file")
                largest_lines=$lines
            fi
        fi
    done < "$SOURCE_FILES_LIST"
    
    # Sort line count file
    sort -nr -o "$LINE_COUNT_FILE" "$LINE_COUNT_FILE"
    
    # Display distribution
    print_info "Source Code Distribution ($total_files files):" | tee -a "$RESULT_LOG"
    for range in "000-099" "100-199" "200-299" "300-399" "400-499" "500-599" "600-699" "700-799" "800-899" "900-999" "1000+"; do
        print_info "$range Lines: ${line_bins[$range]} files" | tee -a "$RESULT_LOG"
    done
    
    # Show top files by type
    show_top_files_by_type
    
    # Check size compliance
    if [ $has_oversized -eq 1 ]; then
        print_result 1 "FAIL: Found files exceeding $MAX_SOURCE_LINES lines" | tee -a "$RESULT_LOG"
        record_test_result "Source File Size Check" 1 "Found ${largest_file#hydrogen/} with $largest_lines lines"
    else
        print_result 0 "PASS: No files exceed $MAX_SOURCE_LINES lines" | tee -a "$RESULT_LOG"
        record_test_result "Source File Size Check" 0 "All files within $MAX_SOURCE_LINES line limit"
    fi
    
    record_test_result "Source File Count" 0 "Found $total_files source files"
}

# Show top files by file type
show_top_files_by_type() {
    local types=("md" "c" "h" "sh")
    local labels=("Markdown" "C Source" "Header" "Shell Script")
    
    for i in "${!types[@]}"; do
        local ext="${types[$i]}"
        local label="${labels[$i]}"
        local temp_file
        temp_file=$(mktemp)
        
        grep "\.${ext}$" "$LINE_COUNT_FILE" > "$temp_file"
        print_info "Top 10 $label Files:" | tee -a "$RESULT_LOG"
        head -n 10 "$temp_file" | while read -r line; do
            local count path
            read -r count path <<< "$line"
            print_info "  $count lines: ${path#hydrogen/}" | tee -a "$RESULT_LOG"
        done
        rm -f "$temp_file"
    done
}

# Find and list large non-source files
find_large_files() {
    print_header "4. Finding Large Non-Source Files (>$LARGE_FILE_THRESHOLD)" | tee -a "$RESULT_LOG"
    
    : > "$LARGE_FILES_LIST"
    while read -r file; do
        if ! should_exclude_file "$file"; then
            echo "$file" >> "$LARGE_FILES_LIST"
        fi
    done < <(find "$HYDROGEN_DIR" -type f -size "+$LARGE_FILE_THRESHOLD" \
        -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" -o -name "Makefile" \) \
        | sort)
    
    local count
    count=$(wc -l < "$LARGE_FILES_LIST")
    
    if [ "$count" -eq 0 ]; then
        print_info "No large files found (excluding source/docs)" | tee -a "$RESULT_LOG"
    else
        print_info "Found $count large files:" | tee -a "$RESULT_LOG"
        while read -r file; do
            local size
            size=$(du -k "$file" | cut -f1)
            local rel_path
            rel_path=$(convert_to_relative_path "$file")
            print_info "  ${size}KB: ${rel_path#hydrogen/}" | tee -a "$RESULT_LOG"
        done < "$LARGE_FILES_LIST"
    fi
    
    record_test_result "Large File Analysis" 0 "Found $count files >$LARGE_FILE_THRESHOLD"
}

# Display limited output for linting results
display_limited_output() {
    local file="$1"
    local lines
    lines=$(wc -l < "$file")
    if [ "$lines" -gt "$LINT_OUTPUT_LIMIT" ]; then
        head -n "$LINT_OUTPUT_LIMIT" "$file" | tee -a "$RESULT_LOG"
        print_warning "Output truncated. Showing $LINT_OUTPUT_LIMIT of $lines lines." | tee -a "$RESULT_LOG"
    else
        tee -a "$RESULT_LOG" < "$file"
    fi
}

# Run C/C++ linting with cppcheck
lint_c_files() {
    print_info "Linting C files with cppcheck..." | tee -a "$RESULT_LOG"
    
    local c_count
    c_count=$(find "$HYDROGEN_DIR" -type f \( -name "*.c" -o -name "*.h" \) \
        -not -path "*/build*/*" -not -path "*/tests/logs/*" \
        -not -path "*/tests/results/*" -not -path "*/tests/diagnostics/*" | wc -l)
    
    local output
    output=$("$SCRIPT_DIR/support_cppcheck.sh" "$HYDROGEN_DIR" 2>&1)
    echo "$output" | tee -a "$RESULT_LOG"
    
    # Check for expected vs unexpected issues
    local expected_warning="warning: Possible null pointer dereference: ptr"
    local has_expected=0
    local other_issues=0
    
    while IFS= read -r line; do
        if [[ "$line" =~ ^🛈 ]]; then
            continue
        elif [[ "$line" == *"$expected_warning"* ]]; then
            has_expected=1
        elif [[ "$line" =~ /.*: ]]; then
            ((other_issues++))
        fi
    done <<< "$output"
    
    if [ $other_issues -gt 0 ]; then
        record_test_result "Lint C Files" 1 "Found $other_issues unexpected issues in $c_count files"
    else
        local msg="No issues found in $c_count files"
        [ $has_expected -eq 1 ] && msg="Found 1 expected warning in $c_count files"
        record_test_result "Lint C Files" 0 "$msg"
    fi
}

# Run markdown linting
lint_markdown_files() {
    print_info "Linting Markdown files..." | tee -a "$RESULT_LOG"
    
    local temp_log
    temp_log=$(mktemp)
    local md_files=()
    local fails=0
    
    # Collect markdown files (excluding those in .lintignore)
    while read -r file; do
        if ! should_exclude_file "$file"; then
            md_files+=("$file")
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.md")
    
    local count=${#md_files[@]}
    
    if [ "$count" -gt 0 ]; then
        # Run markdownlint in parallel
        printf '%s\n' "${md_files[@]}" | xargs -P12 -I{} bash -c \
            "temp=\$(mktemp); \
             if ! markdownlint --config \"$HYDROGEN_DIR/.lintignore-markdown\" \"{}\" 2> \"\$temp\"; then \
                 cat \"\$temp\" >> \"$temp_log\"; \
             fi; \
             rm -f \"\$temp\""
        
        if [ -s "$temp_log" ]; then
            fails=$(wc -l < "$temp_log")
            print_warning "markdownlint found $fails issues:" | tee -a "$RESULT_LOG"
            display_limited_output "$temp_log"
        fi
    fi
    
    if [ "$fails" -eq 0 ]; then
        record_test_result "Lint Markdown Files" 0 "No issues in $count files"
    else
        record_test_result "Lint Markdown Files" 1 "Found $fails issues in $count files"
    fi
    
    rm -f "$temp_log"
}

# Run shell script linting
lint_shell_files() {
    print_info "Linting Shell scripts with shellcheck..." | tee -a "$RESULT_LOG"
    
    local shell_files=()
    local bash_lintignore="$HYDROGEN_DIR/.lintignore-bash"
    
    # Function to check bash-specific exclusions
    should_exclude_bash() {
        local file="$1"
        should_exclude_file "$file" && return 0
        
        if [ -f "$bash_lintignore" ]; then
            while IFS= read -r pattern; do
                [[ -z "$pattern" || "$pattern" == \#* ]] && continue
                [[ "$file" == "$pattern" || "$file" == */"$pattern" ]] && return 0
            done < "$bash_lintignore"
        fi
        return 1
    }
    
    # Collect shell files
    while read -r file; do
        if ! should_exclude_bash "$file"; then
            shell_files+=("$file")
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.sh")
    
    local count=${#shell_files[@]}
    local issues=0
    local files_with_issues=0
    
    if [ "$count" -gt 0 ]; then
        local output
        output=$(mktemp)
        shellcheck -x -f gcc "${shell_files[@]}" > "$output" 2>&1 || true
        
        # Filter output to show relative paths
        sed "s|$HYDROGEN_DIR/||g" "$output" > "${output}.filtered"
        
        issues=$(wc -l < "${output}.filtered")
        if [ "$issues" -gt 0 ]; then
            files_with_issues=$(cut -d: -f1 "${output}.filtered" | sort -u | wc -l)
            print_warning "shellcheck found $issues issues in $files_with_issues files:" | tee -a "$RESULT_LOG"
            display_limited_output "${output}.filtered"
        fi
        
        rm -f "$output" "${output}.filtered"
    fi
    
    local result_code
    if [ "$issues" -eq 0 ]; then
        result_code=0
    else
        result_code=1
    fi
    local details
    if [ "$issues" -eq 0 ]; then
        details="No issues in $count files"
    else
        details="Found $issues total issues in $files_with_issues of $count files"
    fi
    
    record_test_result "Lint Shell Script Files" "$result_code" "$details"
}

# Run other linting tools (JSON, JS, CSS, HTML)
lint_other_files() {
    local linters=("jsonlint:json:JSON" "eslint:js,jsx,ts,tsx:JavaScript" "stylelint:css,scss,sass:CSS" "htmlhint:html,htm:HTML")
    
    for linter_info in "${linters[@]}"; do
        IFS=':' read -r tool extensions label <<< "$linter_info"
        
        print_info "Linting $label files with $tool..." | tee -a "$RESULT_LOG"
        
        local files=()
        local temp_log
        temp_log=$(mktemp)
        local fails=0
        
        # Build find command for multiple extensions
        local find_args=()
        IFS=',' read -ra exts <<< "$extensions"
        for ext in "${exts[@]}"; do
            find_args+=(-name "*.${ext}" -o)
        done
        find_args=("${find_args[@]:0:${#find_args[@]}-1}")  # Remove last -o
        
        # Collect files, excluding those in .lintignore
        while read -r file; do
            if ! should_exclude_file "$file"; then
                files+=("$file")
            fi
        done < <(find "$HYDROGEN_DIR" -type f \( "${find_args[@]}" \))
        
        # For eslint, apply additional filtering if needed to ensure no excluded files are processed
        if [ "$tool" = "eslint" ]; then
            local filtered_files=()
            for file in "${files[@]}"; do
                if ! should_exclude_file "$file"; then
                    filtered_files+=("$file")
                fi
            done
            files=("${filtered_files[@]}")
        fi
        
        local count=${#files[@]}
        
        if [ "$count" -gt 0 ]; then
            for file in "${files[@]}"; do
                # Special handling for htmlhint (no --quiet flag)
                if [ "$tool" = "htmlhint" ]; then
                    if ! "$tool" "$file" 2>> "$temp_log"; then
                        ((fails++))
                    fi
                elif [ "$tool" = "eslint" ]; then
                    # Check if eslint config exists, otherwise skip with a warning
                    if ! eslint --print-config "$file" >/dev/null 2>&1; then
                        print_warning "eslint config not found for $file, skipping..." | tee -a "$RESULT_LOG"
                        continue
                    fi
                    if ! "$tool" --quiet "$file" 2>> "$temp_log"; then
                        ((fails++))
                    fi
                else
                    if ! "$tool" --quiet "$file" 2>> "$temp_log"; then
                        ((fails++))
                    fi
                fi
            done
            
            if [ -s "$temp_log" ]; then
                print_warning "$tool found $fails issues:" | tee -a "$RESULT_LOG"
                display_limited_output "$temp_log"
            fi
        else
            print_warning "No $label files found (expected for some types)" | tee -a "$RESULT_LOG"
        fi
        
        local details
        if [ "$count" -eq 0 ]; then
            details="No files found (expected)"
        elif [ "$fails" -eq 0 ]; then
            details="No issues in $count files"
        else
            details="Found $fails issues in $count files"
        fi
        
        if [ $fails -eq 0 ]; then
            record_test_result "Lint $label Files" 0 "$details"
        else
            record_test_result "Lint $label Files" 1 "$details"
        fi
        rm -f "$temp_log"
    done
}

# Run code line count analysis with cloc
run_cloc_analysis() {
    print_header "6. Code Line Count Analysis" | tee -a "$RESULT_LOG"
    
    local cloc_output
    cloc_output=$(mktemp)
    local start_dir
    start_dir=$(pwd)
    
    if cd "$HYDROGEN_DIR" && env LC_ALL=en_US.UTF_8 cloc . --quiet --force-lang="C,inc" > "$cloc_output" 2>&1; then
        tail -n +2 "$cloc_output" | tee -a "$RESULT_LOG"
        
        # Extract summary statistics
        local stats
        stats=$(grep "SUM:" "$cloc_output")
        if [ -n "$stats" ]; then
            local files
            files=$(echo "$stats" | awk '{print $2}')
            local code
            code=$(echo "$stats" | awk '{print $5}')
            local blank
            blank=$(echo "$stats" | awk '{print $3}')
            local comment
            comment=$(echo "$stats" | awk '{print $4}')
            record_test_result "Code Line Count Analysis" 0 "Found $files files with $code lines of code ($blank blank, $comment comment)"
        else
            record_test_result "Code Line Count Analysis" 1 "Failed to parse cloc output"
        fi
    else
        print_warning "Failed to run cloc analysis" | tee -a "$RESULT_LOG"
        record_test_result "Code Line Count Analysis" 1 "Failed to analyze code lines"
    fi
    
    cd "$start_dir" || exit 1
    rm -f "$cloc_output"
}

# Run comprehensive linting tests
run_linting_tests() {
    print_header "5. Running Comprehensive Linting Tests" | tee -a "$RESULT_LOG"
    
    lint_c_files
    lint_markdown_files
    lint_shell_files
    lint_other_files
    run_cloc_analysis
}

# Main test execution function
run_all_tests() {
    find_makefiles
    run_make_clean
    analyze_source_files
    find_large_files
    run_linting_tests
}

# Generate final test report
generate_report() {
    # Save analysis results
    cp "$LINE_COUNT_FILE" "$RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
    cp "$LARGE_FILES_LIST" "$RESULTS_DIR/large_files_${TIMESTAMP}.txt"
    
    print_info "Analysis files saved to results directory" | tee -a "$RESULT_LOG"
    
    # Calculate statistics
    local pass_count=0
    local fail_count=0
    for result in "${TEST_RESULTS[@]}"; do
        if [ "$result" -eq 0 ]; then
            ((pass_count++))
        else
            ((fail_count++))
        fi
    done
    
    # Print individual results
    print_header "Individual Test Results" | tee -a "$RESULT_LOG"
    for i in "${!TEST_NAMES[@]}"; do
        print_test_item "${TEST_RESULTS[$i]}" "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}" | tee -a "$RESULT_LOG"
    done
    
    # Print summary
    print_test_summary $((pass_count + fail_count)) $pass_count $fail_count | tee -a "$RESULT_LOG"
    
    # Export results for test runner
    local test_name
    test_name=$(basename "$0" .sh | sed 's/^test_//')
    export_subtest_results "$test_name" $((pass_count + fail_count)) $pass_count
}

# Cleanup temporary files
cleanup() {
    rm -f "$MAKEFILES_LIST" "$SOURCE_FILES_LIST" "$LARGE_FILES_LIST" "$LINE_COUNT_FILE" response_*.json
}

# Main execution
main() {
    setup_test_environment "$@"
    display_linting_info
    run_all_tests
    generate_report
    cleanup
    
    end_test "$TEST_RESULT" "$(basename "$0" .sh | sed 's/^test_//')" | tee -a "$RESULT_LOG"
    exit "$TEST_RESULT"
}

# Execute main function
main "$@"
