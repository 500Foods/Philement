#!/usr/bin/env bash

# Test: Code Size Analysis
# Performs code size analysis and runs cloc ()

# FUNCTIONS
# show_top_files_by_type()

# CHANGELOG
# 3.5.0 - 2025-08-10 - Cleaned out some mktemp calls
# 3.4.0 - 2025-08-07 - Added (cloc: lines) to test name 
# 3.3.0 - 2025-08-06 - Bit of temp file handling management and cleanup
# 3.2.0 - 2025-08-05 - Minor adjustments to output formatting, added TOPLIST variable
# 3.1.0 - 2025-08-04 - Better presentation of large file sizes
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.0 - 2025-07-18 - Performance optimization: 46% speed improvement (3.136s to 1.705s), background cloc analysis, batch wc -l processing, parallel file type counting, largest-to-smallest file sorting
# 1.0.0 - Initial version

set -euo pipefail

# Test configuration
TEST_NAME="Code Size Analysis"
TEST_ABBR="SIZ"
TEST_NUMBER="80"
TEST_COUNTER=0
TEST_VERSION="3.5.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration constants
readonly MAX_SOURCE_LINES=1000
readonly LARGE_FILE_THRESHOLD="25k"

# Top files
TOPLIST=10

# Create temporary files
SOURCE_FILES_LIST="${LOG_PREFIX}${TIMESTAMP}_source_files.txt"
LARGE_FILES_LIST="${LOG_PREFIX}${TIMESTAMP}_large_files.txt"
LINE_COUNT_FILE="${LOG_PREFIX}${TIMESTAMP}_line_count.txt"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Linting Configuration Information"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking linting configuration files and displaying exclusion patterns..."

exclusion_files=(".lintignore" ".lintignore-c" ".lintignore-markdown" ".lintignore-bash")
found_files=0
missing_files=()

for file in "${exclusion_files[@]}"; do
    if [[ -f "${file}" ]]; then
        found_files=$(( found_files + 1 ))
        if [[ "${file}" = ".lintignore-bash" ]]; then
            # Special handling for .lintignore-bash which doesn't have a SUMMARY section
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${file}: Used by shellcheck for bash script linting exclusions"
        else
            summary=$("${GREP}" -A "${TOPLIST}" "SUMMARY" "${file}" 2>/dev/null | "${GREP}" -v "SUMMARY" | "${GREP}" -v "Used by" | "${SED}" 's/^# /  /' | "${SED}" 's/#$//' || true)
            if [[ -n "${summary}" ]]; then
                # Break multi-line summaries into individual print_message calls
                # Use process substitution to avoid subshell
                while IFS= read -r line; do
                    if [[ -n "${line}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${file}: ${line}"
                    fi
                done < <(echo "${summary}")
            fi
        fi
    else
        missing_files+=("${file}")
    fi
done

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "For details, see tests/TESTING.md and .lintignore files"

if [[ ${#missing_files[@]} -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${found_files} linting configuration files found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing linting files: ${missing_files[*]}"
    EXIT_CODE=1
fi

# Start cloc analysis in background to run in parallel with file processing
CLOC_OUTPUT="${LOG_PREFIX}${TIMESTAMP}_cloc.txt"
CLOC_DATA="${LOG_PREFIX}${TIMESTAMP}_cloc.json"
CLOC_PID=""
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting cloc analysis in background..."
run_cloc_analysis "." ".lintignore" "${CLOC_OUTPUT}" "${CLOC_DATA}" &
CLOC_PID=$!

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Source Code File Analysis"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing source code files and generating statistics..."

# Find all source files, excluding those in .lintignore
true > "${SOURCE_FILES_LIST}"
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        echo "${rel_file}" >> "${SOURCE_FILES_LIST}"
    fi
done < <("${FIND}" . -type f \( -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) | sort || true)

TOTAL_FILES=$(wc -l < "${SOURCE_FILES_LIST}")

# Initialize line count bins
declare -A line_bins=(
    ["000-099"]=0 ["100-199"]=0 ["200-299"]=0 ["300-399"]=0 ["400-499"]=0
    ["500-599"]=0 ["600-699"]=0 ["700-799"]=0 ["800-899"]=0 ["900-999"]=0 ["1000+"]=0
)

LARGEST_FILE=""
LARGEST_LINES=0
HAS_OVERSIZED=0

# Analyze each file using efficient batch processing
if [[ -s "${SOURCE_FILES_LIST}" ]]; then
    # Use wc -l on all files at once for maximum efficiency, then process the output
    "${XARGS}" wc -l < "${SOURCE_FILES_LIST}" > "${LINE_COUNT_FILE}.tmp"
    
    # Process the results to categorize by line count
    while read -r lines file; do
        # Handle the total line at the end
        [[ "${file}" = "total" ]] && continue
        
        "${PRINTF}" "%5d %s\n" "${lines}" "${file}"
        
        # Categorize by line count
        if   [[ "${lines}" -lt 100  ]]; then line_bins["000-099"]=$((line_bins["000-099"] + 1))
        elif [[ "${lines}" -lt 200  ]]; then line_bins["100-199"]=$((line_bins["100-199"] + 1))
        elif [[ "${lines}" -lt 300  ]]; then line_bins["200-299"]=$((line_bins["200-299"] + 1))
        elif [[ "${lines}" -lt 400  ]]; then line_bins["300-399"]=$((line_bins["300-399"] + 1))
        elif [[ "${lines}" -lt 500  ]]; then line_bins["400-499"]=$((line_bins["400-499"] + 1))
        elif [[ "${lines}" -lt 600  ]]; then line_bins["500-599"]=$((line_bins["500-599"] + 1))
        elif [[ "${lines}" -lt 700  ]]; then line_bins["600-699"]=$((line_bins["600-699"] + 1))
        elif [[ "${lines}" -lt 800  ]]; then line_bins["700-799"]=$((line_bins["700-799"] + 1))
        elif [[ "${lines}" -lt 900  ]]; then line_bins["800-899"]=$((line_bins["800-899"] + 1))
        elif [[ "${lines}" -lt 1000 ]]; then line_bins["900-999"]=$((line_bins["900-999"] + 1))
        else
            line_bins["1000+"]=$((line_bins["1000+"] + 1))
            HAS_OVERSIZED=1
            if [[ "${lines}" -gt "${LARGEST_LINES}" ]]; then
                LARGEST_FILE="${file}"
                LARGEST_LINES=${lines}
            fi
        fi
    done < "${LINE_COUNT_FILE}.tmp" > "${LINE_COUNT_FILE}"
fi

# Sort line count file
sort -nr -o "${LINE_COUNT_FILE}" "${LINE_COUNT_FILE}"

# Display distribution
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Source Code Distribution (${TOTAL_FILES} files):"
for range in "000-099" "100-199" "200-299" "300-399" "400-499" "500-599" "600-699" "700-799" "800-899" "900-999" "1,000+ "; do
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${range} Lines: $("${PRINTF}" "%5d" "${line_bins[${range}]:-0}" || true) files"
done

# Show top files by type
show_top_files_by_type() {
    local types=("md" "c" "h" "sh")
    local labels=("Markdown" "C Source" "Header" "Shell Script")
    
    for i in "${!types[@]}"; do
        local ext="${types[${i}]}"
        local label="${labels[${i}]}"
        local temp_file
        temp_file="${LOG_PREFIX}${TIMESTAMP}_top_${ext}.txt"
        
        "${GREP}" "\.${ext}$" "${LINE_COUNT_FILE}" > "${temp_file}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Top ${TOPLIST} ${label} Files:"
        # Avoid subshell by using process substitution instead of pipe
        while read -r line; do
            local count path
            read -r count path <<< "${line}"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${count} lines: ${path}"
        done < <(head -n "${TOPLIST}" "${temp_file}" || true)
    done
}

show_top_files_by_type

# Check size compliance
if [[ "${HAS_OVERSIZED}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found files exceeding ${MAX_SOURCE_LINES} lines"
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Largest file: ${LARGEST_FILE} with ${LARGEST_LINES} lines"
    EXIT_CODE=1
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No files exceed ${MAX_SOURCE_LINES} lines"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Large Non-Source File Detection"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Finding large non-source files (>${LARGE_FILE_THRESHOLD})..."

true > "${LARGE_FILES_LIST}"
while read -r size file; do
    # Strip ./ from find's output to match realpath's format
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        echo "${size} ${rel_file}" >> "${LARGE_FILES_LIST}"
    fi
done < <("${FIND}" . -type f -size "+${LARGE_FILE_THRESHOLD}" -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) -printf "%k %p\n" || true)

LARGE_FILE_COUNT=$(wc -l < "${LARGE_FILES_LIST}")

if [[ "${LARGE_FILE_COUNT}" -eq 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No large files found (excluding source/docs)"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No large files found"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${LARGE_FILE_COUNT} large files:"
    if [[ -s "${LARGE_FILES_LIST}" ]]; then
        sorted_list="${LOG_PREFIX}${TIMESTAMP}_large_files_sorted.txt"
        sort -nr "${LARGE_FILES_LIST}" > "${sorted_list}"
        while read -r size file; do
            size_formatted=$("${PRINTF}" "%'7d" "${size}" || true)
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${size_formatted} KB: ${file}"
        done < "${sorted_list}"
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Found ${LARGE_FILE_COUNT} files >${LARGE_FILE_THRESHOLD}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Code Line Count Analysis (cloc)"

if [[ -n "${CLOC_PID}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for cloc analysis to complete..."
    wait "${CLOC_PID}"
    CLOC_EXIT_CODE=$?

    if [[ "${CLOC_EXIT_CODE}" -eq 0 ]] && [[ -s "${CLOC_OUTPUT}" ]] && [[ -s "${CLOC_DATA}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Code line count analysis results:"
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < "${CLOC_OUTPUT}"

        # Also display the extended statistics table
        cloc_stats="${CLOC_OUTPUT%.txt}_stats.txt"
        if [[ -f "${cloc_stats}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Extended statistics:"
            while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
            done < "${cloc_stats}"
        fi

        # Generate SVGs from the table files using Oh.sh
        # shellcheck disable=SC2154 # OH defined in framework.sh
        if [[ -f "${CLOC_OUTPUT}" ]]; then
            ("${OH}" -i "${CLOC_OUTPUT}" --width 74 -o "${PROJECT_DIR}/images/CLOC_CODE.svg" 2>/dev/null) &
        fi

        # Check for stats table file
        cloc_stats="${CLOC_OUTPUT%.txt}_stats.txt"
        if [[ -f "${cloc_stats}" ]]; then
            ("${OH}" -i "${cloc_stats}" -o "${PROJECT_DIR}/images/CLOC_STAT.svg" 2>/dev/null) &
        fi

        # Extract summary statistics from JSON data
        STATS=$(extract_cloc_stats "${CLOC_DATA}")
        if [[ -n "${STATS}" ]]; then
            IFS=',' read -r files_part lines_part <<< "${STATS}"
            FILES_COUNT=$(echo "${files_part}" | cut -d':' -f2)
            CODE_LINES=$(echo "${lines_part}" | cut -d':' -f2)
            LOCCOUNT=$("${PRINTF}" "%'d" "${CODE_LINES}")
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Found $("${PRINTF}" "%'d" "${FILES_COUNT}" || true) files with ${LOCCOUNT} lines of code"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to parse cloc data"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "cloc analysis failed"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "cloc not available (skipped)"
fi

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analysis files saved to results directory:"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Line counts: ..${SOURCE_FILES_LIST}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Large files: ..${LARGE_FILES_LIST}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Line counts: ..${LINE_COUNT_FILE}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "cloc format: ..${CLOC_OUTPUT}"
print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "cloc output: ..${CLOC_DATA}"

TEST_NAME="${TEST_NAME} {BLUE}(lines: ${LOCCOUNT}){RESET}"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
