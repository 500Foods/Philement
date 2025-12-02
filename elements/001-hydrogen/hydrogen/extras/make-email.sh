#!/usr/bin/env bash

# CHANGELOG
# 1.0.0 - 2025-12-02 - Initial version of make-email.sh script

# About this Script

# Hydrogen Email Notification Script
# Sends email notification with test results and SVG charts
# Uses mutt for email delivery with embedded SVG images

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "${SCRIPT_DIR}/.." && pwd )"

# Change to hydrogen directory
cd "${HYDROGEN_DIR}" || exit 1

echo "=== Hydrogen Email Notification Script ==="
echo "📧 Preparing email with test results..."

# Check if mutt is available
if ! command -v mutt >/dev/null 2>&1; then
    echo "❌ Error: mutt is not installed. Please install mutt first."
    exit 1
fi

# Check if HYDROGEN_DEV_EMAIL is set
if [[ -z "${HYDROGEN_DEV_EMAIL:-}" ]]; then
    echo "❌ Error: HYDROGEN_DEV_EMAIL environment variable is not set."
    echo "Please set HYDROGEN_DEV_EMAIL to the recipient email address(es)."
    exit 1
fi

# Parse test results from JSON data
RESULTS_FILE="build/tests/results/results_data.json"
TABLE_FILE="build/tests/results/results_table.txt"

if [[ ! -f "${RESULTS_FILE}" ]]; then
    echo "❌ Error: Results file not found: ${RESULTS_FILE}"
    echo "Please run the test suite first (test_00_all.sh)"
    exit 1
fi

# Try to use the metrics JSON file first (more reliable data source)
METRICS_JSON="docs/metrics/$(date +%Y-%m)/$(date +%Y-%m-%d).json" || true

if [[ -f "${METRICS_JSON}" ]]; then
    # Use the summary data from the combined JSON if available
    TOTAL_TESTS=$(jq '.summary.total_tests' "${METRICS_JSON}" 2>/dev/null || echo "0")
    TOTAL_PASSED=$(jq '.summary.total_passed' "${METRICS_JSON}" 2>/dev/null || echo "0")
    TOTAL_FAILED=$(jq '.summary.total_failed' "${METRICS_JSON}" 2>/dev/null || echo "0")
    ELAPSED_TIME=$(jq -r '.summary.elapsed_formatted' "${METRICS_JSON}" 2>/dev/null || echo "00:00:00.000")
else
    # Fallback to parsing individual test results
    TOTAL_TESTS=$(jq 'map(.tests) | add' "${RESULTS_FILE}" 2>/dev/null || echo "0") || true
    TOTAL_PASSED=$(jq 'map(.pass) | add' "${RESULTS_FILE}" 2>/dev/null || echo "0")
    TOTAL_FAILED=$(jq 'map(.fail) | add' "${RESULTS_FILE}" 2>/dev/null || echo "0")

    # Use TOTAL_TESTS in the subject line for consistency
    if [[ "${TOTAL_TESTS}" -gt 0 ]]; then
        SUBJECT="Hydrogen Build ${BUILD_NUMBER} Results: ${FORMATTED_PASSED} Passed, ${FORMATTED_FAILED} Failed, ${TRIMMED_ELAPSED_TIME} elapsed (${TOTAL_TESTS} total)"
    fi

    # Extract elapsed time from table file (last line contains the elapsed time)
    ELAPSED_TIME=$(grep -oE "Elapsed[^%]*[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}" "${TABLE_FILE}" | grep -oE "[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}" || true) || true
    if [[ -z "${ELAPSED_TIME}" ]]; then
        ELAPSED_TIME="00:00:00.000"
    fi
fi

# Get version information
VERSION_FULL=$(./hydrogen --version 2>/dev/null || echo "Hydrogen ver 1.0.0.2010 rel 20251201") || true
BUILD_NUMBER=$(echo "${VERSION_FULL}" | grep -oE "ver [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+" | grep -oE "[0-9]+$" || echo "2010") || true

# Format numbers with commas for readability
format_number() {
    local num=$1
    # Use printf to add commas as thousand separators
    printf "%'d" "${num}"
}

FORMATTED_PASSED=$(format_number "${TOTAL_PASSED}")
FORMATTED_FAILED=$(format_number "${TOTAL_FAILED}")

# Trim leading hours from elapsed time (keep only minutes:seconds.milliseconds)
TRIMMED_ELAPSED_TIME=$(echo "${ELAPSED_TIME}" | grep -oE "[0-9]{2}:[0-9]{2}\.[0-9]{3}$" || echo "${ELAPSED_TIME}")

# Create email subject with version
SUBJECT="Hydrogen Build ${BUILD_NUMBER} Results: ${FORMATTED_PASSED} Passed, ${FORMATTED_FAILED} Failed, ${TRIMMED_ELAPSED_TIME} elapsed"

# Check if SVG files exist
SVG_FILES=(
    "images/COMPLETE.svg"
    "images/CLOC_CODE.svg"
    "images/CLOC_STAT.svg"
)

missing_svgs=0
for svg_file in "${SVG_FILES[@]}"; do
    if [[ ! -f "${svg_file}" ]]; then
        echo "⚠️  Warning: SVG file not found: ${svg_file}"
        ((missing_svgs++))
    fi
done

if [[ "${missing_svgs}" -gt 0 ]]; then
    echo "⚠️  Some SVG files are missing. Email will be sent without them."
fi

# Create temporary email body file
EMAIL_BODY=$(mktemp)
TEMP_DIR=$(mktemp -d)

# Copy SVG files to temp directory for embedding
for svg_file in "${SVG_FILES[@]}"; do
    if [[ -f "${svg_file}" ]]; then
        cp "${svg_file}" "${TEMP_DIR}/"
    fi
done

# Get current timestamp
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S %Z')

# Create HTML email body directly with all values substituted
cat > "${EMAIL_BODY}" << EOF
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            font-size: 14px;
        }
        h1 {
            color: #2c3e50;
            border-bottom: 2px solid #3498db;
            padding-bottom: 10px;
            font-size: 24px;
        }
        h2 {
            color: #2980b9;
            margin-top: 25px;
            font-size: 18px;
        }
        .summary {
            background-color: #f8f9fa;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            border-left: 4px solid #3498db;
        }
        .chart-container {
            margin: 20px 0;
            border: 1px solid #ddd;
            padding: 15px;
            border-radius: 5px;
            background-color: #fff;
        }
        .chart-title {
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 10px;
            text-align: center;
        }
        img {
            max-width: 100%;
            height: auto;
            display: block;
            margin: 0 auto;
        }
        .footer {
            margin-top: 30px;
            font-size: 0.9em;
            color: #7f8c8d;
            text-align: center;
            border-top: 1px solid #eee;
            padding-top: 10px;
        }
    </style>
</head>
<body>
    <h3>⚛️ Hydrogen Build Results</h3>
    <div class="footer">
        <p>Generated by Hydrogen Build System | ${TIMESTAMP}</p>
    </div>
</body>
</html>
EOF

# Prepare mutt command with embedded images
MUTT_CMD="mutt -e 'set content_type=text/html' -e 'set charset=utf-8'"

# Add SVG attachments first
if [[ -f "images/COMPLETE.svg" ]]; then
    MUTT_CMD+=" -a images/COMPLETE.svg"
fi

if [[ -f "images/CLOC_CODE.svg" ]]; then
    MUTT_CMD+=" -a images/CLOC_CODE.svg"
fi

if [[ -f "images/CLOC_STAT.svg" ]]; then
    MUTT_CMD+=" -a images/CLOC_STAT.svg"
fi

# Add subject and recipients
IFS=',' read -ra EMAIL_LIST <<< "${HYDROGEN_DEV_EMAIL}"
for email in "${EMAIL_LIST[@]}"; do
    # Trim whitespace
    email=$(echo "${email}" | xargs)
    if [[ -n "${email}" ]]; then
        MUTT_CMD+=" -s '${SUBJECT}' -- ${email}"
    fi
done

# Add the HTML body
MUTT_CMD+=" < ${EMAIL_BODY}"

echo "📧 Sending email to: ${HYDROGEN_DEV_EMAIL}"
echo "📝 Subject: ${SUBJECT}"

# Execute mutt command
eval "${MUTT_CMD}"

# Clean up
rm -f "${EMAIL_BODY}"
rm -rf "${TEMP_DIR}"

echo "✅ Email notification sent successfully!"