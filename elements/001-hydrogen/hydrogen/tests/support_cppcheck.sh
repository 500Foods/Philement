#!/bin/bash
#
# About this Script
#
# Hydrogen CP Check Utility
# Used to encapsulate the logic for running cppcheck on Hydrogen source files
#
NAME_SCRIPT="Hydrogen CPP Check Utility"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

LINTIGNORE=".lintignore"
LINTIGNORE_C=".lintignore-c"
TARGET="${1:-..}"

if [ ! -e "$TARGET" ]; then
    echo "Error: Target '$TARGET' does not exist." >&2
    exit 1
fi
if [ ! -f "$TARGET/$LINTIGNORE" ]; then
    echo "Error: $TARGET/$LINTIGNORE not found." >&2
    exit 1
fi
if [ ! -f "$TARGET/$LINTIGNORE_C" ]; then
    echo "Error: $TARGET/$LINTIGNORE_C not found." >&2
    exit 1
fi

# Cache exclude patterns
exclude_patterns=()
while IFS= read -r pattern; do
    pattern=$(echo "$pattern" | sed 's/#.*//;s/^[[:space:]]*//;s/[[:space:]]*$//')
    [ -z "$pattern" ] && continue
    exclude_patterns+=("$pattern")
done < "$TARGET/$LINTIGNORE"

should_exclude() {
    local file="$1"
    local rel_file="${file#"$TARGET"/}"
    for pattern in "${exclude_patterns[@]}"; do
        shopt -s extglob
        if [[ "$rel_file" == @($pattern) ]]; then
            shopt -u extglob
            return 0
        fi
        shopt -u extglob
    done
    return 1
}

# Parse .lintignore-c
cppcheck_args=()
while IFS='=' read -r key value; do
    case "$key" in
        "enable") cppcheck_args+=("--enable=$value") ;;
        "include") cppcheck_args+=("--include=$value") ;;
        "check-level") cppcheck_args+=("--check-level=$value") ;;
        "template") cppcheck_args+=("--template=$value") ;;
        "option") cppcheck_args+=("$value") ;;
        "suppress") cppcheck_args+=("--suppress=$value") ;;
        *) ;;
    esac
done < <(grep -v '^#' "$TARGET/$LINTIGNORE_C" | grep '=')

cppcheck_suppressions=$(grep -v '^#' "$TARGET/$LINTIGNORE_C" | grep '^suppress' | cut -d'=' -f2 | tr '\n' ' ' | sed 's/ $//')
if [ -n "$cppcheck_suppressions" ]; then
    read -ra suppression_array <<< "$cppcheck_suppressions"
    cppcheck_args+=("${suppression_array[@]}")
fi

# Collect files with inline filtering using grep -v for exclusions
find_cmd="find \"$TARGET\" -type f \( -name \"*.c\" -o -name \"*.h\" -o -name \"*.inc\" \)"
for pattern in "${exclude_patterns[@]}"; do
    # Convert glob pattern to grep pattern
    grep_pattern="${pattern//\*/.*}"
    find_cmd="$find_cmd | grep -v \"$grep_pattern\""
done
mapfile -t files < <(eval "$find_cmd")

if [ ${#files[@]} -gt 0 ]; then
    echo "Running cppcheck on ${#files[@]} files
    ..."
    cppcheck -j24 --quiet "${cppcheck_args[@]}" "${files[@]}" 2>&1
else
    echo "No files to check."
fi
