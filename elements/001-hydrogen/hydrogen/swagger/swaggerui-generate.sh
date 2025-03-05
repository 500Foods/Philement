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
INFO="🛈 "

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
COMPRESSED_TAR_FILE="${SCRIPT_DIR}/payload.tar.br"

# Check for required dependencies
check_dependencies() {
    local missing_deps=0
    
    # Required for basic operation
    command -v curl >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: curl is required but not installed. Please install curl.${NC}"; missing_deps=1; }
    command -v tar >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: tar is required but not installed. Please install tar.${NC}"; missing_deps=1; }
    command -v brotli >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: brotli is required but not installed. Please install brotli.${NC}"; missing_deps=1; }
    
    # Required for encryption
    command -v openssl >/dev/null 2>&1 || { echo -e "${RED}${FAIL} Error: openssl is required but not installed. Please install openssl.${NC}"; missing_deps=1; }
    
    if [ $missing_deps -ne 0 ]; then
        exit 1
    fi
}

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
echo -e "${CYAN}${INFO} Final encrypted file:    ${NC}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")"
echo -e "${CYAN}${INFO} Temporary directory:     ${NC}${TEMP_DIR}"

# Check dependencies
check_dependencies

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
    
    # Remove encryption temporary files if they exist
    rm -f "${TEMP_DIR}/aes_key.bin" "${TEMP_DIR}/encrypted_aes_key.bin" "${TEMP_DIR}/temp_payload.enc"
    
    # Remove any generated .br files in the script directory (except the final compressed tar)
    find "${SCRIPT_DIR}" -name "*.br" -not -name "payload.tar.br" -delete
    
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
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-ui-standalone-preset.js" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-ui.css" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/oauth2-redirect.html" "${SWAGGERUI_DIR}/"
    
    # Copy swagger.json (both compressed and uncompressed)
    if [ -f "${SCRIPT_DIR}/swagger.json" ]; then
        echo -e "${GREEN}${PASS} Found swagger.json, copying for packaging...${NC}"
        cp "${SCRIPT_DIR}/swagger.json" "${SWAGGERUI_DIR}/"
    else
        echo -e "${YELLOW}${WARN} Warning: swagger.json not found. Run swagger-generate.sh first.${NC}"
        # Create a minimal placeholder swagger.json
        echo '{"openapi":"3.1.0","info":{"title":"Hydrogen API","version":"1.0.0"},"paths":{}}' > "${SWAGGERUI_DIR}/swagger.json"
    fi
    
    # Process dynamic files (to remain uncompressed)
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/index.html" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/swagger-initializer.js" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-32x32.png" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-16x16.png" "${SWAGGERUI_DIR}/"
    
    # Customize index.html
    echo -e "${CYAN}${INFO} Customizing index.html...${NC}"
    cat > "${SWAGGERUI_DIR}/index.html" << 'EOF'
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
    
    # Update swagger-initializer.js to use our settings
    echo -e "${CYAN}${INFO} Customizing swagger-initializer.js with recommended settings...${NC}"
    cat > "${SWAGGERUI_DIR}/swagger-initializer.js" << EOF
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
    
    
    echo -e "${GREEN}${PASS} SwaggerUI files prepared for packaging.${NC}"
}

# Apply Brotli compression to static assets
compress_static_assets() {
    print_header "Compressing Static Assets"
    echo -e "${CYAN}${INFO} Applying Brotli compression to static assets...${NC}"
    
    # Compress swagger-ui-bundle.js (won't change at runtime)
    echo -e "${CYAN}${INFO} Compressing swagger-ui-bundle.js with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" -o "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br"
    
    # Compress swagger-ui-standalone-preset.js
    echo -e "${CYAN}${INFO} Compressing swagger-ui-standalone-preset.js with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js" -o "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js.br"
    
    # Compress swagger-ui.css (static styling)
    echo -e "${CYAN}${INFO} Compressing swagger-ui.css with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui.css" -o "${SWAGGERUI_DIR}/swagger-ui.css.br"
    
    # Compress oauth2-redirect.html
    echo -e "${CYAN}${INFO} Compressing oauth2-redirect.html with Brotli (best compression)...${NC}"
    brotli --best -f "${SWAGGERUI_DIR}/oauth2-redirect.html" -o "${SWAGGERUI_DIR}/oauth2-redirect.html.br"
    
    # Remove the uncompressed static files - we'll only include the .br versions in the tar
    rm -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" \
          "${SWAGGERUI_DIR}/swagger-ui-standalone-preset.js" \
          "${SWAGGERUI_DIR}/swagger-ui.css" \
          "${SWAGGERUI_DIR}/oauth2-redirect.html"
    
    echo -e "${GREEN}${PASS} Static assets compressed successfully.${NC}"
}

# Create and encrypt payload
create_tarball() {
    print_header "Creating Distribution Package"
    echo -e "${CYAN}${INFO} Creating SwaggerUI tarball...${NC}"
    
    # Create flat tar file including only what we need
    # - Compressed static assets (.br files)
    # - Uncompressed dynamic files and swagger.json
    # - Strip metadata (permissions and ownership) since we're the only ones using it
    cd "${SWAGGERUI_DIR}" && tar --mode=0000 --owner=0 --group=0 -cf "${TAR_FILE}" \
        swagger-ui-bundle.js.br \
        swagger-ui-standalone-preset.js.br \
        swagger-ui.css.br \
        swagger.json \
        oauth2-redirect.html.br \
        index.html \
        swagger-initializer.js \
        favicon-32x32.png \
        favicon-16x16.png
    
    echo -e "${CYAN}${INFO} Compressing tar file with Brotli (best compression)...${NC}"
    brotli --best -f "${TAR_FILE}" -o "${COMPRESSED_TAR_FILE}"
    
    # Check for PAYLOAD_LOCK environment variable (RSA public key for AES key encryption)
    if [ -z "${PAYLOAD_LOCK}" ]; then
        echo -e "${YELLOW}${WARN} PAYLOAD_LOCK environment variable not set, generating a test key pair...${NC}"
        
        # Generate a temporary RSA key pair for testing
        echo -e "${CYAN}${INFO} Generating temporary RSA key pair for testing...${NC}"
        openssl genrsa -out "${TEMP_DIR}/private_key.pem" 2048 >/dev/null 2>&1
        openssl rsa -in "${TEMP_DIR}/private_key.pem" -pubout -out "${TEMP_DIR}/public_key.pem" >/dev/null 2>&1
        
        # Export the keys to environment variables (base64 encoded)
        export PAYLOAD_LOCK=$(openssl base64 -A -in "${TEMP_DIR}/public_key.pem")
        export PAYLOAD_KEY=$(openssl base64 -A -in "${TEMP_DIR}/private_key.pem")
        
        echo -e "${YELLOW}${WARN} Generated TEST keys (DO NOT USE IN PRODUCTION)${NC}"
        echo -e "${YELLOW}${WARN} PAYLOAD_LOCK (public key) and PAYLOAD_KEY (private key) environment variables set${NC}"
    fi
    
    # Generate a random AES-256 key for encrypting the payload
    echo -e "${CYAN}${INFO} Generating AES-256 key for payload encryption...${NC}"
    openssl rand -out "${TEMP_DIR}/aes_key.bin" 32
    
    # Encrypt the AES key with the RSA public key (PAYLOAD_LOCK)
    echo -e "${CYAN}${INFO} Encrypting AES key with RSA public key (PAYLOAD_LOCK)...${NC}"
    
    # Save the base64-decoded PAYLOAD_LOCK to a temporary PEM file
    echo "${PAYLOAD_LOCK}" | openssl base64 -d -A > "${TEMP_DIR}/public_key.pem"
    
    # Encrypt the AES key with the public key
    openssl rsautl -encrypt -inkey "${TEMP_DIR}/public_key.pem" -pubin \
                   -in "${TEMP_DIR}/aes_key.bin" -out "${TEMP_DIR}/encrypted_aes_key.bin"
    
    # Get the size of the encrypted AES key
    ENCRYPTED_KEY_SIZE=$(stat -c%s "${TEMP_DIR}/encrypted_aes_key.bin")
    
    # Compress the tar with Brotli
    echo -e "${CYAN}${INFO} Compressing tar file with Brotli (best compression)...${NC}"
    brotli --best -f "${TAR_FILE}" -o "${TEMP_DIR}/payload.tar.br"
    
    # Encrypt the Brotli-compressed tar with AES
    echo -e "${CYAN}${INFO} Encrypting compressed tar with AES-256...${NC}"
    openssl enc -aes-256-cbc -salt -in "${TEMP_DIR}/payload.tar.br" \
                -out "${TEMP_DIR}/temp_payload.enc" -pass file:"${TEMP_DIR}/aes_key.bin"
    
    # Combine the encrypted AES key and the encrypted payload
    echo -e "${CYAN}${INFO} Creating final encrypted payload...${NC}"
    
    # First, write the size of the encrypted key as a 4-byte binary header
    printf "%08x" $ENCRYPTED_KEY_SIZE | xxd -r -p > "${COMPRESSED_TAR_FILE}"
    
    # Append the encrypted AES key
    cat "${TEMP_DIR}/encrypted_aes_key.bin" >> "${COMPRESSED_TAR_FILE}"
    
    # Append the encrypted payload
    cat "${TEMP_DIR}/temp_payload.enc" >> "${COMPRESSED_TAR_FILE}"
    
    # Display file sizes for reference
    echo -e "${BLUE}${BOLD}Distribution file sizes:${NC}"
    ls -lh "${TAR_FILE}" | awk '{printf "  '"${GREEN}${INFO}"' Uncompressed tar:      '"${NC}"'%s\n", $5}'
    ls -lh "${TEMP_DIR}/payload.tar.br" | awk '{printf "  '"${GREEN}${INFO}"' Compressed tar (brotli): '"${NC}"'%s\n", $5}'
    ls -lh "${COMPRESSED_TAR_FILE}" | awk '{printf "  '"${GREEN}${INFO}"' Encrypted payload:     '"${NC}"'%s\n", $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br" | awk '{printf "  '"${GREEN}${INFO}"' swagger-ui-bundle.js.br: '"${NC}"'%s\n", $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui.css.br" | awk '{printf "  '"${GREEN}${INFO}"' swagger-ui.css.br:      '"${NC}"'%s\n", $5}'
    
    echo -e "${GREEN}${PASS} Encrypted payload package is ready for distribution.${NC}"
    
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
echo -e "${GREEN}${PASS} ${BOLD}SwaggerUI generation and encryption completed successfully!${NC}"
echo -e "${CYAN}${INFO} Encrypted payload created at: ${BOLD}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")${NC}"
echo -e "${YELLOW}${INFO} Cleaning up will occur on exit...${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"