#!/bin/bash
#
# coverage-unity.sh - Unity test coverage calculation
#
# This script provides functions for calculating Unity test coverage from gcov data.
#

# Source common utilities  
# Don't redefine SCRIPT_DIR if it's already set by the calling script
COVERAGE_UNITY_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$COVERAGE_UNITY_SCRIPT_DIR/coverage-common.sh"

# Function to calculate Unity test coverage from gcov data
# Usage: calculate_unity_coverage <build_dir> <timestamp>
# Returns: Coverage percentage written to unity_coverage.txt
calculate_unity_coverage() {
    local build_dir="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [ ! -d "$build_dir" ]; then
        echo "0.000" > "$UNITY_COVERAGE_FILE"
        return 1
    fi
    
    # Change to build directory to process gcov files
    local original_dir="$PWD"
    cd "$build_dir" || return 1
    
    # Generate gcov files for all source files (place them in same directories as .gcno/.gcda files)
    find . -name "*.gcno" | while IFS= read -r gcno_file; do
        local gcno_dir
        gcno_dir="$(dirname "$gcno_file")"
        local original_pwd="$PWD"
        cd "$gcno_dir" || continue
        gcov "$(basename "$gcno_file")" > /dev/null 2>&1
        cd "$original_pwd" || return 1
    done
    
    # Return to original directory
    cd "$original_dir" || return 1
    
    # Parse gcov output to calculate overall coverage percentage
    local project_root="$original_dir/.."
    
    # Process gcov files to extract coverage data, excluding external libraries and ignored files
    # This gives us coverage on the actual project code, not external dependencies
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    # Process each gcov file (in build directory)
    while IFS= read -r gcov_file; do
        if [[ -f "$gcov_file" ]]; then
            local filename
            filename=$(basename "$gcov_file")
            
            # Skip external libraries and test framework files
            if [[ "$filename" == "unity.c.gcov" ]] || \
               [[ "$filename" == "jansson"*".gcov" ]] || \
               [[ "$filename" == "test_"*".gcov" ]] || \
               [[ "$gcov_file" == *"/usr/include/"* ]]; then
                continue
            fi
            
            # Process project source files only
            if [[ "$filename" == *.gcov ]]; then
                # Extract source filename and check if it should be ignored based on .trial-ignore
                local source_name
                local test_path
                source_name="${filename%.gcov}"
                test_path="$project_root/src/$source_name"
                
                if should_ignore_file "$test_path" "$project_root"; then
                    continue
                fi
                
                # Extract lines executed and total lines from gcov file
                local file_total
                local file_covered
                file_total=$(grep -c "^[[:space:]]*[0-9#-].*:" "$gcov_file" 2>/dev/null || echo "0")
                file_covered=$(grep -c "^[[:space:]]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null || echo "0")
                
                # Ensure values are clean integers (strip any whitespace/newlines)
                file_total=$(echo "$file_total" | tr -d '\n\r\t ' | grep -o '[0-9]*' | head -1)
                file_covered=$(echo "$file_covered" | tr -d '\n\r\t ' | grep -o '[0-9]*' | head -1)
                
                # Default to 0 if empty
                file_total=${file_total:-0}
                file_covered=${file_covered:-0}
                
                total_lines=$((total_lines + file_total))
                covered_lines=$((covered_lines + file_covered))
                instrumented_files=$((instrumented_files + 1))
                
                # Count as covered if any lines were executed
                if [[ $file_covered -gt 0 ]]; then
                    covered_files=$((covered_files + 1))
                fi
            fi
        fi
    done < <(find "$build_dir" -name "*.gcov" -type f 2>/dev/null)
    
    # Calculate coverage percentage with 3 decimal places
    if [[ $total_lines -gt 0 ]]; then
        coverage_percentage=$(awk "BEGIN {printf \"%.3f\", ($covered_lines / $total_lines) * 100}")
    fi
    
    # Store coverage result with timestamp
    echo "$coverage_percentage" > "$UNITY_COVERAGE_FILE"
    echo "$timestamp,$coverage_percentage,$covered_lines,$total_lines,$instrumented_files,$covered_files" > "${UNITY_COVERAGE_FILE}.detailed"
    
    echo "$coverage_percentage"
    return 0
}
