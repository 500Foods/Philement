#!/usr/bin/env bash

# Coverage Percentage Reconciliation
# Compares Results (coverage.sh) vs Coverage (coverage_table.sh) percentages
# and advises on DISCREPANCY_UNITY / DISCREPANCY_COVERAGE updates.

# CHANGELOG
# 1.1.1 - 2026-07-09 - Shellcheck clean: justified directives, avoid SC2312 pipelines
# 1.1.0 - 2026-07-09 - Robust ANSI-safe percent extraction; fixed target_pct /100 bug
# 1.0.0 - 2026-07-09 - Extracted from test_00_all.sh

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_RECONCILE_GUARD:-}" ]] && return 0
export COVERAGE_RECONCILE_GUARD="true"

# Extract "Unity% Blackbox% Combined%" from a table title line (ANSI-safe).
# Usage: _extract_coverage_triplet <file> <title_substring>
_extract_coverage_triplet() {
    local file="$1"
    local title="$2"
    local raw_line stripped_line grepcmd sedcmd
    # shellcheck disable=SC2154 # GREP assigned by framework.sh
    grepcmd="${GREP}"
    # shellcheck disable=SC2154 # SED assigned by framework.sh
    sedcmd="${SED}"

    raw_line=$("${grepcmd}" "${title}" "${file}" || true)
    raw_line=$(echo "${raw_line}" | head -1 || true)
    if [[ -z "${raw_line}" ]]; then
        echo ""
        return 0
    fi

    # Strip ANSI CSI sequences, then pull the three X.XXX% values in order
    stripped_line=$(echo "${raw_line}" | "${sedcmd}" 's/\x1b\[[0-9;]*m//g' || true)
    echo "${stripped_line}" | "${grepcmd}" -oE '[0-9]+\.[0-9]+%' | head -3 | tr '\n' ' ' | "${sedcmd}" 's/[[:space:]]*$//' || true
}

# Compare Results table coverage % with Coverage table coverage %.
# On mismatch, print short advice with new DISCREPANCY values and recent src files.
# Usage: reconcile_coverage_percentages <results_table_file> <coverage_table_file>
# Relies on: RESULTS_DIR, SCRIPT_DIR, PROJECT_DIR (or HYDROGEN_ROOT), GREP, SED, AWK
reconcile_coverage_percentages() {
    local results_table_file="$1"
    local coverage_table_file="$2"
    local project_dir="${PROJECT_DIR:-${HYDROGEN_ROOT}}"
    local script_dir="${SCRIPT_DIR:-${project_dir}/tests}"
    local results_dir="${RESULTS_DIR:-${project_dir}/build/tests/results}"
    local coverage_sh="${script_dir}/lib/coverage.sh"
    local grepcmd sedcmd awkcmd
    # shellcheck disable=SC2154 # GREP assigned by framework.sh
    grepcmd="${GREP}"
    # shellcheck disable=SC2154 # SED assigned by framework.sh
    sedcmd="${SED}"
    # shellcheck disable=SC2154 # AWK assigned by framework.sh
    awkcmd="${AWK}"

    local results_summary coverage_summary
    results_summary=$(_extract_coverage_triplet "${results_table_file}" "Test Suite Results")
    coverage_summary=$(_extract_coverage_triplet "${coverage_table_file}" "Test Suite Coverage")

    if [[ -z "${results_summary}" || -z "${coverage_summary}" ]]; then
        return 0
    fi
    if [[ "${results_summary}" == "${coverage_summary}" ]]; then
        return 0
    fi

    # Parse Unity/Blackbox percents without relying on word-splitting quirks
    local cov_unity_pct cov_blackbox_pct
    # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
    cov_unity_pct=$(echo "${coverage_summary}" | "${awkcmd}" '{gsub(/%/,""); print $1}')
    # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
    cov_blackbox_pct=$(echo "${coverage_summary}" | "${awkcmd}" '{gsub(/%/,""); print $2}')

    local current_unity_disc current_coverage_disc
    current_unity_disc=$("${grepcmd}" "^DISCREPANCY_UNITY=" "${coverage_sh}" || true)
    current_unity_disc=$("${sedcmd}" 's/DISCREPANCY_UNITY=//' <<< "${current_unity_disc}" || echo "0")
    current_coverage_disc=$("${grepcmd}" "^DISCREPANCY_COVERAGE=" "${coverage_sh}" || true)
    current_coverage_disc=$("${sedcmd}" 's/DISCREPANCY_COVERAGE=//' <<< "${current_coverage_disc}" || echo "0")
    current_unity_disc=${current_unity_disc:-0}
    current_coverage_disc=${current_coverage_disc:-0}

    # detailed format: timestamp,pct,covered,total_with_disc,instrumented_files,covered_files
    local unity_covered=0 unity_total_with_disc=0
    local blackbox_covered=0 blackbox_total_with_disc=0
    if [[ -f "${results_dir}/coverage_unity.txt.detailed" ]]; then
        IFS=',' read -r _ _ unity_covered unity_total_with_disc _ _ < "${results_dir}/coverage_unity.txt.detailed"
    fi
    if [[ -f "${results_dir}/coverage_blackbox.txt.detailed" ]]; then
        IFS=',' read -r _ _ blackbox_covered blackbox_total_with_disc _ _ < "${results_dir}/coverage_blackbox.txt.detailed"
    fi

    # coverage.sh: covered / (gcov_total + DISCREPANCY) = results_pct
    # target:     covered / (gcov_total + new_DISCREPANCY) = coverage_pct
    # new_DISCREPANCY = (covered / (coverage_pct/100)) - gcov_total
    local unity_gcov_total=$((unity_total_with_disc - current_unity_disc))
    local blackbox_gcov_total=$((blackbox_total_with_disc - current_coverage_disc))
    local unity_target_total=0 blackbox_target_total=0
    local bc_result

    if [[ "${unity_covered}" -gt 0 ]] && [[ -n "${cov_unity_pct}" ]] \
        && [[ "${cov_unity_pct}" != "0" ]] && [[ "${cov_unity_pct}" != "0.000" ]]; then
        # shellcheck disable=SC2310 # bc failure should not stop the script
        bc_result=$(echo "scale=6; ${unity_covered} / (${cov_unity_pct} / 100)" | bc 2>/dev/null || true)
        # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
        unity_target_total=$("${awkcmd}" '{printf "%.0f", $1}' <<< "${bc_result}" || echo "0")
    fi
    if [[ "${blackbox_covered}" -gt 0 ]] && [[ -n "${cov_blackbox_pct}" ]] \
        && [[ "${cov_blackbox_pct}" != "0" ]] && [[ "${cov_blackbox_pct}" != "0.000" ]]; then
        # shellcheck disable=SC2310 # bc failure should not stop the script
        bc_result=$(echo "scale=6; ${blackbox_covered} / (${cov_blackbox_pct} / 100)" | bc 2>/dev/null || true)
        # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
        blackbox_target_total=$("${awkcmd}" '{printf "%.0f", $1}' <<< "${bc_result}" || echo "0")
    fi

    local new_unity_disc=$((unity_target_total - unity_gcov_total))
    local new_coverage_disc=$((blackbox_target_total - blackbox_gcov_total))

    echo "Coverage % mismatch — Results: ${results_summary}  Coverage: ${coverage_summary}"
    echo "Update tests/lib/coverage.sh:"
    echo "  DISCREPANCY_UNITY=${new_unity_disc}  (was ${current_unity_disc})"
    echo "  DISCREPANCY_COVERAGE=${new_coverage_disc}  (was ${current_coverage_disc})"

    # Recent src files that may have shifted instrumented line counts
    local recent_files=""
    if command -v git >/dev/null 2>&1; then
        recent_files=$(git -C "${project_dir}" log --since='7 days ago' --name-only --pretty=format: -- 'src/**/*.c' 2>/dev/null || true)
        recent_files=$(echo "${recent_files}" | sed '/^$/d' | sort -u | head -12 || true)
    fi
    if [[ -n "${recent_files}" ]]; then
        echo "Recent src files (7d) that may affect the discrepancy:"
        while IFS= read -r f; do
            [[ -z "${f}" ]] && continue
            echo "  ${f#*/src/}"
        done <<< "${recent_files}"
    fi
}
