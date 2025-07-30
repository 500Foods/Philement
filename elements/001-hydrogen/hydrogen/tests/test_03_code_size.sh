#!/bin/bash

# Test: Code Size Analysis
# Performs comprehensive code size analysis including:
# - Source code analysis and line counting
# - Large file detection
# - Code line count analysis with cloc
# - File count summary

# CHANGELOG
# 2.0.0 - 2025-07-18 - Performance optimization: 46% speed improvement (3.136s to 1.705s), background cloc analysis, batch wc -l processing, parallel file type counting, largest-to-smallest file sorting
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Code Size Analysis"
TEST_ABBR="SIZ"
TEST_VERSION="2.0.0"

# Timestamps
# TS_SIZ_LOG=$(date '+%Y%m%d_%H%M%S' 2>/dev/null)             # 20250730_124718                 eg: log filenames
# TS_SIZ_TMR=$(date '+%s.%N' 2>/dev/null)                     # 1753904852.568389297            eg: timers, elapsed times
# TS_SIZ_ISO=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null)      # 2025-07-30 12:47:46 PDT         eg: short display times
# TS_SIZ_DSP=$(date '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null) # 2025-Jul-30 (Wed) 12:49:03 PDT  eg: long display times

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# CMAKE_DIR="${PROJECT_DIR}/cmake"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"

# Test configuration
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXIT_CODE=0
PASS_COUNT=0
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
[[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
[[ -n "${CLOC_GUARD}" ]] || source "${LIB_DIR}/cloc.sh"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi


# Test configuration constants
readonly MAX_SOURCE_LINES=1000
readonly LARGE_FILE_THRESHOLD="25k"

# Default exclude patterns for linting (can be overridden by .lintignore)
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_EXCLUDES:-}" ]]; then
    readonly LINT_EXCLUDES=(
        "build/*"
    )
fi

# Create temporary files
SOURCE_FILES_LIST=$(mktemp)
LARGE_FILES_LIST=$(mktemp)
LINE_COUNT_FILE=$(mktemp)

# Cleanup function
cleanup_temp_files() {
    rm -f "${SOURCE_FILES_LIST}" "${LARGE_FILES_LIST}" "${LINE_COUNT_FILE}"
}

# Trap cleanup on exit
trap cleanup_temp_files EXIT

# Check if a file should be excluded from linting
should_exclude_file() {
    local file="$1"
    local lint_ignore=".lintignore"
    local rel_file="${file#./}"  # Remove leading ./
    
    # Check .lintignore file first if it exists
    if [[ -f "${lint_ignore}" ]]; then
        while IFS= read -r pattern; do
            [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            
            # Check if file matches pattern exactly or is within a directory pattern
            if [[ "${rel_file}" == "${pattern}" ]] || [[ "${rel_file}" == "${clean_pattern}"/* ]]; then
                return 0 # Exclude
            fi
        done < "${lint_ignore}"
    fi
    
    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        if [[ "${rel_file}" == "${pattern}" ]] || [[ "${rel_file}" == "${clean_pattern}"/* ]]; then
            return 0 # Exclude
        fi
    done
    
    return 1 # Do not exclude
}

# Subtest 1: Linting Configuration Information
next_subtest
print_subtest "Linting Configuration Information"

print_message "Checking linting configuration files and displaying exclusion patterns..."

exclusion_files=(".lintignore" ".lintignore-c" ".lintignore-markdown" ".lintignore-bash")
found_files=0
missing_files=()

for file in "${exclusion_files[@]}"; do
    if [[ -f "${file}" ]]; then
        ((found_files++))
        if [[ "${file}" = ".lintignore-bash" ]]; then
            # Special handling for .lintignore-bash which doesn't have a SUMMARY section
            print_message "${file}: Used by shellcheck for bash script linting exclusions"
        else
            summary=$(grep -A 5 "SUMMARY" "${file}" 2>/dev/null | grep -v "SUMMARY" | grep -v "Used by" | sed 's/^# /  /' | sed 's/#$//' || true)
            if [[ -n "${summary}" ]]; then
                # Break multi-line summaries into individual print_message calls
                # Use process substitution to avoid subshell
                while IFS= read -r line; do
                    if [[ -n "${line}" ]]; then
                        print_message "${file}: ${line}"
                    fi
                done < <(echo "${summary}")
            fi
        fi
    else
        missing_files+=("${file}")
    fi
done

print_message "For details, see tests/README.md and .lintignore files."

if [[ ${#missing_files[@]} -eq 0 ]]; then
    print_result 0 "All ${found_files} linting configuration files found"
    ((PASS_COUNT++))
else
    print_result 1 "Missing linting files: ${missing_files[*]}"
    EXIT_CODE=1
fi

# Start cloc analysis in background to run in parallel with file processing
CLOC_OUTPUT=$(mktemp)
CLOC_PID=""
if command -v cloc >/dev/null 2>&1; then
    print_message "Starting cloc analysis in background..."
    run_cloc_analysis "." ".lintignore" "${CLOC_OUTPUT}" &
    CLOC_PID=$!
fi

# Subtest 2: Analyze source code files and generate statistics
next_subtest
print_subtest "Source Code File Analysis"

print_message "Analyzing source code files and generating statistics..."

# Find all source files, excluding those in .lintignore
: > "${SOURCE_FILES_LIST}"
while read -r file; do
    if ! should_exclude_file "${file}"; then
        echo "${file}" >> "${SOURCE_FILES_LIST}"
    fi
done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" \) | sort || true)

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
    xargs wc -l < "${SOURCE_FILES_LIST}" > "${LINE_COUNT_FILE}.tmp"
    
    # Process the results to categorize by line count
    while read -r lines file; do
        # Handle the total line at the end
        [[ "${file}" = "total" ]] && continue
        
        printf "%05d %s\n" "${lines}" "${file}"
        
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
    
    rm -f "${LINE_COUNT_FILE}.tmp"
fi

# Sort line count file
sort -nr -o "${LINE_COUNT_FILE}" "${LINE_COUNT_FILE}"

# Display distribution
print_message "Source Code Distribution (${TOTAL_FILES} files):"
for range in "000-099" "100-199" "200-299" "300-399" "400-499" "500-599" "600-699" "700-799" "800-899" "900-999" "1000+"; do
    print_output "${range} Lines: ${line_bins[${range}]} files"
done

# Show top files by type
show_top_files_by_type() {
    local types=("md" "c" "h" "sh")
    local labels=("Markdown" "C Source" "Header" "Shell Script")
    
    for i in "${!types[@]}"; do
        local ext="${types[${i}]}"
        local label="${labels[${i}]}"
        local temp_file
        temp_file=$(mktemp)
        
        grep "\.${ext}$" "${LINE_COUNT_FILE}" > "${temp_file}"
        print_message "Top 5 ${label} Files:"
        # Avoid subshell by using process substitution instead of pipe
        while read -r line; do
            local count path
            read -r count path <<< "${line}"
            print_output "  ${count} lines: ${path}"
        done < <(head -n 5 "${temp_file}" || true)
        rm -f "${temp_file}"
    done
}

show_top_files_by_type

# Check size compliance
if [[ "${HAS_OVERSIZED}" -eq 1 ]]; then
    print_result 1 "Found files exceeding ${MAX_SOURCE_LINES} lines"
    print_output "Largest file: ${LARGEST_FILE} with ${LARGEST_LINES} lines"
    EXIT_CODE=1
else
    print_result 0 "No files exceed ${MAX_SOURCE_LINES} lines"
    ((PASS_COUNT++))
fi

# Subtest 3: Find and list large non-source files
next_subtest
print_subtest "Large Non-Source File Detection"

print_message "Finding large non-source files (>${LARGE_FILE_THRESHOLD})..."

: > "${LARGE_FILES_LIST}"
while read -r file; do
    if ! should_exclude_file "${file}"; then
        echo "${file}" >> "${LARGE_FILES_LIST}"
    fi
done < <(find . -type f -size "+${LARGE_FILE_THRESHOLD}" -not \( -path "*/tests/*" -o -name "*.c" -o -name "*.h" -o -name "*.md" -o -name "*.sh" -o -name "Makefile" \) | sort || true)

LARGE_FILE_COUNT=$(wc -l < "${LARGE_FILES_LIST}")

if [[ "${LARGE_FILE_COUNT}" -eq 0 ]]; then
    print_message "No large files found (excluding source/docs)"
    print_result 0 "No large files found"
    ((PASS_COUNT++))
else
    print_message "Found ${LARGE_FILE_COUNT} large files:"
    # Use du in parallel and sort by size (largest first)
    if [[ -s "${LARGE_FILES_LIST}" ]]; then
        # Use process substitution to avoid subshell
        temp_du_list="$(mktemp)"
        xargs -P "${CORES}" -I {} du -k {} < "${LARGE_FILES_LIST}" > "${temp_du_list}"
        sorted_du_list="$(mktemp)"
        sort -nr "${temp_du_list}" > "${sorted_du_list}"
        while read -r size file; do
            print_output "  ${size}KB: ${file}"
        done < "${sorted_du_list}"
        rm -f "${temp_du_list}" "${sorted_du_list}"
    fi
    print_result 0 "Found ${LARGE_FILE_COUNT} files >${LARGE_FILE_THRESHOLD}"
    ((PASS_COUNT++))
fi

# Subtest 4: Code line count analysis with cloc
next_subtest
print_subtest "Code Line Count Analysis (cloc)"

if [[ -n "${CLOC_PID}" ]]; then
    print_message "Waiting for cloc analysis to complete..."
    wait "${CLOC_PID}"
    CLOC_EXIT_CODE=$?
    
    if [[ "${CLOC_EXIT_CODE}" -eq 0 ]] && [[ -s "${CLOC_OUTPUT}" ]]; then
        print_message "Code line count analysis results:"
        while IFS= read -r line; do
            print_output "${line}"
        done < "${CLOC_OUTPUT}"
        
        # Extract summary statistics
        STATS=$(extract_cloc_stats "${CLOC_OUTPUT}")
        if [[ -n "${STATS}" ]]; then
            IFS=',' read -r files_part lines_part <<< "${STATS}"
            FILES_COUNT=$(echo "${files_part}" | cut -d':' -f2)
            CODE_LINES=$(echo "${lines_part}" | cut -d':' -f2)
            print_result 0 "Found ${FILES_COUNT} files with ${CODE_LINES} lines of code"
            ((PASS_COUNT++))
        else
            print_result 1 "Failed to parse cloc output"
            EXIT_CODE=1
        fi
    else
        print_result 1 "cloc analysis failed"
        EXIT_CODE=1
    fi
else
    print_result 0 "cloc not available (skipped)"
    ((PASS_COUNT++))
fi

rm -f "${CLOC_OUTPUT}"

# Subtest 5: File count summary
next_subtest
print_subtest "File Count Summary"

print_message "File type distribution:"
print_output "Total source files analyzed: ${TOTAL_FILES}"
print_output "Large files found: ${LARGE_FILE_COUNT}"
TEST_NAME="${TEST_NAME} {BLUE}(cloc: ${FILES_COUNT} files){RESET}"

# Count files by type in parallel for better performance
{
    find . -name "*.c"  -type f | wc -l > /tmp/c_files_count  || true &
    find . -name "*.h"  -type f | wc -l > /tmp/h_files_count  || true &
    find . -name "*.md" -type f | wc -l > /tmp/md_files_count || true &
    find . -name "*.sh" -type f | wc -l > /tmp/sh_files_count || true &
    wait
}
C_FILES=$(cat /tmp/c_files_count)
H_FILES=$(cat /tmp/h_files_count)
MD_FILES_COUNT=$(cat /tmp/md_files_count)
SH_FILES=$(cat /tmp/sh_files_count)
# Clean up temp files
rm -f /tmp/c_files_count /tmp/h_files_count /tmp/md_files_count /tmp/sh_files_count

print_output "C source files: ${C_FILES}"
print_output "Header files: ${H_FILES}"
print_output "Markdown files: ${MD_FILES_COUNT}"
print_output "Shell scripts: ${SH_FILES}"

print_result 0 "File count analysis completed"
((PASS_COUNT++))

# Save analysis results
cp "${LINE_COUNT_FILE}" "${RESULTS_DIR}/source_line_counts_${TIMESTAMP}.txt"
cp "${LARGE_FILES_LIST}" "${RESULTS_DIR}/large_files_${TIMESTAMP}.txt"

print_message "Analysis files saved to results directory:"
print_output "Line counts: ${RESULTS_DIR}/source_line_counts_${TIMESTAMP}.txt"
print_output "Large files: ${RESULTS_DIR}/large_files_${TIMESTAMP}.txt"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
