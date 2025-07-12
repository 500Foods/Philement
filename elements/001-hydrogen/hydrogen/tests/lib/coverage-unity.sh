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
    
    # Generate gcov files more efficiently using parallel processing
    if command -v xargs >/dev/null 2>&1; then
        # Use xargs for parallel processing if available (fixed conflicting options)
        find . -name "*.gcno" -print0 | xargs -0 -P4 -I{} sh -c "
            gcno_dir=\"\$(dirname '{}')\"
            cd \"\$gcno_dir\" && gcov \"\$(basename '{}')\" >/dev/null 2>&1
        "
    else
        # Fallback to optimized sequential processing
        find . -name "*.gcno" -exec sh -c '
            gcno_dir="$(dirname "$1")"
            cd "$gcno_dir" && gcov "$(basename "$1")" >/dev/null 2>&1
        ' _ {} \;
    fi
    
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
    
    # Build list of valid gcov files first, then process them all at once
    local valid_gcov_files=()
    while IFS= read -r gcov_file; do
        if [[ -f "$gcov_file" ]]; then
            local filename
            filename=$(basename "$gcov_file")
            
            # Skip external libraries and test framework files (these are not project source files)
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
                
                # Use centralized .trial-ignore checking for consistency
                if should_ignore_file "$test_path" "$project_root"; then
                    continue
                fi
                
                valid_gcov_files+=("$gcov_file")
                instrumented_files=$((instrumented_files + 1))
            fi
        fi
    done < <(find "$build_dir" -name "*.gcov" -type f 2>/dev/null)
    
    # Process all valid gcov files at once using cat for maximum efficiency
    if [[ ${#valid_gcov_files[@]} -gt 0 ]]; then
        local combined_data
        combined_data=$(cat "${valid_gcov_files[@]}" 2>/dev/null | awk '
            /^[[:space:]]*[0-9#-].*:/ { total++ }
            /^[[:space:]]*[1-9][0-9]*.*:/ { covered++ }
            END { print total "," covered }
        ')
        
        total_lines="${combined_data%,*}"
        covered_lines="${combined_data#*,}"
        
        # Default to 0 if parsing failed
        total_lines=${total_lines:-0}
        covered_lines=${covered_lines:-0}
        
        # Count covered files by checking each file individually for any coverage
        for gcov_file in "${valid_gcov_files[@]}"; do
            if grep -q "^[[:space:]]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null; then
                covered_files=$((covered_files + 1))
            fi
        done
    fi
    
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
