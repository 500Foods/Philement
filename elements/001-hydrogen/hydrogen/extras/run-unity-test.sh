#!/bin/bash

# run-unity-test.sh - Run Unity tests by name from any directory
#
# Usage: ./run-unity-test.sh <test_name>
# Example: ./run-unity-test.sh beryllium_test_analyze_gcode
#
# This script will:
# 1. Find the test source file in tests/unity/src/
# 2. Determine the build target and executable path
# 3. Build the specific test using CMake
# 4. Run the test and show results

set -e  # Exit on any error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 <test_name>"
    echo ""
    echo "Examples:"
    echo "  $0 beryllium_test_analyze_gcode"
    echo "  $0 hydrogen_test"
    echo "  $0 api_utils_test_create_jwt"
    echo ""
    echo "The script will:"
    echo "  - Find the test source file in tests/unity/src/"
    echo "  - Build the test using CMake"
    echo "  - Run the test and show results"
}

# Check if test name is provided
if [[ $# -eq 0 ]]; then
    print_error "No test name provided"
    show_usage
    exit 1
fi

TEST_NAME="$1"

# Find the project root directory (where this script is located)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Define important paths
TESTS_DIR="${PROJECT_ROOT}/tests"
UNITY_SRC_DIR="${TESTS_DIR}/unity/src"
BUILD_DIR="${PROJECT_ROOT}/build"
UNITY_BUILD_DIR="${BUILD_DIR}/unity/src"

print_info "Project root: ${PROJECT_ROOT}"
print_info "Looking for test: ${TEST_NAME}"

# Find the test source file
print_info "Searching for test source file..."
TEST_SOURCE_FILE=""
TEST_RELATIVE_PATH=""

# Search for the test file recursively
while IFS= read -r -d '' file; do
    # Extract the relative path from tests/unity/src/
    REL_PATH="${file#"${UNITY_SRC_DIR}/"}"
    #REL_DIR="$(dirname "${REL_PATH}")"
    BASENAME="$(basename "${file}" .c)"

    if [[ "${BASENAME}" == "${TEST_NAME}" ]]; then
        TEST_SOURCE_FILE="${file}"
        TEST_RELATIVE_PATH="${REL_PATH}"
        break
    fi
done < <(find "${UNITY_SRC_DIR}" -name "*.c" -print0 || true)

# Check if test file was found
if [[ -z "${TEST_SOURCE_FILE}" ]]; then
    print_error "Test source file '${TEST_NAME}.c' not found in ${UNITY_SRC_DIR}"
    print_info "Matching tests (showing tests containing '${TEST_NAME}'):"

    # Show only tests that match the search pattern
    matching_tests=$(find "${UNITY_SRC_DIR}" -name "*.c" -exec basename {} \; | sed 's/\.c$//' | grep -i "${TEST_NAME}" | sort) || true

    if [[ -z "${matching_tests}" ]]; then
        print_info "No tests found matching '${TEST_NAME}'. Here are some similar tests:"
        # Show some similar tests using fuzzy matching
        find "${UNITY_SRC_DIR}" -name "*.c" -exec basename {} \; | sed 's/\.c$//' | sort | head -20 | while read -r test; do
            echo "  - ${test}"
        done || true
    else
        echo "${matching_tests}" | while read -r test; do
            echo "  - ${test}"
        done || true
    fi

    print_info "Use 'mku <test_name>' to run a specific test"
    print_info "Use 'mku' without arguments to see this help"
    exit 1
fi

print_success "Found test source: ${TEST_SOURCE_FILE}"

# Determine the executable path
EXECUTABLE_PATH="${UNITY_BUILD_DIR}/$(dirname "${TEST_RELATIVE_PATH}")/${TEST_NAME}"

print_info "Expected executable: ${EXECUTABLE_PATH}"

# Ensure build directory exists
if [[ ! -d "${BUILD_DIR}" ]]; then
    print_error "Build directory does not exist: ${BUILD_DIR}"
    print_info "Please run the initial build first:"
    echo "  cd ${PROJECT_ROOT}"
    echo "  cmake -S . -B build"
    echo "  cmake --build build --target unity"
    exit 1
fi

# Build the specific test
print_info "Building test: ${TEST_NAME}"

# Change to project root for building
cd "${PROJECT_ROOT}"

# Build the test using make (assuming ninja or make is available)
if command -v ninja &> /dev/null && [[ -f "${BUILD_DIR}/build.ninja" ]]; then
    BUILD_CMD="ninja -C ${BUILD_DIR} ${TEST_NAME}"
elif command -v make &> /dev/null; then
    BUILD_CMD="make -C ${BUILD_DIR} ${TEST_NAME}"
else
    print_error "Neither ninja nor make found, cannot build test"
    exit 1
fi

print_info "Build command: ${BUILD_CMD}"

if ${BUILD_CMD}; then
    print_success "Test built successfully"
else
    print_error "Failed to build test: ${TEST_NAME}"
    print_info "Try a trial build first - this only has to happen once when adding a brand new test:"
    print_info "  mkt"
    print_info "After this runs cleanly, you can revert to the individual 'mku' approach"
    exit 1
fi

# Check if executable exists
if [[ ! -f "${EXECUTABLE_PATH}" ]]; then
    print_error "Executable not found after build: ${EXECUTABLE_PATH}"
    exit 1
fi

# Make executable runnable
chmod +x "${EXECUTABLE_PATH}"

# Run the test
print_info "Running test: ${TEST_NAME}"
echo "========================================"
echo "Test Output:"
echo "========================================"

# Run the test and capture exit code
set +e  # Don't exit on test failure
"${EXECUTABLE_PATH}"
TEST_EXIT_CODE=$?
set -e  # Restore exit on error

echo "========================================"

# Report results
if [[ "${TEST_EXIT_CODE}" -eq 0 ]]; then
    print_success "Test ${TEST_NAME} PASSED"
else
    print_error "Test ${TEST_NAME} FAILED (exit code: ${TEST_EXIT_CODE})"
fi

exit "${TEST_EXIT_CODE}"
