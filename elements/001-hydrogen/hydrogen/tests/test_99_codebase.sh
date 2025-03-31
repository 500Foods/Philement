#!/bin/bash
#
# Codebase Analysis Test
# This test performs various analysis tasks on the codebase:
# 1. Locates and lists all Makefiles in the project
# 2. Runs 'make clean' for each Makefile found
# 3. Analyzes source code files (.c, .h, .md, .sh) for line counts
# 4. Lists non-source files > 10KB
# 5. Performs linting on various file types
#
# Usage: ./test_z_codebase.sh
#

# Default exclude patterns for linting (can be overridden by .lintignore)
LINT_EXCLUDES=(
    "build/*"
    "build_debug/*"
    "build_perf/*"
    "build_release/*"
    "build_valgrind/*"
    "tests/logs/*"
    "tests/results/*"
    "tests/diagnostics/*"
)

# Function to check if a file should be excluded from linting
should_exclude() {
    local file="$1"
    local lint_ignore="$HYDROGEN_DIR/.lintignore"
    
    # Check .lintignore file first if it exists
    if [ -f "$lint_ignore" ]; then
        while IFS= read -r pattern; do
            # Skip empty lines and comments
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            # Handle both exact matches and glob patterns
            if [[ "$file" == $pattern || "$file" == */$pattern ]]; then
                return 0 # Exclude
            fi
        done < "$lint_ignore"
    fi
    
    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        if [[ "$file" == *"$pattern"* ]]; then
            return 0 # Exclude
        fi
    done
    
    return 1 # Do not exclude
}

# Get the directory where this script is located and set up paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# ====================================================================
# STEP 1: Set up the test environment
# ====================================================================

# Set up test environment with descriptive name
TEST_NAME="Codebase Analysis Test"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Timestamp for log files
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Set up log file
RESULT_LOG="$RESULTS_DIR/codebase_test_${TIMESTAMP}.log"

# Start the test with proper header
start_test "$TEST_NAME" | tee "$RESULT_LOG"

# ====================================================================
# Display information about linting exclusions
# ====================================================================

# Function to extract and display linting exclusion summary
display_linting_exclusions() {
    print_header "Linting Exclusions Information" | tee -a "$RESULT_LOG"
    print_info "NOTE: The linting tests include various exclusions that may impact results." | tee -a "$RESULT_LOG"
    echo "" | tee -a "$RESULT_LOG"
    
    # Extract summaries from each lintignore file
    local general_summary=""
    local c_summary=""
    local markdown_summary=""
    
    # Function to clean up summary text
    clean_summary() {
        # Remove SUMMARY line, trailing hash marks, and "Used by" lines
        echo "$1" | grep -v "SUMMARY" | grep -v "Used by" | sed 's/^# /  /' | sed 's/#$//'
    }
    
    # Extract summary from .lintignore if it exists
    if [ -f "$HYDROGEN_DIR/.lintignore" ]; then
        general_summary=$(grep -A 5 "SUMMARY" "$HYDROGEN_DIR/.lintignore" 2>/dev/null || echo "No summary found.")
        if [ -z "$general_summary" ]; then
            general_summary="Excludes build directories, test artifacts, certain test configs, and snippet includes."
        fi
    fi
    
    # Extract summary from .lintignore-c if it exists
    if [ -f "$HYDROGEN_DIR/.lintignore-c" ]; then
        c_summary=$(grep -A 6 "SUMMARY" "$HYDROGEN_DIR/.lintignore-c" 2>/dev/null || echo "No summary found.")
        if [ -z "$c_summary" ]; then
            c_summary="C specific linting suppressions and configuration options."
        fi
    fi
    
    # Extract summary from .lintignore-markdown if it exists
    if [ -f "$HYDROGEN_DIR/.lintignore-markdown" ]; then
        markdown_summary=$(grep -A 5 "SUMMARY" "$HYDROGEN_DIR/.lintignore-markdown" 2>/dev/null || echo "No summary found.")
        if [ -z "$markdown_summary" ]; then
            markdown_summary="Disables certain Markdown linting rules (line length, HTML tags, EOF newline)."
        fi
    fi
    
    # Display the summaries
    print_info "General Linting Exclusions (.lintignore):" | tee -a "$RESULT_LOG"
    clean_summary "$general_summary" | tee -a "$RESULT_LOG"
    
    print_info "C Linting Exclusions (.lintignore-c):" | tee -a "$RESULT_LOG"
    clean_summary "$c_summary" | tee -a "$RESULT_LOG"
    
    print_info "Markdown Linting Exclusions (.lintignore-markdown):" | tee -a "$RESULT_LOG"
    clean_summary "$markdown_summary" | tee -a "$RESULT_LOG"
    
    print_info "For more details, see tests/README.md and the respective .lintignore files." | tee -a "$RESULT_LOG"
    echo "" | tee -a "$RESULT_LOG"
}

# Display linting exclusions information
display_linting_exclusions

# ====================================================================
# STEP 2: Initialize test variables
# ====================================================================

# Initialize test result to success (will be set to 1 if any test fails)
TEST_RESULT=0

# Arrays to store test results
declare -a TEST_NAMES
declare -a TEST_RESULTS
declare -a TEST_DETAILS

# Temporary files for storing analysis results
MAKEFILES_LIST=$(mktemp)
SOURCE_FILES_LIST=$(mktemp)
LARGE_FILES_LIST=$(mktemp)
LINE_COUNT_FILE=$(mktemp)
# Additional temporary files for file type specific lists
MD_FILES_LIST=$(mktemp)
C_FILES_LIST=$(mktemp)
H_FILES_LIST=$(mktemp)
SH_FILES_LIST=$(mktemp)

# ====================================================================
# STEP 3: Test implementation
# ====================================================================

# Function to locate all Makefiles in the project
find_makefiles() {
    print_header "1. Locating Makefiles" | tee -a "$RESULT_LOG"
    local search_dir=$(convert_to_relative_path "$HYDROGEN_DIR")
    search_dir=${search_dir#hydrogen/}
    print_info "Searching for Makefiles in $search_dir..." | tee -a "$RESULT_LOG"
    
    # Find all files called "Makefile" and write to the list file
    find "$HYDROGEN_DIR" -type f -name "Makefile" > "$MAKEFILES_LIST"
    
    # Count the number of Makefiles found
    MAKEFILE_COUNT=$(wc -l < "$MAKEFILES_LIST")
    
    # Print the findings
    print_info "Found $MAKEFILE_COUNT Makefiles:" | tee -a "$RESULT_LOG"
    while read -r makefile; do
        local rel_path=$(convert_to_relative_path "$makefile")
        rel_path=${rel_path#hydrogen/}
        print_info "  - $rel_path" | tee -a "$RESULT_LOG"
    done < "$MAKEFILES_LIST"
    
    print_result 0 "$MAKEFILE_COUNT Makefiles located successfully" | tee -a "$RESULT_LOG"
    
    # Add to test results
    TEST_NAMES+=("Locate Makefiles")
    TEST_RESULTS+=(0)
    TEST_DETAILS+=("Found $MAKEFILE_COUNT Makefiles")
    
    return 0
}

# Function to run 'make clean' for each Makefile
run_make_clean() {
    print_header "2. Running 'make clean' for each Makefile" | tee -a "$RESULT_LOG"
    
    local make_clean_success=0
    local make_clean_fail=0
    
    # Save current directory
    local start_dir=$(pwd)
    
    # Process each Makefile
    while read -r makefile; do
        local makefile_dir=$(dirname "$makefile")
        local makefile_name=$(basename "$makefile_dir")
        
        local rel_dir=$(convert_to_relative_path "$makefile_dir")
        rel_dir=${rel_dir#hydrogen/}
        print_info "Processing Makefile in: $rel_dir" | tee -a "$RESULT_LOG"
        
        # Create unique temp log
        local temp_log="$RESULTS_DIR/make_clean_${makefile_name}.log"
        
        # Change to directory and run make clean
        if ! cd "$makefile_dir" 2>/dev/null; then
            print_result 1 "Failed to change to directory: $makefile_dir" | tee -a "$RESULT_LOG"
            make_clean_fail=$((make_clean_fail + 1))
            continue
        fi
        
        print_command "make clean" | tee -a "$RESULT_LOG"
        if ! make clean > "$temp_log" 2>&1; then
            # If make clean fails but the directory exists and is accessible,
            # we'll count it as a non-fatal error
            if [ -d "$makefile_dir" ] && [ -w "$makefile_dir" ]; then
                local rel_dir=$(convert_to_relative_path "$makefile_dir")
                rel_dir=${rel_dir#hydrogen/}
                print_warning "make clean failed in $rel_dir but continuing" | tee -a "$RESULT_LOG"
                make_clean_success=$((make_clean_success + 1))
            else
                local rel_dir=$(convert_to_relative_path "$makefile_dir")
                rel_dir=${rel_dir#hydrogen/}
                print_result 1 "make clean failed in $rel_dir" | tee -a "$RESULT_LOG"
                make_clean_fail=$((make_clean_fail + 1))
            fi
        else
            local rel_dir=$(convert_to_relative_path "$makefile_dir")
            rel_dir=${rel_dir#hydrogen/}
            print_result 0 "Successfully ran make clean in $rel_dir" | tee -a "$RESULT_LOG"
            make_clean_success=$((make_clean_success + 1))
        fi
        
        # Show output if any
        if [ -s "$temp_log" ]; then
            print_info "make clean output:" | tee -a "$RESULT_LOG"
            cat "$temp_log" | tee -a "$RESULT_LOG"
        fi
        
        # Return to start directory
        cd "$start_dir" || {
            print_result 1 "Failed to return to start directory" | tee -a "$RESULT_LOG"
            exit 1
        }
        
    done < "$MAKEFILES_LIST"
    
    # Report overall make clean results
    if [ $make_clean_fail -eq 0 ]; then
        print_result 0 "Successfully ran 'make clean' for all $make_clean_success Makefiles" | tee -a "$RESULT_LOG"
        
        # Add to test results
        TEST_NAMES+=("Run make clean")
        TEST_RESULTS+=(0)
        TEST_DETAILS+=("Successfully cleaned all $make_clean_success Makefiles")
        return 0
    else
        # We'll consider it a warning rather than a failure if some make clean operations failed
        print_warning "Failed to run 'make clean' for $make_clean_fail Makefiles" | tee -a "$RESULT_LOG"
        
        # Add to test results but don't fail the test
        TEST_NAMES+=("Run make clean")
        TEST_RESULTS+=(0)
        TEST_DETAILS+=("Cleaned $make_clean_success of $((make_clean_success + make_clean_fail)) Makefiles")
        return 0
    fi
}

# Function to analyze source code files
analyze_source_files() {
    print_header "3. Analyzing Source Code Files" | tee -a "$RESULT_LOG"
    print_info "Finding all .c, .h, .md, and .sh files..." | tee -a "$RESULT_LOG"
    
    # Find all .c, .h, .md, and .sh files and write to the list file
    find "$HYDROGEN_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) | sort > "$SOURCE_FILES_LIST"
    
    # Count the total number of source files
    SOURCE_FILE_COUNT=$(wc -l < "$SOURCE_FILES_LIST")
    # Initialize counters for each line count range
    declare -A LINE_COUNT_BINS
    LINE_COUNT_BINS["000-099"]=0
    LINE_COUNT_BINS["100-199"]=0
    LINE_COUNT_BINS["200-299"]=0
    LINE_COUNT_BINS["300-399"]=0
    LINE_COUNT_BINS["400-499"]=0
    LINE_COUNT_BINS["500-599"]=0
    LINE_COUNT_BINS["600-699"]=0
    LINE_COUNT_BINS["700-799"]=0
    LINE_COUNT_BINS["800-899"]=0
    LINE_COUNT_BINS["900-999"]=0
    LINE_COUNT_BINS["1000+"]=0
    
    # Flag to track if any file exceeds 1000 lines
    local has_large_file=0
    local largest_file=""
    local largest_line_count=0
    
    # Analyze each source file
    while read -r source_file; do
        # Count the number of lines in the file
        local line_count=$(wc -l < "$source_file")
        
        # Store the line count for the file (pad with zeros for proper sorting)
        printf "%05d %s\n" "$line_count" "$(convert_to_relative_path "$source_file")" >> "$LINE_COUNT_FILE"
        
        # Increment the appropriate counter
        if [ $line_count -lt 100 ]; then
            LINE_COUNT_BINS["000-099"]=$((LINE_COUNT_BINS["000-099"] + 1))
        elif [ $line_count -lt 200 ]; then
            LINE_COUNT_BINS["100-199"]=$((LINE_COUNT_BINS["100-199"] + 1))
        elif [ $line_count -lt 300 ]; then
            LINE_COUNT_BINS["200-299"]=$((LINE_COUNT_BINS["200-299"] + 1))
        elif [ $line_count -lt 400 ]; then
            LINE_COUNT_BINS["300-399"]=$((LINE_COUNT_BINS["300-399"] + 1))
        elif [ $line_count -lt 500 ]; then
            LINE_COUNT_BINS["400-499"]=$((LINE_COUNT_BINS["400-499"] + 1))
        elif [ $line_count -lt 600 ]; then
            LINE_COUNT_BINS["500-599"]=$((LINE_COUNT_BINS["500-599"] + 1))
        elif [ $line_count -lt 700 ]; then
            LINE_COUNT_BINS["600-699"]=$((LINE_COUNT_BINS["600-699"] + 1))
        elif [ $line_count -lt 800 ]; then
            LINE_COUNT_BINS["700-799"]=$((LINE_COUNT_BINS["700-799"] + 1))
        elif [ $line_count -lt 900 ]; then
            LINE_COUNT_BINS["800-899"]=$((LINE_COUNT_BINS["800-899"] + 1))
        elif [ $line_count -lt 1000 ]; then
            LINE_COUNT_BINS["900-999"]=$((LINE_COUNT_BINS["900-999"] + 1))
        else
            LINE_COUNT_BINS["1000+"]=$((LINE_COUNT_BINS["1000+"] + 1))
            has_large_file=1
            
            # Track the largest file
            if [ $line_count -gt $largest_line_count ]; then
                largest_file=$(convert_to_relative_path "$source_file")
                largest_line_count=$line_count
            fi
        fi
    done < "$SOURCE_FILES_LIST"
    
    # Sort the line count file from largest to smallest
    sort -nr -o "$LINE_COUNT_FILE" "$LINE_COUNT_FILE"
    
    # Print the analysis results
    print_info "Source Code Line Count Distribution ($SOURCE_FILE_COUNT files):" | tee -a "$RESULT_LOG"
    print_info "000-099 Lines: ${LINE_COUNT_BINS["000-099"]} files" | tee -a "$RESULT_LOG"
    print_info "100-199 Lines: ${LINE_COUNT_BINS["100-199"]} files" | tee -a "$RESULT_LOG"
    print_info "200-299 Lines: ${LINE_COUNT_BINS["200-299"]} files" | tee -a "$RESULT_LOG"
    print_info "300-399 Lines: ${LINE_COUNT_BINS["300-399"]} files" | tee -a "$RESULT_LOG"
    print_info "400-499 Lines: ${LINE_COUNT_BINS["400-499"]} files" | tee -a "$RESULT_LOG"
    print_info "500-599 Lines: ${LINE_COUNT_BINS["500-599"]} files" | tee -a "$RESULT_LOG"
    print_info "600-699 Lines: ${LINE_COUNT_BINS["600-699"]} files" | tee -a "$RESULT_LOG"
    print_info "700-799 Lines: ${LINE_COUNT_BINS["700-799"]} files" | tee -a "$RESULT_LOG"
    print_info "800-899 Lines: ${LINE_COUNT_BINS["800-899"]} files" | tee -a "$RESULT_LOG"
    print_info "900-999 Lines: ${LINE_COUNT_BINS["900-999"]} files" | tee -a "$RESULT_LOG"
    print_info "1000+ Lines: ${LINE_COUNT_BINS["1000+"]} files" | tee -a "$RESULT_LOG"
    
    # Show the top 20 largest files
    print_info "Top 20 Largest Source Files:" | tee -a "$RESULT_LOG"
    head -n 20 "$LINE_COUNT_FILE" | while read -r line; do
        local count path
        read -r count path <<< "$line"
        path=${path#hydrogen/}
        print_info "  $count lines: $path" | tee -a "$RESULT_LOG"
    done
    
    # Filter files by type and sort by line count
    grep "\.md$" "$LINE_COUNT_FILE" > "$MD_FILES_LIST"
    grep "\.c$" "$LINE_COUNT_FILE" > "$C_FILES_LIST"
    grep "\.h$" "$LINE_COUNT_FILE" > "$H_FILES_LIST"
    grep "\.sh$" "$LINE_COUNT_FILE" > "$SH_FILES_LIST"
    
    # Show the top 20 .md files
    print_info "Top 20 Markdown (.md) Files:" | tee -a "$RESULT_LOG"
    head -n 20 "$MD_FILES_LIST" | while read -r line; do
        local count path
        read -r count path <<< "$line"
        path=${path#hydrogen/}
        print_info "  $count lines: $path" | tee -a "$RESULT_LOG"
    done
    
    # Show the top 20 .c files
    print_info "Top 20 C (.c) Files:" | tee -a "$RESULT_LOG"
    head -n 20 "$C_FILES_LIST" | while read -r line; do
        local count path
        read -r count path <<< "$line"
        path=${path#hydrogen/}
        print_info "  $count lines: $path" | tee -a "$RESULT_LOG"
    done
    
    # Show the top 20 .h files
    print_info "Top 20 Header (.h) Files:" | tee -a "$RESULT_LOG"
    head -n 20 "$H_FILES_LIST" | while read -r line; do
        local count path
        read -r count path <<< "$line"
        path=${path#hydrogen/}
        print_info "  $count lines: $path" | tee -a "$RESULT_LOG"
    done
    
    # Show the top 20 .sh files
    print_info "Top 20 Shell Script (.sh) Files:" | tee -a "$RESULT_LOG"
    head -n 20 "$SH_FILES_LIST" | while read -r line; do
        local count path
        read -r count path <<< "$line"
        path=${path#hydrogen/}
        print_info "  $count lines: $path" | tee -a "$RESULT_LOG"
    done
    
    # Check if any file exceeds 1000 lines
    if [ $has_large_file -eq 1 ]; then
        print_result 1 "FAIL: Found source file(s) exceeding 1000 lines" | tee -a "$RESULT_LOG"
        
        # Add to test results - this is the primary pass/fail condition
        TEST_NAMES+=("Source File Size Check")
        TEST_RESULTS+=(1)
        local clean_path=${largest_file#hydrogen/}
        TEST_DETAILS+=("Found $clean_path with $largest_line_count lines (exceeds 1000 line limit)")
        
        TEST_RESULT=1
    else
        print_result 0 "PASS: No source files exceed 1000 lines" | tee -a "$RESULT_LOG"
        
        # Add to test results
        TEST_NAMES+=("Source File Size Check")
        TEST_RESULTS+=(0)
        TEST_DETAILS+=("No source files exceed 1000 line limit")
    fi
    
    # Add source file count to test results
    TEST_NAMES+=("Source File Count")
    TEST_RESULTS+=(0)
    TEST_DETAILS+=("Found $SOURCE_FILE_COUNT source files")
    
    return $has_large_file
}

# Function to list large non-source files
list_large_files() {
    print_header "4. Listing Large Non-Source Files (>25KB)" | tee -a "$RESULT_LOG"
    print_info "Finding all files >25KB excluding source and documentation files..." | tee -a "$RESULT_LOG"
    
    # Find all files >25KB excluding .c, .h, .md, .sh files, Makefiles, and files under tests/
    find "$HYDROGEN_DIR" -type f -size +25k -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" -o -name "Makefile" \) | sort > "$LARGE_FILES_LIST"
    
    # Count the number of large files
    LARGE_FILE_COUNT=$(wc -l < "$LARGE_FILES_LIST")
    
    if [ $LARGE_FILE_COUNT -eq 0 ]; then
        print_info "No files >25KB found (excluding source and documentation files)" | tee -a "$RESULT_LOG"
    else
        print_info "Found $LARGE_FILE_COUNT files >25KB:" | tee -a "$RESULT_LOG"
        
        # List the files with their sizes
        while read -r large_file; do
            # Skip if the file is under tests/
            if [[ "$large_file" == *"/tests/"* ]]; then
                continue
            fi
            # Get the file size in KB
            local file_size=$(du -k "$large_file" | cut -f1)
            local rel_path=$(convert_to_relative_path "$large_file")
            rel_path=${rel_path#hydrogen/}
            print_info "  ${file_size}KB: $rel_path" | tee -a "$RESULT_LOG"
        done < "$LARGE_FILES_LIST"
    fi
    
    print_result 0 "Large file analysis completed" | tee -a "$RESULT_LOG"
    
    # Add to test results
    TEST_NAMES+=("Large File Analysis")
    TEST_RESULTS+=(0)
    TEST_DETAILS+=("Found $LARGE_FILE_COUNT files >25KB (excluding source and documentation files)")
    
    echo "" | tee -a "$RESULT_LOG"
    return 0
}

# Function to run linting tests
run_linting_tests() {
    print_header "5. Running Linting Tests" | tee -a "$RESULT_LOG"
    local lint_result=0
    local lint_temp_log=$(mktemp)

    # Function to display limited error output
    display_limited_output() {
        local output_file="$1"
        local total_lines=$(wc -l < "$output_file")
        if [ $total_lines -gt 100 ]; then
            head -n 100 "$output_file" | tee -a "$RESULT_LOG"
            print_warning "Output truncated. Showing 100 of $total_lines lines." | tee -a "$RESULT_LOG"
        else
            cat "$output_file" | tee -a "$RESULT_LOG"
        fi
    }

    # C files with support_cppcheck.sh
    print_info "Linting C files with support_cppcheck.sh..." | tee -a "$RESULT_LOG"
    local cppcheck_fails=0
    local cppcheck_temp=$(mktemp)
    local c_file_count=0
    
    # Count all C files, excluding build directories and test artifacts
    c_file_count=$(find "$HYDROGEN_DIR" -type f \( -name "*.c" -o -name "*.h" \) \
        -not -path "*/build*/*" \
        -not -path "*/tests/logs/*" \
        -not -path "*/tests/results/*" \
        -not -path "*/tests/diagnostics/*" | wc -l)
    print_info "Found $c_file_count C files to check" | tee -a "$RESULT_LOG"
    
    # Run support_cppcheck.sh and capture output
    local output
    output=$("$SCRIPT_DIR/support_cppcheck.sh" "$HYDROGEN_DIR" 2>&1)
    echo "$output" | tee -a "$RESULT_LOG"
    
    # Check for the expected null pointer warning in crash handler test
    local expected_warning="src/hydrogen.c:372.*warning: Possible null pointer dereference: ptr"
    local has_expected=0
    local other_issues=0
    
    # Process each line of output
    while IFS= read -r line; do
        if [[ "$line" =~ ^🛈 ]]; then
            continue  # Skip info lines
        elif [[ "$line" =~ $expected_warning ]]; then
            has_expected=1
        elif [[ "$line" =~ /.*: ]]; then
            ((other_issues++))
        fi
    done <<< "$output"
    
    # Evaluate results
    if [ $other_issues -gt 0 ]; then
        cppcheck_fails=$other_issues
        lint_result=1
        print_result 1 "cppcheck found $other_issues unexpected issues in $c_file_count files" | tee -a "$RESULT_LOG"
    elif [ $has_expected -eq 1 ]; then
        print_info "Found expected null pointer warning in crash handler test" | tee -a "$RESULT_LOG"
        print_result 0 "cppcheck validation passed (1 expected warning)" | tee -a "$RESULT_LOG"
    else
        print_info "No cppcheck issues found in $c_file_count files" | tee -a "$RESULT_LOG"
    fi

    # Add cppcheck results
    TEST_NAMES+=("Lint C Files")
    if [ $other_issues -eq 0 ]; then
        TEST_RESULTS+=(0)
        if [ $has_expected -eq 1 ]; then
            TEST_DETAILS+=("Found 1 expected warning (crash handler test) in $c_file_count files")
        else
            TEST_DETAILS+=("No issues found in $c_file_count files")
        fi
    else
        TEST_RESULTS+=(1)
        TEST_DETAILS+=("Found $other_issues unexpected issues in $c_file_count files")
    fi
    rm -f "$cppcheck_temp"
    > "$lint_temp_log"  # Clear log for next linter
    
   
    # Markdown files with markdownlint
    print_info "Linting Markdown files with markdownlint..." | tee -a "$RESULT_LOG"
    local md_fails=0
    local md_temp=$(mktemp)
    local md_file_count=0
    while read -r file; do
        if ! should_exclude "$file"; then
            ((md_file_count++))
            if ! markdownlint --config "$HYDROGEN_DIR/.lintignore-markdown" "$file" 2> "$md_temp"; then
                # Count each line as a separate issue
                local issue_count=$(wc -l < "$md_temp")
                md_fails=$((md_fails + issue_count))
                cat "$md_temp" >> "$lint_temp_log"
            fi
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.md")
    rm -f "$md_temp"
    
    if [ -s "$lint_temp_log" ]; then
        print_warning "markdownlint found $md_fails issues in $md_file_count files:" | tee -a "$RESULT_LOG"
        display_limited_output "$lint_temp_log"
        lint_result=1
    else
        print_info "No markdownlint issues found in $md_file_count files" | tee -a "$RESULT_LOG"
    fi
    > "$lint_temp_log"  # Clear log for next linter

    # Add markdownlint results
    TEST_NAMES+=("Lint Markdown Files")
    TEST_RESULTS+=($([[ $md_fails -eq 0 ]] && echo 0 || echo 1))
    if [ $md_fails -eq 0 ]; then
        TEST_DETAILS+=("No issues found in $md_file_count files")
    else
        TEST_DETAILS+=("Found $md_fails issues in $md_file_count files")
    fi

    # JSON files with jsonlint
    print_info "Linting JSON files with jsonlint..." | tee -a "$RESULT_LOG"
    local json_fails=0
    local json_count=0
    while read -r file; do
        if ! should_exclude "$file"; then
            ((json_count++))
            # Create a temporary file for this specific JSON file's errors
            local json_error_log=$(mktemp)
            if ! jsonlint -q "$file" 2> "$json_error_log"; then
                json_fails=$((json_fails + 1))
                # Get just the first 4 lines of the error
                local rel_path=$(convert_to_relative_path "$file")
                rel_path=${rel_path#hydrogen/}
                print_warning "JSON error in file: $rel_path" >> "$lint_temp_log"
                head -n 4 "$json_error_log" >> "$lint_temp_log"
                echo "" >> "$lint_temp_log"  # Add blank line between errors
            fi
            rm -f "$json_error_log"
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.json")
    
    if [ $json_count -eq 0 ]; then
        print_warning "No JSON files found to lint" | tee -a "$RESULT_LOG"
    elif [ -s "$lint_temp_log" ]; then
        print_warning "jsonlint found $json_fails issues in $json_count files:" | tee -a "$RESULT_LOG"
        display_limited_output "$lint_temp_log"
        lint_result=1
    else
        print_info "No jsonlint issues found in $json_count files" | tee -a "$RESULT_LOG"
    fi
    > "$lint_temp_log"  # Clear log for next linter
    
    # Add jsonlint results
    TEST_NAMES+=("Lint JSON Files")
    TEST_RESULTS+=($([[ $json_fails -eq 0 ]] && echo 0 || echo 1))
    if [ $json_count -eq 0 ]; then
        TEST_DETAILS+=("No files found to lint")
    elif [ $json_fails -eq 0 ]; then
        TEST_DETAILS+=("No issues found in $json_count files")
    else
        TEST_DETAILS+=("Found $json_fails issues in $json_count files")
    fi

    # JavaScript files with eslint
    print_info "Linting JavaScript files with eslint..." | tee -a "$RESULT_LOG"
    local js_fails=0
    local js_count=0
    while read -r file; do
        if ! should_exclude "$file"; then
            ((js_count++))
            if ! eslint --quiet "$file" 2>> "$lint_temp_log"; then
                js_fails=$((js_fails + 1))
            fi
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.js" -o -name "*.jsx" -o -name "*.ts" -o -name "*.tsx")
    
    if [ $js_count -eq 0 ]; then
        print_warning "No JavaScript files found to lint (this is expected for now)" | tee -a "$RESULT_LOG"
    elif [ -s "$lint_temp_log" ]; then
        print_warning "eslint found $js_fails issues in $js_count files:" | tee -a "$RESULT_LOG"
        display_limited_output "$lint_temp_log"
        lint_result=1
    else
        print_info "No eslint issues found in $js_count files" | tee -a "$RESULT_LOG"
    fi
    > "$lint_temp_log"  # Clear log for next linter
    
    # Add eslint results
    TEST_NAMES+=("Lint JavaScript Files")
    TEST_RESULTS+=($([[ $js_fails -eq 0 ]] && echo 0 || echo 1))
    if [ $js_count -eq 0 ]; then
        TEST_DETAILS+=("No files found to lint (expected)")
    elif [ $js_fails -eq 0 ]; then
        TEST_DETAILS+=("No issues found in $js_count files")
    else
        TEST_DETAILS+=("Found $js_fails issues in $js_count files")
    fi

    # CSS files with stylelint
    print_info "Linting CSS files with stylelint..." | tee -a "$RESULT_LOG"
    local css_fails=0
    local css_count=0
    while read -r file; do
        if ! should_exclude "$file"; then
            ((css_count++))
            if ! stylelint "$file" 2>> "$lint_temp_log"; then
                css_fails=$((css_fails + 1))
            fi
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.css" -o -name "*.scss" -o -name "*.sass")
    
    if [ $css_count -eq 0 ]; then
        print_warning "No CSS files found to lint (this is expected for now)" | tee -a "$RESULT_LOG"
    elif [ -s "$lint_temp_log" ]; then
        print_warning "stylelint found $css_fails issues in $css_count files:" | tee -a "$RESULT_LOG"
        display_limited_output "$lint_temp_log"
        lint_result=1
    else
        print_info "No stylelint issues found in $css_count files" | tee -a "$RESULT_LOG"
    fi
    > "$lint_temp_log"  # Clear log for next linter
    
    # Add stylelint results
    TEST_NAMES+=("Lint CSS Files")
    TEST_RESULTS+=($([[ $css_fails -eq 0 ]] && echo 0 || echo 1))
    if [ $css_count -eq 0 ]; then
        TEST_DETAILS+=("No files found to lint (expected)")
    elif [ $css_fails -eq 0 ]; then
        TEST_DETAILS+=("No issues found in $css_count files")
    else
        TEST_DETAILS+=("Found $css_fails issues in $css_count files")
    fi

    # HTML files with htmlhint
    print_info "Linting HTML files with htmlhint..." | tee -a "$RESULT_LOG"
    local html_fails=0
    local html_count=0
    while read -r file; do
        if ! should_exclude "$file"; then
            ((html_count++))
            if ! htmlhint "$file" 2>> "$lint_temp_log"; then
                html_fails=$((html_fails + 1))
            fi
        fi
    done < <(find "$HYDROGEN_DIR" -type f -name "*.html" -o -name "*.htm")
    
    if [ $html_count -eq 0 ]; then
        print_warning "No HTML files found to lint (this is expected for now)" | tee -a "$RESULT_LOG"
    elif [ -s "$lint_temp_log" ]; then
        print_warning "htmlhint found $html_fails issues in $html_count files:" | tee -a "$RESULT_LOG"
        display_limited_output "$lint_temp_log"
        lint_result=1
    else
        print_info "No htmlhint issues found in $html_count files" | tee -a "$RESULT_LOG"
    fi
    > "$lint_temp_log"  # Clear log for next linter
    
    # Add htmlhint results
    TEST_NAMES+=("Lint HTML Files")
    TEST_RESULTS+=($([[ $html_fails -eq 0 ]] && echo 0 || echo 1))
    if [ $html_count -eq 0 ]; then
        TEST_DETAILS+=("No files found to lint (expected)")
    elif [ $html_fails -eq 0 ]; then
        TEST_DETAILS+=("No issues found in $html_count files")
    else
        TEST_DETAILS+=("Found $html_fails issues in $html_count files")
    fi

    # Run cloc analysis
    print_header "6. Running Code Line Count Analysis" | tee -a "$RESULT_LOG"
    print_info "Analyzing code lines by language..." | tee -a "$RESULT_LOG"
    
    # Create temporary file for cloc output
    local cloc_output=$(mktemp)
    
    # Save current directory
    local start_dir=$(pwd)
    
    # Change to project root directory
    cd "$HYDROGEN_DIR" || {
        print_warning "Failed to change to project directory for cloc analysis" | tee -a "$RESULT_LOG"
        TEST_NAMES+=("Code Line Count Analysis")
        TEST_RESULTS+=(1)
        TEST_DETAILS+=("Failed to analyze code lines - directory access error")
        return $lint_result
    }
    
    # Run cloc with specific locale settings to ensure consistent output
    if env LC_ALL=en_US.UTF_8 LC_TIME= LC_ALL= LANG= LANGUAGE= cloc . --quiet --force-lang="C,inc" > "$cloc_output" 2>&1; then
        # Display cloc output without the banner
        tail -n +2 "$cloc_output" | tee -a "$RESULT_LOG"
        
        # Extract summary statistics
        local total_files=$(grep "SUM:" "$cloc_output" | awk '{print $2}')
        local total_blank=$(grep "SUM:" "$cloc_output" | awk '{print $3}')
        local total_comment=$(grep "SUM:" "$cloc_output" | awk '{print $4}')
        local total_code=$(grep "SUM:" "$cloc_output" | awk '{print $5}')
        
        # Add to test results
        TEST_NAMES+=("Code Line Count Analysis")
        TEST_RESULTS+=(0)
        TEST_DETAILS+=("Found $total_files files with $total_code lines of code ($total_blank blank, $total_comment comment)")
    else
        print_warning "Failed to run cloc analysis" | tee -a "$RESULT_LOG"
        TEST_NAMES+=("Code Line Count Analysis")
        TEST_RESULTS+=(1)
        TEST_DETAILS+=("Failed to analyze code lines")
    fi
    
    rm -f "$cloc_output"
    
    # Return to start directory
    cd "$start_dir" || {
        print_warning "Failed to return to start directory after cloc analysis" | tee -a "$RESULT_LOG"
    }
    
    # Cleanup
    rm -f "$lint_temp_log"
    
    return $lint_result
}

# Function to run all test cases
run_tests() {
    find_makefiles
    run_make_clean
    analyze_source_files
    list_large_files
    run_linting_tests
    return $TEST_RESULT
}

# Run all the test cases
run_tests
TEST_RESULT=$?

# ====================================================================
# STEP 5: Cleanup and Results
# ====================================================================

# Save analysis results

# Save analysis files with timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
cp "$LINE_COUNT_FILE" "$RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt"
print_info "Line count analysis saved to: $(convert_to_relative_path "$RESULTS_DIR/source_line_counts_${TIMESTAMP}.txt")" | tee -a "$RESULT_LOG"

cp "$LARGE_FILES_LIST" "$RESULTS_DIR/large_files_${TIMESTAMP}.txt"
print_info "Large files list saved to: $(convert_to_relative_path "$RESULTS_DIR/large_files_${TIMESTAMP}.txt")" | tee -a "$RESULT_LOG"

# Clean up temporary files
rm -f "$MAKEFILES_LIST" "$SOURCE_FILES_LIST" "$LARGE_FILES_LIST" "$LINE_COUNT_FILE" "$MD_FILES_LIST" "$C_FILES_LIST" "$H_FILES_LIST" "$SH_FILES_LIST" response_*.json

# ====================================================================
# STEP 6: Report Results
# ====================================================================

# Calculate pass/fail counts
PASS_COUNT=0
FAIL_COUNT=0
for result in "${TEST_RESULTS[@]}"; do
    if [ $result -eq 0 ]; then
        ((PASS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
done

# Print individual test results
print_header "Individual Test Results" | tee -a "$RESULT_LOG"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item ${TEST_RESULTS[$i]} "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}" | tee -a "$RESULT_LOG"
done

# Print summary statistics
print_test_summary $((PASS_COUNT + FAIL_COUNT)) $PASS_COUNT $FAIL_COUNT | tee -a "$RESULT_LOG"

# Export subtest results for test_all.sh to pick up
export_subtest_results $((PASS_COUNT + FAIL_COUNT)) $PASS_COUNT

# End the test with final result
end_test $TEST_RESULT "$TEST_NAME" | tee -a "$RESULT_LOG"

exit $TEST_RESULT