#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# Payload Embedding Script for Hydrogen Release Build
# 
# This script embeds an encrypted payload into the release executable.
# ──────────────────────────────────────────────────────────────────────────────

set -e

EXECUTABLE="$1"
PAYLOAD_FILE="$2"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NORMAL='\033[0m'
INFO='🛈'
PASS='✅'
FAIL='❌'

if [ -z "$EXECUTABLE" ] || [ -z "$PAYLOAD_FILE" ]; then
    echo "Usage: $0 <executable> <payload_file>"
    exit 1
fi

if [ ! -f "$EXECUTABLE" ]; then
    printf "${RED}${FAIL} Executable not found: $EXECUTABLE${NORMAL}\n"
    exit 1
fi

if [ ! -f "$PAYLOAD_FILE" ]; then
    printf "${RED}${FAIL} Payload file not found: $PAYLOAD_FILE${NORMAL}\n"
    exit 1
fi

printf "${CYAN}${INFO} Embedding encrypted payload into executable...${NORMAL}\n"

# Get payload size
PAYLOAD_SIZE=$(stat -c %s "$PAYLOAD_FILE")

# Create temporary file
TEMP_FILE="${EXECUTABLE}.tmp"

# Combine executable, payload, marker, and size
cat "$EXECUTABLE" "$PAYLOAD_FILE" > "$TEMP_FILE"
printf "<<< HERE BE ME TREASURE >>>" >> "$TEMP_FILE"

# Append payload size as 8-byte hex (little-endian)
printf "$(printf '%016x' $PAYLOAD_SIZE | sed 's/\(..\)/\\x\1/g')" >> "$TEMP_FILE"

# Replace original executable
mv "$TEMP_FILE" "$EXECUTABLE"
chmod +x "$EXECUTABLE"

printf "${GREEN}${PASS} Payload embedded successfully (${PAYLOAD_SIZE} bytes)${NORMAL}\n"
