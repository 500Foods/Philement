#!/bin/bash
#
# coverage-blackbox.sh - Blackbox test coverage calculation and analysis
#
# This script provides functions for calculating blackbox test coverage,
# combined coverage analysis, and identifying uncovered files.
#

# Source common utilities  
# Don't redefine SCRIPT_DIR if it's already set by the calling script
COVERAGE_BLACKBOX_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$COVERAGE_BLACKBOX_SCRIPT_DIR/coverage-common.sh"

# Function to collect blackbox test coverage
# Usage: collect_blackbox_coverage <hydrogen_coverage_binary> <timestamp>
# Returns: Coverage percentage written to blackbox_coverage.txt
collect_blackbox_coverage() {
    local hydrogen_coverage_bin="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [ ! -f "$hydrogen_coverage_bin" ]; then
        echo "0.000" > "$BLACKBOX_COVERAGE_FILE"
        return 1
    fi
    
    # Note: hydrogen_coverage is a server binary, not a utility
    # For blackbox coverage, we need to convert .gcda files to .gcov files
    local original_dir="$PWD"
    local project_root="$original_dir"
    
    # Find the directory where hydrogen_coverage was built (contains .gcno and .gcda files)
    local build_dir
    build_dir="$(dirname "$hydrogen_coverage_bin")"
    
    # Generate fresh gcov files from accumulated .gcda files (blackbox test results)
    cd "$build_dir" || return 1
    
    # Find .gcda files and ensure corresponding .gcno files exist
    local gcda_files=()
    mapfile -t gcda_files < <(find . -name "*.gcda" -type f)
    
    if [ ${#gcda_files[@]} -eq 0 ]; then
        echo "No .gcda files found in $build_dir" >&2
        cd "$original_dir" || return 1
        echo "0.000" > "$BLACKBOX_COVERAGE_FILE"
        return 1
    fi
    
    # Generate gcov files for all .gcda files (place them in same directories as .gcno/.gcda files)
    find . -name "*.gcda" | while IFS= read -r gcda_file; do
        local gcda_dir
        gcda_dir="$(dirname "$gcda_file")"
        local original_pwd="$PWD"
        cd "$gcda_dir" || continue
        gcov "$(basename "$gcda_file")" > /dev/null 2>&1
        cd "$original_pwd" || return 1
    done
    
    # Return to original directory
    cd "$original_dir" || return 1
    
    # Parse gcov output similar to Unity but for blackbox tests
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    # Process project source files against gcov data, similar to Unity coverage approach
    while IFS= read -r source_file; do
        if should_ignore_file "$source_file" "$project_root"; then
            continue
        fi
        
        local source_basename
        source_basename=$(basename "$source_file")
        local gcov_filename="${source_basename}.gcov"
        
        # Look for corresponding gcov file in build directory
        local gcov_file=""
        while IFS= read -r potential_gcov; do
            if [[ "$(basename "$potential_gcov")" == "$gcov_filename" ]]; then
                gcov_file="$potential_gcov"
                break
            fi
        done < <(find "$build_dir" -name "$gcov_filename" -type f 2>/dev/null)
        
        if [[ -f "$gcov_file" ]]; then
            # Extract lines executed and total lines from gcov file
            local file_total
            local file_covered
            file_total=$(grep -c "^[[:space:]]*[0-9#-].*:" "$gcov_file" 2>/dev/null || echo "0")
            file_covered=$(grep -c "^[[:space:]]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null || echo "0")
            
            # Ensure values are clean integers
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
    done < <(find "$project_root/src" -name "*.c" -type f 2>/dev/null)
    
    # Calculate coverage percentage with 3 decimal places
    if [[ $total_lines -gt 0 ]]; then
        coverage_percentage=$(awk "BEGIN {printf \"%.3f\", ($covered_lines / $total_lines) * 100}")
    fi
    
    # Store coverage result with timestamp
    echo "$coverage_percentage" > "$BLACKBOX_COVERAGE_FILE"
    echo "$timestamp,$coverage_percentage,$covered_lines,$total_lines,$instrumented_files,$covered_files" > "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    echo "$coverage_percentage"
    return 0
}

# Function to calculate combined coverage from Unity and blackbox tests
# Usage: calculate_combined_coverage
# Returns: Combined coverage percentage
calculate_combined_coverage() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local combined_coverage="0.000"
    
    # Read coverage data from files
    if [ -f "$UNITY_COVERAGE_FILE" ]; then
        unity_coverage=$(cat "$UNITY_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    if [ -f "$BLACKBOX_COVERAGE_FILE" ]; then
        blackbox_coverage=$(cat "$BLACKBOX_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    # For combined coverage, we need to analyze the actual gcov files to avoid double-counting
    # This is a simplified approach - take the maximum of the two coverage percentages
    # In practice, combined coverage could be higher if tests cover different parts
    if [[ $(awk "BEGIN {print ($unity_coverage > $blackbox_coverage)}") -eq 1 ]]; then
        combined_coverage="$unity_coverage"
    else
        combined_coverage="$blackbox_coverage"
    fi
    
    # Store combined coverage result
    echo "$combined_coverage" > "$COMBINED_COVERAGE_FILE"
    
    echo "$combined_coverage"
    return 0
}

# Function to calculate coverage overlap between Unity and blackbox tests
# Usage: calculate_coverage_overlap
# Returns: Overlap percentage between the two test types
calculate_coverage_overlap() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local overlap_percentage="0.000"
    
    # Read coverage data from files
    if [ -f "$UNITY_COVERAGE_FILE" ]; then
        unity_coverage=$(cat "$UNITY_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    if [ -f "$BLACKBOX_COVERAGE_FILE" ]; then
        blackbox_coverage=$(cat "$BLACKBOX_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    # Calculate overlap as the minimum of the two coverage percentages
    # This represents the files/lines that are covered by both test types
    if [[ $(awk "BEGIN {print ($unity_coverage < $blackbox_coverage)}") -eq 1 ]]; then
        overlap_percentage="$unity_coverage"
    else
        overlap_percentage="$blackbox_coverage"
    fi
    
    # Store overlap coverage result
    echo "$overlap_percentage" > "$OVERLAP_COVERAGE_FILE"
    
    echo "$overlap_percentage"
    return 0
}

# Function to identify uncovered source files
# Usage: identify_uncovered_files <project_root>
# Returns: Analysis of covered and uncovered source files
identify_uncovered_files() {
    local project_root="$1"
    local covered_count=0
    local uncovered_count=0
    local total_count=0
    local uncovered_files=()
    
    # Find potential build directories to check for gcov files
    local build_dirs=()
    build_dirs+=("$project_root/build_unity")
    build_dirs+=("$project_root/build")
    build_dirs+=("$project_root")  # In case gcov files are in project root
    
    # Get all source files in the project
    while IFS= read -r source_file; do
        if should_ignore_file "$source_file" "$project_root"; then
            continue
        fi
        
        total_count=$((total_count + 1))
        
        # Check if this source file has corresponding gcov coverage in any build directory
        local source_basename
        source_basename=$(basename "$source_file")
        local found_coverage=false
        local file_covered=0
        
        for build_dir in "${build_dirs[@]}"; do
            if [[ -d "$build_dir" ]]; then
                # Look for gcov files in subdirectories (since they're now placed alongside .gcno/.gcda files)
                while IFS= read -r gcov_file; do
                    if [[ "$(basename "$gcov_file")" == "${source_basename}.gcov" ]]; then
                        # Check if the file has any coverage
                        file_covered=$(grep -c "^[[:space:]]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null || echo "0")
                        # Clean the variable to remove any whitespace/newlines
                        file_covered=$(echo "$file_covered" | tr -d '\n\r\t ' | grep -o '[0-9]*' | head -1)
                        file_covered=${file_covered:-0}
                        
                        if [[ $file_covered -gt 0 ]]; then
                            found_coverage=true
                            break 2  # Break out of both loops
                        fi
                    fi
                done < <(find "$build_dir" -name "${source_basename}.gcov" -type f 2>/dev/null)
                
                if [[ "$found_coverage" == true ]]; then
                    break
                fi
            fi
        done
        
        if [[ "$found_coverage" == true ]]; then
            covered_count=$((covered_count + 1))
        else
            uncovered_count=$((uncovered_count + 1))
            uncovered_files+=("$source_file")
        fi
    done < <(find "$project_root/src" -name "*.c" -type f 2>/dev/null)
    
    # Output results in a structured format
    echo "COVERED_FILES_COUNT:$covered_count"
    echo "UNCOVERED_FILES_COUNT:$uncovered_count"
    echo "TOTAL_SOURCE_FILES:$total_count"
    echo "UNCOVERED_FILES:"
    
    # List uncovered files
    for file in "${uncovered_files[@]}"; do
        echo "$file"
    done
    
    return 0
}
