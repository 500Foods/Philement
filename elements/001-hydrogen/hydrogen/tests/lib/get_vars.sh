#!/usr/bin/env bash

# getvars.sh - Extract environment variables from files
# Utility script to find and format environment variables for whitelisting
#
# Usage: ./getvars.sh [file|directory]
#
# Auto-detects file type based on extension:
#   .sh files: Searches for ${VARIABLE} patterns in shell scripts
#   .json files: Searches for ${env.VARIABLE} patterns in JSON files
#   Directories/other: Searches for both patterns in all files
#
# Output format matches the whitelist array format in test_03_shell.sh
#
# Examples:
#   ./getvars.sh tests/configs/                    # All vars in configs
#   ./getvars.sh tests/lib/framework.sh --shell    # Shell vars in framework.sh
#   ./getvars.sh . --json                          # JSON vars in entire project

# Shared functions for environment variable extraction
# These are available both when sourced and when executed

# Function to extract environment variables from files
# Usage: extract_env_vars_from_files <path> [json_only|shell_only|both]
extract_env_vars_from_files() {
    local search_path="${1}"
    local mode="${2:-both}"

    local search_json=false
    local search_shell=false

    case "${mode}" in
        "json_only") search_json=true ;;
        "shell_only") search_shell=true ;;
        "both") search_json=true; search_shell=true ;;
        *) echo "Invalid mode: ${mode}" >&2; return 1 ;;
    esac

    local all_vars=""

    if [[ "${search_json}" == true ]]; then
        # Search for ${env.VARIABLE} in JSON files
        local json_vars
        # shellcheck disable=SC2016,SC2312 # Single quotes intentional for sed; separate command for clarity
        json_vars=$(grep -r -h -o '\${env\.[A-Z_][A-Z0-9_]*}' "${search_path}" --include="*.json" 2>/dev/null | \
            sed 's/^\${env\.//' | sed 's/}$//' | sort | uniq || true)
        all_vars="${all_vars}${json_vars}"$'\n'
    fi

    if [[ "${search_shell}" == true ]]; then
        # Search for ${VARIABLE} in shell scripts
        local shell_vars
        # shellcheck disable=SC2016,SC2312 # Single quotes intentional for sed; separate command for clarity
        shell_vars=$(grep -r -h -o '\${[A-Z_][A-Z0-9_]*}' "${search_path}" --include="*.sh" 2>/dev/null | \
            sed 's/^\${//' | sed 's/}$//' | sort | uniq || true)
        all_vars="${all_vars}${shell_vars}"$'\n'
    fi

    # Return deduplicated list
    echo "${all_vars}" | grep -v '^$' | sort | uniq
}

# Function to format variables as quoted list for whitelist
format_vars_for_whitelist() {
    local vars="${1}"
    echo "${vars}" | sed 's/.*/    "&"/' | tr '\n' ' ' | fold -w 80 -s
    echo ""
}

# If this script is sourced (not executed), return early after defining functions
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    return 0
fi

# Executed mode - standalone script
set -euo pipefail

# Parse arguments
TARGET_PATH="."
while [[ $# -gt 0 ]]; do
    case $1 in
        -*)
            echo "Unknown option: $1"
            echo "Usage: $0 [path]"
            echo "Auto-detects file type: .sh for shell scripts, .json for JSON files"
            exit 1
            ;;
        *)
            TARGET_PATH="$1"
            shift
            ;;
    esac
done

# Auto-detect search patterns based on file extension
if [[ -f "${TARGET_PATH}" ]]; then
    # Single file
    case "${TARGET_PATH}" in
        *.sh)
            SEARCH_JSON=false
            SEARCH_SHELL=true
            ;;
        *.json)
            SEARCH_JSON=true
            SEARCH_SHELL=false
            ;;
        *)
            SEARCH_JSON=true
            SEARCH_SHELL=true
            ;;
    esac
else
    # Directory - search both
    SEARCH_JSON=true
    SEARCH_SHELL=true
fi

# Use the shared function
# shellcheck disable=SC2312 # Command substitution with conditional is intentional
ALL_VARS=$(extract_env_vars_from_files "${TARGET_PATH}" "$(if [[ "${SEARCH_JSON}" == true && "${SEARCH_SHELL}" == true ]]; then echo "both"; elif [[ "${SEARCH_JSON}" == true ]]; then echo "json_only"; else echo "shell_only"; fi)")

# Format and output
# shellcheck disable=SC2312 # Pipeline is intentional for processing
format_vars_for_whitelist "${ALL_VARS}"