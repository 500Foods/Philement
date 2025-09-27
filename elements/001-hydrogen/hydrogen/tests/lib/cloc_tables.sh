#!/usr/bin/env bash

# CLOC Tables Generator
# Generates the two cloc tables (main code table and extended statistics) without test framework overhead

# CHANGELOG
# 1.1.0 - 2025-09-23 - Updated to use CLOC Library 6.0.0 with table section breaks
# 1.0.0 - 2025-09-16 - Initial creation, extracted from test_98_code_size.sh

set -euo pipefail

# Test configuration
TEST_NAME="Cloc Tables"
TEST_ABBR="CLT"
TEST_NUMBER="CT"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
if [[ -z "${FRAMEWORK_GUARD:-}" ]]; then
    source "$(dirname "${BASH_SOURCE[0]}")/framework.sh" >/dev/null 2>&1
    setup_test_environment >/dev/null 2>&1
fi

# Create temporary files for cloc analysis
CLOC_OUTPUT=""
CLOC_DATA=""
CLOC_OUTPUT=$(mktemp)
CLOC_DATA=$(mktemp)

# Run cloc analysis
run_cloc_analysis "." ".lintignore" "${CLOC_OUTPUT}" "${CLOC_DATA}"

# shellcheck disable=SC2181 # We want to check the exit code of the function
if [[ $? -eq 0 ]]; then
    # Output the main code table
    cat "${CLOC_OUTPUT}"

    # Check if stats table exists and output it
    cloc_stats="${CLOC_OUTPUT%.txt}_stats.txt"
    if [[ -f "${cloc_stats}" ]]; then
        cat "${cloc_stats}"
    fi

    # Save JSON data to results directory for capture by test_00_all.sh
    if [[ -f "${CLOC_DATA}" ]]; then
        # shellcheck disable=SC2154 # RESULTS_DIR defined externally in framework.sh
        cp "${CLOC_DATA}" "${RESULTS_DIR}/cloc_main_data.json"
    fi

    # Save stats JSON data if it exists
    if [[ -f "${CLOC_DATA%.json}_stats.json" ]]; then
        cp "${CLOC_DATA%.json}_stats.json" "${RESULTS_DIR}/cloc_stats_data.json"
    fi
else
    echo "Error: cloc analysis failed" >&2
    exit 1
fi

# Clean up temporary files
rm -f "${CLOC_OUTPUT}" "${CLOC_DATA}"
