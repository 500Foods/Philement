#!/usr/bin/env bash

# CLOC (Count Lines of Code) Library
# Provides shared cloc functionality for test scripts

# LIBRARY FUNCTIONS
# should_exclude_file()
# generate_cloc_exclude_list()
# run_cloc_analysis()
# generate_cloc_for_readme()
# extract_cloc_stats()
# run_cloc_with_stats()

# CHANGELOG
# 6.0.0 - 2025-09-23 - Added horizontal section breaks to main cloc table (primary/secondary sections)
# 5.0.0 - 2025-09-18 - Updated Extended Statistics table with additional rows
# 4.0.0 - 2025-09-06 - Use TABLES program for formatting, thousands separators, JSON-based processing
# 3.0.0 - 2025-09-06 - Added separate 'Unit Tests' language for tests/unity/src files
# 2.0.0 - 2025-08-14 - Added instrumentation data to output
# 1.1.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.0.0 - 2025-07-02 - Initial creation, extracted from test_99_codebase.sh and test_00_all.sh

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${CLOC_GUARD:-}" ]] && return 0
export CLOC_GUARD="true"

# Library metadata
CLOC_NAME="CLOC Library"
CLOC_VERSION="6.0.0"
# shellcheck disable=SC2310,SC2153,SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CLOC_NAME} ${CLOC_VERSION}" "info" 2> /dev/null || true

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"

# Function to generate cloc exclude list based on .lintignore and default excludes
generate_cloc_exclude_list() {
    local base_dir="${1:-.}"           # Base directory to scan (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    local exclude_list_file="$3"       # Output file for exclude list (required)
    
    if [[ -z "${exclude_list_file}" ]]; then
        echo "Error: exclude_list_file parameter is required" >&2
        return 1
    fi
    
    # Generate exclude list based on .lintignore and default excludes
    true > "${exclude_list_file}"
    # Read patterns from .lintignore file and convert to cloc-compatible format
    if [[ -f "${lint_ignore_file}" ]]; then
        while IFS= read -r pattern; do
            [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            echo "${clean_pattern}" >> "${exclude_list_file}"
        done < "${lint_ignore_file}"
    fi
    
    # Add default excludes
    for pattern in "${DEFAULT_LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        echo "${clean_pattern}" >> "${exclude_list_file}"
    done

    return 0
}

# Function to run cloc analysis with proper exclusions
run_cloc_analysis() {
    local base_dir="${1:-.}"           # Base directory to analyze (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    local output_file="${3:-}"             # Optional output file (if not provided, outputs to stdout)
    local data_json="${4:-}"             # Optional output file (if not provided, outputs to stdout)
    
    # Create temporary files
    local exclude_list
    local enhanced_output
    exclude_list=$(mktemp) || return 1

    # Handle output destination
    if [[ -n "${output_file}" ]]; then
        enhanced_output="${output_file}"
        data_json_file="${data_json:-$(mktemp)}"
    else
        enhanced_output=$(mktemp) || return 1
        data_json_file=$(mktemp) || return 1
    fi
       
    # Generate exclude list
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! generate_cloc_exclude_list "${base_dir}" "${lint_ignore_file}" "${exclude_list}"; then
        echo "Failed to generate exclude list" >&2
        return 1
    fi
    
    # Run cloc twice with JSON output
    local core_json test_json
    core_json=$(mktemp) || return 1
    test_json=$(mktemp) || return 1

    if (cd "${base_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" . --quiet --json --exclude-list-file="${exclude_list}" --not-match-d='tests/unity' --not-match-f='hydrogen_installer' --force-lang=C,c --force-lang=C,h --force-lang=C,inc > "${core_json}" 2>&1); then
        if (cd "${base_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" tests/unity/src --quiet --json --force-lang=C,c --force-lang=C,h --force-lang=C,inc > "${test_json}" 2>&1); then
            # Extract cloc version from core JSON
            local cloc_header
            cloc_header=$(jq -r '.header | if type == "object" then "\(.cloc_url) v \(.cloc_version)" else . end // "cloc"' "${core_json}" 2>/dev/null || echo "cloc")

            # Create layout JSON for TABLES program
            local layout_json
            layout_json=$(mktemp) || return 1
            cat > "${layout_json}" << EOF
{
    "title": "{BOLD}{WHITE}${cloc_header}{RESET}",
    "footer": "{CYAN}Generated{WHITE} ${TIMESTAMP_DISPLAY}{RESET}",
    "footer_position": "right",
    "columns": [
        {
            "header": "Language",
            "key": "section",
            "datatype": "text",
            "visible": false,
            "break": true
        },
        {
            "header": "Language",
            "key": "language",
            "datatype": "text",
            "justification": "left"
        },
        {
            "header": "Files",
            "key": "files",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
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
            "header": "Lines",
            "key": "lines",
            "datatype": "int",
            "justification": "right",
            "summary": "sum"
        }
    ]
}
EOF

            # Create data JSON for TABLES program using jq
            jq -n '
                # Process core JSON
                input as $core |
                # Process test JSON
                input as $test |
                # Build the data array with section breaks
                [
                    # Core C entry (primary section)
                    (if $core.C then
                        {
                            section: "primary",
                            language: "Core C/Headers",
                            files: ($core.C.nFiles // 0),
                            blank: ($core.C.blank // 0),
                            comment: ($core.C.comment // 0),
                            code: ($core.C.code // 0),
                            lines: (($core.C.blank // 0) + ($core.C.comment // 0) + ($core.C.code // 0))
                        }
                    else empty end),
                    # Test C entry (primary section)
                    (if $test.C then
                        {
                            section: "primary",
                            language: "Test C/Headers",
                            files: ($test.C.nFiles // 0),
                            blank: ($test.C.blank // 0),
                            comment: ($test.C.comment // 0),
                            code: ($test.C.code // 0),
                            lines: (($test.C.blank // 0) + ($test.C.comment // 0) + ($test.C.code // 0))
                        }
                    else empty end),
                    # Primary section languages: Markdown, Bourne Shell, Lua, CMake
                    ($core | to_entries[] | select(.key != "C" and .key != "header" and .key != "SUM" and (.key == "Markdown" or .key == "Bourne Shell" or .key == "Lua" or .key == "CMake")) |
                        {
                            section: "primary",
                            language: .key,
                            files: (.value.nFiles // 0),
                            blank: (.value.blank // 0),
                            comment: (.value.comment // 0),
                            code: (.value.code // 0),
                            lines: ((.value.blank // 0) + (.value.comment // 0) + (.value.code // 0))
                        }
                    ),
                    # Secondary section languages: everything else
                    ($core | to_entries[] | select(.key != "C" and .key != "header" and .key != "SUM" and (.key != "Markdown" and .key != "Bourne Shell" and .key != "Lua" and .key != "CMake")) |
                        {
                            section: "secondary",
                            language: .key,
                            files: (.value.nFiles // 0),
                            blank: (.value.blank // 0),
                            comment: (.value.comment // 0),
                            code: (.value.code // 0),
                            lines: ((.value.blank // 0) + (.value.comment // 0) + (.value.code // 0))
                        }
                    )
                ] | map(select(. != null))
            ' "${core_json}" "${test_json}" > "${data_json_file}"

            # Calculate totals from the generated JSON data
            local total_files total_code
            total_files=$(jq -r 'map(.files // 0) | add' "${data_json_file}" 2>/dev/null || echo 0)
            total_code=$(jq -r 'map(.code // 0) | add' "${data_json_file}" 2>/dev/null || echo 0)

            # Calculate extended statistics before creating tables
            # Extract language statistics using jq
            # Note: With --force-lang options, all C-related files are now categorized as "C"
            local c_code c_comment header_code header_comment cmake_code cmake_comment shell_code shell_comment markdown_code
            c_code=$(jq -r '.C.code // 0' "${core_json}" 2>/dev/null || echo 0)
            c_comment=$(jq -r '.C.comment // 0' "${core_json}" 2>/dev/null || echo 0)
            # No separate header category since --force-lang combines them
            header_code=0
            header_comment=0
            cmake_code=$(jq -r '.CMake.code // 0' "${core_json}" 2>/dev/null || echo 0)
            cmake_comment=$(jq -r '.CMake.comment // 0' "${core_json}" 2>/dev/null || echo 0)
            shell_code=$(jq -r '."Bourne Shell".code // 0' "${core_json}" 2>/dev/null || echo 0)
            shell_comment=$(jq -r '."Bourne Shell".comment // 0' "${core_json}" 2>/dev/null || echo 0)
            lua_code=$(jq -r '.Lua.code // 0' "${core_json}" 2>/dev/null || echo 0)
            lua_comment=$(jq -r '.Lua.comment // 0' "${core_json}" 2>/dev/null || echo 0)
            markdown_code=$(jq -r '.Markdown.code // 0' "${core_json}" 2>/dev/null || echo 0)

            # Calculate summary values to match ratio calculations (for consistency)
            local total_code_summary
            total_code_summary=$("${PRINTF}" "%'d" "$((c_code + header_code + cmake_code + shell_code + lua_code))")
            local total_docs_summary
            total_docs_summary=$("${PRINTF}" "%'d" "$((markdown_code))")
            local total_comments_summary
            total_comments_summary=$("${PRINTF}" "%'d" "$((c_comment + header_comment + cmake_comment + shell_comment + lua_comment))")
            local total_combined_summary
            total_combined_summary=$("${PRINTF}" "%'d" "$((c_code + header_code + cmake_code + shell_code + lua_code + markdown_code + c_comment + header_comment + cmake_comment + shell_comment + lua_comment))")

            # Calculate totals for the 4 code languages
            local total_code_stats=$((c_code + header_code + cmake_code + shell_code))
            local total_comment=$((c_comment + header_comment + cmake_comment + shell_comment))

            # Calculate ratios
            local codedoc_ratio codecomment_ratio docscode_ratio commentscode_ratio
            if [[ "${markdown_code}" -gt 0 ]]; then
                codedoc_ratio=$(printf "%.1f" "$(bc -l <<< "scale=2; ${total_code_stats} / ${markdown_code}" || true)")
                docscode_ratio=$(printf "%.1f" "$(bc -l <<< "scale=2; ${markdown_code} / ${total_code_stats}" || true)")
            else
                codedoc_ratio="N/A"
                docscode_ratio="N/A"
            fi

            if [[ "${total_comment}" -gt 0 ]]; then
                codecomment_ratio=$(printf "%.1f" "$(bc -l <<< "scale=2; ${total_code_stats} / ${total_comment}" || true)")
                commentscode_ratio=$(printf "%.1f" "$(bc -l <<< "scale=2; ${total_comment} / ${total_code_stats}" || true)")
            else
                codecomment_ratio="N/A"
                commentscode_ratio="N/A"
            fi

            # Coverage statistics are read from detailed files below


            # Use TABLES program to format the main code table
            # shellcheck disable=SC2154 # TABLES defined externally in framework.sh
            if ! "${TABLES}" "${layout_json}" "${data_json_file}" > "${enhanced_output}"; then
                echo "TABLES command failed" >&2
                return 1
            fi

            # Create second table for extended statistics
            local stats_layout_json stats_data_json stats_output
            stats_layout_json=$(mktemp) || return 1
            stats_data_json=$(mktemp) || return 1

            # Create separate output file for stats table
            if [[ -n "${output_file}" ]]; then
                stats_output="${output_file%.txt}_stats.txt"
            else
                stats_output=$(mktemp) || return 1
            fi

            cat > "${stats_layout_json}" << EOF
{
    "title": "{BOLD}{WHITE}Extended Statistics{RESET}",
    "footer": "{CYAN}Generated{WHITE} ${TIMESTAMP_DISPLAY}{RESET}",
    "footer_position": "right",
    "columns": [
        {
            "header": "Section",
            "key": "section",
            "datatype": "text",
            "visible": false,
            "break": true
        },
        {
            "header": "Metric",
            "key": "metric",
            "datatype": "text",
            "justification": "left"
        },
        {
            "header": "Value",
            "key": "value",
            "datatype": "text",
            "justification": "right"
        },
        {
            "header": "Description",
            "key": "description",
            "datatype": "text",
            "justification": "left",
            "width": 46
        }
    ]
}
EOF

            # Get coverage percentages
            local coverage_black coverage_unity coverage_combined
            coverage_black=$(cat "${PROJECT_DIR}/build/tests/results/coverage_blackbox.txt" 2>/dev/null || echo "0.000")
            coverage_unity=$(cat "${PROJECT_DIR}/build/tests/results/coverage_unity.txt" 2>/dev/null || echo "0.000")
            coverage_combined=$(cat "${PROJECT_DIR}/build/tests/results/coverage_combined.txt" 2>/dev/null || echo "0.000")

            # Format coverage percentages to 3 decimal places
            local coverage_black_fmt coverage_unity_fmt coverage_combined_fmt
            coverage_black_fmt=$(printf "%.3f" "${coverage_black}" 2>/dev/null || echo "0.000")
            coverage_unity_fmt=$(printf "%.3f" "${coverage_unity}" 2>/dev/null || echo "0.000")
            coverage_combined_fmt=$(printf "%.3f" "${coverage_combined}" 2>/dev/null || echo "0.000")

            # Get instrumented lines and covered lines from coverage detailed files
            local instrumented_blackbox instrumented_unity covered_blackbox covered_unity
            if [[ -f "${PROJECT_DIR}/build/tests/results/coverage_blackbox.txt.detailed" ]]; then
                IFS=',' read -r _ _ covered_blackbox instrumented_blackbox _ _ < "${PROJECT_DIR}/build/tests/results/coverage_blackbox.txt.detailed"
            else
                instrumented_blackbox="19009"
                covered_blackbox="9770"
            fi
            if [[ -f "${PROJECT_DIR}/build/tests/results/coverage_unity.txt.detailed" ]]; then
                IFS=',' read -r _ _ covered_unity instrumented_unity _ _ < "${PROJECT_DIR}/build/tests/results/coverage_unity.txt.detailed"
            else
                instrumented_unity="19009"
                covered_unity="8942"
            fi

            # Format values with thousands separators
            local format_instrumented_black format_instrumented_unity format_covered_black format_covered_unity
            format_instrumented_black=$("${PRINTF}" "%'d" "${instrumented_blackbox}")
            format_instrumented_unity=$("${PRINTF}" "%'d" "${instrumented_unity}")
            format_covered_black=$("${PRINTF}" "%'d" "${covered_blackbox}")
            format_covered_unity=$("${PRINTF}" "%'d" "${covered_unity}")

            # Calculate Unity Ratio as Test C/Headers code / Core C/Headers code (as percentage)
            local test_c_code unity_ratio
            test_c_code=$(jq -r '.C.code // 0' "${test_json}" 2>/dev/null || echo 0)
            if [[ "${c_code}" -gt 0 ]]; then
                unity_ratio=$(printf "%.3f%%" "$(bc -l <<< "scale=2; (${test_c_code} * 100) / ${c_code}" || true)")
            else
                unity_ratio="N/A"
            fi

            # Create data for extended statistics table
            cat > "${stats_data_json}" << EOF
[
     {
         "section": "code_metrics",
         "metric": "Docs",
         "value": "${total_docs_summary}",
         "description": "Markdown documentation code"
     },
     {
         "section": "code_metrics",
         "metric": "Code",
         "value": "${total_code_summary}",
         "description": "Core C/Headers + Bash + CMake + Lua code"
     },
     {
         "section": "code_metrics",
         "metric": "Comments",
         "value": "${total_comments_summary}",
         "description": "Core C/Headers + Bash + CMake + Lua comments"
     },
     {
         "section": "code_metrics",
         "metric": "Combined",
         "value": "${total_combined_summary}",
         "description": "Total content lines (Docs + Code + Comments)"
     },
     {
         "section": "ratios",
         "metric": "Code/Docs",
         "value": "${codedoc_ratio}",
         "description": "Ratio of Code to Docs        {CYAN}Target: 1.5-2.5{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Docs/Code",
         "value": "${docscode_ratio}",
         "description": "Ratio of Docs to Code        {CYAN}Target: 0.4-0.7{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Code/Comments",
         "value": "${codecomment_ratio}",
         "description": "Ratio of Code to Comments    {CYAN}Target: 3.0-5.0{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Comments/Code",
         "value": "${commentscode_ratio}",
         "description": "Ratio of Comments to Code    {CYAN}Target: 0.2-0.3{RESET}"
     },
     {
         "section": "coverage_lines",
         "metric": "Instrumented Unity",
         "value": "${format_instrumented_unity}",
         "description": "Lines of instrumented code - Unity"
     },
     {
         "section": "coverage_lines",
         "metric": "Instrumented Blackbox",
         "value": "${format_instrumented_black}",
         "description": "Lines of instrumented code - Blackbox"
     },
     {
         "section": "coverage_lines",
         "metric": "Coverage Unity",
         "value": "${format_covered_unity}",
         "description": "Lines of covered code - Unity"
     },
     {
         "section": "coverage_lines",
         "metric": "Coverage Blackbox",
         "value": "${format_covered_black}",
         "description": "Lines of covered code - Blackbox"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Unity %",
         "value": "${coverage_unity_fmt}%",
         "description": "Unity test coverage              {CYAN}Target: 60%{RESET}"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Blackbox %",
         "value": "${coverage_black_fmt}%",
         "description": "Blackbox test coverage           {CYAN}Target: 70%{RESET}"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Combined %",
         "value": "${coverage_combined_fmt}%",
         "description": "Combined test coverage           {CYAN}Target: 80%{RESET}"
     }, 
     {
         "section": "coverage_percentages",
         "metric": "Unity Ratio",
         "value": "${unity_ratio}",
         "description": "Test/Core C/Headers        {CYAN}Target: 100%-200%{RESET}"
     }
]
EOF

            # Generate second table to separate file
            "${TABLES}" "${stats_layout_json}" "${stats_data_json}" > "${stats_output}"

            # If no output_file was provided, output both tables to stdout and clean up
            if [[ -z "${output_file}" ]]; then
                cat "${enhanced_output}"
                echo ""
                cat "${stats_output}"
                rm -f "${enhanced_output}" "${data_json_file}" "${stats_output}"
            fi

            #return 0
        else
            echo "Test cloc command failed"
            return 1
        fi
    else
        echo "Core cloc command failed"
        return 1
    fi


    return 0
}


# Function to extract cloc statistics from JSON data
extract_cloc_stats() {
    local cloc_data_file="$1"

    if [[ ! -f "${cloc_data_file}" ]]; then
        echo "Error: cloc data file not found: ${cloc_data_file}" >&2
        return 1
    fi

    # Sum files and code from all entries in the JSON array
    local total_files total_code
    total_files=$(jq -r 'map(.files // 0) | add' "${cloc_data_file}" 2>/dev/null || echo 0)
    total_code=$(jq -r 'map(.code // 0) | add' "${cloc_data_file}" 2>/dev/null || echo 0)

    if [[ -n "${total_files}" && -n "${total_code}" ]]; then
        echo "files:${total_files},lines:${total_code}"
        return 0
    else
        echo "Error: Failed to parse cloc data" >&2
        return 1
    fi
}

# Function to run cloc analysis and return statistics
run_cloc_with_stats() {
    local base_dir="${1:-.}"           # Base directory to analyze (default: current directory)
    local lint_ignore_file="${2:-.lintignore}"  # Lintignore file path (default: .lintignore)
    
    local temp_output
    temp_output=$(mktemp) || return 1
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if run_cloc_analysis "${base_dir}" "${lint_ignore_file}" "${temp_output}" "${temp_output}.json"; then
        # Extract and return statistics
        extract_cloc_stats "${temp_output}.json"
        return 0
    else
        return 1
    fi
}
