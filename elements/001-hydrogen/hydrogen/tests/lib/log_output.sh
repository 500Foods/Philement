#!/bin/bash

# Log Output Library
# Provides consistent logging, formatting, and display functions for test scripts

# LIBRARY FUNCTIONS
# set_absolute_root()
# process_message()
# set_test_number()
# set_subtest_number()
# extract_test_number()
# next_subtest()
# reset_subtest_counter()
# get_test_prefix()
# start_test_timer()
# record_test_result()
# get_elapsed_time()
# get_elapsed_time_decimal()
# format_file_size()
# enable_output_collection()
# disable_output_collection()
# dump_collected_output()
# clear_collected_output()
# print_test_header()
# print_subtest()
# print_command()
# print_output()
# print_result()
# print_warning()
# print_error()
# print_message()
# print_test_suite_header()
# print_test_completion()
# print_test_item()

# CHANGELOG
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
LOG_OUTPUT_VERSION="3.2.1"
export LOG_OUTPUT_NAME LOG_OUTPUT_VERSION
# print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"

# Global variables for test/subtest numbering
CURRENT_TEST_NUMBER=""
CURRENT_SUBTEST_NUMBER=""
SUBTEST_COUNTER=0

# Global variables for timing and statistics
TEST_START_TIME=""
TEST_PASSED_COUNT=0
TEST_FAILED_COUNT=0
declare -a TEST_ELAPSED_TIMES

# Variable for absolute path replacement
ABSOLUTE_ROOT=""

# Array for collecting output messages (for performance optimization and progressive feedback)
# Output is cached and dumped each time a new TEST starts, providing progressive feedback
declare -a OUTPUT_COLLECTION=()
COLLECT_OUTPUT_MODE=false

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

#==============================================================================
# PATH HANDLING FUNCTIONS
#==============================================================================

# Function to set the absolute root to the project root (two levels up from the script path)
# Usage: set_absolute_root <script_path>
set_absolute_root() {
  local script_path="$1"
  if [[ -n "${script_path}" && "${script_path}" == /* ]]; then
    local script_dir
    script_dir="$(dirname "${script_path}")"
    local project_root
    project_root="$(dirname "$(dirname "${script_dir}")")/"
    ABSOLUTE_ROOT="${project_root}"
  else
    ABSOLUTE_ROOT=""
  fi
  export ABSOLUTE_ROOT
}

# Function to process a message and replace full paths with relative paths
# Usage: process_message <message>
process_message() {
  local message="$1"
  local processed="${message}"
  if [[ -n "${ABSOLUTE_ROOT}" ]]; then
    processed="${message//${ABSOLUTE_ROOT}/}"
  fi
  echo "${processed}"
}

# Set the absolute root based on the current script's directory (assuming this is sourced from a test script)
# This ensures ABSOLUTE_ROOT is set early to the project root (two levels up from tests/lib)
if [[ "${BASH_SOURCE[0]}" == /* ]]; then
  set_absolute_root "${BASH_SOURCE[0]}"
fi

#==============================================================================
# TEST NUMBERING AND TIMING FUNCTIONS
#==============================================================================

# Function to set the current test number (e.g., "10" for test_10_compilation.sh)
set_test_number() {
    CURRENT_TEST_NUMBER="$1"
    CURRENT_SUBTEST_NUMBER=""
}

# Function to set the current subtest number (e.g., "001", "002", etc.)
set_subtest_number() {
    CURRENT_SUBTEST_NUMBER="$1"
}

# Function to extract test number from script filename
extract_test_number() {
    local script_path="$1"
    local filename
    filename=$(basename "${script_path}")
    
    # Extract number from test_XX_name.sh format
    if [[ "${filename}" =~ test_([0-9]+)_ ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo "XX"
    fi
}

# Function to increment and get next subtest number
next_subtest() {
    ((SUBTEST_COUNTER++))
    CURRENT_SUBTEST_NUMBER=$(printf "%03d" "${SUBTEST_COUNTER}")
}

# Function to reset subtest counter
reset_subtest_counter() {
    SUBTEST_COUNTER=0
    CURRENT_SUBTEST_NUMBER="000"
}

# Function to get the current test prefix for output
get_test_prefix() {
    if [[ -n "${CURRENT_SUBTEST_NUMBER}" ]]; then
        echo "${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}"
    elif [[ -n "${CURRENT_TEST_NUMBER}" ]]; then
        echo "${CURRENT_TEST_NUMBER}"
    else
        echo "XX"
    fi
}

# Function to start test timing
start_test_timer() {
    TEST_START_TIME=$(date +%s.%3N)
    TEST_PASSED_COUNT=0
    TEST_FAILED_COUNT=0
}

# Function to record test result for statistics
record_test_result() {
    local status=$1
    if [[ "${status}" -eq 0 ]]; then
        ((TEST_PASSED_COUNT++))
    else
        ((TEST_FAILED_COUNT++))
    fi
}

# Function to calculate elapsed time in SSS.ZZZ format for console output
get_elapsed_time() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time
        end_time=$(date +%s.%3N) 
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        end_ms=$((10#${end_ms}))
        start_ms=$((10#${start_ms}))
        local end_total_ms=$((end_secs * 1000 + end_ms))
        local start_total_ms=$((start_secs * 1000 + start_ms))
        local elapsed_ms=$((end_total_ms - start_total_ms))
        local seconds=$((elapsed_ms / 1000))
        local milliseconds=$((elapsed_ms % 1000))
        printf "%03d.%03d" "${seconds}" "${milliseconds}"
    else
        echo "000.000"
    fi
}

# Function to calculate elapsed time in decimal format (X.XXX) for table output
get_elapsed_time_decimal() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time
        end_time=$(date +%s.%3N) 
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        end_ms=$((10#${end_ms}))
        start_ms=$((10#${start_ms}))
        local end_total_ms=$((end_secs * 1000 + end_ms))
        local start_total_ms=$((start_secs * 1000 + start_ms))
        local elapsed_ms=$((end_total_ms - start_total_ms))
        local seconds=$((elapsed_ms / 1000))
        local milliseconds=$((elapsed_ms % 1000))
        printf "%d.%03d" "${seconds}" "${milliseconds}"
    else
        echo "0.000"
    fi
}

# Function to format file size with thousands separators
format_file_size() {
    local file_size="$1"
    # Use printf with thousands separator if available, otherwise return as-is
    if command -v printf >/dev/null 2>&1; then
        printf "%'d" "${file_size}" 2>/dev/null || echo "${file_size}"
    else
        echo "${file_size}"
    fi
}

#==============================================================================
# OUTPUT COLLECTION FUNCTIONS
#==============================================================================

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

#==============================================================================
# HEADER AND COMPLETION FUNCTIONS
#==============================================================================

# Function to print beautiful test header using tables.sh
print_test_header() {
    local test_name="$1"
    local test_abbr="$2"
    local test_number="$3"
    local test_version="$4"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Start the test timer
    start_test_timer
    
    # Automatically enable output collection for performance optimization
    enable_output_collection
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create the test header content
    local test_id="${test_number}-${test_abbr}"

    # Fall back to external process for backward compatibility
    local temp_dir
    temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; return 1; }
    local layout_json="${temp_dir}/header_layout.json"
    local data_json="${temp_dir}/header_data.json"
    
    # Create layout JSON with proper columns
    cat > "${layout_json}" << EOF
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
    local tables_exe="${script_dir}/tables"
    if [[ -f "${tables_exe}" ]]; then
        "${tables_exe}" "${layout_json}" "${data_json}" 2>/dev/null
    fi
    
    # Clean up temporary files
    rm -rf "${temp_dir}" 2>/dev/null
}

# Function to print subtest headers
print_subtest() {
    local subtest_name="$1"
    local elapsed
    elapsed=$(get_elapsed_time)
    local processed_name
    processed_name=$(process_message "${subtest_name}")
    local formatted_output="  ${TEST_COLOR}${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}   ${elapsed}   ${NC}${TEST_ICON}${TEST_COLOR} TEST   ${processed_name}${NC}"
    
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

#==============================================================================
# CORE OUTPUT FUNCTIONS
#==============================================================================

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

# Function to print beautiful test suite runner header using tables.sh with blue theme
print_test_suite_header() {
    local test_name="$1"
    local test_abbr="$2"
    local test_number="$3"
    local test_version="$4"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Start the test timer
    start_test_timer
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create the test header content
    local test_id="${test_number}-${test_abbr}"
    
    # Fall back to external process for backward compatibility
    local temp_dir
    temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; return 1; }
    local layout_json="${temp_dir}/suite_header_layout.json"
    local data_json="${temp_dir}/suite_header_data.json"
    
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
    local tables_exe="${script_dir}/tables"
    if [[ -f "${tables_exe}" ]]; then
        "${tables_exe}" "${layout_json}" "${data_json}" 2>/dev/null
    fi
    
    # Clean up temporary files
    rm -rf "${temp_dir}" 2>/dev/null
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
        disable_output_collection
        dump_collected_output
    fi
    
    # Write elapsed time to the subtest result file if running in test suite
    # Use the SAME elapsed_time value that was calculated above - no additional calls to get_elapsed_time
    if [[ -n "${ORCHESTRATION}" ]]; then
        # Use the elapsed_time that was already calculated above - SINGLE SOURCE OF TRUTH
        local file_elapsed_time="${elapsed_time}"
        # Write the elapsed time to a result file with a timestamp and PID for uniqueness
        local timestamp
        timestamp=$(date +%s.%3N 2>/dev/null || date +%s)
        # Add process ID and random component to ensure uniqueness in parallel execution
        local unique_id="${timestamp}_$${_}${RANDOM}"
        local subtest_file="${RESULTS_DIR}/subtest_${CURRENT_TEST_NUMBER}_${unique_id}.txt"
        echo "${total_subtests},${TEST_PASSED_COUNT},${test_name},${file_elapsed_time},${test_abbr},${test_version}" > "${subtest_file}" 2>/dev/null
    fi

    # Fall back to external process for backward compatibility
    local temp_dir
    temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; return 1; }
    local layout_json="${temp_dir}/completion_layout.json"
    local data_json="${temp_dir}/completion_data.json"
    
    # Create layout JSON with proper columns
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
    cat > "${data_json}" << EOF
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
    local tables_exe="${LIB_DIR}/tables"
    if [[ -f "${tables_exe}" ]]; then
        "${tables_exe}" "${layout_json}" "${data_json}" 2>/dev/null
    fi
    
    # Clean up temporary files
    # rm -rf "${temp_dir}" 2>/dev/null
}

# Function to print individual test items in a summary
print_test_item() {
    local status=$1
    local name=$2
    local details=$3
    local prefix
    prefix=$(get_test_prefix)
    local processed_name
    processed_name=$(process_message "${name}")
    local processed_details
    processed_details=$(process_message "${details}")
    
    local formatted_output
    if [[ "${status}" -eq 0 ]]; then
        formatted_output="${prefix} ${PASS_COLOR}${PASS_ICON} ${PASS_COLOR}PASS${NC} ${BOLD}${processed_name}${NC} - ${processed_details}"
    else
        formatted_output="${prefix} ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}FAIL${NC} ${BOLD}${processed_name}${NC} - ${processed_details}"
    fi
    
    if [[ "${COLLECT_OUTPUT_MODE}" == "true" ]]; then
        OUTPUT_COLLECTION+=("${formatted_output}")
    else
        echo -e "${formatted_output}"
    fi
}
