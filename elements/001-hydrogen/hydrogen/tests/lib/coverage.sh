#!/bin/bash
#
# coverage.sh - Test Coverage Calculation Functions
#
# This script provides functions for calculating and managing test coverage data
# for both Unity tests and blackbox tests in the Hydrogen project.
#
# VERSION HISTORY
# 1.0.0 - 2025-07-11 - Initial version with Unity and blackbox coverage functions
#

# Coverage data storage locations
COVERAGE_RESULTS_DIR="$SCRIPT_DIR/results"
GCOV_OUTPUT_DIR="$SCRIPT_DIR/gcov"
UNITY_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/unity_coverage.txt"
BLACKBOX_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/blackbox_coverage.txt"
COMBINED_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/combined_coverage.txt"
OVERLAP_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/overlap_coverage.txt"

# Ensure coverage directories exist
mkdir -p "$COVERAGE_RESULTS_DIR"
mkdir -p "$GCOV_OUTPUT_DIR"

# Global ignore patterns loaded once
IGNORE_PATTERNS_LOADED=""
IGNORE_PATTERNS=()

# Function to load ignore patterns from .trial-ignore file
# Usage: load_ignore_patterns <project_root>
load_ignore_patterns() {
    local project_root="$1"
    
    # Only load once
    if [[ "$IGNORE_PATTERNS_LOADED" == "true" ]]; then
        return 0
    fi
    
    IGNORE_PATTERNS=()
    if [ -f "$project_root/.trial-ignore" ]; then
        while IFS= read -r line; do
            # Skip comments and empty lines
            if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "${line// }" ]]; then
                continue
            fi
            # Remove leading ./ and add to patterns
            local pattern="${line#./}"
            if [[ -n "$pattern" ]]; then
                IGNORE_PATTERNS+=("$pattern")
            fi
        done < "$project_root/.trial-ignore"
    fi
    
    IGNORE_PATTERNS_LOADED="true"
    return 0
}

# Function to check if a file should be ignored
# Usage: should_ignore_file <file_path> <project_root>
should_ignore_file() {
    local file_path="$1"
    local project_root="$2"
    
    # Load patterns if not loaded
    load_ignore_patterns "$project_root"
    
    local relative_path="${file_path#$project_root/}"
    
    # Check if this file matches any ignore pattern
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        if [[ "$relative_path" == *"$pattern" ]]; then
            return 0  # Should ignore
        fi
    done
    
    return 1  # Should not ignore
}

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
    
    # Generate gcov files for all source files and move them to gcov directory
    find . -name "*.gcno" -exec gcov {} \; > /dev/null 2>&1
    
    # Move all generated .gcov files to the gcov directory
    find . -name "*.gcov" -type f -exec mv {} "$GCOV_OUTPUT_DIR/" \; 2>/dev/null || true
    
    # Also clean up any .gcov files that might be in the project root from blackbox tests
    find "$original_dir/.." -maxdepth 1 -name "*.gcov" -type f -exec mv {} "$GCOV_OUTPUT_DIR/" \; 2>/dev/null || true
    
    # Parse gcov output to calculate overall coverage percentage
    local total_lines=0
    local covered_lines=0
    
    # Process gcov files to extract coverage data, excluding external libraries and ignored files
    # This gives us coverage of the actual project code, not external dependencies
    local instrumented_files=0
    local covered_files=0
    while IFS= read -r gcov_file; do
        if [[ -f "$gcov_file" ]]; then
            local filename
            filename=$(basename "$gcov_file")
            
            # Skip external libraries and test framework files
            if [[ "$filename" == "unity.c.gcov" ]] || \
               [[ "$filename" == "jansson.h.gcov" ]] || \
               [[ "$filename" == "test_"*".gcov" ]] || \
               [[ "$gcov_file" == *"/usr/include/"* ]]; then
                continue
            fi
            
            # Process project source files only
            if [[ "$filename" == *.gcov ]]; then
                # Extract source file name and check if it should be ignored
                local source_name="${filename%.gcov}"
                local project_root="$original_dir/.."
                
                # Check if this file should be ignored based on .trial-ignore
                local test_path="$project_root/src/$source_name"
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
    done < <(find "$GCOV_OUTPUT_DIR" -name "*.gcov" -type f)
    
    # Calculate coverage percentage with 3 decimal places
    if [[ $total_lines -gt 0 ]]; then
        coverage_percentage=$(awk "BEGIN {printf \"%.3f\", ($covered_lines / $total_lines) * 100}")
    fi
    
    # Store coverage result with timestamp
    echo "$coverage_percentage" > "$UNITY_COVERAGE_FILE"
    echo "$timestamp,$coverage_percentage,$covered_lines,$total_lines,$instrumented_files,$covered_files" > "${UNITY_COVERAGE_FILE}.detailed"
    
    cd "$original_dir" || return 1
    echo "$coverage_percentage"
    return 0
}

# Function to collect blackbox coverage data after test execution
# Usage: collect_blackbox_coverage <hydrogen_coverage_binary> <timestamp>
# Returns: Coverage percentage written to blackbox_coverage.txt
collect_blackbox_coverage() {
    local hydrogen_coverage_binary="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    # Always ensure the blackbox coverage file exists, even if binary is not found
    if [ ! -f "$hydrogen_coverage_binary" ]; then
        echo "0.000" > "$BLACKBOX_COVERAGE_FILE"
        echo "$timestamp,0.000,0,0,0,0" > "${BLACKBOX_COVERAGE_FILE}.detailed"
        return 1
    fi
    
    # Look for gcda files in multiple possible locations and move them to results directory
    # 1. The hydrogen project root (where tests run from after navigate_to_project_root)
    # 2. The build directories
    # 3. Already in the results directory
    local hydrogen_project_root
    hydrogen_project_root=$(dirname "$hydrogen_coverage_binary")
    
    # Create a subdirectory in results for gcda files to keep them organized
    local gcda_storage_dir="$COVERAGE_RESULTS_DIR/gcda"
    mkdir -p "$gcda_storage_dir"
    
    # Search for gcda files in various locations and move them to results directory
    local found_gcda_files=0
    
    # Search in hydrogen project root first (most likely location)
    while IFS= read -r gcda_file; do
        if [ -f "$gcda_file" ]; then
            mv "$gcda_file" "$gcda_storage_dir/" 2>/dev/null && ((found_gcda_files++))
        fi
    done < <(find "$hydrogen_project_root" -maxdepth 1 -name "*.gcda" 2>/dev/null)
    
    # Search in build directories
    while IFS= read -r gcda_file; do
        if [ -f "$gcda_file" ]; then
            mv "$gcda_file" "$gcda_storage_dir/" 2>/dev/null && ((found_gcda_files++))
        fi
    done < <(find "$hydrogen_project_root/build" "$hydrogen_project_root/build_unity" -name "*.gcda" 2>/dev/null)
    
    # Also copy corresponding .gcno files to the same location
    # Find .gcno files in build directories and copy them with simplified names
    while IFS= read -r gcno_file; do
        if [ -f "$gcno_file" ]; then
            # Extract the base filename from the complex path
            local base_name
            base_name=$(basename "$gcno_file")
            # Copy to gcda storage directory with same name
            cp "$gcno_file" "$gcda_storage_dir/$base_name" 2>/dev/null || true
        fi
    done < <(find "$hydrogen_project_root/build" -name "*.gcno" 2>/dev/null)
    
    # Now get the list of gcda files from our centralized location
    local gcda_files=()
    mapfile -t gcda_files < <(find "$gcda_storage_dir" -name "*.gcda" 2>/dev/null)
    
    if [ ${#gcda_files[@]} -eq 0 ]; then
        echo "0.000" > "$BLACKBOX_COVERAGE_FILE"
        echo "$timestamp,0.000,0,0,0,0" > "${BLACKBOX_COVERAGE_FILE}.detailed"
        return 1
    fi
    
    # Process gcda files to generate gcov data, excluding external libraries
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    for gcda_file in "${gcda_files[@]}"; do
        local gcda_dir
        local gcda_name
        gcda_dir=$(dirname "$gcda_file")
        gcda_name=$(basename "$gcda_file" .gcda)
        
        # Skip external libraries and test framework files
        if [[ "$gcda_name" == "unity" ]] || \
           [[ "$gcda_name" == "jansson" ]] || \
           [[ "$gcda_name" == "test_"* ]] || \
           [[ "$gcda_file" == *"/usr/include/"* ]]; then
            continue
        fi
        
        # Check if this file should be ignored based on .trial-ignore
        local test_path="$hydrogen_project_root/src/$gcda_name.c"
        if should_ignore_file "$test_path" "$hydrogen_project_root"; then
            continue
        fi
        
        # Find corresponding gcno file
        local gcno_file="$gcda_dir/$gcda_name.gcno"
        if [ -f "$gcno_file" ]; then
            # Generate gcov data
            gcov -o "$gcda_dir" "$gcno_file" > /dev/null 2>&1
            
            # Process generated gcov file
            local gcov_file="$gcda_name.gcov"
            if [ -f "$gcov_file" ]; then
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
                
                # Move gcov file to gcov directory for organization
                mv "$gcov_file" "$GCOV_OUTPUT_DIR/" 2>/dev/null || true
            fi
        fi
    done
    
    # Clean up any remaining .gcov files from the project root and move them to gcov directory
    find . -maxdepth 1 -name "*.gcov" -type f -exec mv {} "$GCOV_OUTPUT_DIR/" \; 2>/dev/null || true
    
    # Also check the parent directory (project root) for stray .gcov files
    if [ -d "../" ]; then
        find "../" -maxdepth 1 -name "*.gcov" -type f -exec mv {} "$GCOV_OUTPUT_DIR/" \; 2>/dev/null || true
    fi
    
    # Calculate coverage percentage with 3 decimal places
    if [[ $total_lines -gt 0 ]]; then
        coverage_percentage=$(awk "BEGIN {printf \"%.3f\", ($covered_lines / $total_lines) * 100}")
    fi
    
    # Store coverage result with timestamp
    echo "$coverage_percentage" > "$BLACKBOX_COVERAGE_FILE"
    echo "$timestamp,$coverage_percentage,$covered_lines,$total_lines,$instrumented_files,$covered_files" > "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    # Clean up any temporary files (no temp_dir used in this function)
    
    echo "$coverage_percentage"
    return 0
}

# Function to calculate combined coverage from Unity and blackbox tests
# Usage: calculate_combined_coverage
# Returns: Combined coverage percentage written to combined_coverage.txt
calculate_combined_coverage() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local combined_coverage="0.000"
    
    # Read Unity coverage
    if [ -f "$UNITY_COVERAGE_FILE" ]; then
        unity_coverage=$(cat "$UNITY_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    # Read blackbox coverage
    if [ -f "$BLACKBOX_COVERAGE_FILE" ]; then
        blackbox_coverage=$(cat "$BLACKBOX_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    # Read detailed data if available
    local unity_lines_covered=0
    local unity_lines_total=0
    local blackbox_lines_covered=0
    local blackbox_lines_total=0
    
    if [ -f "${UNITY_COVERAGE_FILE}.detailed" ]; then
        local unity_data
        unity_data=$(cat "${UNITY_COVERAGE_FILE}.detailed" 2>/dev/null)
        if [[ "$unity_data" =~ ^[^,]*,[^,]*,([0-9]+),([0-9]+) ]]; then
            unity_lines_covered="${BASH_REMATCH[1]}"
            unity_lines_total="${BASH_REMATCH[2]}"
        fi
    fi
    
    if [ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]; then
        local blackbox_data
        blackbox_data=$(cat "${BLACKBOX_COVERAGE_FILE}.detailed" 2>/dev/null)
        if [[ "$blackbox_data" =~ ^[^,]*,[^,]*,([0-9]+),([0-9]+) ]]; then
            blackbox_lines_covered="${BASH_REMATCH[1]}"
            blackbox_lines_total="${BASH_REMATCH[2]}"
        fi
    fi
    
    # Calculate combined coverage (union of both test types)
    # This is a simplified calculation - in reality, we'd need to analyze
    # which specific lines are covered by each test type
    local total_unique_lines=$((unity_lines_total > blackbox_lines_total ? unity_lines_total : blackbox_lines_total))
    local total_covered_lines=$((unity_lines_covered > blackbox_lines_covered ? unity_lines_covered : blackbox_lines_covered))
    
    if [[ $total_unique_lines -gt 0 ]]; then
        combined_coverage=$(awk "BEGIN {printf \"%.3f\", ($total_covered_lines / $total_unique_lines) * 100}")
    fi
    
    # Store combined coverage result
    echo "$combined_coverage" > "$COMBINED_COVERAGE_FILE"
    
    echo "$combined_coverage"
    return 0
}

# Function to calculate coverage overlap between Unity and blackbox tests
# Usage: calculate_coverage_overlap
# Returns: Overlap percentage written to overlap_coverage.txt
calculate_coverage_overlap() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local overlap_percentage="0.000"
    
    # Read coverage data
    if [ -f "$UNITY_COVERAGE_FILE" ]; then
        unity_coverage=$(cat "$UNITY_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    if [ -f "$BLACKBOX_COVERAGE_FILE" ]; then
        blackbox_coverage=$(cat "$BLACKBOX_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    fi
    
    # Calculate overlap as the minimum of the two coverages
    # This is a simplified calculation - actual overlap would require
    # line-by-line analysis of gcov data
    if (( $(echo "$unity_coverage > 0" | bc -l) )) && (( $(echo "$blackbox_coverage > 0" | bc -l) )); then
        overlap_percentage=$(awk "BEGIN {
            unity = $unity_coverage;
            blackbox = $blackbox_coverage;
            overlap = (unity < blackbox) ? unity : blackbox;
            if (unity > 0) {
                printf \"%.3f\", (overlap / unity) * 100;
            } else {
                printf \"0.000\";
            }
        }")
    fi
    
    # Store overlap result
    echo "$overlap_percentage" > "$OVERLAP_COVERAGE_FILE"
    
    echo "$overlap_percentage"
    return 0
}

# Function to get the latest coverage data for display
# Usage: get_coverage_summary
# Returns: Formatted coverage summary for display
get_coverage_summary() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local combined_coverage="0.000"
    local overlap_percentage="0.000"
    
    # Read all coverage files
    [ -f "$UNITY_COVERAGE_FILE" ] && unity_coverage=$(cat "$UNITY_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    [ -f "$BLACKBOX_COVERAGE_FILE" ] && blackbox_coverage=$(cat "$BLACKBOX_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    [ -f "$COMBINED_COVERAGE_FILE" ] && combined_coverage=$(cat "$COMBINED_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    [ -f "$OVERLAP_COVERAGE_FILE" ] && overlap_percentage=$(cat "$OVERLAP_COVERAGE_FILE" 2>/dev/null || echo "0.000")
    
    # Format output
    echo "Unity Test Coverage: ${unity_coverage}%"
    echo "Blackbox Test Coverage: ${blackbox_coverage}%"
    echo "Combined Coverage: ${combined_coverage}%"
    echo "Coverage Overlap: ${overlap_percentage}%"
}

# Function to identify uncovered source files
# Usage: identify_uncovered_files <project_root>
# Returns: List of source files not covered by blackbox tests
identify_uncovered_files() {
    local project_root="$1"
    local uncovered_files=()
    local covered_files=()
    
    # Read ignore patterns from .trial-ignore
    local patterns=()
    if [ -f "/.trial-ignore" ]; then
        while IFS= read -r line; do
            # Skip comments and empty lines
            if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "${line// }" ]]; then
                continue
            fi
            # Remove leading ./ and add to patterns
            pattern="${line#./}"
            if [[ -n "$pattern" ]]; then
                patterns+=("$pattern")
            fi
        done < "/.trial-ignore"
    fi
    
    # Get list of all C source files in the project
    local all_source_files=()
    mapfile -t all_source_files < <(find "$project_root/src" -name "*.c" -type f)
    
    # Filter source files based on patterns
    local filtered_source_files=()
    for source_file in "${all_source_files[@]}"; do
        local relative_path="${source_file#$project_root/}"
        local should_ignore=false
        
        # Check if this file matches any ignore pattern
        for pattern in "${patterns[@]}"; do
            if [[ "$relative_path" == *"$pattern"* ]]; then
                should_ignore=true
                break
            fi
        done
        
        if [[ "$should_ignore" == false ]]; then
            filtered_source_files+=("$source_file")
        fi
    done
    
    # Use filtered list instead of all files
    all_source_files=("${filtered_source_files[@]}")
    
    # Get list of files that have coverage data (from gcov files that have actual coverage)
    local gcov_files=()
    mapfile -t gcov_files < <(find "$GCOV_OUTPUT_DIR" -name "*.gcov" -type f 2>/dev/null)
    
    # Extract covered file names from gcov files that actually have coverage
    for gcov_file in "${gcov_files[@]}"; do
        local gcov_filename
        gcov_filename=$(basename "$gcov_file")
        
        # Skip external libraries and test files
        if [[ "$gcov_filename" == "unity.c.gcov" ]] || \
           [[ "$gcov_filename" == "jansson"*".gcov" ]] || \
           [[ "$gcov_filename" == "test_"*".gcov" ]]; then
            continue
        fi
        
        # Check if this gcov file has any covered lines
        local covered_lines
        covered_lines=$(grep -c "^[[:space:]]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null || echo "0")
        
        # Ensure covered_lines is a clean integer (strip any whitespace/newlines)
        covered_lines=$(echo "$covered_lines" | tr -d '\n\r\t ' | grep -o '[0-9]*' | head -1)
        covered_lines=${covered_lines:-0}

        if [[ $covered_lines -gt 0 ]]; then
            # Extract the source file name from gcov filename
            # gcov files are typically named like "source.c.gcov"
            local source_name
            source_name=$(echo "$gcov_filename" | sed 's/\.gcov$//')
            
            # Find matching source file
            for source_file in "${all_source_files[@]}"; do
                local source_basename
                source_basename=$(basename "$source_file")
                if [[ "$source_basename" == "$source_name" ]]; then
                    covered_files+=("$source_file")
                    break
                fi
            done
        fi
    done
    
    # Find uncovered files
    for source_file in "${all_source_files[@]}"; do
        local is_covered=false
        for covered_file in "${covered_files[@]}"; do
            if [[ "$source_file" == "$covered_file" ]]; then
                is_covered=true
                break
            fi
        done
        
        if [[ "$is_covered" == false ]]; then
            uncovered_files+=("$source_file")
        fi
    done
    
    # Output results
    echo "COVERED_FILES_COUNT:${#covered_files[@]}"
    echo "UNCOVERED_FILES_COUNT:${#uncovered_files[@]}"
    echo "TOTAL_SOURCE_FILES:${#all_source_files[@]}"
    
    if [[ ${#uncovered_files[@]} -gt 0 ]]; then
        echo "UNCOVERED_FILES:"
        printf '%s\n' "${uncovered_files[@]}"
    fi
    
    return 0
}

# Function to clean up coverage data files
# Usage: cleanup_coverage_data
cleanup_coverage_data() {
    rm -f "$UNITY_COVERAGE_FILE" "$BLACKBOX_COVERAGE_FILE" "$COMBINED_COVERAGE_FILE" "$OVERLAP_COVERAGE_FILE"
    rm -f "${UNITY_COVERAGE_FILE}.detailed" "${BLACKBOX_COVERAGE_FILE}.detailed"
    rm -rf "$GCOV_PREFIX"
    rm -rf "$GCOV_OUTPUT_DIR"
    return 0
}
