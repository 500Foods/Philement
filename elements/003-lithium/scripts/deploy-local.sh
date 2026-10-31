#!/bin/bash
#
# Deploy Lithium to a local copy (default ~/pintura/web) for offline/local dev.
# Quick build (no tests), preserves the target's config/lithium.json.
# Usage: npm run deploy:local            (target ~/pintura/web)
#        LITHIUM_LOCAL_TARGET=/path npm run deploy:local
#
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCAL_DEPLOY_DIR="$PROJECT_ROOT/dist-deploy-local"
TARGET="${LITHIUM_LOCAL_TARGET:-$HOME/pintura/web}"
cd "$PROJECT_ROOT"
# public/assets/courses is a symlink into /fvl (Ceph); if dangling, park it so Vite can build
PARKED=
if [ -L public/assets/courses ] && [ ! -e public/assets/courses ]; then
  mv public/assets/courses .courses-link-parked; PARKED=1
  trap '[ -n "$PARKED" ] && mv .courses-link-parked public/assets/courses' EXIT
  echo "(courses assets unavailable: /fvl offline, skipping)"
fi
echo "=== Lithium deploy:local -> $TARGET ==="
npm run validate:tabulator
npm run templates:copy
rm -rf "$LOCAL_DEPLOY_DIR"; mkdir -p "$LOCAL_DEPLOY_DIR"
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run build:deploy-only
[ -d public/assets ] && rsync -a --safe-links public/assets/ "$LOCAL_DEPLOY_DIR/assets/" || true
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run style:registry
LITHIUM_DEPLOY="$LOCAL_DEPLOY_DIR" npm run icon:registry
mkdir -p "$LOCAL_DEPLOY_DIR/config"
cp -rn config/. "$LOCAL_DEPLOY_DIR/config/" 2>/dev/null || true
mkdir -p "$TARGET"
# Keep the target's own lithium.json (point it at local hydrogen)
rsync -a --delete --exclude='config/lithium.json' --exclude='config/lithium.json.br' "$LOCAL_DEPLOY_DIR/" "$TARGET/"
[ -f "$TARGET/config/lithium.json" ] || cp config/lithium.json "$TARGET/config/lithium.json"
echo "=== Done: $(grep '"version"' version.json | sed 's/.*: "\([^"]*\)".*/\1/') at $(date) ==="
