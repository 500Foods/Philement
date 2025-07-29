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

if [[ -z "${EXECUTABLE}" ]] || [[ -z "${PAYLOAD_FILE}" ]]; then
    echo "Usage: $0 <executable> <payload_file>"
    exit 1
fi

if [[ ! -f "${EXECUTABLE}" ]]; then
    printf "%s%s Executable not found: %s%s\n" "${RED}" "${FAIL}" "${EXECUTABLE}" "${NORMAL}"
    exit 1
fi

if [[ ! -f "${PAYLOAD_FILE}" ]]; then
    printf "%s%s Payload file not found: %s%s\n" "${RED}" "${FAIL}" "${PAYLOAD_FILE}" "${NORMAL}"
    exit 1
fi

printf "%s%s Embedding encrypted payload into executable...%s\n" "${CYAN}" "${INFO}" "${NORMAL}"

# Get payload size
PAYLOAD_SIZE=$(stat -c %s "${PAYLOAD_FILE}")
echo "${PAYLOAD_SIZE}"

# Create temporary file
TEMP_FILE="${EXECUTABLE}.tmp"

# Combine executable, payload, marker, and size
cat "${EXECUTABLE}" "${PAYLOAD_FILE}" > "${TEMP_FILE}"
printf "<<< HERE BE ME TREASURE >>>" >> "${TEMP_FILE}"

# Append payload size as 8-byte binary (big-endian)
printf '%016x' "${PAYLOAD_SIZE}" | xxd -r -p >> "${TEMP_FILE}"

# Replace original executable
mv "${TEMP_FILE}" "${EXECUTABLE}"
chmod +x "${EXECUTABLE}"

printf "%s%s Payload embedded successfully (%s bytes)%s\n" "${GREEN}" "${PASS}" "${PAYLOAD_SIZE}" "${NORMAL}"
