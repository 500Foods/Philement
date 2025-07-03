#!/usr/bin/env bash

# tables_tests.sh: Script to sequentially launch all tables_test*.sh scripts in numerical order
# Version: 1.0.0
# Usage: ./tables_tests.sh

echo "Starting execution of table test scripts..."

# Find all tables_test*.sh scripts and sort them by the numerical part of the filename
# Use a while loop to handle filenames with spaces or special characters
find . -type f -name "tables_test*[0-9]*.sh" | sort -t '_' -k 3n | while read -r test_script; do
    if [[ -x "$test_script" ]]; then
        echo "Running $test_script..."
        "$test_script"
        echo "Completed $test_script"
        echo "------------------------"
    else
        echo "Warning: $test_script is not executable, skipping."
    fi
done

echo "All table test scripts have been executed."
