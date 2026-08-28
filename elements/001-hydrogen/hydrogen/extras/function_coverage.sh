#!/bin/bash
# function_coverage.sh - Per-function coverage analysis for a source file.
#
# Compares gcov function-level call counts between Unity unit tests and
# Blackbox integration tests, and checks whether each function appears in
# the related Unity test source files.
#
# A function "covered by Unity" means gcov reports call count > 0 in the
# Unity build. A function "covered by Blackbox" means gcov reports call count
# > 0 in the coverage (integration) build. A function "in test source" means
# the function name appears as a whole word in any of the related Unity test
# files (those following the <source>_test* naming convention).
#
# This distinguishes functions that are exercised *incidentally* (called
# as a side effect by some other test) from functions that are tested
# *independently* (explicitly referenced in a test file).
#
# Usage: ./function_coverage.sh <relative_source_path>
#   Example: ./function_coverage.sh scripting/scripting_api_mail_repo.c
#            ./function_coverage.sh utils/utils_time.c
#            ./function_coverage.sh hydrogen.c
#
# CHANGELOG
# 1.0.0 - 2026-08-28 - Initial version

set -euo pipefail

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "Error: HYDROGEN_ROOT environment variable is not set" >&2
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory" >&2
    exit 1
fi

PROJECT_DIR="${HYDROGEN_ROOT}"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <relative_source_path>" >&2
    echo "  Example: $0 scripting/scripting_api_mail_repo.c" >&2
    exit 1
fi

src_arg="${1}"

# Strip src/ prefix if the user included it
src_rel="${src_arg#src/}"
# Strip .c extension for gcov path construction
src_noext="${src_rel%.c}"

# ---------------------------------------------------------------------------
# Path construction
# ---------------------------------------------------------------------------

UNITY_GCOV_DIR="${PROJECT_DIR}/build/unity/src"
COVERAGE_GCOV_DIR="${PROJECT_DIR}/build/coverage/src"
UNITY_GCOV="${UNITY_GCOV_DIR}/${src_noext}.c.gcov"
COVERAGE_GCOV="${COVERAGE_GCOV_DIR}/${src_noext}.c.gcov"
UNITY_GCDA="${UNITY_GCOV_DIR}/${src_noext}.gcda"
COVERAGE_GCDA="${COVERAGE_GCOV_DIR}/${src_noext}.gcda"
SRC_PATH="${PROJECT_DIR}/src/${src_rel}"

src_basename="$(basename "${src_noext}")"
src_dirname="$(dirname "${src_noext}")"

if [[ "${src_dirname}" == "." ]]; then
    TEST_SEARCH_DIR="${PROJECT_DIR}/tests/unity/src"
else
    TEST_SEARCH_DIR="${PROJECT_DIR}/tests/unity/src/${src_dirname}"
fi

ALL_TESTS_DIR="${PROJECT_DIR}/tests/unity/src"

# ---------------------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------------------

# Compute a percentage as X.X (returns 0.0 when denominator is 0)
pct() {
    local num="$1"
    local den="$2"
    if [[ "${den}" -eq 0 ]]; then
        echo "0.0"
    else
        awk "BEGIN {printf \"%.1f\", (${num} / ${den}) * 100}"
    fi
}

# Generate a .gcov file if it doesn't exist or is stale, using .gcno + .gcda files.
# Refuses to generate from .gcno alone (would be all-zero); requires .gcda.
generate_gcov_if_needed() {
    local gcov_file="$1"
    local gcov_parent
    gcov_parent="$(dirname "${gcov_file}")"
    local gcov_base
    gcov_base="$(basename "${gcov_file}" .c.gcov)"

    local gcda_file="${gcov_parent}/${gcov_base}.gcda"

    # Need .gcda for real coverage data; .gcno alone produces all-zero gcov
    if [[ ! -f "${gcda_file}" ]]; then
        return 0
    fi

    # Regenerate if gcov is missing or stale (older than gcda)
    if [[ ! -f "${gcov_file}" ]] || [[ "${gcda_file}" -nt "${gcov_file}" ]]; then
        (cd "${gcov_parent}" && gcov -b -c "${gcov_base}" >/dev/null 2>&1 || true)
    fi
}

# Parse function records from a gcov file.
# Output: func_name<TAB>call_count<TAB>source_line_number  (one per line)
parse_gcov_functions() {
    local gcov_file="$1"
    if [[ ! -f "${gcov_file}" ]]; then
        return 0
    fi
    awk '
        BEGIN { fname = ""; calls = 0; want_line = 0 }
        /^function / {
            fname = $2
            calls = 0
            for (i = 3; i <= NF; i++) {
                if ($i == "called" && (i + 1) <= NF) {
                    calls = $(i + 1) + 0
                    break
                }
            }
            want_line = 1
            next
        }
        want_line == 1 && /:/ {
            n = split($0, parts, ":")
            if (n >= 2) {
                line = parts[2]
                gsub(/^[ \t]+|[ \t]+$/, "", line)
                if (line ~ /^[0-9]+$/) {
                    print fname "\t" calls "\t" line
                } else {
                    print fname "\t" calls "\t" ""
                }
            } else {
                print fname "\t" calls "\t" ""
            }
            fname = ""
            calls = 0
            want_line = 0
        }
        END {
            if (want_line == 1 && fname != "") {
                print fname "\t" calls "\t" ""
            }
        }
    ' "${gcov_file}"
}

# ---------------------------------------------------------------------------
# Generate gcov files if missing or stale
# ---------------------------------------------------------------------------

generate_gcov_if_needed "${UNITY_GCOV}"
generate_gcov_if_needed "${COVERAGE_GCOV}"

# ---------------------------------------------------------------------------
# Check coverage data availability (.gcda must exist for real data)
# ---------------------------------------------------------------------------

UNITY_DATA="ok"
COVERAGE_DATA="ok"

if [[ ! -f "${UNITY_GCDA}" ]]; then
    UNITY_DATA="no-gcda"
elif [[ ! -f "${UNITY_GCOV}" ]]; then
    UNITY_DATA="no-gcov"
fi

if [[ ! -f "${COVERAGE_GCDA}" ]]; then
    COVERAGE_DATA="no-gcda"
elif [[ ! -f "${COVERAGE_GCOV}" ]]; then
    COVERAGE_DATA="no-gcov"
fi

# ---------------------------------------------------------------------------
# Collect function data from both gcov files
# ---------------------------------------------------------------------------

declare -A unity_calls
declare -A coverage_calls
declare -A func_lines

# Parse Unity gcov (only if .gcda exists and gcov file is available)
if [[ "${UNITY_DATA}" == "ok" ]]; then
    # shellcheck disable=SC2312 # process substitution return value is non-fatal
    while IFS=$'\t' read -r func calls line; do
        [[ -z "${func}" ]] && continue
        unity_calls["${func}"]="${calls}"
        func_lines["${func}"]="${line}"
    done < <(parse_gcov_functions "${UNITY_GCOV}")
fi

# Parse Coverage gcov (only if .gcda exists and gcov file is available)
if [[ "${COVERAGE_DATA}" == "ok" ]]; then
    # shellcheck disable=SC2312 # process substitution return value is non-fatal
    while IFS=$'\t' read -r func calls line; do
        [[ -z "${func}" ]] && continue
        coverage_calls["${func}"]="${calls}"
        [[ -n "${func_lines[${func}]:-}" ]] || func_lines["${func}"]="${line}"
    done < <(parse_gcov_functions "${COVERAGE_GCOV}")
fi

# Build the union of all function names
declare -A func_set
for func in "${!unity_calls[@]}"; do
    func_set["${func}"]=1
done
for func in "${!coverage_calls[@]}"; do
    func_set["${func}"]=1
done

if [[ ${#func_set[@]} -eq 0 ]]; then
    echo "Error: No function records found in gcov files for ${src_rel}" >&2
    echo "  Unity gcov:    ${UNITY_GCOV} (${UNITY_DATA})" >&2
    echo "  Coverage gcov: ${COVERAGE_GCOV} (${COVERAGE_DATA})" >&2
    echo "  Run Test 10 (Unity) or other tests to generate coverage data first." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Find related test files and check function name presence (word-boundary)
# ---------------------------------------------------------------------------

related_test_files=()
if [[ -d "${TEST_SEARCH_DIR}" ]]; then
    # shellcheck disable=SC2312 # find failure is non-fatal
    while IFS= read -r -d '' tf; do
        related_test_files+=("${tf}")
    done < <(find "${TEST_SEARCH_DIR}" -maxdepth 1 -name "${src_basename}_test*.c" -type f -print0 2>/dev/null || true)
fi

# Write all function names to a temp file for batch grep
funcs_temp="$(mktemp)" || { echo "Error: Failed to create temp file" >&2; exit 1; }
printf '%s\n' "${!func_set[@]}" | sort > "${funcs_temp}"

# Check "in related tests" — grep all function names against related test files
declare -A in_related
if [[ ${#related_test_files[@]} -gt 0 ]]; then
    related_temp="$(mktemp)" || { rm -f "${funcs_temp}"; echo "Error: Failed to create temp file" >&2; exit 1; }
    cat "${related_test_files[@]}" > "${related_temp}" 2>/dev/null || true
    if [[ -s "${related_temp}" ]]; then
        while IFS= read -r matched; do
            [[ -n "${matched}" ]] && in_related["${matched}"]=1
        done < <(grep -howwf "${funcs_temp}" "${related_temp}" | sort -u || true)
    fi
    rm -f "${related_temp}"
fi

# Check "in any tests" — grep all function names against entire test directory
declare -A in_any
if [[ -d "${ALL_TESTS_DIR}" ]]; then
    while IFS= read -r matched; do
        [[ -n "${matched}" ]] && in_any["${matched}"]=1
    done < <(grep -r -owf "${funcs_temp}" "${ALL_TESTS_DIR}" --include='*.c' 2>/dev/null \
        | sed 's/^[^:]*://' | sort -u || true)
fi

rm -f "${funcs_temp}"

# ---------------------------------------------------------------------------
# Sort functions by source line number, then by name
# ---------------------------------------------------------------------------

sorted_entries=""
for func in "${!func_set[@]}"; do
    line="${func_lines[${func}]:-0}"
    sorted_entries+="${line}"$'\t'"${func}"$'\n'
done

# shellcheck disable=SC2312 # process substitution return value is non-fatal
mapfile -t sorted_funcs < <(printf '%s' "${sorted_entries}" | sort -t$'\t' -k1,1n -k2,2)

# ---------------------------------------------------------------------------
# Compute per-function results and summary statistics
# ---------------------------------------------------------------------------

total_funcs=${#sorted_funcs[@]}
unity_covered=0
unity_zero=0
blackbox_covered=0
blackbox_zero=0
in_related_count=0
in_any_count=0
never_entered_count=0

declare -a table_rows=()

for entry in "${sorted_funcs[@]}"; do
    sline="${entry%%$'\t'*}"
    func="${entry#*$'\t'}"

    uc="${unity_calls[${func}]:-0}"
    cc="${coverage_calls[${func}]:-0}"

    if [[ "${uc}" -gt 0 ]]; then
        unity_str="YES"
        unity_covered=$((unity_covered + 1))
    else
        unity_str="NO"
        unity_zero=$((unity_zero + 1))
    fi

    if [[ "${cc}" -gt 0 ]]; then
        bb_str="YES"
        blackbox_covered=$((blackbox_covered + 1))
    else
        bb_str="NO"
        blackbox_zero=$((blackbox_zero + 1))
    fi

    if [[ -n "${in_related[${func}]:-}" ]]; then
        related_str="YES"
        in_related_count=$((in_related_count + 1))
    else
        related_str="NO"
    fi

    if [[ -n "${in_any[${func}]:-}" ]]; then
        any_str="YES"
        in_any_count=$((in_any_count + 1))
    else
        any_str="NO"
    fi

    if [[ "${uc}" -gt 0 && "${cc}" -gt 0 ]]; then
        overall="BOTH"
    elif [[ "${uc}" -gt 0 ]]; then
        overall="UNITY"
    elif [[ "${cc}" -gt 0 ]]; then
        overall="BBOX"
    else
        overall="NEITHER"
        never_entered_count=$((never_entered_count + 1))
    fi

    table_rows+=("${func}|${sline}|${unity_str}|${uc}|${bb_str}|${cc}|${related_str}|${any_str}|${overall}")
done

# ---------------------------------------------------------------------------
# Pre-compute summary values for clean output
# ---------------------------------------------------------------------------

unity_pct="$(pct "${unity_covered}" "${total_funcs}")"
blackbox_pct="$(pct "${blackbox_covered}" "${total_funcs}")"
in_related_pct="$(pct "${in_related_count}" "${total_funcs}")"
in_any_pct="$(pct "${in_any_count}" "${total_funcs}")"

unity_gcov_status="${UNITY_DATA}"
coverage_gcov_status="${COVERAGE_DATA}"

# ---------------------------------------------------------------------------
# Render table output
# ---------------------------------------------------------------------------

render_with_tables() {
    local temp_dir layout_json data_json
    temp_dir="$(mktemp -d)" || true
    layout_json="${temp_dir}/layout.json"
    data_json="${temp_dir}/data.json"

    # shellcheck disable=SC2154 # {RESET} etc. are literal tokens for the tables command, not bash variables
    cat > "${layout_json}" << EOF
{
    "title": "{BOLD}Function Coverage: ${src_rel}{RESET}  {CYAN}Unity{RESET} ${unity_pct}%  {MAGENTA}Blackbox{RESET} ${blackbox_pct}%",
    "footer": "{CYAN}Total:{RESET} ${total_funcs}  {MAGENTA}Unity:{RESET} ${unity_covered}  {BLUE}Blackbox:{RESET} ${blackbox_covered}  {GREEN}Related:{RESET} ${in_related_count}  {YELLOW}Any:{RESET} ${in_any_count}  {RED}Unity 0:{RESET} ${unity_zero}  {RED}Never:{RESET} ${never_entered_count}",
    "footer_position": "left",
    "theme": "Red",
    "columns": [
        {"header": "Function", "key": "function", "datatype": "text"},
        {"header": "Line", "key": "line", "datatype": "num", "justification": "right"},
        {"header": "Unity", "key": "unity_calls", "datatype": "num", "justification": "right"},
        {"header": "Black", "key": "blackbox_calls", "datatype": "num", "justification": "right"},
        {"header": "In Tests", "key": "in_related", "datatype": "text"},
        {"header": "In Any", "key": "in_any", "datatype": "text"},
        {"header": "Overall", "key": "overall", "datatype": "text"}
    ]
}
EOF

    {
        echo "["
        local first=1
        for row in "${table_rows[@]}"; do
            IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
            if [[ "${first}" -eq 0 ]]; then
                echo ","
            fi
            first=0
            printf '    {"function": "%s", "line": %s, "unity_calls": %s, "blackbox_calls": %s, "in_related": "%s", "in_any": "%s", "overall": "%s"}' \
                "${func}" "${sline:-0}" "${uc}" "${cc}" "${related_str}" "${any_str}" "${overall}"
        done
        echo ""
        echo "]"
    } > "${data_json}"

    tables "${layout_json}" "${data_json}" 2>/dev/null || true

    rm -rf "${temp_dir}"
}

render_text_table() {
    local max_func=8
    local max_line=4
    for row in "${table_rows[@]}"; do
        IFS='|' read -r func sline _ _ _ _ _ _ _ <<< "${row}"
        [[ ${#func} -gt "${max_func}" ]] && max_func=${#func}
        [[ ${#sline} -gt "${max_line}" ]] && max_line=${#sline}
    done

    printf "Function%*s  Line  Unity  Black  In Tests  In Any  Overall\n" \
        $((max_func - 8)) ""
    printf '%*s\n' $((max_func + 52)) '' | tr ' ' '-'

    for row in "${table_rows[@]}"; do
        IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
        printf "%-${max_func}s  %${max_line}d  %5s  %5s  %8s  %6s  %s\n" \
            "${func}" "${sline:-0}" "${uc}" "${cc}" "${related_str}" "${any_str}" "${overall}"
    done
}

echo ""
echo "Function Coverage Analysis: ${src_rel}"
echo "Source: ${SRC_PATH}"
echo "Unity gcov:    ${UNITY_GCOV} (${unity_gcov_status})"
echo "Coverage gcov: ${COVERAGE_GCOV} (${coverage_gcov_status})"
if [[ ${#related_test_files[@]} -gt 0 ]]; then
    echo "Related test files:"
    for tf in "${related_test_files[@]}"; do
        tf_rel="${tf#"${PROJECT_DIR}"/}"
        echo "  ${tf_rel}"
    done
else
    echo "Related test files: (none found — pattern: ${src_basename}_test*.c)"
fi
echo ""

if command -v tables >/dev/null 2>&1; then
    render_with_tables
else
    render_text_table
fi

echo ""
echo "Summary:"
echo "  Total functions:       ${total_funcs}"
echo "  Covered by Unity:      ${unity_covered} (${unity_pct}%)"
echo "  Covered by Blackbox:   ${blackbox_covered} (${blackbox_pct}%)"
echo "  In related tests:      ${in_related_count} (${in_related_pct}%)"
echo "  In any Unity tests:    ${in_any_count} (${in_any_pct}%)"
echo "  Not entered by Unity:  ${unity_zero}"
echo "  Never entered (both):  ${never_entered_count}"
echo ""

# Functions not called by Unity tests
echo "Functions not called by Unity tests:"
if [[ "${unity_zero}" -eq 0 ]]; then
    echo "  (none — all functions entered by Unity tests)"
else
    for row in "${table_rows[@]}"; do
        IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
        if [[ "${uc}" -eq 0 ]]; then
            extra=""
            if [[ "${cc}" -gt 0 ]]; then
                extra=" (blackbox: ${cc})"
            fi
            if [[ -n "${in_related[${func}]:-}" ]]; then
                extra="${extra} [in related tests]"
            fi
            if [[ -n "${in_any[${func}]:-}" ]]; then
                extra="${extra} [in any tests]"
            fi
            echo "  ${func} (line ${sline})${extra}"
        fi
    done
fi
echo ""

# Functions not entered by either suite
if [[ "${never_entered_count}" -gt 0 ]]; then
    echo "Functions not entered by either Unity or Blackbox tests (${never_entered_count}):"
    for row in "${table_rows[@]}"; do
        IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
        if [[ "${overall}" == "NEITHER" ]]; then
            echo "  ${func} (line ${sline})"
        fi
    done
    echo ""
fi

# Functions entered by Unity but not explicitly in related test source
incidental_count=0
for row in "${table_rows[@]}"; do
    IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
    if [[ "${uc}" -gt 0 && -z "${in_related[${func}]:-}" ]]; then
        incidental_count=$((incidental_count + 1))
    fi
done

if [[ "${incidental_count}" -gt 0 ]]; then
    echo "Functions entered by Unity but not in related tests (${incidental_count}):"
    for row in "${table_rows[@]}"; do
        IFS='|' read -r func sline unity_str uc bb_str cc related_str any_str overall <<< "${row}"
        if [[ "${uc}" -gt 0 && -z "${in_related[${func}]:-}" ]]; then
            echo "  ${func} (line ${sline}, unity: ${uc}, blackbox: ${cc})"
        fi
    done
    echo ""
fi
