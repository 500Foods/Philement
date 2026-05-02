#!/bin/bash
# Postbuild script: minify HTML (excluding page HTML files) and JS
# Usage: bash scripts/postbuild.sh [target_dir]
# If no argument, uses $LITHIUM_ROOT/dist

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TARGET_DIR="${1:-${LITHIUM_ROOT:-$PROJECT_ROOT}/dist}"
TMP_DIR="/tmp/lithium-page-html-bak"
BIN_DIR="$PROJECT_ROOT/node_modules/.bin"

# Validate target directory exists
if [ ! -d "$TARGET_DIR" ]; then
  echo "Error: Target directory '$TARGET_DIR' does not exist"
  exit 1
fi

PAGES_BASE="$TARGET_DIR/src/managers/profile-manager/pages"

# Create temp directory
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

# Move page HTML files to temp, preserving directory structure
if [ -d "$PAGES_BASE" ]; then
  find "$PAGES_BASE" -name "*.html" -type f -exec bash -c 'f="$1"; rel_path="${f#'"$TARGET_DIR/"'}"; target_dir="'"$TMP_DIR"'/$(dirname "$rel_path")"; mkdir -p "$target_dir"; mv "$f" "$target_dir/"' _ {} \;
fi

# Run html-minifier-terser on remaining HTML files in parallel
find "$TARGET_DIR" -name "*.html" -type f -print0 | \
  xargs -0 -P "$(nproc 2>/dev/null || echo 4)" -I {} \
  "$BIN_DIR/html-minifier-terser" \
    --collapse-whitespace \
    --remove-comments \
    --minify-js \
    --minify-css \
    --output "{}" \
    "{}"

# Move page HTML files back
if [ -d "$TMP_DIR" ]; then
  find "$TMP_DIR" -name "*.html" -type f -exec bash -c 'f="$1"; rel_path="${f#'"$TMP_DIR/"'}"; target="'"$TARGET_DIR"'/$rel_path"; mkdir -p "$(dirname "$target")"; mv "$f" "$target"' _ {} \;
  rm -rf "$TMP_DIR"
fi

# Minify service-worker.js
SW_FILE="$TARGET_DIR/service-worker.js"
if [ -f "$SW_FILE" ]; then
  "$BIN_DIR/terser" "$SW_FILE" \
    -o "$SW_FILE" \
    --compress \
    --mangle
fi

echo "Postbuild complete!"
