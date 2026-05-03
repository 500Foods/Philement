#!/bin/bash
#
# Parallelized deploy script for Lithium
# Runs independent post-build steps in parallel for faster deploys
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCAL_DEPLOY_DIR="$PROJECT_ROOT/dist-deploy"
TIMESTAMP_CACHE="$PROJECT_ROOT/.deploy-timestamps"

cd "$PROJECT_ROOT"

# Function to save file timestamps for incremental builds
save_timestamps() {
  if [ -d "$LOCAL_DEPLOY_DIR" ]; then
    echo "Saving timestamps for incremental builds..."
    find "$LOCAL_DEPLOY_DIR" -type f -exec stat -c "%Y %s %n" {} \; > "$TIMESTAMP_CACHE" 2>/dev/null || true
  fi
}

# Function to restore timestamps for unchanged files
restore_timestamps() {
  if [ -f "$TIMESTAMP_CACHE" ] && [ -d "$LOCAL_DEPLOY_DIR" ]; then
    echo "Restoring timestamps for unchanged files..."
    while read -r mtime size file; do
      # Only restore timestamp if file exists and size matches
      if [ -f "$file" ] && [ "$(stat -c %s "$file" 2>/dev/null || echo 0)" = "$size" ]; then
        touch -m -d "@$mtime" "$file" 2>/dev/null || true
      fi
    done < "$TIMESTAMP_CACHE"
  fi
}

echo "=== Lithium Deploy (Parallelized) ==="
echo "Starting at: $(date)"
echo "Building locally to: $LOCAL_DEPLOY_DIR"
echo ""

# Pre-build steps (sequential - each depends on previous)
echo "[1/8] Validating Tabulator config..."
npm run validate:tabulator

# Run tests and CSS linting in parallel
echo "[2/8] Running tests with coverage..."
npm run test:coverage &

echo "[3/8] Linting CSS..."
npm run lint:css &

# Wait for both parallel tasks to complete
wait

echo "[4/8] Copying coverage..."
npm run coverage:copy

echo "[5/8] Copying templates..."
npm run templates:copy

echo "[6/8] Bumping version..."
npm run version:bump

# Clean local deploy directory
echo "[7/8] Cleaning local deploy directory..."
rm -rf "$LOCAL_DEPLOY_DIR"
mkdir -p "$LOCAL_DEPLOY_DIR"

# Build locally (must be sequential)
echo "[8/8] Building with Vite..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run build:deploy-only

echo "[8.5/8] Preserving timestamps for static assets..."
# Copy static assets with preserved timestamps to avoid unnecessary rsync transfers
if [ -d "$PROJECT_ROOT/public/assets" ]; then
  echo "  Copying static assets (fonts, videos, images) with preserved timestamps..."
  find "$PROJECT_ROOT/public/assets" -type f \( -name "*.woff*" -o -name "*.ttf" -o -name "*.mp4" -o -name "*.webm" -o -name "*.png" -o -name "*.jpg" -o -name "*.svg" -o -name "*.ico" \) \
    -exec bash -c '
      src="$1"
      dst="$2/${src#'"$PROJECT_ROOT/public/"'}"
      mkdir -p "$(dirname "$dst")"
      cp -p "$src" "$dst" 2>/dev/null || cp "$src" "$dst"
    ' _ {} "$LOCAL_DEPLOY_DIR" \;
  echo "  ✓ Static assets copied with timestamps preserved"
fi

echo ""
echo "=== Post-build steps (running in parallel) ==="

# Post-build steps that can run in parallel on local build
# These all work on different files/directories
echo "[Post 1/3] Generating style registry..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run style:registry &

echo "[Post 2/3] Generating icon registry..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run icon:registry &

echo "[Post 3/3] Copying config..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run deploy:config &

# Wait for all parallel steps to complete
wait

echo ""
echo "=== Final steps (sequential) ==="

# These must run sequentially (postbuild must complete before brotli)
echo "[Final 1/3] Running postbuild..."
npm run deploy:postbuild -- "$LOCAL_DEPLOY_DIR"

echo "[Final 2/3] Pruning old assets..."
npm run deploy:prune -- "$LOCAL_DEPLOY_DIR"

echo "[Final 3/3] Generating Brotli compression..."
npm run deploy:brotli:fast -- "$LOCAL_DEPLOY_DIR"

echo ""
echo "=== Sync to target (incremental) ==="

echo "[Sync] Synchronizing build to target directory..."
echo "  Source: $LOCAL_DEPLOY_DIR/"
echo "  Target: $LITHIUM_DEPLOY/"

# Calculate and display sync statistics
FILE_COUNT=$(find "$LOCAL_DEPLOY_DIR" -type f 2>/dev/null | wc -l)
DIR_SIZE=$(du -sh "$LOCAL_DEPLOY_DIR" 2>/dev/null | cut -f1)
echo "  Files to sync: $FILE_COUNT files ($DIR_SIZE)"

# Create target directory if it doesn't exist
mkdir -p "$LITHIUM_DEPLOY"

# Use rsync optimized for slow network filesystems
# -a archive mode (recursive + preserve times)
# --delete removes files from target that don't exist in source
# --no-perms --no-owner --no-group minimizes metadata operations on slow FS
# --inplace updates files in place instead of temp files (faster on network FS)
# --info=progress2 shows detailed transfer progress
echo "  Starting rsync transfer..."
# rsync -a --delete --no-perms --times --whole-file --no-owner --no-group --inplace --info=progress2 "$LOCAL_DEPLOY_DIR/" "$LITHIUM_DEPLOY/"
find "$LITHIUM_DEPLOY" -type f -o -type d > /dev/null 2>&1
rsync -a --delete --no-perms --size-only --whole-file --no-owner --no-group --inplace --stats --info=progress2 "$LOCAL_DEPLOY_DIR/" "$LITHIUM_DEPLOY/"

# Always copy index.html as it likely has changed but its size has not
cp "${LOCAL_DEPLOY_DIR}"/*.* "${LITHIUM_DEPLOY}/"

echo "  ✓ Build synchronized to target"

echo "[Cleanup] Keeping local build directory for inspection..."
echo "  Local build preserved at: $LOCAL_DEPLOY_DIR"

echo ""
echo "=== Deploy Complete ==="
echo "Version: $(grep '"version"' version.json | sed 's/.*: "\([^"]*\)".*/\1/')"
echo "Finished at: $(date)"
