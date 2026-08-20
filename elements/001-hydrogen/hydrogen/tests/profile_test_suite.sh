#!/usr/bin/env bash
#
# profile_test_suite.sh
# Profile execve activity for a Hydrogen test script under strace.
#
#   ./profile_test_suite.sh                       profile the full suite
#   ./profile_test_suite.sh test_40_auth.sh       profile one test
#   ./profile_test_suite.sh --timeout 600 ...     cap the strace run (seconds)
#   ./profile_test_suite.sh --skip-email ...      skip the make-email/mutt tail
#   ./profile_test_suite.sh --mono ...            disable ANSI colours
#   ./profile_test_suite.sh --help
#
# Output files (dated; re-runs on the same day overwrite that day's files):
#   profile_trace-<DATE>.txt    raw strace output
#   profile_summary-<DATE>.txt  rendered summary (tables; mono archive)
#   profile_error-<DATE>.txt    diagnostics + strace stderr
#
# CHANGELOG
# 1.2.2 - 2026-08-20 - Title is basename @ HH:MM:SS.
# 1.2.1 - 2026-08-20 - Tables layout nitpicks:
#   * Blank zeros (tables default); omit column widths so Command/Count/title/footer auto-size.
#   * Command summary is count of non-zero rows (zero rows are annotated); Count stays sum.
#   * Leftover bucket labelled Uncategorized.
# 1.2.0 - 2026-08-20 - Timeout, catalog, tables:
#   * Replace the setsid/watchdog (setsid without -w returns immediately and
#     the watchdog is then killed) with GNU timeout -k, which process-group
#     kills strace and every tracee. A timeout is the only reliable stop.
#   * strace -z counts successful execve only, dropping PATH-walk ENOENT
#     probes that inflated bash/sh/Other.
#   * One awk pass catalogs every basename (aliases gawk->awk, etc.).
#   * tables: Count is num + sum. JSON is built with jq.
# 1.1.0 - 2026-08-20 - Path-basename matching; tables num/break.
# 1.0.0 - 2026-08-19 - Rewrite: dated outputs, timeout, tables, flags.

set -euo pipefail

SCRIPT_VERSION="1.2.2"

# --- Toolchain ----------------------------------------------------------
DATE_BIN=$(command -v gdate 2>/dev/null || command -v date || true)
AWK_BIN=$(command -v gawk 2>/dev/null || command -v awk || true)
GREP_BIN=$(command -v ggrep 2>/dev/null || command -v grep || true)
JQ_BIN=$(command -v jq || true)
TABLES_BIN=$(command -v tables || true)
STRACE_BIN=$(PATH=/usr/bin command -v strace || true)
TIMEOUT_BIN=$(command -v gtimeout 2>/dev/null || command -v timeout || true)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PROFILE_START_EPOCH=$("${DATE_BIN}" +%s 2>/dev/null || echo 0)
PROFILE_START_HUMAN=$("${DATE_BIN}" '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || true)
PROFILE_START_CLOCK=$("${DATE_BIN}" '+%H:%M:%S' 2>/dev/null || true)

# --- Defaults -----------------------------------------------------------
DEFAULT_SCRIPT="test_00_all.sh"
PROFILE_TIMEOUT="${PROFILE_TIMEOUT:-3600}"
MONO=0
SKIP_EMAIL=0
TEST_SCRIPT=""
EXTRA_ARGS=()
TMPDIR_PROFILE=""

cleanup() {
    [[ -n "${TMPDIR_PROFILE}" ]] && rm -rf "${TMPDIR_PROFILE}" || true
}
trap cleanup EXIT

usage() {
    cat <<EOF
Usage: $(basename "$0") [--help] [--timeout SECONDS] [--skip-email] [--mono] [test_script]

Profile successful execve activity for a Hydrogen test script under strace.

  (no argument)        Profile the full suite (test_00_all.sh)
  test_script          Profile an individual test, e.g. test_40_auth.sh
  --timeout N          Hard cap (seconds) on the strace run (default: ${PROFILE_TIMEOUT}).
                       GNU timeout process-group kills strace and every tracee.
                       0 disables the cap (unbounded; not recommended).
  --skip-email         Set HYDROGEN_DISABLE_EMAIL=1 so the suite's make-email
                       (mutt) tail is skipped
  --mono               Disable ANSI colours in the rendered summary
  --help, -h           Show this help

Environment:
  PROFILE_TIMEOUT      Overrides the default --timeout value

Version: ${SCRIPT_VERSION}
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

if [[ ! "${PROFILE_TIMEOUT}" =~ ^[0-9]+$ ]]; then
    echo "Error: --timeout must be a non-negative integer (seconds)" >&2
    exit 2
fi

echo "" >&2
echo "Profiling Summary for ${TEST_SCRIPT} on $("${DATE_BIN}" || true)" >&2

# --- Pre-flight ---------------------------------------------------------
if [[ -z "${STRACE_BIN}" ]]; then
    echo "Error: strace not found in /usr/bin" >&2
    exit 1
fi
if [[ -z "${TIMEOUT_BIN}" ]]; then
    echo "Error: timeout not found (needed to bound strace -f)" >&2
    exit 1
fi
if [[ -z "${GREP_BIN}" ]]; then
    echo "Error: grep not found" >&2
    exit 1
fi
if [[ -z "${AWK_BIN}" ]]; then
    echo "Error: awk not found" >&2
    exit 1
fi
if [[ -z "${JQ_BIN}" ]]; then
    echo "Error: jq not found" >&2
    exit 1
fi

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

case "${RUN_SCRIPT}" in
    */*) : ;;
    *)   RUN_SCRIPT="./${RUN_SCRIPT}" ;;
esac

ulimit -n 4096 2>/dev/null || true

# --- Output files -------------------------------------------------------
RUN_DATE_TAG=$("${DATE_BIN}" +%Y%m%d)
TRACE_OUT="profile_trace-${RUN_DATE_TAG}.txt"
SUMMARY_OUT="profile_summary-${RUN_DATE_TAG}.txt"
ERROR_LOG="profile_error-${RUN_DATE_TAG}.txt"

rm -f profile_trace.txt profile_summary.txt profile_error.txt

{
    echo "Profiling run started at $("${DATE_BIN}" || true)"
    echo "profile_test_suite.sh ${SCRIPT_VERSION}"
    echo "Test script: ${RUN_SCRIPT}"
    echo "Timeout: ${PROFILE_TIMEOUT}s  skip-email: ${SKIP_EMAIL}"
    echo "System limits: $(ulimit -n 2>/dev/null || true), Memory: $(free -m 2>/dev/null | "${GREP_BIN}" Mem || true)"
    echo "strace: ${STRACE_BIN}, timeout: ${TIMEOUT_BIN}, tables: ${TABLES_BIN:-n/a}, jq: ${JQ_BIN}"
} > "${ERROR_LOG}"

if [[ "${SKIP_EMAIL}" -eq 1 ]]; then
    export HYDROGEN_DISABLE_EMAIL=1
fi

# --- Run strace (GNU timeout process-group kills the whole tree) --------
# strace -f follows every fork. Tracees that outlive the script (mutt on
# SMTP, orphaned hydrogen servers, hbm_browser) keep strace alive until
# timeout fires. GNU timeout puts the command in its own process group
# and signals the whole group; -k 10 sends KILL if TERM is ignored.
# -z records successful execve only (PATH-walk ENOENT is dropped).
echo "Running strace on ${RUN_SCRIPT}..." >> "${ERROR_LOG}"
STRACE_STATUS=0
STRACE_TIMED_OUT=0

"${TIMEOUT_BIN}" -k 10 "${PROFILE_TIMEOUT}" \
    "${STRACE_BIN}" -f -z -s 256 -e trace=execve \
    -o "${TRACE_OUT}" -- "${RUN_SCRIPT}" 2>> "${ERROR_LOG}" \
    || STRACE_STATUS=$?

if [[ "${STRACE_STATUS}" -eq 124 || "${STRACE_STATUS}" -eq 137 ]]; then
    STRACE_TIMED_OUT=1
    echo "strace timed out after ${PROFILE_TIMEOUT}s (status ${STRACE_STATUS}); process group terminated." >> "${ERROR_LOG}"
elif [[ "${STRACE_STATUS}" -ne 0 ]]; then
    echo "strace exited with status ${STRACE_STATUS}." >> "${ERROR_LOG}"
fi

if [[ ! -f "${TRACE_OUT}" ]]; then
    echo "Error: trace file ${TRACE_OUT} was not created" >&2
    exit 1
fi

# --- Catalog ------------------------------------------------------------
# Single source of truth: basename <tab> category. Display order is file order.
# GNU aliases (gawk, ggrep, ...) remap onto these names so they do not
# land in Uncategorized and do not get a second row.
TMPDIR_PROFILE=$(mktemp -d)
CATALOG_FILE="${TMPDIR_PROFILE}/catalog.txt"
ALIAS_FILE="${TMPDIR_PROFILE}/alias.txt"
TALLY_FILE="${TMPDIR_PROFILE}/tally.txt"
OTHER_FILE="${TMPDIR_PROFILE}/other.txt"

cat > "${CATALOG_FILE}" <<'EOF'
hydrogen	Hydrogen
hydrogen_coverage	Hydrogen
hydrogen_debug	Hydrogen
hydrogen_naked	Hydrogen
hydrogen_perf	Hydrogen
hydrogen_release	Hydrogen
hydrogen_valgrind	Hydrogen
bash	Shell
sh	Shell
dash	Shell
zsh	Shell
xargs	Shell
*.sh	Shell
cat	SysUtils
find	SysUtils
bc	SysUtils
tr	SysUtils
wc	SysUtils
date	SysUtils
md5sum	SysUtils
printf	SysUtils
true	SysUtils
false	SysUtils
env	SysUtils
nproc	SysUtils
id	SysUtils
uname	SysUtils
stat	SysUtils
df	SysUtils
tee	SysUtils
cut	SysUtils
uniq	SysUtils
base64	SysUtils
openssl	SysUtils
seq	SysUtils
mkdir	PathTools
mktemp	PathTools
realpath	PathTools
basename	PathTools
dirname	PathTools
du	PathTools
rm	PathTools
mv	PathTools
cp	PathTools
ln	PathTools
chmod	PathTools
touch	PathTools
ls	PathTools
grep	TextTools
sed	TextTools
awk	TextTools
curl	TextTools
jq	TextTools
head	TextTools
tail	TextTools
sort	TextTools
git	TextTools
cmake	Build
make	Build
ninja	Build
ninja-build	Build
cc	Build
gcc	Build
gcov	Build
ccache	Build
cppcheck	Lint
shellcheck	Lint
markdownlint	Lint
jsonlint	Lint
eslint	Lint
stylelint	Lint
htmlhint	Lint
xmlstarlet	Lint
luacheck	Lint
jsonschema-cli	Lint
cloc	Reporting
tables	Reporting
Oh	Reporting
convert	Reporting
magick	Reporting
mutt	Reporting
lua	Reporting
flock	Process
sleep	Process
which	Process
timeout	Process
kill	Process
pkill	Process
pgrep	Process
ps	Process
sqlite3	DB
psql	DB
mysql	DB
mariadb	DB
python3	Misc
node	Misc
perl	Misc
addto	Misc
mailval	Misc
EOF

cat > "${ALIAS_FILE}" <<'EOF'
gawk	awk
ggrep	grep
gsed	sed
gdate	date
gfind	find
gtimeout	timeout
grealpath	realpath
gbasename	basename
gdirname	dirname
gxargs	xargs
gstat	stat
g++	gcc
EOF

# shellcheck disable=SC2016 # awk program is single-quoted on purpose
"${AWK_BIN}" -v catalog_file="${CATALOG_FILE}" -v alias_file="${ALIAS_FILE}" \
    -v tally_file="${TALLY_FILE}" -v other_file="${OTHER_FILE}" '
BEGIN {
    FS = "\t"
    while ((getline < alias_file) > 0) {
        if (NF >= 2) alias[$1] = $2
    }
    close(alias_file)
    while ((getline < catalog_file) > 0) {
        if (NF >= 2) {
            n++
            order[n] = $1
            category[$1] = $2
        }
    }
    close(catalog_file)
}
/^[0-9]+ execve\("/ {
    split($0, parts, "\"")
    path = parts[2]
    if (path == "") next
    nparts = split(path, segs, "/")
    base = segs[nparts]
    if (base == "") next
    if (base in alias) base = alias[base]
    if (base ~ /\.sh$/) base = "*.sh"
    total++
    if (base in category) {
        count[base]++
    } else {
        other++
        otherc[base]++
    }
}
END {
    print total + 0
    print other + 0
    for (i = 1; i <= n; i++) {
        print order[i] "\t" category[order[i]] "\t" (count[order[i]] + 0) > tally_file
    }
    nout = 0
    for (b in otherc) {
        nout++
        obase[nout] = b
        ocount[nout] = otherc[b]
    }
    for (i = 1; i <= nout; i++) {
        for (j = i + 1; j <= nout; j++) {
            if (ocount[j] > ocount[i] || (ocount[j] == ocount[i] && obase[j] < obase[i])) {
                tb = obase[i]; obase[i] = obase[j]; obase[j] = tb
                tc = ocount[i]; ocount[i] = ocount[j]; ocount[j] = tc
            }
        }
    }
    for (i = 1; i <= nout; i++) {
        print ocount[i] "\t" obase[i] > other_file
    }
}
' "${TRACE_OUT}" > "${TMPDIR_PROFILE}/meta.txt"

{
    read -r TOTAL_EXEC
    read -r OTHER_COUNT
} < "${TMPDIR_PROFILE}/meta.txt"
TOTAL_EXEC=${TOTAL_EXEC:-0}
OTHER_COUNT=${OTHER_COUNT:-0}

if [[ -s "${OTHER_FILE}" ]]; then
    {
        echo "Uncategorized execve counts (for triage):"
        cat "${OTHER_FILE}"
        echo ""
    } >> "${ERROR_LOG}"
fi

CAT_SUM=0
while IFS=$'\t' read -r _cmd _cat cnt; do
    [[ -z "${_cmd}" ]] && continue
    CAT_SUM=$(( CAT_SUM + cnt ))
done < "${TALLY_FILE}"
CAT_SUM=$(( CAT_SUM + OTHER_COUNT ))
RESIDUAL=$(( TOTAL_EXEC - CAT_SUM ))
[[ ${RESIDUAL} -lt 0 ]] && RESIDUAL=0

# --- Render -------------------------------------------------------------
PROFILE_END_EPOCH=$("${DATE_BIN}" +%s 2>/dev/null || echo 0)
PROFILE_END_HUMAN=$("${DATE_BIN}" '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || true)
PROFILE_DURATION_S=$(( PROFILE_END_EPOCH - PROFILE_START_EPOCH ))
if [[ "${PROFILE_DURATION_S}" -lt 0 ]]; then
    PROFILE_DURATION_S=0
fi
if [[ "${PROFILE_DURATION_S}" -ge 3600 ]]; then
    PROFILE_DURATION=$(printf '%dh %dm %ds' $(( PROFILE_DURATION_S / 3600 )) $(( (PROFILE_DURATION_S % 3600) / 60 )) $(( PROFILE_DURATION_S % 60 )))
elif [[ "${PROFILE_DURATION_S}" -ge 60 ]]; then
    PROFILE_DURATION=$(printf '%dm %ds' $(( PROFILE_DURATION_S / 60 )) $(( PROFILE_DURATION_S % 60 )))
else
    PROFILE_DURATION=$(printf '%ds' "${PROFILE_DURATION_S}")
fi
if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
    PROFILE_DURATION="${PROFILE_DURATION} (timed out)"
fi

LAYOUT="${TMPDIR_PROFILE}/layout.json"
DATA="${TMPDIR_PROFILE}/data.json"

fmt_count() {
    local n="$1"
    printf '%s' "${n}" | sed ':a;s/\B[0-9]\{3\}\>/,&/;ta' || printf '%s' "${n}"
}

OTHER_LABEL="Uncategorized"
if [[ ${RESIDUAL} -gt 0 ]]; then
    OTHER_LABEL="Uncategorized + ${RESIDUAL} probes"
fi

# shellcheck disable=SC2016 # jq program is single-quoted on purpose
{
    cat "${TALLY_FILE}"
    printf '%s\t%s\t%s\n' "${OTHER_LABEL}" "Uncategorized" "${OTHER_COUNT}"
} | "${JQ_BIN}" -R -s -c '
    split("\n")
    | map(select(length > 0))
    | map(split("\t"))
    | map(
        (.[2] | tonumber) as $count
        | {
            category: .[1],
            command: .[0],
            count: $count,
            annotate: ($count == 0)
        }
    )
' > "${DATA}"

FOOTER_TEXT="${PROFILE_DURATION}"
if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
    FOOTER_TEXT="${FOOTER_TEXT} — partial trace"
fi

# shellcheck disable=SC2016 # jq program is single-quoted on purpose
"${JQ_BIN}" -n \
    --arg title "$(basename "${RUN_SCRIPT}") @ ${PROFILE_START_CLOCK}" \
    --arg footer "${FOOTER_TEXT}" \
    '{
      theme: "Blue",
      title: $title,
      footer: $footer,
      footer_position: "right",
      columns: [
        {
          header: "Category",
          key: "category",
          datatype: "text",
          justification: "left",
          visible: false,
          break: true
        },
        {
          header: "Command",
          key: "command",
          datatype: "text",
          justification: "left",
          summary: "count"
        },
        {
          header: "Count",
          key: "count",
          datatype: "num",
          justification: "right",
          summary: "sum"
        }
      ]
    }' > "${LAYOUT}"

echo "" >&2
if [[ -n "${TABLES_BIN}" ]]; then
    "${TABLES_BIN}" "${LAYOUT}" "${DATA}" --mono > "${SUMMARY_OUT}" 2>/dev/null || true
    if [[ "${MONO}" -eq 1 || ! -t 1 ]]; then
        "${TABLES_BIN}" "${LAYOUT}" "${DATA}" --mono || true
    else
        "${TABLES_BIN}" "${LAYOUT}" "${DATA}" || true
    fi
else
    FMT_TOTAL="$(fmt_count "${TOTAL_EXEC}")"
    FMT_TOTAL="${FMT_TOTAL:-${TOTAL_EXEC}}"
    {
        echo "Profiling Summary for ${RUN_SCRIPT}"
        echo "  started: ${PROFILE_START_HUMAN}"
        echo "  ended:   ${PROFILE_END_HUMAN} (${PROFILE_DURATION})"
        echo "-----------------------------------"
        printf '  Total exec: %s\n' "${FMT_TOTAL}"
        [[ ${RESIDUAL} -gt 0 ]] && echo "  (${RESIDUAL} non-result execve lines excluded from sum)"
        if [[ "${STRACE_TIMED_OUT}" -ne 0 ]]; then
            echo "  NOTE: timed out — partial trace"
        fi
        if [[ "${OTHER_COUNT}" -gt 0 ]]; then
            FMT_OTHER="$(fmt_count "${OTHER_COUNT}")"
            FMT_OTHER="${FMT_OTHER:-${OTHER_COUNT}}"
            echo "  Uncategorized: ${FMT_OTHER} — see ${ERROR_LOG}"
        fi
        echo ""
        prev_cat=""
        while IFS=$'\t' read -r cmd cat cnt; do
            [[ -z "${cmd}" ]] && continue
            if [[ -n "${prev_cat}" && "${cat}" != "${prev_cat}" ]]; then
                echo "  ----"
            fi
            FMT_C="$(fmt_count "${cnt}")"
            FMT_C="${FMT_C:-${cnt}}"
            printf '  [%s] %s: %s\n' "${cat}" "${cmd}" "${FMT_C}"
            prev_cat="${cat}"
        done < "${TALLY_FILE}"
        echo "  ----"
        FMT_OTHER2="$(fmt_count "${OTHER_COUNT}")"
        FMT_OTHER2="${FMT_OTHER2:-${OTHER_COUNT}}"
        printf '  [Uncategorized] %s: %s\n' "${OTHER_LABEL}" "${FMT_OTHER2}"
        echo "==================================="
        printf '  [SUM] %s\n' "${FMT_TOTAL}"
        echo "-----------------------------------"
        echo "- Check ${ERROR_LOG} for strace errors and stall diagnostics."
    } | tee "${SUMMARY_OUT}"
fi
echo "Trace output saved in ${TRACE_OUT}, errors in ${ERROR_LOG}. Trace preserved for debugging." >> "${SUMMARY_OUT}"
