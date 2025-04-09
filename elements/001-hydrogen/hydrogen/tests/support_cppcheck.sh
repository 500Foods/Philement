#!/bin/bash

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
    local abs_file=$(realpath "$file" 2>/dev/null || echo "$file")
    local rel_file=$(realpath --relative-to="$TARGET" "$abs_file" 2>/dev/null || echo "$abs_file")
    for pattern in "${exclude_patterns[@]}"; do
        [[ "$rel_file" == $pattern ]] && return 0
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
cppcheck_args+=($cppcheck_suppressions)

# Collect files with inline filtering
files=($(find "$TARGET" -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \) -exec bash -c 'for f; do ! should_exclude "$f" && echo "$f"; done' _ {} + 2>/dev/null))

if [ ${#files[@]} -gt 0 ]; then
    echo "Running cppcheck on ${#files[@]} files
    ..."
    cppcheck -j24 --quiet "${cppcheck_args[@]}" "${files[@]}" 2>&1
else
    echo "No files to check."
fi