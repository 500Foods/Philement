#!/usr/bin/env bash

# Comment Analysis Script
# Analyzes comment density in Test C/Headers, Core C/Headers, and Bourne Shell files
# Outputs files sorted by descending C/C % for balance assessment
# Shows only files with >50% comment ratios

# CHANGELOG
# 2.0.0 - 2025-10-01 - Major overhaul: single cloc run with --by-file, filtering, section breaks
# 1.0.0 - 2025-10-01 - Initial creation

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${COMMENT_ANALYSIS_GUARD:-}" ]] && return 0
export COMMENT_ANALYSIS_GUARD="true"

# Library metadata
# This script provides comment density analysis for project files

# Show progress messages
show_progress() {
    echo "$1" >&2
}

# Showstopper: Need access to cloc command
if ! command -v cloc >/dev/null 2>&1; then
    echo "Error: 'cloc' command not found" >&2
    exit 1
fi

# Showstopper: Need access to tables command
if ! command -v tables >/dev/null 2>&1; then
    echo "Error: 'tables' command not found" >&2
    exit 1
fi

# Showstopper: Need access to jq command
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: 'jq' command not found" >&2
    exit 1
fi

# Function to find a command, preferring GNU version
find_command() {
    local gnu_cmd=$1 std_cmd=$2 var_name=$3
    if command -v "${gnu_cmd}" >/dev/null 2>&1; then
        local cmd_path
        cmd_path=$(command -v "${gnu_cmd}") || return 1
        eval "${var_name}=${cmd_path}"
    elif command -v "${std_cmd}" >/dev/null 2>&1; then
        local cmd_path
        cmd_path=$(command -v "${std_cmd}") || return 1
        eval "${var_name}=${cmd_path}"
    else
        echo "Error: Neither ${gnu_cmd} nor ${std_cmd} found" >&2
        exit 1
    fi
}

# Common utilities - use GNU versions if available
find_command printf    printf   PRINTF
find_command gdate     date     DATE
find_command gfind     find     FIND
find_command ggrep     grep     GREP
find_command gsed      sed      SED
find_command gawk      awk      AWK
find_command gbasename basename BASENAME
find_command gdirname  dirname  DIRNAME
find_command gxargs    xargs    XARGS
find_command grealpath realpath REALPATH

export PRINTF DATE FIND GREP SED AWK BASENAME DIRNAME XARGS REALPATH

# Our own utilities
CLOC=$(command -v cloc)
TABLES=$(command -v tables)
JQ=$(command -v jq)
export CLOC TABLES JQ

# Set timestamp for output
TIMESTAMP_DISPLAY=$("${DATE}" '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null) || TIMESTAMP_DISPLAY="Unknown"

# Function to generate cloc exclude list based on .lintignore
generate_cloc_exclude_list() {
    local lint_ignore_file="${1:-.lintignore}"
    local exclude_list_file="$2"
    
    if [[ -z "${exclude_list_file}" ]]; then
        echo "Error: exclude_list_file parameter is required" >&2
        return 1
    fi
    
    # Generate exclude list based on .lintignore
    true > "${exclude_list_file}"
    
    # Read patterns from .lintignore file
    if [[ -f "${lint_ignore_file}" ]]; then
        while IFS= read -r pattern; do
            [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            echo "${clean_pattern}" >> "${exclude_list_file}"
        done < "${lint_ignore_file}"
    fi
    
    return 0
}

# Function to run cloc analysis with --by-file flag for detailed per-file output
run_cloc_with_by_file() {
    local project_dir="$1"
    local output_json="$2"
    
    # Create exclude list
    local exclude_list
    exclude_list=$(mktemp) || return 1
    
    # shellcheck disable=SC2310 # Function invoked in condition, set -e disabled intentionally
    if ! generate_cloc_exclude_list "${project_dir}/.lintignore" "${exclude_list}"; then
        echo "Failed to generate exclude list" >&2
        return 1
    fi
    
    # Create temporary files for core and test runs
    local core_json test_json
    core_json=$(mktemp) || return 1
    test_json=$(mktemp) || return 1
    
    show_progress "Running cloc on core files..."
    
    # Run cloc on core files (excluding tests directory) with --by-file flag
    if (cd "${project_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" . \
        --quiet --json --by-file \
        --exclude-list-file="${exclude_list}" \
        --not-match-d="build" \
        --not-match-d='images' \
        --not-match-d='tests/lib/node_modules' \
        --not-match-d='tests/unity' \
        --not-match-f='hydrogen_installer' \
        --force-lang=C,c \
        --force-lang=C,h \
        --force-lang=C,inc \
        > "${core_json}" 2>&1); then
        
        show_progress "Running cloc on test files..."
        
        # Run cloc on test files
        if (cd "${project_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" tests/unity \
            --not-match-d='tests/unity/framework' \
            --quiet --json --by-file \
            --force-lang=C,c \
            --force-lang=C,h \
            --force-lang=C,inc \
            > "${test_json}" 2>&1); then
            
            # Count files before processing
            local file_count
            file_count=$("${JQ}" -r '[to_entries[] | select(.key != "header" and .key != "SUM")] | length' "${core_json}")
            local test_file_count
            test_file_count=$("${JQ}" -r '[to_entries[] | select(.key != "header" and .key != "SUM")] | length' "${test_json}")
            local total_file_count=$((file_count + test_file_count))
            
            show_progress "Processing cloc results for ${total_file_count} files..."
            
            # Combine the results into a single JSON array with proper structure
            # shellcheck disable=SC2016 # Single quotes intentional for jq script
            "${JQ}" -n \
                --slurpfile core "${core_json}" \
                --slurpfile test "${test_json}" \
                '
                # Helper function to determine file category
                def categorize_file:
                    if test("^tests/unity") then "Test C/Headers"
                    elif test("\\.sh$") then "Bourne Shell"
                    elif test("\\.(c|h|inc)$") then "Core C/Headers"
                    else "Other"
                    end;
                
                # Process both core and test files
                [
                    # Process core files
                    ($core[0] | to_entries[] |
                    select(.key != "header" and .key != "SUM") |
                    {
                        filename: .key,
                        language: .value.language,
                        blank: .value.blank,
                        comment: .value.comment,
                        code: .value.code,
                        category: (.key | categorize_file)
                    }),
                    
                    # Process test files
                    ($test[0] | to_entries[] |
                    select(.key != "header" and .key != "SUM") |
                    {
                        filename: .key,
                        language: .value.language,
                        blank: .value.blank,
                        comment: .value.comment,
                        code: .value.code,
                        category: (.key | categorize_file)
                    })
                ] |
                # Add calculated fields
                map(
                    . + {
                        lines: (.blank + .comment + .code),
                        comment_code_percentage: (
                            if .code > 0 then
                                ((.comment / .code * 100) * 10 | round / 10)
                            else 0
                            end
                        )
                    }
                )
                ' > "${output_json}"
            
            rm -f "${core_json}" "${test_json}" "${exclude_list}"
            return 0
        else
            echo "Test cloc command failed" >&2
            rm -f "${core_json}" "${test_json}" "${exclude_list}"
            return 1
        fi
    else
        echo "Core cloc command failed" >&2
        rm -f "${core_json}" "${test_json}" "${exclude_list}"
        return 1
    fi
}

# Function to create the main analysis table with filtering
create_comment_analysis_table() {
    local project_dir="$1"
    local output_file="${2:-}"

    # Create temporary files
    local temp_data temp_json temp_layout temp_output
    temp_json=$(mktemp) || return 1
    temp_data=$(mktemp) || return 1
    temp_layout=$(mktemp) || return 1

    # Handle output destination
    if [[ -n "${output_file}" ]]; then
        temp_output="${output_file}"
    else
        temp_output=$(mktemp) || return 1
    fi

    # Run cloc analysis with --by-file flag
    # shellcheck disable=SC2310 # Function invoked in condition, set -e disabled intentionally
    if ! run_cloc_with_by_file "${project_dir}" "${temp_json}"; then
        echo "Error: Failed to run cloc analysis" >&2
        rm -f "${temp_json}" "${temp_data}" "${temp_layout}"
        return 1
    fi

    show_progress "Filtering and sorting results..."

    # Get total count before filtering
    local total_analyzed
    total_analyzed=$("${JQ}" -r 'length' "${temp_json}")

    # Filter files to show only those with >50% comment ratios
    # Sort within each language category by comment percentage descending
    # shellcheck disable=SC2016 # Single quotes intentional for jq script
    "${JQ}" -r \
        --arg total "${total_analyzed}" \
        '
        # Filter to only files with >50% comment ratios
        map(select(.comment_code_percentage > 50)) |
        
        # Group by category
        group_by(.category) |
        
        # Sort each group by comment_code_percentage descending
        map(sort_by(.comment_code_percentage) | reverse) |
        
        # Build output with section breaks in proper order
        # Order: Test C/Headers, Core C/Headers, Bourne Shell
        {
            total_analyzed: ($total | tonumber),
            filtered_count: (map(length) | add // 0),
            data: (
                # Test C/Headers section
                (map(select(.[0].category == "Test C/Headers")) | .[0] // []) +
                # Core C/Headers section
                (map(select(.[0].category == "Core C/Headers")) | .[0] // []) +
                # Bourne Shell section
                (map(select(.[0].category == "Bourne Shell")) | .[0] // []) |
                # Format percentage and language values
                map(
                    # Format percentage with one decimal place
                    if .comment_code_percentage != "" and (.comment_code_percentage | type == "number") then
                        .comment_code_percentage = (
                            if (.comment_code_percentage | tostring | contains(".")) then
                                "\(.comment_code_percentage) %"
                            else
                                "\(.comment_code_percentage).0 %"
                            end
                        )
                    else . end |
                    # Use category as language for display and strip leading ./
                    .language = .category |
                    .filename = (.filename | sub("^\\./"; ""))
                )
            )
        }
        ' "${temp_json}" > "${temp_data}.meta"

    # Extract metadata
    local total_analyzed_final filtered_count
    total_analyzed_final=$("${JQ}" -r '.total_analyzed' "${temp_data}.meta")
    filtered_count=$("${JQ}" -r '.filtered_count' "${temp_data}.meta")

    # Extract just the data array
    "${JQ}" '.data' "${temp_data}.meta" > "${temp_data}"

    # Create layout JSON for TABLES program
    cat > "${temp_layout}" << EOF
{
    "title": "{BOLD}{WHITE}Comment Analysis Report{RESET} - Showing files with >50% comments",
    "subtitle": "{CYAN}Searched ${total_analyzed_final} files, showing ${filtered_count} of interest{RESET}",
    "footer": "{CYAN}Generated{WHITE} ${TIMESTAMP_DISPLAY}{RESET}",
    "footer_position": "right",
    "columns": [
        {
            "header": "Section",
            "key": "language",
            "datatype": "text",
            "visible": false,
            "break": true
        },
        {
            "header": "File",
            "key": "filename",
            "datatype": "text",
            "justification": "left",
            "summary": "count"
        },
        {
            "header": "Language",
            "key": "language",
            "datatype": "text",
            "justification": "left"
        },
        {
            "header": "Blank",
            "key": "blank",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Comment",
            "key": "comment",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Code",
            "key": "code",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "C/C %",
            "key": "comment_code_percentage",
            "datatype": "text",
            "justification": "right"
        },
        {
            "header": "Lines",
            "key": "lines",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
        }
    ]
}
EOF

    # Generate the table using TABLES program
    if ! "${TABLES}" "${temp_layout}" "${temp_data}" > "${temp_output}"; then
        echo "Error: TABLES command failed" >&2
        rm -f "${temp_json}" "${temp_data}" "${temp_data}.meta" "${temp_layout}" "${temp_output}"
        return 1
    fi

    # Output the result
    if [[ -z "${output_file}" ]]; then
        cat "${temp_output}"
        rm -f "${temp_output}"
    fi
    
    rm -f "${temp_json}" "${temp_data}" "${temp_data}.meta" "${temp_layout}"
    return 0
}

# Main function
main() {
    local project_dir="${1:-.}"
    local output_file="${2:-}"

    # Make project_dir absolute
    project_dir=$("${REALPATH}" "${project_dir}" 2>/dev/null) || project_dir="$(cd "${project_dir}" && pwd)"

    echo "Analyzing comment density in ${project_dir}..."
    echo "Categories: Test C/Headers, Core C/Headers, Bourne Shell"
    echo "Filtering: Showing only files with >50% comment ratios"

    # shellcheck disable=SC2310 # Function invoked in condition, set -e disabled intentionally
    create_comment_analysis_table "${project_dir}" "${output_file}"
    # ; then
#         echo "
#         # echo "Analysis complete. Files sorted by descending C/C % within each language."
#         # echo "Use this information to identify files that need comment balance adjustments."
#     else
#         echo "Error: Failed to create comment analysis table" >&2
#         exit 1
#     fi
}

# If script is run directly (not sourced), execute main function
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi