#!/bin/bash

# Hydrogen Build Metrics Browser - Command Line Interface
# Generates SVG charts from Hydrogen build metrics data
#
# Usage: ./hbm_browser.sh [config_file] [output_file]
#
# If no arguments are provided, uses default configuration

# Configuration
SCRIPT_DIR=$(dirname "$(realpath "$0")") || true
DEFAULT_CONFIG="${SCRIPT_DIR}/hbm_browser_auto.json"
DEFAULT_OUTPUT="hydrogen_metrics_report.svg"

# Check if Node.js is available
if ! command -v node &> /dev/null; then
    echo "Error: Node.js is required but not installed."
    exit 1
fi

# Parse command line arguments
CONFIG_FILE="${1:-${DEFAULT_CONFIG}}"
OUTPUT_FILE="${2:-${DEFAULT_OUTPUT}}"

# Validate config file exists
if [[ ! -f "${CONFIG_FILE}" ]]; then
    echo "Error: Config file '${CONFIG_FILE}' not found."
    exit 1
fi

# Check if output directory exists, create if not
OUTPUT_DIR=$(dirname "${OUTPUT_FILE}")
if [[ ! -d "${OUTPUT_DIR}" ]]; then
    if ! mkdir -p "${OUTPUT_DIR}"; then
        echo "Error: Failed to create output directory '${OUTPUT_DIR}'"
        exit 1
    fi
fi

# Show starting message
printf "Hydrogen Build Metrics Browser - CLI Mode\n" || true
printf "=================================\n" || true
printf "Config:     %s\n" "${CONFIG_FILE}" || true
printf "Output:     %s\n" "${OUTPUT_FILE}" || true
printf "Started:    %s\n" "$(date)" || true
printf "\n" || true

# Run the browser in headless mode using Node.js
if ! node "${SCRIPT_DIR}/hbm_browser_cli.js" "${CONFIG_FILE}" "${OUTPUT_FILE}"; then
    printf "\n" || true
    printf "❌ Error generating metrics report\n" || true
    printf "Completed:  %s\n" "$(date)" || true
    exit 1
fi

# Success case
printf "\n" || true
printf "✅ Successfully generated metrics report: %s\n" "${OUTPUT_FILE}" || true
printf "Completed:  %s\n" "$(date)" || true