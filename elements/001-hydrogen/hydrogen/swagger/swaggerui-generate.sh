#!/bin/bash
# SwaggerUI downloader and packager for Hydrogen REST API
# This script downloads a specific version of SwaggerUI from GitHub,
# extracts essential files, selectively compresses static assets with Brotli,
# creates an optimized tar file compressed with brotli for distribution, 
# and cleans up all temporary files

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

# Display script info
echo "SwaggerUI Generator for Hydrogen API"
echo "SwaggerUI Version: ${SWAGGERUI_VERSION}"
echo "Temporary SwaggerUI directory: ${SWAGGERUI_DIR}"
echo "Final compressed file: ${COMPRESSED_TAR_FILE}"
echo "Using temporary directory: ${TEMP_DIR}"

# Check if required tools are installed
command -v curl >/dev/null 2>&1 || { echo "Error: curl is required but not installed. Please install curl."; exit 1; }
command -v tar >/dev/null 2>&1 || { echo "Error: tar is required but not installed. Please install tar."; exit 1; }
command -v brotli >/dev/null 2>&1 || { echo "Error: brotli is required but not installed. Please install brotli."; exit 1; }

# Clean up on exit - remove all temporary files and directories
cleanup() {
    echo "Cleaning up temporary files and directories..."
    # Remove the temporary working directory
    rm -rf "${TEMP_DIR}"
    
    # Remove the temporary swaggerui directory if it exists
    if [ -d "${SWAGGERUI_DIR}" ]; then
        echo "Removing temporary SwaggerUI directory..."
        rm -rf "${SWAGGERUI_DIR}"
    fi
    
    # Remove intermediate tar file if it exists
    if [ -f "${TAR_FILE}" ]; then
        echo "Removing intermediate tar file..."
        rm -f "${TAR_FILE}"
    fi
    
    # Remove any generated .br files in the script directory (except the final compressed tar)
    find "${SCRIPT_DIR}" -name "*.br" -not -name "swaggerui.tar.br" -delete
    
    echo "Cleanup completed."
}
trap cleanup EXIT

# Download and extract SwaggerUI
download_swaggerui() {
    echo "Downloading SwaggerUI v${SWAGGERUI_VERSION}..."
    
    # Download SwaggerUI from GitHub
    curl -L "https://github.com/swagger-api/swagger-ui/archive/refs/tags/v${SWAGGERUI_VERSION}.tar.gz" -o "${TEMP_DIR}/swagger-ui.tar.gz"
    
    echo "Extracting SwaggerUI distribution files..."
    # Extract the distribution directory
    tar -xzf "${TEMP_DIR}/swagger-ui.tar.gz" -C "${TEMP_DIR}" "swagger-ui-${SWAGGERUI_VERSION}/dist"
    
    # Create the temporary swaggerui directory for processing
    echo "Creating temporary SwaggerUI directory..."
    # Remove existing swaggerui directory if it exists
    if [ -d "${SWAGGERUI_DIR}" ]; then
        echo "Removing existing SwaggerUI directory..."
        rm -rf "${SWAGGERUI_DIR}"
    fi
    mkdir -p "${SWAGGERUI_DIR}"
    
    # List of required files from the distribution
    # We'll selectively copy only what we need
    echo "Selecting essential files for distribution..."
    
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
    echo "Customizing index.html..."
    sed -i 's|https://petstore.swagger.io/v2/swagger.json|swagger.json.br|' "${SWAGGERUI_DIR}/index.html"
    
    # Update swagger-initializer.js to use our settings
    echo "Customizing swagger-initializer.js with recommended settings..."
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
        echo "Copying swagger.json for packaging..."
        cp "${SCRIPT_DIR}/swagger.json" "${SWAGGERUI_DIR}/"
    else
        echo "Warning: swagger.json not found. Run swagger-generate.sh first."
        # Create a minimal placeholder swagger.json
        echo '{"openapi":"3.1.0","info":{"title":"Hydrogen API","version":"1.0.0"},"paths":{}}' > "${SWAGGERUI_DIR}/swagger.json"
    fi
    
    echo "SwaggerUI files prepared for packaging."
}

# Apply Brotli compression to static assets
compress_static_assets() {
    echo "Applying Brotli compression to static assets..."
    
    # Compress swagger-ui-bundle.js (won't change at runtime)
    echo "Compressing swagger-ui-bundle.js with Brotli (best compression)..."
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" -o "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br"
    
    # Compress swagger-ui.css (static styling)
    echo "Compressing swagger-ui.css with Brotli (best compression)..."
    brotli --best -f "${SWAGGERUI_DIR}/swagger-ui.css" -o "${SWAGGERUI_DIR}/swagger-ui.css.br"
    
    # Compress swagger.json
    echo "Compressing swagger.json with Brotli (best compression)..."
    brotli --best -f "${SWAGGERUI_DIR}/swagger.json" -o "${SWAGGERUI_DIR}/swagger.json.br"
    
    # Compress oauth2-redirect.html
    echo "Compressing oauth2-redirect.html with Brotli (best compression)..."
    brotli --best -f "${SWAGGERUI_DIR}/oauth2-redirect.html" -o "${SWAGGERUI_DIR}/oauth2-redirect.html.br"
    
    # Remove the uncompressed static files - we'll only include the .br versions in the tar
    rm -f "${SWAGGERUI_DIR}/swagger-ui-bundle.js" "${SWAGGERUI_DIR}/swagger-ui.css" "${SWAGGERUI_DIR}/swagger.json" "${SWAGGERUI_DIR}/oauth2-redirect.html"
    
    echo "Static assets compressed successfully."
}

# Create tar file for distribution and compress it with brotli
create_tarball() {
    echo "Creating SwaggerUI tarball..."
    
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
    
    echo "Compressing tar file with Brotli (best compression)..."
    brotli --best -f "${TAR_FILE}" -o "${COMPRESSED_TAR_FILE}"
    
    # Display file sizes for reference
    echo "Distribution file sizes:"
    ls -lh "${TAR_FILE}" | awk '{print "Uncompressed tar: " $5}'
    ls -lh "${COMPRESSED_TAR_FILE}" | awk '{print "Compressed tar (brotli): " $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui-bundle.js.br" | awk '{print "swagger-ui-bundle.js.br: " $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger-ui.css.br" | awk '{print "swagger-ui.css.br: " $5}'
    ls -lh "${SWAGGERUI_DIR}/swagger.json.br" | awk '{print "swagger.json.br: " $5}'
    
    echo "SwaggerUI package is ready for distribution."
}

# Main execution
echo "Starting SwaggerUI generation process..."

download_swaggerui
compress_static_assets
create_tarball

echo "SwaggerUI generation completed successfully."
echo "Compressed tarball created at: ${COMPRESSED_TAR_FILE}"
echo "Cleaning up will occur on exit..."