# Test 18 JSON Error Handling Script Documentation

## Overview

The `test_18_json_error_handling.sh` script (Test 18: JSON Error Handling) is a comprehensive testing tool within the Hydrogen test suite that validates the application's ability to correctly handle JSON syntax errors in configuration files and provide meaningful error messages with position information.

## Purpose

This script ensures that the Hydrogen server can gracefully handle malformed JSON configuration files by properly detecting syntax errors, providing detailed error messages with line and column information, and exiting appropriately rather than crashing or exhibiting undefined behavior. It is essential for verifying the robustness of configuration parsing and error reporting.

## Key Features

- **Malformed JSON Testing**: Tests the server's response to invalid or malformed JSON configurations (specifically missing comma syntax errors)
- **Error Detection Validation**: Verifies that the application properly detects and reports JSON syntax errors
- **Position Information**: Ensures error messages include line and column information for debugging
- **Graceful Failure**: Validates that the application exits with appropriate error codes rather than crashing
- **Error Output Analysis**: Captures and analyzes error output for comprehensive validation

## Technical Details

- **Script Version**: 3.0.0 (Complete rewrite using new modular test libraries)
- **Test Number**: 18
- **Total Subtests**: 4
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `lifecycle.sh` - Server lifecycle management

### Test Configuration

- **Test Config**: `hydrogen_test_json.json` (contains intentional JSON syntax error - missing comma)
- **Error Output Capture**: Both stdout and stderr are captured for analysis
- **Position Validation**: Checks for "line" and "column" keywords in error messages

## Test Process

1. **Binary Validation**: Locates and validates the Hydrogen binary
2. **Configuration Validation**: Validates the test configuration file with intentional JSON errors
3. **Launch Test**: Attempts to launch Hydrogen with malformed JSON configuration
4. **Error Detection**: Verifies that the application exits with an error as expected
5. **Error Message Analysis**: Examines error output for line and column position information
6. **Output Preservation**: Saves full error output for review and debugging

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_18_json_error_handling.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Result Log**: `tests/results/test_18_[timestamp].log`
- **Error Output**: `tests/results/json_error_output_[timestamp].txt` (temporary)
- **Full Error Log**: `tests/results/json_error_full_[timestamp].txt` (preserved)

## Expected Behavior

The test validates that when provided with malformed JSON:

1. The application **exits with an error code** (does not start successfully)
2. Error messages contain **line and column information** for debugging
3. Error output is **captured and preserved** for analysis
4. The application **fails gracefully** without crashes or undefined behavior

## Version History

- **3.0.0** (2025-07-02): Complete rewrite to use new modular test libraries
- **2.0.0** (2025-06-17): Major refactoring with shellcheck fixes, improved modularity, enhanced comments
- **1.0.0**: Original version with basic JSON error handling test

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test orchestration system
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
