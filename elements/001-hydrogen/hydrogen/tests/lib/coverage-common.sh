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
COVERAGE_RESULTS_DIR="$(dirname "$COVERAGE_SCRIPT_DIR")/results"
UNITY_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/unity_coverage.txt"
BLACKBOX_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/blackbox_coverage.txt"
COMBINED_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/combined_coverage.txt"
OVERLAP_COVERAGE_FILE="$COVERAGE_RESULTS_DIR/overlap_coverage.txt"

# Ensure coverage directories exist
mkdir -p "$COVERAGE_RESULTS_DIR"

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
    
    local relative_path="${file_path#"$project_root"/}"
    
    # Check if this file matches any ignore pattern
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        if [[ "$relative_path" == *"$pattern"* ]]; then
            return 0  # Should ignore
        fi
    done
    
    return 1  # Should not ignore
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
