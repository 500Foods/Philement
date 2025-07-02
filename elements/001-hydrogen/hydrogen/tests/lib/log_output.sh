#!/bin/bash
#
# Hydrogen Test Suite - Log Output Library
# Provides consistent logging, formatting, and display functions for test scripts
# ALL output must go through functions in this library - zero exceptions
#
NAME_SCRIPT="Hydrogen Log Output Library"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-07-02 - Complete rewrite with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Global variables for test/subtest numbering
CURRENT_TEST_NUMBER=""
CURRENT_SUBTEST_NUMBER=""
SUBTEST_COUNTER=0

# Global variables for timing and statistics
TEST_START_TIME=""
TEST_PASSED_COUNT=0
TEST_FAILED_COUNT=0

# Function to display script version information
print_log_output_version() {
    echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="
}

# Set up color codes for better readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Icons/symbols for test results
PASS_ICON="✅"
FAIL_ICON="✖️"
WARN_ICON="⚠️"
INFO_ICON="ℹ️"

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
    echo "$file_size" | sed ':a;s/\B[0-9]\{3\}\>/,&/;ta'
}

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
    local left_content="$test_id $test_name"
    local right_content="v$script_version $timestamp"
    local full_content="$left_content - $right_content"
    
    # Create temporary files for tables.sh
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
    else
        # Fallback to manual formatting if tables.sh not found
        echo -e "${BLUE}${BOLD}╭──────────────────────────────────────────────────────────────────────────────────────────────────╮${NC}"
        echo -e "${BLUE}${BOLD}│ ${full_content} │${NC}"
        echo -e "${BLUE}${BOLD}╰──────────────────────────────────────────────────────────────────────────────────────────────────╯${NC}"
    fi
    
    # Clean up temporary files
    rm -rf "$temp_dir" 2>/dev/null
}

# Function to print test headers (main test level) - legacy version
print_test() {
    local test_name="$1"
    echo ""
    echo -e "${BLUE}${BOLD}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}${BOLD}TEST ${CURRENT_TEST_NUMBER}: ${test_name}${NC}"
    echo -e "${BLUE}${BOLD}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}Started at: $(date)${NC}"
    echo ""
}

# Function to print subtest headers
print_subtest() {
    local subtest_name="$1"
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "${BLUE}${BOLD}  ${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}   ${elapsed}   ${subtest_name}${NC}"
}

# Function to print commands that will be executed
print_command() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    # Convert any absolute paths in the command to relative paths
    local cmd
    cmd=$(echo "$1" | sed -E "s|/[[:alnum:]/_-]+/elements/001-hydrogen/hydrogen/|hydrogen/|g")
    # Truncate command to approximately 100 characters
    local truncated_cmd
    if [ ${#cmd} -gt 100 ]; then
        truncated_cmd="${cmd:0:97}..."
    else
        truncated_cmd="$cmd"
    fi
    echo -e "  ${prefix}   ${elapsed}   ${YELLOW}➡️  EXEC${NC}   ${truncated_cmd}"
}

# Function to print command output
print_output() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${prefix}   ${elapsed}   ${CYAN}OUT${NC}   ${1}"
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
        echo -e "  ${prefix}   ${elapsed}   ${GREEN}${PASS_ICON} PASS${NC}   ${message}"
    else
        echo -e "  ${prefix}   ${elapsed}   ${RED}${FAIL_ICON}  FAIL${NC}   ${message}"
    fi
}

# Function to print warnings
print_warning() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${prefix}   ${elapsed}   ${YELLOW}${WARN_ICON} WARN${NC}   ${1}"
}

# Function to print error messages
print_error() {
    local prefix
    prefix=$(get_test_prefix)
    local elapsed
    elapsed=$(get_elapsed_time)
    echo -e "  ${prefix}   ${elapsed}   ${RED}${FAIL_ICON}  ERROR${NC}   ${1}"
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
    echo -e "  ${prefix}   ${elapsed}   ${CYAN}${INFO_ICON}  INFO${NC}   ${message}"
}

# Function to print newlines (controlled spacing)
print_newline() {
    echo ""
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
    
    # Create temporary files for tables.sh
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
        echo ""
    else
        # Fallback to manual formatting if tables.sh not found
        echo -e "${BLUE}${BOLD}Test Completion Summary${NC}"
        echo -e "${BLUE}Test: ${CURRENT_TEST_NUMBER}-000 $test_name${NC}"
        echo -e "${BLUE}Subtests: $total_subtests, Passed: $TEST_PASSED_COUNT, Failed: $TEST_FAILED_COUNT${NC}"
        echo -e "${BLUE}Elapsed: ${elapsed_time}s${NC}"
        echo ""
    fi
    
    # Clean up temporary files
    rm -rf "$temp_dir" 2>/dev/null
}

# Function to print test summary at the end (legacy version)
print_test_summary() {
    local total=$1
    local passed=$2
    local failed=$3
    
    echo ""
    echo -e "${BLUE}${BOLD}───────────────────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD}Test Summary${NC}"
    echo -e "${BLUE}${BOLD}───────────────────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}Completed at: $(date)${NC}"
    if [ "$failed" -eq 0 ]; then
        echo -e "${GREEN}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}"
    else
        echo -e "${RED}${FAIL_ICON} OVERALL RESULT: SOME TESTS FAILED${NC}"
    fi
    echo -e "${BLUE}${BOLD}───────────────────────────────────────────────────────────────────────────────${NC}"
    echo ""
    # Suppress ShellCheck warnings for unused parameters
    # shellcheck disable=SC2034
    local dummy_total=$total
    # shellcheck disable=SC2034
    local dummy_passed=$passed
}

# Function to print individual test items in a summary
print_test_item() {
    local status=$1
    local name=$2
    local details=$3
    local prefix
    prefix=$(get_test_prefix)
    
    if [ "$status" -eq 0 ]; then
        echo -e "${prefix} ${GREEN}${PASS_ICON} PASS${NC} ${BOLD}$name${NC} - $details"
    else
        echo -e "${prefix} ${RED}${FAIL_ICON} FAIL${NC} ${BOLD}$name${NC} - $details"
    fi
}

# Legacy compatibility functions (deprecated - use specific functions above)
print_header() {
    print_warning "print_header() is deprecated - use print_test() or print_subtest()"
    print_test "$1"
}

print_info() {
    print_warning "print_info() is deprecated - use print_message()"
    print_message "$1"
}
