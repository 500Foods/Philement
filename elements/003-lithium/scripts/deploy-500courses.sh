#!/bin/bash
#
# Deploy script for 500courses Lithium instance
# Deploys to /fvl/tnt/t-500courses/lithium while preserving its config
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCAL_DEPLOY_DIR="$PROJECT_ROOT/dist-deploy-500courses"
TARGET_DEPLOY_DIR="/fvl/tnt/t-500courses/lithium"

cd "$PROJECT_ROOT"

echo "=== Lithium Deploy (500courses) ==="
echo "Starting at: $(date)"
echo "Building locally to: $LOCAL_DEPLOY_DIR"
echo "Target: $TARGET_DEPLOY_DIR"
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

# Post-build steps - skip config copy to preserve 500courses config
echo "[Post 1/2] Generating style registry..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run style:registry &

echo "[Post 2/2] Generating icon registry..."
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run icon:registry &

# Wait for parallel steps to complete
wait

echo ""
echo "=== Final steps (sequential) ==="

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
echo "  Target: $TARGET_DEPLOY_DIR/"

FILE_COUNT=$(find "$LOCAL_DEPLOY_DIR" -type f 2>/dev/null | wc -l)
DIR_SIZE=$(du -sh "$LOCAL_DEPLOY_DIR" 2>/dev/null | cut -f1)
echo "  Files to sync: $FILE_COUNT files ($DIR_SIZE)"

mkdir -p "$TARGET_DEPLOY_DIR"

# Sync preserving target config
# First sync everything except config
echo "  Syncing assets, src, and static files..."
rsync -a --delete --no-perms --size-only --whole-file --no-owner --no-group --inplace --exclude='config/' --stats "$LOCAL_DEPLOY_DIR/" "$TARGET_DEPLOY_DIR/"

# Then sync config files selectively
echo "  Syncing config files..."
if [ -d "$LOCAL_DEPLOY_DIR/config" ]; then
  mkdir -p "$TARGET_DEPLOY_DIR/config"
  
  # Sync tabulator config (generic configuration)
  if [ -d "$LOCAL_DEPLOY_DIR/config/tabulator" ]; then
    mkdir -p "$TARGET_DEPLOY_DIR/config/tabulator"
    rsync -a --delete --no-perms --size-only --whole-file --no-owner --no-group --inplace "$LOCAL_DEPLOY_DIR/config/tabulator/" "$TARGET_DEPLOY_DIR/config/tabulator/" 2>/dev/null || true
  fi
  
# Sync generated config files (icons, style registries, coltypes-update) but preserve lithium.json
   cp -f "$LOCAL_DEPLOY_DIR/config/icons-dev.txt" "$TARGET_DEPLOY_DIR/config/" 2>/dev/null || true
   cp -f "$LOCAL_DEPLOY_DIR/config/icons-usr.txt" "$TARGET_DEPLOY_DIR/config/" 2>/dev/null || true
   # Copy style and coltype files but skip lithium.json and lithium.json.br
   for f in lithium-style-vars.json lithium-style-vars.json.br lithium-style-classes.json lithium-style-classes.json.br coltypes-update.json; do
     if [ -f "$LOCAL_DEPLOY_DIR/config/$f" ]; then
       cp -f "$LOCAL_DEPLOY_DIR/config/$f" "$TARGET_DEPLOY_DIR/config/"
     fi
   done
   # DO NOT overwrite lithium.json - keep the 500courses version
   echo "  Preserving 500courses lithium.json config..."
fi

# Always copy index.html and other root files as they likely have changed but size has not
for f in index.html manifest.json service-worker.js version.json; do
  if [ -f "${LOCAL_DEPLOY_DIR}/${f}" ]; then
    cp -f "${LOCAL_DEPLOY_DIR}/${f}" "${TARGET_DEPLOY_DIR}/"
  fi
done
# Copy root-level .br files
for f in "${LOCAL_DEPLOY_DIR}"/*.br; do
  if [ -f "$f" ]; then
    cp -f "$f" "${TARGET_DEPLOY_DIR}/"
  fi
done

echo "  ✓ Build synchronized to target"

echo ""
echo "=== Deploy Complete ==="
echo "Version: $(grep '"version"' version.json | sed 's/.*: "\([^"]*\)".*/\1/')"
echo "Finished at: $(date)"