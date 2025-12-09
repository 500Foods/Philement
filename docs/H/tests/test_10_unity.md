# Test 11: Unity Script Documentation

## Overview

The `test_11_unity.sh` script is a comprehensive unit testing tool within the Hydrogen test suite, focused on running unit tests for the Hydrogen project using the Unity testing framework with automatic framework management and detailed subtest reporting.

## Purpose

This script validates the functionality of core components of the Hydrogen server through unit tests written in C, ensuring that individual units of code work as expected before integration testing. It provides automated Unity framework management, compilation, and execution with detailed reporting for maintaining code quality and reliability at a granular level.

## Script Details

- **Script Name**: `test_11_unity.sh`
- **Test Name**: Unity
- **Version**: 2.0.0 (Complete rewrite using modular test libraries)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Automatic Unity Framework Management**: Downloads Unity framework automatically if not present
- **CMake Integration**: Uses CMake for building Unity tests with proper configuration
- **Individual Test Execution**: Runs each Unity test as a separate subtest using ctest
- **Dynamic Test Discovery**: Automatically enumerates available Unity tests
- **Comprehensive Logging**: Detailed logging with path normalization for readability

### Advanced Testing Capabilities

- **Framework Auto-Download**: Supports both curl and git for Unity framework acquisition
- **Build Management**: Creates and manages separate build directory for Unity tests
- **Test Enumeration**: Uses ctest to discover and list available tests
- **Individual Test Reporting**: Each Unity test reported as separate subtest
- **Output Processing**: Normalizes paths and formats output for clarity
- **Diagnostic Preservation**: Preserves build logs and test outputs for analysis

### Output Management

- **Results Directory**: Organized results in `tests/results/`
- **Diagnostic Directory**: Detailed diagnostics in `tests/diagnostics/`
- **Log Files**: Comprehensive logging in `tests/logs/unity_tests.log`
- **Timestamped Output**: All results include timestamp for tracking

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking
- `lifecycle.sh` - Process lifecycle management

### Unity Framework Management

- **Download Location**: `tests/unity/framework/Unity/`
- **Download Methods**:
  - Primary: curl with ZIP download from GitHub
  - Fallback: git clone from GitHub repository
- **Automatic Extraction**: Handles ZIP extraction and directory organization

### Build Process

- **Build Directory**: `build_unity_tests/` in Hydrogen root
- **CMake Configuration**: Uses CMake from Unity directory
- **Compilation**: Full build process with error handling
- **Test Discovery**: Uses ctest to enumerate available tests

### Test Execution Sequence

1. **Output Directory Setup**: Creates results, diagnostics, and log directories
2. **Unity Framework Check**: Downloads Unity framework if missing
3. **Compilation**: Builds Unity tests using CMake
4. **Test Enumeration**: Discovers available Unity tests using ctest
5. **Individual Test Execution**: Runs each test separately with detailed reporting

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_11_unity.sh
```

## Expected Results

### Successful Test Criteria

- Output directories created successfully
- Unity framework available (downloaded if necessary)
- Unity tests compile without errors
- All individual Unity tests pass
- Comprehensive logging and diagnostics available

### Test Reporting

- **Dynamic Subtest Count**: Total subtests calculated based on available Unity tests
- **Individual Test Results**: Each Unity test reported as separate subtest
- **Pass/Fail Tracking**: Detailed tracking of individual test results
- **Comprehensive Summary**: Final results table with complete statistics

## Troubleshooting

### Common Issues

- **Framework Download**: Requires curl or git for automatic Unity framework download
- **Build Dependencies**: Requires CMake and appropriate C compiler
- **Test Compilation**: Check build logs in diagnostics directory for compilation errors
- **Test Failures**: Individual test logs preserved for analysis

### Diagnostic Information

- **Build Logs**: CMake configuration and compilation logs in diagnostics
- **Test Output**: Individual test execution logs with normalized paths
- **Unity Framework**: Automatic download and setup status
- **Error Preservation**: Failed builds and tests preserve logs for debugging

## Unity Framework Integration

### Automatic Download

- **GitHub Source**: Downloads from ThrowTheSwitch/Unity repository
- **ZIP Method**: Downloads master branch ZIP and extracts
- **Git Method**: Clones repository as fallback
- **Directory Structure**: Maintains proper Unity framework structure

### CMake Integration

- **Configuration**: Uses Unity's CMake configuration
- **Build Process**: Standard CMake build workflow
- **Test Discovery**: Leverages ctest for test enumeration and execution

## Version History

- **2.0.0** (2025-07-02): Complete rewrite using modular test libraries
- **1.0.0** (2025-06-25): Initial version for running Unity tests

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test suite documentation
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Modular library documentation
- [Unity Framework README](/elements/001-hydrogen/hydrogen/tests/unity/README.md) - Unity testing framework information
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
- [coding_guidelines.md](/docs/H/core/coding_guidelines.md) - Code quality standards