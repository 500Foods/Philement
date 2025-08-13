#!/bin/bash

# Log Output Library
# Provides consistent logging, formatting, and display functions for test scripts

# LIBRARY FUNCTIONS
# process_message()
# enable_output_collection()
# disable_output_collection()
# dump_collected_output()
# clear_collected_output(}
# print_test_suite_header()
# print_test_header()
# print_subtest()
# print_command()
# print_output()
# print_result()
# print_warning()
# print_error()
# print_message()
# print_test_completion()

# CHANGELOG
# 3.7.0 - 2025-08-10 - Simplifid some log output naming, cleared out mktemp calls
# 3.6.0 - 2025-08-07 - Support for commas in test names (ie, thousands separators)
# 3.5.0 - 2025-08-06 - Improvements to logging file handling, common TAB file naming
# 3.4.0 - 2025-08-02 - Removed unused functions, moved test functions to framework.sh
# 3.3.1 - 2025-07-31 - Fixed issue where not all collected output was output/cleared at end of test
# 3.3.0 - 2025-07-20 - Updated guard clause to prevent multiple sourcing
# 3.2.1 - 2025-07-18 - Fixed hanging issue in output collection mechanism when running through orchestrator
# 3.2.0 - 2025-07-18 - Modified output collection to dump cache when new TEST starts for progressive feedback
# 3.1.0 - 2025-07-07 - Restructured how test elapsed times are stored and accessed
# 3.0.4 - 2025-07-06 - Added mechanism to handle absolute paths in log output
# 3.0.3 - 2025-07-03 - Applied color consistency to all output types (DATA, EXEC, PASS, FAIL)
# 3.0.2 - 2025-07-03 - Completely removed legacy functions (print_header, print_info)
# 3.0.1 - 2025-07-03 - Enhanced documentation, removed unused functions, improved comments
# 3.0.0 - 2025-07-03 - General overhaul of colors and icons, removed legacy functions, etc.
# 2.1.0 - 2025-07-02 - Added DATA_ICON and updated DATA_COLOR to pale yellow (256-color palette)
# 2.0.0 - 2025-07-02 - Complete rewrite with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Guard clause to prevent multiple sourcing
[[ -n "${LOG_OUTPUT_GUARD}" ]] && return 0
export LOG_OUTPUT_GUARD="true"

# Library metadata
LOG_OUTPUT_NAME="Log Output Library"
LOG_OUTPUT_VERSION="3.7.0"
export LOG_OUTPUT_NAME LOG_OUTPUT_VERSION

# Global variables for test/subtest numbering
CURRENT_TEST_NUMBER=""
CURRENT_SUBTEST_NUMBER=""
SUBTEST_COUNTER=0
TEST_START_TIME=""
TEST_PASSED_COUNT=0
TEST_FAILED_COUNT=0
declare -a TEST_ELAPSED_TIMES
export SUBTEST_COUNTER TEST_START_TIME

# Icon-specific colors for consistent theming
PASS_COLOR='\033[0;32m'     # Green
FAIL_COLOR='\033[0;31m'     # Red
WARN_COLOR='\033[0;33m'     # Yellow
INFO_COLOR='\033[0;36m'     # Cyan
DATA_COLOR='\033[38;5;229m' # Pale, soft yellow from 256-color palette
EXEC_COLOR='\033[0;33m'     # Yellow
TEST_COLOR='\033[0;34m'     # Blue

# Other formatting codes
if [[ -z "${BOLD:-}" ]]; then
    BOLD='\033[1m'
    NC='\033[0m' # No Color
fi

# Rounded rectangle icons with colors matching label themes (double-wide)
PASS_ICON="${PASS_COLOR}\U2587\U2587${NC}"
FAIL_ICON="${FAIL_COLOR}\U2587\U2587${NC}"
WARN_ICON="${WARN_COLOR}\U2587\U2587${NC}"
INFO_ICON="${INFO_COLOR}\U2587\U2587${NC}"
DATA_ICON="${DATA_COLOR}\U2587\U2587${NC}"
EXEC_ICON="${EXEC_COLOR}\U2587\U2587${NC}"
TEST_ICON="${TEST_COLOR}\U2587\U2587${NC}"

# Function to process a message and replace full paths with relative paths
process_message() {
  local message="$1"
  local processed="${message}"
  processed="${message//${PROJECT_DIR}/}"
  echo "${processed}"
}

# Function to enable output collection mode
enable_output_collection() {
    COLLECT_OUTPUT_MODE=true
    OUTPUT_COLLECTION=()
}

# Function to disable output collection mode
disable_output_collection() {
    COLLECT_OUTPUT_MODE=false
}

# Function to dump collected output in a single printf call
dump_collected_output() {
    # Defensive check to ensure array is properly initialized
    if [[ ! -v OUTPUT_COLLECTION ]]; then
        return 0
    fi
    
    # Check if array has elements
    if [[ "${#OUTPUT_COLLECTION[@]}" -gt 0 ]]; then
        # Use printf instead of echo -e to avoid potential hanging issues
        # Process array elements safely with proper quoting and limits
        local line
        local count=0
        local max_lines=1000  # Prevent infinite loops with large arrays
        
        for line in "${OUTPUT_COLLECTION[@]}"; do
            # Safety check for runaway loops
            if [[ ${count} -ge ${max_lines} ]]; then
                printf '%b\n' "... (output truncated after ${max_lines} lines)"
                break
            fi
            
            # Only output non-empty lines
            if [[ -n "${line}" ]]; then
                printf '%b\n' "${line}" 2>/dev/null || true
            fi
            ((count++))
        done
    fi
}

# Function to clear collected output
clear_collected_output() {
    # Safely clear the array with defensive checks
    if [[ -v OUTPUT_COLLECTION ]]; then
        OUTPUT_COLLECTION=()
    else
        # Initialize array if it doesn't exist
        declare -ga OUTPUT_COLLECTION=()
    fi
}

# Function to print beautiful test suite runner header using tables.sh with blue theme
print_test_suite_header() {
    local test_name="$1"
    local test_abbr="$2"
    local test_number="$3"
    local test_version="$4"
    
    # Start the test timer
    start_test_timer
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(/usr/bin/date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create the test header content
    local test_id="${test_number}-${test_abbr}"
    
    # Fall back to external process for backward compatibility
    # shellcheck disable=SC2154,SC2153 # RESULTS_DIR, TEST_NUMBER defined in caller (Test 00)
    local layout_json="${RESULTS_DIR}/suite_header_${TIMESTAMP}_layout.json"
    local data_json="${RESULTS_DIR}/suite_header_${TIMESTAMP}_data.json"
    
    # Create layout JSON with proper columns and blue theme
    cat > "${layout_json}" << EOF
{
    "theme": "Blue",
    "columns": [
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text"
        },
        {
            "header": "Test Title",
            "key": "test_name",
            "datatype": "text",
            "width": 53
        },
        {
            "header": "Version",
            "key": "test_version",
            "datatype": "text",
            "width": 9
        },
        {
            "header": "Started",
            "key": "timestamp",
            "datatype": "text",
            "justification": "right"
        }
    ]
}
EOF

    # Create data JSON with the test information
    cat > "${data_json}" << EOF
[
    {
        "test_id": "${test_id}",
        "test_name": "${test_name}",
        "test_version": "${test_version}",
        "timestamp": "${timestamp}"
    }
]
EOF
    
    # Use tables executable to render the header
    # shellcheck disable=SC2154 # TABLES_EXTERNAL defined in framework.sh
    "${TABLES_EXTERNAL}" "${layout_json}" "${data_json}" 2>/dev/null
   
}

# Function to print beautiful test header using tables.sh
print_test_header() {
    local test_name="$1"
    local test_abbr="$2"
    local test_number="$3"
    local test_version="$4"
    local test_id
    test_id=$(printf "%s-%s" "${test_number}" "${test_abbr}")

    # Start the test timer
    start_test_timer
    
    # Automatically enable output collection for performance optimization
    enable_output_collection
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(/usr/bin/date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create layout JSON with proper columns
    # shellcheck disable=SC2154 # Defined in framework.sh
    cat > "${TAB_LAYOUT_HEADER}" << EOF
{
    "columns": [
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text"
        },
        {
            "header": "Test Title",
            "key": "test_name",
            "datatype": "text",
            "width": 53
        },
        {
            "header": "Version",
            "key": "test_version",
            "datatype": "text",
            "width": 9
        },
        {
            "header": "Started",
            "key": "timestamp",
            "datatype": "text",
            "justification": "right"
        }
    ]
}
EOF

    # Create data JSON with the test information
    # shellcheck disable=SC2154 # Defined in framework.sh    
    cat > "${TAB_DATA_HEADER}" << EOF
[
    {
        "test_id": "${test_id}",
        "test_name": "${test_name}",
        "test_version": "${test_version}",
        "timestamp": "${timestamp}"
    }
]
EOF
    
    # Use tables executable to render the header
    # shellcheck disable=SC2154 # TABLES_EXTERNAL Defined externally in framework.sh
    "${TABLES_EXTERNAL}" "${TAB_LAYOUT_HEADER}" "${TAB_DATA_HEADER}" 2>/dev/null
}

# Function to print subtest headers
print_subtest() {
    local subtest_name="$1"
    local elapsed
    elapsed=$(get_elapsed_time)
    local processed_name
    processed_name=$(process_message "${subtest_name}")
    local formatted_output="  ${TEST_COLOR}${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}   ${elapsed}   ${NC}${TEST_ICON}${TEST_COLOR} TEST   ${processed_name}${NC}"

    ((SUBTEST_COUNTER++))
    CURRENT_SUBTEST_NUMBER=$(printf "%03d" "${SUBTEST_COUNTER}")
    
    # If we're in collection mode and have cached output, dump it before starting new test
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        # Defensive check for array state to prevent hanging
        if [[ -n "${OUTPUT_COLLECTION+set}" ]] && [[ "${#OUTPUT_COLLECTION[@]}" -gt 0 ]]; then
            # Try to dump the cached output, but don't fail if it causes issues
            dump_collected_output 2>/dev/null || true
        fi
        # Clear the cache for new test regardless of dump success
        clear_collected_output 2>/dev/null || true
    fi
    
    # Always output the TEST entry immediately so user sees what test is starting
    echo -e "${formatted_output}"
}

# Function to print commands that will be executed
print_command() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local cmd
    cmd=$(process_message "$1")
    # Truncate command to approximately 200 characters
    local truncated_cmd
    if [[ ${#cmd} -gt 200 ]]; then
        truncated_cmd="${cmd:0:197}..."
    else
        truncated_cmd="${cmd}"
    fi
    local formatted_output="  ${prefix}   ${elapsed}   ${EXEC_COLOR}${EXEC_ICON} ${EXEC_COLOR}EXEC${NC}   ${EXEC_COLOR}${truncated_cmd}${NC}"
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}

# Function to print command output
print_output() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local message
    message=$(process_message "$1")
    # Skip output if message is empty or contains only whitespace
    if [[ -n "${message}" && ! "${message}" =~ ^[[:space:]]*$ ]]; then
        local formatted_output="  ${prefix}   ${elapsed}   ${DATA_COLOR}${DATA_ICON} ${DATA_COLOR}DATA${NC}   ${DATA_COLOR}${message}${NC}"
        if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
            OUTPUT_COLLECTION+=("${formatted_output}")
        else
            echo -e "${formatted_output}"
        fi
    fi
}

# Function to print test results
print_result() {
    local status=$1
    local message=$2
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local processed_message
    processed_message=$(process_message "${message}")
    
    # Record the result for statistics
    record_test_result "${status}"
    
    local formatted_output
    if [[ "${status}" -eq 0 ]]; then
        formatted_output="  ${prefix}   ${elapsed}   ${PASS_COLOR}${PASS_ICON} ${PASS_COLOR}PASS${NC}   ${PASS_COLOR}${processed_message}${NC}"
    else
        formatted_output="  ${prefix}   ${elapsed}   ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}FAIL${NC}   ${FAIL_COLOR}${processed_message}${NC}"
    fi
    
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}

# Function to print warnings
print_warning() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local message
    message=$(process_message "$1")
    local formatted_output="  ${prefix}   ${elapsed}   ${WARN_COLOR}${WARN_ICON} ${WARN_COLOR}WARN${NC}   ${message}"
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}

# Function to print error messages
print_error() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local message
    message=$(process_message "$1")
    local formatted_output="  ${prefix}   ${elapsed}   ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}ERROR${NC}   ${message}"
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}

# Function to print informational messages
print_message() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local message
    message=$(process_message "$1")
    local formatted_output="  ${prefix}   ${elapsed}   ${INFO_COLOR}${INFO_ICON} ${INFO_COLOR}INFO${NC}   ${message}"
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}

# Function to print beautiful test completion table using tables.sh
print_test_completion() {
    local test_name="$1"
    local test_abbr="$2"
    local test_number="$3"
    local test_version="$4"

    # Calculate totals and elapsed time
    local total_subtests=$((TEST_PASSED_COUNT + TEST_FAILED_COUNT))
    local elapsed_time
    local processed_name
    processed_name=$(process_message "${test_name}")
    
    # Calculate elapsed time once and store it for consistent use - this is the SINGLE SOURCE OF TRUTH
    # Use decimal format for table output
    if [[ -z "${TEST_ELAPSED_TIMES[CURRENT_TEST_NUMBER]}" ]]; then
        elapsed_time=$(get_elapsed_time_decimal)
        TEST_ELAPSED_TIMES[CURRENT_TEST_NUMBER]="${elapsed_time}"
    else
        elapsed_time="${TEST_ELAPSED_TIMES[CURRENT_TEST_NUMBER]}"
    fi
    
    # Automatically dump collected output before final results table
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        dump_collected_output
        clear_collected_output
    fi
    
    # Write elapsed time to the subtest result file if running in test suite
    # Use the SAME elapsed_time value that was calculated above - no additional calls to get_elapsed_time
    if [[ -n "${ORCHESTRATION}" ]]; then
        # Use the elapsed_time that was already calculated above - SINGLE SOURCE OF TRUTH
        local file_elapsed_time="${elapsed_time}"
        # shellcheck disable=SC2154 # TS_ORC_LOG, is defined externally in framework.sh
        local subtest_file="${RESULTS_DIR}/test_${CURRENT_TEST_NUMBER}_${TIMESTAMP}_results.txt"
        test_name_adjusted=${test_name//,/\{COMMA\}}
        echo "${total_subtests},${TEST_PASSED_COUNT},${test_name_adjusted},${file_elapsed_time},${test_abbr},${test_version}" > "${subtest_file}" 2>/dev/null
    fi

    # Create layout JSON with proper columns
    # shellcheck disable=SC2154 # TAB_LAYOUT defined in framework.sh
    cat > "${TAB_LAYOUT_FOOTER}" << EOF
{
    "theme": "Blue",
    "columns": [
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text"
        },
        {
            "header": "Test Name",
            "key": "test_name",
            "datatype": "text",
            "width": 44    
        },
        {
            "header": "Version",
            "key": "test_version",
            "datatype": "text",
            "width": 9
        },
        {
            "header": "Tests",
            "key": "total_subtests",
            "datatype": "int",
            "justification": "right",
            "width": 7
        },
        {
            "header": "Pass",
            "key": "passed",
            "datatype": "int",
            "justification": "right",
            "width": 7
        },
        {
            "header": "Fail",
            "key": "failed",
            "datatype": "int",
            "justification": "right",
            "width": 7
        },
        {
            "header": "Duration",
            "key": "elapsed",
            "datatype": "float",
            "justification": "right",
            "width": 10
        }
    ]
}
EOF

    # Create data JSON with the test completion information, using the SAME elapsed time
    # that was calculated at the top of this function - SINGLE SOURCE OF TRUTH
    local display_elapsed_time="${elapsed_time}"
    # shellcheck disable=SC2154 # TAB_DATA defined in framework.sh
    cat > "${TAB_DATA_FOOTER}" << EOF
[
    {
        "test_id": "${test_number}-${test_abbr}",
        "test_name": "${processed_name}",
        "test_version": "${test_version}",
        "total_subtests": ${total_subtests},
        "passed": ${TEST_PASSED_COUNT},
        "failed": ${TEST_FAILED_COUNT},
        "elapsed": "${display_elapsed_time}"
    }
]
EOF
    
    # Use tables executable to render the completion table
    # shellcheck disable=SC2154 # LIB_DIR defined externally in framework.sh    
    "${TABLES_EXTERNAL}" "${TAB_LAYOUT_FOOTER}" "${TAB_DATA_FOOTER}" 2>/dev/null
    
    # Let's end up here so the next script doesn't have a fit
    popd >/dev/null 2>&1 || return

    # Let's kill any stragglers that didn't exit cleanly
    if [[ -z "${ORCHESTRATION}" ]]; then
        pkill -f hydrogen_test_    
    fi

}
