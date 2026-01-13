#!/usr/bin/env bash

# Discrepancy Analysis Library
# Compares coverage line counts between Test 89 (Unity coverage) and coverage_table.sh
# to identify files with mismatched coverage counts

# CHANGELOG
# 2.0.0 - 2026-01-13 - Added GLOBAL concat method (what Test 89 ACTUALLY uses), stored file comparison
# 1.0.0 - 2026-01-12 - Initial version for identifying coverage discrepancies

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

set -euo pipefail

DISCREPANCY_NAME="Discrepancy Analysis Library"
DISCREPANCY_VERSION="2.0.0"
export DISCREPANCY_NAME DISCREPANCY_VERSION

# Test Configuration - used by framework.sh
# shellcheck disable=SC2034 # These variables are used by framework.sh for test identification
TEST_NAME="Coverage Discrepancy Analysis"
# shellcheck disable=SC2034 # Used by framework.sh for test abbreviation display
TEST_ABBR="DIS"
# shellcheck disable=SC2034 # Used by framework.sh for test numbering system
TEST_NUMBER="DI"
# shellcheck disable=SC2034 # Used by framework.sh for subtest counting
TEST_COUNTER=0
# shellcheck disable=SC2034 # Used by framework.sh for version tracking
TEST_VERSION="2.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "${HYDROGEN_ROOT}/tests/lib/framework.sh"
# shellcheck disable=SC2310 # setup_test_environment return value is not relevant; we continue regardless
setup_test_environment &>/dev/null || true

# Test Parameters - shellcheck disable for variables set by framework
# shellcheck disable=SC2154 # BUILD_DIR is set by framework.sh during environment setup
UNITY_COVS="${BUILD_DIR}/unity"
# shellcheck disable=SC2154 # BUILD_DIR is set by framework.sh during environment setup
BLACKBOX_COVS="${BUILD_DIR}/coverage"
# shellcheck disable=SC2034,SC2154 # RESULTS_DIR used by other functions; BUILD_DIR set by framework.sh
RESULTS_DIR="${BUILD_DIR}/tests/results"

show_version() {
    echo "${DISCREPANCY_NAME} - ${DISCREPANCY_VERSION} - Compares coverage data between calculation methods" >&2
}

show_help() {
    cat >&2 << 'EOF'
Discrepancy Analysis Library - Identifies mismatches between coverage calculation methods

USAGE:
    discrepancy.sh [OPTIONS]

OPTIONS:
    -h, --help              Show this help
    -v, --version           Show version information
    --unity-only            Show only Unity test discrepancies
    --blackbox-only         Show only Blackbox test discrepancies
    --summary               Show summary totals only, skip per-file details

DESCRIPTION:
    Compares coverage line counts from two different calculation methods:
    1. Test 89 method: Uses calculate_unity_coverage() / calculate_blackbox_coverage()
       which processes gcov files with specific filtering and DISCREPANCY adjustments
    2. coverage_table.sh method: Uses analyze_all_gcov_coverage_batch()
       which processes gcov files for the coverage table display

    The script shows side-by-side comparisons per source file to identify
    where counting differences occur. This helps tune DISCREPANCY_UNITY
    and DISCREPANCY_COVERAGE values in coverage.sh.

EOF
}

# Parse command line arguments
UNITY_ONLY=false
BLACKBOX_ONLY=false
SUMMARY_ONLY=false

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
        --unity-only)
            UNITY_ONLY=true
            ;;
        --blackbox-only)
            BLACKBOX_ONLY=true
            ;;
        --summary)
            SUMMARY_ONLY=true
            ;;
        *)
            echo "Unknown option: ${arg}" >&2
            echo "Use --help for usage information" >&2
            exit 1
            ;;
    esac
done

# Associative arrays for Method 1 (Test 89 style - coverage.sh calculate_coverage_generic)
declare -A method1_unity_covered
declare -A method1_unity_instrumented
declare -A method1_blackbox_covered
declare -A method1_blackbox_instrumented

# Associative arrays for Method 2 (coverage_table.sh style - analyze_all_gcov_coverage_batch)
declare -A method2_unity_covered
declare -A method2_unity_instrumented
declare -A method2_blackbox_covered
declare -A method2_blackbox_instrumented

# Union of all files
declare -A all_files

# Function to process gcov files using Method 1 (Test 89 style)
# This mimics calculate_coverage_generic from coverage.sh
# KEY DIFFERENCE: This concatenates ALL gcov files and counts lines globally
# shellcheck disable=SC2154 # AWK, GREP, FIND are set by framework.sh
process_method1() {
    local dir="$1"
    local coverage_type="$2"  # "unity" or "blackbox"
    
    if [[ ! -d "${dir}" ]]; then
        return 1
    fi
    
    local project_root="${HYDROGEN_ROOT}"
    
    # Load ignore patterns
    local -a IGNORE_PATTERNS=()
    if [[ -f "${project_root}/.trial-ignore" ]]; then
        while IFS= read -r line; do
            [[ "${line}" =~ ^[[:space:]]*# ]] && continue
            [[ -z "${line}" ]] && continue
            local pattern="${line#./}"
            IGNORE_PATTERNS+=("${pattern}")
        done < "${project_root}/.trial-ignore"
    fi
    
    # Build list of gcov files to process (same filtering as calculate_coverage_generic)
    local -a gcov_files_to_process=()
    
    while IFS= read -r gcov_file; do
        if [[ -f "${gcov_file}" ]]; then
            local basename_file
            basename_file=$(basename "${gcov_file}")
            
            # Common skips (matching calculate_coverage_generic)
            [[ "${basename_file}" == "unity.c.gcov" ]] && continue
            [[ "${gcov_file}" == *"/usr/"* ]] && continue
            
            # Skip system libraries and external dependencies
            # Note: We no longer skip "crypto" as we have our own utils_crypto.c
            [[ "${basename_file}" == *"jansson"* ]] && continue
            [[ "${basename_file}" == *"mock"* ]] && continue
            [[ "${basename_file}" == *"json"* ]] && continue
            [[ "${basename_file}" == *"curl"* ]] && continue
            [[ "${basename_file}" == *"ssl"* ]] && continue
            [[ "${basename_file}" == *"pthread"* ]] && continue
            [[ "${basename_file}" == *"uuid"* ]] && continue
            [[ "${basename_file}" == *"zlib"* ]] && continue
            [[ "${basename_file}" == *"pcre"* ]] && continue
            
            # Skip system include files (extra_filters=true for unity)
            if [[ "${coverage_type}" == "unity" ]]; then
                if "${GREP}" -q "Source:/usr/include/" "${gcov_file}" 2>/dev/null; then
                    continue
                fi
                [[ "${basename_file}" == "test_"* ]] && continue
            fi
            
            # Check ignore patterns
            local source_file="${basename_file%.gcov}"
            local should_ignore=false
            for pattern in "${IGNORE_PATTERNS[@]}"; do
                if [[ "${source_file}" == *"${pattern}"* ]]; then
                    should_ignore=true
                    break
                fi
            done
            [[ "${should_ignore}" == true ]] && continue
            
            gcov_files_to_process+=("${gcov_file}")
            
            # Also store per-file data for comparison
            local source_line
            source_line=$("${GREP}" '^        -:    0:Source:' "${gcov_file}" 2>/dev/null | cut -d':' -f3- || true)
            local display_path
            if [[ -n "${source_line}" ]]; then
                display_path="${source_line#*/hydrogen/}"
            else
                display_path=$(basename "${gcov_file}" .gcov)
                display_path="src/${display_path}"
            fi
            
            # Count lines per file
            local line_counts
            line_counts=$("${AWK}" '
                BEGIN {total=0; covered=0}
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
                /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
                END { print total "," covered }
            ' "${gcov_file}" 2>/dev/null)
            
            local instrumented="${line_counts%,*}"
            local covered="${line_counts#*,}"
            instrumented=${instrumented:-0}
            covered=${covered:-0}
            
            all_files["${display_path}"]=1
            if [[ "${coverage_type}" == "unity" ]]; then
                method1_unity_instrumented["${display_path}"]=${instrumented}
                method1_unity_covered["${display_path}"]=${covered}
            else
                method1_blackbox_instrumented["${display_path}"]=${instrumented}
                method1_blackbox_covered["${display_path}"]=${covered}
            fi
        fi
    done < <("${FIND}" "${dir}" -name "*.c.gcov" -type f 2>/dev/null | "${GREP}" -v '_test' || true)
    
    # NOW the key part: concat all files and count globally (like calculate_coverage_generic)
    if [[ ${#gcov_files_to_process[@]} -gt 0 ]]; then
        local combined_gcov="/tmp/discrepancy_combined_$$.gcov"
        cat "${gcov_files_to_process[@]}" > "${combined_gcov}" 2>/dev/null || true
        
        local global_counts
        global_counts=$("${AWK}" '
            BEGIN {total=0; covered=0}
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
            END { print total "," covered }
        ' "${combined_gcov}" 2>/dev/null)
        
        rm -f "${combined_gcov}" 2>/dev/null
        
        local global_instrumented="${global_counts%,*}"
        local global_covered="${global_counts#*,}"
        
        # Store the GLOBAL totals in special keys for comparison
        if [[ "${coverage_type}" == "unity" ]]; then
            method1_unity_global_instrumented="${global_instrumented:-0}"
            method1_unity_global_covered="${global_covered:-0}"
        else
            method1_blackbox_global_instrumented="${global_instrumented:-0}"
            method1_blackbox_global_covered="${global_covered:-0}"
        fi
    fi
}

# Initialize global totals for method 1
method1_unity_global_instrumented=0
method1_unity_global_covered=0
method1_blackbox_global_instrumented=0
method1_blackbox_global_covered=0

# Function to process gcov files using Method 2 (coverage_table.sh style)
# This uses the same batch processing as coverage_table.sh
process_method2() {
    local unity_dir="$1"
    local blackbox_dir="$2"
    
    local project_root="${HYDROGEN_ROOT}"
    
    # Load ignore patterns
    local -a IGNORE_PATTERNS=()
    IGNORE_PATTERNS+=("mock_")
    IGNORE_PATTERNS+=("jansson")
    IGNORE_PATTERNS+=("microhttpd")
    IGNORE_PATTERNS+=("src/unity.c")
    
    if [[ -f "${project_root}/.trial-ignore" ]]; then
        local lines=()
        mapfile -t lines < "${project_root}/.trial-ignore"
        for line in "${lines[@]}"; do
            [[ "${line}" =~ ^[[:space:]]*# ]] && continue
            [[ -z "${line// }" ]] && continue
            local pattern="${line#./}"
            [[ -n "${pattern}" ]] && IGNORE_PATTERNS+=("${pattern}")
        done
    fi
    
    # should_ignore helper
    should_ignore_file() {
        local file_path="$1"
        for pattern in "${IGNORE_PATTERNS[@]}"; do
            if [[ "${file_path}" == *"${pattern}"* ]]; then
                return 0
            fi
        done
        return 1
    }
    
    # Get all .gcov files from both directories
    local unity_files=()
    local blackbox_files=()
    if [[ -d "${unity_dir}" ]]; then
        mapfile -t unity_files < <("${FIND}" "${unity_dir}" -name "*.c.gcov" -type f 2>/dev/null | "${GREP}" -v '_test' || true)
    fi
    if [[ -d "${blackbox_dir}" ]]; then
        mapfile -t blackbox_files < <("${FIND}" "${blackbox_dir}" -name "*.c.gcov" -type f 2>/dev/null | "${GREP}" -v '_test' || true)
    fi
    
    # Create union of all files by relative path  
    declare -A file_set
    for file in "${unity_files[@]}" "${blackbox_files[@]}"; do
        [[ -z "${file}" ]] && continue
        local base_dir rel_path source_path
        if [[ "${file}" == "${unity_dir}"/* ]]; then
            base_dir="${unity_dir}"
        else
            base_dir="${blackbox_dir}"
        fi
        rel_path="${file#"${base_dir}/"}"
        source_path="${rel_path%.gcov}"
        if [[ "${source_path}" != src/* ]]; then
            source_path="src/${source_path}"
        fi
        # shellcheck disable=SC2310 # should_ignore_file return value checked directly; masking not an issue
        if ! should_ignore_file "${source_path}"; then
            file_set["${rel_path}"]=1
        fi
    done
    
    # Process each file
    for file_relpath in "${!file_set[@]}"; do
        local unity_file="${unity_dir}/${file_relpath}"
        local blackbox_file="${blackbox_dir}/${file_relpath}"
        
        # Determine display path
        local source_path="${file_relpath%.gcov}"
        if [[ "${source_path}" != src/* ]]; then
            source_path="src/${source_path}"
        fi
        
        all_files["${source_path}"]=1
        
        # Process Unity file
        if [[ -f "${unity_file}" ]]; then
            local line_counts
            line_counts=$("${AWK}" '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
                /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
                END { 
                    if (total == "") total = 0;
                    if (covered == "") covered = 0;
                    print total "," covered 
                }
            ' "${unity_file}" 2>/dev/null)
            method2_unity_instrumented["${source_path}"]="${line_counts%,*}"
            method2_unity_covered["${source_path}"]="${line_counts#*,}"
        fi
        
        # Process Blackbox file
        if [[ -f "${blackbox_file}" ]]; then
            local line_counts
            line_counts=$("${AWK}" '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
                /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
                END { 
                    if (total == "") total = 0;
                    if (covered == "") covered = 0;
                    print total "," covered 
                }
            ' "${blackbox_file}" 2>/dev/null)
            method2_blackbox_instrumented["${source_path}"]="${line_counts%,*}"
            method2_blackbox_covered["${source_path}"]="${line_counts#*,}"
        fi
    done
}

echo "╭─────────────────────────────────────────────────────────────────────────────╮"
echo "│  Coverage Discrepancy Analysis - Comparing calculation methods              │"
echo "╰─────────────────────────────────────────────────────────────────────────────╯"
echo ""
echo "Processing coverage data..."

# Process both methods
if [[ "${BLACKBOX_ONLY}" != true ]]; then
    echo "  Method 1 (Test 89 style): Unity coverage..."
    process_method1 "${UNITY_COVS}/src" "unity"
fi

if [[ "${UNITY_ONLY}" != true ]]; then
    echo "  Method 1 (Test 89 style): Blackbox coverage..."
    process_method1 "${BLACKBOX_COVS}/src" "blackbox"
fi

echo "  Method 2 (coverage_table.sh style): Batch processing..."
process_method2 "${UNITY_COVS}/src" "${BLACKBOX_COVS}/src"

echo ""
echo "Analysis complete. Generating comparison..."
echo ""

# Calculate totals
m1_unity_total_inst=0
m1_unity_total_cov=0
m2_unity_total_inst=0
m2_unity_total_cov=0
m1_bb_total_inst=0
m1_bb_total_cov=0
m2_bb_total_inst=0
m2_bb_total_cov=0

# Sort files for consistent output
# shellcheck disable=SC2312 # sort return value not relevant; we just need sorted output
sorted_files=()
while IFS= read -r file; do
    sorted_files+=("${file}")
done < <(printf '%s\n' "${!all_files[@]}" | sort || true)

# Count discrepancies
unity_discrepancy_count=0
blackbox_discrepancy_count=0

# Generate output
if [[ "${SUMMARY_ONLY}" != true ]]; then
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo "Per-File Comparison (files with differences only)"
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo ""
    printf "%-45s │ %8s │ %8s │ %8s │ %8s │ %8s\n" \
        "Source File" "M1-Unity" "M2-Unity" "Diff" "M1-BB" "M2-BB"
    printf "%-45s─┼──────────┼──────────┼──────────┼──────────┼──────────\n" \
        "─────────────────────────────────────────────"
fi

for file_path in "${sorted_files[@]}"; do
    # Skip empty entries
    [[ -z "${file_path}" ]] && continue
    
    # Get values (default to 0) and ensure they're numeric
    m1_u_inst="${method1_unity_instrumented[${file_path}]:-0}"
    m1_u_cov="${method1_unity_covered[${file_path}]:-0}"
    m2_u_inst="${method2_unity_instrumented[${file_path}]:-0}"
    m2_u_cov="${method2_unity_covered[${file_path}]:-0}"
    m1_b_inst="${method1_blackbox_instrumented[${file_path}]:-0}"
    m1_b_cov="${method1_blackbox_covered[${file_path}]:-0}"
    m2_b_inst="${method2_blackbox_instrumented[${file_path}]:-0}"
    m2_b_cov="${method2_blackbox_covered[${file_path}]:-0}"
    
    # Ensure all values are numeric (handle empty strings)
    [[ -z "${m1_u_inst}" || ! "${m1_u_inst}" =~ ^[0-9]+$ ]] && m1_u_inst=0
    [[ -z "${m1_u_cov}" || ! "${m1_u_cov}" =~ ^[0-9]+$ ]] && m1_u_cov=0
    [[ -z "${m2_u_inst}" || ! "${m2_u_inst}" =~ ^[0-9]+$ ]] && m2_u_inst=0
    [[ -z "${m2_u_cov}" || ! "${m2_u_cov}" =~ ^[0-9]+$ ]] && m2_u_cov=0
    [[ -z "${m1_b_inst}" || ! "${m1_b_inst}" =~ ^[0-9]+$ ]] && m1_b_inst=0
    [[ -z "${m1_b_cov}" || ! "${m1_b_cov}" =~ ^[0-9]+$ ]] && m1_b_cov=0
    [[ -z "${m2_b_inst}" || ! "${m2_b_inst}" =~ ^[0-9]+$ ]] && m2_b_inst=0
    [[ -z "${m2_b_cov}" || ! "${m2_b_cov}" =~ ^[0-9]+$ ]] && m2_b_cov=0
    
    # Add to totals
    m1_unity_total_inst=$((m1_unity_total_inst + m1_u_inst))
    m1_unity_total_cov=$((m1_unity_total_cov + m1_u_cov))
    m2_unity_total_inst=$((m2_unity_total_inst + m2_u_inst))
    m2_unity_total_cov=$((m2_unity_total_cov + m2_u_cov))
    m1_bb_total_inst=$((m1_bb_total_inst + m1_b_inst))
    m1_bb_total_cov=$((m1_bb_total_cov + m1_b_cov))
    m2_bb_total_inst=$((m2_bb_total_inst + m2_b_inst))
    m2_bb_total_cov=$((m2_bb_total_cov + m2_b_cov))
    
    # Calculate differences
    unity_diff=$((m1_u_cov - m2_u_cov))
    bb_diff=$((m1_b_cov - m2_b_cov))
    
    # Check for discrepancies
    has_unity_diff=false
    has_bb_diff=false
    if [[ "${unity_diff}" -ne 0 ]]; then
        has_unity_diff=true
        unity_discrepancy_count=$((unity_discrepancy_count + 1))
    fi
    if [[ "${bb_diff}" -ne 0 ]]; then
        has_bb_diff=true
        blackbox_discrepancy_count=$((blackbox_discrepancy_count + 1))
    fi
    
    # Only show files with differences (unless showing all)
    if [[ "${SUMMARY_ONLY}" != true ]] && { [[ "${has_unity_diff}" == true ]] || [[ "${has_bb_diff}" == true ]]; }; then
        # Truncate file path if too long
        display_name="${file_path}"
        if [[ ${#display_name} -gt 44 ]]; then
            display_name="...${display_name: -41}"
        fi
        
        unity_diff_str="${unity_diff}"
        bb_diff_str="${bb_diff}"
        [[ "${unity_diff}" -gt 0 ]] && unity_diff_str="+${unity_diff}"
        [[ "${bb_diff}" -gt 0 ]] && bb_diff_str="+${bb_diff}"
        
        printf "%-45s │ %8s │ %8s │ %8s │ %8s │ %8s\n" \
            "${display_name}" "${m1_u_cov}" "${m2_u_cov}" "${unity_diff_str}" "${m1_b_cov}" "${bb_diff_str}"
    fi
done

# Show running totals comparison right after per-file output
if [[ "${SUMMARY_ONLY}" != true ]]; then
    echo ""
    echo "Running Total Verification:"
    echo "  Method 1 Unity Covered (sum):      ${m1_unity_total_cov}"
    echo "  Method 2 Unity Covered (sum):      ${m2_unity_total_cov}"
    echo "  Method 1 Unity Instrumented (sum): ${m1_unity_total_inst}"
    echo "  Method 2 Unity Instrumented (sum): ${m2_unity_total_inst}"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Summary Totals"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo ""
echo "Unity Coverage (per-file sum method):"
echo "  Method 1 (Test 89):        Instrumented: ${m1_unity_total_inst}, Covered: ${m1_unity_total_cov}"
echo "  Method 2 (coverage_table): Instrumented: ${m2_unity_total_inst}, Covered: ${m2_unity_total_cov}"
echo "  Difference:                Instrumented: $((m1_unity_total_inst - m2_unity_total_inst)), Covered: $((m1_unity_total_cov - m2_unity_total_cov))"
echo "  Files with differences:    ${unity_discrepancy_count}"
echo ""
echo "Unity Coverage (GLOBAL concat method - what Test 89 ACTUALLY uses):"
echo "  Method 1 GLOBAL:           Instrumented: ${method1_unity_global_instrumented}, Covered: ${method1_unity_global_covered}"
echo "  Method 1 Per-file sum:     Instrumented: ${m1_unity_total_inst}, Covered: ${m1_unity_total_cov}"
echo "  Concat vs Sum difference:  Instrumented: $((method1_unity_global_instrumented - m1_unity_total_inst)), Covered: $((method1_unity_global_covered - m1_unity_total_cov))"
echo ""
echo "Blackbox Coverage (per-file sum method):"
echo "  Method 1 (Test 89):        Instrumented: ${m1_bb_total_inst}, Covered: ${m1_bb_total_cov}"
echo "  Method 2 (coverage_table): Instrumented: ${m2_bb_total_inst}, Covered: ${m2_bb_total_cov}"
echo "  Difference:                Instrumented: $((m1_bb_total_inst - m2_bb_total_inst)), Covered: $((m1_bb_total_cov - m2_bb_total_cov))"
echo "  Files with differences:    ${blackbox_discrepancy_count}"
echo ""
echo "Blackbox Coverage (GLOBAL concat method - what Test 89 ACTUALLY uses):"
echo "  Method 1 GLOBAL:           Instrumented: ${method1_blackbox_global_instrumented}, Covered: ${method1_blackbox_global_covered}"
echo "  Method 1 Per-file sum:     Instrumented: ${m1_bb_total_inst}, Covered: ${m1_bb_total_cov}"
echo "  Concat vs Sum difference:  Instrumented: $((method1_blackbox_global_instrumented - m1_bb_total_inst)), Covered: $((method1_blackbox_global_covered - m1_bb_total_cov))"
echo ""

# Read ACTUAL stored values from the coverage files
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "ACTUAL stored coverage values (what Test 89 reads)"
echo "═══════════════════════════════════════════════════════════════════════════════"
unity_detailed_file="${RESULTS_DIR}/coverage_unity.txt.detailed"
blackbox_detailed_file="${RESULTS_DIR}/coverage_blackbox.txt.detailed"
echo ""
if [[ -f "${unity_detailed_file}" ]]; then
    # Format: timestamp,percentage,covered_lines,total_lines,instrumented_files,covered_files
    IFS=',' read -r ts pct covered total inst_files cov_files < "${unity_detailed_file}"
    echo "Unity (from ${unity_detailed_file}):"
    echo "  Stored covered lines:      ${covered}"
    echo "  Stored instrumented lines: ${total}"
    echo "  vs Script GLOBAL covered:  ${method1_unity_global_covered} (diff: $((covered - method1_unity_global_covered)))"
    echo "  vs Script per-file sum:    ${m1_unity_total_cov} (diff: $((covered - m1_unity_total_cov)))"
    echo "  vs Method 2 per-file sum:  ${m2_unity_total_cov} (diff: $((covered - m2_unity_total_cov)))"
else
    echo "Unity: No stored file found at ${unity_detailed_file}"
fi
echo ""
if [[ -f "${blackbox_detailed_file}" ]]; then
    # shellcheck disable=SC2034 # ts, pct, inst_files, cov_files are parsed for completeness but only covered/total used
    IFS=',' read -r ts pct covered total inst_files cov_files < "${blackbox_detailed_file}"
    echo "Blackbox (from ${blackbox_detailed_file}):"
    echo "  Stored covered lines:      ${covered}"
    echo "  Stored instrumented lines: ${total}"
    echo "  vs Script GLOBAL covered:  ${method1_blackbox_global_covered} (diff: $((covered - method1_blackbox_global_covered)))"
    echo "  vs Script per-file sum:    ${m1_bb_total_cov} (diff: $((covered - m1_bb_total_cov)))"
    echo "  vs Method 2 per-file sum:  ${m2_bb_total_cov} (diff: $((covered - m2_bb_total_cov)))"
else
    echo "Blackbox: No stored file found at ${blackbox_detailed_file}"
fi
echo ""

# Read current DISCREPANCY values from coverage.sh
discrepancy_unity=$(grep -E "^DISCREPANCY_UNITY=" "${HYDROGEN_ROOT}/tests/lib/coverage.sh" 2>/dev/null | cut -d'=' -f2 || echo "0")
discrepancy_coverage=$(grep -E "^DISCREPANCY_COVERAGE=" "${HYDROGEN_ROOT}/tests/lib/coverage.sh" 2>/dev/null | cut -d'=' -f2 || echo "0")

echo "Current DISCREPANCY values in coverage.sh:"
echo "  DISCREPANCY_UNITY=${discrepancy_unity}"
echo "  DISCREPANCY_COVERAGE=${discrepancy_coverage}"
echo ""

# Suggest new values if needed
unity_inst_diff=$((m2_unity_total_inst - m1_unity_total_inst))
bb_inst_diff=$((m2_bb_total_inst - m1_bb_total_inst))

if [[ "${unity_inst_diff}" -ne 0 ]] || [[ "${bb_inst_diff}" -ne 0 ]]; then
    echo "Suggested adjustments (based on instrumented line differences):"
    if [[ "${unity_inst_diff}" -ne 0 ]]; then
        echo "  DISCREPANCY_UNITY adjustment: ${unity_inst_diff}"
    fi
    if [[ "${bb_inst_diff}" -ne 0 ]]; then
        echo "  DISCREPANCY_COVERAGE adjustment: ${bb_inst_diff}"
    fi
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Files examined for potential filtering differences"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo ""

# Show files that only appear in one method (potential filtering issue)
echo "Files only in Method 1 (Test 89):"
m1_only_count=0
for file_path in "${sorted_files[@]}"; do
    m1_u="${method1_unity_instrumented[${file_path}]:-}"
    m1_b="${method1_blackbox_instrumented[${file_path}]:-}"
    m2_u="${method2_unity_instrumented[${file_path}]:-}"
    m2_b="${method2_blackbox_instrumented[${file_path}]:-}"
    
    if { [[ -n "${m1_u}" ]] || [[ -n "${m1_b}" ]]; } && { [[ -z "${m2_u}" ]] && [[ -z "${m2_b}" ]]; }; then
        echo "  ${file_path}"
        ((m1_only_count++)) || true
    fi
done
[[ "${m1_only_count}" -eq 0 ]] && echo "  (none)"

echo ""
echo "Files only in Method 2 (coverage_table):"
m2_only_count=0
for file_path in "${sorted_files[@]}"; do
    m1_u="${method1_unity_instrumented[${file_path}]:-}"
    m1_b="${method1_blackbox_instrumented[${file_path}]:-}"
    m2_u="${method2_unity_instrumented[${file_path}]:-}"
    m2_b="${method2_blackbox_instrumented[${file_path}]:-}"
    
    if { [[ -z "${m1_u}" ]] && [[ -z "${m1_b}" ]]; } && { [[ -n "${m2_u}" ]] || [[ -n "${m2_b}" ]]; }; then
        echo "  ${file_path}"
        ((m2_only_count++)) || true
    fi
done
[[ "${m2_only_count}" -eq 0 ]] && echo "  (none)"

echo ""
echo "Total files analyzed: ${#sorted_files[@]}"
echo "Files only in Method 1: ${m1_only_count}"
echo "Files only in Method 2: ${m2_only_count}"
echo ""
