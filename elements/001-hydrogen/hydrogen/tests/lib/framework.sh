#!/usr/bin/env bash

# Framework Library
# Provides test lifecycle management and result tracking functions

# LIBRARY FUNCTIONS
# format_time_duration()
# format_file_size()
# format_number()
# get_elapsed_time()
# get_elapsed_time_decimal()
# set_test_number()
# reset_subtest_counter()
# start_test_timer()
# get_test_prefix()
# record_test_result()
# setup_orchestration_environment()
# setup_test_environment()
# evaluate_test_result_silent()

# CHANGELOG
# 3.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 2.8.1 - 2025-08-18 - Upgraded elapsed_time() to use $EPOCHREALTIME for more performance
# 2.8.0 - 2025-08-08 - Cleaned out some mktemp calls
# 2.7.0 - 2025-08-07 - Support for commas in test names (ie, thousands separators)
# 2.6.0 - 2025-08-06 - Improvements to logging file handling, common TAB file naming
# 2.5.1 - 2025-08-03 - Removed extraneous command -v calls
# 2.5.0 - 2025-08-02 - Removed old functions, added some from log_output
# 2.4.0 - 2025-08-02 - Optimizations for formatting functions
# 2,3,1 - 2025-07-31 - Added LINT_EXCLUDES to setup_test_environment
# 2.2.0 - 2025-07-20 - Added guard
# 2.1.0 - 2025-07-07 - Restructured how test elapsed times are stored and accessed
# 2.0.0 - 2025-07-02 - Updated to integrate with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi
set -euo pipefail

# Let's get this party started... Maybe

# Guard clause to prevent multiple sourcing
[[ -n "${FRAMEWORK_GUARD:-}" ]] && return 0
export FRAMEWORK_GUARD="true"

# Showstopper: Bash is too old
if (( BASH_VERSINFO[0] < 5 )); then
    echo "SHOWSTOPPER: Bash 5.0 or higher required"
    exit 1
fi

# Showstopper: Not enough available disk space
# NOTE: Unity Test Framework builds are huge
available_space=$(df -m . | awk 'NR==2 {print $4}')
if (( available_space < 750 )); then
    echo "SHOWSTOPPER: At least 750 MB of free disk space required"
    exit 1
fi

# Showstopper: Need access to tables command
if ! command -v tables >/dev/null 2>&1; then
    echo "SHOWSTOPPER: 'tables' command not found"
    exit 1
fi

# Showstopper: Need access to Oh command
# NOTE: This isn't a showstopper, really
if ! command -v Oh >/dev/null 2>&1; then
    echo "SHOWSTOPPER: 'Oh' command not found"
    exit 1
fi

# Library metadata
FRAMEWORK_NAME="Framework Library"
FRAMEWORK_VERSION="3.0.0"
export FRAMEWORK_NAME FRAMEWORK_VERSION

# Use this once
TIMESTAMP_DISPLAY=$(date '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null) # 2025-Jul-30 (Wed) 12:49:03 PDT  eg: long display times

# Function to find a command, preferring GNU version
find_command() {
    local gnu_cmd=$1 std_cmd=$2 var_name=$3
    if command -v "${gnu_cmd}" >/dev/null 2>&1; then
        eval "${var_name}=$(command -v "${gnu_cmd}" || true)"
    elif command -v "${std_cmd}" >/dev/null 2>&1; then
        eval "${var_name}=$(command -v "${std_cmd}" || true)"
    else
        echo "Error: Neither ${gnu_cmd} nor ${std_cmd} found" >&2
        exit 1
    fi
}

# Common utilities - use GNU versions if available (eg: homebrew on macOS)
find_command printf    printf   PRINTF
find_command gdate     date     DATE
find_command gfind     find     FIND
find_command ggrep     grep     GREP
find_command gsed      sed      SED
find_command gawk      awk      AWK
find_command gtar      tar      TAR
find_command gstat     stat     STAT
find_command grealpath realpath REALPATH
find_command gbasename basename BASENAME
find_command gdirname  dirname  DIRNAME
find_command gxargs    xargs    XARGS
find_command gtimeout  timeout  TIMEOUT
find_command nproc     nproc    NPROC
export PRINTF DATE FIND GREP SED AWK TAR STAT REALPATH BASENAME DIRNAME XARGS TIMEOUT NPROC

# These are standard utilities not tied to a particular OS
CLOC=$(command -v cloc)
GIT=$(command -v git)
MD5SUM=$(command -v md5sum)
export CLOC GIT MD5SUM

# Our own utilities built and installed presumably in /usr/local/bin
TABLES=$(command -v tables)
OH=$(command -v Oh) 
export TABLES OH

# Set the number of CPU cores for parallel processing - why not oversubscribe?
CORES_ACTUAL=$("${NPROC}")
CORES=$((CORES_ACTUAL * 2))
export CORES

# Function to format seconds as HH:MM:SS.ZZZ
format_time_duration() {
    local seconds="$1"
    local hours minutes secs milliseconds
    
    # Handle seconds that start with a decimal point (e.g., ".492")
    if [[ "${seconds}" =~ ^\. ]]; then
        seconds="0${seconds}"
    fi
    
    # Handle decimal seconds using parameter expansion
    if [[ "${seconds}" == *.* ]]; then
        secs="${seconds%.*}"
        milliseconds="${seconds#*.}"
        # Pad or truncate milliseconds to 3 digits
        milliseconds="${milliseconds}000"
        milliseconds="${milliseconds:0:3}"
    else
        secs="${seconds}"
        milliseconds="000"
    fi
    
    hours=$((secs / 3600))
    minutes=$(((secs % 3600) / 60))
    secs=$((secs % 60))
    
    "${PRINTF}" "%02d:%02d:%02d.%s" "${hours}" "${minutes}" "${secs}" "${milliseconds}"
}

# Function to format file size with thousands separators
format_file_size() {
    local file_size="$1"
    "${PRINTF}" "%'d" "${file_size}" 2>/dev/null || echo "${file_size}"
}

# Function to format file size with thousands separators
format_number() {
    local any_number="$1"
    env LC_ALL=en_US.UTF_8 "${PRINTF}" "%'d" "${any_number}" 2>/dev/null || echo "${any_number}"
}


# Function to calculate elapsed time in SSS.ZZZ format for console output
get_elapsed_time() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time="${EPOCHREALTIME}"
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        end_ms=${end_ms:0:3}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        start_ms=${start_ms:0:3}
        local diff_secs=$((end_secs - start_secs))
        local diff_ms=$((10#${end_ms} - 10#${start_ms}))
        if ((diff_ms < 0)); then
            diff_ms=$((diff_ms + 1000))
            diff_secs=$((diff_secs - 1))
        fi
        "${PRINTF}" "%03d.%03d" "${diff_secs}" "${diff_ms}"
    else
        echo "000.000"
    fi
}

# Function to calculate elapsed time in decimal format (X.XXX) for table output
get_elapsed_time_decimal() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time
        end_time=$("${DATE}" +%s.%3N) 
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        end_ms=$((10#${end_ms}))
        start_ms=$((10#${start_ms}))
        local end_total_ms=$((end_secs * 1000 + end_ms))
        local start_total_ms=$((start_secs * 1000 + start_ms))
        local elapsed_ms=$((end_total_ms - start_total_ms))
        local seconds=$((elapsed_ms / 1000))
        local milliseconds=$((elapsed_ms % 1000))
        "${PRINTF}" "%d.%03d" "${seconds}" "${milliseconds}"
    else
        echo "0.000"
    fi
}

# Function to set the current test number (e.g., "10" for test_10_compilation.sh)
set_test_number() {
    CURRENT_TEST_NUMBER="$1"
    CURRENT_SUBTEST_NUMBER=""
}

# Function to reset subtest counter
reset_subtest_counter() {
    SUBTEST_COUNTER=0
    CURRENT_SUBTEST_NUMBER="000"
}

# Function to start test timing
start_test_timer() {
    TEST_START_TIME=$("${DATE}" +%s.%3N)
    TEST_PASSED_COUNT=0
    TEST_FAILED_COUNT=0
}

# Function to record test result for statistics
record_test_result() {
    local status=$1
    if [[ "${status}" -eq 0 ]]; then
        TEST_PASSED_COUNT=$(( TEST_PASSED_COUNT +1 ))
    else
        TEST_FAILED_COUNT=$(( TEST_FAILED_COUNT +1 ))
    fi
}

setup_orchestration_environment() {

    # Get start time
    START_TIME=$("${DATE}" +%s.%N 2>/dev/null)

    # Naturally we're orchestrating
    ORCHESTRATION=true

    # Starting point
    TIMESTAMP=$("${DATE}" +%Y%m%d_%H%M%S)

    # All tests that run hydrogen run with a config that starts with hydrogen_test so we can
    # ensure nothing else is running by killing those processes at the start and at the end
    pkill -9 -f hydrogen_test_ || true

    # Array for collecting output messages (for performance optimization and progressive feedback)
    # Output is cached and dumped each time a new TEST starts, providing progressive feedback
    OUTPUT_COLLECTION=""
    COLLECT_OUTPUT_MODE=true
    
    # Global folder variables
    PROJECT_DIR="${HYDROGEN_ROOT}"
    pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return
    
    CMAKE_DIR="${PROJECT_DIR}/cmake"
    SCRIPT_DIR="${PROJECT_DIR}/tests"
    LIB_DIR="${SCRIPT_DIR}/lib"
    BUILD_DIR="${PROJECT_DIR}/build"
    TESTS_DIR="${BUILD_DIR}/tests"
    RESULTS_DIR="${TESTS_DIR}/results"
    DIAGS_DIR="${TESTS_DIR}/diagnostics"
    LOGS_DIR="${TESTS_DIR}/logs"
    CONFIG_DIR="${SCRIPT_DIR}/configs"

    COVERAGE="${LIB_DIR}/coverage_table.sh"
    SITEMAP="${LIB_DIR}/github-sitemap.sh"

    # shellcheck disable=SC2153,SC2154 # TEST_NUMBER defined by caller
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${LOGS_DIR}" "${DIAGS_DIR}" "${DIAG_TEST_DIR}" 
    
    # Need to load log_output a little earlier than the others
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    [[ -n "${LOG_OUTPUT_GUARD:-}" ]] || source "${LIB_DIR}/log_output.sh"
    
    # Reset test counter to zero
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined in caller (Test 00)
    set_test_number "${TEST_NUMBER}"
    reset_subtest_counter

    # Print beautiful test header
    # shellcheck disable=SC2154,SC2153 # TEST_NAME, TEST_ABBR, TEST_NUMBER, TEST_VERSION defined externally in caller
    print_test_suite_header "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Loading Test Suite Libraries"
    # Print framework and log output versions as they are already sourced
    [[ -n "${ORCHESTRATION:-}" ]] || print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
    [[ -n "${ORCHESTRATION:-}" ]] || print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
    # shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
    [[ -n "${LIFECYCLE_GUARD:-}" ]] || source "${LIB_DIR}/lifecycle.sh"
    # shellcheck source=tests/lib/env_utils.sh # Resolve path statically
    [[ -n "${ENV_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/env_utils.sh"
    # shellcheck source=tests/lib/network_utils.sh # Resolve path statically
    [[ -n "${NETWORK_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/network_utils.sh"
    # shellcheck source=tests/lib/coverage.sh # Resolve path statically
    [[ -n "${COVERAGE_GUARD:-}" ]] || source "${LIB_DIR}/coverage.sh"
    # shellcheck source=tests/lib/cloc.sh # Resolve path statically
    [[ -n "${CLOC_GUARD:-}" ]] || source "${LIB_DIR}/cloc.sh"
    # shellcheck source=tests/lib/file_utils.sh # Resolve path statically
    [[ -n "${FILE_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/file_utils.sh"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Test Suite libraries initialized"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking Build Directory"
    if [[ -d "build" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Nice Build directory you have there"

    else
        # Create the build directory and mount as tmpfs
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Creating Build directory..."
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "mkdir build"
        if mkdir build 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully created Build directory"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to create Build directory"
            EXIT_CODE=1
        fi
    fi

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    TAB_LAYOUT_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_header.json"
    TAB_DATA_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_header.json"
    TAB_LAYOUT_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_footer.json"
    TAB_DATA_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_footer.json"
    LOG_PREFIX="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    dump_collected_output
    clear_collected_output
}

# Function to set up the standard test environment with numbering
setup_test_environment() {

    if [[ -z "${ORCHESTRATION:-}" ]]; then

        # All tests that run hydrogen run with a config that starts with hydrogen_test so we can
        # ensure nothing else is running by killing those processes at the start and at the end
        # We only do this for single tests if it isn't running under orchestration
        pkill -9 -f hydrogen_test_ || true

        # Starting point
        TIMESTAMP=$("${DATE}" +%Y%m%d_%H%M%S)

        # Global folder variables
        PROJECT_DIR="${HYDROGEN_ROOT}"
        pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return

        CMAKE_DIR="${PROJECT_DIR}/cmake"
        SCRIPT_DIR="${PROJECT_DIR}/tests"
        LIB_DIR="${SCRIPT_DIR}/lib"
        BUILD_DIR="${PROJECT_DIR}/build"
        TESTS_DIR="${BUILD_DIR}/tests"
        RESULTS_DIR="${TESTS_DIR}/results"
        DIAGS_DIR="${TESTS_DIR}/diagnostics"
        LOGS_DIR="${TESTS_DIR}/logs"
        CONFIG_DIR="${SCRIPT_DIR}/configs"

        COVERAGE="${LIB_DIR}/coverage_table.sh"
        SITEMAP="${LIB_DIR}/github-sitemap.sh"

        # Array for collecting output messages (for performance optimization and progressive feedback)
        # Output is cached and dumped each time a new TEST starts, providing progressive feedback
        declare -a OUTPUT_COLLECTION=()
        COLLECT_OUTPUT_MODE=true

    else
      pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return
    fi

    # Common test configuration
    EXIT_CODE=0
    PASS_COUNT=0

    # Setup build folders
    # shellcheck disable=SC2153,SC2154 # TEST_NUMBER defined by caller
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}" "${DIAG_TEST_DIR}"

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    # Extra handling
    TAB_LAYOUT_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_header.json"
    TAB_DATA_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_header.json"
    TAB_LAYOUT_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_footer.json"
    TAB_DATA_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_footer.json"
    LOG_PREFIX="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    # Need to load log_output a little earlier than the others
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    [[ -n "${LOG_OUTPUT_GUARD:-}" ]] || source "${LIB_DIR}/log_output.sh"
    
    # Reset test counter to zero
    set_test_number "${TEST_NUMBER}"
    reset_subtest_counter

    # Print beautiful test header
    # shellcheck disable=SC2154,SC2153 # TEST_NAME, TEST_ABBR, TEST_NUMBER, TEST_VERSION defined externally in caller
    print_test_header "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    
    if [[ -z "${ORCHESTRATION:-}" ]]; then
         # Print framework and log output versions as they are already sourced
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
        # shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
        [[ -n "${LIFECYCLE_GUARD:-}" ]] || source "${LIB_DIR}/lifecycle.sh"
        # shellcheck source=tests/lib/env_utils.sh # Resolve path statically
        [[ -n "${ENV_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/env_utils.sh"
        # shellcheck source=tests/lib/network_utils.sh # Resolve path statically
        [[ -n "${NETWORK_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/network_utils.sh"
        # shellcheck source=tests/lib/coverage.sh # Resolve path statically
        [[ -n "${COVERAGE_GUARD:-}" ]] || source "${LIB_DIR}/coverage.sh"
        # shellcheck source=tests/lib/cloc.sh # Resolve path statically
        [[ -n "${CLOC_GUARD:-}" ]] || source "${LIB_DIR}/cloc.sh"
        # shellcheck source=tests/lib/file_utils.sh # Resolve path statically
        [[ -n "${FILE_UTILS_GUARD:-}" ]] || source "${LIB_DIR}/file_utils.sh"

    fi

    dump_collected_output
    clear_collected_output
}

# Function to evaluate test results silently (no output, just update counts)
# Intentionally not calling print_result to avoid duplicate PASS messages
evaluate_test_result_silent() {
    local test_name="$1"
    local failed_checks="$2"
    local pass_count_var="$3"
    local exit_code_var="$4"
    
    if [[ "${failed_checks}" -eq 0 ]]; then
        eval "${pass_count_var}=\$((\${${pass_count_var}} + 1))"
    else
        eval "${exit_code_var}=1"
    fi
}

