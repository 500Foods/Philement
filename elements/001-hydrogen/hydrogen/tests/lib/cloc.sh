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
# 6.6.1 - 2025-12-02 - Fixed ShellCheck SC2312 and SC2310 issues with explicit error handling
# 6.6.0 - 2025-12-02 - Added file size reporting section to extended statistics table
# 6.5.1 - 2025-11-20 - Updated coverage target thresholds in extended statistics table to 70/75/80 from 60/70/80
# 6.5.0 - 2025-11-17 - Added "Coverage Combined" row to extended statistics table
# 6.4.0 - 2025-10-15 - Added "Instrumented Tests" row to extended statistics table
# 6.3.1 - 2025-09-30 - Fixed decimal .0 problem
# 6.3.0 - 2025-09-29 - Added sorting by total lines within primary/secondary sections in table 1
# 6.2.0 - 2025-09-29 - Promoted JavaScript to primary section in table 1, included in Code metrics in table 2
# 6.1.0 - 2025-09-26 - Added C/C column to table 1, color coded targets in table 2
# 6.0.1 - 2025-09-25 - Fixed count to match C files found in Test 01 and Test 91
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
CLOC_VERSION="6.6.1"
# shellcheck disable=SC2310,SC2153,SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${CLOC_NAME} ${CLOC_VERSION}" "info" 2> /dev/null || true

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"

# Function to calculate file sizes for the size reporting section
calculate_file_sizes() {
    local project_root="${1}"

    # Initialize all sizes to 0
    local naked_size="0.000"
    local release_size="0.000"
    local payload_size="0.000"
    local installer_size="0.000"
    local repo_size="0.000"

    # Calculate Hydrogen Binary size (first item)
    if [[ -f "${project_root}/hydrogen_release" ]]; then
        release_size=$(stat -c "%s" "${project_root}/hydrogen_release" 2>/dev/null || echo "0")
        if release_size_calc=$(bc -l <<< "scale=0; ${release_size} / 1024" 2>/dev/null); then
            release_size=$(printf "%.0f" "${release_size_calc}")
        else
            release_size="0"
        fi
    fi

    # Calculate Hydrogen Naked size
    if [[ -f "${project_root}/hydrogen_naked" ]]; then
        naked_size=$(stat -c "%s" "${project_root}/hydrogen_naked" 2>/dev/null || echo "0")
        if naked_size_calc=$(bc -l <<< "scale=0; ${naked_size} / 1024" 2>/dev/null); then
            naked_size=$(printf "%.0f" "${naked_size_calc}")
        else
            naked_size="0"
        fi
    fi

    # Calculate Payload size
    if [[ -f "${project_root}/payloads/payload.tar.br.enc" ]]; then
        payload_size=$(stat -c "%s" "${project_root}/payloads/payload.tar.br.enc" 2>/dev/null || echo "0")
        if payload_size_calc=$(bc -l <<< "scale=0; ${payload_size} / 1024" 2>/dev/null); then
            payload_size=$(printf "%.0f" "${payload_size_calc}")
        else
            payload_size="0"
        fi
    fi

    # Calculate Installer size (find the latest installer file)
    local latest_installer=""
    if [[ -d "${project_root}/installer" ]]; then
        # Find the most recent installer file by date in filename
        latest_installer=$(find "${project_root}/installer" -name "hydrogen_installer_*.sh" -type f 2>/dev/null | sort | tail -n 1)
        if [[ -n "${latest_installer}" && -f "${latest_installer}" ]]; then
            installer_size=$(stat -c "%s" "${latest_installer}" 2>/dev/null || echo "0")
            if installer_size_calc=$(bc -l <<< "scale=0; ${installer_size} / 1024" 2>/dev/null); then
                installer_size=$(printf "%.0f" "${installer_size_calc}")
            else
                installer_size="0"
            fi
        fi
    fi

    # Calculate Repository size (excluding build/ and tests/unity/framework folders)
    if [[ -d "${project_root}" ]]; then
        # Use du to calculate size, excluding specified directories
        local repo_bytes="0"
        if command -v du >/dev/null 2>&1; then
            # Calculate size excluding build/ and tests/unity/framework directories
            repo_bytes=$(find "${project_root}" -type f \
                -not \( -path "${project_root}/build/*" -o -path "${project_root}/tests/unity/framework/*" \) \
                -exec stat -c "%s" {} + 2>/dev/null | awk '{sum += $1} END {print sum}' 2>/dev/null || echo "0")
            if [[ "${repo_bytes}" -gt 0 ]]; then
                # Convert to MB (not GB)
                if repo_size_calc=$(bc -l <<< "scale=0; ${repo_bytes} / (1024*1024)" 2>/dev/null); then
                    repo_size=$(printf "%.0f" "${repo_size_calc}")
                else
                    repo_size="0"
                fi
            fi
        fi
    fi

    # Return the sizes as a JSON object for easy parsing
    cat << EOF
{
    "naked_size": "${naked_size}",
    "release_size": "${release_size}",
    "payload_size": "${payload_size}",
    "installer_size": "${installer_size}",
    "repo_size": "${repo_size}"
}
EOF
}

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

    if (cd "${base_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" . --quiet --json --exclude-list-file="${exclude_list}" --not-match-d="build" --not-match-d='tests/lib/node_modules' --not-match-d='tests/unity' --not-match-f='hydrogen_installer' --force-lang=C,c --force-lang=C,h --force-lang=C,inc > "${core_json}" 2>&1); then
        if (cd "${base_dir}" && env LC_ALL=en_US.UTF_8 "${CLOC}" tests/unity --not-match-d='tests/unity/framework' --quiet --json --force-lang=C,c --force-lang=C,h --force-lang=C,inc > "${test_json}" 2>&1); then
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
                            language: "Core Code C/Headers",
                            files: ($core.C.nFiles // 0),
                            blank: ($core.C.blank // 0),
                            comment: ($core.C.comment // 0),
                            code: ($core.C.code // 0),
                            comment_code_percentage: (if ($core.C.code // 0) > 0 then (if (($core.C.comment // 0) / ($core.C.code // 0) * 100) > 0 then ((($core.C.comment // 0) / ($core.C.code // 0) * 100 | . * 10 | round / 10) | tostring + " %") else "" end) else "" end),
                            lines: (($core.C.blank // 0) + ($core.C.comment // 0) + ($core.C.code // 0))
                        }
                    else empty end),
                    # Test C entry (primary section)
                    (if $test.C then
                        {
                            section: "primary",
                            language: "Unit Test C/Headers",
                            files: ($test.C.nFiles // 0),
                            blank: ($test.C.blank // 0),
                            comment: ($test.C.comment // 0),
                            code: ($test.C.code // 0),
                            comment_code_percentage: (if ($test.C.code // 0) > 0 then (if (($test.C.comment // 0) / ($test.C.code // 0) * 100) > 0 then ((($test.C.comment // 0) / ($test.C.code // 0) * 100 | . * 10 | round / 10) | tostring + " %") else "" end) else "" end),
                            lines: (($test.C.blank // 0) + ($test.C.comment // 0) + ($test.C.code // 0))
                        }
                    else empty end),
                    # Primary section languages: Markdown, Bourne Shell, Lua, CMake, JavaScript
                    # Combine counts from core and test scans for these languages
                    (
                        # Markdown
                        (($core.Markdown // {}) as $core_md | ($test.Markdown // {}) as $test_md |
                            {
                                section: "primary",
                                language: "Markdown",
                                files: (($core_md.nFiles // 0) + ($test_md.nFiles // 0)),
                                blank: (($core_md.blank // 0) + ($test_md.blank // 0)),
                                comment: (($core_md.comment // 0) + ($test_md.comment // 0)),
                                code: (($core_md.code // 0) + ($test_md.code // 0)),
                                comment_code_percentage: (if (($core_md.code // 0) + ($test_md.code // 0)) > 0 then (if (((($core_md.comment // 0) + ($test_md.comment // 0))) / (($core_md.code // 0) + ($test_md.code // 0)) * 100) > 0 then (((($core_md.comment // 0) + ($test_md.comment // 0))) / (($core_md.code // 0) + ($test_md.code // 0)) * 100 | . * 10 | round / 10 | tostring + " %") else "" end) else "" end)
                            } | .lines = (.blank + .comment + .code)
                        ),
                        # Bourne Shell
                        (($core."Bourne Shell" // {}) as $core_sh | ($test."Bourne Shell" // {}) as $test_sh |
                            {
                                section: "primary",
                                language: "Bourne Shell",
                                files: (($core_sh.nFiles // 0) + ($test_sh.nFiles // 0)),
                                blank: (($core_sh.blank // 0) + ($test_sh.blank // 0)),
                                comment: (($core_sh.comment // 0) + ($test_sh.comment // 0)),
                                code: (($core_sh.code // 0) + ($test_sh.code // 0)),
                                comment_code_percentage: (if (($core_sh.code // 0) + ($test_sh.code // 0)) > 0 then (if (((($core_sh.comment // 0) + ($test_sh.comment // 0))) / (($core_sh.code // 0) + ($test_sh.code // 0)) * 100) > 0 then (((($core_sh.comment // 0) + ($test_sh.comment // 0))) / (($core_sh.code // 0) + ($test_sh.code // 0)) * 100 | . * 10 | round / 10 | tostring | if contains(".") then . else . + ".0" end | . + " %") else "" end) else "" end)
                            } | .lines = (.blank + .comment + .code)
                        ),
                        # Lua
                        (($core.Lua // {}) as $core_lua | ($test.Lua // {}) as $test_lua |
                            {
                                section: "primary",
                                language: "Lua",
                                files: (($core_lua.nFiles // 0) + ($test_lua.nFiles // 0)),
                                blank: (($core_lua.blank // 0) + ($test_lua.blank // 0)),
                                comment: (($core_lua.comment // 0) + ($test_lua.comment // 0)),
                                code: (($core_lua.code // 0) + ($test_lua.code // 0)),
                                comment_code_percentage: (if (($core_lua.code // 0) + ($test_lua.code // 0)) > 0 then (if (((($core_lua.comment // 0) + ($test_lua.comment // 0))) / (($core_lua.code // 0) + ($test_lua.code // 0)) * 100) > 0 then (((($core_lua.comment // 0) + ($test_lua.comment // 0))) / (($core_lua.code // 0) + ($test_lua.code // 0)) * 100 | . * 10 | round / 10 | tostring + " %") else "" end) else "" end)
                            } | .lines = (.blank + .comment + .code)
                        ),
                        # CMake
                        (($core.CMake // {}) as $core_cmake | ($test.CMake // {}) as $test_cmake |
                            {
                                section: "primary",
                                language: "CMake",
                                files: (($core_cmake.nFiles // 0) + ($test_cmake.nFiles // 0)),
                                blank: (($core_cmake.blank // 0) + ($test_cmake.blank // 0)),
                                comment: (($core_cmake.comment // 0) + ($test_cmake.comment // 0)),
                                code: (($core_cmake.code // 0) + ($test_cmake.code // 0)),
                                comment_code_percentage: (if (($core_cmake.code // 0) + ($test_cmake.code // 0)) > 0 then (if (((($core_cmake.comment // 0) + ($test_cmake.comment // 0))) / (($core_cmake.code // 0) + ($test_cmake.code // 0)) * 100) > 0 then (((($core_cmake.comment // 0) + ($test_cmake.comment // 0))) / (($core_cmake.code // 0) + ($test_cmake.code // 0)) * 100 | . * 10 | round / 10 | tostring + " %") else "" end) else "" end)
                            } | .lines = (.blank + .comment + .code)
                        ),
                        # JavaScript
                        (($core.JavaScript // {}) as $core_js | ($test.JavaScript // {}) as $test_js |
                            {
                                section: "primary",
                                language: "JavaScript",
                                files: (($core_js.nFiles // 0) + ($test_js.nFiles // 0)),
                                blank: (($core_js.blank // 0) + ($test_js.blank // 0)),
                                comment: (($core_js.comment // 0) + ($test_js.comment // 0)),
                                code: (($core_js.code // 0) + ($test_js.code // 0)),
                                comment_code_percentage: (if (($core_js.code // 0) + ($test_js.code // 0)) > 0 then (if (((($core_js.comment // 0) + ($test_js.comment // 0))) / (($core_js.code // 0) + ($test_js.code // 0)) * 100) > 0 then (((($core_js.comment // 0) + ($test_js.comment // 0))) / (($core_js.code // 0) + ($test_js.code // 0)) * 100 | . * 10 | round / 10 | tostring + " %") else "" end) else "" end)
                            } | .lines = (.blank + .comment + .code)
                        )
                    ),
                    # Secondary section languages: everything else
                    ($core | to_entries[] | select(.key != "C" and .key != "header" and .key != "SUM" and (.key != "Markdown" and .key != "Bourne Shell" and .key != "Lua" and .key != "CMake" and .key != "JavaScript")) |
                        {
                            section: "secondary",
                            language: .key,
                            files: (.value.nFiles // 0),
                            blank: (.value.blank // 0),
                            comment: (.value.comment // 0),
                            code: (.value.code // 0),
                            comment_code_percentage: (if (.value.code // 0) > 0 then (if ((.value.comment // 0) / (.value.code // 0) * 100) > 0 then ((.value.comment // 0) / (.value.code // 0) * 100 | . * 10 | round / 10 | tostring + " %") else "" end) else "" end),
                            lines: ((.value.blank // 0) + (.value.comment // 0) + (.value.code // 0))
                        }
                    )
                ] | map(select(. != null)) |
                ( . | map(select(.section == "primary")) | sort_by(.lines) | reverse ) + ( . | map(select(.section == "secondary")) | sort_by(.lines) | reverse )
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
            javascript_code=$(jq -r '.JavaScript.code // 0' "${core_json}" 2>/dev/null || echo 0)
            javascript_comment=$(jq -r '.JavaScript.comment // 0' "${core_json}" 2>/dev/null || echo 0)
            markdown_code=$(jq -r '.Markdown.code // 0' "${core_json}" 2>/dev/null || echo 0)

            # Calculate summary values to match ratio calculations (for consistency)
            local total_code_summary total_test_summary
            total_code_summary=$("${PRINTF}" "%'d" "$((c_code + header_code + cmake_code + shell_code + lua_code + javascript_code))")

            # Extract test code value for the Test metric
            local test_code_value
            test_code_value=$(jq -r '.C.code // 0' "${test_json}" 2>/dev/null || echo 0)
            total_test_summary=$("${PRINTF}" "%'d" "${test_code_value}")

            local total_docs_summary
            total_docs_summary=$("${PRINTF}" "%'d" "$((markdown_code))")
            local total_comments_summary
            total_comments_summary=$("${PRINTF}" "%'d" "$((c_comment + header_comment + cmake_comment + shell_comment + lua_comment + javascript_comment))")
            local total_combined_summary
            total_combined_summary=$("${PRINTF}" "%'d" "$((c_code + header_code + cmake_code + shell_code + lua_code + markdown_code + c_comment + header_comment + cmake_comment + shell_comment + lua_comment))")

            # Calculate totals for the 5 code languages
            local total_code_stats=$((c_code + header_code + cmake_code + shell_code + lua_code + javascript_code))
            local total_comment=$((c_comment + header_comment + cmake_comment + shell_comment + lua_comment + javascript_comment))

            # Calculate ratios
            local codedoc_ratio codecomment_ratio docscode_ratio commentscode_ratio
            if [[ "${markdown_code}" -gt 0 ]]; then
                codedoc_ratio=$(printf "%.3f" "$(bc -l <<< "scale=3; ${total_code_stats} / ${markdown_code}" || true)")
                docscode_ratio=$(printf "%.3f" "$(bc -l <<< "scale=3; ${markdown_code} / ${total_code_stats}" || true)")
            else
                codedoc_ratio="N/A"
                docscode_ratio="N/A"
            fi

            if [[ "${total_comment}" -gt 0 ]]; then
                codecomment_ratio=$(printf "%.3f" "$(bc -l <<< "scale=3; ${total_code_stats} / ${total_comment}" || true)")
                commentscode_ratio=$(printf "%.3f" "$(bc -l <<< "scale=3; ${total_comment} / ${total_code_stats}" || true)")
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
            "width": 51
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
            local instrumented_blackbox instrumented_unity covered_blackbox covered_unity instrumented_tests
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
            # Get test instrumented lines - check environment variable first, then file
            instrumented_tests="${TEST_INSTRUMENTED_LINES:-}"
            if [[ -z "${instrumented_tests}" ]]; then
                instrumented_tests=$(cat "${PROJECT_DIR}/build/tests/results/test_instrumented_lines.txt" 2>/dev/null || echo "0")
                if [[ "${instrumented_tests}" == "0" ]]; then
                    # Try to calculate it on the fly if not cached
                    local unity_build_dir="${PROJECT_DIR}/build/unity"
                    local timestamp
                    timestamp=$("${DATE}" +"%Y-%m-%d %H:%M:%S")
                    # Source coverage functions if available
                    [[ -n "${COVERAGE_GUARD:-}" ]] || source "${PROJECT_DIR}/tests/lib/coverage.sh" >/dev/null 2>&1 || true
                    # shellcheck disable=SC2310 # We want to continue even if the test fails
                    instrumented_tests=$(calculate_test_instrumented_lines "${unity_build_dir}" "${timestamp}" 2>/dev/null || echo "0")
                fi
            fi

            # Debug: ensure we have a valid number
            if [[ -z "${instrumented_tests}" ]] || [[ "${instrumented_tests}" == "0" ]]; then
                instrumented_tests="0"
            fi

            # Format values with thousands separators
            local format_instrumented_black format_instrumented_unity format_covered_black format_covered_unity format_instrumented_tests
            format_instrumented_black=$("${PRINTF}" "%'d" "${instrumented_blackbox}")
            format_instrumented_unity=$("${PRINTF}" "%'d" "${instrumented_unity}")
            format_covered_black=$("${PRINTF}" "%'d" "${covered_blackbox}")
            format_covered_unity=$("${PRINTF}" "%'d" "${covered_unity}")
            # Calculate covered lines for combined coverage
            local covered_combined
            covered_combined=$(printf "%.0f" "$(bc -l <<< "scale=0; (${coverage_combined} * ${instrumented_blackbox}) / 100" 2>/dev/null || echo 0)" 2>/dev/null || echo 0) || true
            local format_covered_combined
            format_covered_combined=$("${PRINTF}" "%'d" "${covered_combined}")
            # Debug: ensure instrumented_tests is not empty before formatting
            if [[ -n "${instrumented_tests}" ]] && [[ "${instrumented_tests}" != "0" ]]; then
                format_instrumented_tests=$("${PRINTF}" "%'d" "${instrumented_tests}")
            else
                format_instrumented_tests=""
            fi

            # Calculate Unity Ratio as Test C/Headers code / Core C/Headers code (as percentage)
            local test_c_code unity_ratio
            test_c_code=$(jq -r '.C.code // 0' "${test_json}" 2>/dev/null || echo 0)
            if [[ "${c_code}" -gt 0 ]]; then
                unity_ratio=$(printf "%.3f %%" "$(bc -l <<< "scale=2; (${test_c_code} * 100) / ${c_code}" || true)")
            else
                unity_ratio="N/A"
            fi

            # Calculate file sizes for the new size reporting section
            local file_sizes_json
            # shellcheck disable=SC2310 # Function failure should not exit script
            if ! file_sizes_json=$(calculate_file_sizes "${base_dir}" 2>/dev/null); then
                file_sizes_json='{"naked_size": "0.000", "release_size": "0.000", "payload_size": "0.000", "installer_size": "0.000", "repo_size": "0.000"}'
            fi
            local naked_size release_size payload_size installer_size repo_size
            naked_size=$(echo "${file_sizes_json}" | jq -r '.naked_size' 2>/dev/null || echo "0.000")
            release_size=$(echo "${file_sizes_json}" | jq -r '.release_size' 2>/dev/null || echo "0.000")
            payload_size=$(echo "${file_sizes_json}" | jq -r '.payload_size' 2>/dev/null || echo "0.000")
            installer_size=$(echo "${file_sizes_json}" | jq -r '.installer_size' 2>/dev/null || echo "0.000")
            repo_size=$(echo "${file_sizes_json}" | jq -r '.repo_size' 2>/dev/null || echo "0.000")

            # Compute all bc-based checks before creating JSON to avoid nested command substitutions
            local codedoc_color docscode_color codecomment_color commentscode_color coverage_unity_color coverage_black_color coverage_combined_color unity_color
            
            if [[ "${codedoc_ratio}" != "N/A" ]]; then
                local codedoc_check1 codedoc_check2
                codedoc_check1=$(echo "${codedoc_ratio} >= 1.5" | bc -l 2>/dev/null || echo 0)
                codedoc_check2=$(echo "${codedoc_ratio} <= 2.5" | bc -l 2>/dev/null || echo 0)
                if (( codedoc_check1 )) && (( codedoc_check2 )); then
                    codedoc_color="{GREEN}"
                else
                    codedoc_color="{CYAN}"
                fi
            else
                codedoc_color="{CYAN}"
            fi
            
            if [[ "${docscode_ratio}" != "N/A" ]]; then
                local docscode_check1 docscode_check2
                docscode_check1=$(echo "${docscode_ratio} >= 0.4" | bc -l 2>/dev/null || echo 0)
                docscode_check2=$(echo "${docscode_ratio} <= 0.667" | bc -l 2>/dev/null || echo 0)
                if (( docscode_check1 )) && (( docscode_check2 )); then
                    docscode_color="{GREEN}"
                else
                    docscode_color="{CYAN}"
                fi
            else
                docscode_color="{CYAN}"
            fi
            
            if [[ "${codecomment_ratio}" != "N/A" ]]; then
                local codecomment_check1 codecomment_check2
                codecomment_check1=$(echo "${codecomment_ratio} >= 3.0" | bc -l 2>/dev/null || echo 0)
                codecomment_check2=$(echo "${codecomment_ratio} <= 5.0" | bc -l 2>/dev/null || echo 0)
                if (( codecomment_check1 )) && (( codecomment_check2 )); then
                    codecomment_color="{GREEN}"
                else
                    codecomment_color="{CYAN}"
                fi
            else
                codecomment_color="{CYAN}"
            fi
            
            if [[ "${commentscode_ratio}" != "N/A" ]]; then
                local commentscode_check1 commentscode_check2
                commentscode_check1=$(echo "${commentscode_ratio} >= 0.2" | bc -l 2>/dev/null || echo 0)
                commentscode_check2=$(echo "${commentscode_ratio} <= 0.333" | bc -l 2>/dev/null || echo 0)
                if (( commentscode_check1 )) && (( commentscode_check2 )); then
                    commentscode_color="{GREEN}"
                else
                    commentscode_color="{CYAN}"
                fi
            else
                commentscode_color="{CYAN}"
            fi
            
            if [[ "${coverage_unity_fmt}" != "N/A" ]]; then
                local coverage_unity_check
                coverage_unity_check=$(echo "${coverage_unity_fmt} >= 70" | bc -l 2>/dev/null || echo 0)
                if (( coverage_unity_check )); then
                    coverage_unity_color="{GREEN}"
                else
                    coverage_unity_color="{CYAN}"
                fi
            else
                coverage_unity_color="{CYAN}"
            fi
            
            if [[ "${coverage_black_fmt}" != "N/A" ]]; then
                local coverage_black_check
                coverage_black_check=$(echo "${coverage_black_fmt} >= 75" | bc -l 2>/dev/null || echo 0)
                if (( coverage_black_check )); then
                    coverage_black_color="{GREEN}"
                else
                    coverage_black_color="{CYAN}"
                fi
            else
                coverage_black_color="{CYAN}"
            fi
            
            if [[ "${coverage_combined_fmt}" != "N/A" ]]; then
                local coverage_combined_check
                coverage_combined_check=$(echo "${coverage_combined_fmt} >= 80" | bc -l 2>/dev/null || echo 0)
                if (( coverage_combined_check )); then
                    coverage_combined_color="{GREEN}"
                else
                    coverage_combined_color="{CYAN}"
                fi
            else
                coverage_combined_color="{CYAN}"
            fi
            
            if [[ "${unity_ratio}" != "N/A" ]]; then
                local unity_numeric unity_check1 unity_check2
                unity_numeric=${unity_ratio%'%'}
                unity_check1=$(echo "${unity_numeric} >= 100" | bc -l 2>/dev/null || echo 0)
                unity_check2=$(echo "${unity_numeric} <= 200" | bc -l 2>/dev/null || echo 0)
                if (( unity_check1 )) && (( unity_check2 )); then
                    unity_color="{GREEN}"
                else
                    unity_color="{CYAN}"
                fi
            else
                unity_color="{CYAN}"
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
         "metric": "Test",
         "value": "${total_test_summary}",
         "description": "Test C/Headers code"
     },
     {
         "section": "code_metrics",
         "metric": "Code",
         "value": "${total_code_summary}",
         "description": "Core C/Headers + Bash + CMake + Lua + JS code"
     },
     {
         "section": "code_metrics",
         "metric": "Comments",
         "value": "${total_comments_summary}",
         "description": "Core C/Headers + Bash + CMake + Lua + JS comments"
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
         "description": "Ratio of Code to Docs       ${codedoc_color}Target: 1.500 - 2.500{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Docs/Code",
         "value": "${docscode_ratio}",
         "description": "Ratio of Docs to Code       ${docscode_color}Target: 0.400 - 0.667{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Code/Comments",
         "value": "${codecomment_ratio}",
         "description": "Ratio of Code to Comments   ${codecomment_color}Target: 3.000 - 5.000{RESET}"
     },
     {
         "section": "ratios",
         "metric": "Comments/Code",
         "value": "${commentscode_ratio}",
         "description": "Ratio of Comments to Code   ${commentscode_color}Target: 0.200 - 0.333{RESET}"
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
         "metric": "Instrumented Tests",
         "value": "${format_instrumented_tests}",
         "description": "Lines of instrumented code - Test articles"
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
         "section": "coverage_lines",
         "metric": "Coverage Combined",
         "value": "${format_covered_combined}",
         "description": "Lines of covered code - Combined"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Unity %",
         "value": "${coverage_unity_fmt} %",
         "description": "Unity test coverage                  ${coverage_unity_color}Target: 70 %{RESET}"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Blackbox %",
         "value": "${coverage_black_fmt} %",
         "description": "Blackbox test coverage               ${coverage_black_color}Target: 75 %{RESET}"
     },
     {
         "section": "coverage_percentages",
         "metric": "Coverage Combined %",
         "value": "${coverage_combined_fmt} %",
         "description": "Combined test coverage               ${coverage_combined_color}Target: 80 %{RESET}"
     },
     {
         "section": "coverage_percentages",
         "metric": "Unity Ratio",
         "value": "${unity_ratio}",
         "description": "Test/Core C/Headers         ${unity_color}Target: 100 % - 200 %{RESET}"
     },
     {
         "section": "file_sizes",
         "metric": "Hydrogen Binary",
         "value": "${release_size} KB",
         "description": "Hydrogen Release Executable with Payload"
     },
     {
         "section": "file_sizes",
         "metric": "Hydrogen Naked",
         "value": "${naked_size} KB",
         "description": "Hydrogen Release Executable without Payload"
     },
     {
         "section": "file_sizes",
         "metric": "Hydrogen Payload",
         "value": "${payload_size} KB",
         "description": "Payload with Swagger, Migrations, Terminal, OIDC"
     },
     {
         "section": "file_sizes",
         "metric": "Hydrogen Installer",
         "value": "${installer_size} KB",
         "description": "Base64-Encoded Bash Installer Script"
     },
     {
         "section": "file_sizes",
         "metric": "Hydrogen Repository",
         "value": "${repo_size} MB",
         "description": "Repository -Build -Unity Framework"
     }
]
EOF

            # Generate second table to separate file
            "${TABLES}" "${stats_layout_json}" "${stats_data_json}" > "${stats_output}"

            # Save stats JSON data to results directory for capture by test_00_all.sh
            # shellcheck disable=SC2154 # RESULTS_DIR defined externally in framework.sh
            if [[ -n "${RESULTS_DIR:-}" ]]; then
                cp "${stats_data_json}" "${RESULTS_DIR}/cloc_stats_data.json"
            fi

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

