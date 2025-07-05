#!/bin/bash
#
# Hydrogen Test Suite - CLOC (Count Lines of Code) Library
# Provides shared cloc functionality for test scripts
#
CLOC_LIB_NAME="Hydrogen CLOC Library"
CLOC_LIB_VERSION="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-07-02 - Initial creation, extracted from test_99_codebase.sh and test_00_all.sh

# Function to display library version information
print_cloc_lib_version() {
    echo "=== $CLOC_LIB_NAME v$CLOC_LIB_VERSION ==="
}

# Default exclude patterns for linting (can be overridden by .lintignore)
if [ -z "${DEFAULT_LINT_EXCLUDES+x}" ]; then
    readonly DEFAULT_LINT_EXCLUDES=(
        "build/*"
        "build_debug/*"
        "build_perf/*"
        "build_release/*"
        "build_valgrind/*"
        "tests/logs/*"
        "tests/results/*"
        "tests/diagnostics/*"
    )
fi

# Function to check if a file should be excluded from linting/cloc analysis
should_exclude_file() {
    local file="$1"
    local lint_ignore_file="${2:-.lintignore}"  # Default to .lintignore if not specified
    local rel_file="${file#./}"  # Remove leading ./
    
    # Check .lintignore file first if it exists
    if [ -f "$lint_ignore_file" ]; then
        while IFS= read -r pattern; do
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            
            # Check if file matches pattern exactly or is within a directory pattern
            if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
                return 0 # Exclude
            fi
        done < "$lint_ignore_file"
    fi
    
    # Check default excludes
    for pattern in "${DEFAULT_LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        if [[ "$rel_file" == "$pattern" ]] || [[ "$rel_file" == "$clean_pattern"/* ]]; then
            return 0 # Exclude
        fi
    done
    
    return 1 # Do not exclude
}

# Function to generate cloc exclude list based on .lintignore and default excludes
generate_cloc_exclude_list() {
    local base_dir="${1:-.}"           # Base directory to scan (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    local exclude_list_file="$3"       # Output file for exclude list (required)
    
    if [ -z "$exclude_list_file" ]; then
        echo "Error: exclude_list_file parameter is required" >&2
        return 1
    fi
    
    # Generate exclude list based on .lintignore and default excludes
    : > "$exclude_list_file"
    # Read patterns from .lintignore file and convert to cloc-compatible format
    if [ -f "$lint_ignore_file" ]; then
        while IFS= read -r pattern; do
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            echo "$clean_pattern" >> "$exclude_list_file"
        done < "$lint_ignore_file"
    fi
    
    # Add default excludes
    for pattern in "${DEFAULT_LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        echo "$clean_pattern" >> "$exclude_list_file"
    done
    
    return 0
}

# Function to run cloc analysis with proper exclusions
run_cloc_analysis() {
    local base_dir="${1:-.}"           # Base directory to analyze (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    local output_file="$3"             # Optional output file (if not provided, outputs to stdout)
    
    # Check if cloc is available
    if ! command -v cloc >/dev/null 2>&1; then
        echo "cloc not available"
        return 1
    fi
    
    # Create temporary files
    local cloc_output
    local exclude_list
    local enhanced_output
    cloc_output=$(mktemp) || return 1
    exclude_list=$(mktemp) || return 1
    enhanced_output=$(mktemp) || return 1
    
    # Ensure cleanup on exit
    trap 'rm -f "$cloc_output" "$exclude_list" "$enhanced_output"' EXIT
    
    # Generate exclude list
    if ! generate_cloc_exclude_list "$base_dir" "$lint_ignore_file" "$exclude_list"; then
        echo "Failed to generate exclude list" >&2
        return 1
    fi
    
    # Run cloc with proper environment and parameters
    if (cd "$base_dir" && env LC_ALL=en_US.UTF_8 cloc . --quiet --force-lang="C,inc" --exclude-list-file="$exclude_list" > "$cloc_output" 2>&1); then
        # Skip the first line (header) and process the results
        tail -n +2 "$cloc_output" > "$enhanced_output"
        
        # Calculate CodeDoc and CodeComment ratios
        local c_code=0 c_comment=0
        local header_code=0 header_comment=0
        local cmake_code=0 cmake_comment=0
        local shell_code=0 shell_comment=0
        local markdown_code=0
        
        # Parse cloc output to extract values
        while IFS= read -r line; do
            # Skip empty lines and separator lines
            [[ -z "$line" || "$line" =~ ^-+$ ]] && continue
            
            # Skip SUM line for now (we'll process it separately if needed)
            [[ "$line" =~ ^SUM: ]] && continue
            
            # Use awk to parse the line more reliably
            local lang files comment code
            lang=$(echo "$line" | awk '{$1=$1; for(i=1;i<=NF-4;i++) printf "%s ", $i; print ""}' | sed 's/ *$//')
            files=$(echo "$line" | awk '{print $(NF-3)}')
            comment=$(echo "$line" | awk '{print $(NF-1)}')
            code=$(echo "$line" | awk '{print $NF}')
            
            # Only process lines that have numeric values
            if [[ "$files" =~ ^[0-9]+$ && "$code" =~ ^[0-9]+$ && "$comment" =~ ^[0-9]+$ ]]; then
                case "$lang" in
                    "C") 
                        c_code=$code
                        c_comment=$comment
                        ;;
                    "C/C++ Header")
                        header_code=$code
                        header_comment=$comment
                        ;;
                    "CMake")
                        cmake_code=$code
                        cmake_comment=$comment
                        ;;
                    "Bourne Shell")
                        shell_code=$code
                        shell_comment=$comment
                        ;;
                    "Markdown")
                        markdown_code=$code
                        ;;
                esac
            fi
        done < "$enhanced_output"
        
        # Calculate totals for the 4 code languages
        local total_code=$((c_code + header_code + cmake_code + shell_code))
        local total_comment=$((c_comment + header_comment + cmake_comment + shell_comment))
        
        # Calculate ratios
        local codedoc_ratio=""
        local codecomment_ratio=""
        
        if [ "$markdown_code" -gt 0 ]; then
            codedoc_ratio=$(awk "BEGIN {printf \"%.1f\", $total_code / $markdown_code}")
        else
            codedoc_ratio="N/A"
        fi
        
        if [ "$total_comment" -gt 0 ]; then
            codecomment_ratio=$(awk "BEGIN {printf \"%.1f\", $total_code / $total_comment}")
        else
            codecomment_ratio="N/A"
        fi
        
        # Output the original cloc results
        if [ -n "$output_file" ]; then
            cat "$enhanced_output" > "$output_file"
            echo "" >> "$output_file"
            echo "CodeDoc: $codedoc_ratio    CodeComment: $codecomment_ratio" >> "$output_file"
        else
            cat "$enhanced_output"
            echo ""
            echo "CodeDoc: $codedoc_ratio    CodeComment: $codecomment_ratio"
        fi
        return 0
    else
        echo "cloc command failed"
        return 1
    fi
}

# Function to generate cloc output for README.md format
generate_cloc_for_readme() {
    local base_dir="${1:-.}"           # Base directory to analyze (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    
    # Check if cloc is available
    if ! command -v cloc >/dev/null 2>&1; then
        echo '```'
        echo "cloc not available"
        echo '```'
        return 1
    fi
    
    echo '```cloc'
    
    if run_cloc_analysis "$base_dir" "$lint_ignore_file"; then
        # Success - output was already printed by run_cloc_analysis
        :
    else
        echo "cloc command failed"
    fi
    
    echo '```'
    return 0
}

# Function to extract cloc statistics from output
extract_cloc_stats() {
    local cloc_output_file="$1"
    
    if [ ! -f "$cloc_output_file" ]; then
        echo "Error: cloc output file not found: $cloc_output_file" >&2
        return 1
    fi
    
    # Extract summary statistics from the SUM line
    local stats_line
    stats_line=$(grep "SUM:" "$cloc_output_file")
    
    if [ -n "$stats_line" ]; then
        local files_count code_lines
        files_count=$(echo "$stats_line" | awk '{print $2}')
        code_lines=$(echo "$stats_line" | awk '{print $5}')
        
        echo "files:$files_count,lines:$code_lines"
        return 0
    else
        echo "Error: Failed to parse cloc output" >&2
        return 1
    fi
}

# Function to run cloc analysis and return statistics
run_cloc_with_stats() {
    local base_dir="${1:-.}"           # Base directory to analyze (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    
    local temp_output
    temp_output=$(mktemp) || return 1
    
    # Ensure cleanup
    trap 'rm -f "$temp_output"' RETURN
    
    if run_cloc_analysis "$base_dir" "$lint_ignore_file" "$temp_output"; then
        # Extract and return statistics
        extract_cloc_stats "$temp_output"
        return 0
    else
        return 1
    fi
}
