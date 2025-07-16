#!/bin/bash
#
# coverage-common.sh - Common utilities and variables for coverage calculation
#
# This script provides shared variables, functions, and utilities used by
# all coverage calculation modules.
#

# Coverage data storage locations
# Use the calling script's SCRIPT_DIR if available, otherwise determine our own
if [[ -z "$SCRIPT_DIR" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi
COVERAGE_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Always use build/tests/results directory
COVERAGE_BUILD_DIR="$(dirname "$(dirname "$COVERAGE_SCRIPT_DIR")")/build"
COVERAGE_RESULTS_DIR="$COVERAGE_BUILD_DIR/tests/results"

UNITY_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/unity_coverage.txt"
BLACKBOX_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/blackbox_coverage.txt"
COMBINED_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/combined_coverage.txt"
OVERLAP_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/overlap_coverage.txt"

# Function to analyze combined coverage from two gcov files for the same source file
# Usage: analyze_combined_gcov_coverage <unity_gcov_file> <blackbox_gcov_file>
# Returns: "instrumented_lines,combined_covered_lines"
analyze_combined_gcov_coverage() {
    local unity_gcov="$1"
    local blackbox_gcov="$2"
    
    # Handle cases where only one file exists
    if [[ ! -f "$unity_gcov" && ! -f "$blackbox_gcov" ]]; then
        echo "0,0,0"
        return 1
    elif [[ ! -f "$unity_gcov" ]]; then
        # Only blackbox coverage exists
        local result
        result=$(awk '
            /^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:/ { covered++; total++ }
            /^[[:space:]]*#####:[[:space:]]*[0-9]+:/ { total++ }
            END { 
                if (total == "") total = 0;
                if (covered == "") covered = 0;
                print total "," covered "," covered
            }' "$blackbox_gcov")
        echo "$result"
        return 0
    elif [[ ! -f "$blackbox_gcov" ]]; then
        # Only unity coverage exists
        local result
        result=$(awk '
            /^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:/ { covered++; total++ }
            /^[[:space:]]*#####:[[:space:]]*[0-9]+:/ { total++ }
            END { 
                if (total == "") total = 0;
                if (covered == "") covered = 0;
                print total "," covered "," covered
            }' "$unity_gcov")
        echo "$result"
        return 0
    fi
    
    # Both files exist - calculate TRUE union using the same logic as manual test
    local temp_dir
    temp_dir=$(mktemp -d 2>/dev/null) || return 1
    local unity_covered="$temp_dir/unity_covered"
    local blackbox_covered="$temp_dir/blackbox_covered"
    local all_instrumented="$temp_dir/all_instrumented"
    local union_covered="$temp_dir/union_covered"
    
    # Extract covered lines from Unity gcov - ensure we strip whitespace properly
    grep -E "^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:" "$unity_gcov" 2>/dev/null | \
        cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//' | sort -n | uniq > "$unity_covered"
    
    # Extract covered lines from Blackbox gcov - ensure we strip whitespace properly  
    grep -E "^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:" "$blackbox_gcov" 2>/dev/null | \
        cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//' | sort -n | uniq > "$blackbox_covered"
    
    # Extract all instrumented lines (covered + uncovered) from both files
    {
        grep -E "^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:" "$unity_gcov" 2>/dev/null | cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//'
        grep -E "^[[:space:]]*#####:[[:space:]]*[0-9]+:" "$unity_gcov" 2>/dev/null | cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//'
        grep -E "^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:" "$blackbox_gcov" 2>/dev/null | cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//'
        grep -E "^[[:space:]]*#####:[[:space:]]*[0-9]+:" "$blackbox_gcov" 2>/dev/null | cut -d: -f2 | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//'
    } | sort -n | uniq > "$all_instrumented"
    
    # Calculate union of covered lines (same as manual: cat files | sort -n | uniq)
    cat "$unity_covered" "$blackbox_covered" | sort -n | uniq > "$union_covered"
    
    # Count the results
    local total_instrumented
    total_instrumented=$(wc -l < "$all_instrumented" 2>/dev/null || echo "0")
    local total_covered
    total_covered=$(wc -l < "$union_covered" 2>/dev/null || echo "0")
    
    # Clean up temp files
    rm -rf "$temp_dir" 2>/dev/null
    
    echo "$total_instrumented,$total_covered,$total_covered"
}

# Ensure coverage directories exist
mkdir -p "$COVERAGE_RESULTS_DIR"

# Global ignore patterns and source file cache loaded once
IGNORE_PATTERNS_LOADED=""
IGNORE_PATTERNS=()
SOURCE_FILE_CACHE_LOADED=""
SOURCE_FILES_CACHE=()

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
        # Use more efficient reading with mapfile
        local lines=()
        mapfile -t lines < "$project_root/.trial-ignore"
        
        for line in "${lines[@]}"; do
            # Skip comments and empty lines more efficiently
            [[ "$line" =~ ^[[:space:]]*# ]] && continue
            [[ -z "${line// }" ]] && continue
            
            # Remove leading ./ and add to patterns
            local pattern="${line#./}"
            [[ -n "$pattern" ]] && IGNORE_PATTERNS+=("$pattern")
        done
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
    
    # Check against patterns
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        if [[ "$file_path" == *"$pattern"* ]]; then
            return 0  # Ignore
        fi
    done
    
    return 1  # Do not ignore
}

# Function to load all source files from project directory
# Usage: load_source_files <project_root>
load_source_files() {
    local project_root="$1"
    
    # Only load once
    if [[ "$SOURCE_FILE_CACHE_LOADED" == "true" ]]; then
        return 0
    fi
    
    SOURCE_FILES_CACHE=()
    local src_dir="$project_root/src"
    
    if [ -d "$src_dir" ]; then
        while IFS= read -r -d '' file; do
            # Get relative path from project root
            local rel_path="${file#"$project_root"/}"
            
            # Skip ignored files
            if should_ignore_file "$rel_path" "$project_root"; then
                continue
            fi
            
            # Skip test files
            local basename_file
            basename_file=$(basename "$file")
            if [[ "$basename_file" == "test_"* ]]; then
                continue
            fi
            
            SOURCE_FILES_CACHE+=("$rel_path")
        done < <(find "$src_dir" -type f \( -name "*.c" -o -name "*.h" \) -print0 2>/dev/null)
    fi
    
    SOURCE_FILE_CACHE_LOADED="true"
    return 0
}

# Function to identify uncovered source files
# Usage: identify_uncovered_files <project_root>
identify_uncovered_files() {
    local project_root="$1"
    local uncovered_files=()
    
    # Load source files
    load_source_files "$project_root"
    
    # Create temporary file for uncovered files
    local temp_uncovered
    temp_uncovered=$(mktemp)
    
    # Process each source file
    for file in "${SOURCE_FILES_CACHE[@]}"; do
        # Check if file has a corresponding .gcov in blackbox coverage directory
        local basename_file
        basename_file=$(basename "$file" .c)
        local gcov_file="$BLACKBOX_COVS/${basename_file}.c.gcov"
        
        # If gcov doesn't exist or has zero coverage, consider it uncovered
        if [[ ! -f "$gcov_file" ]] || [[ $(awk '/^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:/ { covered++ } END { print (covered == 0 ? 0 : 1) }' "$gcov_file") -eq 0 ]]; then
            uncovered_files+=("$file")
            echo "$file" >> "$temp_uncovered"
        fi
    done
    
    # Calculate uncovered count
    local uncovered_count=${#uncovered_files[@]}
    
    # Output results
    echo "UNCOVERED_FILES_COUNT: $uncovered_count"
    echo "UNCOVERED_FILES:"
    cat "$temp_uncovered"
    
    rm -f "$temp_uncovered"
}

# Function to clean up coverage data files
# Usage: cleanup_coverage_data
cleanup_coverage_data() {
    rm -f "$UNITY_COVERAGE_FILE" "$BLACKBOX_COVERAGE_FILE" "$COMBINED_COVERAGE_FILE" "$OVERLAP_COVERAGE_FILE"
    rm -f "${UNITY_COVERAGE_FILE}.detailed" "${BLACKBOX_COVERAGE_FILE}.detailed"
    rm -rf "$GCOV_PREFIX" 2>/dev/null || true
    # Note: We don't remove .gcov files since they stay in their respective build directories
    # Only clean up the centralized results
    return 0
}

# Function to analyze a single gcov file and store data
analyze_gcov_file() {
    local gcov_file="$1"
    local coverage_type="$2"  # "unity" or "coverage"
    
    # Input validation - prevent hanging on empty or nonexistent files
    if [[ -z "$gcov_file" ]]; then
        return 1
    fi
    
    if [[ ! -f "$gcov_file" ]]; then
        return 1
    fi
    
    # Use the correct awk parsing logic for gcov format
    local line_counts
    line_counts=$(awk '
        /^[[:space:]]*[0-9]+\*?:[[:space:]]*[0-9]+:/ { covered++; total++ }
        /^[[:space:]]*#####:[[:space:]]*[0-9]+:/ { total++ }
        END {
            if (total == "") total = 0
            if (covered == "") covered = 0
            print total "," covered
        }
    ' "$gcov_file" 2>/dev/null)
    
    local instrumented_lines="${line_counts%,*}"
    local covered_lines="${line_counts#*,}"
    
    # Default to 0 if parsing failed
    instrumented_lines=${instrumented_lines:-0}
    covered_lines=${covered_lines:-0}
    
    # Include all files, even those with 0 instrumented lines, for complete coverage table
    # This ensures the table shows all files that have gcov data, matching test 99's behavior
    
    # Extract relative path from Source: line in gcov file
    local source_line
    source_line=$(grep '^        -:    0:Source:' "$gcov_file" | cut -d':' -f3-)
    local display_path
    if [[ -n "$source_line" ]]; then
        display_path="${source_line#*/hydrogen/}"
    else
        # Fallback to basename
        display_path=$(basename "$gcov_file" .gcov)
        display_path="src/$display_path"
    fi
    
    # Store data in appropriate arrays
    all_files["$display_path"]=1
    if [[ "$coverage_type" == "unity" ]]; then
        unity_covered_lines["$display_path"]=$covered_lines
        unity_instrumented_lines["$display_path"]=$instrumented_lines
    else
        coverage_covered_lines["$display_path"]=$covered_lines
        coverage_instrumented_lines["$display_path"]=$instrumented_lines
    fi
}

# Function to collect and process gcov files from a directory
collect_gcov_files() {
    local build_dir="$1"
    local coverage_type="$2"
    local files_found=0
    
    if [ -d "$build_dir" ]; then
        while IFS= read -r gcov_file; do
            if [[ -f "$gcov_file" ]]; then
                # Use the exact same filtering logic as the working sections
                basename_file=$(basename "$gcov_file")
                if [[ "$basename_file" == "unity.c.gcov" ]] || [[ "$gcov_file" == *"/usr/"* ]]; then
                    continue
                fi
                
                # Skip system include files that show up in Source: lines
                if grep -q "Source:/usr/include/" "$gcov_file" 2>/dev/null; then
                    continue
                fi
                
                if [[ "$basename_file" == "test_"* ]]; then
                    continue
                fi
                
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
                
                source_file="${basename_file%.gcov}"
                should_ignore=false
                
                if [[ -f "$project_root/.trial-ignore" ]]; then
                    while IFS= read -r line; do
                        if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "$line" ]]; then
                            continue
                        fi
                        pattern="${line#./}"
                        if [[ "$source_file" == *"$pattern"* ]]; then
                            should_ignore=true
                            break
                        fi
                    done < "$project_root/.trial-ignore"
                fi
                
                if [[ "$should_ignore" == true ]]; then
                    continue
                fi
                
                analyze_gcov_file "$gcov_file" "$coverage_type"
                ((files_found++))
            fi
        done < <(find "$build_dir" -name "*.gcov" -type f 2>/dev/null)
    fi
    
    return "$files_found"
}