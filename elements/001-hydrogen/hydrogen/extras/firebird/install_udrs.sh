#!/usr/bin/env bash
# Firebird UDR (Brotli + JSON) install — builds and copies .so to UDR plugin dir
#
# Usage:
#   ./install_udrs.sh
#
# Requires:
#   - Firebird 4 development headers (firebird-devel)
#   - libbrotli development headers (libbrotli-devel)
#   - libjansson development headers (jansson-devel)
#   - g++ compiler
#   - Write access to Firebird UDR plugin directory (default: /plugins/udr)
#
# CHANGELOG
# 1.0.0 - 2026-09-19 - Initial version for Firebird 4.0 on Fedora 43

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HYDROGEN_ROOT="${HYDROGEN_ROOT:-$(cd "${SCRIPT_DIR}/../../.." && pwd)}"
UDR_DIR="${UDR_DIR:-/plugins/udr}"
SO_BROTLI="${UDR_DIR}/brotli_decfn.so"
SO_JSON="${UDR_DIR}/json_udfn.so"

echo "Installing Firebird UDRs..."
echo "  Brotli UDR:  ${SO_BROTLI}"
echo "  JSON UDR:    ${SO_JSON}"

# Check for required tools
for cmd in g++ make; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        echo "Error: ${cmd} is required but not found." >&2
        exit 1
    fi
done

# Check for Firebird headers
FIREBIRD_INCLUDE=""
if [[ -d "/usr/include/firebird" ]]; then
    FIREBIRD_INCLUDE="/usr/include/firebird"
elif [[ -d "/usr/firebird/include" ]]; then
    FIREBIRD_INCLUDE="/usr/firebird/include"
else
    echo "Error: Firebird development headers not found." >&2
    echo "  Install with: sudo dnf install firebird-devel" >&2
    exit 1
fi

echo "  Firebird headers: ${FIREBIRD_INCLUDE}"

# Check for brotli and jansson headers
if ! pkg-config --exists libbrotlienc 2>/dev/null && ! test -f /usr/include/brotli/decode.h; then
    echo "Error: Brotli development headers not found." >&2
    echo "  Install with: sudo dnf install libbrotli-devel" >&2
    exit 1
fi

if ! pkg-config --exists jansson 2>/dev/null && ! test -f /usr/include/jansson.h; then
    echo "Error: Jansson development headers not found." >&2
    echo "  Install with: sudo dnf install jansson-devel" >&2
    exit 1
fi

# Build Brotli UDR
echo "Building Brotli UDR..."
(
    cd "${HYDROGEN_ROOT}/extras/brotli_udf_firebird"
    make clean
    make
)

# Build JSON UDR
echo "Building JSON UDR..."
(
    cd "${HYDROGEN_ROOT}/extras/json_udf_firebird"
    make clean
    make
)

# Install to UDR directory
echo "Installing UDRs to ${UDR_DIR}..."
sudo mkdir -p "${UDR_DIR}"
sudo cp "${HYDROGEN_ROOT}/extras/brotli_udf_firebird/brotli_decfn.so" "${SO_BROTLI}"
sudo cp "${HYDROGEN_ROOT}/extras/json_udf_firebird/json_udfn.so" "${SO_JSON}"
sudo chmod 755 "${SO_BROTLI}" "${SO_JSON}"

echo ""
echo "UDRs installed successfully."
echo ""
echo "Register the functions in SQL (or via migration 1000):"
echo "  CREATE FUNCTION BROTLI_DECOMPRESS(compressed BLOB)"
echo "  RETURNS BLOB"
echo "  EXTERNAL NAME 'brotli_decfn!brotli_decompress'"
echo "  ENGINE UDR;"
echo ""
echo "  CREATE FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))"
echo "  RETURNS BLOB SUB_TYPE TEXT"
echo "  EXTERNAL NAME 'json_udfn!json_value'"
echo "  ENGINE UDR;"
