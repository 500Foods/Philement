#!/usr/bin/env zsh

# shellcheck disable=SC2154  # words is a zsh completion variable
autoload -Uz compinit
compinit

alias mkr='source ~/.mku-zsh-complete.zsh && echo "Reloaded mku tests"'
# alias mku='~/Philement/elements/001-hydrogen/hydrogen && extras/run-unity-test.sh'

# First remove any existing mku alias
unalias mku 2>/dev/null || true

# Create a separate completion that doesn't rely on the alias
_mku_complete_simple() {
    local cur="${words[CURRENT]}"
    local -a tests

    # Get tests from cache or directory
    if [[ -f "${HOME}/.mku_cache" ]]; then
        read -r -a tests < "${HOME}/.mku_cache"
    else
        local unity_src_dir="${HOME}/Philement/elements/001-hydrogen/hydrogen/tests/unity/src"
        if [[ -d "${unity_src_dir}" ]]; then
            read -r -a tests < <(find "${unity_src_dir}" -name "*.c" -exec basename {} .c \; | sort -u || true)
        fi
    fi

    # Debug output
    # echo "DEBUG: _mku_complete_simple called, cur='$cur', ${#tests[@]} tests" >&2

    # Generate completions that match current word
    local -a matches
    for test in "${tests[@]}"; do
        if [[ "${test}" == "${cur}"* ]]; then
            matches+=("${test}")
        fi
    done

    # echo "DEBUG: Found ${#matches[@]} matches" >&2

    # Set the completions
    compadd -a matches
}

# Register this completion for mku
compdef _mku_complete_simple mku

# Define mku as a function instead of alias (this might work better for completion)
mku() {
    local test_name="$1"
    local project_root="${HOME}/Philement/elements/001-hydrogen/hydrogen"

    # If no test name provided, show available tests
    if [[ -z "${test_name}" ]]; then
        echo "Available tests (use mku <test_name>):"
        if [[ -f "${HOME}/.mku_cache" ]]; then
            cache_content=$(< "${HOME}/.mku_cache")
            test_count=$(echo "${cache_content}" | wc -w)
            echo "Found ${test_count} tests. Examples:"
            echo "${cache_content}" | tr ' ' '\n' | grep -i terminal | head -10 || true
            echo "... (total ${test_count} tests)"
        else
            echo "Cache not found. Run 'mkr' to reload cache."
        fi
        return 1
    fi

    # If test name provided, run the actual command
    cd "${project_root}" || return 1
    extras/run-unity-test.sh "${test_name}" || return 1
}

# echo "DEBUG: Using function-based mku instead of alias" >&2

# Enable case-insensitive matching and menu-style completion
zstyle ':completion:*' matcher-list 'm:{a-zA-Z}={A-Za-z}'  # Case-insensitive
zstyle ':completion:*:descriptions' format '%B%d%b'  # Bold descriptions
zstyle ':completion:*' menu select  # Menu selection

# Cache management function
_mku_cache_update() {
    local project_root="${HOME}/Philement/elements/001-hydrogen/hydrogen"
    local unity_src_dir="${project_root}/tests/unity/src"

    if [[ -d "${unity_src_dir}" ]]; then
        find "${unity_src_dir}" -name "*.c" -exec basename {} .c \; | sort -u | tr '\n' ' ' > "${HOME}/.mku_cache" || return 1 || true
        cache_content=$(< "${HOME}/.mku_cache")
        test_count=$(echo "${cache_content}" | wc -w)
        echo "Reloaded ${test_count} mku tests"
    else
        echo "Error: Unity test directory not found"
        return 1
    fi
}

# Update the mkr alias to use our cache update
alias mkr='_mku_cache_update'
      
