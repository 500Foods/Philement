#!/bin/bash
#
# Hydrogen Test Runner
# Executes all tests with standardized formatting and generates a summary report
#
# Usage: ./test_all.sh [test_name|min|max|all] [--skip-tests]

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"
source "$SCRIPT_DIR/support_cleanup.sh"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Get timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_LOG="$RESULTS_DIR/test_summary_${TIMESTAMP}.log"

# Arrays to track all tests
declare -a ALL_TEST_NAMES
declare -a ALL_TEST_RESULTS
declare -a ALL_TEST_DETAILS
declare -a ALL_TEST_SUBTESTS
declare -a ALL_TEST_PASSED_SUBTESTS
declare -a ALL_TEST_DURATIONS

# Start time for test runtime calculation
START_TIME=$(date +%s)

# Print header
print_header "Hydrogen Test Runner" | tee "$SUMMARY_LOG"
echo "Started at: $(date)" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Clean up before starting tests
cleanup_old_tests
ensure_server_not_running

# Make all test scripts executable
make_scripts_executable

# Function to register a test without executing it
register_test_without_executing() {
    local test_script="$1"
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    local relative_script=$(convert_to_relative_path "$test_script")
    
    print_header "Skipping Test: $test_name" | tee -a "$SUMMARY_LOG"
    print_info "Test would run command: $relative_script" | tee -a "$SUMMARY_LOG"
    
    # Default to 1 subtest
    local total_subtests=1
    local passed_subtests=0
    
    # Record skipped test
    print_result 2 "Test $test_name skipped" | tee -a "$SUMMARY_LOG"
    ALL_TEST_NAMES+=("$test_name")
    ALL_TEST_RESULTS+=(2) # 2 indicates skipped
    ALL_TEST_DETAILS+=("Test was skipped")
    ALL_TEST_SUBTESTS+=($total_subtests)
    ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
    
    echo "" | tee -a "$SUMMARY_LOG"
    return 0
}

# Function to format duration in HH:MM:SS
format_duration() {
    local duration=$1
    local hours=$((duration / 3600))
    local minutes=$(((duration % 3600) / 60))
    local seconds=$((duration % 60))
    printf "%02d:%02d:%02d" $hours $minutes $seconds
}

# Function to run a specific test script
run_test() {
    local test_script="$1"
    
    if [ "$SKIP_TESTS" = true ]; then
        register_test_without_executing "$test_script"
        return 0
    fi
    
    local test_start_time=$(date +%s)
    
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    
    print_header "Running Test: $test_name" | tee -a "$SUMMARY_LOG"
    
    # Execute the test script
    print_command "$test_script" | tee -a "$SUMMARY_LOG"
    $test_script
    local test_exit_code=$?
    
    # Check for subtest results file
    local subtest_file="$RESULTS_DIR/subtest_${test_name}.txt"
    local total_subtests=1
    local passed_subtests=0
    
    if [ -f "$subtest_file" ]; then
        # Read subtest results
        IFS=',' read -r total_subtests passed_subtests < "$subtest_file"
        print_info "Found subtest results: $passed_subtests of $total_subtests subtests passed" | tee -a "$SUMMARY_LOG"
        # Keep the file for debugging
        mv "$subtest_file" "${subtest_file}.${TIMESTAMP}"
    else
        # Default to 1 subtest
        total_subtests=1
        if [ $test_exit_code -eq 0 ]; then
            passed_subtests=1
        else
            passed_subtests=0
        fi
        # Create a subtest file for debugging
        echo "${total_subtests},${passed_subtests}" > "${subtest_file}.${TIMESTAMP}"
    fi
    
    # Record test results
    local test_end_time=$(date +%s)
    local test_duration=$((test_end_time - test_start_time))
    
    if [ $test_exit_code -eq 0 ]; then
        print_result 0 "Test $test_name completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("Test completed without errors")
        ALL_TEST_SUBTESTS+=($total_subtests)
        ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
        ALL_TEST_DURATIONS+=($test_duration)
    else
        print_result 1 "Test $test_name failed with exit code $test_exit_code" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("Test failed with errors")
        ALL_TEST_SUBTESTS+=($total_subtests)
        ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
        ALL_TEST_DURATIONS+=($test_duration)
        
        # Look for the most recent log file for this test
        local log_pattern="*$(echo $test_name | tr '_' '*')*.log"
        local latest_log=$(find "$RESULTS_DIR" -type f -name "$log_pattern" | sort -r | head -1)
        if [ -n "$latest_log" ] && [ -f "$latest_log" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$latest_log" | tee -a "$SUMMARY_LOG"
            echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    
    # Only check for running server when not in skip mode
    if [ "$SKIP_TESTS" = false ]; then
        ensure_server_not_running
    fi
    
    return $test_exit_code
}

# Function to register a test with configuration without executing it
register_test_with_config_without_executing() {
    local test_script="$1"
    local config_file="$2"
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    local config_name=$(basename "$config_file" .json)
    local config_path=$(get_config_path "$config_file")
    local relative_script=$(convert_to_relative_path "$test_script")
    
    print_header "Skipping Test: $test_name with config $config_name" | tee -a "$SUMMARY_LOG"
    print_info "Test would run command: $relative_script $config_path" | tee -a "$SUMMARY_LOG"
    
    # Default to 1 subtest
    local total_subtests=1
    local passed_subtests=0
    
    # Record skipped test
    print_result 2 "Test $test_name with $config_name skipped" | tee -a "$SUMMARY_LOG"
    ALL_TEST_NAMES+=("$test_name ($config_name)")
    ALL_TEST_RESULTS+=(2) # 2 indicates skipped
    ALL_TEST_DETAILS+=("Test was skipped")
    ALL_TEST_SUBTESTS+=($total_subtests)
    ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
    
    echo "" | tee -a "$SUMMARY_LOG"
    return 0
}

# Function to run a specific test with configuration
run_test_with_config() {
    local test_script="$1"
    local config_file="$2"
    
    if [ "$SKIP_TESTS" = true ]; then
        register_test_with_config_without_executing "$test_script" "$config_file"
        return 0
    fi
    
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    local config_name=$(basename "$config_file" .json)
    local config_path=$(get_config_path "$config_file")
    
    local test_start_time=$(date +%s)
    
    print_header "Running Test: $test_name with config $config_name" | tee -a "$SUMMARY_LOG"
    
    # Execute the test script with config file
    print_command "$test_script $config_path" | tee -a "$SUMMARY_LOG"
    $test_script "$config_path"
    local test_exit_code=$?
    
    # Check for subtest results file
    local subtest_file="$RESULTS_DIR/subtest_${test_name}.txt"
    local total_subtests=1
    local passed_subtests=0
    
    if [ -f "$subtest_file" ]; then
        # Read subtest results
        IFS=',' read -r total_subtests passed_subtests < "$subtest_file"
        print_info "Found subtest results: $passed_subtests of $total_subtests subtests passed" | tee -a "$SUMMARY_LOG"
        # Keep the file for debugging
        mv "$subtest_file" "${subtest_file}.${TIMESTAMP}"
    else
        # Default to 1 subtest
        total_subtests=1
        if [ $test_exit_code -eq 0 ]; then
            passed_subtests=1
        else
            passed_subtests=0
        fi
        # Create a subtest file for debugging
        echo "${total_subtests},${passed_subtests}" > "${subtest_file}.${TIMESTAMP}"
    fi
    
    # Record test results
    local test_end_time=$(date +%s)
    local test_duration=$((test_end_time - test_start_time))
    
    if [ $test_exit_code -eq 0 ]; then
        print_result 0 "Test $test_name with $config_name completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name ($config_name)")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("Test completed without errors")
        ALL_TEST_SUBTESTS+=($total_subtests)
        ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
        ALL_TEST_DURATIONS+=($test_duration)
    else
        print_result 1 "Test $test_name with $config_name failed with exit code $test_exit_code" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name ($config_name)")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("Test failed with errors")
        ALL_TEST_SUBTESTS+=($total_subtests)
        ALL_TEST_PASSED_SUBTESTS+=($passed_subtests)
        ALL_TEST_DURATIONS+=($test_duration)
        
        # Look for the most recent log file for this test
        local log_pattern="*$(echo $test_name | tr '_' '*')*.log"
        local latest_log=$(find "$RESULTS_DIR" -type f -name "$log_pattern" | sort -r | head -1)
        if [ -n "$latest_log" ] && [ -f "$latest_log" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$latest_log" | tee -a "$SUMMARY_LOG"
            echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    
    # Only check for running server when not in skip mode
    if [ "$SKIP_TESTS" = false ]; then
        ensure_server_not_running
    fi
    
    return $test_exit_code
}

# Function to discover and run all tests
run_all_tests() {
    # Run compilation test first - it's critical
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_10_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed with other tests if compilation passes
    if [ $compilation_exit_code -ne 0 ] && [ "$SKIP_TESTS" = false ]; then
        print_warning "Compilation failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Find and run all test scripts in order
    local test_scripts=$(find "$SCRIPT_DIR" -type f -name "test_*.sh" | grep -v "test_00_all.sh" | grep -v "test_10_compilation.sh" | grep -v "test_template.sh" | sort)
    
    local all_exit_codes=0
    for test_script in $test_scripts; do
        
        run_test "$test_script"
        local test_exit_code=$?
        
        if [ $test_exit_code -ne 0 ] && [ "$SKIP_TESTS" = false ]; then
            all_exit_codes=1
        fi
        
        # Only ensure server is not running if not in skip mode
        if [ "$SKIP_TESTS" = false ]; then
            ensure_server_not_running
        fi
        echo "" | tee -a "$SUMMARY_LOG"
    done
    
    # Calculate overall result
    if [ "$SKIP_TESTS" = true ]; then
        return 0
    elif [ $compilation_exit_code -eq 0 ] && [ $all_exit_codes -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Function to run minimal configuration test only
run_min_configuration_test() {
    # First run environment variable test
    print_header "Running Environment Variable Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_12_env_payload.sh"
    local env_exit_code=$?
    
    # Only proceed if environment variables are properly set
    if [ $env_exit_code -ne 0 ]; then
        print_warning "Environment variable test failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
        return $env_exit_code
    fi

    # Run compilation test next
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_10_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed if compilation passes
    if [ $compilation_exit_code -ne 0 ]; then
        print_warning "Compilation failed - skipping startup/shutdown test" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Run startup/shutdown test with minimal configuration
    print_header "Running Startup/Shutdown Test (Min Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_15_startup_shutdown.sh" "hydrogen_test_min.json"
    local min_exit_code=$?
    
    return $min_exit_code
}

# Function to run maximal configuration test only
run_max_configuration_test() {
    # First run environment variable test
    print_header "Running Environment Variable Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_12_env_payload.sh"
    local env_exit_code=$?
    
    # Only proceed if environment variables are properly set
    if [ $env_exit_code -ne 0 ]; then
        print_warning "Environment variable test failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
        return $env_exit_code
    fi

    # Run compilation test next
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_10_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed if compilation passes
    if [ $compilation_exit_code -ne 0 ]; then
        print_warning "Compilation failed - skipping startup/shutdown test" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Run startup/shutdown test with maximal configuration
    print_header "Running Startup/Shutdown Test (Max Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_15_startup_shutdown.sh" "hydrogen_test_max.json"
    local max_exit_code=$?
    
    return $max_exit_code
}

# Function to run a specific named test
run_specific_test() {
    local test_name="$1"
    
    # Check if the test script exists
    if [ -f "$SCRIPT_DIR/test_${test_name}.sh" ]; then
        # Skip all tests if SKIP_TESTS is true
        if [ "$SKIP_TESTS" = true ]; then
            print_header "Skipping Requested Test: $test_name" | tee -a "$SUMMARY_LOG"
            register_test_without_executing "$SCRIPT_DIR/test_${test_name}.sh"
            return 0
        fi
        
        # Run environment variable test first if it's not the test being requested
        if [ "$test_name" != "05_env_payload" ]; then
            print_header "Running Environment Variable Test" | tee -a "$SUMMARY_LOG"
            run_test "$SCRIPT_DIR/test_12_env_payload.sh"
            local env_exit_code=$?
            
            # Only proceed if environment variables are properly set
            if [ $env_exit_code -ne 0 ]; then
                print_warning "Environment variable test failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
                return $env_exit_code
            fi

            # Run compilation test next if it's not the test being requested
            if [ "$test_name" != "10_compilation" ]; then
                print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
                run_test "$SCRIPT_DIR/test_10_compilation.sh"
                local compilation_exit_code=$?
                
                # Only proceed if compilation passes
                if [ $compilation_exit_code -ne 0 ]; then
                    print_warning "Compilation failed - skipping $test_name test" | tee -a "$SUMMARY_LOG"
                    return $compilation_exit_code
                fi
            fi
        fi
        
        # Only proceed if compilation defined and passes
        if [ -n "$compilation_exit_code" ] && [ $compilation_exit_code -ne 0 ]; then
            print_warning "Compilation failed - skipping $test_name test" | tee -a "$SUMMARY_LOG"
            return $compilation_exit_code
        fi
        
        # Run the requested test
        print_header "Running Requested Test: $test_name" | tee -a "$SUMMARY_LOG"
        run_test "$SCRIPT_DIR/test_${test_name}.sh"
        return $?
    else
        print_result 1 "Test script not found: test_${test_name}.sh" | tee -a "$SUMMARY_LOG"
        return 1
    fi
}

# Custom print_test_item function to handle skipped tests with more compact output
print_test_item() {
    local result=$1
    local test_name=$2
    local details=$3
    local duration=$4
    
    # Format the duration if provided, otherwise use empty string
    local duration_str=""
    if [ -n "$duration" ]; then
        duration_str="${NC}$(format_duration $duration)${GREEN} "
    fi
    
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}${PASS_ICON} ${duration_str}${test_name}${NC} - ${details}"
    elif [ $result -eq 2 ]; then
        echo -e "${YELLOW}⏭️  ${duration_str}${test_name}${NC} - ${details}"
    else
        echo -e "${RED}${FAIL_ICON} ${duration_str}${test_name}${NC} - ${details}"
    fi
}

# Function to print summary statistics
print_summary_statistics() {
    local exit_code=$1
    
    # Calculate pass/fail/skip counts
    local pass_count=0
    local fail_count=0
    local skip_count=0
    local total_count=${#ALL_TEST_NAMES[@]}
    
    for result in "${ALL_TEST_RESULTS[@]}"; do
        if [ $result -eq 0 ]; then
            ((pass_count++))
        elif [ $result -eq 2 ]; then
            ((skip_count++))
        else
            ((fail_count++))
        fi
    done
    
    # Calculate end time and runtime
    local end_time=$(date +%s)
    local total_runtime=$((end_time - START_TIME))
    local runtime_min=$((total_runtime / 60))
    local runtime_sec=$((total_runtime % 60))
    local runtime_formatted="${runtime_min}m ${runtime_sec}s"
    
    # Print individual test results
    echo -e "\n${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$SUMMARY_LOG"
    for i in "${!ALL_TEST_NAMES[@]}"; do
        # Add subtest count to the details
        local total_subtests=${ALL_TEST_SUBTESTS[$i]}
        local passed_subtests=${ALL_TEST_PASSED_SUBTESTS[$i]}
        
        local subtest_info
        if [ ${ALL_TEST_RESULTS[$i]} -eq 2 ]; then
            subtest_info="(Skipped)"
        else
            subtest_info="(${passed_subtests} of ${total_subtests} Passed)"
        fi
        
        local test_info="${ALL_TEST_NAMES[$i]} ${subtest_info}"
        local duration_str=""
        if [ ${ALL_TEST_RESULTS[$i]} -ne 2 ]; then  # Don't show duration for skipped tests
            duration_str="${ALL_TEST_DURATIONS[$i]}"
        fi
        print_test_item ${ALL_TEST_RESULTS[$i]} "${test_info}" "${ALL_TEST_DETAILS[$i]}" "$duration_str" | tee -a "$SUMMARY_LOG"
    done
    
    # Calculate total subtests
    local total_subtests=0
    local total_passed_subtests=0
    for i in "${!ALL_TEST_SUBTESTS[@]}"; do
        if [ ${ALL_TEST_RESULTS[$i]} -ne 2 ]; then  # Don't count skipped tests
            total_subtests=$((total_subtests + ${ALL_TEST_SUBTESTS[$i]}))
            total_passed_subtests=$((total_passed_subtests + ${ALL_TEST_PASSED_SUBTESTS[$i]}))
        fi
    done
    local total_failed_subtests=$((total_subtests - total_passed_subtests))
    
    # Print comprehensive statistics 
    echo "" | tee -a "$SUMMARY_LOG"
    echo -e "${BLUE}${BOLD}Summary Statistics:${NC}" | tee -a "$SUMMARY_LOG"
    echo -e "Total tests: ${total_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Passed: ${pass_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Failed: ${fail_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Skipped: ${skip_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Total subtests: ${total_subtests}" | tee -a "$SUMMARY_LOG"
    echo -e "Passed subtests: ${total_passed_subtests}" | tee -a "$SUMMARY_LOG"
    echo -e "Failed subtests: ${total_failed_subtests}" | tee -a "$SUMMARY_LOG"
    echo -e "Test runtime: ${runtime_formatted}" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    
    if [ "$SKIP_TESTS" = true ]; then
        echo -e "${YELLOW}${BOLD}OVERALL RESULT: ALL TESTS SKIPPED${NC}" | tee -a "$SUMMARY_LOG"
    elif [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}" | tee -a "$SUMMARY_LOG"
    else
        echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}" | tee -a "$SUMMARY_LOG"
    fi
    echo -e "${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}" | tee -a "$SUMMARY_LOG"
    
    # Tips for additional diagnostics
    echo "" | tee -a "$SUMMARY_LOG"
    echo "Diagnostic Tools:" | tee -a "$SUMMARY_LOG"
    echo "  • tests/support_analyze_stuck_threads.sh <pid>   - Analyze thread deadlocks or hangs" | tee -a "$SUMMARY_LOG"
    echo "  • tests/support_monitor_resources.sh <pid> [sec] - Monitor CPU/memory usage of a process" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    
    # Always generate README section
    generate_readme_section
}

# Function to run cloc and generate repository information
generate_repository_info() {
    local repo_info_file="$RESULTS_DIR/repository_info.md"
    
    echo "## Repository Information" > "$repo_info_file"
    echo "" >> "$repo_info_file"
    echo "Generated via cloc: $(date)" >> "$repo_info_file"
    echo "" >> "$repo_info_file"
    
    # Save current directory
    local start_dir=$(pwd)
    
    # Change to project root directory
    cd "$SCRIPT_DIR/.." || {
        print_warning "Failed to change to project directory for repository analysis" | tee -a "$SUMMARY_LOG"
        return 1
    }
    
    # Run cloc with specific locale settings to ensure consistent output
    local cloc_output=$(mktemp)
    if env LC_ALL=en_US.UTF_8 LC_TIME= LC_ALL= LANG= LANGUAGE= cloc . --quiet --force-lang="C,inc" > "$cloc_output" 2>&1; then
        # No Code Summary subtitle per user feedback
        echo "| Language | Files | Blank Lines | Comment Lines | Code Lines |" >> "$repo_info_file"
        echo "| -------- | ----- | ----------- | ------------- | ---------- |" >> "$repo_info_file"
        
        # Extract each language row
        grep -v "SUM:" "$cloc_output" | grep -v "^-" | grep -v "Language" | tail -n +2 | while read -r line; do
            # Parse the line
            local lang=$(echo $line | awk '{print $1}')
            local files=$(echo $line | awk '{print $2}')
            local blank=$(echo $line | awk '{print $3}')
            local comment=$(echo $line | awk '{print $4}')
            local code=$(echo $line | awk '{print $5}')
            
            # Skip empty lines
            if [ -z "$lang" ]; then
                continue
            fi
            
            # Add to markdown table
            echo "| $lang | $files | $blank | $comment | $code |" >> "$repo_info_file"
        done
        
        # Add the SUM row
        local sum_line=$(grep "SUM:" "$cloc_output")
        local total_files=$(echo $sum_line | awk '{print $2}')
        local total_blank=$(echo $sum_line | awk '{print $3}')
        local total_comment=$(echo $sum_line | awk '{print $4}')
        local total_code=$(echo $sum_line | awk '{print $5}')
        
        echo "| **Total** | **$total_files** | **$total_blank** | **$total_comment** | **$total_code** |" >> "$repo_info_file"
        
        print_info "Generated repository information with cloc analysis" | tee -a "$SUMMARY_LOG"
    else
        echo "### Code Summary" >> "$repo_info_file"
        echo "" >> "$repo_info_file"
        echo "Unable to generate code statistics. Make sure 'cloc' is installed." >> "$repo_info_file"
        print_warning "Failed to run cloc analysis for repository information" | tee -a "$SUMMARY_LOG"
    fi
    
    rm -f "$cloc_output"
    
    # Return to start directory
    cd "$start_dir" || {
        print_warning "Failed to return to start directory after repository analysis" | tee -a "$SUMMARY_LOG"
    }
    
    return 0
}

# Function to generate README section with test results
generate_readme_section() {
    local readme_section_file="$RESULTS_DIR/latest_test_results.md"
    
    echo "## Latest Test Results" > "$readme_section_file"
    echo "" >> "$readme_section_file"
    echo "Generated on: $(date)" >> "$readme_section_file"
    echo "" >> "$readme_section_file"
    
    echo "### Summary" >> "$readme_section_file"
    echo "" >> "$readme_section_file"
    echo "| Metric | Value |" >> "$readme_section_file"
    echo "| ------ | ----- |" >> "$readme_section_file"
    
    # Calculate pass/fail/skip counts
    local pass_count=0
    local fail_count=0
    local skip_count=0
    local total_count=${#ALL_TEST_NAMES[@]}
    
    for result in "${ALL_TEST_RESULTS[@]}"; do
        if [ $result -eq 0 ]; then
            ((pass_count++))
        elif [ $result -eq 2 ]; then
            ((skip_count++))
        else
            ((fail_count++))
        fi
    done
    
    # Calculate total subtests
    local total_subtests=0
    local total_passed_subtests=0
    for i in "${!ALL_TEST_SUBTESTS[@]}"; do
        if [ ${ALL_TEST_RESULTS[$i]} -ne 2 ]; then  # Don't count skipped tests
            total_subtests=$((total_subtests + ${ALL_TEST_SUBTESTS[$i]}))
            total_passed_subtests=$((total_passed_subtests + ${ALL_TEST_PASSED_SUBTESTS[$i]}))
        fi
    done
    local total_failed_subtests=$((total_subtests - total_passed_subtests))
    
    # Calculate end time and runtime
    local end_time=$(date +%s)
    local total_runtime=$((end_time - START_TIME))
    local runtime_min=$((total_runtime / 60))
    local runtime_sec=$((total_runtime % 60))
    local runtime_formatted="${runtime_min}m ${runtime_sec}s"
    
    echo "| Total Tests | ${total_count} |" >> "$readme_section_file"
    echo "| Passed | ${pass_count} |" >> "$readme_section_file"
    echo "| Failed | ${fail_count} |" >> "$readme_section_file"
    echo "| Skipped | ${skip_count} |" >> "$readme_section_file"
    echo "| Total Subtests | ${total_subtests} |" >> "$readme_section_file"
    echo "| Passed Subtests | ${total_passed_subtests} |" >> "$readme_section_file"
    echo "| Failed Subtests | ${total_failed_subtests} |" >> "$readme_section_file"
    echo "| Runtime | ${runtime_formatted} |" >> "$readme_section_file"
    
    echo "" >> "$readme_section_file"
    echo "### Individual Test Results" >> "$readme_section_file"
    echo "" >> "$readme_section_file"
    echo "| Test | Status | Details |" >> "$readme_section_file"
    echo "| ---- | ------ | ------- |" >> "$readme_section_file"
    
    for i in "${!ALL_TEST_NAMES[@]}"; do
        local status
        if [ ${ALL_TEST_RESULTS[$i]} -eq 0 ]; then
            status="✅ Passed"
        elif [ ${ALL_TEST_RESULTS[$i]} -eq 2 ]; then
            status="⏭️ Skipped"
        else
            status="❌ Failed"
        fi
        
        local subtest_info
        if [ ${ALL_TEST_RESULTS[$i]} -eq 2 ]; then
            subtest_info=""
        else
            subtest_info="(${ALL_TEST_PASSED_SUBTESTS[$i]} of ${ALL_TEST_SUBTESTS[$i]} subtests passed)"
        fi
        
        echo "| ${ALL_TEST_NAMES[$i]} | ${status} | ${ALL_TEST_DETAILS[$i]} ${subtest_info} |" >> "$readme_section_file"
    done
    
    # No update instructions as per user feedback
    
    local relative_path=$(convert_to_relative_path "$readme_section_file")
    print_info "Prepared test results section in $relative_path" | tee -a "$SUMMARY_LOG"
    
    # Generate repository information as well
    generate_repository_info
    local repo_info_file="$RESULTS_DIR/repository_info.md"
    local repo_info_path=$(convert_to_relative_path "$repo_info_file")
    print_info "Prepared repository statistics from cloc in $repo_info_path" | tee -a "$SUMMARY_LOG"
    
    # Add to README.md if requested
    local readme_file="$SCRIPT_DIR/../README.md"
    # Use a simpler path for display purposes
    local readme_path="hydrogen/README.md"
    
    if [ -f "$readme_file" ]; then
        # Check if "Latest Test Results" section already exists
        if grep -q "^## Latest Test Results" "$readme_file"; then
            # Remove existing sections (both test results and repository info)
            local temp_file=$(mktemp)
            sed '/^## Latest Test Results/,/^## /{ /^## Latest Test Results/d; /^## /!d; }' "$readme_file" > "$temp_file"
            sed -i '/^## Repository Information/,/^## /{ /^## Repository Information/d; /^## /!d; }' "$temp_file"
            mv "$temp_file" "$readme_file"
        fi
        
        # Append new sections
        cat "$readme_section_file" >> "$readme_file"
        echo "" >> "$readme_file"
        cat "$repo_info_file" >> "$readme_file"
        print_result 0 "Added test results and repository statistics to $readme_path" | tee -a "$SUMMARY_LOG"
    else
        print_warning "README.md not found at $readme_file" | tee -a "$SUMMARY_LOG"
    fi
}

# Parse command line arguments
TEST_TYPE=${1:-"all"}  # Default to "all" if not specified
SKIP_TESTS=false
UPDATE_README=true  # Always update README by default

# Check if --skip-tests is provided as any argument
for arg in "$@"; do
    if [ "$arg" == "--skip-tests" ]; then
        SKIP_TESTS=true
    fi
done

# If the first argument is --skip-tests, set TEST_TYPE to "all"
if [ "$TEST_TYPE" == "--skip-tests" ]; then
    TEST_TYPE="all"
fi

# Run tests based on command line argument
case "$TEST_TYPE" in
    "min")
        print_info "Running test with minimal configuration only" | tee -a "$SUMMARY_LOG"
        run_min_configuration_test
        EXIT_CODE=$?
        ;;
    "max")
        print_info "Running test with maximal configuration only" | tee -a "$SUMMARY_LOG"
        run_max_configuration_test
        EXIT_CODE=$?
        ;;
    "all")
        print_info "Running all tests" | tee -a "$SUMMARY_LOG"
        if [ "$SKIP_TESTS" = true ]; then
            print_info "Skipping actual test execution (--skip-tests enabled)" | tee -a "$SUMMARY_LOG"
        fi
        run_all_tests
        EXIT_CODE=$?
        ;;
    *)
        # Check if it's a specific test name
        if [ -f "$SCRIPT_DIR/test_${TEST_TYPE}.sh" ]; then
            print_info "Running specific test: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            if [ "$SKIP_TESTS" = true ]; then
                print_info "Skipping actual test execution (--skip-tests enabled)" | tee -a "$SUMMARY_LOG"
            fi
            run_specific_test "$TEST_TYPE"
            EXIT_CODE=$?
        else
            print_warning "Invalid test type or test name: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            echo "Usage: $0 [test_name|min|max|all] [--skip-tests]" | tee -a "$SUMMARY_LOG"
            echo "  test_name: Run a specific test (e.g., compilation, startup_shutdown)" | tee -a "$SUMMARY_LOG"
            echo "  min: Run with minimal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  max: Run with maximal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  all: Run all tests (default)" | tee -a "$SUMMARY_LOG"
            echo "  --skip-tests: Skip actual test execution, just show what tests would run" | tee -a "$SUMMARY_LOG"
            EXIT_CODE=1
        fi
        ;;
esac

# Print overall summary
print_header "Test Summary" | tee -a "$SUMMARY_LOG"
echo "Completed at: $(date)" | tee -a "$SUMMARY_LOG"
echo "Summary log: $(convert_to_relative_path "$SUMMARY_LOG")" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Print summary statistics
print_summary_statistics $EXIT_CODE

# Clean up any leftover log files in the test directory (not in results)
rm -f "$SCRIPT_DIR"/*.log

exit $EXIT_CODE