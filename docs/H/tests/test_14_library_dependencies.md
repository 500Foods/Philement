# Test 16 Library Dependencies Script Documentation

## Overview

The `test_16_library_dependencies.sh` script (Test 25: Library Dependencies) is a comprehensive validation tool within the Hydrogen test suite that tests the library dependency checking system added to the Hydrogen server initialization process. This test verifies that all required library dependencies are present and properly detected during server startup.

## Purpose

This script validates the Hydrogen server's built-in dependency checking system by starting the server and analyzing the dependency check logs. It ensures that all required libraries are detected with appropriate versions and status indicators, preventing runtime issues due to missing or incompatible dependencies.

## Key Features

- **Dependency Detection Testing**: Tests the server's ability to detect and report library dependencies during initialization
- **Version Analysis**: Analyzes expected vs. found versions of dependencies
- **Status Validation**: Verifies dependency status indicators (Good, Less Good, etc.)
- **Log Analysis**: Parses dependency check logs for comprehensive validation
- **Lifecycle Testing**: Uses the new modular library system for server startup/shutdown testing

## Technical Details

- **Script Version**: 2.0.0 (Complete rewrite using new modular test libraries)
- **Test Number**: 16 (internally referenced as Test 25)
- **Total Subtests**: 13
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `lifecycle.sh` - Server lifecycle management

### Tested Library Dependencies

The test validates detection of these critical libraries:

1. **pthreads** - POSIX threads library
2. **jansson** - JSON parsing library
3. **libm** - Math library
4. **microhttpd** - HTTP server library
5. **libwebsockets** - WebSocket library
6. **OpenSSL** - Cryptographic library
7. **libbrotlidec** - Brotli decompression library
8. **libtar** - TAR archive library

## Configuration

- **Config File**: `configs/hydrogen_test_min.json` (minimal configuration for dependency testing)
- **Startup Timeout**: 10 seconds
- **Shutdown Timeout**: 10 seconds (hard limit)
- **Shutdown Activity Timeout**: 3 seconds (no new log activity)

## Test Process

1. **Binary Validation**: Locates and validates the Hydrogen binary
2. **Directory Setup**: Creates output and diagnostic directories
3. **Configuration Validation**: Validates the minimal test configuration
4. **Server Startup**: Starts Hydrogen to trigger dependency checks
5. **Log Analysis**: Parses dependency check logs for each required library
6. **Server Shutdown**: Cleanly stops the server after dependency validation
7. **Results Export**: Exports results for integration with the main test suite

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_16_library_dependencies.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Log File**: `tests/logs/library_deps.log`
- **Diagnostics**: `tests/diagnostics/library_deps_[timestamp]/`
- **Result Log**: `tests/results/test_16_[timestamp].log`

## Log Analysis

The test analyzes dependency check log entries with the format:

```log
[ STATE ]  [ DepCheck           ]  [Library] ... Expecting: [version] Found: [version] ... Status: [status]
```

Status indicators include:

- **Good**: Library found with compatible version
- **Less Good**: Library found but with version concerns
- **Other**: Potential issues requiring attention

## Version History

- **2.0.0** (2025-07-02): Complete rewrite to use new modular test libraries
- **1.1.0**: Enhanced with proper error handling, modular functions, and shellcheck compliance
- **1.0.0**: Initial version with library dependency checking

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test orchestration system
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
