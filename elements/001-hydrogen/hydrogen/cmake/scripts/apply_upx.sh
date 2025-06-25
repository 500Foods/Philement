#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# UPX Compression Script for Hydrogen Release Build
# 
# This script applies UPX compression to the release executable if UPX is available.
# ──────────────────────────────────────────────────────────────────────────────

set -e

EXECUTABLE="$1"
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NORMAL='\033[0m'
WARN='⚠️'
PASS='✅'
INFO='🛈'

if [ -z "$EXECUTABLE" ]; then
    echo "Usage: $0 <executable>"
    exit 1
fi

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found: $EXECUTABLE"
    exit 1
fi

# Check if UPX is available
if ! command -v upx >/dev/null 2>&1; then
    printf "${YELLOW}${WARN} UPX not found, skipping compression${NORMAL}\n"
    exit 0
fi

# Apply UPX compression
printf "${CYAN}${INFO} Applying UPX compression with --best option...${NORMAL}\n"
upx --best "$EXECUTABLE"

printf "${GREEN}${PASS} UPX compression completed successfully${NORMAL}\n"
