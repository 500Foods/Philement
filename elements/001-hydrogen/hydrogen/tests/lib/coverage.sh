#!/bin/bash

# Coverage Calculation Functions
# This script provides functions for calculating and managing test coverage data
# for both Unity tests and blackbox tests in the Hydrogen project.

# CHANGELOG
# 2.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.0.0 - 2025-07-11 - Refactored into modular components
# 1.0.0 - 2025-07-11 - Initial version with Unity and blackbox coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "$COVERAGE_GUARD" ]] && return 0
export COVERAGE_GUARD="true"

# Library metadata
COVERAGE_NAME="Hydrogen Coverage Library"
COVERAGE_VERSION="2.1.0"

# Get script directory - don't overwrite if already set by calling script
COVERAGE_MAIN_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source modular coverage components
source "$COVERAGE_MAIN_SCRIPT_DIR/coverage-common.sh"
source "$COVERAGE_MAIN_SCRIPT_DIR/coverage-unity.sh"
source "$COVERAGE_MAIN_SCRIPT_DIR/coverage-blackbox.sh"

# Function to validate coverage consistency and provide detailed breakdown
validate_coverage_consistency() {
    local project_root="$1"
    local timestamp="$2"
    
    # Get detailed coverage data
    local total_files=0
    local covered_files=0
    local total_lines=0
    local covered_lines=0
    
    # Find potential build directories to check for gcov files
    local build_dirs=()
    build_dirs+=("$project_root/build/unity")
    build_dirs+=("$project_root/build")
    
    # Analyze each gcov file for detailed statistics from build directories
    for build_dir in "${build_dirs[@]}"; do
        if [[ -d "$build_dir" ]]; then
            while IFS= read -r gcov_file; do
                if [[ -f "$gcov_file" ]]; then
                    local filename
                    filename=$(basename "$gcov_file")
                    
                    # Skip external files
                    if [[ "$filename" == "unity.c.gcov" ]] || [[ "$filename" == *"/usr/include/"* ]]; then
                        continue
                    fi
                    
                    local source_name
                    local test_path
                    source_name="${filename%.gcov}"
                    test_path="$project_root/src/$source_name"
                    
                    # Check if should be ignored
                    if should_ignore_file "$test_path" "$project_root"; then
                        continue
                    fi
                    
                    # Detailed line analysis
                    local file_total
                    local file_covered
                    file_total=$(grep -c "^[ \t]*[0-9#-].*:" "$gcov_file" 2>/dev/null || echo "0")
                    file_covered=$(grep -c "^[ \t]*[1-9][0-9]*.*:" "$gcov_file" 2>/dev/null || echo "0")
                    
                    if [[ $file_covered -gt 0 ]]; then
                        covered_files=$((covered_files + 1))
                    fi
                    
                    total_files=$((total_files + 1))
                    total_lines=$((total_lines + file_total))
                    covered_lines=$((covered_lines + file_covered))
                fi
            done < <(find "$build_dir" -name "*.gcov" -type f 2>/dev/null)
        fi
    done
    
    # Calculate overall statistics
    local file_coverage_pct=0
    local line_coverage_pct=0
    if [[ $total_files -gt 0 ]]; then
        file_coverage_pct=$(awk "BEGIN {printf \"%.1f\", ($covered_files / $total_files) * 100}")
    fi
    if [[ $total_lines -gt 0 ]]; then
        line_coverage_pct=$(awk "BEGIN {printf \"%.1f\", ($covered_lines / $total_lines) * 100}")
    fi
    
    # Generate detailed report
    echo "COVERAGE_VALIDATION_REPORT_START"
    echo "Timestamp: $timestamp"
    echo "Total Files Analyzed: $total_files"
    echo "Files With Coverage: $covered_files"
    echo "File Coverage: $file_coverage_pct%"
    echo "Total Lines Analyzed: $total_lines"
    echo "Lines With Coverage: $covered_lines"
    echo "Line Coverage: $line_coverage_pct%"
    echo "COVERAGE_VALIDATION_REPORT_END"
    
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
