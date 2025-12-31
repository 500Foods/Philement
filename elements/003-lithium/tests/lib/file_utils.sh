#!/usr/bin/env bash

# File Utilities Library
# Provides file and path manipulation functions for test scripts

# LIBRARY FUNCTIONS
# should_exclude_file()
# convert_to_relative_path()
# safe_cd()
# get_file_size()
# get_config_path()
# extract_web_server_port()
# validate_json()
# get_webserver_port()
# handle_timing_file()
# test_file_download()
# collect_timing_data()

# CHANGELOG
# 3.0.0 - 2025-12-31 - Updated for Lithium project
# 2.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 1.3.0 - 2025-09-19 - Added timing and file download functions from test_22_swagger.sh
#                    - Added handle_timing_file(), test_file_download(), collect_timing_data()
#                    - Support for test script refactoring and code reuse
# 1.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.2.0 - 2025-07-30 - Added common should_exclude_file() from other scripts
# 1.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Check for required LITHIUM_ROOT environment variable
if [[ -z "${LITHIUM_ROOT:-}" ]]; then
    echo "❌ Error: LITHIUM_ROOT environment variable is not set"
    echo "Please set LITHIUM_ROOT to the Lithium project's root directory"
    exit 1
fi                         

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${FILE_UTILS_GUARD:-}" ]] && return 0
export FILE_UTILS_GUARD="true"

# Library metadata
FILE_UTILS_NAME="File Utilities Library"
FILE_UTILS_VERSION="2.0.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${FILE_UTILS_NAME} ${FILE_UTILS_VERSION}" "info"

# Enable recursive globbing and include dotfiles
shopt -s dotglob globstar nullglob

# Preload .lintignore patterns into an array
declare -ag LINT_IGNORE_PATTERNS=()
if [[ -f ".lintignore" ]]; then
    while IFS= read -r pattern; do
        [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
        # shellcheck disable=SC2154 # SED defined externally in framework.sh
        pattern="$(echo "${pattern}" | tr -d '\r' | "${SED}" 's/[[:space:]]*$//' || true)"
        if [[ "${pattern}" == */'*' ]]; then
            pattern="${pattern%/*}/**"
        fi
        LINT_IGNORE_PATTERNS+=("${pattern}")
    done < ".lintignore"
fi

# Check if a file should be excluded from linting
should_exclude_file() {
    local rel_file="$1"
    for pattern in "${LINT_IGNORE_PATTERNS[@]}"; do
        # shellcheck disable=SC2053,SC2250 # Right side needs to be a glob which doesn't work when quoted ?!
        if [[ "${rel_file}" == $pattern ]]; then
            return 0
        fi
    done
    return 1
}

# Function to convert absolute path to path relative to lithium project root
convert_to_relative_path() {
    local absolute_path="$1"
    local relative_path=""

    case "${absolute_path}" in
        */elements/003-lithium/*)
            relative_path="${absolute_path##*/elements/003-lithium/}"
            echo "/${relative_path}"
            ;;
        */elements/003-lithium)
            echo "lithium"
            ;;
        */lithiumn/*)
            relative_path="${absolute_path##*/lithiumn/}"
            echo "lithium/${relative_path}"
            ;;
        *)
            echo "${absolute_path}"
            ;;
    esac
}

# Function to safely change directory with error handling
safe_cd() {
    local target_dir="$1"
    if ! cd "${target_dir}"; then
        print_error "Failed to change directory to ${target_dir}"
        return 1
    fi
    return 0
}

# Function to get file size safely
get_file_size() {
    local file_path="$1"
    local size
    # shellcheck disable=SC2154 # STAT defined externally in framework.sh
    if size=$("${STAT}" -c %s "${file_path}" 2>/dev/null); then
        echo "${size}"
        return 0
    else
        echo "0"
        return 1
    fi
}

# Function to get the full path to a configuration file
# This centralizes config file access and handles the configs/ subdirectory
get_config_path() {
    local config_file="$1"
    local config_path="${LITHIUM_ROOT}/tests/configs/${config_file}"
    echo "${config_path}"
}

# Function to extract web server port from a JSON configuration file
extract_web_server_port() {
    local config_file="$1"
    local default_port=5000
    
    # Extract port using grep and sed (simple approach, could be improved with jq)
    local port
    port=$(jq -r '.WebServer.Port // 5000' "${config_file}" 2>/dev/null)
    if jq -r '.WebServer.Port // 5000' "${config_file}" >/dev/null 2>&1 && [[ -n "${port}" ]] && [[ "${port}" != "null" ]]; then
        echo "${port}"
        return 0
    fi
   
    # Return default port if extraction fails
    echo "${default_port}"
    return 0
}

# Function to validate JSON syntax in a file
validate_json() {
    local file="$1"
    
    # Use jq if available for proper JSON validation
    jq . "${file}" > /dev/null 2>&1
    if jq . "${file}" >/dev/null 2>&1; then
        print_result 0 "Response contains valid JSON"
        return 0
    else
        print_result 1 "Response contains invalid JSON"
        return 1
    fi
}

# Function to get the web server port from config (if not already defined)
get_webserver_port() {
    local config="$1"
    local port
    
    port=$(jq -r '.WebServer.Port // 8080' "${config}" 2>/dev/null)
    if [[ -z "${port}" ]] || [[ "${port}" = "null" ]]; then
        port=8080
    fi
    echo "${port}"
}

# Function to handle timing file operations
handle_timing_file() {
    local timing_file="$1"
    # local test_name="$2"

    if [[ -f "${timing_file}" ]]; then
        local timing
        timing=$(cat "${timing_file}" 2>/dev/null || echo "0.000")
        local timing_calc
        timing_calc=$(echo "scale=6; ${timing} * 1000" | bc 2>/dev/null || echo "0.000")
        local timing_ms
        # shellcheck disable=SC2312 # Intentional command substitution to format timing
        timing_ms=$(printf "%.3f" "${timing_calc}" 2>/dev/null || echo "0.000")
        echo "${timing_ms}"
    else
        echo "0.000"
    fi
}

# Function to test file downloads
test_file_download() {
    local url="$1"
    local output_file="$2"
    local file_type="$3"
    local result_marker="$4"
    local result_file="$5"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 --compressed \"${url}\""
    if curl -s --max-time 10 --compressed "${url}" > "${output_file}"; then
        if [[ -s "${output_file}" ]]; then
            echo "${result_marker}_PASSED" >> "${result_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully downloaded ${file_type} file ($(wc -c < "${output_file}" || true) bytes)"
        else
            echo "${result_marker}_FAILED" >> "${result_file}"
            return 1
        fi
    else
        echo "${result_marker}_FAILED" >> "${result_file}"
        return 1
    fi
    return 0
}

# Function to collect timing data from multiple files
collect_timing_data() {
    local log_prefix="$1"
    local log_suffix="$2"
    local total_time_var="$3"
    local count_var="$4"

    local timing_files=(
        "${log_prefix}_${log_suffix}_trailing_slash.timing"
        "${log_prefix}_${log_suffix}_redirect.timing"
        "${log_prefix}_${log_suffix}_content.timing"
        "${log_prefix}_${log_suffix}_initializer.timing"
    )

    local total="0"
    local count=0

    for timing_file in "${timing_files[@]}"; do
        if [[ -f "${timing_file}" ]]; then
            local timing_value
            timing_value=$(tr -d '\n' < "${timing_file}" 2>/dev/null || echo "0")
            if [[ -n "${timing_value}" ]] && [[ "${timing_value}" =~ ^[0-9]+\.[0-9]+$ ]]; then
                # shellcheck disable=SC2312 # Intentional command substitution with || true to ignore return value
                if (( $(echo "${timing_value} > 0" | bc -l 2>/dev/null || echo "0") )); then
                    total=$(echo "scale=6; ${total} + ${timing_value}" | bc 2>/dev/null || echo "${total}")
                    count=$(( count + 1 ))
                fi
            fi
        fi
    done

    # Use eval to set the variables in the caller's scope
    eval "${total_time_var}=\"${total}\""
    eval "${count_var}=\"${count}\""
}
