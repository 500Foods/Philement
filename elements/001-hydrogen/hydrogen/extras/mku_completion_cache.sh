# Dynamic mku completions for zsh (generated on Mon Dec  1 16:09:32 PST 2025)
_mku() {
    # Only proceed if we're actually completing (not during shell startup)
    [[ -z "$compstate" ]] && return 0

    local cur="${words[CURRENT]}"
    local project_root unity_src_dir cache_file

    # Use cached project root path to avoid repeated directory checks
    if [[ -z "$_MKU_PROJECT_ROOT" ]]; then
        _MKU_PROJECT_ROOT="~/Philement/elements/001-hydrogen/hydrogen"
        [[ ! -d "$_MKU_PROJECT_ROOT" ]] && return 1
    fi

    unity_src_dir="$_MKU_PROJECT_ROOT/tests/unity/src"
    cache_file="${HOME}/.mku_cache"

    # Fast cache check - only rebuild if cache doesn't exist or is outdated
    if [[ ! -f "$cache_file" || "$unity_src_dir" -nt "$cache_file" ]]; then
        # Use find with -printf for faster execution when available
        if find "$unity_src_dir" -name "*.c" -printf "%f\n" >/dev/null 2>&1; then
            _mku_tests=($(find "$unity_src_dir" -name "*.c" -printf "%f\n" | sed 's/\.c$//' | sort -u))
        else
            _mku_tests=($(find "$unity_src_dir" -name "*.c" -exec basename {} .c \; | sort -u))
        fi
        # Store in global array and cache file
        print -l "${_mku_tests[@]}" >| "$cache_file"
    else
        # Load from cache
        _mku_tests=($(< "$cache_file"))
    fi

    # Only complete first argument (CURRENT = 1 for first arg, 2 for second, etc.)
    [[ $CURRENT -gt 1 ]] && return 0

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
