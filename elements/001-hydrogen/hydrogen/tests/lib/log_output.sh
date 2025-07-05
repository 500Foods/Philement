#!/bin/bash
#
# Test Suite - Log Output Library

# Guard clause to prevent multiple sourcing
if [[ -n "$LOG_OUTPUT_SH_GUARD" ]]; then
    return 0
fi
export LOG_OUTPUT_SH_GUARD="true"
# Provides consistent logging, formatting, and display functions for test scripts
# ALL output must go through functions in this library - zero exceptions
#
# OVERVIEW:
# This library provides a comprehensive logging system for test scripts with:
# - Consistent color-coded output with icons
# - Test/subtest numbering and timing
# - Beautiful table-based headers and completion summaries
# - Automatic test statistics tracking
#
# USAGE:
# 1. Source this library in your test script
# 2. Call extract_test_number() and set_test_number() to initialize
# 3. Use print_* functions for all output
# 4. Call print_test_completion() at the end
#
# VERSION HISTORY
# 3.0.3 - 2025-07-03 - Applied color consistency to all output types (DATA, EXEC, PASS, FAIL)
# 3.0.2 - 2025-07-03 - Completely removed legacy functions (print_header, print_info)
# 3.0.1 - 2025-07-03 - Enhanced documentation, removed unused functions, improved comments
# 3.0.0 - 2025-07-03 - General overhaul of colors and icons, removed legacy functions, etc.
# 2.1.0 - 2025-07-02 - Added DATA_ICON and updated DATA_COLOR to pale yellow (256-color palette)
# 2.0.0 - 2025-07-02 - Complete rewrite with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

#==============================================================================
# GLOBAL VARIABLES AND CONSTANTS
#==============================================================================

# Global variables for test/subtest numbering
CURRENT_TEST_NUMBER=""
CURRENT_SUBTEST_NUMBER=""
SUBTEST_COUNTER=0

# Global variables for timing and statistics
TEST_START_TIME=""
TEST_PASSED_COUNT=0
TEST_FAILED_COUNT=0

# Icon-specific colors for consistent theming
PASS_COLOR='\033[0;32m'     # Green
FAIL_COLOR='\033[0;31m'     # Red
WARN_COLOR='\033[0;33m'     # Yellow
INFO_COLOR='\033[0;36m'     # Cyan
DATA_COLOR='\033[38;5;229m' # Pale, soft yellow from 256-color palette
EXEC_COLOR='\033[0;33m'     # Yellow
TEST_COLOR='\033[0;34m'     # Blue

# Other formatting codes
BOLD='\033[1m'
NC='\033[0m' # No Color

# Rounded rectangle icons with colors matching label themes (double-wide)
PASS_ICON="${PASS_COLOR}\U2587\U2587${NC}"
FAIL_ICON="${FAIL_COLOR}\U2587\U2587${NC}"
WARN_ICON="${WARN_COLOR}\U2587\U2587${NC}"
INFO_ICON="${INFO_COLOR}\U2587\U2587${NC}"
DATA_ICON="${DATA_COLOR}\U2587\U2587${NC}"
EXEC_ICON="${EXEC_COLOR}\U2587\U2587${NC}"
TEST_ICON="${TEST_COLOR}\U2587\U2587${NC}"

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
    filename=$(basename "$script_path")
    
    # Extract number from test_XX_name.sh format
    if [[ "$filename" =~ test_([0-9]+)_ ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo "XX"
    fi
}

# Function to increment and get next subtest number
next_subtest() {
    ((SUBTEST_COUNTER++))
    CURRENT_SUBTEST_NUMBER=$(printf "%03d" $SUBTEST_COUNTER)
}

# Function to reset subtest counter
reset_subtest_counter() {
    SUBTEST_COUNTER=0
    CURRENT_SUBTEST_NUMBER=""
}

# Function to get the current test prefix for output
get_test_prefix() {
    if [ -n "$CURRENT_SUBTEST_NUMBER" ]; then
        echo "${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}"
    elif [ -n "$CURRENT_TEST_NUMBER" ]; then
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
    if [ "$status" -eq 0 ]; then
        ((TEST_PASSED_COUNT++))
    else
        ((TEST_FAILED_COUNT++))
    fi
}

# Function to calculate elapsed time in SSS.ZZZ format with at least three leading zeros for seconds
get_elapsed_time() {
    if [ -n "$TEST_START_TIME" ]; then
        local end_time
        end_time=$(date +%s.%3N)
        local elapsed
        elapsed=$(echo "$end_time - $TEST_START_TIME" | bc -l 2>/dev/null || echo "0.000")
        printf "%07.3f" "$elapsed"
    else
        echo "000.000"
    fi
}

# Function to format file size with thousands separators
format_file_size() {
    local file_size="$1"
    # Use printf with thousands separator if available, otherwise return as-is
    if command -v printf >/dev/null 2>&1; then
        printf "%'d" "$file_size" 2>/dev/null || echo "$file_size"
    else
        echo "$file_size"
    fi
}

#==============================================================================
# HEADER AND COMPLETION FUNCTIONS
#==============================================================================

# Function to print beautiful test header using tables.sh
print_test_header() {
    local test_name="$1"
    local script_version="$2"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Start the test timer
    start_test_timer
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create the test header content
    local test_id="${CURRENT_TEST_NUMBER}-000"

    # Check if tables.sh is sourced for performance optimization
    if [[ "$TABLES_SOURCED" == "true" ]]; then
        # Use sourced function directly with JSON strings
        local layout_json_content='{
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
                    "width": 50
                },
                {
                    "header": "Version",
                    "key": "version",
                    "datatype": "text",
                    "width": 12
                },
                {
                    "header": "Started",
                    "key": "timestamp",
                    "datatype": "text",
                    "justification": "right"
                }
            ]
        }'
        
        local data_json_content='[
            {
                "test_id": "'"$test_id"'",
                "test_name": "'"$test_name"'",
                "version": "v'"$script_version"'",
                "timestamp": "'"$timestamp"'"
            }
        ]'
        
        # Use sourced tables.sh function directly
        tables_render_from_json "$layout_json_content" "$data_json_content"
    else
        echo "Falling back to external tables.sh process for header rendering" >&2
        print_message "Falling back to external tables.sh process for header rendering"
        # Fall back to external process for backward compatibility
        local temp_dir
        temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; return 1; }
        local layout_json="$temp_dir/header_layout.json"
        local data_json="$temp_dir/header_data.json"
        
        # Create layout JSON with proper columns
        cat > "$layout_json" << EOF
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
            "width": 50
        },
        {
            "header": "Version",
            "key": "version",
            "datatype": "text",
            "width": 12
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
        cat > "$data_json" << EOF
[
    {
        "test_id": "$test_id",
        "test_name": "$test_name",
        "version": "v$script_version",
        "timestamp": "$timestamp"
    }
]
EOF
        
        # Use tables.sh to render the header
        local tables_script="$script_dir/tables.sh"
        if [[ -f "$tables_script" ]]; then
            bash "$tables_script" "$layout_json" "$data_json" 2>/dev/null
        fi
        
        # Clean up temporary files
        rm -rf "$temp_dir" 2>/dev/null
    fi
}

# Function to print subtest headers
print_subtest() {
    local subtest_name="$1"
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${TEST_COLOR}${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}   ${elapsed}   ${NC}${TEST_ICON}${TEST_COLOR} TEST   ${subtest_name}${NC}"
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
    # Convert any absolute paths in the command to relative paths
    local cmd
    cmd=$(echo "$1" | sed -E "s|/[[:alnum:]/_-]+/elements/001-hydrogen/hydrogen/|hydrogen/|g")
    # Truncate command to approximately 200 characters
    local truncated_cmd
    if [ ${#cmd} -gt 200 ]; then
        truncated_cmd="${cmd:0:197}..."
    else
        truncated_cmd="$cmd"
    fi
    echo -e "  ${prefix}   ${elapsed}   ${EXEC_COLOR}${EXEC_ICON} ${EXEC_COLOR}EXEC${NC}   ${EXEC_COLOR}${truncated_cmd}${NC}"
}

# Function to print command output
print_output() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    local message="$1"
    message=$(echo "$message" | sed -E "s|\/home\/asimard\/Projects\/Philement\/elements\/001-hydrogen\/|hydrogen/|g")
    # Skip output if message is empty or contains only whitespace
    if [[ -n "$message" && ! "$message" =~ ^[[:space:]]*$ ]]; then
        echo -e "  ${prefix}   ${elapsed}   ${DATA_COLOR}${DATA_ICON} ${DATA_COLOR}DATA${NC}   ${DATA_COLOR}${message}${NC}"
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
    
    # Record the result for statistics
    record_test_result "$status"
    
    if [ "$status" -eq 0 ]; then
        echo -e "  ${prefix}   ${elapsed}   ${PASS_COLOR}${PASS_ICON} ${PASS_COLOR}PASS${NC}   ${PASS_COLOR}${message}${NC}"
    else
        echo -e "  ${prefix}   ${elapsed}   ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}FAIL${NC}   ${FAIL_COLOR}${message}${NC}"
    fi
}

# Function to print warnings
print_warning() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${prefix}   ${elapsed}   ${WARN_COLOR}${WARN_ICON} ${WARN_COLOR}WARN${NC}   ${1}"
}

# Function to print error messages
print_error() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${prefix}   ${elapsed}   ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}ERROR${NC}   ${1}"
}

# Function to print informational messages
print_message() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    # Convert absolute paths to relative paths if convert_to_relative_path function is available
    local message="$1"
    if command -v convert_to_relative_path >/dev/null 2>&1; then
        message=$(echo "$message" | sed -E "s|/[[:alnum:]/_-]+/elements/001-hydrogen/hydrogen/|hydrogen/|g")
    fi
    echo -e "  ${prefix}   ${elapsed}   ${INFO_COLOR}${INFO_ICON} ${INFO_COLOR}INFO${NC}   ${message}"
}

# Function to print newlines (controlled spacing)
print_newline() {
    echo ""
}

# Function to print beautiful test suite runner header using tables.sh with blue theme
print_test_suite_header() {
    local test_name="$1"
    local script_version="$2"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Start the test timer
    start_test_timer
    
    # Create timestamp with milliseconds
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S.%3N')
    
    # Create the test header content
    local test_id="${CURRENT_TEST_NUMBER}-000"
    
    # Check if tables.sh is sourced for performance optimization
    if [[ "$TABLES_SOURCED" == "true" ]]; then
        # Use sourced function directly with JSON strings
        local layout_json_content='{
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
                    "width": 50
                },
                {
                    "header": "Version",
                    "key": "version",
                    "datatype": "text",
                    "width": 12
                },
                {
                    "header": "Started",
                    "key": "timestamp",
                    "datatype": "text",
                    "justification": "right"
                }
            ]
        }'
        
        local data_json_content='[
            {
                "test_id": "'"$test_id"'",
                "test_name": "'"$test_name"'",
                "version": "v'"$script_version"'",
                "timestamp": "'"$timestamp"'"
            }
        ]'
        
        # Use sourced tables.sh function directly
        tables_render_from_json "$layout_json_content" "$data_json_content"
    else
        # Fall back to external process for backward compatibility
        local temp_dir
        temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; return 1; }
        local layout_json="$temp_dir/suite_header_layout.json"
        local data_json="$temp_dir/suite_header_data.json"
        
        # Create layout JSON with proper columns and blue theme
        cat > "$layout_json" << EOF
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
            "width": 50
        },
        {
            "header": "Version",
            "key": "version",
            "datatype": "text",
            "width": 12
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
        cat > "$data_json" << EOF
[
    {
        "test_id": "$test_id",
        "test_name": "$test_name",
        "version": "v$script_version",
        "timestamp": "$timestamp"
    }
]
EOF
        
        # Use tables.sh to render the header
        local tables_script="$script_dir/tables.sh"
        if [[ -f "$tables_script" ]]; then
            bash "$tables_script" "$layout_json" "$data_json" 2>/dev/null
        fi
        
        # Clean up temporary files
        rm -rf "$temp_dir" 2>/dev/null
    fi
}

# Function to print beautiful test completion table using tables.sh
print_test_completion() {
    local test_name="$1"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Calculate totals and elapsed time
    local total_subtests=$((TEST_PASSED_COUNT + TEST_FAILED_COUNT))
    local elapsed_time
    elapsed_time=$(get_elapsed_time)
    
    # Check if tables.sh is sourced for performance optimization
    if [[ "$TABLES_SOURCED" == "true" ]]; then
        # Use sourced function directly with JSON strings
        local layout_json_content='{
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
                    "width": 50    
                },
                {
                    "header": "Tests",
                    "key": "total_subtests",
                    "datatype": "int",
                    "justification": "right",
                    "width": 8
                },
                {
                    "header": "Pass",
                    "key": "passed",
                    "datatype": "int",
                    "justification": "right",
                    "width": 8
                },
                {
                    "header": "Fail",
                    "key": "failed",
                    "datatype": "int",
                    "justification": "right",
                    "width": 8
                },
                {
                    "header": "Elapsed",
                    "key": "elapsed",
                    "datatype": "text",
                    "justification": "right",
                    "width": 11
                }
            ]
        }'
        
        local data_json_content='[
            {
                "test_id": "'"${CURRENT_TEST_NUMBER}-000"'",
                "test_name": "'"$test_name"'",
                "total_subtests": '"$total_subtests"',
                "passed": '"$TEST_PASSED_COUNT"',
                "failed": '"$TEST_FAILED_COUNT"',
                "elapsed": "'"${elapsed_time}s"'"
            }
        ]'
        
        # Use sourced tables.sh function directly
        tables_render_from_json "$layout_json_content" "$data_json_content"
    else
        # Fall back to external process for backward compatibility
        local temp_dir
        temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; return 1; }
        local layout_json="$temp_dir/completion_layout.json"
        local data_json="$temp_dir/completion_data.json"
        
        # Create layout JSON with proper columns
        cat > "$layout_json" << EOF
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
            "width": 50    
        },
        {
            "header": "Tests",
            "key": "total_subtests",
            "datatype": "int",
            "justification": "right",
            "width": 8
        },
        {
            "header": "Pass",
            "key": "passed",
            "datatype": "int",
            "justification": "right",
            "width": 8
        },
        {
            "header": "Fail",
            "key": "failed",
            "datatype": "int",
            "justification": "right",
            "width": 8
        },
        {
            "header": "Elapsed",
            "key": "elapsed",
            "datatype": "text",
            "justification": "right",
            "width": 11
        }
    ]
}
EOF

        # Create data JSON with the test completion information
        cat > "$data_json" << EOF
[
    {
        "test_id": "${CURRENT_TEST_NUMBER}-000",
        "test_name": "$test_name",
        "total_subtests": $total_subtests,
        "passed": $TEST_PASSED_COUNT,
        "failed": $TEST_FAILED_COUNT,
        "elapsed": "${elapsed_time}s"
    }
]
EOF
        
        # Use tables.sh to render the completion table
        local tables_script="$script_dir/tables.sh"
        if [[ -f "$tables_script" ]]; then
            bash "$tables_script" "$layout_json" "$data_json" 2>/dev/null
        fi
        
        # Clean up temporary files
        rm -rf "$temp_dir" 2>/dev/null
    fi
}

# Function to print individual test items in a summary
print_test_item() {
    local status=$1
    local name=$2
    local details=$3
    local prefix
    prefix=$(get_test_prefix)
    
    if [ "$status" -eq 0 ]; then
        echo -e "${prefix} ${PASS_COLOR}${PASS_ICON} ${PASS_COLOR}PASS${NC} ${BOLD}$name${NC} - $details"
    else
        echo -e "${prefix} ${FAIL_COLOR}${FAIL_ICON} ${FAIL_COLOR}FAIL${NC} ${BOLD}$name${NC} - $details"
    fi
}
