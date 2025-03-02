#!/bin/bash
#
# Simple test script to verify path conversion function
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/test_utils.sh"

# Test paths
ABSOLUTE_PATH="/home/asimard/Projects/Philement/Philement/elements/001-hydrogen/hydrogen/tests/results/test_log.log"
EXPECTED_RELATIVE="hydrogen/tests/results/test_log.log"

# Run the conversion
CONVERTED_PATH=$(convert_to_relative_path "$ABSOLUTE_PATH")

# Print results
echo "Original path: $ABSOLUTE_PATH"
echo "Converted to: $CONVERTED_PATH"
echo "Expected:     $EXPECTED_RELATIVE"

# Verify
if [ "$CONVERTED_PATH" = "$EXPECTED_RELATIVE" ]; then
    echo "✅ Path conversion successful!"
else
    echo "❌ Path conversion failed!"
    echo "Conversion didn't match expected result."
fi

# Test with more paths
TEST_PATHS=(
    "/home/asimard/Projects/Philement/Philement/elements/001-hydrogen/hydrogen/src/main.c"
    "/home/asimard/Projects/Philement/Philement/elements/001-hydrogen/hydrogen/tests/test_compilation.sh"
    "/home/asimard/Projects/Philement/Philement/elements/001-hydrogen/hydrogen/hydrogen_release"
)

echo ""
echo "Testing additional paths:"
for path in "${TEST_PATHS[@]}"; do
    CONVERTED=$(convert_to_relative_path "$path")
    echo "Original: $path"
    echo "Converted: $CONVERTED"
    echo ""
done

exit 0