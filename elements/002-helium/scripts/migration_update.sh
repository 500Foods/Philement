#!/bin/bash

# migration_update.sh
# Updates migration indexes for all Helium schemas and performs documentation quality checks
# Uses HELIUM_ROOT and HELIUM_DOCS_ROOT environment variables for path resolution

# CHANGEHISTORY
# 2.0.0 - 2025-12-15 - Added documentation quality checks (link validation and markdown linting)
# 1.0.0 - 2025-11-24 - Initial version

set -euo pipefail

# Use HELIUM_ROOT if set, otherwise assume we're in scripts/ subdirectory
if [[ -n "${HELIUM_ROOT:-}" ]]; then
    BASE_DIR="${HELIUM_ROOT}"
else
    # Assume script is in scripts/ subdirectory of helium root
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BASE_DIR="$(dirname "${SCRIPT_DIR}")"
fi

# Use HELIUM_DOCS_ROOT if set, otherwise assume docs/He relative to BASE_DIR
if [[ -n "${HELIUM_DOCS_ROOT:-}" ]]; then
    DOCS_DIR="${HELIUM_DOCS_ROOT}"
else
    DOCS_DIR="${BASE_DIR}/../docs/He"
fi

echo "=== Updating Migration Indexes ==="
"${BASE_DIR}/scripts/migration_index.sh" "${BASE_DIR}/acuranzo/README.md" "${BASE_DIR}/acuranzo/migrations"
"${BASE_DIR}/scripts/migration_index.sh" "${BASE_DIR}/gaius/README.md" "${BASE_DIR}/gaius/migrations"
"${BASE_DIR}/scripts/migration_index.sh" "${BASE_DIR}/glm/README.md" "${BASE_DIR}/glm/migrations"
"${BASE_DIR}/scripts/migration_index.sh" "${BASE_DIR}/helium/README.md" "${BASE_DIR}/helium/migrations"

echo ""
echo "=== Checking Documentation Links ==="
# Run github-sitemap.sh to check links (similar to Hydrogen test 04)
# Set paths similar to Hydrogen test 4
PHILEMENT_ROOT="$(realpath "${BASE_DIR}/../..")"
HELIUM_ROOT="${BASE_DIR}"
HELIUM_DOCS_ROOT="${DOCS_DIR}"
export PHILEMENT_ROOT HELIUM_ROOT HELIUM_DOCS_ROOT

"${BASE_DIR}/scripts/github-sitemap.sh" "${DOCS_DIR}/README.md" --webroot "${PHILEMENT_ROOT}" --include "${HELIUM_ROOT}" --include "${HELIUM_DOCS_ROOT}" --noreport --quiet

echo ""
echo "=== Checking Markdown Quality ==="
# Run markdownlint on documentation files (similar to Hydrogen test 90)
if command -v markdownlint >/dev/null 2>&1; then
    # Find markdown files in docs/He and elements/002-helium
    mapfile -t md_files < <(find "${DOCS_DIR}" "${BASE_DIR}" -name "*.md" -type f 2>/dev/null || true)
    if [[ ${#md_files[@]} -gt 0 ]]; then
        echo "Running markdownlint on ${#md_files[@]} files..."
        # Use same config as Hydrogen for consistency, suppress Node.js warnings
        NODE_OPTIONS="--no-deprecation" markdownlint --config "${BASE_DIR}/.lintignore-markdown" "${md_files[@]}" 2>&1 || echo "Markdownlint found issues (see above)"
    else
        echo "No markdown files found to check"
    fi
else
    echo "markdownlint not found, skipping markdown quality check"
fi

echo ""
echo "=== Checking Lua Code Quality ==="
# Run luacheck on migration files (similar to Hydrogen test 98)
if command -v luacheck >/dev/null 2>&1; then
    for schema in acuranzo gaius glm helium; do
        migrations_dir="${BASE_DIR}/${schema}/migrations"
        if [[ -d "${migrations_dir}" ]]; then
            echo "Checking ${schema} migrations..."
            luacheck "${migrations_dir}" | tail -n 1
        fi
    done
else
    echo "luacheck not found, skipping Lua code quality check"
fi

echo ""
echo "=== Updating Repository Information ==="
# Update cloc statistics in README.md (similar to main repo)
if command -v cloc >/dev/null 2>&1; then
    echo "Running cloc on Helium project files..."
    # Generate cloc output for HELIUM_ROOT and HELIUM_DOCS_ROOT
    CLOC_TEMP=$(mktemp)
    cloc --quiet "${BASE_DIR}" "${DOCS_DIR}" > "${CLOC_TEMP}" 2>&1 || true

    if [[ -s "${CLOC_TEMP}" ]]; then
        # Generate timestamp in desired format
        TIMESTAMP=$(date +"%Y-%b-%d (%a) %H:%M:%S %Z")

        # Strip blank lines from cloc output but keep the version line
        CLOC_CLEAN=$(sed '/^$/d' "${CLOC_TEMP}")

        # Create a temp file with the replacement content
        CLOC_CONTENT_TEMP=$(mktemp)
        {
            echo "Generated ${TIMESTAMP}"
            echo ""
            echo "\`\`\`cloc"
            echo "${CLOC_CLEAN}"
            echo "\`\`\`"
        } > "${CLOC_CONTENT_TEMP}"

        # Replace content after "## Repository Information" heading (similar to migration_index.sh)
        TEMP_README=$(mktemp)
        if grep -q "^## Repository Information" "${DOCS_DIR}/README.md"; then
            sed '/^## Repository Information/,$d' "${DOCS_DIR}/README.md" > "${TEMP_README}"
        else
            cat "${DOCS_DIR}/README.md" > "${TEMP_README}"
        fi

        # Add the Repository Information section
        {
            echo "## Repository Information"
            echo ""
            cat "${CLOC_CONTENT_TEMP}"
        } >> "${TEMP_README}"

        # Replace the original file
        mv "${TEMP_README}" "${DOCS_DIR}/README.md"

        rm -f "${CLOC_CONTENT_TEMP}"
        echo "Repository information updated in ${DOCS_DIR}/README.md"
    else
        echo "cloc failed or produced no output"
    fi

    rm -f "${CLOC_TEMP}"
else
    echo "cloc not found, skipping repository information update"
fi

echo ""
echo "=== Documentation Update Complete ==="
