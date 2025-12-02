#!/usr/bin/env zsh

# Dynamic mku completions for zsh (generated on Mon Dec  1 16:09:32 PST 2025)
_mku() {
    # Only proceed if we're actually completing (not during shell startup)
    [[ -z "${compstate}" ]] && return 0

    # shellcheck disable=SC2154  # words is a zsh completion variable
    # local cur="${words[CURRENT]}"
    local unity_src_dir cache_file

    # Use cur variable meaningfully for completion context
    # local completion_context="completion"
    # if [[ -n "${cur}" ]]; then
    #     completion_context="completion:${cur}"
    # fi

    # Use completion_context in a meaningful way for shellcheck
    # local debug_completion="${completion_context}"

    # Use cached project root path to avoid repeated directory checks
    if [[ -z "${_MKU_PROJECT_ROOT}" ]]; then
        _MKU_PROJECT_ROOT="${HOME}/Philement/elements/001-hydrogen/hydrogen"
        [[ ! -d "${_MKU_PROJECT_ROOT}" ]] && return 1
    fi

    unity_src_dir="${_MKU_PROJECT_ROOT}/tests/unity/src"
    cache_file="${HOME}/.mku_cache"

    # Fast cache check - only rebuild if cache doesn't exist or is outdated
    if [[ ! -f "${cache_file}" || "${unity_src_dir}" -nt "${cache_file}" ]]; then
        # Use find with -printf for faster execution when available
        if find "${unity_src_dir}" -name "*.c" -printf "%f\n" >/dev/null 2>&1; then
            # Create temporary file for the find results
            temp_file=$(mktemp)
            find "${unity_src_dir}" -name "*.c" -printf "%f\n" | sed 's/\.c$//' | sort -u > "${temp_file}" || true
            read -r -a _mku_tests < "${temp_file}" || true
            rm -f "${temp_file}" || true
        else
            # Create temporary file for the find results
            temp_file=$(mktemp)
            find "${unity_src_dir}" -name "*.c" -exec basename {} .c \; | sort -u > "${temp_file}" || true
            read -r -a _mku_tests < "${temp_file}" || true
            rm -f "${temp_file}" || true
        fi
        # Store in global array and cache file
        print -l "${_mku_tests[@]}" >| "${cache_file}"
    else
        # Load from cache
        read -r -a _mku_tests < "${cache_file}"
    fi

    # Only complete first argument (CURRENT = 1 for first arg, 2 for second, etc.)
    # shellcheck disable=SC2154  # CURRENT is a zsh completion variable
    [[ ${CURRENT} -gt 1 ]] && return 0

    # Use compadd for better performance
    compadd -a _mku_tests
}

# Register completion (lazy-loaded)
compdef _mku mku

# Pre-cache on first use to avoid delay during completion
_mku_precache() {
    [[ -f "${HOME}/.mku_cache" ]] && return
    _mku 2>/dev/null >/dev/null
}
autoload -Uz _mku_precache

# Embedded count for reload feedback
export MKU_NUM_TESTS=611
