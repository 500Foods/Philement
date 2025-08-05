#!/bin/bash

# profile_test_suite.sh: Profile forks and command executions for test_00_all.sh
# Usage: ./profile_test_suite.sh

# Output files
TRACE_OUT="profile_trace.txt"
SUMMARY_OUT="profile_summary.txt"
ERROR_LOG="profile_error.txt"

# Ensure sufficient file descriptors
ulimit -n 4096 2>> "${ERROR_LOG}"

# Log start
echo "Starting profiling at $(date || true)" > "${ERROR_LOG}"
echo "System limits: $(ulimit -n || true), Memory: $(free -m || true | grep Mem || true)" >> "${ERROR_LOG}"

# Verify test_00_all.sh
if [[ ! -x "./test_00_all.sh" ]]; then
    echo "Error: test_00_all.sh not found or not executable" | tee -a "${ERROR_LOG}" "${SUMMARY_OUT}"
    exit 1
fi

rm -f ./*.txt

# Run strace
echo "Running strace on test_00_all.sh..." >> "${ERROR_LOG}"
strace -f -e trace=fork,execve -o "${TRACE_OUT}" ./test_00_all.sh 2>> "${ERROR_LOG}"
STRACE_STATUS=$?

# Check strace status
if [[ "${STRACE_STATUS}" -ne 0 ]]; then
    echo "strace failed with status ${STRACE_STATUS}. See ${ERROR_LOG}." | tee -a "${SUMMARY_OUT}"
fi

# Count forks
FORKS=$(grep -cE 'fork|vfork|clone' "${TRACE_OUT}" 2>/dev/null || true)

# Count command invocations
BASH_COUNT=$(grep -c 'execve.*[ /]bash[ ""]' "${TRACE_OUT}" 2>/dev/null)
SH_COUNT=$(grep -c 'execve.*[ /]sh[ ""]' "${TRACE_OUT}" 2>/dev/null)
XARGS_COUNT=$(grep -c 'execve.*[ /]xargs[ ""]' "${TRACE_OUT}" 2>/dev/null)

CAT_COUNT=$(grep -c 'execve.*[ /]cat[ ""]' "${TRACE_OUT}" 2>/dev/null)
BC_COUNT=$(grep -c 'execve.*[ /]bc[ ""]' "${TRACE_OUT}" 2>/dev/null)
TR_COUNT=$(grep -c 'execve.*[ /]tr[ ""]' "${TRACE_OUT}" 2>/dev/null)
WC_COUNT=$(grep -c 'execve.*[ /]wc[ ""]' "${TRACE_OUT}" 2>/dev/null)
DATE_COUNT=$(grep -c 'execve.*[ /]date[ ""]' "${TRACE_OUT}" 2>/dev/null)
MD5_COUNT=$(grep -c 'execve.*[ /]md5sum[ ""]' "${TRACE_OUT}" 2>/dev/null)

MKTEMP_COUNT=$(grep -c 'execve.*[ /]mktemp[ ""]' "${TRACE_OUT}" 2>/dev/null)
REALPATH_COUNT=$(grep -c 'execve.*[ /]realpath[ ""]' "${TRACE_OUT}" 2>/dev/null)
BASENAME_COUNT=$(grep -c 'execve.*[ /]basename[ ""]' "${TRACE_OUT}" 2>/dev/null)
DIRNAME_COUNT=$(grep -c 'execve.*[ /]dirname[ ""]' "${TRACE_OUT}" 2>/dev/null)
DU_COUNT=$(grep -c 'execve.*[ /]du[ ""]' "${TRACE_OUT}" 2>/dev/null)

GREP_COUNT=$(grep -c 'execve.*[ /]grep[ ""]' "${TRACE_OUT}" 2>/dev/null)
SED_COUNT=$(grep -c 'execve.*[ /]sed[ ""]' "${TRACE_OUT}" 2>/dev/null)
AWK_COUNT=$(grep -c 'execve.*[ /]awk[ ""]' "${TRACE_OUT}" 2>/dev/null)
CURL_COUNT=$(grep -c 'execve.*[ /]curl[ ""]' "${TRACE_OUT}" 2>/dev/null)

CMAKE_COUNT=$(grep -c 'execve.*[ /]cmake[ ""]' "${TRACE_OUT}" 2>/dev/null)
MAKE_COUNT=$(grep -c 'execve.*[ /]make[ ""]' "${TRACE_OUT}" 2>/dev/null)
CC_COUNT=$(grep -c 'execve.*[ /]cc[ ""]' "${TRACE_OUT}" 2>/dev/null)
GCOV_COUNT=$(grep -c 'execve.*[ /]gcov[ ""]' "${TRACE_OUT}" 2>/dev/null)

CPPCHECK_COUNT=$(grep -c 'execve.*[ /]tables[ ""]' "${TRACE_OUT}" 2>/dev/null)
SHELLCHECK_COUNT=$(grep -c 'execve.*[ /]shellcheck[ ""]' "${TRACE_OUT}" 2>/dev/null)
MARKDOWNLINT_COUNT=$(grep -c 'execve.*[ /]markdownlint[ ""]' "${TRACE_OUT}" 2>/dev/null)
JSONLINT_COUNT=$(grep -c 'execve.*[ /]jsonlint[ ""]' "${TRACE_OUT}" 2>/dev/null)
ESLINT_COUNT=$(grep -c 'execve.*[ /]eslint[ ""]' "${TRACE_OUT}" 2>/dev/null)
STYLELINT_COUNT=$(grep -c 'execve.*[ /]stylelint[ ""]' "${TRACE_OUT}" 2>/dev/null)
HTMLHINT_COUNT=$(grep -c 'execve.*[ /]htmlhint[ ""]' "${TRACE_OUT}" 2>/dev/null)

TABLES_COUNT=$(grep -c 'execve.*[ /]tables[ ""]' "${TRACE_OUT}" 2>/dev/null)


# Generate summary
{
    echo "Profiling Summary for test_00_all.sh ($(date || true))"
    echo "-----------------------------------"
    echo "Total forks (fork/vfork/clone): ${FORKS}"
    echo "Command invocations:"
    echo "  bash: ${BASH_COUNT}"
    echo "  sh: ${SH_COUNT}"
    echo "  xargs: ${XARGS_COUNT}"
    echo " "
    echo "  cat: ${CAT_COUNT}"
    echo "  tr: ${TR_COUNT}"
    echo "  bc: ${BC_COUNT}"
    echo "  wc: ${WC_COUNT}"
    echo "  date: ${DATE_COUNT}"
    echo "  md5: ${MD5_COUNT}"
    echo " "
    echo "  mktemp: ${MKTEMP_COUNT}"
    echo "  realpath: ${REALPATH_COUNT}"
    echo "  basename: ${BASENAME_COUNT}"
    echo "  dirname: ${DIRNAME_COUNT}"
    echo "  du: ${DU_COUNT}"
    echo " "
    echo "  grep: ${GREP_COUNT}"
    echo "  sed: ${SED_COUNT}"
    echo "  awk: ${AWK_COUNT}"
    echo "  curl: ${CURL_COUNT}"
    echo " "
    echo "  cmake: ${CMAKE_COUNT}"
    echo "  make: ${MAKE_COUNT}"
    echo "  cc: ${CC_COUNT}"
    echo "  gcov: ${GCOV_COUNT}"
    echo " "
    echo "  cppcheck: ${CPPCHECK_COUNT}"
    echo "  shellcheck: ${SHELLCHECK_COUNT}"
    echo "  markdownlint: ${MARKDOWNLINT_COUNT}"
    echo "  jsonlint: ${JSONLINT_COUNT}"
    echo "  eslint: ${ESLINT_COUNT}"
    echo "  stylelint: ${STYLELINT_COUNT}"
    echo "  htmlhint: ${HTMLHINT_COUNT}"
    echo " "
    echo "  tables: ${TABLES_COUNT}"
    echo "-----------------------------------"
    echo "- Check ${ERROR_LOG} for strace errors."
} > "${SUMMARY_OUT}"

# Display summary
cat "${SUMMARY_OUT}"

# Keep trace_output.txt for debugging
echo "Trace output saved in ${TRACE_OUT}, errors in ${ERROR_LOG}" >> "${SUMMARY_OUT}"
