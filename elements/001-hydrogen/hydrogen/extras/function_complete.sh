#!/bin/bash
# function_complete.sh - Project-wide function coverage scan.
#
# Iterates over all compiled source files, analyzing per-function
# coverage across Unity and Blackbox test suites. Reports only files
# that have functions with gaps — not covered by some test suite or
# not referenced in test source files.
#
# Output is grouped by folder with linebreaks between groups, matching
# the coverage table format.
#
# Usage: ./function_complete.sh
#
# CHANGELOG
# 1.0.0 - 2026-08-28 - Initial version

set -euo pipefail

if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "Error: HYDROGEN_ROOT environment variable is not set" >&2
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory" >&2
    exit 1
fi

PROJECT_DIR="${HYDROGEN_ROOT}"
UNITY_GCOV_DIR="${PROJECT_DIR}/build/unity/src"
COVERAGE_GCOV_DIR="${PROJECT_DIR}/build/coverage/src"
SRC_DIR="${PROJECT_DIR}/src"
UNITY_TESTS_DIR="${PROJECT_DIR}/tests/unity/src"

FIND="${FIND:-find}"
GREP="${GREP:-grep}"
SORT="${SORT:-sort}"
STAT="${STAT:-stat}"
DATE="${DATE:-date}"
AWK="${AWK:-awk}"

TMPDIR_WORK="$(mktemp -d)" || { echo "Error: Failed to create temp directory" >&2; exit 1; }
trap 'rm -rf "${TMPDIR_WORK}"' EXIT

FUNC_DATA_FILE="${TMPDIR_WORK}/func_data.tsv"
ALL_FUNCS_FILE="${TMPDIR_WORK}/all_funcs.txt"
ANY_TESTS_FILE="${TMPDIR_WORK}/any_tests.txt"
RESULTS_FILE="${TMPDIR_WORK}/results.tsv"

# ---------------------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------------------

# Generate a .gcov file from .gcno + .gcda if it's missing or stale.
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
# Output: func_name<TAB>call_count<TAB>source_line_number (one per line)
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

# Compute folder group from a source-relative path (matches coverage table)
compute_folder() {
    local src_rel="$1"
    local path_after_src="${src_rel#src/}"

    if [[ "${path_after_src}" == */ ]]; then
        path_after_src="${path_after_src%/}"
    fi

    if [[ "${path_after_src}" == */* ]]; then
        local first_level="${path_after_src%%/*}"
        local remaining_path="${path_after_src#*/}"

        if [[ "${remaining_path}" == */* ]]; then
            local second_level="${remaining_path%%/*}"
            echo "src/${first_level}/${second_level}"
        else
            echo "src/${first_level}"
        fi
    else
        echo "src/"
    fi
}

# ---------------------------------------------------------------------------
# Phase 1: Find all source files and parse function data
# ---------------------------------------------------------------------------

echo "Scanning source files..." >&2

: > "${FUNC_DATA_FILE}"
: > "${ALL_FUNCS_FILE}"

source_count=0
"${FIND}" "${SRC_DIR}" -name "*.c" -not -name "*_test*" -not -name "test_*" -type f | "${SORT}" > "${TMPDIR_WORK}/source_files.txt"
while IFS= read -r src_file; do
    [[ -z "${src_file}" ]] && continue

    src_rel="${src_file#"${SRC_DIR}/"}"
    src_noext="${src_rel%.c}"

    UNITY_GCOV="${UNITY_GCOV_DIR}/${src_noext}.c.gcov"
    COVERAGE_GCOV="${COVERAGE_GCOV_DIR}/${src_noext}.c.gcov"
    UNITY_GCDA="${UNITY_GCOV_DIR}/${src_noext}.gcda"
    COVERAGE_GCDA="${COVERAGE_GCOV_DIR}/${src_noext}.gcda"

    # Skip files with no .gcda in either build (no coverage data at all)
    if [[ ! -f "${UNITY_GCDA}" && ! -f "${COVERAGE_GCDA}" ]]; then
        continue
    fi

    generate_gcov_if_needed "${UNITY_GCOV}"
    generate_gcov_if_needed "${COVERAGE_GCOV}"

    declare -A unity_calls=()
    declare -A coverage_calls=()
    declare -A func_lines=()

    if [[ -f "${UNITY_GCDA}" && -f "${UNITY_GCOV}" ]]; then
        # shellcheck disable=SC2312 # process substitution return value is non-fatal
        while IFS=$'\t' read -r func calls line; do
            [[ -z "${func}" ]] && continue
            unity_calls["${func}"]="${calls}"
            func_lines["${func}"]="${line}"
        done < <(parse_gcov_functions "${UNITY_GCOV}")
    fi

    if [[ -f "${COVERAGE_GCDA}" && -f "${COVERAGE_GCOV}" ]]; then
        # shellcheck disable=SC2312 # process substitution return value is non-fatal
        while IFS=$'\t' read -r func calls line; do
            [[ -z "${func}" ]] && continue
            coverage_calls["${func}"]="${calls}"
            [[ -n "${func_lines[${func}]:-}" ]] || func_lines["${func}"]="${line}"
        done < <(parse_gcov_functions "${COVERAGE_GCOV}")
    fi

    declare -A func_set=()
    for func in "${!unity_calls[@]}"; do
        func_set["${func}"]=1
    done
    for func in "${!coverage_calls[@]}"; do
        func_set["${func}"]=1
    done

    if [[ ${#func_set[@]} -eq 0 ]]; then
        unset unity_calls coverage_calls func_lines func_set
        continue
    fi

    for func in "${!func_set[@]}"; do
        uc="${unity_calls[${func}]:-0}"
        cc="${coverage_calls[${func}]:-0}"
        sline="${func_lines[${func}]:-0}"
        printf '%s\t%s\t%s\t%s\t%s\n' "${src_rel}" "${func}" "${uc}" "${cc}" "${sline}" >> "${FUNC_DATA_FILE}"
        echo "${func}" >> "${ALL_FUNCS_FILE}"
    done

    source_count=$((source_count + 1))
    unset unity_calls coverage_calls func_lines func_set
    declare -A unity_calls=()
    declare -A coverage_calls=()
    declare -A func_lines=()
    declare -A func_set=()
done < "${TMPDIR_WORK}/source_files.txt"

echo "Processed ${source_count} source files" >&2

"${SORT}" -o "${ALL_FUNCS_FILE}" -u "${ALL_FUNCS_FILE}"

# ---------------------------------------------------------------------------
# Phase 2: Batch grep all function names against all test files
# ---------------------------------------------------------------------------

echo "Grepping function names in test files..." >&2

: > "${ANY_TESTS_FILE}"
if [[ -d "${UNITY_TESTS_DIR}" && -s "${ALL_FUNCS_FILE}" ]]; then
    "${GREP}" -r -owf "${ALL_FUNCS_FILE}" "${UNITY_TESTS_DIR}" --include='*.c' 2>/dev/null \
        | sed 's/^[^:]*://' | "${SORT}" -u > "${ANY_TESTS_FILE}" || true
fi

declare -A any_tests_found=()
if [[ -s "${ANY_TESTS_FILE}" ]]; then
    while IFS= read -r func; do
        [[ -n "${func}" ]] && any_tests_found["${func}"]=1
    done < "${ANY_TESTS_FILE}"
fi

echo "Found ${#any_tests_found[@]} functions referenced in tests." >&2

# ---------------------------------------------------------------------------
# Phase 3: Compute gap counts per source file
# ---------------------------------------------------------------------------

echo "Computing gap counts..." >&2

: > "${RESULTS_FILE}"

# Get unique source file paths from func data
"${SORT}" -u -t$'\t' -k1,1 "${FUNC_DATA_FILE}" | cut -f1 > "${TMPDIR_WORK}/unique_srcs.txt"

while IFS= read -r src_rel; do
    [[ -z "${src_rel}" ]] && continue

    src_basename="$(basename "${src_rel}" .c)"
    src_dirname="$(dirname "${src_rel}")"
    if [[ "${src_dirname}" == "." ]]; then
        test_search_dir="${UNITY_TESTS_DIR}"
    else
        test_search_dir="${UNITY_TESTS_DIR}/${src_dirname}"
    fi

    total_funcs=0
    unity_gap=0
    black_gap=0
    test_gap=0
    any_gap=0

    # Find related test files once for this source file
    related_test_files=()
    if [[ -d "${test_search_dir}" ]]; then
        # shellcheck disable=SC2312 # find failure is non-fatal
        while IFS= read -r -d '' tf; do
            related_test_files+=("${tf}")
        done < <("${FIND}" "${test_search_dir}" -maxdepth 1 -name "${src_basename}_test*.c" -type f -print0 2>/dev/null || true)
    fi

    # Write this file's function names for batch grep against related tests
    funcs_temp="${TMPDIR_WORK}/funcs_temp.txt"
    "${GREP}" -F "${src_rel}$(printf '\t')" "${FUNC_DATA_FILE}" | cut -f2 | "${SORT}" -u > "${funcs_temp}"

    # Check function names against related test files
    declare -A in_related=()
    if [[ ${#related_test_files[@]} -gt 0 ]]; then
        related_concat="${TMPDIR_WORK}/related_concat.txt"
        cat "${related_test_files[@]}" > "${related_concat}" 2>/dev/null || true
        if [[ -s "${related_concat}" ]]; then
            # shellcheck disable=SC2312 # grep return value is non-fatal in process substitution
            while IFS= read -r matched; do
                [[ -n "${matched}" ]] && in_related["${matched}"]=1
            done < <("${GREP}" -howwf "${funcs_temp}" "${related_concat}" | "${SORT}" -u || true)
        fi
        rm -f "${related_concat}"
    fi

    # Compute gap counts from the func data
    # FUNC_DATA_FILE format: src_rel <TAB> func <TAB> unity_calls <TAB> cov_calls <TAB> line_num
    while IFS=$'\t' read -r fsrc ffunc uc cc sline; do
        [[ "${fsrc}" != "${src_rel}" ]] && continue
        total_funcs=$((total_funcs + 1))
        if [[ "${uc}" -eq 0 ]]; then
            unity_gap=$((unity_gap + 1))
        fi
        if [[ "${cc}" -eq 0 ]]; then
            black_gap=$((black_gap + 1))
        fi
        if [[ -z "${in_related[${ffunc}]:-}" ]]; then
            test_gap=$((test_gap + 1))
        fi
        if [[ -z "${any_tests_found[${ffunc}]:-}" ]]; then
            any_gap=$((any_gap + 1))
        fi
    done < "${FUNC_DATA_FILE}"

    rm -f "${funcs_temp}"
    unset in_related

    # Only include files with at least one gap
    if [[ ${unity_gap} -gt 0 || ${black_gap} -gt 0 || ${test_gap} -gt 0 || ${any_gap} -gt 0 ]]; then
        folder="$(compute_folder "${src_rel}")"
        printf '%s\t%s\t%d\t%d\t%d\t%d\t%d\n' \
            "${folder}" "${src_rel}" "${total_funcs}" "${unity_gap}" "${black_gap}" "${test_gap}" "${any_gap}" \
            >> "${RESULTS_FILE}"
    fi
done < "${TMPDIR_WORK}/unique_srcs.txt"

rm -f "${TMPDIR_WORK}/unique_srcs.txt"

# ---------------------------------------------------------------------------
# Phase 4: Sort results by folder, then file
# ---------------------------------------------------------------------------

"${SORT}" -t$'\t' -k1,1 -k2,2 -o "${RESULTS_FILE}" "${RESULTS_FILE}"

# ---------------------------------------------------------------------------
# Phase 5: Render table
# ---------------------------------------------------------------------------

total_files=$(wc -l < "${RESULTS_FILE}")
total_funcs_sum=0
total_unity_gap=0
total_black_gap=0
total_test_gap=0
total_any_gap=0

if [[ -s "${RESULTS_FILE}" ]]; then
    while IFS=$'\t' read -r _ _ total ug bg tg ag; do
        total_funcs_sum=$((total_funcs_sum + total))
        total_unity_gap=$((total_unity_gap + ug))
        total_black_gap=$((total_black_gap + bg))
        total_test_gap=$((total_test_gap + tg))
        total_any_gap=$((total_any_gap + ag))
    done < "${RESULTS_FILE}"
fi

display_timestamp=$("${DATE}" '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null || echo "")

if [[ "${total_files}" -eq 0 ]]; then
    echo "Function Coverage — Complete"
    echo "All ${source_count} source files have full function coverage."
    exit 0
fi

echo ""
echo "Function Coverage — Files with Gaps (${total_files} files, ${total_funcs_sum} functions, ${total_unity_gap} unity gaps, ${total_black_gap} black gaps, ${total_test_gap} missing tests)"
echo ""

render_with_tables() {
    local temp_dir layout_json data_json
    temp_dir="$(mktemp -d)" || true
    layout_json="${temp_dir}/layout.json"
    data_json="${temp_dir}/data.json"

    cat > "${layout_json}" << EOF
{
    "title": "{BOLD}Function Coverage — Files with Gaps{RESET}  {CYAN}Unity Gaps:{RESET} ${total_unity_gap}  {MAGENTA}Black Gaps:{RESET} ${total_black_gap}  {YELLOW}Missing Tests:{RESET} ${total_test_gap}",
    "footer": "{CYAN}Files:{RESET} ${total_files}  {GREEN}Functions:{RESET} ${total_funcs_sum}  {MAGENTA}Unity Gaps:{RESET} ${total_unity_gap}  {BLUE}Black Gaps:{RESET} ${total_black_gap}  {YELLOW}Missing Tests:{RESET} ${total_test_gap}  {CYAN}${display_timestamp}{RESET}",
    "footer_position": "left",
    "theme": "Red",
    "columns": [
        {"header": "Folder", "key": "folder", "datatype": "text", "visible": false, "break": true},
        {"header": "File", "key": "file", "datatype": "text"},
        {"header": "Funcs", "key": "functions", "datatype": "num", "justification": "right"},
        {"header": "U Gap", "key": "unity_gap", "datatype": "num", "justification": "right"},
        {"header": "B Gap", "key": "black_gap", "datatype": "num", "justification": "right"},
        {"header": "No Tests", "key": "missing_tests", "datatype": "num", "justification": "right"}
    ]
}
EOF

    {
        echo "["
        local first=1
        while IFS=$'\t' read -r folder file total ug bg tg ag; do
            if [[ "${first}" -eq 0 ]]; then
                echo ","
            fi
            first=0
            printf '    {"folder": "%s", "file": "%s", "functions": %d, "unity_gap": %d, "black_gap": %d, "missing_tests": %d}' \
                "${folder}" "${file}" "${total}" "${ug}" "${bg}" "${tg}"
        done < "${RESULTS_FILE}"
        echo ""
        echo "]"
    } > "${data_json}"

    tables "${layout_json}" "${data_json}" 2>/dev/null || true

    rm -rf "${temp_dir}"
}

render_text_table() {
    local prev_folder=""
    printf "%-40s  %5s  %5s  %5s  %8s\n" "File" "Funcs" "U Gap" "B Gap" "No Tests"
    printf '%*s\n' 80 '' | tr ' ' '-'

    while IFS=$'\t' read -r folder file total ug bg tg ag; do
        if [[ "${folder}" != "${prev_folder}" ]]; then
            local folder_display="${folder}"
            [[ "${folder_display}" != */ ]] && folder_display="${folder_display}/"
            printf "\n%s\n" "${folder_display}"
            prev_folder="${folder}"
        fi
        printf "%-40s  %5d  %5d  %5d  %8d\n" "${file}" "${total}" "${ug}" "${bg}" "${tg}"
    done < "${RESULTS_FILE}"

    printf '%*s\n' 70 '' | tr ' ' '-'
    printf "Total: %d files, %d functions, %d unity gaps, %d black gaps, %d missing tests\n" \
        "${total_files}" "${total_funcs_sum}" "${total_unity_gap}" "${total_black_gap}" "${total_test_gap}"
}

if command -v tables >/dev/null 2>&1; then
    render_with_tables
else
    render_text_table
fi
