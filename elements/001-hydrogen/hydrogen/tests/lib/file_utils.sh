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

# CHANGELOG
# 1.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.2.0 - 2025-07-30 - Added common should_exclude_file() from other scripts
# 1.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Guard clause to prevent multiple sourcing
[[ -n "${FILE_UTILS_GUARD:-}" ]] && return 0
export FILE_UTILS_GUARD="true"

# Library metadata
FILE_UTILS_NAME="File Utilities Library"
FILE_UTILS_VERSION="1.2.1"
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

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    local relative_path=""

    case "${absolute_path}" in
        */elements/001-hydrogen/hydrogen/*)
            relative_path="${absolute_path##*/elements/001-hydrogen/hydrogen/}"
            echo "hydrogen/${relative_path}"
            ;;
        */elements/001-hydrogen/hydrogen)
            echo "hydrogen"
            ;;
        */hydrogen/*)
            relative_path="${absolute_path##*/hydrogen/}"
            echo "hydrogen/${relative_path}"
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
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    local config_path="${script_dir}/../configs/${config_file}"
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
