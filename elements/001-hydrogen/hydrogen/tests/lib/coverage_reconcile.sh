#!/usr/bin/env bash

# Coverage Percentage Reconciliation
# Compares Results (coverage.sh global counts) vs Coverage table (per-file sums)
# using underlying line counts, not reverse-engineered percentages.

# CHANGELOG
# 1.6.0 - 2026-07-16 - Made the reconciliation self-consistent and deterministic: the
#                       "Results" (Method-1) global totals are now recomputed from the
#                       SAME gcov scan that produces the per-file list (applying
#                       DISCREPANCY exactly as coverage.sh does), instead of reading a
#                       separate, possibly-stale coverage_unity.txt.detailed. The per-file
#                       Δ columns now sum exactly to the recommended DISCREPANCY, which
#                       converges in one step; applying it makes instrumented AND covered
#                       totals match, so the message disappears. --redisplay is now honest
#                       (it no longer mixes a stale detailed file with the current constant).
# 1.5.0 - 2026-07-16 - Split the discrepancy into two honest parts: (1) instrumented
#                       (denominator) alignment via DISCREPANCY, recommended as
#                       table_inst - gcov_raw (converges in one step instead of chasing
#                       itself), only printed when instrumented totals still disagree;
#                       (2) covered (numerator) residual, reported separately as the real
#                       reason percentages differ and explicitly NOT DISCREPANCY-fixable.
# 1.4.0 - 2026-07-16 - Parse coverage_data.json with jq (per project rules) instead of
#                       line-oriented awk. The pretty-printed JSON puts file_path on a
#                       different line than its counts, so line-at-a-time awk read every
#                       file as 0/0 and always reported a spurious discrepancy (and a
#                       668-row garbage per-file list). Global totals and per-file
#                       contributors now come from jq. Also: recommend DISCREPANCY from
#                       raw instrumented-line alignment (not rounded percentages), and
#                       fixed the per-file printer (it reused $1..$8 from the last record,
#                       repeating one file 25x). Early-return now also checks raw totals.
# 1.3.0 - 2026-07-16 - Replaced "recent git files" hint with a per-file
#                       discrepancy breakdown (coverage.sh recomputed vs Table),
#                       listing each contributing file and its Unity/Blackbox line deltas;
#                       removed python3 (pure awk); fixed _table_coverage_totals to awk
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
# Usage: _table_coverage_totals <coverage_data.json> [awk]
# coverage_data.json is a pretty-printed JSON array (one object per file). The
# per-file counts live on different lines than file_path, so we parse it with jq
# (per project rules) rather than line-oriented awk, which would mis-read counts.
_table_coverage_totals() {
    local json_file="$1"
    if [[ ! -f "${json_file}" ]]; then
        echo "0,0,0,0,0,0"
        return 0
    fi
    local out
    out=$(jq -r '
        [
            ( [ .[].unity_covered       | select(type=="number") ] | add // 0 ),
            ( [ .[].unity_instrumented  | select(type=="number") ] | add // 0 ),
            ( [ .[].coverage_covered    | select(type=="number") ] | add // 0 ),
            ( [ .[].coverage_instrumented | select(type=="number") ] | add // 0 ),
            ( [ .[].combined_covered    | select(type=="number") ] | add // 0 ),
            ( [ .[].combined_instrumented | select(type=="number") ] | add // 0 )
        ] | @csv
    ' "${json_file}" 2>/dev/null | tr -d '"\r' | tr ',' '\n' | paste -sd ',' - 2>/dev/null) || true
    [[ -z "${out}" ]] && out="0,0,0,0,0,0"
    echo "${out}"
}

# Recommend a DISCREPANCY value so that coverage.sh's raw instrumented total
# (gcov_raw + D) lines up exactly with the coverage table's instrumented total.
# We align on RAW LINE COUNTS, not percentages: percentages are fine for display
# but two unequal counts can round to the same 3dp %, which would mask a real
# discrepancy. The DISCREPANCY exists because Unity-only #ifdef'd lines change
# the instrumented line count between the two builds.
# Usage: _find_discrepancy <results_covered> <gcov_raw> <table_covered> <table_inst>
_find_discrepancy() {
    local results_covered="$1"
    local gcov_raw="$2"
    local table_covered="$3"
    local table_inst="$4"

    if [[ "${results_covered}" -le 0 || "${table_covered}" -le 0 || "${table_inst}" -le 0 ]]; then
        echo "0"
        return 0
    fi

    # Align raw instrumented totals: D = table_inst - gcov_raw.
    local recommended
    recommended=$(( table_inst - gcov_raw ))
    if [[ ${recommended} -lt 0 ]]; then
        recommended=0
    fi
    echo "${recommended}"
}

# Recompute the coverage.sh (Method 1) per-file counts straight from the gcov
# files, so we can compare them against the per-file table (Method 2) counts in
# coverage_data.json. Method 1 mimics calculate_coverage_generic in coverage.sh:
# it concatenates every kept gcov file and counts covered/instrumented lines
# globally, but here we capture the per-file contribution instead of summing.
# Scan the Method-1 (coverage.sh) gcov files and emit per-file raw
# (instrumented) counts. Writes "file<TAB>side<TAB>covered<TAB>instrumented"
# records (one per file+side) to the file named by $3, and prints the four
# global raw sums "ucov utot bcov btot" to stdout. The global sums and the
# per-file rows come from the SAME scan, so they always agree — this is what
# makes the per-file Δ columns sum exactly to the global discrepancy.
# The filtering mirrors calculate_coverage_generic in coverage.sh exactly.
# Usage: _method1_perfile_rows <unity_dir> <blackbox_dir> <out_rows_file> [awk]
_method1_perfile_rows() {
    local unity_dir="$1"
    local blackbox_dir="$2"
    local out_file="$3"
    local awkcmd="${4:-awk}"

    # shellcheck disable=SC2154 # FIND/GREP assigned by framework.sh
    local -a unity_gcov=()
    # shellcheck disable=SC2154 # FIND/GREP assigned by framework.sh
    local -a blackbox_gcov=()
    if [[ -d "${unity_dir}" ]]; then
        # shellcheck disable=SC2154 # FIND/GREP assigned by framework.sh
        while IFS= read -r f; do
            [[ -z "${f}" ]] && continue
            local b
            b=$(basename "${f}")
            [[ "${b}" == "unity.c.gcov" ]] && continue
            [[ "${f}" == *"/usr/"* ]] && continue
            # System-library basename skips apply only outside src/ (project
            # sources like scripting/scoreboard_json.c must be kept).
            if [[ "${f}" != *"/src/"* ]]; then
                [[ "${b}" == *"jansson"* || "${b}" == *"mock"* || "${b}" == *"json"* || \
                   "${b}" == *"curl"* || "${b}" == *"ssl"* || "${b}" == *"pthread"* || \
                   "${b}" == *"uuid"* || "${b}" == *"zlib"* || "${b}" == *"pcre"* ]] && continue
            fi
            if "${GREP}" -q "Source:/usr/include/" "${f}" 2>/dev/null; then
                continue
            fi
            [[ "${b}" == "test_"* ]] && continue
            unity_gcov+=("${f}")
        done < <("${FIND}" "${unity_dir}" -name "*.c.gcov" -type f 2>/dev/null | "${GREP}" -v '_test' 2>/dev/null || true)
    fi
    if [[ -d "${blackbox_dir}" ]]; then
        while IFS= read -r f; do
            [[ -z "${f}" ]] && continue
            local b
            b=$(basename "${f}")
            [[ "${b}" == "unity.c.gcov" ]] && continue
            [[ "${f}" == *"/usr/"* ]] && continue
            if [[ "${f}" != *"/src/"* ]]; then
                [[ "${b}" == *"jansson"* || "${b}" == *"mock"* || "${b}" == *"json"* || \
                   "${b}" == *"curl"* || "${b}" == *"ssl"* || "${b}" == *"pthread"* || \
                   "${b}" == *"uuid"* || "${b}" == *"zlib"* || "${b}" == *"pcre"* ]] && continue
            fi
            blackbox_gcov+=("${f}")
        done < <("${FIND}" "${blackbox_dir}" -name "*.c.gcov" -type f 2>/dev/null | "${GREP}" -v '_test' 2>/dev/null || true)
    fi

    local -A seen_gcov=()
    local m1_rows_tmp
    m1_rows_tmp=$(mktemp) || { echo ""; return 0; }
    {
        for f in "${unity_gcov[@]}"; do
            local src
            src="${f#*/src/}"; src="${src%.gcov}"
            [[ -n "${seen_gcov[U${src}]-}" ]] && continue
            seen_gcov["U${src}"]=1
            "${awkcmd}" -v p="${src}" -v m="U" '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { c++; t++ }
                /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { t++ }
                END { print p "\t" m "\t" c+0 "\t" t+0 }
            ' "${f}" 2>/dev/null
        done
        for f in "${blackbox_gcov[@]}"; do
            local src
            src="${f#*/src/}"; src="${src%.gcov}"
            [[ -n "${seen_gcov[B${src}]-}" ]] && continue
            seen_gcov["B${src}"]=1
            "${awkcmd}" -v p="${src}" -v m="B" '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { c++; t++ }
                /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { t++ }
                END { print p "\t" m "\t" c+0 "\t" t+0 }
            ' "${f}" 2>/dev/null
        done
    } > "${m1_rows_tmp}"

    # Sum the raw per-file rows into the four global totals (single awk pass),
    # then write the per-file rows to the requested out_file.
    local sums
    # shellcheck disable=SC2016 # single-quoted AWK program is intentional
    sums=$("${awkcmd}" '
        $2 == "U" { uc += $3; ut += $4 }
        $2 == "B" { bc += $3; bt += $4 }
        END { printf "%d %d %d %d", uc+0, ut+0, bc+0, bt+0 }
    ' "${m1_rows_tmp}" 2>/dev/null)
    cat "${m1_rows_tmp}" > "${out_file}"
    rm -f "${m1_rows_tmp}"
    echo "${sums}"
}

# The per-file delta vs the table is what actually drives the global discrepancy,
# so we report the files that contribute to it and how many lines each adds.
# Method-1 per-file rows come from _method1_perfile_rows (the same scan the
# global "Results" totals use), so the per-file Δ columns sum exactly to the
# global discrepancy.
# Usage: _per_file_discrepancy <unity_dir> <blackbox_dir> <coverage_data.json> [awk]
_per_file_discrepancy() {
    local unity_dir="$1"
    local blackbox_dir="$2"
    local json_file="$3"
    local awkcmd="${4:-awk}"

    # No point listing contributors if we have no table to diff against.
    [[ -f "${json_file}" ]] || { return 0; }

    local tmp_method1
    tmp_method1=$(mktemp) || { echo ""; return 0; }
    # Build the Method-1 per-file rows (also discards the global sums on stdout).
    _method1_perfile_rows "${unity_dir}" "${blackbox_dir}" "${tmp_method1}" "${awkcmd}" >/dev/null

    # Build the Method-2 (table) side from coverage_data.json using jq. We parse
    # the array per-object so each file's file_path, unity and coverage counts are
    # read together (the pretty-printed JSON puts them on separate lines, which
    # breaks line-oriented awk). file_path may carry ANSI markup like {YELLOW}...;
    # strip it so we key on the bare source path.
    local tmp_method2
    tmp_method2=$(mktemp) || { rm -f "${tmp_method1}"; echo ""; return 0; }
    jq -r '
        .[] | .file_path as $fp
        | ($fp | gsub("\\{[A-Z]+\\}"; "")) as $clean
        | select($clean != "")
        | "\($clean)\tU\t\(.unity_covered // 0)\t\(.unity_instrumented // 0)",
          "\($clean)\tB\t\(.coverage_covered // 0)\t\(.coverage_instrumented // 0)"
    ' "${json_file}" 2>/dev/null > "${tmp_method2}" || true

    # Join the two sides, keep only files where the counts differ, sort by
    # absolute instrumented delta (largest contributors first) and print.
    # shellcheck disable=SC2154 # join/sort/awk are coreutils, no framework var
    {
        # shellcheck disable=SC2016 # single-quoted AWK program is intentional
        "${awkcmd}" '
            FNR==NR { m1[$1 "\t" $2] = $3 "\t" $4; next }
            {
                key = $1 "\t" $2
                mc = (key in m1) ? m1[key] : "0\t0"
                split(mc, a, "\t")
                m1c = a[1]+0; m1i = a[2]+0
                t2c = $3+0;  t2i = $4+0
                if (m1c != t2c || m1i != t2i) {
                    di = m1i - t2i
                    dc = m1c - t2c
                    printf "%s\t%s\t%s\t%s\t%s\t%s\t%d\t%d\n", \
                        $1, $2, m1c, m1i, t2c, t2i, dc, di
                }
            }
        ' "${tmp_method1}" "${tmp_method2}"
    } > "${tmp_method1}.diff" 2>/dev/null || true

    local diff_count
    # shellcheck disable=SC2154 # AWK assigned by framework.sh
    diff_count=$("${AWK}" 'END {print NR+0}' "${tmp_method1}.diff" 2>/dev/null || echo 0)

    if [[ "${diff_count}" -gt 0 ]]; then
        echo ""
        echo "Per-file contributors to the discrepancy (coverage.sh recomputed vs Table):"
        echo "  <file>  <side>  M1cov/M1inst  TableCov/TableInst  Δcov  Δinst"
        # Single AWK pass: sort by absolute instrumented delta, show top 25.
        # shellcheck disable=SC2016 # single-quoted AWK program is intentional
        "${AWK}" '
            {
                ad = ($8 < 0) ? -$8 : $8
                rows[NR] = $0
                adelta[NR] = ad
            }
            END {
                # insertion sort by descending absolute instrumented delta
                for (i = 1; i <= NR; i++) {
                    for (j = i; j > 1 && adelta[j] > adelta[j-1]; j--) {
                        t = adelta[j]; adelta[j] = adelta[j-1]; adelta[j-1] = t
                        tr = rows[j]; rows[j] = rows[j-1]; rows[j-1] = tr
                    }
                }
                limit = (NR < 25) ? NR : 25
                for (i = 1; i <= limit; i++) {
                    # Re-split the stored row; $1..$8 in END refer to the last
                    # record, not to rows[i], so parse the saved line explicitly.
                    n = split(rows[i], f, "\t")
                    fp = f[1]; side = f[2]; m1c = f[3]; m1i = f[4]
                    t2c = f[5]; t2i = f[6]; dc = f[7]; di = f[8]
                    sname = (side == "B") ? "Black" : "Unity"
                    dc_s = (dc < 0) ? dc : "+" dc
                    di_s = (di < 0) ? di : "+" di
                    printf "  %s  %s  %s/%s  %s/%s  %s  %s\n", \
                        fp, sname, m1c, m1i, t2c, t2i, dc_s, di_s
                }
                if (NR > 25) printf "  ... and %d more file(s)\n", NR - 25
            }
        ' "${tmp_method1}.diff" 2>/dev/null || true
    fi

    rm -f "${tmp_method1}" "${tmp_method2}" "${tmp_method1}.diff" 2>/dev/null || true
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

    # --- Results side (Method 1): recompute from the SAME gcov scan that backs
    # the per-file list, applying the current DISCREPANCY to the instrumented
    # totals exactly as coverage.sh does. This guarantees the per-file Δ columns
    # sum to these global figures (no cross-pass drift). ---
    local current_unity_disc current_coverage_disc
    current_unity_disc=$("${grepcmd}" "^DISCREPANCY_UNITY=" "${coverage_sh}" || true)
    current_unity_disc=$("${sedcmd}" 's/DISCREPANCY_UNITY=//' <<< "${current_unity_disc}" || echo "0")
    current_coverage_disc=$("${grepcmd}" "^DISCREPANCY_COVERAGE=" "${coverage_sh}" || true)
    current_coverage_disc=$("${sedcmd}" 's/DISCREPANCY_COVERAGE=//' <<< "${current_coverage_disc}" || echo "0")
    current_unity_disc=${current_unity_disc:-0}
    current_coverage_disc=${current_coverage_disc:-0}

    local unity_covered=0 unity_total=0
    local blackbox_covered=0 blackbox_total=0
    # shellcheck disable=SC2154 # BUILD_DIR assigned by framework.sh
    local unity_dir="${BUILD_DIR}/unity"
    # shellcheck disable=SC2154 # BUILD_DIR assigned by framework.sh
    local blackbox_dir="${BUILD_DIR}/coverage"
    if [[ -d "${unity_dir}" || -d "${blackbox_dir}" ]]; then
        local m1_rows
        m1_rows=$(mktemp) || { echo ""; return 0; }
        local m1_sums
        m1_sums=$(_method1_perfile_rows "${unity_dir}/src" "${blackbox_dir}/src" "${m1_rows}" "${awkcmd}")
        local ru_cov ru_inst rb_cov rb_inst
        read -r ru_cov ru_inst rb_cov rb_inst <<< "${m1_sums}"
        unity_covered=${ru_cov:-0}
        unity_total=$(( ru_inst + current_unity_disc ))
        blackbox_covered=${rb_cov:-0}
        blackbox_total=$(( rb_inst + current_coverage_disc ))
        rm -f "${m1_rows}"
    fi
    unity_covered=${unity_covered:-0}
    unity_total=${unity_total:-0}
    blackbox_covered=${blackbox_covered:-0}
    blackbox_total=${blackbox_total:-0}

    # --- Coverage table side: per-file gcov sums from coverage_data.json ---
    local table_totals
    table_totals=$(_table_coverage_totals "${results_dir}/coverage_data.json" "${awkcmd}")
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
        # Percentages happen to round the same, but the raw covered/instrumented
        # totals may still differ; the reconciliation decision is made below from
        # the raw counts (instrumented alignment vs covered residual).
        :
    fi

    # coverage.sh's reported total already includes the current DISCREPANCY, so
    # unity_total == gcov_raw + current_unity_disc. The DISCREPANCY constant only
    # adjusts the INSTRUMENTED denominator; it can never change the covered count.
    # gcov_raw is the unbiased instrumented line count coverage.sh measured.
    local unity_gcov_raw=$((unity_total - current_unity_disc))
    local blackbox_gcov_raw=$((blackbox_total - current_coverage_disc))

    # The DISCREPANCY value that makes coverage.sh's instrumented total line up
    # with the table is table_inst - gcov_raw. This is independent of the current
    # value, so applying it converges in one step (it does NOT chase itself).
    local new_unity_disc new_coverage_disc
    new_unity_disc=$(_find_discrepancy "${unity_covered}" "${unity_gcov_raw}" "${table_u_cov}" "${table_u_inst}")
    new_coverage_disc=$(_find_discrepancy "${blackbox_covered}" "${blackbox_gcov_raw}" "${table_b_cov}" "${table_b_inst}")

    # Does the current DISCREPANCY already reconcile the instrumented totals?
    local unity_inst_ok=0 blackbox_inst_ok=0
    [[ ${unity_total} -eq ${table_u_inst} ]] && unity_inst_ok=1
    [[ ${blackbox_total} -eq ${table_b_inst} ]] && blackbox_inst_ok=1

    # Does the covered (numerator) count already agree with the table?
    local unity_cov_ok=0 blackbox_cov_ok=0
    [[ ${unity_covered} -eq ${table_u_cov} ]] && unity_cov_ok=1
    [[ ${blackbox_covered} -eq ${table_b_cov} ]] && blackbox_cov_ok=1

    # Nothing to report if instrumented AND covered both agree.
    if [[ ${unity_inst_ok} -eq 1 && ${blackbox_inst_ok} -eq 1 && \
          ${unity_cov_ok} -eq 1 && ${blackbox_cov_ok} -eq 1 ]]; then
        return 0
    fi

    echo "Coverage count mismatch — Results (coverage.sh) vs Coverage table (per-file):"
    echo "  Unity:    Results ${unity_covered}/${unity_total} = ${results_u_pct}%   Table ${table_u_cov}/${table_u_inst} = ${table_u_pct}%   gcov_raw=${unity_gcov_raw}"
    echo "  Blackbox: Results ${blackbox_covered}/${blackbox_total} = ${results_b_pct}%   Table ${table_b_cov}/${table_b_inst} = ${table_b_pct}%   gcov_raw=${blackbox_gcov_raw}"
    echo "  Combined: Results ${results_c_pct}%   Table ${table_c_cov}/${table_c_inst} = ${table_c_pct}%"

    # DISCREPANCY only reconciles the instrumented (denominator) totals. Report it
    # only when those still disagree; the recommendation converges in one step.
    if [[ ${unity_inst_ok} -eq 0 || ${blackbox_inst_ok} -eq 0 ]]; then
        echo "Update tests/lib/coverage.sh DISCREPANCY (instrumented-line alignment only):"
        if [[ ${unity_inst_ok} -eq 0 ]]; then
            echo "  DISCREPANCY_UNITY=${new_unity_disc}  (was ${current_unity_disc})  → ${unity_gcov_raw}+${new_unity_disc}=${table_u_inst}"
        else
            echo "  DISCREPANCY_UNITY=${current_unity_disc}  (already aligned: ${unity_total}=${table_u_inst})"
        fi
        if [[ ${blackbox_inst_ok} -eq 0 ]]; then
            echo "  DISCREPANCY_COVERAGE=${new_coverage_disc}  (was ${current_coverage_disc})  → ${blackbox_gcov_raw}+${new_coverage_disc}=${table_b_inst}"
        else
            echo "  DISCREPANCY_COVERAGE=${current_coverage_disc}  (already aligned: ${blackbox_total}=${table_b_inst})"
        fi
    fi

    # Covered-count differences are NOT fixable by DISCREPANCY — they are the real
    # residual (Unity #ifdef paths etc.) and are what keeps the percentages apart.
    if [[ ${unity_cov_ok} -eq 0 || ${blackbox_cov_ok} -eq 0 ]]; then
        local unity_cov_delta=$((table_u_cov - unity_covered))
        local blackbox_cov_delta=$((table_b_cov - blackbox_covered))
        echo "Covered-line residual (not DISCREPANCY-fixable; drives the % gap):"
        [[ ${unity_cov_ok} -eq 0 ]] && echo "  Unity:    ${unity_covered} vs ${table_u_cov}  (Δ ${unity_cov_delta})"
        [[ ${blackbox_cov_ok} -eq 0 ]] && echo "  Blackbox: ${blackbox_covered} vs ${table_b_cov}  (Δ ${blackbox_cov_delta})"
    fi

    # List the source files that actually drive the discrepancy (per-file
    # Method1-vs-Table deltas) rather than just recently changed files.
    # shellcheck disable=SC2154 # BUILD_DIR assigned by framework.sh
    local unity_dir="${BUILD_DIR}/unity"
    local blackbox_dir="${BUILD_DIR}/coverage"
    if [[ -d "${unity_dir}" || -d "${blackbox_dir}" ]]; then
        _per_file_discrepancy "${unity_dir}/src" "${blackbox_dir}/src" "${results_dir}/coverage_data.json"
    fi
}
