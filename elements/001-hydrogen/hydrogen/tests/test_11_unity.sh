#!/bin/bash

# Test: Unity
# Runs unit tests using the Unity framework, treating each test file as a subtest

# CHANGELOG
# 2.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.1 - 2025-07-06 - Removed hardcoded absolute path; now handled by log_output.sh
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - 2025-06-25 - Initial version for running Unity tests

# Test configuration
TEST_NAME="Unity Unit Tests"
SCRIPT_VERSION="2.0.2"

# Get the directory where this script is located
TEST_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh  # Resolve path statically
    source "$TEST_SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/lifecycle.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/coverage.sh"

# Restore SCRIPT_DIR after sourcing libraries (they may override it)
SCRIPT_DIR="$TEST_SCRIPT_DIR"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=0
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
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Configuration
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
UNITY_DIR="$SCRIPT_DIR/unity"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
LOG_FILE="$SCRIPT_DIR/logs/unity_tests.log"
DIAG_TEST_DIR="$DIAG_DIR/unity_tests_${TIMESTAMP}"


# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Function to download Unity framework if missing
download_unity_framework() {
    local framework_dir="$UNITY_DIR/framework"
    local unity_dir="$framework_dir/Unity"
    if [ ! -d "$unity_dir" ]; then
        print_message "Unity framework not found in $unity_dir. Downloading now..."
        mkdir -p "$framework_dir"
        if command -v curl >/dev/null 2>&1; then
            if curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o "$framework_dir/unity.zip"; then
                unzip "$framework_dir/unity.zip" -d "$framework_dir/"
                mv "$framework_dir/Unity-master" "$unity_dir"
                rm "$framework_dir/unity.zip"
                print_result 0 "Unity framework downloaded and extracted successfully to $unity_dir."
                return 0
            else
                print_result 1 "Failed to download Unity framework with curl."
                return 1
            fi
        elif command -v git >/dev/null 2>&1; then
            if git clone https://github.com/ThrowTheSwitch/Unity.git "$unity_dir"; then
                print_result 0 "Unity framework cloned successfully to $unity_dir."
                return 0
            else
                print_result 1 "Failed to clone Unity framework with git."
                return 1
            fi
        else
            print_result 1 "Neither curl nor git is available to download the Unity framework."
            return 1
        fi
    else
        print_message "Unity framework already exists in $unity_dir."
        return 0
    fi
}

# Function to compile Unity tests
compile_unity_tests() {
    next_subtest
    print_subtest "Compile Unity Tests"
    print_message "Compiling Unity tests..."
    local build_dir="$HYDROGEN_DIR/build/unity"
    local temp_log="$DIAG_TEST_DIR/compile_log.txt"
    local filtered_log="$DIAG_TEST_DIR/compile_filtered.txt"
    mkdir -p "$build_dir"
    mkdir -p "$(dirname "$temp_log")"
    cd "$build_dir" || { print_result 1 "Failed to change to build directory: ${build_dir#"$SCRIPT_DIR"/..}"; return 1; }
    
    # Run cmake configuration
    cmake "$UNITY_DIR" > "$temp_log" 2>&1 || { print_result 1 "CMake configuration failed for Unity tests"; while IFS= read -r line; do print_output "$line"; done < "$temp_log"; return 1; }
    
    # Filter and display cmake output (keep important messages, skip compilation progress)
    grep -v -E '\[[[:space:]]*[0-9]+%\][[:space:]]*Compiling.*\.c to object file' "$temp_log" > "$filtered_log" || true
    while IFS= read -r line; do print_output "$line"; done < "$filtered_log"
    
    # Detect number of CPU cores for parallel build
    local cpu_cores
    if command -v nproc >/dev/null 2>&1; then
        cpu_cores=$(nproc)
    elif [ -f /proc/cpuinfo ]; then
        cpu_cores=$(grep -c ^processor /proc/cpuinfo)
    else
        cpu_cores=4  # Fallback to 4 cores
    fi
    
    print_message "Using $cpu_cores cores for parallel compilation..."
    
    # Run build with parallel jobs (separate log file to avoid duplication)
    local build_log="$DIAG_TEST_DIR/build_log.txt"
    cmake --build . -j"$cpu_cores" > "$build_log" 2>&1 || { print_result 1 "Build failed for Unity tests"; while IFS= read -r line; do print_output "$line"; done < "$build_log"; return 1; }
    
    # Filter and display build output (keep important messages, skip compilation progress)  
    grep -v -E '\[[[:space:]]*[0-9]+%\][[:space:]]*Compiling.*\.c to object file' "$build_log" > "$filtered_log" || true
    while IFS= read -r line; do print_output "$line"; done < "$filtered_log"
    
    cd "$SCRIPT_DIR" || { print_result 1 "Failed to return to script directory: ${SCRIPT_DIR#"$SCRIPT_DIR"/..}"; return 1; }
    print_result 0 "Unity tests compiled successfully."
    return 0
}

# Function to run Unity tests
run_unity_tests() {
    next_subtest
    print_subtest "Enumerate Unity Tests"
    print_message "Enumerating Unity Tests"
    
    if [ ! -d "$UNITY_DIR" ]; then
        print_result 1 "Unity test directory not found: ${UNITY_DIR#"$SCRIPT_DIR"/..}"
        return 1
    fi
    
    local build_dir="$HYDROGEN_DIR/build/unity"
    if [ ! -d "$build_dir" ]; then
        print_result 1 "Build directory not found: ${build_dir#"$SCRIPT_DIR"/..}"
        return 1
    fi
    
    # Get list of test names using ctest -N
    cd "$build_dir" || { print_result 1 "Failed to change to build directory: ${build_dir#"$SCRIPT_DIR"/..}"; return 1; }
    local test_list=()
    mapfile -t test_list < <(ctest -N | grep "Test #" | awk '{print $3}')
    local unity_test_count=${#test_list[@]}
    # Initialize TOTAL_SUBTESTS to account for initial subtests (4 up to this point: Create Output Directories, Check Unity Framework, Compile Unity Tests, Enumerate Unity Tests)
    TOTAL_SUBTESTS=4
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + unity_test_count))
    print_result 0 "Unity test enumeration completed."
    ((PASS_COUNT++))
    
    local INITIAL_PASS_COUNT=$PASS_COUNT
    
    # Run each test individually to display as subtest
    local test_name
    true > "$LOG_FILE"  # Clear log file
    local temp_test_log="$DIAG_TEST_DIR/test_output.txt"
    mkdir -p "$(dirname "$temp_test_log")"
    PASS_COUNT=0  # Reset for counting Unity tests only in loop
    for test_name in "${test_list[@]}"; do
        next_subtest
        print_subtest "Unity Test: $test_name"
        true > "$temp_test_log"  # Clear temporary log for this test
        ctest -R "^$test_name$" --output-on-failure > "$temp_test_log" 2>&1
        while IFS= read -r line; do
            print_output "$line"
        done < "$temp_test_log"
        if grep -q "Passed" "$temp_test_log"; then
            print_result 0 "$test_name passed."
            ((PASS_COUNT++))
        else
            print_result 1 "$test_name failed. Check ${LOG_FILE#"$SCRIPT_DIR"/..} for details."
            EXIT_CODE=1
        fi
        cat "$temp_test_log" >> "$LOG_FILE"
    done
    
    cd "$SCRIPT_DIR" || { print_result 1 "Failed to return to script directory: ${SCRIPT_DIR#"$SCRIPT_DIR"/..}"; return 1; }
    
    # Update total PASS_COUNT to include passes from initial subtests and Unity tests
    PASS_COUNT=$((INITIAL_PASS_COUNT + PASS_COUNT))
    
    # Summary line removed as per feedback; final table reports the details
    :
    
    return $EXIT_CODE
}

# Check and download Unity framework
next_subtest
print_subtest "Check Unity Framework"
if download_unity_framework; then
    print_result 0 "Unity framework check passed."
    ((PASS_COUNT++))
else
    print_result 1 "Unity framework check failed."
    EXIT_CODE=1
fi

# Compile and run tests
if compile_unity_tests; then
    ((PASS_COUNT++))
    if ! run_unity_tests; then
        EXIT_CODE=1
    fi
    
    # Calculate Unity test coverage
    next_subtest
    print_subtest "Calculate Unity Test Coverage"
    print_message "Calculating Unity test coverage..."
    
    build_dir="$HYDROGEN_DIR/build/unity"
    unity_coverage=$(calculate_unity_coverage "$build_dir" "$TIMESTAMP")
    result=$?
    
    if [ $result -eq 0 ]; then
        # Read .trial-ignore patterns to exclude files from analysis
        trial_ignore_patterns=()
        if [[ -f "$HYDROGEN_DIR/.trial-ignore" ]]; then
            while IFS= read -r line; do
                # Skip comments and empty lines
                if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "$line" ]]; then
                    continue
                fi
                # Remove leading ./ if present and add pattern
                pattern="${line#./}"
                trial_ignore_patterns+=("$pattern")
            done < "$HYDROGEN_DIR/.trial-ignore"
        fi
        
        # Count coverage statistics directly from .gcov files
        total_files=0
        covered_files=0
        total_lines=0
        covered_lines=0
        
        # Create a list of .gcov files to process, filtering out excluded ones
        gcov_files_to_process=()
        while IFS= read -r gcov_file; do
            if [[ -f "$gcov_file" ]]; then
                # Skip Unity framework files and system files
                basename_file=$(basename "$gcov_file")
                if [[ "$basename_file" == "unity.c.gcov" ]] || [[ "$gcov_file" == *"/usr/"* ]]; then
                    continue
                fi
                
                # Skip system libraries and external dependencies
                if [[ "$basename_file" == *"jansson"* ]] || \
                   [[ "$basename_file" == *"json"* ]] || \
                   [[ "$basename_file" == *"curl"* ]] || \
                   [[ "$basename_file" == *"ssl"* ]] || \
                   [[ "$basename_file" == *"crypto"* ]] || \
                   [[ "$basename_file" == *"pthread"* ]] || \
                   [[ "$basename_file" == *"uuid"* ]] || \
                   [[ "$basename_file" == *"zlib"* ]] || \
                   [[ "$basename_file" == *"pcre"* ]]; then
                    continue
                fi
                
                # Check if this file should be ignored based on .trial-ignore patterns
                source_file="${basename_file%.gcov}"
                should_ignore=false
                for pattern in "${trial_ignore_patterns[@]}"; do
                    if [[ "$source_file" == *"$pattern"* ]]; then
                        should_ignore=true
                        break
                    fi
                done
                if [[ "$should_ignore" == true ]]; then
                    continue
                fi
                
                gcov_files_to_process+=("$gcov_file")
            fi
        done < <(find "$build_dir/src" -name "*.gcov" -type f 2>/dev/null)
        
        # Concatenate all relevant .gcov files and process them efficiently
        if [[ ${#gcov_files_to_process[@]} -gt 0 ]]; then
            # Create temporary combined file
            combined_gcov="$DIAG_TEST_DIR/combined.gcov"
            cat "${gcov_files_to_process[@]}" > "$combined_gcov"
            
            # Count only instrumented lines (covered + uncovered but measurable) using same method as extras script
            line_counts=$(awk '
                /^[[:space:]]*[0-9]+:[[:space:]]*[0-9]+:/ { covered++; total++ }
                /^[[:space:]]*#####:[[:space:]]*[0-9]+:/ { total++ }
                END { print total "," covered }
            ' "$combined_gcov" 2>/dev/null)
            
            total_lines="${line_counts%,*}"
            covered_lines="${line_counts#*,}"
            
            if [[ -z "$total_lines" ]] || [[ ! "$total_lines" =~ ^[0-9]+$ ]]; then
                total_lines=0
            fi
            
            if [[ -z "$covered_lines" ]] || [[ ! "$covered_lines" =~ ^[0-9]+$ ]]; then
                covered_lines=0
            fi
            
            # Count files individually for file statistics
            total_files=${#gcov_files_to_process[@]}
            covered_files=0
            for gcov_file in "${gcov_files_to_process[@]}"; do
                file_covered_lines=$(grep -c "^[[:space:]]*[1-9][0-9]*:[[:space:]]*[0-9][0-9]*:" "$gcov_file" 2>/dev/null)
                if [[ -z "$file_covered_lines" ]] || [[ ! "$file_covered_lines" =~ ^[0-9]+$ ]]; then
                    file_covered_lines=0
                fi
                if [[ $file_covered_lines -gt 0 ]]; then
                    covered_files=$((covered_files + 1))
                fi
            done
            
            # Clean up temporary file
            rm -f "$combined_gcov"
        fi
        
        # Display detailed coverage information and calculate percentage
        if [[ $total_files -gt 0 ]]; then
            print_message "Files instrumented: $covered_files of $total_files source files have coverage"
            print_message "Lines instrumented: $covered_lines of $total_lines executable lines covered"
            
            # Calculate actual percentage from our analysis
            if [[ $total_lines -gt 0 ]]; then
                actual_percentage=$(awk "BEGIN {printf \"%.3f\", ($covered_lines / $total_lines) * 100}")
            else
                actual_percentage="0.000"
            fi
        else
            actual_percentage="0.000"
        fi
        
        print_result 0 "Unity test coverage calculated: $actual_percentage%"
        ((PASS_COUNT++))
        TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + 1))
    else
        print_result 1 "Failed to calculate Unity test coverage"
        EXIT_CODE=1
        TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + 1))
    fi
else
    print_error "Compilation failed, skipping test execution"
    EXIT_CODE=1
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test "$EXIT_CODE" "$TOTAL_SUBTESTS" "$PASS_COUNT" > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
