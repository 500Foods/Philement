# Test 26 Shutdown Script Documentation

## Overview

The `test_26_shutdown.sh` script (Test 26: Shutdown) is a specialized testing tool within the Hydrogen test suite that validates the shutdown functionality of the Hydrogen server using a minimal configuration. This test ensures proper server lifecycle management with focus on clean shutdown behavior.

## Purpose

This script ensures that the Hydrogen server can start up successfully and then terminate cleanly, releasing all resources and processes properly during shutdown. It validates the complete lifecycle (startup and shutdown) using minimal configuration to test core shutdown functionality without complex configuration dependencies.

## Key Features

- **Lifecycle Testing**: Tests complete server lifecycle from startup through shutdown
- **Minimal Configuration**: Uses minimal configuration to focus on core shutdown functionality
- **Clean Shutdown Validation**: Ensures resources are released and processes terminate cleanly
- **Timeout Management**: Implements proper timeout handling for both startup and shutdown phases
- **Diagnostic Capture**: Captures diagnostic information during the shutdown process

## Technical Details

- **Script Version**: 2.0.1 (Complete rewrite using new modular test libraries)
- **Test Number**: 26
- **Total Subtests**: 6
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `lifecycle.sh` - Server lifecycle management

### Configuration

- **Config File**: `configs/hydrogen_test_min.json` (minimal configuration for shutdown testing)
- **Startup Timeout**: 5 seconds
- **Shutdown Timeout**: 10 seconds (hard limit)
- **Shutdown Activity Timeout**: 3 seconds (no new log activity)

## Test Process

1. **Binary Validation**: Locates and validates the Hydrogen binary
2. **Configuration Validation**: Validates the minimal test configuration file
3. **Directory Setup**: Creates output and diagnostic directories
4. **Lifecycle Test**: Runs complete startup and shutdown cycle using the lifecycle library
5. **Shutdown Validation**: Verifies clean shutdown with proper resource cleanup
6. **Results Export**: Exports results for integration with the main test suite

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_26_shutdown.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Log File**: `tests/logs/hydrogen_shutdown_test.log`
- **Diagnostics**: `tests/diagnostics/shutdown_test_[timestamp]/`
- **Result Log**: `tests/results/test_26_[timestamp].log`

## Shutdown Testing Focus

This test specifically validates:

- **Clean Startup**: Server starts successfully with minimal configuration
- **Proper Shutdown**: Server responds to shutdown signals appropriately
- **Resource Cleanup**: All resources are properly released during shutdown
- **Process Termination**: Server process terminates within expected timeouts
- **Log Analysis**: Shutdown process is properly logged and can be analyzed

## Version History

- **2.0.1** (2025-07-06): Added missing shellcheck justifications
- **2.0.0** (2025-07-02): Complete rewrite to use new modular test libraries
- **1.1.0**: Enhanced with proper error handling, modular functions, and shellcheck compliance
- **1.0.0**: Initial version with basic shutdown testing

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test orchestration system
- [test_17_startup_shutdown.md](test_17_startup_shutdown.md) - Related startup/shutdown lifecycle testing
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory