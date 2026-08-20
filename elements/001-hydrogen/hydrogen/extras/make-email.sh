#!/usr/bin/env bash

# CHANGELOG
# 1.0.2 - 2026-08-19 - Honour HYDROGEN_DISABLE_EMAIL=1 (skip mail tail); bounded
#                      mutt with MUTT_TIMEOUT (default 120s) so an unreachable
#                      SMTP relay can no longer strand test_00_all.sh or any
#                      strace -f profile run indefinitely.
# 1.0.1 - 2025-12-07 - Updated to use HYDROGEN_DOCS_ROOT to locate metrics
# 1.0.0 - 2025-12-02 - Initial version of make-email.sh script

# How long (seconds) mutt may block on SMTP before we give up.
MUTT_TIMEOUT="${MUTT_TIMEOUT:-120}"

# About this Script

# Hydrogen Email Notification Script
# Sends email notification with test results and SVG charts
# Uses mutt for email delivery with embedded SVG images

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HYDROGEN_DOCS_ROOT environment variable
if [[ -z "${HYDROGEN_DOCS_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_DOCS_ROOT environment variable is not set"
    echo "Please set HYDROGEN_DOCS_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

# Opt-out: skip the mail tail entirely. Useful while profiling the suite under
# strace (profile_test_suite.sh --skip-email sets this for the traced child)
# or whenever notification delivery is not wanted.
if [[ "${HYDROGEN_DISABLE_EMAIL:-0}" -eq 1 ]]; then
    echo "📧 Email notification skipped (HYDROGEN_DISABLE_EMAIL=1)."
    exit 0
fi
# Get the directory where this script is located
HYDROGEN_DIR="${HYDROGEN_ROOT}"

# Change to hydrogen directory
cd "${HYDROGEN_DIR}" || exit 1

echo "=== Hydrogen Email Notification Script ==="
echo "📧 Preparing email with test results..."

# Prefer SVGs written by this suite run. test_00_all.sh now waits for Oh before
# invoking us; keep a short settle delay for filesystem mtime visibility.
sleep 2

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

# Try to use the metrics JSON file first (more reliable data source)
METRICS_JSON="${HYDROGEN_DOCS_ROOT}/metrics/$(date +%Y-%m)/$(date +%Y-%m-%d).json" || true
# TOTAL_TESTS=$(jq '.summary.total_tests' "${METRICS_JSON}" 2>/dev/null || echo "0")
TOTAL_PASSED=$(jq '.summary.total_passed' "${METRICS_JSON}" 2>/dev/null || echo "0")
TOTAL_FAILED=$(jq '.summary.total_failed' "${METRICS_JSON}" 2>/dev/null || echo "0")
ELAPSED_TIME=$(jq -r '.summary.elapsed_formatted' "${METRICS_JSON}" 2>/dev/null || echo "00:00:00.000")

# Get version information
VERSION_FULL=$(./hydrogen --version 2>/dev/null || echo "Hydrogen ver 1.0.0.2010 rel 20251201") || true
BUILD_NUMBER=$(echo "${VERSION_FULL}" | grep -oE "ver [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+" | grep -oE "[0-9]+$" || echo "2010") || true

# Format numbers with commas for readability
format_number() {
    local num=$1
    # Use sed to add commas as thousand separators
    echo "${num}" | sed ':a;s/\B[0-9]\{3\}\>/,&/;ta'
}

FORMATTED_PASSED=$(format_number "${TOTAL_PASSED}")
FORMATTED_FAILED=$(format_number "${TOTAL_FAILED}")

# Trim leading hours from elapsed time (keep only minutes:seconds.milliseconds)
TRIMMED_ELAPSED_TIME=$(echo "${ELAPSED_TIME}" | grep -oE "[0-9]{2}:[0-9]{2}\.[0-9]{3}$" || echo "${ELAPSED_TIME}")

# Create email subject with version
SUBJECT="Hydrogen Build ${BUILD_NUMBER}: ${FORMATTED_PASSED} Passed — ${FORMATTED_FAILED} Failed — ${TRIMMED_ELAPSED_TIME} elapsed"

# Check if SVG files exist
SVG_FILES=(
    "images/COMPLETE.svg"
    "images/CLOC_CODE.svg"
    "images/CLOC_STAT.svg"
    "images/HISTORY.svg"
    "images/COVERAGE.svg"
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
            font-family: 'Vanadium Sans Semi-Extended', sans-serif;
            line-height: 1.6;
            color: #fff;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            font-size: 14px;
            background-color: #333;
        }
        h3 {
            color: #ccc;
            border-bottom: 2px solid #3498db;
            padding-bottom: 20px;
            font-size: 20px;
        }
        .summary {
            color: #aaa;
            padding: 15px;
            border-radius: 5px;
            margin: 16px 0;
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
            font-family: 'Vanadium Mono Semi-Extended', monospace;
            max-width: 100%;
            height: auto;
            display: block;
            margin: 0 auto;
        }
        .footer {
            margin-top: 30px;
            font-size: 16px;
            color: #7f8c8d;
            padding-top: 0px;
        }
    </style>
</head>
<body>
    <h3>⚛️ Hydrogen Build ${BUILD_NUMBER} Results</h3>
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

if [[ -f "images/HISTORY.svg" ]]; then
    MUTT_CMD+=" -a images/HISTORY.svg"
fi

if [[ -f "images/COVERAGE.svg" ]]; then
    MUTT_CMD+=" -a images/COVERAGE.svg"
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

# Execute mutt command. Bounded by MUTT_TIMEOUT so an unreachable/misconfigured
# SMTP relay cannot hang the suite (or a strace -f profile) indefinitely.
mutt_rc=0
timeout "${MUTT_TIMEOUT}" bash -c "${MUTT_CMD}" || mutt_rc=$?
if [[ "${mutt_rc}" -ne 0 ]]; then
    if [[ "${mutt_rc}" -eq 124 ]]; then
        echo "⚠️  Email delivery timed out after ${MUTT_TIMEOUT}s; skipping." >&2
    else
        echo "⚠️  Email delivery failed (mutt exit ${mutt_rc}); skipping." >&2
    fi
fi

# Clean up
rm -f "${EMAIL_BODY}"
rm -rf "${TEMP_DIR}"

echo "✅ Email notification sent successfully!"