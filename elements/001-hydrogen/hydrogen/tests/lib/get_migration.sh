#!/usr/bin/env bash

#set -xeuo pipefail

# Script configuration
#SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#LIB_DIR="${SCRIPT_DIR}"

# Find global project root reliably
find_project_root() {
    local script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

    # Go up directories until we find the global project root (which contains elements/)
    local current_dir="${script_dir}"
    while [[ "${current_dir}" != "/" ]]; do
        # Check if this is the global project root (has elements/ subdirectory)
        if [[ -d "${current_dir}/elements" ]]; then
            echo "${current_dir}"
            return 0
        fi
        current_dir="$(dirname "${current_dir}")"
    done

    # Fallback: go up four levels from lib directory to reach project root
    # lib -> tests -> hydrogen -> 001-hydrogen -> elements -> project root
    current_dir="$(cd "${script_dir}/../../.." && pwd)"
    echo "${current_dir}"
    return 0
}

# Calculate absolute path to helium directory
PROJECT_DIR="$(find_project_root)"
HELIUM_DIR="${PROJECT_DIR}/elements/002-helium"

pushd  "${HELIUM_DIR}/$2/migrations" >/dev/null 2>&1 || return
lua "${PROJECT_DIR}/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua" "${1}" "${2}" "${3}" "${4}"
popd >/dev/null 2>&1 || return

