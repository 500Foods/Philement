#!/bin/bash
#
# Hydrogen Test Utilities
# Provides common functions for test scripts to ensure consistent formatting and reporting
#

# Set up color codes for better readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Icons/symbols for test results
PASS_ICON="✅"
FAIL_ICON="❌"
WARN_ICON="⚠️ "
INFO_ICON="🛈 "

# Function to print section headers
print_header() {
    echo -e "\n${BLUE}============================================${NC}"
    echo -e "${BLUE}${BOLD}$1${NC}"
    echo -e "${BLUE}============================================${NC}"
}

# Function to print test results
print_result() {
    local status=$1
    local message=$2
    
    if [ $status -eq 0 ]; then
        echo -e "${GREEN}${PASS_ICON} $message${NC}"
    else
        echo -e "${RED}${FAIL_ICON} $message${NC}"
    fi
}

# Function to print a warning
print_warning() {
    echo -e "${YELLOW}${WARN_ICON} $1${NC}"
}

# Function to print an informational message
print_info() {
    echo -e "${CYAN}${INFO_ICON} $1${NC}"
}

# Function to print a command that will be executed
print_command() {
    echo -e "${YELLOW}Executing: $1${NC}"
}

# Function to start a test run with proper header
start_test() {
    local test_name=$1
    print_header "$test_name"
    echo "Started at: $(date)"
    echo ""
}

# Function to end a test run with proper footer
end_test() {
    local test_result=$1
    local test_name=$2
    
    print_header "Test Summary"
    echo "Completed at: $(date)"
    
    if [ $test_result -eq 0 ]; then
        echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}"
    else
        echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}"
    fi
    echo -e "${BLUE}============================================${NC}"
    echo ""
    
    return $test_result
}

# Function to print a test summary with details
print_test_summary() {
    local total=$1
    local passed=$2
    local failed=$3
    
    echo -e "\n${BLUE}${BOLD}Summary Statistics:${NC}"
    echo -e "  Total Tests: $total"
    echo -e "  ${GREEN}Passed: $passed${NC}"
    if [ $failed -gt 0 ]; then
        echo -e "  ${RED}Failed: $failed${NC}"
    else
        echo -e "  Failed: $failed"
    fi
    echo ""
}

# Function to print individual test results for a summary
print_test_item() {
    local status=$1
    local name=$2
    local details=$3
    
    if [ $status -eq 0 ]; then
        echo -e "  ${GREEN}${PASS_ICON} PASS${NC}: ${BOLD}$name${NC} - $details"
    else
        echo -e "  ${RED}${FAIL_ICON} FAIL${NC}: ${BOLD}$name${NC} - $details"
    fi
}

# Function to generate an HTML summary report for a test
generate_html_report() {
    local result_file=$1
    local html_file="${result_file%.log}.html"
    
    # Create HTML header
    cat > "$html_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hydrogen Test Results</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1000px;
            margin: 0 auto;
            padding: 20px;
        }
        h1, h2, h3 {
            color: #0066cc;
        }
        .test-summary {
            margin: 20px 0;
            padding: 15px;
            border-radius: 5px;
            background-color: #f5f5f5;
        }
        .test-header {
            background-color: #0066cc;
            color: white;
            padding: 10px 15px;
            border-radius: 5px 5px 0 0;
            margin-bottom: 0;
        }
        .test-body {
            border: 1px solid #ddd;
            border-top: none;
            border-radius: 0 0 5px 5px;
            padding: 15px;
            background-color: white;
        }
        .pass {
            color: #00aa00;
            font-weight: bold;
        }
        .fail {
            color: #dd0000;
            font-weight: bold;
        }
        .warning {
            color: #ff9900;
            font-weight: bold;
        }
        .stats {
            display: flex;
            justify-content: space-between;
            max-width: 300px;
            margin: 10px 0;
        }
        .test-item {
            margin: 10px 0;
            padding: 10px;
            border-radius: 5px;
            background-color: #f9f9f9;
        }
        .pass-icon:before {
            content: "✅ ";
        }
        .fail-icon:before {
            content: "❌ ";
        }
        .warn-icon:before {
            content: "⚠️ ";
        }
        .timestamp {
            color: #666;
            font-size: 0.9em;
        }
        pre {
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 3px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <h1>Hydrogen Test Results</h1>
    <p class="timestamp">Generated on $(date)</p>
EOF
    
    # Parse the log file and generate the report content
    # This is a placeholder - each test would need to call this with appropriate parsing
    
    # Close the HTML
    cat >> "$html_file" << EOF
</body>
</html>
EOF
    
    echo "HTML report generated: $html_file"
    return 0
}

# Export the test result to a standardized JSON format for the main summary
export_test_results() {
    local test_name=$1
    local result=$2
    local details=$3
    local output_file=$4
    
    # Create a simple JSON structure
    cat > "$output_file" << EOF
{
    "test_name": "$test_name",
    "status": $result,
    "details": "$details",
    "timestamp": "$(date +%Y-%m-%d\ %H:%M:%S)"
}
EOF
}