#!/usr/bin/env bash

# Caller-site and mapping snapshots for the native RSS exercise (test 44).
# libheapmon.so records public malloc/free callers. /proc/pid/smaps records
# which mappings actually gained resident pages. Neither number replaces the
# RSS pass/fail; they explain it.

# CHANGELOG
# 1.0.0 - 2026-09-23 - Initial heap-site monitor: mallinfo2, smaps delta,
#                     live caller delta from midpoint to end.

# shellcheck disable=SC2154 # Caller sets TEST_NUMBER, TEST_COUNTER, METRICS_LOG, PROJECT_DIR, HYDROGEN_BIN, TIMESTAMP

[[ -n "${HEAPMON_GUARD:-}" ]] && return 0
export HEAPMON_GUARD="true"

HEAPMON_LIB_NAME="Heap Monitor"
HEAPMON_LIB_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${HEAPMON_LIB_NAME} ${HEAPMON_LIB_VERSION}" "info"

HEAPMON_ACTIVE=0
HEAPMON_NAME=""
HEAPMON_CTL=""
HEAPMON_REAL_BIN=""

heapmon_say() {
    local msg="$1"
    if [[ -n "${HEAPMON_REPORT_FILE:-}" ]]; then
        printf '%s\n' "${msg}" >> "${HEAPMON_REPORT_FILE}"
    fi
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${msg}"
}

heapmon_fmt_signed() {
    local n="$1"
    local sign="+"
    local whole frac a
    if [[ "${n}" -lt 0 ]]; then
        sign="-"
        a=$(( -n ))
    else
        a="${n}"
    fi
    if [[ "${a}" -ge 1048576 ]]; then
        whole=$(( a / 1048576 ))
        frac=$(( (a % 1048576) * 10 / 1048576 ))
        printf '%s%d.%d MB' "${sign}" "${whole}" "${frac}"
    elif [[ "${a}" -ge 1024 ]]; then
        whole=$(( a / 1024 ))
        frac=$(( (a % 1024) * 10 / 1024 ))
        printf '%s%d.%d KB' "${sign}" "${whole}" "${frac}"
    else
        printf '%s%d B' "${sign}" "${a}"
    fi
}

heapmon_bpr() {
    local delta="$1"
    local reqs="$2"
    if [[ ! "${reqs}" =~ ^[0-9]+$ ]] || [[ "${reqs}" -eq 0 ]]; then
        printf 'n/a\n'
        return 0
    fi
    printf '%s\n' "$(( delta / reqs ))"
}

heapmon_abs() {
    local n="$1"
    if [[ "${n}" -lt 0 ]]; then
        printf '%s\n' "$(( -n ))"
    else
        printf '%s\n' "${n}"
    fi
}

# Resolve a stripped or exported symbol. pc and off are hex without 0x.
heapmon_symbol() {
    local module="$1"
    local pc="$2"
    local off="$3"
    local known="$4"
    local kind="" addr="" line="" base=""
    if [[ -n "${known}" && "${known}" != "-" ]]; then
        printf '%s\n' "${known}"
        return 0
    fi
    base=$(basename "${module}" 2>/dev/null || printf '%s\n' "${module}")
    if [[ ! -f "${module}" ]] || ! command -v addr2line >/dev/null 2>&1; then
        printf '%s+0x%s\n' "${base}" "${off}"
        return 0
    fi
    if command -v readelf >/dev/null 2>&1; then
        kind=$(readelf -h "${module}" 2>/dev/null | awk '/Type:/ { print $2; exit }' || true)
    fi
    if [[ "${kind}" == "DYN" ]]; then
        addr="${off}"
    else
        addr="${pc}"
    fi
    line=$(addr2line -e "${module}" -f -C -p "0x${addr}" 2>/dev/null | head -n 1 || true)
    if [[ -z "${line}" || "${line}" == \?\?* ]]; then
        printf '%s+0x%s\n' "${base}" "${off}"
        return 0
    fi
    printf '%s\n' "${line}"
}

heapmon_compile() {
    local cc="$1"
    local dir="$2"
    local src_dir="$3"
    local log="${dir}/compile.log"

    if ! "${cc}" -std=c11 -Wall -Wextra -Wmissing-prototypes -Werror -shared -fPIC -pthread -O2 -g \
        -o "${dir}/libheapmon.so" "${src_dir}/heapmon.c" -ldl > "${log}" 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor failed to compile libheapmon.so (see ${log})"
        return 1
    fi
    if ! "${cc}" -std=c11 -Wall -Wextra -Werror -O2 -g \
        -o "${dir}/heapmon_ctl" "${src_dir}/heapmon_ctl.c" > "${log}" 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor failed to compile heapmon_ctl (see ${log})"
        return 1
    fi
    if ! "${cc}" -std=c11 -Wall -Wextra -Werror -O0 -g -fno-inline -fno-optimize-sibling-calls -rdynamic \
        -o "${dir}/heapmon_check" "${src_dir}/heapmon_check.c" -pthread > "${log}" 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor failed to compile heapmon_check (see ${log})"
        return 1
    fi
    return 0
}

# Build the preload, prove it attributes known leaks, and point HYDROGEN_BIN
# at a launcher that sets LD_PRELOAD only for the server process.
# Always returns 0. HEAPMON_ACTIVE=1 only when the self-test passed.
heapmon_prepare() {
    local real_bin="$1"
    local cc="" src_dir="" dir="" so="" ctl="" check_bin="" launcher=""
    local check_name="" check_log=""

    HEAPMON_ACTIVE=0
    HEAPMON_REAL_BIN="${real_bin}"
    if [[ "${EXERCISE_NATIVE_HEAPMON:-1}" == "0" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor off (EXERCISE_NATIVE_HEAPMON=0); RSS and /proc mappings only"
        return 0
    fi
    if command -v cc >/dev/null 2>&1; then
        cc="cc"
    elif command -v gcc >/dev/null 2>&1; then
        cc="gcc"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor skipped: no C compiler (smaps mapping delta still runs)"
        return 0
    fi

    src_dir="$(dirname "${BASH_SOURCE[0]}")/../heapmon"
    dir="${PROJECT_DIR}/build/tests/heapmon"
    mkdir -p "${dir}" || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor skipped: cannot create ${dir}"
        return 0
    }
    so="${dir}/libheapmon.so"
    ctl="${dir}/heapmon_ctl"
    check_bin="${dir}/heapmon_check"

    if [[ ! -x "${so}" || "${src_dir}/heapmon.c" -nt "${so}" \
        || ! -x "${ctl}" || "${src_dir}/heapmon_ctl.c" -nt "${ctl}" \
        || ! -x "${check_bin}" || "${src_dir}/heapmon_check.c" -nt "${check_bin}" ]]; then
        # shellcheck disable=SC2310 # Compile failure falls back to smaps-only
        if ! heapmon_compile "${cc}" "${dir}" "${src_dir}"; then
            return 0
        fi
    fi

    check_name="h44c-${TIMESTAMP}-$$"
    check_log="${dir}/selftest.log"
    if command -v timeout >/dev/null 2>&1; then
        # shellcheck disable=SC2310 # A failed self-test must not abort the RSS exercise
        if ! timeout 30 env HEAPMON_NAME="${check_name}" LD_PRELOAD="${so}" "${check_bin}" > "${check_log}" 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor self-test failed; caller sites disabled (see ${check_log})"
            return 0
        fi
    else
        # shellcheck disable=SC2310 # A failed self-test must not abort the RSS exercise
        if ! env HEAPMON_NAME="${check_name}" LD_PRELOAD="${so}" "${check_bin}" > "${check_log}" 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor self-test failed; caller sites disabled (see ${check_log})"
            return 0
        fi
    fi

    HEAPMON_NAME="h44-${TIMESTAMP}-$$"
    HEAPMON_CTL="${ctl}"
    launcher="${dir}/launch-${TIMESTAMP}-$$.sh"
    cat > "${launcher}" << EOF
#!/usr/bin/env bash
set -euo pipefail
if [[ -n "\${LD_PRELOAD:-}" ]]; then
    export LD_PRELOAD="${so}:\${LD_PRELOAD}"
else
    export LD_PRELOAD="${so}"
fi
export HEAPMON_NAME="${HEAPMON_NAME}"
exec -a "$(basename "${real_bin}")" "${real_bin}" "\$@"
EOF
    chmod +x "${launcher}" || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor skipped: cannot install launcher"
        return 0
    }
    export HYDROGEN_BIN="${launcher}"
    HEAPMON_ACTIVE=1
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap monitor on: live caller sites plus glibc in-use versus RSS. Mappings come from /proc/smaps either way."
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Monitor tables are faulted in at startup, so they sit in warmup RSS rather than the per-request slope. EXERCISE_NATIVE_HEAPMON=0 skips the preload."
    return 0
}

heapmon_write_smaps() {
    local pid="$1"
    local out="$2"
    if [[ ! -r "/proc/${pid}/smaps" ]]; then
        return 0
    fi
    awk '
        $1 ~ /^[0-9a-f]+-[0-9a-f]+$/ {
            if (NF >= 6) {
                label = $6
                for (i = 7; i <= NF; i++) {
                    label = label " " $i
                }
            } else {
                label = "[anon]"
            }
            next
        }
        $1 == "Rss:" {
            rss[label] += $2
            next
        }
        END {
            for (k in rss) {
                printf "%s\t%d\n", k, rss[k] * 1024
            }
        }
    ' "/proc/${pid}/smaps" > "${out}" || true
}

# tag is init, mid, or end. Writes ${METRICS_LOG}.smaps-TAG and, when the
# preload is active, ${METRICS_LOG}.heap-TAG. Never fails the caller.
heapmon_capture() {
    local tag="$1"
    local pid="$2"
    local heap=""
    heapmon_write_smaps "${pid}" "${METRICS_LOG}.smaps-${tag}"
    if [[ "${HEAPMON_ACTIVE}" != "1" || -z "${HEAPMON_CTL}" || -z "${HEAPMON_NAME}" ]]; then
        return 0
    fi
    heap="${METRICS_LOG}.heap-${tag}"
    if command -v timeout >/dev/null 2>&1; then
        # shellcheck disable=SC2310 # A missed dump is reported and the RSS run continues
        if ! timeout 30 "${HEAPMON_CTL}" "${HEAPMON_NAME}" > "${heap}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap dump (${tag}) failed"
            rm -f "${heap}"
        fi
    else
        # shellcheck disable=SC2310 # A missed dump is reported and the RSS run continues
        if ! "${HEAPMON_CTL}" "${HEAPMON_NAME}" > "${heap}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Heap dump (${tag}) failed"
            rm -f "${heap}"
        fi
    fi
    return 0
}

heapmon_report_mappings() {
    local mid="$1"
    local end="$2"
    local diff="$3"
    local shown=0
    local dbytes="" name="" formatted=""
    if [[ ! -s "${mid}" || ! -s "${end}" ]]; then
        heapmon_say "Mapping delta unavailable (smaps snapshot missing)"
        return 0
    fi
    awk -F '\t' '
        NR == FNR { mid[$1] = $2; next }
        { end[$1] = $2 }
        END {
            for (k in mid) {
                if (!(k in end)) {
                    end[k] = 0
                }
            }
            for (k in end) {
                m = (k in mid) ? mid[k] : 0
                d = end[k] - m
                if (d != 0) {
                    printf "%d\t%s\n", d, k
                }
            }
        }
    ' "${mid}" "${end}" | sort -t $'\t' -k1,1nr > "${diff}" || true
    heapmon_say "Where resident pages moved (midpoint to end):"
    while IFS=$'\t' read -r dbytes name; do
        [[ -n "${dbytes}" ]] || continue
        if [[ "${dbytes}" -le 0 ]]; then
            continue
        fi
        shown=$(( shown + 1 ))
        if [[ "${shown}" -gt 6 ]]; then
            break
        fi
        formatted=$(heapmon_fmt_signed "${dbytes}")
        heapmon_say "  ${formatted}  ${name}"
    done < "${diff}"
    if [[ "${shown}" -eq 0 ]]; then
        heapmon_say "  no mapping gained resident pages"
    fi
}

heapmon_report_sites() {
    local mid="$1"
    local end="$2"
    local diff="$3"
    local half="$4"
    local rss_bpr="$5"
    local uord_mid=0 uord_end=0 ford_mid=0 ford_end=0
    local tracked_mid=0 tracked_end=0 untracked_mid=0 untracked_end=0
    local uord_delta=0 ford_delta=0 tracked_delta=0
    local uord_bpr="" tracked_bpr="" ford_bpr=""
    local rss_txt=""
    local shown=0 covered=0
    local dbytes="" dcount="" ealloc="" efree="" pc="" off="" module="" symbol=""
    local formatted="" resolved="" base="" stripped="0"

    if [[ ! -s "${mid}" || ! -s "${end}" ]]; then
        heapmon_say "Caller sites unavailable (heap dump missing). Mapping list above is still the where."
        return 0
    fi
    if ! awk '$0 == "END" { found = 1; exit } END { exit !found }' "${end}"; then
        heapmon_say "Caller dump was truncated; site list may be partial"
    fi

    uord_mid=$(awk -F '\t' '$1 == "MALLINFO" { print $3; exit }' "${mid}" || true)
    uord_end=$(awk -F '\t' '$1 == "MALLINFO" { print $3; exit }' "${end}" || true)
    ford_mid=$(awk -F '\t' '$1 == "MALLINFO" { print $5; exit }' "${mid}" || true)
    ford_end=$(awk -F '\t' '$1 == "MALLINFO" { print $5; exit }' "${end}" || true)
    tracked_mid=$(awk -F '\t' '$1 == "TOTALS" { print $3; exit }' "${mid}" || true)
    tracked_end=$(awk -F '\t' '$1 == "TOTALS" { print $3; exit }' "${end}" || true)
    untracked_mid=$(awk -F '\t' '$1 == "TOTALS" { print $7; exit }' "${mid}" || true)
    untracked_end=$(awk -F '\t' '$1 == "TOTALS" { print $7; exit }' "${end}" || true)
    [[ "${uord_mid}" =~ ^[0-9]+$ ]] || uord_mid=0
    [[ "${uord_end}" =~ ^[0-9]+$ ]] || uord_end=0
    [[ "${ford_mid}" =~ ^[0-9]+$ ]] || ford_mid=0
    [[ "${ford_end}" =~ ^[0-9]+$ ]] || ford_end=0
    [[ "${tracked_mid}" =~ ^-?[0-9]+$ ]] || tracked_mid=0
    [[ "${tracked_end}" =~ ^-?[0-9]+$ ]] || tracked_end=0
    [[ "${untracked_mid}" =~ ^[0-9]+$ ]] || untracked_mid=0
    [[ "${untracked_end}" =~ ^[0-9]+$ ]] || untracked_end=0

    uord_delta=$(( uord_end - uord_mid ))
    ford_delta=$(( ford_end - ford_mid ))
    tracked_delta=$(( tracked_end - tracked_mid ))
    uord_bpr=$(heapmon_bpr "${uord_delta}" "${half}")
    ford_bpr=$(heapmon_bpr "${ford_delta}" "${half}")
    tracked_bpr=$(heapmon_bpr "${tracked_delta}" "${half}")
    local uord_txt="" ford_txt="" tracked_txt=""
    uord_txt=$(heapmon_fmt_signed "${uord_delta}")
    ford_txt=$(heapmon_fmt_signed "${ford_delta}")
    tracked_txt=$(heapmon_fmt_signed "${tracked_delta}")

    heapmon_say "glibc heap in use ${uord_bpr} B/req (${uord_txt}), free in heap ${ford_bpr} B/req (${ford_txt}), tracked live callers ${tracked_bpr} B/req (${tracked_txt})"

    if [[ "${uord_bpr}" =~ ^-?[0-9]+$ && "${tracked_bpr}" =~ ^-?[0-9]+$ ]]; then
        local rss_delta=0
        local uord_abs_b=0
        local tracked_abs_b=0
        uord_abs_b=$(heapmon_abs "${uord_delta}")
        tracked_abs_b=$(heapmon_abs "${tracked_delta}")
        if [[ "${rss_bpr}" =~ ^-?[0-9]+$ && "${half}" =~ ^[0-9]+$ && "${half}" -gt 0 ]]; then
            rss_delta=$(( rss_bpr * half ))
        fi
        if [[ "${rss_delta}" -ge 8192 && "${uord_abs_b}" -lt $(( rss_delta / 4 )) && "${tracked_abs_b}" -lt $(( rss_delta / 4 )) ]]; then
            rss_txt=$(heapmon_fmt_signed "${rss_delta}")
            heapmon_say "Live heap grew ${uord_txt} and tracked callers grew ${tracked_txt}, against about ${rss_txt} of RSS. The slope is retained address space (free pages or a non-malloc mapping). Callers below are leftovers, not the slope."
        elif [[ "${uord_delta}" -gt 8192 && "${tracked_delta}" -ge $(( uord_delta / 2 )) ]]; then
            heapmon_say "Live glibc heap grew with the callers below. Those callers still hold the bytes at the end of the run."
        elif [[ "${uord_delta}" -gt 8192 && "${tracked_abs_b}" -lt $(( uord_abs_b / 2 )) ]]; then
            heapmon_say "glibc in-use grew faster than the tracked callers. The rest was allocated inside libc or by a private arena that does not call public malloc."
        elif [[ "${uord_abs_b}" -lt 8192 && "${tracked_abs_b}" -lt 8192 ]]; then
            heapmon_say "Live allocations are flat across the second half."
        else
            heapmon_say "Caller bytes and glibc in-use diverged from RSS. The mapping list is the page-level where; the sites below are still-live public mallocs."
        fi
    fi

    if [[ "${untracked_end}" -gt "${untracked_mid}" ]]; then
        heapmon_say "Pointer table missed $(( untracked_end - untracked_mid )) allocations in the second half; caller totals are short by that many blocks."
    fi

    awk -F '\t' '
        NR == FNR {
            if ($1 == "SITE") {
                mid_bytes[$2] = $4
                mid_count[$2] = $5
                mid_off[$2] = $3
                mid_mod[$2] = $8
                mid_sym[$2] = $9
            }
            next
        }
        $1 == "SITE" {
            end_bytes[$2] = $4
            end_count[$2] = $5
            end_alloc[$2] = $6
            end_free[$2] = $7
            end_off[$2] = $3
            end_mod[$2] = $8
            end_sym[$2] = $9
        }
        END {
            for (pc in mid_bytes) {
                seen[pc] = 1
            }
            for (pc in end_bytes) {
                seen[pc] = 1
            }
            for (pc in seen) {
                mb = (pc in mid_bytes) ? mid_bytes[pc] : 0
                eb = (pc in end_bytes) ? end_bytes[pc] : 0
                mc = (pc in mid_count) ? mid_count[pc] : 0
                ec = (pc in end_count) ? end_count[pc] : 0
                dbytes = eb - mb
                dcount = ec - mc
                if (dbytes == 0) {
                    continue
                }
                ealloc = (pc in end_alloc) ? end_alloc[pc] : 0
                efree = (pc in end_free) ? end_free[pc] : 0
                off = (pc in end_off) ? end_off[pc] : mid_off[pc]
                mod = (pc in end_mod) ? end_mod[pc] : mid_mod[pc]
                sym = (pc in end_sym) ? end_sym[pc] : mid_sym[pc]
                printf "%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\n", dbytes, dcount, ealloc, efree, pc, off, mod, sym
            }
        }
    ' "${mid}" "${end}" | sort -t $'\t' -k1,1nr > "${diff}" || true

    heapmon_say "Who still holds more than they did at the midpoint (public malloc):"
    while IFS=$'\t' read -r dbytes dcount ealloc efree pc off module symbol; do
        [[ -n "${dbytes}" ]] || continue
        if [[ "${dbytes}" -le 0 ]]; then
            continue
        fi
        shown=$(( shown + 1 ))
        if [[ "${shown}" -gt 8 ]]; then
            break
        fi
        covered=$(( covered + dbytes ))
        formatted=$(heapmon_fmt_signed "${dbytes}")
        resolved=$(heapmon_symbol "${module}" "${pc}" "${off}" "${symbol}")
        base=$(basename "${module}" 2>/dev/null || printf '%s\n' "${module}")
        if [[ "${symbol}" == "-" && -n "${HEAPMON_REAL_BIN}" && "${module}" == "${HEAPMON_REAL_BIN}" ]]; then
            stripped="1"
        fi
        heapmon_say "  ${formatted} (${dcount} blocks; lifetime ${ealloc} alloc / ${efree} free)  ${base}  ${resolved}"
    done < "${diff}"
    if [[ "${shown}" -eq 0 ]]; then
        heapmon_say "  no caller gained live bytes in the second half"
    elif [[ "${tracked_delta}" -gt 0 && "${covered}" -lt "${tracked_delta}" ]]; then
        local covered_txt="" delta_txt=""
        covered_txt=$(heapmon_fmt_signed "${covered}")
        delta_txt=$(heapmon_fmt_signed "${tracked_delta}")
        heapmon_say "  top callers cover ${covered_txt} of ${delta_txt} tracked live growth; the rest is spread across smaller sites"
    fi
    if [[ "${stripped}" == "1" ]]; then
        heapmon_say "Hydrogen itself is stripped in this binary, so its sites are module+offset. Re-run with EXERCISE_NATIVE_DIAG=1 (hydrogen_perf, symbols) to name those functions. Library sites are named either way. RSS from the perf binary is not the release number."
    fi
}

# half_reqs and rss_bpr describe the steady-state window. Either may be empty
# when the RSS sample itself was incomplete; the snapshots are still reported.
heapmon_report() {
    local half="${1:-0}"
    local rss_bpr="${2:-}"
    local mid_s="${METRICS_LOG}.smaps-mid"
    local end_s="${METRICS_LOG}.smaps-end"
    local mid_h="${METRICS_LOG}.heap-mid"
    local end_h="${METRICS_LOG}.heap-end"
    local diff_dir=""

    diff_dir="$(dirname "${METRICS_LOG}")"
    HEAPMON_REPORT_FILE="${METRICS_LOG}.heap-report"
    : > "${HEAPMON_REPORT_FILE}" || true

    if [[ "${half}" =~ ^[0-9]+$ ]] && [[ "${half}" -gt 0 ]]; then
        heapmon_say "Heap analysis over the last ${half} requests (RSS ${rss_bpr:-n/a} B/req is the pass/fail number):"
    else
        heapmon_say "Heap analysis (steady-state window incomplete):"
    fi
    heapmon_report_mappings "${mid_s}" "${end_s}" "${diff_dir}/smaps.diff"
    heapmon_report_sites "${mid_h}" "${end_h}" "${diff_dir}/sites.diff" "${half}" "${rss_bpr}"
    return 0
}
