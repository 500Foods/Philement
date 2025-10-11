#!/bin/bash

# CHANGELOG: 3.0.0 - Ultra-simple script with focused patterns

# migrate_paths.sh - Migrate C/C++ include paths from relative to cmake-style includes
#
# Simple migration patterns:
# 1. #include "unity.h" -> #include <unity.h>
# 2. #include "../hydrogen.h" -> #include <src/hydrogen.h>
# 3. #include "../../../../src/something" -> #include <src/something>
# 4. #include "../../../../tests/unity/something" -> #include <unity/something>
# 5. #include <config/something> -> #include <src/config/something>
#
# Usage: ./migrate_paths.sh [file_or_directory]
#
# Examples:
#   ./migrate_paths.sh src/websocket/websocket_server_message.c
#   ./migrate_paths.sh src/
#   ./migrate_paths.sh tests/unity/src/

set -uo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print usage
print_usage() {
    echo "Usage: $0 [file_or_directory]"
    echo
    echo "Migrate C/C++ include paths from relative to cmake-style includes"
    echo
    echo "Examples:"
    echo "  $0 src/websocket/websocket_server_message.c"
    echo "  $0 src/"
    echo "  $0 tests/unity/src/"
    exit 1
}

# Function to log messages
log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

# Function to migrate includes in a single file
migrate_file() {
    local file="$1"

    # Skip if file doesn't exist
    if [[ ! -f "${file}" ]]; then
        log_warn "File not found: ${file}"
        return 1
    fi

    # Skip if not a C/C++ file
    if [[ ! "${file}" =~ \.(c|h|cpp|hpp)$ ]]; then
        log_warn "Skipping non-C/C++ file: ${file}"
        return 0
    fi

    log_info "Processing: ${file}"

    # Check if file has any includes that need migration
    if ! grep -q '#include.*["<]' "${file}"; then
        log_info "No includes found in: ${file}"
        return 0
    fi

    # Create backup
    local backup
    backup="${file}.backup.$(date +%Y%m%d_%H%M%S)"
    cp "${file}" "${backup}"
    log_info "Created backup: ${backup}"

    # Apply migrations using sed - simple, focused patterns

    # 1. Special case: #include "unity.h" -> #include <unity.h>
    sed -i 's|#include "unity\.h"|#include <unity.h>|g' "${file}"

    # 2. Migrate relative hydrogen.h includes
    sed -i 's|#include "\([^"]*hydrogen\.h\)"|#include <src/hydrogen.h>|g' "${file}"

    # 3. Migrate relative src/ includes
    sed -i 's|#include "\([^"]*src/\([^"]*\)\)"|#include <src/\2>|g' "${file}"

    # 4. Migrate relative tests/unity/ includes
    sed -i 's|#include "\([^"]*tests/unity/\([^"]*\)\)"|#include <unity/\2>|g' "${file}"

    # 5. Migrate relative project includes (no ../ prefix)
    sed -i 's|#include "\([a-zA-Z_][a-zA-Z0-9_]*\)/\([a-zA-Z_][a-zA-Z0-9_]*\.h\)"|#include <src/\1/\2>|g' "${file}"

    # 5. Fix cmake-style includes missing src/ prefix (terminal)
    sed -i 's|#include <terminal/|#include <src/terminal/|g' "${file}"

    # 6. Fix relative includes (general pattern)
    sed -i 's|#include "\.\./\([a-zA-Z_][a-zA-Z0-9_]*\)/\([^"]*\)"|#include <src/\1/\2>|g' "${file}"

    log_info "Successfully migrated includes in: ${file}"
    return 0
}

# Function to process a directory recursively
process_directory() {
    local dir="$1"
    local processed_count=0
    local error_count=0

    # Skip if directory doesn't exist
    if [[ ! -d "${dir}" ]]; then
        log_error "Directory not found: ${dir}"
        return 1
    fi

    log_info "Processing directory: ${dir}"

    # Find all C/C++ files and process them
    while IFS= read -r -d '' file; do
        if migrate_file "${file}"; then
            processed_count=$(( processed_count + 1 ))
        else
            error_count=$(( error_count + 1 ))
        fi
    done < <(find "${dir}" -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) -print0 2>/dev/null || true)

    log_info "Directory processing complete. Processed: ${processed_count} files, Errors: ${error_count}"
    return "${error_count}"
}

# Main script logic
main() {
    # Check if argument provided
    if [[ $# -eq 0 ]]; then
        print_usage
    fi

    local target="$1"

    # Check if it's a file or directory
    if [[ -f "${target}" ]]; then
        migrate_file "${target}"
    elif [[ -d "${target}" ]]; then
        process_directory "${target}"
    else
        log_error "Target does not exist: ${target}"
        print_usage
    fi
}

# Run main function with all arguments
main "$@"