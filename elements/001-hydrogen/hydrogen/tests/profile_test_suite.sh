#!/usr/bin/env bash
#
# profile_test_suite.sh
# Profile fork/execve activity for a Hydrogen test script under strace.
#
# Default target is the full suite (test_00_all.sh). Pass any test_NN_*.sh
# script to profile a single test instead, so changes can be validated quickly
# without re-running the entire suite:
#
#   ./profile_test_suite.sh                       profile the full suite
#   ./profile_test_suite.sh test_40_auth.sh       profile one test
#   ./profile_test_suite.sh --timeout 600 ...     cap the strace run (seconds)
#   ./profile_test_suite.sh --skip-email ...      skip the make-email/mutt tail
#   ./profile_test_suite.sh --mono ...            disable ANSI colours
#   ./profile_test_suite.sh --help
#
# Output files (dated so re-runs never clobber one another or stray *.txt):
#   profile_trace-<DATE>.txt    raw strace output
#   profile_summary-<DATE>.txt  rendered summary (tables; mono archive)
#   profile_error-<DATE>.txt    diagnostics + strace stderr
#
# CHANGELOG
# 1.0.0 - 2026-08-19 - Rewrite. Fixes:
#   * `rm -f ./*.txt` used to wipe the error-log header (and any other *.txt)
#     because it ran *after* the header was written. Cleanup is now scoped to
#     legacy non-dated profile outputs only.
#   * strace -f could hang indefinitely on background processes that outlive
#     the suite (make-email/mutt on SMTP, hbm_browser, orphaned servers). A
#     watchdog now bounds the run and kills the whole traced process group.
#   * Summary is now rendered with the project `tables` executable (Blue
#     theme), with a plain-text fallback.
#   * Individual-test profiling and --skip-email/--timeout/--mono flags added;
#     --help documents usage.

set -o pipefail

# --- Toolchain ----------------------------------------------------------
DATE_BIN=$(command -v date)
GREP_BIN=$(command -v ggrep 2>/dev/null || command -v grep)
TABLES_BIN=$(command -v tables)
STRACE_BIN=$(PATH=/usr/bin command -v strace)
TIMEOUT_BIN=$(command -v timeout)
SETSID_BIN=$(command -v setsid)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Record the profiling start time (seconds + human string) for the title.
PROFILE_START_EPOCH=$("${DATE_BIN}" +%s 2>/dev/null || echo 0)
PROFILE_START_HUMAN=$("${DATE_BIN}" '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || true)

# --- Defaults -----------------------------------------------------------
DEFAULT_SCRIPT="test_00_all.sh"
PROFILE_TIMEOUT="${PROFILE_TIMEOUT:-3600}"
MONO=0
SKIP_EMAIL=0
TEST_SCRIPT=""
EXTRA_ARGS=()
STRACE_PID=""
WATCHDOG_PID=""
TMPDIR_PROFILE=""

cleanup() {
    if [[ -n "${WATCHDOG_PID}" ]]; then
        kill "${WATCHDOG_PID}" 2>/dev/null || true
    fi
    if [[ -n "${STRACE_PID}" ]]; then
        kill -TERM -"${STRACE_PID}" 2>/dev/null || true
        kill -KILL -"${STRACE_PID}" 2>/dev/null || true
    fi
    [[ -n "${TMPDIR_PROFILE}" ]] && rm -rf "${TMPDIR_PROFILE}" || true
    rm -f "${SCRIPT_DIR}/.profile_watchdog_fired" 2>/dev/null || true
}
trap cleanup EXIT

usage() {
    cat <<EOF
Usage: $(basename "$0") [--help] [--timeout SECONDS] [--skip-email] [--mono] [test_script]

Profile fork/execve activity for a Hydrogen test script under strace.

  (no argument)        Profile the full suite (test_00_all.sh)
  test_script          Profile an individual test, e.g. test_40_auth.sh
  --timeout N          Hard cap (seconds) on the strace run (default: ${PROFILE_TIMEOUT})
  --skip-email         Set HYDROGEN_DISABLE_EMAIL=1 so the suite's make-email
                       (mutt) tail is skipped — handy while profiling under strace
  --mono               Disable ANSI colours in the rendered summary
  --help, -h           Show this help

Environment:
  PROFILE_TIMEOUT      Overrides the default --timeout value
EOF
}

# --- Argument parsing ---------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --timeout)
            [[ $# -ge 2 ]] || { echo "Error: --timeout requires a value" >&2; exit 2; }
            PROFILE_TIMEOUT="$2"; shift 2 ;;
        --timeout=*) PROFILE_TIMEOUT="${1#*=}"; shift ;;
        --skip-email) SKIP_EMAIL=1; shift ;;
        --mono) MONO=1; shift ;;
        --) shift; break ;;
        -*) echo "Error: unknown option: $1" >&2; usage >&2; exit 2 ;;
        *)  if [[ -z "${TEST_SCRIPT}" ]]; then TEST_SCRIPT="$1"; else EXTRA_ARGS+=("$1"); fi; shift ;;
    esac
done

TEST_SCRIPT="${TEST_SCRIPT:-${DEFAULT_SCRIPT}}"
if [[ ${#EXTRA_ARGS[@]} -gt 0 ]]; then
    echo "Warning: ignoring extra arguments: ${EXTRA_ARGS[*]}" >&2
fi

echo "" >&2
echo "Profiling Summary for ${TEST_SCRIPT} on $("${DATE_BIN}" || true)" >&2

# --- Pre-flight ---------------------------------------------------------
if [[ -z "${STRACE_BIN}" ]]; then
    echo "Error: strace not found in /usr/bin" >&2
    exit 1
fi
if [[ -z "${GREP_BIN}" ]]; then
    echo "Error: grep not found" >&2
    exit 1
fi

# Resolve the target script (accept a bare name in cwd or a path).
RUN_SCRIPT=""
for candidate in "${TEST_SCRIPT}" "./${TEST_SCRIPT}" "${SCRIPT_DIR}/${TEST_SCRIPT}"; do
    if [[ -x "${candidate}" ]]; then
        RUN_SCRIPT="${candidate}"
        break
    fi
done
if [[ -z "${RUN_SCRIPT}" ]]; then
    echo "Error: ${TEST_SCRIPT} not found or not executable" >&2
    exit 1
fi

# strace execs the target via execvp: a bare name (e.g. test_40_auth.sh) is
# looked up on PATH and not found. Prefix bare names with ./ so the target is
# located relative to cwd, matching the original ./${TEST_SCRIPT} behaviour.
case "${RUN_SCRIPT}" in
    */*) : ;;
    *)   RUN_SCRIPT="./${RUN_SCRIPT}" ;;
esac

# --- Output files (dated: never clobber a prior run's archives) -----------
RUN_DATE_TAG=$("${DATE_BIN}" +%Y%m%d)
TRACE_OUT="profile_trace-${RUN_DATE_TAG}.txt"
SUMMARY_OUT="profile_summary-${RUN_DATE_TAG}.txt"
ERROR_LOG="profile_error-${RUN_DATE_TAG}.txt"

# Only ever touch our own legacy non-dated outputs. The old `rm -f ./*.txt`
# wiped the error-log header (written just above) and could destroy unrelated
# *.txt files; dated outputs make a blanket clean-up unnecessary.
rm -f profile_trace.txt profile_summary.txt profile_error.txt

# Diagnostics header (written first, never wiped by a later rm).
{
    echo "Profiling run started at $("${DATE_BIN}" || true)"
    echo "Test script: ${RUN_SCRIPT}"
    echo "Timeout: ${PROFILE_TIMEOUT}s  skip-email: ${SKIP_EMAIL}"
    echo "System limits: $(ulimit -n 2>/dev/null || true), Memory: $(free -m 2>/dev/null | "${GREP_BIN}" Mem || true)"
    echo "strace: ${STRACE_BIN}, setsid: ${SETSID_BIN:-n/a}, tables: ${TABLES_BIN:-n/a}"
} > "${ERROR_LOG}"

if [[ "${SKIP_EMAIL}" -eq 1 ]]; then
    export HYDROGEN_DISABLE_EMAIL=1
fi

# --- Run strace (bounded; whole traced tree is killed on timeout) --------
# strace -f follows every fork. Background daemons spawned by the suite
# (make-email/mutt hanging on SMTP, hbm_browser, orphaned hydrogen/mailval
# servers) can keep strace alive long after the script "finishes", which is
# exactly the stall observed in the 2026-08-19 run. setsid isolates the tree
# in its own process group so the watchdog can kill strace AND every forked
# descendant atomically when the timeout fires.
echo "Running strace on ${RUN_SCRIPT}..." >> "${ERROR_LOG}"
STRACE_STATUS=0
STRACE_TIMED_OUT=0

if [[ -n "${SETSID_BIN}" && -n "${TIMEOUT_BIN}" ]]; then
    "${SETSID_BIN}" "${STRACE_BIN}" -f -e trace=fork,execve \
        -o "${TRACE_OUT}" "${RUN_SCRIPT}" 2>> "${ERROR_LOG}" &
    STRACE_PID=$!
    (
        sleep "${PROFILE_TIMEOUT}"
        echo "watchdog" > "${SCRIPT_DIR}/.profile_watchdog_fired"
        kill -TERM -"${STRACE_PID}" 2>/dev/null || true
        sleep 5
        kill -KILL -"${STRACE_PID}" 2>/dev/null || true
    ) &
    WATCHDOG_PID=$!
    wait "${STRACE_PID}" 2>/dev/null
    STRACE_STATUS=$?
    # Allow strace to finish flushing the trace file and any lingering tracees
    # (forked daemons that may still be writing) to settle. Without this, the
    # post-hoc grep sees a partial trace.
    sleep 1
    if [[ -f "${SCRIPT_DIR}/.profile_watchdog_fired" ]]; then
        STRACE_TIMED_OUT=1
        rm -f "${SCRIPT_DIR}/.profile_watchdog_fired"
        echo "strace timed out after ${PROFILE_TIMEOUT}s; process group ${STRACE_PID} terminated." >> "${ERROR_LOG}"
    fi
    kill "${WATCHDOG_PID}" 2>/dev/null || true
    wait "${WATCHDOG_PID}" 2>/dev/null
else
    # Fallback: bare timeout (no group kill; tracees may survive a timeout).
    "${TIMEOUT_BIN:-}" -k 10 "${PROFILE_TIMEOUT}" "${STRACE_BIN}" -f -e trace=fork,execve \
        -o "${TRACE_OUT}" "${RUN_SCRIPT}" 2>> "${ERROR_LOG}"
    STRACE_STATUS=$?
    [[ "${STRACE_STATUS}" -ne 0 ]] && STRACE_TIMED_OUT=1
fi

if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
    echo "Warning: strace did not exit cleanly (status ${STRACE_STATUS}; timed out). Summary is from a partial trace." >> "${ERROR_LOG}"
fi

# --- Tally execve invocations -------------------------------------------
# Count distinct execve *invocations*: the initial `execve("PATH", ["argv0"...`
# line, excluding strace's `<... execve resumed>)` continuation lines (which
# appear when a prior execve was <unfinished ...>). Counting only the initial
# lines keeps TOTAL_EXEC consistent with the per-command breakdown and the
# "Other" bucket below.
TOTAL_EXEC=$("${GREP_BIN}" -cE '^[0-9]+ execve\(' "${TRACE_OUT}" 2>/dev/null || true)
TOTAL_EXEC=${TOTAL_EXEC:-0}

# Parallel arrays preserve the exact grep patterns and counts from the
# original script so the numbers stay comparable across runs.
COUNT_LABELS=( hydrogen bash sh xargs cat find bc tr wc date md5sum \
               mkdir mktemp realpath basename dirname du rm \
               grep sed awk curl \
               cmake make cc gcov \
               cppcheck shellcheck markdownlint jsonlint eslint stylelint htmlhint \
               cloc tables \
               flock head cp sleep tail sort which jsonschema-cli )
COUNT_CATS=(  Hydrogen Shell Shell Shell SysUtils SysUtils SysUtils SysUtils SysUtils SysUtils SysUtils \
               PathTools PathTools PathTools PathTools PathTools PathTools PathTools \
               TextTools TextTools TextTools TextTools \
               Build Build Build Build \
               Lint Lint Lint Lint Lint Lint Lint \
               Reporting Reporting \
               Misc Misc Misc Misc Misc Misc Misc Misc )
counts=()
# Build a precise argv[0]-anchored pattern per label from the label itself.
# strace format: execve("PATH", ["ARGV0", ...]) — matching on the 2nd quoted
# token (argv[0]) avoids false positives when a binary NAME appears in arguments
# or env of another command (e.g. `which jsonschema-cli`, or `jsonschema-cli`
# echoed in bash's PATH-lookup env block).
# argv[0] may be a bare name ("bash"), a relative path ("./hydrogen_debug"), or
# absolute ("/usr/bin/grep"). So we match the argv[0] field as ending in the
# label, with an optional path prefix: ["[^"]*LABEL".
for label in "${COUNT_LABELS[@]}"; do
    # Escape any regex-special chars in the label (e.g. - in jsonschema-cli).
    # Use double quotes so backslash sequences are interpreted by sed.
    esc_label=$(printf '%s' "${label}" | sed "s#[.[\*^$()+?{|\\]#\\\\&#g")
    if [[ "${label}" == "hydrogen" ]]; then
        pattern='execve\("[^"]*hydrogen/hydrogen", \["hydrogen"'
    else
        pattern='execve\("[^"]*'${esc_label}'", \["[^"]*'${esc_label}'"'
    fi
    c=$("${GREP_BIN}" -cE "${pattern}" "${TRACE_OUT}" 2>/dev/null || true)
    c=${c:-0}
    counts+=("${c}")
done

# Hydrogen variants: the build produces several binaries (hydrogen,
# hydrogen_coverage, hydrogen_debug, hydrogen_naked, hydrogen_perf,
# hydrogen_release, hydrogen_valgrind). The generic `hydrogen` pattern above
# only catches the bare name, so enumerate each variant into its own row so a
# zero for one variant is visible rather than merged into the main bucket.
HYDROGEN_VARIANTS=( hydrogen_coverage hydrogen_debug hydrogen_naked \
                    hydrogen_perf hydrogen_release hydrogen_valgrind )
HYDRO_VARIANT_COUNTS=()
for v in "${HYDROGEN_VARIANTS[@]}"; do
    # argv[0] may be "./hydrogen_debug" or "/build/hydrogen_debug"; match the
    # basename at the end of the argv[0] field (2nd quoted token).
    # shellcheck disable=SC2086 # ${v} is intentionally unquoted: spliced into regex
    hydro_pat='execve\("[^"]*'${v}'", \["[^"]*'${v}'"'
    c=$("${GREP_BIN}" -cE "${hydro_pat}" "${TRACE_OUT}" 2>/dev/null || true)
    c=${c:-0}
    HYDRO_VARIANT_COUNTS+=("${c}")
done

# --- Render summary via `tables` (plain-text fallback) ------------------
# Capture the end time + total profiling duration for the footer.
PROFILE_END_EPOCH=$("${DATE_BIN}" +%s 2>/dev/null || echo 0)
PROFILE_END_HUMAN=$("${DATE_BIN}" '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || true)
PROFILE_DURATION_S=$(( PROFILE_END_EPOCH - PROFILE_START_EPOCH ))
PROFILE_DURATION=$(printf '%dh %dm %ds' $(( PROFILE_DURATION_S / 3600 )) $(( (PROFILE_DURATION_S % 3600) / 60 )) $(( PROFILE_DURATION_S % 60 )))
if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
    PROFILE_DURATION="${PROFILE_DURATION} (timed out)"
fi

TMPDIR_PROFILE=$(mktemp -d)

# "Other": any execve line not matching a tracked category. These are
# typically PATH-resolution noise (failed bash lookups from lmod, `id`/`uname`
# from shell init) and low-frequency tooling. We report the count and also log
# the distinct binaries (for ad-hoc triage).
# Build a newline-separated list of all tracked argv[0] names; an execve line
# is "tracked" if its argv[0] (the 2nd quoted field) equals one of them.
TRACKED_NAMES_FILE="${TMPDIR_PROFILE}/tracked_names.txt"
{
    printf '%s\n' "hydrogen"
    printf '%s\n' "${COUNT_LABELS[@]}"
    printf '%s\n' "${HYDROGEN_VARIANTS[@]}"
} | sort -u > "${TRACKED_NAMES_FILE}"

# Extract the executable path (1st quoted field, $4) from each execve result
# line, then keep only those whose basename is NOT a tracked binary (= "Other").
# Fixed-string -F lookup avoids regex pitfalls with names like "jsonschema-cli".
OTHER_COUNT=$("${GREP_BIN}" -E '^[0-9]+ execve\(' "${TRACE_OUT}" 2>/dev/null \
    | awk -F'"' '{print $4}' \
    | awk -F'/' '{print $NF}' \
    | "${GREP_BIN}" -vxFf "${TRACKED_NAMES_FILE}" 2>/dev/null \
    | wc -l 2>/dev/null || true)
OTHER_COUNT=${OTHER_COUNT:-0}

# Distinct uncategorized binaries (basenames, for the diagnostics log).
OTHER_BINS=$("${GREP_BIN}" -E '^[0-9]+ execve\(' "${TRACE_OUT}" 2>/dev/null \
    | awk -F'"' '{print $4}' \
    | awk -F'/' '{print $NF}' \
    | "${GREP_BIN}" -vxFf "${TRACKED_NAMES_FILE}" 2>/dev/null \
    | sort | uniq -c | sort -nr || true)
if [[ -n "${OTHER_BINS}" ]]; then
    {
        echo "Uncategorized execve counts (for triage):"
        echo "${OTHER_BINS}"
        echo ""
    } >> "${ERROR_LOG}"
fi

# Reconcile: the sum of all rows (tracked + hydrogen variants + Other) should
# equal TOTAL_EXEC. Any residual is execve lines that aren't result lines
# (e.g. strace's "<unfinished ...>" probes), reported in the footer for audit.
CAT_SUM=0
for i in "${!counts[@]}"; do
    CAT_SUM=$(( CAT_SUM + counts[i] ))
done
for c in "${HYDRO_VARIANT_COUNTS[@]}"; do
    CAT_SUM=$(( CAT_SUM + c ))
done
CAT_SUM=$(( CAT_SUM + OTHER_COUNT ))
RESIDUAL=$(( TOTAL_EXEC - CAT_SUM ))
[[ ${RESIDUAL} -lt 0 ]] && RESIDUAL=0

LAYOUT="${TMPDIR_PROFILE}/layout.json"
DATA="${TMPDIR_PROFILE}/data.json"

# Build data JSON (safe: categories/labels are fixed identifiers).
# Rows: each tracked command, followed by hydrogen variants (under Hydrogen
# so they group together via the 'break' column), then a final 'Other' row.
printf '[' > "${DATA}"
row_idx=0
# Format an integer with thousands separators (e.g. 15504 -> "15,504").
fmt_count() {
    local n="$1"
    printf '%s' "${n}" | sed ':a;s/\B[0-9]\{3\}\>/,&/;ta' || printf '%s' "${n}"
}
emit_row() {
    [[ ${row_idx} -gt 0 ]] && printf ',' >> "${DATA}"
    printf '\n  {"category":"%s","command":"%s","count":"%s"}' "$1" "$2" "$3" >> "${DATA}"
    row_idx=$(( row_idx + 1 ))
}
emit_sep() {
    [[ ${row_idx} -gt 0 ]] && printf ',' >> "${DATA}"
    printf '\n  {"category":"","command":"","count":""}' >> "${DATA}"
    row_idx=$(( row_idx + 1 ))
}
prev_cat=""
for i in "${!COUNT_LABELS[@]}"; do
    cat="${COUNT_CATS[${i}]}"
    if [[ -n "${prev_cat}" && "${cat}" != "${prev_cat}" ]]; then
        emit_sep
    fi
    emit_row "${cat}" "${COUNT_LABELS[${i}]}" "$(fmt_count "${counts[${i}]}" )"
    # Hydrogen variants: emit immediately after the hydrogen entry so they
    # render within the same Hydrogen group.
    if [[ "${COUNT_LABELS[${i}]}" == "hydrogen" ]]; then
        for j in "${!HYDROGEN_VARIANTS[@]}"; do
            emit_row "Hydrogen" "${HYDROGEN_VARIANTS[${j}]}" "$(fmt_count "${HYDRO_VARIANT_COUNTS[${j}]}" )"
        done
    fi
    prev_cat="${cat}"
done
# Other bucket as its own group.
if [[ ${RESIDUAL} -gt 0 ]]; then
    emit_sep
    emit_row "Other" "(uncategorized + ${RESIDUAL} probes)" "$(fmt_count "${OTHER_COUNT}")"
else
    emit_sep
    emit_row "Other" "(uncategorized)" "$(fmt_count "${OTHER_COUNT}")"
fi
printf '\n]\n' >> "${DATA}"

cat > "${LAYOUT}" <<EOF
{
  "theme": "Blue",
   "title": "Fork/Exec Profile: ${RUN_SCRIPT}  [${PROFILE_START_HUMAN}]",
    "footer": "Total: ${TOTAL_EXEC}  |  ${PROFILE_DURATION}",
   "footer_position": "right",
   "columns": [
     {"header": "Category", "key": "category", "datatype": "text", "width": 12, "justification": "left"},
     {"header": "Command", "key": "command", "datatype": "text", "width": 24, "justification": "left"},
     {"header": "Count", "key": "count", "datatype": "text", "width": 10, "justification": "right"}
   ]
}
EOF

echo "" >&2
if [[ -n "${TABLES_BIN}" ]]; then
    # Archive a clean mono copy; print a (optionally coloured) copy to console.
    "${TABLES_BIN}" "${LAYOUT}" "${DATA}" --mono > "${SUMMARY_OUT}" 2>/dev/null
    if [[ "${MONO}" -eq 1 || ! -t 1 ]]; then
        "${TABLES_BIN}" "${LAYOUT}" "${DATA}" --mono
    else
        "${TABLES_BIN}" "${LAYOUT}" "${DATA}"
    fi
else
    # Plain-text fallback when the tables executable is unavailable.
    {
        echo "Profiling Summary for ${RUN_SCRIPT}"
        echo "  started: ${PROFILE_START_HUMAN}"
        echo "  ended:   ${PROFILE_END_HUMAN} (${PROFILE_DURATION})"
        echo "-----------------------------------"
        echo "  Total exec: $(fmt_count "${TOTAL_EXEC}")"
        [[ ${RESIDUAL} -gt 0 ]] && echo "  (${RESIDUAL} non-result execve lines excluded from sum)"
        if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
            echo "  NOTE: timed out — partial trace"
        fi
        if [[ "${OTHER_COUNT}" -gt 0 ]]; then
            echo "  Uncategorized (Other): $(fmt_count "${OTHER_COUNT}") — see ${ERROR_LOG}"
        fi
        echo ""
        prev_cat=""
        for i in "${!COUNT_LABELS[@]}"; do
            cat="${COUNT_CATS[${i}]}"
            if [[ -n "${prev_cat}" && "${cat}" != "${prev_cat}" ]]; then
                echo "  ----"
            fi
            printf '  [%s] %s: %s\n' "${cat}" "${COUNT_LABELS[${i}]}" "$(fmt_count "${counts[${i}]}" )"
            # Hydrogen variants render in the same group, right after hydrogen.
            if [[ "${COUNT_LABELS[${i}]}" == "hydrogen" ]]; then
                for j in "${!HYDROGEN_VARIANTS[@]}"; do
                    printf '  [%s] %s: %s\n' "Hydrogen" "${HYDROGEN_VARIANTS[${j}]}" "$(fmt_count "${HYDRO_VARIANT_COUNTS[${j}]}" )"
                done
            fi
            prev_cat="${cat}"
        done
        echo "  ----"
        if [[ ${RESIDUAL} -gt 0 ]]; then
            printf '  [Other] (uncategorized + %s probes): %s\n' "${RESIDUAL}" "$(fmt_count "${OTHER_COUNT}")"
        else
            printf '  [Other] (uncategorized): %s\n' "$(fmt_count "${OTHER_COUNT}")"
        fi
        echo "==================================="
        printf '  [SUM] %s\n' "$(fmt_count "${TOTAL_EXEC}")"
        echo "-----------------------------------"
        echo "- Check ${ERROR_LOG} for strace errors and stall diagnostics."
    } | tee "${SUMMARY_OUT}"
fi
echo "Trace output saved in ${TRACE_OUT}, errors in ${ERROR_LOG}. Trace preserved for debugging." >> "${SUMMARY_OUT}"
