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
BASH_COUNT=$(grep -c 'execve.*[ /]bash[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
SH_COUNT=$(grep -c 'execve.*[ /]sh[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
XARGS_COUNT=$(grep -c 'execve.*[ /]xargs[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
GREP_COUNT=$(grep -c 'execve.*[ /]grep[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
GREP_SHELLCHECK_COUNT=$(grep -c 'execve.*[ /]grep.*shellcheck' "${TRACE_OUT}" 2>/dev/null)
SED_COUNT=$(grep -c 'execve.*[ /]sed[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
AWK_COUNT=$(grep -c 'execve.*[ /]awk[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
BC_COUNT=$(grep -c 'execve.*[ /]bc[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
WC_COUNT=$(grep -c 'execve.*[ /]wc[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
DU_COUNT=$(grep -c 'execve.*[ /]du[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
SHELLCHECK_COUNT=$(grep -c 'execve.*[ /]shellcheck[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
MAKE_COUNT=$(grep -c 'execve.*[ /]make[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
CURL_COUNT=$(grep -c 'execve.*[ /]curl[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)
TABLES_COUNT=$(grep -c 'execve.*[ /]tables[ ""]' "${TRACE_OUT}" 2>/dev/null || echo 0)


# Generate summary
{
    echo "Profiling Summary for test_00_all.sh ($(date || true))"
    echo "-----------------------------------"
    echo "Total forks (fork/vfork/clone): ${FORKS}"
    echo "Command invocations:"
    echo "  bash: ${BASH_COUNT}"
    echo "  sh: ${SH_COUNT}"
    echo "  xargs: ${XARGS_COUNT}"
    echo "  grep: ${GREP_COUNT}"
    echo "  grep (with 'shellcheck'): ${GREP_SHELLCHECK_COUNT}"
    echo "  sed: ${SED_COUNT}"
    echo "  awk: ${AWK_COUNT}"
    echo "  bc: ${BC_COUNT}"
    echo "  wc: ${WC_COUNT}"
    echo "  du: ${DU_COUNT}"
    echo "  shellcheck: ${SHELLCHECK_COUNT}"
    echo "  make: ${MAKE_COUNT}"
    echo "  curl: ${CURL_COUNT}"
    echo "  tables: ${TABLES_COUNT}"
    echo "-----------------------------------"
    echo "- Check ${ERROR_LOG} for strace errors."
} > "${SUMMARY_OUT}"

# Display summary
cat "${SUMMARY_OUT}"

# Keep trace_output.txt for debugging
echo "Trace output saved in ${TRACE_OUT}, errors in ${ERROR_LOG}" >> "${SUMMARY_OUT}"
