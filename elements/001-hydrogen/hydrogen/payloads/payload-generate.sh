#!/usr/bin/env bash

# payload-generate.sh
# Encrypted Payload Generator for Hydrogen

# This script creates an encrypted payload package for the Hydrogen project.
# Currently, it packages SwaggerUI content, but is designed to be extendable
# for other payload types in the future. The script:
# - Downloads SwaggerUI from GitHub
# - Downloads and xterm.js for terminal subsystem
# - Extracts essential files and compresses static assets with Brotli
# - Creates an optimized tar file compressed with Brotli
# - Encrypts the package using RSA+AES hybrid encryption
# - Cleans up all temporary files

# CHANGELONG
# 4.3.0 - 2026-01-10 - Changed SwaggerUI download to use git clone instead of GitHub API to avoid rate limits
# 4.2.0 - 2025-12-13 - Added folder breakdown section showing file counts, sizes, and percentages for payload components
# 4.1.0 - 2025-12-08 - Added Hydrogen Build Metrics Browser files to swagger folder in payload
# 4.0.0 - 2024-12-05 - Added env var dependency checks
# 3.0.0 - 2025-09-30 - Added Helium migration files to payload
# 2.2.0 - Added terminal payload generation with xterm.js
# 2.1.0 - Added swagger/ directory structure within tar file for better organization
# 2.0.0 - Better at downloading latest version of swagger
# 1.2.0 - Improved modularity, fixed shellcheck warnings, enhanced error handling
# 1.1.0 - Added RSA+AES hybrid encryption support
# 1.0.0 - Initial release with basic payload generation

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

set -e

# Display script information
echo "payload-generate.sh version 4.3.0"
echo "Encrypted Payload Generator for Hydrogen"

# Helium project Migrations to include (same as test_31_migrations.sh)
DESIGNS=("helium" "acuranzo")
HELIUM_DIR="${HELIUM_ROOT}"

# Path configuration
SCRIPT_DIR="${HYDROGEN_ROOT}/payloads"
readonly SCRIPT_DIR
readonly SWAGGERUI_DIR="${SCRIPT_DIR}/swaggerui"
readonly XTERMJS_DIR="${SCRIPT_DIR}/xtermjs"
readonly TAR_FILE="${SCRIPT_DIR}/payload.tar"
readonly COMPRESSED_TAR_FILE="${SCRIPT_DIR}/payload.tar.br.enc"

# Common utilities - use GNU versions if available (eg: homebrew on macOS)
FIND=$(command -v gfind 2>/dev/null || command -v find)
# GREP=$(command -v ggrep 2>/dev/null || command -v grep)
SED=$(command -v gsed 2>/dev/null || command -v sed)
TAR=$(command -v gtar 2>/dev/null || command -v tar)
STAT=$(command -v gstat 2>/dev/null || command -v stat)

# Terminal formatting codes
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[0;33m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly MAGENTA='\033[0;35m'
readonly BOLD='\033[1m'
readonly NC='\033[0m'  # No Color

# Status symbols
readonly PASS="✅"
readonly FAIL="❌"
readonly WARN="⚠️"
readonly INFO="🛈 "

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    local relative_path

    # Extract the part starting from "hydrogen" and keep everything after
    relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/hydrogen/|hydrogen/|p')

    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [[ -z "${relative_path}" ]]; then
        relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi

    # If we still couldn't find a match, return the original
    if [[ -z "${relative_path}" ]]; then
        echo "${absolute_path}"
    else
        echo "${relative_path}"
    fi
}

# Create temporary directory
TEMP_DIR=$(mktemp -d)
readonly TEMP_DIR

# Function to check for required dependencies
check_dependencies() {
    local missing_deps=0

    # Required for basic operation
    if ! command -v git >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: git is required but not installed. Please install git.${NC}"
        missing_deps=1
    fi

    if ! command -v curl >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: curl is required but not installed. Please install curl.${NC}"
        missing_deps=1
    fi

    if ! command -v "${TAR}" >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: tar is required but not installed. Please install tar.${NC}"
        missing_deps=1
    fi

    if ! command -v brotli >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: brotli is required but not installed. Please install brotli.${NC}"
        missing_deps=1
    fi

    # Required for encryption
    if ! command -v openssl >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: openssl is required but not installed. Please install openssl.${NC}"
        missing_deps=1
    fi

    if [[ "${missing_deps}" -ne 0 ]]; then
        exit 1
    fi
}

# Function to print formatted header
print_header() {
    local title="$1"
    echo -e "
${BLUE}────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD} ${title} ${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Function to display script information
display_script_info() {
    print_header "Encrypted Payload Generator for Hydrogen"
    PATH1=$(set -e; convert_to_relative_path "${SWAGGERUI_DIR}")
    PATH2=$(set -e; convert_to_relative_path "${COMPRESSED_TAR_FILE}")
    echo -e "${CYAN}${INFO} SwaggerUI Version:       ${NC}Latest (determined at runtime)"
    echo -e "${CYAN}${INFO} xterm.js Version:        ${NC}Latest (determined at runtime)"
    echo -e "${CYAN}${INFO} SwaggerUI Directory:     ${NC}${PATH1}"
    echo -e "${CYAN}${INFO} Final encrypted file:    ${NC}${PATH2}"
    echo -e "${CYAN}${INFO} Temporary directory:     ${NC}${TEMP_DIR}"
}

# Function to clean up temporary files and directories
cleanup() {
    print_header "Cleanup Process"
    echo -e "${CYAN}${INFO} Cleaning up temporary files and directories...${NC}"

    # Remove the temporary working directory
    rm -rf "${TEMP_DIR}"

    # Remove the temporary swaggerui directory if it exists
    if [[ -d "${SWAGGERUI_DIR}" ]]; then
        echo -e "${CYAN}${INFO} Removing temporary SwaggerUI directory...${NC}"
        rm -rf "${SWAGGERUI_DIR}"
    fi

    # Remove the temporary xtermjs directory if it exists
    if [[ -d "${XTERMJS_DIR}" ]]; then
        echo -e "${CYAN}${INFO} Removing temporary xterm.js directory...${NC}"
        rm -rf "${XTERMJS_DIR}"
    fi

    # Remove intermediate tar file if it exists
    if [[ -f "${TAR_FILE}" ]]; then
        echo -e "${CYAN}${INFO} Removing intermediate tar file...${NC}"
        rm -f "${TAR_FILE}"
    fi

    # Remove any generated .br files in the script directory (except the final encrypted payload)
    "${FIND}" "${SCRIPT_DIR}" -name "*.br" -not -name "payload.tar.br.enc" -delete 2>/dev/null || true

    echo -e "${GREEN}${PASS} Cleanup completed successfully.${NC}"
}

# Function to create SwaggerUI swagger.html
create_swagger_html() {
    local target_file="$1"
    cat > "${target_file}" << 'EOF'
<!-- HTML for static distribution bundle build -->
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Hydrogen API Documentation</title>
    <link rel="stylesheet" type="text/css" href="swagger-ui.css" />
    <link rel="icon" type="image/png" href="favicon-32x32.png" sizes="32x32" />
    <link rel="icon" type="image/png" href="favicon-16x16.png" sizes="16x16" />
    <style>
      html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
      *, *:before, *:after { box-sizing: inherit; }
      body { margin: 0; background: #fafafa; }
    </style>
  </head>

  <body>
    <div id="swagger-ui"></div>
    <script src="swagger-ui-bundle.js"></script>
    <script src="swagger-ui-standalone-preset.js"></script>
    <script src="swagger-initializer.js"></script>
  </body>
</html>
EOF
}

# Function to create SwaggerUI initializer
create_swagger_initializer() {
    local target_file="$1"
    cat > "${target_file}" << EOF
window.onload = function() {
  window.ui = SwaggerUIBundle({
    url: "swagger.json",
    dom_id: '#swagger-ui',
    deepLinking: true,
    presets: [
      SwaggerUIBundle.presets.apis,
      SwaggerUIStandalonePreset
    ],
    plugins: [
      SwaggerUIBundle.plugins.DownloadUrl
    ],
    layout: "StandaloneLayout",
    tryItOutEnabled: true,
    displayOperationId: true,
    defaultModelsExpandDepth: 1,
    defaultModelExpandDepth: 1,
    docExpansion: "list"
  });
};
EOF
}

# Function to get the latest SwaggerUI version from GitHub using git ls-remote
get_latest_swaggerui_version() {
    echo -e "${CYAN}${INFO} Fetching latest SwaggerUI version from GitHub...${NC}" >&2

    local latest_version
    # Use git ls-remote to get the latest tag (no API rate limiting)
    latest_version=$(git ls-remote --tags --refs --sort="v:refname" https://github.com/swagger-api/swagger-ui.git |
                     tail -n1 |
                     "${SED}" 's/.*refs\/tags\/v/v/' || true)

    if [[ -z "${latest_version}" ]]; then
        echo -e "${YELLOW}${WARN} Failed to fetch latest version, falling back to v5.27.1${NC}" >&2
        echo "5.27.1"
    else
        # Remove 'v' prefix if present
        latest_version=${latest_version#v}
        echo -e "${CYAN}${INFO} Latest SwaggerUI version: v${latest_version}${NC}" >&2
        echo "${latest_version}"
    fi
}

# Function to generate terminal payload
generate_terminal_payload() {
    print_header "Generating Terminal Payload"

    # Run the terminal generation script (uncommented to ensure fresh generation)
    echo -e "${CYAN}${INFO} Running terminal-generate.sh...${NC}"
    if ! "${SCRIPT_DIR}/terminal-generate.sh" >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Terminal payload generation failed${NC}"
        exit 1
    fi

    echo -e "${GREEN}${PASS} Terminal payload generated successfully.${NC}"
}

# Function to copy migration files into payload
copy_migration_files() {
    print_header "Copying Migration Files"

    echo -e "${CYAN}${INFO} Copying database migration files...${NC}"

    # Process each design
    for design in "${DESIGNS[@]}"; do
        echo -e "${CYAN}${INFO} Processing design: ${design}${NC}"

        # Create design directory in swaggerui
        local design_dir="${SWAGGERUI_DIR}/${design}"
        mkdir -p "${design_dir}"

        # Copy database.lua and database_<engine>.lua files for this design
        local source_db="${HELIUM_DIR}/${design}/migrations/database.lua"
        if [[ -f "${source_db}" ]]; then
            cp "${source_db}" "${design_dir}/"
            echo -e "${GREEN}${PASS} Copied ${design}/migrations/database.lua${NC}"
        else
            echo -e "${YELLOW}${WARN} Warning: ${source_db} not found${NC}"
        fi

        # Copy database_<engine>.lua files for this design
        local engine_files=()
        while IFS= read -r -d '' engine_file; do
            engine_files+=("${engine_file}")
        done < <("${FIND}" "${HELIUM_DIR}/${design}/migrations" -name "database_*.lua" -type f -print0 2>/dev/null | sort -z || true)

        for engine_file in "${engine_files[@]}"; do
            if [[ -f "${engine_file}" ]]; then
                local filename
                filename=$(basename "${engine_file}")
                cp "${engine_file}" "${design_dir}/"
                echo -e "${GREEN}${PASS} Copied ${design}/migrations/${filename}${NC}"
            fi
        done

        # Copy all migration files for this design
        local migration_count=0
        local migration_files=()
        while IFS= read -r -d '' file; do
            migration_files+=("${file}")
        done < <("${FIND}" "${HELIUM_DIR}/${design}/migrations" -name "${design}_????.lua" -type f -print0 2>/dev/null | sort -z || true)

        for migration_file in "${migration_files[@]}"; do
            if [[ -f "${migration_file}" ]]; then
                local filename
                filename=$(basename "${migration_file}")
                cp "${migration_file}" "${design_dir}/"
                echo -e "${GREEN}${PASS} Copied ${design}/migrations/${filename}${NC}"
                migration_count=$(( migration_count + 1 ))
            fi
        done

        echo -e "${CYAN}${INFO} Copied ${migration_count} migration files for ${design}${NC}"
    done

    echo -e "${GREEN}${PASS} Migration files copied successfully.${NC}"
}

# Function to download and extract SwaggerUI
download_swaggerui() {
    print_header "Downloading and Extracting SwaggerUI"

    # Get the latest version dynamically
    local swaggerui_version
    swaggerui_version=$(set -e; get_latest_swaggerui_version)
    readonly SWAGGERUI_VERSION="${swaggerui_version}"

    echo -e "${CYAN}${INFO} Downloading SwaggerUI v${SWAGGERUI_VERSION}...${NC}"

    # Clone the specific version tag from GitHub (no API rate limiting)
    git clone --depth 1 --branch "v${SWAGGERUI_VERSION}" https://github.com/swagger-api/swagger-ui.git "${TEMP_DIR}/swagger-ui"

    echo -e "${CYAN}${INFO} SwaggerUI cloned successfully...${NC}"
    
    # Set extracted directory to the cloned directory
    EXTRACTED_DIR="${TEMP_DIR}/swagger-ui"
    
    echo -e "${CYAN}${INFO} Using directory: swagger-ui${NC}"

    # Create the temporary swaggerui directory for processing
    echo -e "${CYAN}${INFO} Creating temporary SwaggerUI directory...${NC}"
    # Remove existing swaggerui directory if it exists
    if [[ -d "${SWAGGERUI_DIR}" ]]; then
        echo -e "${YELLOW}${WARN} Removing existing SwaggerUI directory...${NC}"
        rm -rf "${SWAGGERUI_DIR}"
    fi
    mkdir -p "${SWAGGERUI_DIR}"

    # List of required files from the distribution
    # We'll selectively copy only what we need
    echo -e "${CYAN}${INFO} Selecting essential files for distribution...${NC}"

    # Process static files (to be compressed with brotli)
    cp "${EXTRACTED_DIR}/dist/swagger-ui-bundle.js" "${SWAGGERUI_DIR}/"
    cp "${EXTRACTED_DIR}/dist/swagger-ui-standalone-preset.js" "${SWAGGERUI_DIR}/"
    cp "${EXTRACTED_DIR}/dist/oauth2-redirect.html" "${SWAGGERUI_DIR}/"

    # Replace swagger-ui.css with custom dark theme instead of using the default
    echo -e "${CYAN}${INFO} Applying custom dark theme...${NC}"
    if [[ -f "${SCRIPT_DIR}/swagger-ui-custom.css" ]]; then
        cp "${SCRIPT_DIR}/swagger-ui-custom.css" "${SWAGGERUI_DIR}/swagger-ui.css"
        echo -e "${GREEN}${PASS} Custom dark theme applied successfully${NC}"
    else
        # Fall back to default theme if custom CSS not found
        cp "${EXTRACTED_DIR}/dist/swagger-ui.css" "${SWAGGERUI_DIR}/"
        echo -e "${YELLOW}${WARN} Warning: swagger-ui-custom.css not found, using default theme${NC}"
    fi

    # Copy swagger.json (both compressed and uncompressed)
    if [[ -f "${SCRIPT_DIR}/swagger.json" ]]; then
        echo -e "${GREEN}${PASS} Found swagger.json, copying for packaging...${NC}"
        cp "${SCRIPT_DIR}/swagger.json" "${SWAGGERUI_DIR}/"
    else
        echo -e "${YELLOW}${WARN} Warning: swagger.json not found. Run swagger-generate.sh first.${NC}"
        # Create a minimal placeholder swagger.json
        echo '{"openapi":"3.1.0","info":{"title":"Hydrogen API","version":"1.0.0"},"paths":{}}' > "${SWAGGERUI_DIR}/swagger.json"
    fi

    # Process dynamic files (to remain uncompressed)
    cp "${EXTRACTED_DIR}/dist/favicon-32x32.png" "${SWAGGERUI_DIR}/"
    cp "${EXTRACTED_DIR}/dist/favicon-16x16.png" "${SWAGGERUI_DIR}/"

    # Customize swagger.html and swagger-initializer.js
    echo -e "${CYAN}${INFO} Customizing swagger.html...${NC}"
    create_swagger_html "${SWAGGERUI_DIR}/swagger.html"

    echo -e "${CYAN}${INFO} Customizing swagger-initializer.js with recommended settings...${NC}"
    create_swagger_initializer "${SWAGGERUI_DIR}/swagger-initializer.js"

    echo -e "${GREEN}${PASS} SwaggerUI files prepared for packaging.${NC}"
}

# Function to apply Brotli compression to static assets
compress_static_assets() {
    print_header "Compressing Static Assets"
    echo -e "${CYAN}${INFO} Applying Brotli compression to static assets...${NC}"

    # Compress swagger-ui-bundle.js (won't change at runtime)
    echo -e "${CYAN}${INFO} Compressing swagger-ui-bundle.js with Brotli...${NC}"
    brotli --quality=11 --lgwin=24 -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" -o "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br"

    # Compress swagger-ui-standalone-preset.js
    echo -e "${CYAN}${INFO} Compressing swagger-ui-standalone-preset.js with Brotli...${NC}"
    brotli --quality=11 --lgwin=24 -f "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js" -o "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js.br"

    # Compress swagger-ui.css (static styling)
    echo -e "${CYAN}${INFO} Compressing swagger-ui.css with Brotli...${NC}"
    brotli --quality=11 --lgwin=24 -f "${SWAGGERUI_DIR}/swagger-ui.css" -o "${SWAGGERUI_DIR}/swagger-ui.css.br"

    # Compress oauth2-redirect.html
    echo -e "${CYAN}${INFO} Compressing oauth2-redirect.html with Brotli...${NC}"
    brotli --quality=11 --lgwin=24 -f "${SWAGGERUI_DIR}/oauth2-redirect.html" -o "${SWAGGERUI_DIR}/oauth2-redirect.html.br"

    # Compress terminal files if xtermjs directory exists
    if [[ -d "${XTERMJS_DIR}" ]]; then
        echo -e "${CYAN}${INFO} Compressing terminal static assets with Brotli...${NC}"

        if [[ -f "${XTERMJS_DIR}/xterm.js" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/xterm.js" -o "${XTERMJS_DIR}/xterm.js.br"
        fi

        if [[ -f "${XTERMJS_DIR}/xterm.css" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/xterm.css" -o "${XTERMJS_DIR}/xterm.css.br"
        fi

        if [[ -f "${XTERMJS_DIR}/xterm-addon-attach.js" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/xterm-addon-attach.js" -o "${XTERMJS_DIR}/xterm-addon-attach.js.br"
        fi

        if [[ -f "${XTERMJS_DIR}/xterm-addon-fit.js" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/xterm-addon-fit.js" -o "${XTERMJS_DIR}/xterm-addon-fit.js.br"
        fi

        if [[ -f "${XTERMJS_DIR}/terminal.css" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/terminal.css" -o "${XTERMJS_DIR}/terminal.css.br"
        fi

        if [[ -f "${XTERMJS_DIR}/terminal.html" ]]; then
            brotli --quality=11 --lgwin=24 -f "${XTERMJS_DIR}/terminal.html" -o "${XTERMJS_DIR}/terminal.html.br"
        fi

        # Remove uncompressed terminal files (keep HTML, CSS, and JS uncompressed as they contain dynamic content)
        rm -f "${XTERMJS_DIR}/xterm.js" "${XTERMJS_DIR}/xterm.css" "${XTERMJS_DIR}/xterm-addon-attach.js" "${XTERMJS_DIR}/xterm-addon-fit.js"
    fi

    # Remove the uncompressed static files - we'll only include the .br versions in the tar
    rm -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" \
          "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js" \
          "${SWAGGERUI_DIR}/swagger-ui.css" \
          "${SWAGGERUI_DIR}/oauth2-redirect.html"

    echo -e "${GREEN}${PASS} Static assets compressed successfully.${NC}"
}

# Function to validate brotli compression
validate_brotli_compression() {
    local compressed_file="$1"
    local original_file="$2"

    echo -e "${CYAN}${INFO} Testing Brotli decompression...${NC}"
    if ! brotli -d "${compressed_file}" -o "${TEMP_DIR}/test.tar" 2>/dev/null; then
        echo -e "${RED}${FAIL} Brotli validation failed - invalid compressed data${NC}"
        exit 1
    fi

    # Compare original and decompressed files
    if ! cmp -s "${original_file}" "${TEMP_DIR}/test.tar"; then
        echo -e "${RED}${FAIL} Brotli validation failed - decompressed data mismatch${NC}"
        exit 1
    fi

    echo -e "${GREEN}${PASS} Brotli compression validated successfully${NC}"
    rm -f "${TEMP_DIR}/test.tar"
}

# Function to create validation files
create_validation_files() {
    local br_size="$1"
    local br_head_16="$2"
    local br_tail_16="$3"

    # Create validation file for debugging
    {
        echo "BROTLI_VALIDATION"
        echo "Size: ${br_size}"
        echo "First16: ${br_head_16}"
        echo "Last16: ${br_tail_16}"
    } > "${TEMP_DIR}/validation.txt"
}

# Function to create and encrypt payload
create_tarball() {
    print_header "Creating Payload Package"
    echo -e "${CYAN}${INFO} Creating payload tarball...${NC}"

    # Create swagger directory structure within the working directory
    echo -e "${CYAN}${INFO} Organizing files into directory structures...${NC}"

    # Create terminal directory structure if xtermjs directory exists
    if [[ -d "${XTERMJS_DIR}" ]]; then
        mkdir -p "${SWAGGERUI_DIR}/terminal"
        echo -e "${CYAN}${INFO} Creating terminal/ directory structure...${NC}"

        if [[ -f "${XTERMJS_DIR}/terminal.html.br" ]]; then
            mv "${XTERMJS_DIR}/terminal.html.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/terminal.css.br" ]]; then
            mv "${XTERMJS_DIR}/terminal.css.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/xterm.js.br" ]]; then
            mv "${XTERMJS_DIR}/xterm.js.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/xterm.css.br" ]]; then
            mv "${XTERMJS_DIR}/xterm.css.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/xterm-addon-attach.js.br" ]]; then
            mv "${XTERMJS_DIR}/xterm-addon-attach.js.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/xterm-addon-fit.js.br" ]]; then
            mv "${XTERMJS_DIR}/xterm-addon-fit.js.br" "${SWAGGERUI_DIR}/terminal/"
        fi
        if [[ -f "${XTERMJS_DIR}/xtermjs_version.txt" ]]; then
            cp "${XTERMJS_DIR}/xtermjs_version.txt" "${SWAGGERUI_DIR}/terminal/"
        fi
    fi

    # Create swagger directory structure within the working directory
    mkdir -p "${SWAGGERUI_DIR}/swagger"

    # Move all files into the swagger directory to create proper folder structure
    mv "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js.br" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/swagger-ui.css.br" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/swagger.json" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/oauth2-redirect.html.br" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/swagger.html" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/swagger-initializer.js" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/favicon-32x32.png" "${SWAGGERUI_DIR}/swagger/"
    mv "${SWAGGERUI_DIR}/favicon-16x16.png" "${SWAGGERUI_DIR}/swagger/"

    # Copy and compress HBM browser files to swagger directory
    echo -e "${CYAN}${INFO} Copying and compressing HBM browser files to swagger directory...${NC}"
    for hbm_file in "${HYDROGEN_ROOT}/extras/hbm_browser/hbm_browser"*; do
        if [[ -f "${hbm_file}" ]]; then
            filename=$(basename "${hbm_file}")
            brotli --quality=11 --lgwin=24 -f "${hbm_file}" -o "${SWAGGERUI_DIR}/swagger/${filename}.br"
        fi
    done

    # Create tar file with swagger directory structure
    # - Compressed static assets (.br files)
    # - Uncompressed dynamic files and swagger.json
    # - Strip metadata (permissions and ownership) since we're the only ones using it

    # Build the tar command with appropriate files
    TAR_FILES=(
        "swagger/swagger-ui-bundle.js.br"
        "swagger/swagger-ui-standalone-preset.js.br"
        "swagger/swagger-ui.css.br"
        "swagger/swagger.json"
        "swagger/oauth2-redirect.html.br"
        "swagger/swagger.html"
        "swagger/swagger-initializer.js"
        "swagger/favicon-32x32.png"
        "swagger/favicon-16x16.png"
    )

    # Add HBM browser files
    while IFS= read -r -d '' hbm_file; do
        local rel_path="swagger/${hbm_file##*/}"
        TAR_FILES+=("${rel_path}")
    done < <("${FIND}" "${SWAGGERUI_DIR}/swagger" -name "hbm_browser*.br" -type f -print0 2>/dev/null || true)

    # Add terminal files if they exist
    if [[ -d "${SWAGGERUI_DIR}/terminal" ]]; then
        TAR_FILES+=(
            "terminal/terminal.html.br"
            "terminal/terminal.css.br"
            "terminal/xterm.js.br"
            "terminal/xterm.css.br"
            "terminal/xterm-addon-attach.js.br"
            "terminal/xterm-addon-fit.js.br"
            "terminal/xtermjs_version.txt"
        )
    fi

    # Add migration files for each design
    for design in "${DESIGNS[@]}"; do
        if [[ -d "${SWAGGERUI_DIR}/${design}" ]]; then
            # Add database.lua
            if [[ -f "${SWAGGERUI_DIR}/${design}/database.lua" ]]; then
                TAR_FILES+=("${design}/database.lua")
            fi

            # Add database_<engine>.lua files
            while IFS= read -r -d '' engine_file; do
                local rel_path="${engine_file#"${SWAGGERUI_DIR}"/}"
                TAR_FILES+=("${rel_path}")
            done < <("${FIND}" "${SWAGGERUI_DIR}/${design}" -name "database_*.lua" -type f -print0 2>/dev/null || true)

            # Add all migration files
            while IFS= read -r -d '' migration_file; do
                local rel_path="${migration_file#"${SWAGGERUI_DIR}"/}"
                TAR_FILES+=("${rel_path}")
            done < <("${FIND}" "${SWAGGERUI_DIR}/${design}" -name "${design}_????.lua" -type f -print0 2>/dev/null || true)
        fi
    done

    cd "${SWAGGERUI_DIR}" && "${TAR}" --mode=0000 --owner=0 --group=0 -cf "${TAR_FILE}" "${TAR_FILES[@]}"

    # Calculate tar size for breakdown percentages
    local tar_size
    tar_size=$("${STAT}" -c%s "${TAR_FILE}")

    # Compress the tar file with Brotli using explicit settings
    echo -e "${CYAN}${INFO} Compressing tar file with Brotli...${NC}"
    echo -e "${CYAN}${INFO} - Quality: 11 (maximum)${NC}"
    echo -e "${CYAN}${INFO} - Window: 24 (16MB)${NC}"
    echo -e "${CYAN}${INFO} - Input size: $("${STAT}" -c%s "${TAR_FILE}" || true) bytes${NC}"

    # Use explicit Brotli parameters with correct syntax
    brotli --quality=11 --lgwin=24 --force \
           "${TAR_FILE}" \
           -o "${TEMP_DIR}/payload.tar.br"

    # Check if brotli compression succeeded
    if ! brotli --quality=11 --lgwin=24 --force "${TAR_FILE}" -o "${TEMP_DIR}/payload.tar.br"; then
        echo -e "${RED}${FAIL} Brotli compression failed${NC}"
        exit 1
    fi

    # Verify the compressed file
    if [[ ! -f "${TEMP_DIR}/payload.tar.br" ]]; then
        echo -e "${RED}${FAIL} Compressed file not created${NC}"
        exit 1
    fi

    # Log detailed Brotli stream information
    local br_size
    local br_head_16
    local br_tail_16
    br_size=$("${STAT}" -c%s "${TEMP_DIR}/payload.tar.br" || true)
    br_head_16=$(head -c16 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n' || true)
    br_tail_16=$(tail -c16 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n' || true)

    echo -e "${CYAN}${INFO} Brotli stream validation:${NC}"
    echo -e "${CYAN}${INFO} - Compressed size: ${br_size} bytes${NC}"
    echo -e "${CYAN}${INFO} - Compression ratio: $(echo "scale=2; ${br_size}*100/$("${STAT}" -c%s "${TAR_FILE}" || true)" | bc || true)%${NC}"
    echo -e "${CYAN}${INFO} - First 32 bytes: ${br_head_16}${NC}"
    echo -e "${CYAN}${INFO} - Last 32 bytes: ${br_tail_16}${NC}"

    # Validate brotli compression
    validate_brotli_compression "${TEMP_DIR}/payload.tar.br" "${TAR_FILE}"

    # Create validation files
    create_validation_files "${br_size}" "${br_head_16}" "${br_tail_16}"

    print_header "Encrypting Payload Package"
    echo -e "${CYAN}${INFO} Encrypting payload tarball...${NC}"

    # Check for PAYLOAD_LOCK environment variable (RSA public key for AES key encryption)
    if [[ -z "${PAYLOAD_LOCK}" ]]; then
        echo -e "${RED}${FAIL} Error: PAYLOAD_LOCK environment variable must be set${NC}"
        echo -e "${YELLOW}${INFO} This variable should contain the base64-encoded RSA public key for payload encryption${NC}"
        exit 1
    fi

    # Log first 5 chars of PAYLOAD_LOCK
    local payload_lock_prefix
    payload_lock_prefix=$(echo "${PAYLOAD_LOCK}" | cut -c1-5)
    echo -e "${CYAN}${INFO} PAYLOAD_LOCK first 5 chars: ${BOLD}${payload_lock_prefix}${NC}"

    # Generate a random AES-256 key and IV for encrypting the payload
    echo -e "${CYAN}${INFO} Generating AES-256 key and IV for payload encryption...${NC}"
    openssl rand -out "${TEMP_DIR}/aes_key.bin" 32
    openssl rand -out "${TEMP_DIR}/aes_iv.bin" 16

    # Log first 5 chars of AES key and IV (hex)
    echo -e "${CYAN}${INFO} AES key first 5 chars (hex): ${BOLD}$(xxd -p -l 5 "${TEMP_DIR}/aes_key.bin" || true)${NC}"
    echo -e "${CYAN}${INFO} AES IV first 5 chars (hex): ${BOLD}$(xxd -p -l 5 "${TEMP_DIR}/aes_iv.bin" || true)${NC}"

    # Encrypt the AES key with the RSA public key (PAYLOAD_LOCK)
    echo -e "${CYAN}${INFO} Encrypting AES key with RSA public key (PAYLOAD_LOCK)...${NC}"

    # Save the base64-decoded PAYLOAD_LOCK to a temporary PEM file
    echo "${PAYLOAD_LOCK}" | openssl base64 -d -A > "${TEMP_DIR}/public_key.pem"

    # Encrypt the AES key with the public key
    openssl pkeyutl -encrypt -inkey "${TEMP_DIR}/public_key.pem" -pubin \
                   -in "${TEMP_DIR}/aes_key.bin" -out "${TEMP_DIR}/encrypted_aes_key.bin"

    # Get the size of the encrypted AES key
    local encrypted_key_size
    encrypted_key_size=$("${STAT}" -c%s "${TEMP_DIR}/encrypted_aes_key.bin")

    # Save pre-encryption validation data
    echo -e "${CYAN}${INFO} Pre-encryption validation:${NC}"
    echo -e "${CYAN}${INFO} AES encryption parameters:${NC}"
    echo -e "${CYAN}${INFO} - Algorithm: AES-256-CBC${NC}"
    echo -e "${CYAN}${INFO} - Key: Direct (32 bytes)${NC}"
    echo -e "${CYAN}${INFO} - IV: Direct (16 bytes)${NC}"
    echo -e "${CYAN}${INFO} - Padding: PKCS7${NC}"

    # Encrypt the Brotli-compressed tar with AES using direct key and IV
    echo -e "${CYAN}${INFO} Encrypting compressed tar with AES-256 (direct key/IV)...${NC}"
    openssl enc -aes-256-cbc \
                -in "${TEMP_DIR}/payload.tar.br" \
                -out "${TEMP_DIR}/temp_payload.enc" \
                -K "$(xxd -p -c 32 "${TEMP_DIR}/aes_key.bin" || true)" \
                -iv "$(xxd -p -c 16 "${TEMP_DIR}/aes_iv.bin" || true)"

    # Verify encryption size
    local enc_size
    enc_size=$("${STAT}" -c%s "${TEMP_DIR}/temp_payload.enc")
    echo -e "${CYAN}${INFO} Encryption validation:${NC}"
    echo -e "${CYAN}${INFO} - Original size: ${br_size} bytes${NC}"
    echo -e "${CYAN}${INFO} - Encrypted size: ${enc_size} bytes${NC}"

    # Quick validation test (decrypt and compare headers)
    echo -e "${CYAN}${INFO} Performing validation test...${NC}"
    openssl enc -d -aes-256-cbc \
                -in "${TEMP_DIR}/temp_payload.enc" \
                -out "${TEMP_DIR}/validation.br" \
                -K "$(xxd -p -c 32 "${TEMP_DIR}/aes_key.bin" || true)" \
                -iv "$(xxd -p -c 16 "${TEMP_DIR}/aes_iv.bin" || true)"

    # Compare the first 16 bytes
    local val_head_16
    val_head_16=$(head -c16 "${TEMP_DIR}/validation.br" | xxd -p | tr -d '\n' || true)
    if [[ "${val_head_16}" = "${br_head_16}" ]]; then
        echo -e "${GREEN}${PASS} Encryption validation passed - headers match${NC}"
    else
        echo -e "${RED}${FAIL} Encryption validation failed!${NC}"
        echo -e "${RED}${INFO} Original : ${br_head_16}${NC}"
        echo -e "${RED}${INFO} Decrypted: ${val_head_16}${NC}"
        exit 1
    fi

    # Log the first 16 bytes of the encrypted payload
    local enc_head_16
    enc_head_16=$(head -c16 "${TEMP_DIR}/temp_payload.enc" | xxd -p | tr -d '\n' || true)
    echo -e "${CYAN}${INFO} AES-encrypted payload first 16 bytes: ${NC}${enc_head_16}"

    # Combine the encrypted AES key, IV, and encrypted payload
    echo -e "${CYAN}${INFO} Creating final encrypted payload with IV...${NC}"

    # Write the size of the encrypted key as a 4-byte binary header
    printf "%08x" "${encrypted_key_size}" | xxd -r -p > "${COMPRESSED_TAR_FILE}"

    # Append the encrypted AES key, IV, and encrypted payload using grouped redirects
    {
        cat "${TEMP_DIR}/encrypted_aes_key.bin"
        cat "${TEMP_DIR}/aes_iv.bin"
        cat "${TEMP_DIR}/temp_payload.enc"
    } >> "${COMPRESSED_TAR_FILE}"

    # List the contents of the tarball before encryption
    print_header "Tarball Contents"
    echo -e "${CYAN}${INFO} Listing contents of compressed tarball:${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"

    # Declare associative arrays for folder breakdown
    declare -A dir_counts
    declare -A dir_sizes

    while read -r line; do
        # Check if the file is a brotli-compressed file
        if [[ "${line}" == *".br" ]]; then
            echo -e "  ${MAGENTA}${BOLD}${line}${NC} ${YELLOW}(brotli-compressed)${NC}"
        else
            echo -e "  ${CYAN}${line}${NC}"
        fi

        # Parse line for folder breakdown
        size=$(echo "${line}" | awk '{print $3}')
        path=$(echo "${line}" | awk '{for(i=6;i<=NF;i++) printf "%s ", $i; print ""}' | sed 's/ $//')
        dir=$(dirname "${path}")
        if [[ "${dir}" == "." ]]; then
            dir="root"
        fi
        ((dir_counts[${dir}]++))
        ((dir_sizes[${dir}] += size))
    done < <(brotli -d < "${TEMP_DIR}/payload.tar.br" | "${TAR}" -tvf - | sort -k 6) || true

    # Calculate total file size for accurate percentages
    local total_file_size=0
    local total_count=0
    for dir in "${!dir_sizes[@]}"; do
        total_file_size=$(( total_file_size + dir_sizes[${dir}] ))
        total_count=$(( total_count + dir_counts[${dir}] ))
    done

    # Display folder breakdown
    print_header "Folder Breakdown"
    for dir in $(printf '%s\n' "${!dir_counts[@]}" | sort); do
        count=${dir_counts[${dir}]}
        total_size=${dir_sizes[${dir}]}
        size_kb=$(( (total_size + 1023) / 1024 ))
        percentage=$(awk "BEGIN {printf \"%.1f\", ${total_size} * 100 / ${total_file_size}}")
        printf "  %-12s %4d files, %4d KB, %5.1f%%\n" "${dir}" "${count}" "${size_kb}" "${percentage}"
    done

    # Display totals
    local total_kb=$(( (total_file_size + 1023) / 1024 ))
    local overhead_kb=$(( (tar_size - total_file_size + 1023) / 1024 ))
    printf "  %-12s %4d files, %4d KB, %5.1f%%\n" "Total" "${total_count}" "${total_kb}" "100.0"
    printf "  %-12s %16d KB\n" "Overhead" " ${overhead_kb}"

    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"

    # Display file sizes and hex samples for reference
    print_header "Payload Details"
    
    # Original tarball
    local tar_head tar_tail
    tar_head=$(head -c5 "${TAR_FILE}" | xxd -p | tr -d '\n' || true)
    tar_tail=$(tail -c5 "${TAR_FILE}" | xxd -p | tr -d '\n' || true)
    echo -e "  ${GREEN}${INFO} Uncompressed tar:         ${NC}${tar_size} bytes"
    echo -e "  ${GREEN}${INFO} First 5 bytes (hex):      ${NC}${tar_head}"
    echo -e "  ${GREEN}${INFO} Last 5 bytes (hex):       ${NC}${tar_tail}"

    # Brotli compressed tar
    local br_head_5 br_tail_5
    br_head_5=$(head -c5 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n' || true)
    br_tail_5=$(tail -c5 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n' || true)
    echo -e "  ${GREEN}${INFO} Compressed tar (brotli):  ${NC}${br_size} bytes"
    echo -e "  ${GREEN}${INFO} First 5 bytes (hex):      ${NC}${br_head_5}"
    echo -e "  ${GREEN}${INFO} Last 5 bytes (hex):       ${NC}${br_tail_5}"

    # Final encrypted payload
    local final_enc_size final_enc_head final_enc_tail
    final_enc_size=$("${STAT}" -c%s "${COMPRESSED_TAR_FILE}")
    final_enc_head=$(head -c5 "${COMPRESSED_TAR_FILE}" | xxd -p | tr -d '\n' || true)
    final_enc_tail=$(tail -c5 "${COMPRESSED_TAR_FILE}" | xxd -p | tr -d '\n' || true)
    echo -e "  ${GREEN}${INFO} Encrypted payload:        ${NC}${final_enc_size} bytes"
    echo -e "  ${GREEN}${INFO} First 5 bytes (hex):      ${NC}${final_enc_head}"
    echo -e "  ${GREEN}${INFO} Last 5 bytes (hex):       ${NC}${final_enc_tail}"

    # Log size of encrypted AES key being added
    echo -e "  ${GREEN}${INFO} Encrypted AES key size:   ${NC}${encrypted_key_size} bytes"

    echo -e "${GREEN}${PASS} Encrypted payload package is ready for distribution.${NC}"
}

# Function to display completion summary
display_completion_summary() {
    print_header "Generation Complete"
    PATH3=$(set -e;convert_to_relative_path "${COMPRESSED_TAR_FILE}")
    echo -e "${GREEN}${PASS} ${BOLD}Encrypted payload generation completed successfully!${NC}"
    echo -e "${CYAN}${INFO} Encrypted payload created at: ${BOLD}${PATH3}${NC}"
    echo -e "${YELLOW}${INFO} Cleaning up will occur on exit...${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Set up cleanup trap
trap cleanup EXIT

# Main execution flow
main() {
    # Display script information
    display_script_info

    # Check dependencies
    check_dependencies

    # Execute main workflow
    print_header "Payload Generation Process"
    download_swaggerui
    generate_terminal_payload
    copy_migration_files
    compress_static_assets
    create_tarball

    # Display completion summary
    display_completion_summary
}

# Execute main function
main "$@"
