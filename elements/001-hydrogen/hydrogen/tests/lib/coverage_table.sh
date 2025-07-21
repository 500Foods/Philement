#!/bin/bash

# Coverage Table Library
# Displays comprehensive coverage data from Unity and blackbox tests in a formatted table

# CHANGELOG
# 1.0.6 - 2025-07-18 - Added timestamp to footer for coverage table generation time
# 1.0.5 - 2025-07-16 - Fixed Cover column to calculate TRUE union of coverage from Unity and Blackbox tests
# 1.0.4 - 2025-07-14 - Fixed file path extraction using Source: line from gcov files for consistent table alignment
# 1.0.3 - 2025-07-14 - Fixed Tests column to use same gcov processing logic as test 99 for accurate coverage summaries
# 1.0.2 - Added MAGENTA color flag for files with >0 coverage but <50% coverage when >100 instrumented lines
# 1.0.1 - Added YELLOW color flag for files with no coverage in either test type
# 1.0.0 - Initial version

COVERAGE_TABLE_NAME="Coverage Table Library"
COVERAGE_TABLE_VERSION="1.0.0"

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
SCRIPT_DIR="$PROJECT_DIR/tests"
LIB_DIR="$SCRIPT_DIR/lib"
BUILD_DIR="$PROJECT_DIR/build"
TESTS_DIR="$BUILD_DIR/tests"
RESULTS_DIR="$TESTS_DIR/results"
DIAGS_DIR="$TESTS_DIR/diagnostics"
LOGS_DIR="$TESTS_DIR/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

UNITY_COVS="$BUILD_DIR/unity/src"
BLACKBOX_COVS="$BUILD_DIR/coverage/src"
TABLES_EXE="$LIB_DIR/tables"

# Source the coverage common functions for combined analysis
source "$LIB_DIR/coverage-common.sh"
source "$LIB_DIR/coverage-unity.sh"
source "$LIB_DIR/coverage-blackbox.sh"

# Associative arrays to store coverage data from both directories
declare -A unity_covered_lines
declare -A unity_instrumented_lines
declare -A coverage_covered_lines
declare -A coverage_instrumented_lines
declare -A all_files

# Use the exact same functions as test 99 to get coverage data
timestamp=$(date '+%Y%m%d_%H%M%S')

# Generate display timestamp for footer
display_timestamp=$(date '+%Y-%m-%d %H:%M:%S %Z')

# Call the same functions that test 99 uses - this ensures we get the same results
unity_coverage_percentage=$(calculate_unity_coverage "$UNITY_COVS" "$timestamp" 2>/dev/null || echo "0.000")
blackbox_coverage_percentage=$(collect_blackbox_coverage_from_dir "$BLACKBOX_COVS" "$timestamp" 2>/dev/null || echo "0.000")

# Read the detailed coverage data that was just generated
unity_files=0
coverage_files=0
if [ -f "${UNITY_COVERAGE_FILE}.detailed" ]; then
    IFS=',' read -r _ _ _ _ unity_instrumented_files unity_covered_files < "${UNITY_COVERAGE_FILE}.detailed"
    unity_files=$unity_covered_files
fi

if [ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]; then
    IFS=',' read -r _ _ _ _ coverage_instrumented_files coverage_covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
    coverage_files=$coverage_covered_files
fi

gcov_files_found=$((unity_files + coverage_files))

if [ "$gcov_files_found" -eq 0 ]; then
    # No gcov files found - create table with zero values instead of exiting
    # echo "No gcov files found - generating empty coverage table with zero values"
    
    # Set all totals to zero
    unity_total_covered=0
    unity_total_instrumented=0
    coverage_total_covered=0
    coverage_total_instrumented=0
    combined_total_covered=0
    combined_total_instrumented=0
    
    # Set all percentages to zero
    unity_total_pct="0.000"
    coverage_total_pct="0.000" 
    combined_total_pct="0.000"
    
    # Store the zero combined coverage value for other scripts to use
    echo "$combined_total_pct" > "$COMBINED_COVERAGE_FILE"
    
    # Create temporary directory for JSON files
    temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
    layout_json="$temp_dir/coverage_layout.json"
    data_json="$temp_dir/coverage_data.json"
    
    # Create layout JSON for empty table
    cat > "$layout_json" << EOF
{
    "title": "Test Suite Coverage {NC}{RED}———{RESET}{BOLD} Unity: ${unity_total_pct}% {RESET}{RED}———{RESET}{BOLD} Blackbox: ${coverage_total_pct}% {RESET}{RED}———{RESET}{BOLD} Combined: ${combined_total_pct}%",
    "footer": "{YELLOW}Zero Coverage:{RESET} {RED}———{RESET} {MAGENTA}100+ Lines < 50% Coverage:{RESET} - No coverage data available {RED}———{RESET} {CYAN}${display_timestamp}{RESET}",
    "footer_position": "right", 
    "theme": "Red",
    "columns": [
        {
            "header": "Status",
            "key": "status",
            "datatype": "text"
        }
    ]
}
EOF

    # Create data JSON with single message
    cat > "$data_json" << EOF
[
    {
        "status": "No source file coverage data available. Run Test 11 (Unity) or other Tests to generate gcov data"
    }
]
EOF

    # Use tables executable to render the empty table
    "$TABLES_EXE" "$layout_json" "$data_json" 2>/dev/null || {
        echo "Error: Failed to run tables executable"
        exit 1
    }
    
    # Clean up temporary files
    rm -rf "$temp_dir" 2>/dev/null
    exit 0
fi

# Clear our arrays and repopulate them using the working logic
declare -A unity_covered_lines
declare -A unity_instrumented_lines
declare -A coverage_covered_lines
declare -A coverage_instrumented_lines
declare -A combined_covered_lines
declare -A combined_instrumented_lines

# Process all coverage types using optimized batch processing
analyze_all_gcov_coverage_batch "$UNITY_COVS" "$BLACKBOX_COVS"

# Populate all_files array from coverage arrays (already filtered at batch level)
for file_path in "${!unity_covered_lines[@]}"; do
    all_files["$file_path"]=1
done
for file_path in "${!coverage_covered_lines[@]}"; do
    all_files["$file_path"]=1
done

# Calculate totals for summary first
unity_total_covered=0
unity_total_instrumented=0
coverage_total_covered=0
coverage_total_instrumented=0
combined_total_covered=0
combined_total_instrumented=0

for file_path in "${!all_files[@]}"; do
    # Get Unity data
    u_covered=${unity_covered_lines["$file_path"]:-0}
    u_instrumented=${unity_instrumented_lines["$file_path"]:-0}
    
    # Get Coverage data
    c_covered=${coverage_covered_lines["$file_path"]:-0}
    c_instrumented=${coverage_instrumented_lines["$file_path"]:-0}
    
    # Get Combined data
    combined_covered=${combined_covered_lines["$file_path"]:-0}
    combined_instrumented=${combined_instrumented_lines["$file_path"]:-0}
    
    # Add to totals
    unity_total_covered=$((unity_total_covered + u_covered))
    unity_total_instrumented=$((unity_total_instrumented + u_instrumented))
    coverage_total_covered=$((coverage_total_covered + c_covered))
    coverage_total_instrumented=$((coverage_total_instrumented + c_instrumented))
    combined_total_covered=$((combined_total_covered + combined_covered))
    combined_total_instrumented=$((combined_total_instrumented + combined_instrumented))
done

# Calculate total percentages
unity_total_pct="0.000"
if [ "$unity_total_instrumented" -gt 0 ]; then
    unity_total_pct=$(awk "BEGIN {printf \"%.3f\", ($unity_total_covered / $unity_total_instrumented) * 100}")
fi

coverage_total_pct="0.000"
if [ "$coverage_total_instrumented" -gt 0 ]; then
    coverage_total_pct=$(awk "BEGIN {printf \"%.3f\", ($coverage_total_covered / $coverage_total_instrumented) * 100}")
fi

combined_total_pct="0.000"
if [ "$combined_total_instrumented" -gt 0 ]; then
    combined_total_pct=$(awk "BEGIN {printf \"%.3f\", ($combined_total_covered / $combined_total_instrumented) * 100}")
fi

# Store the combined coverage value for other scripts to use
echo "$combined_total_pct" > "$COMBINED_COVERAGE_FILE"

# Create temporary directory for JSON files
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
layout_json="$temp_dir/coverage_layout.json"
data_json="$temp_dir/coverage_data.json"

# Start the data JSON array
echo '[' > "$data_json"
first_record=true

# Create array for sorting by folder and file
declare -a file_data=()
zero_coverage_count=0
low_coverage_count=0

for file_path in "${!all_files[@]}"; do
    # Extract folder name from file path using first two levels below src/
    folder="src/"
    if [[ "$file_path" == src/* ]]; then
        # Get the path after src/
        path_after_src="${file_path#src/}"
        if [[ "$path_after_src" == */* ]]; then
            # Has at least one subdirectory
            first_level="${path_after_src%%/*}"
            remaining_path="${path_after_src#*/}"
            
            if [[ "$remaining_path" == */* ]]; then
                # Has at least two subdirectories
                second_level="${remaining_path%%/*}"
                folder="src/${first_level}/${second_level}"
            else
                # Only one subdirectory level
                folder="src/${first_level}"
            fi
        else
            # File directly in src/
            folder="src/"
        fi
    fi
    
    # Store data with sort key: folder:filename
    file_data+=("$folder:$file_path")
done

# Sort by folder, then by filename, ensuring src/hydrogen.c appears first
mapfile -t sorted_file_data < <(printf '%s\n' "${file_data[@]}" | sort -t: -k1,1 -k2,2)

# Generate JSON data for tables
for file_data_entry in "${sorted_file_data[@]}"; do
    folder="${file_data_entry%%:*}"
    file_path="${file_data_entry#*:}"
    
    # Skip empty entries
    if [[ -z "$file_path" ]]; then
        continue
    fi
    
    # Get Unity data
    u_covered=${unity_covered_lines["$file_path"]:-0}
    u_instrumented=${unity_instrumented_lines["$file_path"]:-0}
    u_percentage="0.000"
    if [ "$u_instrumented" -gt 0 ]; then
        u_percentage=$(awk "BEGIN {printf \"%.3f\", ($u_covered / $u_instrumented) * 100}")
    fi
    
    # Get Coverage data
    c_covered=${coverage_covered_lines["$file_path"]:-0}
    c_instrumented=${coverage_instrumented_lines["$file_path"]:-0}
    c_percentage="0.000"
    if [ "$c_instrumented" -gt 0 ]; then
        c_percentage=$(awk "BEGIN {printf \"%.3f\", ($c_covered / $c_instrumented) * 100}")
    fi
    
    # Get Combined data
    combined_covered=${combined_covered_lines["$file_path"]:-0}
    combined_instrumented=${combined_instrumented_lines["$file_path"]:-0}
    combined_percentage="0.000"
    if [ "$combined_instrumented" -gt 0 ]; then
        combined_percentage=$(awk "BEGIN {printf \"%.3f\", ($combined_covered / $combined_instrumented) * 100}")
    fi
    
    # Calculate maximum coverage percentage
    max_percentage=$(awk "BEGIN {
        u = $u_percentage
        c = $c_percentage
        if (u > c) print u; else print c
    }")
    
    # Highlight files with no coverage in either test type
    display_file_path="$file_path"
    if [[ $u_covered -eq 0 && $c_covered -eq 0 ]]; then
        display_file_path="{YELLOW}$file_path{RESET}"
        ((zero_coverage_count++))
    elif [[ $combined_covered -gt 0 && $combined_instrumented -gt 100 ]]; then
        # Check if coverage is less than 50%
        coverage_below_50=$(awk "BEGIN {print ($combined_percentage < 50.0) ? 1 : 0}")
        if [[ $coverage_below_50 -eq 1 ]]; then
            display_file_path="{MAGENTA}$file_path{RESET}"
            ((low_coverage_count++))
        fi
    fi
    
    # Add comma before record if not first
    if [ "$first_record" = false ]; then
        echo '    ,' >> "$data_json"
    else
        first_record=false
    fi
    
    # Add JSON record with folder field and max coverage
    cat >> "$data_json" << EOF
    {
        "folder": "$folder",
        "max_coverage_percentage": $max_percentage,
        "combined_coverage_percentage": $combined_percentage,
        "file_path": "$display_file_path",
        "unity_covered": $u_covered,
        "unity_instrumented": $u_instrumented,
        "unity_percentage": $u_percentage,
        "coverage_covered": $c_covered,
        "coverage_instrumented": $c_instrumented,
        "coverage_percentage": $c_percentage,
        "combined_covered": $combined_covered,
        "combined_instrumented": $combined_instrumented
    }
EOF
done

# Close the JSON array
echo '' >> "$data_json"
echo ']' >> "$data_json"

# Create layout JSON for the coverage table (now with correct totals and counts)
cat > "$layout_json" << EOF
{
    "title": "Test Suite Coverage {NC}{RED}———{RESET}{BOLD}{CYAN} Unity {WHITE}${unity_total_pct}% {RESET}{RED}———{RESET}{BOLD}{CYAN} Blackbox {WHITE}${coverage_total_pct}% {RESET}{RED}———{RESET}{BOLD}{CYAN} Combined {WHITE}${combined_total_pct}%{RESET}",
    "footer": "{YELLOW}Zero Coverage{RESET} {WHITE}${zero_coverage_count}{RESET} {RED}———{RESET} {MAGENTA}100+ Lines < 50% Coverage{RESET} {WHITE}${low_coverage_count}{RESET} {RED}———{RESET} {CYAN}Timestamp {WHITE}${display_timestamp}{RESET}",
    "footer_position": "right",
    "theme": "Red",
    "columns": [
        {
            "header": "Folder",
            "key": "folder",
            "datatype": "text",
            "visible": false,
            "break": true
        },
        {
            "header": "Cover %",
            "key": "combined_coverage_percentage",
            "datatype": "float",
            "justification": "right",
            "summary":"nonblanks"
        },
        {
            "header": "Lines",
            "key": "coverage_instrumented",
            "datatype": "int", 
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Source File",
            "key": "file_path",
            "datatype": "text",
            "summary": "count",
            "width": 52
        },
        {
            "header": "Unity",
            "key": "unity_covered",
            "datatype": "int",
            "justification": "right",
            "summary": "sum",
            "width": 8
        },
        {
            "header": "Black",
            "key": "coverage_covered",
            "datatype": "int",
            "justification": "right",
            "summary": "sum",
            "width": 8
        },
        {
            "header": "Cover",
            "key": "combined_covered",
            "datatype": "int",
            "justification": "right",
            "summary": "sum",
            "width": 8
        }
    ]
}
EOF

# Use tables executable to render the table
"$TABLES_EXE" "$layout_json" "$data_json" 2>/dev/null

# Clean up temporary files
rm -rf "$temp_dir" 2>/dev/null
