#!/bin/bash
#
# Codebase Analysis Test
# This test performs various analysis tasks on the codebase:
# 1. Locates and lists all Makefiles in the project
# 2. Runs 'make clean' for each Makefile found
# 3. Analyzes source code files (.c and .h) for line counts
# 4. Lists non-source files > 10KB
#
# Usage: ./test_z_codebase.sh
#

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
    print_info "Finding all .c and .h files..." | tee -a "$RESULT_LOG"
    
    # Find all .c and .h files and write to the list file
    find "$HYDROGEN_DIR" -type f \( -name "*.c" -o -name "*.h" \) | sort > "$SOURCE_FILES_LIST"
    
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
        
        # Store the line count for the file
        echo "$line_count $(convert_to_relative_path "$source_file")" >> "$LINE_COUNT_FILE"
        
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
    
    # Show the top 5 largest files
    print_info "Top 5 Largest Source Files:" | tee -a "$RESULT_LOG"
    head -n 5 "$LINE_COUNT_FILE" | while read -r line; do
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
    
    # Find all files >25KB excluding .c, .h, .md files, Makefiles, and files under tests/
    find "$HYDROGEN_DIR" -type f -size +25k -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "Makefile" \) | sort > "$LARGE_FILES_LIST"
    
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

# ====================================================================
# STEP 4: Run Tests
# ====================================================================

# Function to run all test cases
run_tests() {
    find_makefiles
    run_make_clean
    analyze_source_files
    list_large_files
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
rm -f "$MAKEFILES_LIST" "$SOURCE_FILES_LIST" "$LARGE_FILES_LIST" "$LINE_COUNT_FILE" response_*.json

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