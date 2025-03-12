#!/bin/bash

# Configuration files (relative to TARGET)
LINTIGNORE=".lintignore"
LINTIGNORE_C=".lintignore-c"

# Determine target (file or directory) from command-line argument
TARGET="${1:-..}" # Default to parent directory (project root) when run from tests/
if [ ! -e "$TARGET" ]; then
    echo "Error: Target '$TARGET' does not exist." >&2
    exit 1
fi

# Ensure required files exist in TARGET directory
if [ ! -f "$TARGET/$LINTIGNORE" ]; then
    echo "Error: $TARGET/$LINTIGNORE not found." >&2
    exit 1
fi
if [ ! -f "$TARGET/$LINTIGNORE_C" ]; then
    echo "Error: $TARGET/$LINTIGNORE_C not found." >&2
    exit 1
fi

# Function to check if a file should be excluded
should_exclude() {
    local file="$1"
    # Convert file to absolute path and then relative to TARGET
    local abs_file=$(realpath "$file" 2>/dev/null || echo "$file")
    local rel_file=$(realpath --relative-to="$TARGET" "$abs_file" 2>/dev/null || echo "$abs_file")
    while IFS= read -r exclude_pattern; do
        exclude_pattern=$(echo "$exclude_pattern" | sed 's/#.*//;s/^[[:space:]]*//;s/[[:space:]]*$//') # Remove comments and trim
        [ -z "$exclude_pattern" ] && continue
        # Match the pattern against the relative file path
        if [[ "$rel_file" == $exclude_pattern ]]; then
            return 0 # Exclude
        fi
    done < "$TARGET/$LINTIGNORE"
    return 1 # Do not exclude
}

# Parse .lintignore-c for cppcheck settings
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

# Add suppressions from standalone lines
cppcheck_suppressions=$(grep -v '^#' "$TARGET/$LINTIGNORE_C" | grep '^suppress' | cut -d'=' -f2 | tr '\n' ' ' | sed 's/ $//')
cppcheck_args+=($cppcheck_suppressions)

# Lint C, H, and Inc files with cppcheck
if find "$TARGET" -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \) -print -quit 2>/dev/null | grep -q .; then
    find "$TARGET" -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \) | while read -r file; do
        if ! should_exclude "$file"; then
            cppcheck "${cppcheck_args[@]}" "$file" 2>&1
        fi
    done
fi