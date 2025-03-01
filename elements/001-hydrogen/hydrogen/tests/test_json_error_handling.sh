#!/bin/bash
# Test script for configuration JSON error handling
# This script tests if the hydrogen application handles JSON syntax errors correctly

# Set strict error handling
set -e

# Setup
TEST_DIR=$(dirname "$0")
cd "$TEST_DIR/.."
echo "Testing JSON error handling in configuration module..."

# Use the permanent test file with a JSON syntax error (missing comma)
TEST_CONFIG="tests/hydrogen_test_json.json"
echo "Using test file with JSON syntax error at line 3 (missing comma)"

# Determine which hydrogen build to use (prefer release build if available)
if [ -f "./hydrogen_release" ]; then
    HYDROGEN_BIN="./hydrogen_release"
    echo "Using release build for testing"
else
    HYDROGEN_BIN="./hydrogen"
    echo "Release build not found, using standard build"
fi

# Run hydrogen with the malformed config and capture output
echo "Running hydrogen with malformed config..."
if $HYDROGEN_BIN "$TEST_CONFIG" 2> tests/json_error_output.txt; then
    echo "FAIL: Hydrogen should have exited with an error but didn't"
    rm -f tests/json_error_output.txt
    exit 1
else
    echo "Hydrogen exited with error as expected"
fi

# Check if the error output contains line and column information
if grep -q "line" tests/json_error_output.txt && grep -q "column" tests/json_error_output.txt; then
    echo "PASS: Error message contains line and column information"
    cat tests/json_error_output.txt
else
    echo "FAIL: Error message does not contain line and column information"
    cat tests/json_error_output.txt
    rm -f tests/json_error_output.txt
    exit 1
fi

# Clean up
rm -f tests/json_error_output.txt
echo "Test completed successfully!"
exit 0