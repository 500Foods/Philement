#!/usr/bin/env bash

# get_migration.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 1.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

#set -xeuo pipefail

# Script configuration
PROJECT_DIR="${HYDROGEN_ROOT}"
HELIUM_DIR="${HELIUM_ROOT}"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"

pushd  "${HELIUM_DIR}/$2/migrations" >/dev/null 2>&1 || return
lua "${LIB_DIR}/get_migration.lua" "${1}" "${2}" "${3}" "${4}"
popd >/dev/null 2>&1 || return

