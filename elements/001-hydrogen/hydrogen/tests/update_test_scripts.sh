#!/bin/bash
#
# Script to update all test scripts to pass TEST_NAME to export_subtest_results
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Find all test scripts excluding test_00_all.sh and test_template.sh
TEST_SCRIPTS=$(find "$SCRIPT_DIR" -name "test_*.sh" -type f -not -name "test_00_all.sh" -not -name "test_template.sh")

# Loop through each test script and update the export_subtest_results call
for script in $TEST_SCRIPTS; do
    echo "Updating $script..."
    # Check if the script already passes TEST_NAME
    if grep -q "export_subtest_results.*\"\$TEST_NAME\"" "$script"; then
        echo "  - Already updated, skipping."
    else
        # Replace the export_subtest_results line to include TEST_NAME
        sed -i 's/export_subtest_results "\${TEST_NUMBER}_\${TEST_IDENTIFIER}" "\$TOTAL_SUBTESTS" "\$PASS_COUNT" > \/dev\/null/export_subtest_results "\${TEST_NUMBER}_\${TEST_IDENTIFIER}" "\$TOTAL_SUBTESTS" "\$PASS_COUNT" "\$TEST_NAME" > \/dev\/null/' "$script"
        if [ $? -eq 0 ]; then
            echo "  - Updated successfully."
        else
            echo "  - Failed to update, manual check required."
        fi
    fi
done

echo "Update complete for all test scripts."
