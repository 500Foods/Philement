#!/usr/bin/env bash

# Coverage Table Library
# Displays comprehensive coverage data from Unity and blackbox tests in a formatted table

# CHANGELOG
# 4.1.0 - 2025-09-17 - Added CYAN color flag for files where Unity Framework tests exceed Unity lines of coverage found
# 4.0.0 - 2025-08-13 - Added Tests column showing Unity test count per source file with caching
# 3.0.0 - 2025-08-04 - GCOV Optimization Adventure
# 2.0.1 - 2025-07-23 - Added --help and --version options
# 2.0.0 - 2025-07-22 - Upgraded for more stringent shellcheck compliance
# 1.0.6 - 2025-07-18 - Added timestamp to footer for coverage table generation time
# 1.0.5 - 2025-07-16 - Fixed Cover column to calculate TRUE union of coverage from Unity and Blackbox tests
# 1.0.4 - 2025-07-14 - Fixed file path extraction using Source: line from gcov files for consistent table alignment
# 1.0.3 - 2025-07-14 - Fixed Tests column to use same gcov processing logic as test 99 for accurate coverage summaries
# 1.0.2 - Added MAGENTA color flag for files with >0 coverage but <50% coverage when >100 instrumented lines
# 1.0.1 - Added YELLOW color flag for files with no coverage in either test type
# 1.0.0 - Initial version

set -euo pipefail

COVERAGE_TABLE_NAME="Coverage Table Library"
COVERAGE_TABLE_VERSION="4.1.0"
export COVERAGE_TABLE_NAME COVERAGE_TABLE_VERSION

# Test Configuration
TEST_NAME="Coverage Table"
TEST_ABBR="CVT"
TEST_NUMBER="CT"
TEST_COUNTER=0
TEST_VERSION="4.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/framework.sh"
# shellcheck disable=SC2310 # We want to continue even if the test fails
#source "$(dirname "${BASH_SOURCE[0]}")/framework.sh"
setup_test_environment &>/dev/null || true

# Test Parameters
UNITY_COVS="${BUILD_DIR}/unity"
BLACKBOX_COVS="${BUILD_DIR}/coverage"
CACHE_DIR="${BUILD_DIR}/.cache/unity"

show_version() {
    echo "${COVERAGE_TABLE_NAME} - ${COVERAGE_TABLE_VERSION} - Displays comprehensive coverage data from Unity and blackbox tests in a formatted table" >&2
}

show_help() {
    cat >&2 << 'EOF'
Coverage Table Library - Displays comprehensive coverage data from Unity and blackbox tests in a formatted table

USAGE:
    coverage_table.sh [OPTIONS]

OPTIONS:
    -h, --help              Show this help
    -v, --version           Show version information

DESCRIPTION:
    Generates a formatted table showing Unity test coverage, blackbox test coverage, 
    and combined coverage data. The table includes color-coded flags:
    - YELLOW: Files with no coverage in either test type
    - MAGENTA: Files with >0 coverage but <50% coverage when >100 instrumented lines
    - CYAN: Files where Unity Framework tests exceed Unity lines of coverage found
    
    The script processes gcov files from both Unity and blackbox test directories
    and displays comprehensive coverage statistics.

EOF
}

# Parse command line arguments
for arg in "$@"; do
    case ${arg} in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--version)
            show_version
            exit 0
            ;;
        *)
            echo "Unknown option: ${arg}" >&2
            echo "Use --help for usage information" >&2
            exit 1
            ;;
    esac
done

# Associative arrays to store coverage data from both directories
declare -A unity_covered_lines
declare -A unity_instrumented_lines
declare -A coverage_covered_lines
declare -A coverage_instrumented_lines
declare -A all_files
declare -A unity_test_counts

# Function to count Unity tests for each source file with caching
count_unity_tests_per_source_file() {
    # Create cache directory if it doesn't exist
    mkdir -p "${CACHE_DIR}" 2>/dev/null
    local cache_file="${CACHE_DIR}/unity_test_counts_${timestamp}.cache"
    
    # Check if cache exists and is recent (within 1 hour)
    if [[ -f "${cache_file}" ]] && [[ $(($("${DATE}" +%s || true) - $("${STAT}" -c %Y "${cache_file}" 2>/dev/null || echo 0 || true))) -lt 3600 ]]; then
        # Load from cache
        while IFS='=' read -r source_file test_count; do
            unity_test_counts["${source_file}"]="${test_count}"
        done < "${cache_file}"
        return
    fi
    
    # Clear the cache and rebuild
    : > "${cache_file}"
    
    # Find all Unity test files - use correct path relative to hydrogen root
    local unity_test_dir="./tests/unity/src"
    if [[ ! -d "${unity_test_dir}" ]]; then
        return
    fi
    
    # Process each Unity test file
    while IFS= read -r -d '' test_file; do
        # Skip if not a .c file
        [[ "${test_file}" =~ \.c$ ]] || continue
        
        # Get relative path from tests/unity/src/
        # shellcheck disable=SC2295 # Seems to work, might not want to change it
        local rel_path="${test_file#${unity_test_dir}/}"
        
        # Extract source file name from test file name
        local test_basename
        test_basename=$(basename "${rel_path}" .c)
        
        # Map test file to source file based on naming convention
        local source_file=""
        
        if [[ "${test_basename}" =~ _test ]]; then
            local source_name="${test_basename%%_test*}"
            local source_dir
            source_dir=$("${DIRNAME}" "${rel_path}")
            
            # Handle directory mapping
            if [[ "${source_dir}" == "." ]]; then
                # Test file is in root of unity/src, map to src/
                source_file="src/${source_name}.c"
            else
                # Test file is in subdirectory, map to corresponding src subdirectory
                source_file="src/${source_dir}/${source_name}.c"
            fi
            
            # Count RUN_TEST occurrences in the test file
            local test_count
            test_count=$("${GREP}" -c "RUN_TEST(" "${test_file:-}" 2>/dev/null || true)

            local ignore_count
            ignore_count=$("${GREP}" -Ec "\if \(0\) RUN_TEST\(" "${test_file}" 2>/dev/null || true)
                        
            # Add to existing count for this source file
            local current_count="${unity_test_counts[${source_file}]:-0}"
            unity_test_counts["${source_file}"]=$((current_count + test_count - ignore_count))
            
            # Cache the result
            echo "${source_file}=${unity_test_counts[${source_file}]}" >> "${cache_file}"
        fi
    done < <("${FIND}" "${unity_test_dir}" -name "*_test*.c" -type f -print0 2>/dev/null || true)
}

# Use the exact same functions as test 99 to get coverage data
timestamp=$("${DATE}" '+%Y%m%d_%H%M%S')

# Generate display timestamp for footer
display_timestamp=$("${DATE}" '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null)

# print_subtest "Getting started"

# Call the same functions that test 99 uses - this ensures we get the same results
# shellcheck disable=SC2034 # declared globally elsewhere
# unity_coverage_percentage=$(calculate_unity_coverage "${UNITY_COVS}" "${timestamp}" 2>/dev/null || echo "0.000") ) &
# shellcheck disable=SC2034 # declared globally elsewhere
# blackbox_coverage_percentage=$(calculate_blackbox_coverage "${BLACKBOX_COVS}" "${timestamp}" 2>/dev/null || echo "0.000")

tmp_unity=$(mktemp)
tmp_blackbox=$(mktemp)
# shellcheck disable=SC2310 # We want to continue even if the test fails
( calculate_unity_coverage "${UNITY_COVS}" "${timestamp}" > "${tmp_unity}" 2>/dev/null || echo "0.000" > "${tmp_unity}" ) &
# shellcheck disable=SC2310 # We want to continue even if the test fails
( calculate_blackbox_coverage "${BLACKBOX_COVS}" "${timestamp}" > "${tmp_blackbox}" 2>/dev/null || echo "0.000" > "${tmp_blackbox}" ) &
wait
unity_coverage_percentage=$(cat "${tmp_unity}")
blackbox_coverage_percentage=$(cat "${tmp_blackbox}")
export unity_coverage_percentage blackbox_coverage_percentage
rm -f "${tmp_unity}" "${tmp_blackbox}"

# Count Unity tests (run in main shell to preserve associative array)
count_unity_tests_per_source_file

# print_subtest "Calculations done"

# Read the detailed coverage data that was just generated
unity_file_count=0
coverage_file_count=0

if [[ -f "${UNITY_COVERAGE_FILE}.detailed" ]]; then
    # shellcheck disable=SC2034 # That's not what we are doing here
    IFS=',' read -r _ _ _ _ unity_instrumented_files unity_covered_files < "${UNITY_COVERAGE_FILE}.detailed"
    unity_file_count=${unity_covered_files}
fi

if [[ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]]; then
    # shellcheck disable=SC2034 # That's not what we are doing here
    IFS=',' read -r _ _ _ _ coverage_instrumented_files coverage_covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
    coverage_file_count=${coverage_covered_files}
fi

gcov_files_found=$((unity_file_count + coverage_file_count))

# print_subtest "gcov files found"

if [[ "${gcov_files_found}" -eq 0 ]]; then
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
    echo "${combined_total_pct}" > "${COMBINED_COVERAGE_FILE}"
    
    # Create temporary directory for JSON files
    temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
    layout_json="${temp_dir}/coverage_layout.json"
    data_json="${temp_dir}/coverage_data.json"
    
    # Create layout JSON for empty table
    cat > "${layout_json}" << EOF
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
    cat > "${data_json}" << EOF
[
    {
        "status": "No source file coverage data available. Run Test 10 (Unity) or other Tests to generate gcov data"
    }
]
EOF

    # Use tables to render the empty table
    "${TABLES}" "${layout_json}" "${data_json}" 2>/dev/null || {
        echo "Error: Failed to run tables executable"
        exit 1
    }
    
    # Clean up temporary files
    rm -rf "${temp_dir}" 2>/dev/null
    exit 0
fi

# Clear our arrays and repopulate them using the working logic
declare -A unity_covered_lines
declare -A unity_instrumented_lines
declare -A coverage_covered_lines
declare -A coverage_instrumented_lines
declare -A combined_covered_lines
declare -A combined_instrumented_lines

# print_subtest "starting analyze"

# Process all coverage types using optimized batch processing
analyze_all_gcov_coverage_batch "${UNITY_COVS}" "${BLACKBOX_COVS}"

# print_subtest "analyze completee"

# Populate all_files array from coverage arrays with test file filtering
for file_path in "${!unity_covered_lines[@]}"; do
    # Skip test files from table display
    # basename_file=$(basename "${file_path}")
    # if [[ "${basename_file}" == "test_"* ]] || \
    #    [[ "${basename_file}" == *"_test.c" ]] || \
    #    [[ "${basename_file}" == *"test.c" ]] || \
    #    [[ "${file_path}" == *"test"* ]]; then
    #     continue
    # fi
    all_files["${file_path}"]=1
done
for file_path in "${!coverage_covered_lines[@]}"; do
    # Skip test files from table display
    # basename_file=$(basename "${file_path}")
    # if [[ "${basename_file}" == "test_"* ]] || \
    #    [[ "${basename_file}" == *"_test.c" ]] || \
    #    [[ "${basename_file}" == *"test.c" ]] || \
    #    [[ "${file_path}" == *"test"* ]]; then
    #     continue
    # fi
    all_files["${file_path}"]=1
done

# print_subtest "array populated from coverage arrays"

# Calculate totals for summary first
unity_total_covered=0
unity_total_instrumented=0
coverage_total_covered=0
coverage_total_instrumented=0
combined_total_covered=0
combined_total_instrumented=0

for file_path in "${!all_files[@]}"; do
    # Get Unity data
    u_covered=${unity_covered_lines["${file_path}"]:-0}
    u_instrumented=${unity_instrumented_lines["${file_path}"]:-0}
    
    # Get Coverage data
    c_covered=${coverage_covered_lines["${file_path}"]:-0}
    c_instrumented=${coverage_instrumented_lines["${file_path}"]:-0}
    
    # Get Combined data
    combined_covered=${combined_covered_lines["${file_path}"]:-0}
    combined_instrumented=${combined_instrumented_lines["${file_path}"]:-0}
    
    # Add to totals
    unity_total_covered=$((unity_total_covered + u_covered))
    unity_total_instrumented=$((unity_total_instrumented + u_instrumented))
    coverage_total_covered=$((coverage_total_covered + c_covered))
    coverage_total_instrumented=$((coverage_total_instrumented + c_instrumented))
    combined_total_covered=$((combined_total_covered + combined_covered))
    combined_total_instrumented=$((combined_total_instrumented + combined_instrumented))
done

# print_subtest "totals calculated"

# Calculate total percentages
unity_total_pct="0.000"
if [[ "${unity_total_instrumented}" -gt 0 ]]; then
    unity_total_pct=$("${AWK}" "BEGIN {printf \"%.3f\", (${unity_total_covered} / ${unity_total_instrumented}) * 100}")
fi

coverage_total_pct="0.000"
if [[ "${coverage_total_instrumented}" -gt 0 ]]; then
    coverage_total_pct=$("${AWK}" "BEGIN {printf \"%.3f\", (${coverage_total_covered} / ${coverage_total_instrumented}) * 100}")
fi

combined_total_pct="0.000"
if [[ "${combined_total_instrumented}" -gt 0 ]]; then
    combined_total_pct=$("${AWK}" "BEGIN {printf \"%.3f\", (${combined_total_covered} / ${combined_total_instrumented}) * 100}")
fi

# Store the combined coverage value for other scripts to use
echo "${combined_total_pct}" > "${COMBINED_COVERAGE_FILE}"

# print_subtest "percentages calculated"

# Create temporary directory for JSON files
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
layout_json="${temp_dir}/coverage_layout.json"
data_json="${temp_dir}/coverage_data.json"

# Start the data JSON array
echo '[' > "${data_json}"
first_record=true

# Create array for sorting by folder and file
declare -a file_data=()
zero_coverage_count=0
low_coverage_count=0

for file_path in "${!all_files[@]}"; do
    # Extract folder name from file path using first two levels below src/
    folder="src/"
    if [[ "${file_path}" == src/* ]]; then
        # Get the path after src/
        path_after_src="${file_path#src/}"
        if [[ "${path_after_src}" == */* ]]; then
            # Has at least one subdirectory
            first_level="${path_after_src%%/*}"
            remaining_path="${path_after_src#*/}"
            
            if [[ "${remaining_path}" == */* ]]; then
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
    file_data+=("${folder}:${file_path}")
done

# print_subtest "folder created"

# Sort by folder, then by filename, ensuring src/hydrogen.c appears first
# shellcheck disable=SC2312 # This is fine as-is, we want to sort by folder and file name
mapfile -t sorted_file_data < <("${PRINTF}" '%s\n' "${file_data[@]}" | sort -t: -k1,1 -k2,2)

# print_subtest "folder sorted"

# Before the loop: Initialize variables
data_records=""
first_record=true
zero_coverage_count=0
low_coverage_count=0
cyan_coverage_count=0

# Generate JSON data for tables
for file_data_entry in "${sorted_file_data[@]}"; do
    folder="${file_data_entry%%:*}"
    file_path="${file_data_entry#*:}"
    
    # Skip empty entries
    if [[ -z "${file_path}" ]]; then
        continue
    fi
    
    # Get all data
    u_covered=${unity_covered_lines["${file_path}"]:-0}
    u_instrumented=${unity_instrumented_lines["${file_path}"]:-0}
    c_covered=${coverage_covered_lines["${file_path}"]:-0}
    c_instrumented=${coverage_instrumented_lines["${file_path}"]:-0}
    combined_covered=${combined_covered_lines["${file_path}"]:-0}
    combined_instrumented=${combined_instrumented_lines["${file_path}"]:-0}

    # Get Unity test count for this source file
    test_count="${unity_test_counts[${file_path}]:-0}"
    
    # Compute all percentages and checks in one awk call
    # shellcheck disable=SC2312 # Not gonna touch this one
    IFS=' ' read -r u_percentage c_percentage combined_percentage max_percentage coverage_below_50 focus_value <<< "$(
        "${AWK}" -v u_c="${u_covered}" -v u_i="${u_instrumented}" \
            -v c_c="${c_covered}" -v c_i="${c_instrumented}" \
            -v comb_c="${combined_covered}" -v comb_i="${combined_instrumented}" '
        BEGIN {
            u_pct = (u_i > 0) ? (u_c / u_i * 100) : 0;
            c_pct = (c_i > 0) ? (c_c / c_i * 100) : 0;
            comb_pct = (comb_i > 0) ? (comb_c / comb_i * 100) : 0;
            max_pct = (u_pct > c_pct) ? u_pct : c_pct;
            below_50 = (comb_c > 0 && comb_i > 100 && comb_pct < 50.0) ? 1 : 0;

            # Focus calculation using combined coverage and instrumented lines
            coverage = comb_pct;
            lines = comb_i;

            # coverage_gap = 100 - coverage (handle None as 0)
            coverage_gap = 100 - coverage;

            # size_weight = min(log10(lines), 3) if lines > 0 else 0
            size_weight = (lines > 0) ? ((lines <= 1) ? 0 : ((log(lines)/log(10) > 3) ? 3 : log(lines)/log(10))) : 0;

            # coverage_penalty = 1.0 if coverage < 80 else 0.5 if coverage <= 90 else 0.25
            coverage_penalty = (coverage < 80) ? 1.0 : ((coverage <= 90) ? 0.5 : 0.25);

            # small_file_penalty = 1.0 if lines >= 50 or (coverage is not None and coverage < 20) else 0.2
            small_file_penalty = ((lines >= 50 || (coverage < 20 && comb_i > 0)) ? 1.0 : 0.2);

            # Calculate focus value
            focus = coverage_gap * size_weight * coverage_penalty * small_file_penalty;

            # Set focus to 0 if basic requirements are met
            if (comb_pct >= 50 && lines <= 100) focus = 0;
            if (comb_pct >= 75 && lines >= 100) focus = 0;

            printf "%.3f %.3f %.3f %.3f %d %.1f\n", u_pct, c_pct, comb_pct, max_pct, below_50, focus;
        }')"
    
    # Remove src/ prefix from display path and highlight files
    display_file_path="${file_path#src/}"
    if [[ ${u_covered} -eq 0 && ${c_covered} -eq 0 ]]; then
        display_file_path="{YELLOW}${display_file_path}{RESET}"
        zero_coverage_count=$(( zero_coverage_count + 1 ))
    elif [[ ${coverage_below_50} -eq 1 ]]; then
        display_file_path="{MAGENTA}${display_file_path}{RESET}"
        low_coverage_count=$(( low_coverage_count + 1 ))
    elif [[ ${test_count} -gt ${u_covered} ]]; then
        display_file_path="{CYAN}${display_file_path}{RESET}"
        cyan_coverage_count=$(( cyan_coverage_count + 1 ))
    fi

    # Build JSON record in memory
    if [[ "${first_record}" = false ]]; then
        data_records+=","
    else
        first_record=false
    fi
    data_records+=$'\n    {
        "folder": "'"${folder}"'",
        "max_coverage_percentage": '"${max_percentage}"',
        "combined_coverage_percentage": '"${combined_percentage}"',
        "file_path": "'"${display_file_path}"'",
        "unity_test_count": '"${test_count}"',
        "unity_covered": '"${u_covered}"',
        "unity_instrumented": '"${u_instrumented}"',
        "unity_percentage": '"${u_percentage}"',
        "coverage_covered": '"${c_covered}"',
        "coverage_instrumented": '"${c_instrumented}"',
        "coverage_percentage": '"${c_percentage}"',
        "combined_covered": '"${combined_covered}"',
        "combined_instrumented": '"${combined_instrumented}"',
        "focus": '"${focus_value}"'
    }'
done

# After the loop: Write JSON once
{
    echo '['
    echo -n "${data_records}"
    echo $'\n]'
} > "${data_json}"

# print_subtest "JSON generated"

# # Close the JSON array
# echo '' >> "${data_json}"
# echo ']' >> "${data_json}"

# Create layout JSON for the coverage table (now with correct totals and counts)
cat > "${layout_json}" << EOF
{
    "title": "{BOLD}{WHITE}Test Suite Coverage {RED}——— {BOLD}{CYAN}Unity{WHITE} ${unity_total_pct}% {RED}——— {BOLD}{CYAN}Blackbox{WHITE} ${coverage_total_pct}% {RED}——— {BOLD}{CYAN}Combined{WHITE} ${combined_total_pct}%{RESET}",
    "footer": "{YELLOW}Zero Cover{WHITE} ${zero_coverage_count} {RED}——— {CYAN}Tests > Cover{WHITE} ${cyan_coverage_count} {RED}——— {MAGENTA}100+ < 50% Cover{WHITE} ${low_coverage_count} {RED}——— {CYAN}Generated{WHITE} ${display_timestamp}{RESET}",
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
            "summary": "nonblanks"
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
            "width": 44
        },
        {
            "header": "Tests",
            "key": "unity_test_count",
            "datatype": "int",
            "justification": "right",
            "summary": "sum",
            "width": 7
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
        },
        {
            "header": "Focus",
            "key": "focus",
            "datatype": "float",
            "justification": "right",
            "summary": "",
            "width": 7
        }
    ]
}
EOF

# print_subtest "Layout generated"

# Use tables executable to render the table
"${TABLES}" "${layout_json}" "${data_json}" 2>/dev/null

# Save JSON data to results directory for capture by test_00_all.sh
# shellcheck disable=SC2154 # RESULTS_DIR defined externally in framework.sh
if [[ -f "${data_json}" ]]; then
    cp "${data_json}" "${RESULTS_DIR}/coverage_data.json"
fi

# Clean up temporary files
rm -rf "${temp_dir}" 2>/dev/null || true

# print_subtest "Script complete"
