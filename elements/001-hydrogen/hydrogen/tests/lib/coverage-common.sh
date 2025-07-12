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
    
    local relative_path="${file_path#"$project_root"/}"
    
    # Use case statement for faster pattern matching when possible
    # Note: These fast-path patterns should match common entries in .trial-ignore
    # shellcheck disable=SC2221,SC2222  # Overlapping patterns are intentional for performance
    case "$relative_path" in
        src/not_hydrogen.c|not_hydrogen.c)
            return 0  # Should ignore (matches .trial-ignore)
            ;;
        print/print_queue_manager.c)
            return 0  # Should ignore (matches .trial-ignore)
            ;;
        websocket/websocket_dynamic.c)
            return 0  # Should ignore (matches .trial-ignore)
            ;;
        api/system/system_service.c)
            return 0  # Should ignore (matches .trial-ignore)
            ;;
        config/*)
            return 0  # Should ignore config files (matches .trial-ignore patterns)
            ;;
        */config_*.c)
            return 0  # Should ignore config files with config_ prefix
            ;;
    esac
    
    # Check remaining patterns only if quick patterns didn't match
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        if [[ "$relative_path" == *"$pattern"* ]]; then
            return 0  # Should ignore
        fi
    done
    
    return 1  # Should not ignore
}

# Function to load source files cache to avoid repeated filesystem scans
# Usage: load_source_files_cache <project_root>
load_source_files_cache() {
    local project_root="$1"
    
    # Only load once
    if [[ "$SOURCE_FILE_CACHE_LOADED" == "true" ]]; then
        return 0
    fi
    
    SOURCE_FILES_CACHE=()
    if [[ -d "$project_root/src" ]]; then
        # Use more efficient approach to cache all source files
        mapfile -t SOURCE_FILES_CACHE < <(find "$project_root/src" -name "*.c" -type f 2>/dev/null)
    fi
    
    SOURCE_FILE_CACHE_LOADED="true"
    return 0
}

# Function to get cached source files
# Usage: get_cached_source_files <project_root>
get_cached_source_files() {
    local project_root="$1"
    
    # Load cache if not loaded
    load_source_files_cache "$project_root"
    
    # Output cached source files
    printf '%s\n' "${SOURCE_FILES_CACHE[@]}"
}

# Function to clean up coverage data files
# Usage: cleanup_coverage_data
cleanup_coverage_data() {
    rm -f "$UNITY_COVERAGE_FILE" "$BLACKBOX_COVERAGE_FILE" "$COMBINED_COVERAGE_FILE" "$OVERLAP_COVERAGE_FILE"
    rm -f "${UNITY_COVERAGE_FILE}.detailed" "${BLACKBOX_COVERAGE_FILE}.detailed"
    rm -rf "$GCOV_PREFIX" 2>/dev/null || true
    # Reset caches for next run
    IGNORE_PATTERNS_LOADED=""
    SOURCE_FILE_CACHE_LOADED=""
    # Note: We don't remove .gcov files since they stay in their respective build directories
    # Only clean up the centralized results
    return 0
}
