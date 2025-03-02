#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# SwaggerUI Downloader and Packager for Hydrogen REST API
# ──────────────────────────────────────────────────────────────────────────────
# This script downloads a specific version of SwaggerUI from GitHub,
# extracts essential files, selectively compresses static assets with Brotli,
# creates an optimized tar file compressed with brotli for distribution, 
# and cleans up all temporary files

# Terminal formatting codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'  # No Color

# Status symbols
PASS="✅"
FAIL="❌"
WARN="⚠️"
INFO="ℹ️"

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path=$(echo "$absolute_path" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [ -z "$relative_path" ]; then
        relative_path=$(echo "$absolute_path" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [ -z "$relative_path" ]; then
        echo "$absolute_path"
    else
        echo "$relative_path"
    fi
}

# Set script to exit on any error
set -e

# Path configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SWAGGERUI_VERSION="5.19.0"
SWAGGERUI_DIR="${SCRIPT_DIR}/swaggerui"
TEMP_DIR=$(mktemp -d)
TAR_FILE="${SCRIPT_DIR}/swaggerui.tar"
COMPRESSED_TAR_FILE="${SCRIPT_DIR}/swaggerui.tar.br"

# Print header function
print_header() {
    echo -e "\n${BLUE}────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD} $1 ${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Display script info
print_header "SwaggerUI Generator for Hydrogen API"
echo -e "${CYAN}${INFO} SwaggerUI Version:       ${NC}${SWAGGERUI_VERSION}"
echo -e "${CYAN}${INFO} Working directory:       ${NC}$(convert_to_relative_path "${SWAGGERUI_DIR}")"
echo -e "${CYAN}${INFO} Final compressed file:   ${NC}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")"
echo -e "${CYAN}${INFO} Temporary directory:     ${NC}${TEMP_DIR}"

# Check if required tools are installed
command -v curl >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: curl is required but not installed. Please install curl.${NC}"; exit 1; }
command -v tar >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: tar is required but not installed. Please install tar.${NC}"; exit 1; }
command -v brotli >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: brotli is required but not installed. Please install brotli.${NC}"; exit 1; }

# Clean up on exit - remove all temporary files and directories
cleanup() {
    print_header "Cleanup Process"
    echo -e "${CYAN}${INFO} Cleaning up temporary files and directories...${NC}"
    
    # Remove the temporary working directory
    rm -rf "${TEMP_DIR}"
    
    # Remove the temporary swaggerui directory if it exists
    if [ -d "${SWAGGERUI_DIR}" ]; then
        echo -e "${CYAN}${INFO} Removing temporary SwaggerUI directory...${NC}"
        rm -rf "${SWAGGERUI_DIR}"
    fi
    
    # Remove intermediate tar file if it exists
    if [ -f "${TAR_FILE}" ]; then
        echo -e "${CYAN}${INFO} Removing intermediate tar file...${NC}"
        rm -f "${TAR_FILE}"
    fi
    
    # Remove any generated .br files in the script directory (except the final compressed tar)
    find "${SCRIPT_DIR}" -name "*.br" -not -name "swaggerui.tar.br" -delete
    
    echo -e "${GREEN}${PASS} Cleanup completed successfully.${NC}"
}
trap cleanup EXIT

# Download and extract SwaggerUI
download_swaggerui() {
    print_header "Downloading and Extracting SwaggerUI"
    echo -e "${CYAN}${INFO} Downloading SwaggerUI v${SWAGGERUI_VERSION}...${NC}"
    
    # Download SwaggerUI from GitHub
    curl -L "https://github.com/swagger-api/swagger-ui/archive/refs/tags/v${SWAGGERUI_VERSION}.tar.gz" -o "${TEMP_DIR}/swagger-ui.tar.gz"
    
    echo -e "${CYAN}${INFO} Extracting SwaggerUI distribution files...${NC}"
    # Extract the distribution directory
    tar -xzf "${TEMP_DIR}/swagger-ui.tar.gz" -C "${TEMP_DIR}" "swagger-ui-${SWAGGERUI_VERSION}/dist"
    
    # Create the temporary swaggerui directory for processing
    echo -e "${CYAN}${INFO} Creating temporary SwaggerUI directory...${NC}"
    # Remove existing swaggerui directory if it exists
    if [ -d "${SWAGGERUI_DIR}" ]; then
        echo -e "${YELLOW}${WARN} Removing existing SwaggerUI directory...${NC}"
        rm -rf "${SWAGGERUI_DIR}"
    fi
    mkdir -p "${SWAGGERUI_DIR}"
    
    # List of required files from the distribution
    # We'll selectively copy only what we need
    echo -e "${CYAN}${INFO} Selecting essential files for distribution...${NC}"
    
    # Process static files (to be compressed with brotli)
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-ui-bundle.js" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-ui.css" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/oauth2-redirect.html" "${SWAGGERUI_DIR}/"
    
    # Process dynamic files (to remain uncompressed)
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/index.html" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-initializer.js" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-32x32.png" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-16x16.png" "${SWAGGERUI_DIR}/"
    
    # Modify index.html to use our swagger.json.br file
    echo -e "${CYAN}${INFO} Customizing index.html...${NC}"
    sed -i 's|https://petstore.swagger.io/v2/swagger.json|swagger.json.br|' "${SWAGGERUI_DIR}/index.html"
    
    # Update swagger-initializer.js to use our settings
    echo -e "${CYAN}${INFO} Customizing swagger-initializer.js with recommended settings...${NC}"
    cat > "${SWAGGERUI_DIR}/swagger-initializer.js" << EOF
window.onload = function() {
  window.ui = SwaggerUIBundle({
    url: "swagger.json.br",
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
    
    # Copy swagger.json to our temporary directory if it exists
    if [ -f "${SCRIPT_DIR}/swagger.json" ]; then
        echo -e "${GREEN}${PASS} Found swagger.json, copying for packaging...${NC}"
        cp "${SCRIPT_DIR}/swagger.json" "${SWAGGERUI_DIR}/"
    else
        echo -e "${YELLOW}${WARN} Warning: swagger.json not found. Run swagger-generate.sh first.${NC}"
        # Create a minimal placeholder swagger.json
        echo '{"openapi":"3.1.0","info":{"title":"Hydrogen API","version":"1.0.0"},"paths":{}}' > "${SWAGGERUI_DIR}/swagger.json"
    fi
    
    echo -e "${GREEN}${PASS} SwaggerUI files prepared for packaging.${NC}"
}

# Apply Brotli compression to static assets
compress_static_assets() {
    print_header "Compressing Static Assets"
    echo -e "${CYAN}${INFO} Applying Brotli compression to static assets...${NC}"
    
    # Compress swagger-ui-bundle.js (won't change at runtime)
    echo -e "${CYAN}${INFO} Compressing swagger-ui-bundle.js with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" -o "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br"
    
    # Compress swagger-ui.css (static styling)
    echo -e "${CYAN}${INFO} Compressing swagger-ui.css with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui.css" -o "${SWAGGERUI_DIR}/swagger-ui.css.br"
    
    # Compress swagger.json
    echo -e "${CYAN}${INFO} Compressing swagger.json with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger.json" -o "${SWAGGERUI_DIR}/swagger.json.br"
    
    # Compress oauth2-redirect.html
    echo -e "${CYAN}${INFO} Compressing oauth2-redirect.html with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/oauth2-redirect.html" -o "${SWAGGERUI_DIR}/oauth2-redirect.html.br"
    
    # Remove the uncompressed static files - we'll only include the .br versions in the tar
    rm -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" "${SWAGGERUI_DIR}/swagger-ui.css" "${SWAGGERUI_DIR}/swagger.json" "${SWAGGERUI_DIR}/oauth2-redirect.html"
    
    echo -e "${GREEN}${PASS} Static assets compressed successfully.${NC}"
}

# Create tar file for distribution and compress it with brotli
create_tarball() {
    print_header "Creating Distribution Package"
    echo -e "${CYAN}${INFO} Creating SwaggerUI tarball...${NC}"
    
    # Create flat tar file including only what we need
    # - Compressed static assets (.br files)
    # - Uncompressed dynamic files
    # - Strip metadata (permissions and ownership) since we're the only ones using it
    cd "${SWAGGERUI_DIR}" && tar --mode=0000 --owner=0 --group=0 -cf "${TAR_FILE}" \
        swagger-ui-bundle.js.br \
        swagger-ui.css.br \
        swagger.json.br \
        oauth2-redirect.html.br \
        index.html \
        swagger-initializer.js \
        favicon-32x32.png \
        favicon-16x16.png
    
    echo -e "${CYAN}${INFO} Compressing tar file with Brotli (best compression)...${NC}"
    brotli --best -f "${TAR_FILE}" -o "${COMPRESSED_TAR_FILE}"
    
    # Display file sizes for reference
    echo -e "${BLUE}${BOLD}Distribution file sizes:${NC}"
    ls -lh "${TAR_FILE}" | awk '{printf "  '"${GREEN}${INFO}"' Uncompressed tar:      '"${NC}"'%s\n", $5}'
    ls -lh "${COMPRESSED_TAR_FILE}" | awk '{printf "  '"${GREEN}${INFO}"' Compressed tar (brotli): '"${NC}"'%s\n", $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br" | awk '{printf "  '"${GREEN}${INFO}"' swagger-ui-bundle.js.br: '"${NC}"'%s\n", $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui.css.br" | awk '{printf "  '"${GREEN}${INFO}"' swagger-ui.css.br:      '"${NC}"'%s\n", $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger.json.br" | awk '{printf "  '"${GREEN}${INFO}"' swagger.json.br:        '"${NC}"'%s\n", $5}'
    
    echo -e "${GREEN}${PASS} SwaggerUI package is ready for distribution.${NC}"
    
    # List the contents of the tarball
    print_header "Tarball Contents"
    echo -e "${CYAN}${INFO} Listing contents of compressed tarball:${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
    brotli -d < "${COMPRESSED_TAR_FILE}" | tar -tvf - | \
    while read -r line; do
        # Check if the file is a brotli-compressed file
        if [[ "$line" == *".br" ]]; then
            echo -e "  ${MAGENTA}${BOLD}$line${NC} ${YELLOW}(brotli-compressed)${NC}"
        else
            echo -e "  ${CYAN}$line${NC}"
        fi
    done
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Main execution
print_header "SwaggerUI Generation Process"

download_swaggerui
compress_static_assets
create_tarball

print_header "Generation Complete"
echo -e "${GREEN}${PASS} ${BOLD}SwaggerUI generation completed successfully!${NC}"
echo -e "${CYAN}${INFO} Compressed tarball created at: ${BOLD}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")${NC}"
echo -e "${YELLOW}${INFO} Cleaning up will occur on exit...${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"