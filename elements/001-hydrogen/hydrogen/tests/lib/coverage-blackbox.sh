#!/bin/bash

# Coverage Blackbox Library
# This script provides functions for calculating blackbox test coverage,

# LIBRARY FUNCTIONS
# calculate_blackbox_coverage()

# CHANGELOG
# 2.0.0 - 2025-08-04 - GCOV Optimization Adventure
# 1.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.0.0 - 2025-07-21 - Initial version with combined coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_BLACKBOX_GUARD}" ]] && return 0
export COVERAGE_BLACKBOX_GUARD="true"

# Library metadata
COVERAGE_BLACKBOX_NAME="Coverage Blackbox Library"
COVERAGE_BLACKBOX_VERSION="2.0.0"
print_message "${COVERAGE_BLACKBOX_NAME} ${COVERAGE_BLACKBOX_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
[[ -n "${COVERAGE_COMMON_GUARD}" ]] || source "${LIB_DIR}/coverage-common.sh"
