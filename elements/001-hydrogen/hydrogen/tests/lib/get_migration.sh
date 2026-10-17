#!/usr/bin/env bash

# get_migration.sh
# Generate SVG database diagram from migration JSON

# CHANGELOG
# 1.2.0 - 2026-10-16 - Require database.lua and database_<engine>.lua from the shared set
# 1.1.0 - 2026-10-16 - Load database.lua from elements/002-helium/helium
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

DESIGN_DIR="${HELIUM_DIR}/$2/migrations"
SHARED_DIR="${HELIUM_DIR}/helium"
if [[ ! -d "${DESIGN_DIR}" ]]; then
    echo "Error: design migrations not found: ${DESIGN_DIR}" >&2
    exit 1
fi
if [[ ! -f "${SHARED_DIR}/database.lua" && ! -f "${DESIGN_DIR}/database.lua" ]]; then
    echo "Error: database.lua not found in ${SHARED_DIR} or ${DESIGN_DIR}" >&2
    exit 1
fi
if [[ ! -f "${SHARED_DIR}/database_${1}.lua" && ! -f "${DESIGN_DIR}/database_${1}.lua" ]]; then
    echo "Error: database_${1}.lua not found in ${SHARED_DIR} or ${DESIGN_DIR}" >&2
    exit 1
fi

pushd "${DESIGN_DIR}" >/dev/null 2>&1 || exit 1
# Shared scripts first. ;; keeps Lua's default path so the migration file resolves here.
LUA_PATH="${SHARED_DIR}/?.lua;./?.lua;;" lua "${LIB_DIR}/get_migration.lua" "${1}" "${2}" "${3}" "${4}"
lua_status=$?
popd >/dev/null 2>&1 || exit 1
exit "${lua_status}"

