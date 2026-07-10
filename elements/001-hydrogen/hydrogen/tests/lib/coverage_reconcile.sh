#!/usr/bin/env bash

# Coverage Percentage Reconciliation
# Compares Results (coverage.sh global counts) vs Coverage table (per-file sums)
# using underlying line counts, not reverse-engineered percentages.

# CHANGELOG
# 1.2.0 - 2026-07-10 - Count-based reconciliation; show covered/instrumented for both sides
# 1.1.1 - 2026-07-09 - Shellcheck clean: justified directives, avoid SC2312 pipelines
# 1.1.0 - 2026-07-09 - Robust ANSI-safe percent extraction; fixed target_pct /100 bug
# 1.0.0 - 2026-07-09 - Extracted from test_00_all.sh

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_RECONCILE_GUARD:-}" ]] && return 0
export COVERAGE_RECONCILE_GUARD="true"

# Format covered/total as X.XXX% (matches coverage.sh / coverage_table.sh)
# Usage: _coverage_pct <covered> <total>
_coverage_pct() {
    local covered="$1"
    local total="$2"
    local awkcmd="${3:-awk}"
    if [[ "${total}" -le 0 ]]; then
        echo "0.000"
        return 0
    fi
    # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
    "${awkcmd}" -v c="${covered}" -v t="${total}" 'BEGIN {printf "%.3f", (c / t) * 100}'
}

# Sum coverage_data.json totals: covered_u,inst_u,covered_bb,inst_bb,covered_comb,inst_comb
# Usage: _table_coverage_totals <coverage_data.json>
_table_coverage_totals() {
    local json_file="$1"
    if [[ ! -f "${json_file}" ]]; then
        echo "0,0,0,0,0,0"
        return 0
    fi
    # Prefer python for reliable JSON; fall back to zeros
    python3 - "${json_file}" <<'PY' 2>/dev/null || echo "0,0,0,0,0,0"
import json, sys
path = sys.argv[1]
with open(path, encoding="utf-8") as fh:
    data = json.load(fh)
uu = ui = bc = bi = cc = ci = 0
for row in data:
    if not isinstance(row, dict):
        continue
    if "unity_instrumented" not in row and "coverage_instrumented" not in row:
        continue
    uu += int(row.get("unity_covered") or 0)
    ui += int(row.get("unity_instrumented") or 0)
    bc += int(row.get("coverage_covered") or 0)
    bi += int(row.get("coverage_instrumented") or 0)
    cc += int(row.get("combined_covered") or 0)
    ci += int(row.get("combined_instrumented") or 0)
print(f"{uu},{ui},{bc},{bi},{cc},{ci}")
PY
}

# Find DISCREPANCY so covered/(gcov+D) rounds to the same 3dp % as table_covered/table_inst.
# Prefer the D closest to (table_inst - gcov) among matches.
# Usage: _find_discrepancy <results_covered> <gcov_raw> <table_covered> <table_inst>
_find_discrepancy() {
    local results_covered="$1"
    local gcov_raw="$2"
    local table_covered="$3"
    local table_inst="$4"
    local awkcmd="${5:-awk}"

    if [[ "${results_covered}" -le 0 || "${table_covered}" -le 0 || "${table_inst}" -le 0 ]]; then
        echo "0"
        return 0
    fi

    # shellcheck disable=SC2016 # AWK program uses single quotes intentionally
    "${awkcmd}" -v rc="${results_covered}" -v gcov="${gcov_raw}" -v tc="${table_covered}" -v ti="${table_inst}" '
    BEGIN {
        target = sprintf("%.3f", (tc / ti) * 100)
        preferred = ti - gcov
        # Ratio-based center, then search a window for exact 3dp match
        center = int((rc * ti / tc) + 0.5) - gcov
        best_d = center
        best_score = 1e18
        found = 0
        for (d = center - 50; d <= center + 50; d++) {
            total = gcov + d
            if (total <= 0) continue
            pct = sprintf("%.3f", (rc / total) * 100)
            if (pct == target) {
                score = (d > preferred) ? (d - preferred) : (preferred - d)
                if (!found || score < best_score) {
                    best_score = score
                    best_d = d
                    found = 1
                }
            }
        }
        if (!found) {
            # Fall back to instrumented alignment
            best_d = preferred
        }
        print best_d
    }'
}

# Compare Results (coverage.sh counts) with Coverage table (per-file counts).
# On mismatch, print count breakdown and recommended DISCREPANCY values.
# Usage: reconcile_coverage_percentages <results_table_file> <coverage_table_file>
# Table file args kept for call-site compatibility; counts come from RESULTS_DIR files.
# Relies on: RESULTS_DIR, SCRIPT_DIR, PROJECT_DIR (or HYDROGEN_ROOT), GREP, SED, AWK
reconcile_coverage_percentages() {
    # shellcheck disable=SC2034 # retained for call-site compatibility with test_00_all.sh
    local results_table_file="$1"
    # shellcheck disable=SC2034 # retained for call-site compatibility with test_00_all.sh
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

    # --- Results side: coverage.sh detailed files (totals already include current DISCREPANCY) ---
    local unity_covered=0 unity_total=0
    local blackbox_covered=0 blackbox_total=0
    if [[ -f "${results_dir}/coverage_unity.txt.detailed" ]]; then
        IFS=',' read -r _ _ unity_covered unity_total _ _ < "${results_dir}/coverage_unity.txt.detailed"
    fi
    if [[ -f "${results_dir}/coverage_blackbox.txt.detailed" ]]; then
        IFS=',' read -r _ _ blackbox_covered blackbox_total _ _ < "${results_dir}/coverage_blackbox.txt.detailed"
    fi
    unity_covered=${unity_covered:-0}
    unity_total=${unity_total:-0}
    blackbox_covered=${blackbox_covered:-0}
    blackbox_total=${blackbox_total:-0}

    # --- Coverage table side: per-file gcov sums from coverage_data.json ---
    local table_totals
    table_totals=$(_table_coverage_totals "${results_dir}/coverage_data.json")
    local table_u_cov table_u_inst table_b_cov table_b_inst table_c_cov table_c_inst
    IFS=',' read -r table_u_cov table_u_inst table_b_cov table_b_inst table_c_cov table_c_inst <<< "${table_totals}"
    table_u_cov=${table_u_cov:-0}
    table_u_inst=${table_u_inst:-0}
    table_b_cov=${table_b_cov:-0}
    table_b_inst=${table_b_inst:-0}
    table_c_cov=${table_c_cov:-0}
    table_c_inst=${table_c_inst:-0}

    if [[ "${unity_total}" -le 0 && "${blackbox_total}" -le 0 ]]; then
        return 0
    fi
    if [[ "${table_u_inst}" -le 0 && "${table_b_inst}" -le 0 ]]; then
        return 0
    fi

    local results_u_pct results_b_pct results_c_pct
    local table_u_pct table_b_pct table_c_pct
    results_u_pct=$(_coverage_pct "${unity_covered}" "${unity_total}" "${awkcmd}")
    results_b_pct=$(_coverage_pct "${blackbox_covered}" "${blackbox_total}" "${awkcmd}")
    # Combined comes from coverage_combined.txt when present
    local combined_pct="0.000"
    if [[ -f "${results_dir}/coverage_combined.txt" ]]; then
        combined_pct=$(cat "${results_dir}/coverage_combined.txt" 2>/dev/null || echo "0.000")
    fi
    results_c_pct="${combined_pct}"
    table_u_pct=$(_coverage_pct "${table_u_cov}" "${table_u_inst}" "${awkcmd}")
    table_b_pct=$(_coverage_pct "${table_b_cov}" "${table_b_inst}" "${awkcmd}")
    table_c_pct=$(_coverage_pct "${table_c_cov}" "${table_c_inst}" "${awkcmd}")

    if [[ "${results_u_pct}" == "${table_u_pct}" && "${results_b_pct}" == "${table_b_pct}" ]]; then
        return 0
    fi

    local current_unity_disc current_coverage_disc
    current_unity_disc=$("${grepcmd}" "^DISCREPANCY_UNITY=" "${coverage_sh}" || true)
    current_unity_disc=$("${sedcmd}" 's/DISCREPANCY_UNITY=//' <<< "${current_unity_disc}" || echo "0")
    current_coverage_disc=$("${grepcmd}" "^DISCREPANCY_COVERAGE=" "${coverage_sh}" || true)
    current_coverage_disc=$("${sedcmd}" 's/DISCREPANCY_COVERAGE=//' <<< "${current_coverage_disc}" || echo "0")
    current_unity_disc=${current_unity_disc:-0}
    current_coverage_disc=${current_coverage_disc:-0}

    local unity_gcov=$((unity_total - current_unity_disc))
    local blackbox_gcov=$((blackbox_total - current_coverage_disc))

    local new_unity_disc new_coverage_disc
    new_unity_disc=$(_find_discrepancy "${unity_covered}" "${unity_gcov}" "${table_u_cov}" "${table_u_inst}" "${awkcmd}")
    new_coverage_disc=$(_find_discrepancy "${blackbox_covered}" "${blackbox_gcov}" "${table_b_cov}" "${table_b_inst}" "${awkcmd}")

    local unity_new_total=$((unity_gcov + new_unity_disc))
    local blackbox_new_total=$((blackbox_gcov + new_coverage_disc))
    local unity_new_pct blackbox_new_pct
    unity_new_pct=$(_coverage_pct "${unity_covered}" "${unity_new_total}" "${awkcmd}")
    blackbox_new_pct=$(_coverage_pct "${blackbox_covered}" "${blackbox_new_total}" "${awkcmd}")

    echo "Coverage count mismatch — Results (coverage.sh) vs Coverage table (per-file):"
    echo "  Unity:    Results ${unity_covered}/${unity_total} = ${results_u_pct}%   Table ${table_u_cov}/${table_u_inst} = ${table_u_pct}%   gcov_raw=${unity_gcov}"
    echo "  Blackbox: Results ${blackbox_covered}/${blackbox_total} = ${results_b_pct}%   Table ${table_b_cov}/${table_b_inst} = ${table_b_pct}%   gcov_raw=${blackbox_gcov}"
    echo "  Combined: Results ${results_c_pct}%   Table ${table_c_cov}/${table_c_inst} = ${table_c_pct}%"
    echo "Update tests/lib/coverage.sh so Results % matches Table %:"
    echo "  DISCREPANCY_UNITY=${new_unity_disc}  (was ${current_unity_disc})  → ${unity_covered}/${unity_new_total} = ${unity_new_pct}%"
    echo "  DISCREPANCY_COVERAGE=${new_coverage_disc}  (was ${current_coverage_disc})  → ${blackbox_covered}/${blackbox_new_total} = ${blackbox_new_pct}%"

    local recent_files=""
    if command -v git >/dev/null 2>&1; then
        recent_files=$(git -C "${project_dir}" log --since='7 days ago' --name-only --pretty=format: -- 'src/**/*.c' 2>/dev/null || true)
        recent_files=$(echo "${recent_files}" | sed '/^$/d' | sort -u | head -12 || true)
    fi
    if [[ -n "${recent_files}" ]]; then
        echo "Recent src files (7d) that may affect instrumented/covered counts:"
        while IFS= read -r f; do
            [[ -z "${f}" ]] && continue
            echo "  ${f#*/src/}"
        done <<< "${recent_files}"
    fi
}
