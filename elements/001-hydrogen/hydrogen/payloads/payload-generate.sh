#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# Encrypted Payload Generator for Hydrogen
# ──────────────────────────────────────────────────────────────────────────────
# Script Name: payload-generate.sh
# Version: 1.2.0
# Author: Hydrogen Development Team
# Last Modified: 2025-06-17
#
# Version History:
# 1.0.0 - Initial release with basic payload generation
# 1.1.0 - Added RSA+AES hybrid encryption support
# 1.2.0 - Improved modularity, fixed shellcheck warnings, enhanced error handling
#
# Description:
# This script creates an encrypted payload package for the Hydrogen project.
# Currently, it packages SwaggerUI content, but is designed to be extendable
# for other payload types in the future. The script:
# - Downloads SwaggerUI from GitHub
# - Extracts essential files and compresses static assets with Brotli
# - Creates an optimized tar file compressed with Brotli
# - Encrypts the package using RSA+AES hybrid encryption
# - Cleans up all temporary files
# ──────────────────────────────────────────────────────────────────────────────

# Display script information
echo "payload-generate.sh version 1.2.0"
echo "Encrypted Payload Generator for Hydrogen"
echo ""

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
    relative_path=$(echo "$absolute_path" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
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
readonly SCRIPT_DIR
readonly SWAGGERUI_VERSION="5.19.0"
readonly SWAGGERUI_DIR="${SCRIPT_DIR}/swaggerui"
readonly TAR_FILE="${SCRIPT_DIR}/payload.tar"
readonly COMPRESSED_TAR_FILE="${SCRIPT_DIR}/payload.tar.br.enc"

# Create temporary directory
TEMP_DIR=$(mktemp -d)
readonly TEMP_DIR

# Function to check for required dependencies
check_dependencies() {
    local missing_deps=0
    
    # Required for basic operation
    if ! command -v curl >/dev/null 2>&1; then
        echo -e "${RED}${FAIL} Error: curl is required but not installed. Please install curl.${NC}"
        missing_deps=1
    fi
    
    if ! command -v tar >/dev/null 2>&1; then
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
    
    if [ $missing_deps -ne 0 ]; then
        exit 1
    fi
}

# Function to print formatted header
print_header() {
    local title="$1"
    echo -e "
${BLUE}────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD} $title ${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Function to display script information
display_script_info() {
    print_header "Encrypted Payload Generator for Hydrogen"
    echo -e "${CYAN}${INFO} SwaggerUI Version:       ${NC}${SWAGGERUI_VERSION}"
    echo -e "${CYAN}${INFO} Working directory:       ${NC}$(convert_to_relative_path "${SWAGGERUI_DIR}")"
    echo -e "${CYAN}${INFO} Final encrypted file:    ${NC}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")"
    echo -e "${CYAN}${INFO} Temporary directory:     ${NC}${TEMP_DIR}"
}

# Function to clean up temporary files and directories
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
    
    # Remove any generated .br files in the script directory (except the final encrypted payload)
    find "${SCRIPT_DIR}" -name "*.br" -not -name "payload.tar.br.enc" -delete
    
    echo -e "${GREEN}${PASS} Cleanup completed successfully.${NC}"
}

# Function to create SwaggerUI index.html
create_index_html() {
    local target_file="$1"
    cat > "$target_file" << 'EOF'
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
    cat > "$target_file" << EOF
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

# Function to download and extract SwaggerUI
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
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-32x32.png" "${SWAGGERUI_DIR}/"
    cp "${TEMP_DIR}/swagger-ui-${SWAGGERUI_VERSION}/dist/favicon-16x16.png" "${SWAGGERUI_DIR}/"
    
    # Customize index.html and swagger-initializer.js
    echo -e "${CYAN}${INFO} Customizing index.html...${NC}"
    create_index_html "${SWAGGERUI_DIR}/index.html"
    
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
    if ! brotli -d "$compressed_file" -o "${TEMP_DIR}/test.tar" 2>/dev/null; then
        echo -e "${RED}${FAIL} Brotli validation failed - invalid compressed data${NC}"
        exit 1
    fi
    
    # Compare original and decompressed files
    if ! cmp -s "$original_file" "${TEMP_DIR}/test.tar"; then
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
    
    # Compress the tar file with Brotli using explicit settings
    echo -e "${CYAN}${INFO} Compressing tar file with Brotli...${NC}"
    echo -e "${CYAN}${INFO} - Quality: 11 (maximum)${NC}"
    echo -e "${CYAN}${INFO} - Window: 24 (16MB)${NC}"
    echo -e "${CYAN}${INFO} - Input size: $(stat -c%s "${TAR_FILE}") bytes${NC}"
    
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
    if [ ! -f "${TEMP_DIR}/payload.tar.br" ]; then
        echo -e "${RED}${FAIL} Compressed file not created${NC}"
        exit 1
    fi
    
    # Log detailed Brotli stream information
    local br_size
    local br_head_16
    local br_tail_16
    br_size=$(stat -c%s "${TEMP_DIR}/payload.tar.br")
    br_head_16=$(head -c16 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n')
    br_tail_16=$(tail -c16 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n')
    
    echo -e "${CYAN}${INFO} Brotli stream validation:${NC}"
    echo -e "${CYAN}${INFO} - Compressed size: ${br_size} bytes${NC}"
    echo -e "${CYAN}${INFO} - Compression ratio: $(echo "scale=2; ${br_size}*100/$(stat -c%s "${TAR_FILE}")" | bc)%${NC}"
    echo -e "${CYAN}${INFO} - First 32 bytes: ${br_head_16}${NC}"
    echo -e "${CYAN}${INFO} - Last 32 bytes: ${br_tail_16}${NC}"
    
    # Validate brotli compression
    validate_brotli_compression "${TEMP_DIR}/payload.tar.br" "${TAR_FILE}"
    
    # Create validation files
    create_validation_files "$br_size" "$br_head_16" "$br_tail_16"

    print_header "Encrypting Payload Package"
    echo -e "${CYAN}${INFO} Encrypting payload tarball...${NC}"

    # Check for PAYLOAD_LOCK environment variable (RSA public key for AES key encryption)
    if [ -z "${PAYLOAD_LOCK}" ]; then
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
    echo -e "${CYAN}${INFO} AES key first 5 chars (hex): ${BOLD}$(xxd -p -l 5 "${TEMP_DIR}/aes_key.bin")${NC}"
    echo -e "${CYAN}${INFO} AES IV first 5 chars (hex): ${BOLD}$(xxd -p -l 5 "${TEMP_DIR}/aes_iv.bin")${NC}"

    # Encrypt the AES key with the RSA public key (PAYLOAD_LOCK)
    echo -e "${CYAN}${INFO} Encrypting AES key with RSA public key (PAYLOAD_LOCK)...${NC}"
    
    # Save the base64-decoded PAYLOAD_LOCK to a temporary PEM file
    echo "${PAYLOAD_LOCK}" | openssl base64 -d -A > "${TEMP_DIR}/public_key.pem"
    
    # Encrypt the AES key with the public key
    openssl pkeyutl -encrypt -inkey "${TEMP_DIR}/public_key.pem" -pubin \
                   -in "${TEMP_DIR}/aes_key.bin" -out "${TEMP_DIR}/encrypted_aes_key.bin"
    
    # Get the size of the encrypted AES key
    local encrypted_key_size
    encrypted_key_size=$(stat -c%s "${TEMP_DIR}/encrypted_aes_key.bin")

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
                -K "$(xxd -p -c 32 "${TEMP_DIR}/aes_key.bin")" \
                -iv "$(xxd -p -c 16 "${TEMP_DIR}/aes_iv.bin")"
    
    # Verify encryption size
    local enc_size
    enc_size=$(stat -c%s "${TEMP_DIR}/temp_payload.enc")
    echo -e "${CYAN}${INFO} Encryption validation:${NC}"
    echo -e "${CYAN}${INFO} - Original size: ${br_size} bytes${NC}"
    echo -e "${CYAN}${INFO} - Encrypted size: ${enc_size} bytes${NC}"
    
    # Quick validation test (decrypt and compare headers)
    echo -e "${CYAN}${INFO} Performing validation test...${NC}"
    openssl enc -d -aes-256-cbc \
                -in "${TEMP_DIR}/temp_payload.enc" \
                -out "${TEMP_DIR}/validation.br" \
                -K "$(xxd -p -c 32 "${TEMP_DIR}/aes_key.bin")" \
                -iv "$(xxd -p -c 16 "${TEMP_DIR}/aes_iv.bin")"
    
    # Compare the first 16 bytes
    local val_head_16
    val_head_16=$(head -c16 "${TEMP_DIR}/validation.br" | xxd -p | tr -d '\n')
    if [ "${val_head_16}" = "${br_head_16}" ]; then
        echo -e "${GREEN}${PASS} Encryption validation passed - headers match${NC}"
    else
        echo -e "${RED}${FAIL} Encryption validation failed!${NC}"
        echo -e "${RED}${INFO} Original : ${br_head_16}${NC}"
        echo -e "${RED}${INFO} Decrypted: ${val_head_16}${NC}"
        exit 1
    fi
    
    # Log the first 16 bytes of the encrypted payload
    local enc_head_16
    enc_head_16=$(head -c16 "${TEMP_DIR}/temp_payload.enc" | xxd -p | tr -d '\n')
    echo -e "${CYAN}${INFO} AES-encrypted payload first 16 bytes: ${NC}${enc_head_16}"
    
    # Combine the encrypted AES key, IV, and encrypted payload
    echo -e "${CYAN}${INFO} Creating final encrypted payload with IV...${NC}"
    
    # Write the size of the encrypted key as a 4-byte binary header
    printf "%08x" "$encrypted_key_size" | xxd -r -p > "${COMPRESSED_TAR_FILE}"
    
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
    brotli -d < "${TEMP_DIR}/payload.tar.br" | tar -tvf - | \
    while read -r line; do
        # Check if the file is a brotli-compressed file
        if [[ "$line" == *".br" ]]; then
            echo -e "  ${MAGENTA}${BOLD}$line${NC} ${YELLOW}(brotli-compressed)${NC}"
        else
            echo -e "  ${CYAN}$line${NC}"
        fi
    done
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
    
    # Display file sizes and hex samples for reference
    echo -e "${BLUE}${BOLD}Distribution file details:${NC}"
    
    # Original tarball
    local tar_size tar_head tar_tail
    tar_size=$(stat -c%s "${TAR_FILE}")
    tar_head=$(head -c5 "${TAR_FILE}" | xxd -p | tr -d '\n')
    tar_tail=$(tail -c5 "${TAR_FILE}" | xxd -p | tr -d '\n')
    echo -e "  ${GREEN}${INFO} Uncompressed tar:         ${NC}${tar_size} bytes"
    echo -e "  ${GREEN}${INFO} First 5 bytes (hex):      ${NC}${tar_head}"
    echo -e "  ${GREEN}${INFO} Last 5 bytes (hex):       ${NC}${tar_tail}"
    
    # Brotli compressed tar
    local br_head_5 br_tail_5
    br_head_5=$(head -c5 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n')
    br_tail_5=$(tail -c5 "${TEMP_DIR}/payload.tar.br" | xxd -p | tr -d '\n')
    echo -e "  ${GREEN}${INFO} Compressed tar (brotli):  ${NC}${br_size} bytes"
    echo -e "  ${GREEN}${INFO} First 5 bytes (hex):      ${NC}${br_head_5}"
    echo -e "  ${GREEN}${INFO} Last 5 bytes (hex):       ${NC}${br_tail_5}"
    
    # Final encrypted payload
    local final_enc_size final_enc_head final_enc_tail
    final_enc_size=$(stat -c%s "${COMPRESSED_TAR_FILE}")
    final_enc_head=$(head -c5 "${COMPRESSED_TAR_FILE}" | xxd -p | tr -d '\n')
    final_enc_tail=$(tail -c5 "${COMPRESSED_TAR_FILE}" | xxd -p | tr -d '\n')
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
    echo -e "${GREEN}${PASS} ${BOLD}Encrypted payload generation completed successfully!${NC}"
    echo -e "${CYAN}${INFO} Encrypted payload created at: ${BOLD}$(convert_to_relative_path "${COMPRESSED_TAR_FILE}")${NC}"
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
    compress_static_assets
    create_tarball
    
    # Display completion summary
    display_completion_summary
}

# Execute main function
main "$@"
