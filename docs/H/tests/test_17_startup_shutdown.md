# Test 22 Startup Shutdown Script Documentation

## Overview

The `test_22_startup_shutdown.sh` script (Test 15: Startup/Shutdown) is a comprehensive testing tool within the Hydrogen test suite that validates the complete startup and shutdown lifecycle of the Hydrogen server with both minimal and maximal configurations. This test ensures robust lifecycle management across different configuration scenarios.

## Purpose

This script ensures that the core lifecycle management of the Hydrogen server functions correctly by testing the initialization and termination processes under different configuration scenarios. It validates that the server can start up successfully, operate properly, and shut down cleanly with both simple and complex configurations.

## Key Features

- **Dual Configuration Testing**: Tests both minimal and maximal configurations in sequence
- **Complete Lifecycle Validation**: Tests full startup and shutdown cycles for each configuration
- **Comprehensive Subtest Coverage**: Reports on 9 subtests total (3 setup + 3 per configuration)
- **Timeout Management**: Implements proper timeout handling for startup and shutdown phases
- **Diagnostic Collection**: Captures detailed diagnostics during lifecycle operations
- **Library-Based Architecture**: Uses new modular library system for enhanced reliability

## Technical Details

- **Script Version**: 3.0.0 (Complete rewrite using new modular test libraries)
- **Test Number**: 22 (internally referenced as Test 15)
- **Total Subtests**: 9
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `lifecycle.sh` - Server lifecycle management

### Configuration Files

- **Minimal Config**: `configs/hydrogen_test_min.json` - Basic configuration for minimal testing
- **Maximal Config**: `configs/hydrogen_test_max.json` - Full-featured configuration for comprehensive testing

### Timeout Configuration

- **Startup Timeout**: 10 seconds
- **Shutdown Timeout**: 90 seconds (hard limit)
- **Shutdown Activity Timeout**: 5 seconds (no new log activity)

## Test Process

1. **Binary Validation**: Locates and validates the Hydrogen binary
2. **Configuration Validation**: Validates both minimal and maximal configuration files
3. **Directory Setup**: Creates output and diagnostic directories
4. **Minimal Configuration Test**: Runs complete lifecycle test with minimal configuration
5. **Maximal Configuration Test**: Runs complete lifecycle test with maximal configuration
6. **Results Export**: Exports results for integration with the main test suite

### Per-Configuration Testing

Each configuration test includes:

- **Startup Validation**: Verifies successful server startup
- **Operation Verification**: Confirms server is running and responsive
- **Shutdown Testing**: Validates clean shutdown process
- **Resource Cleanup**: Ensures proper resource cleanup after shutdown

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_22_startup_shutdown.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Log File**: `tests/logs/hydrogen_test.log`
- **Diagnostics**: `tests/diagnostics/startup_shutdown_test_[timestamp]/`
- **Result Log**: `tests/results/test_22_[timestamp].log`

## Diagnostic Information

The test captures comprehensive diagnostic information including:

- **Thread States**: Analysis of server thread status during lifecycle operations
- **Stack Traces**: Stack trace information for debugging purposes
- **Open File Descriptors**: File descriptor usage analysis
- **Resource Usage**: Memory and CPU usage monitoring
- **Shutdown Sequence Analysis**: Detailed analysis of shutdown process timing and completeness

## Version History

- **3.0.0** (2025-07-02): Complete rewrite to use new modular test libraries
- **2.0.0** (2025-06-17): Major refactoring with shellcheck fixes, improved modularity, enhanced comments
- **1.0.0**: Initial version with basic startup/shutdown testing

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test orchestration system
- [test_16_shutdown.md](/docs/H/tests/test_16_shutdown.md) - Related shutdown-focused testing
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
