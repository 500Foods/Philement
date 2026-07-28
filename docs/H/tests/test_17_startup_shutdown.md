# Test 22 Startup Shutdown Script Documentation

## Overview

The `test_17_startup_shutdown.sh` script validates the complete startup and shutdown lifecycle of the Hydrogen server with both minimal and maximal configurations, and exercises VictoriaLogs shipping against a local mock insert endpoint.

## Purpose

This script ensures that the core lifecycle management of the Hydrogen server functions correctly by testing the initialization and termination processes under different configuration scenarios. It validates that the server can start up successfully, operate properly, and shut down cleanly with both simple and complex configurations. With `VICTORIALOGS_URL` pointed at `tests/lib/mock_victoria_logs/server.js`, it also drives the production VictoriaLogs worker (queue, batch, HTTP POST) for blackbox coverage of `src/logging/victoria_logs.c`.

## Key Features

- **Dual Configuration Testing**: Tests both minimal and maximal configurations in sequence
- **Complete Lifecycle Validation**: Tests full startup and shutdown cycles for each configuration
- **VictoriaLogs mock sink**: Starts a Node HTTP sink, sets `VICTORIALOGS_*` / `K8S_*` env vars, asserts POST receipts
- **Timeout Management**: Implements proper timeout handling for startup and shutdown phases
- **Diagnostic Collection**: Captures detailed diagnostics during lifecycle operations
- **Library-Based Architecture**: Uses new modular library system for enhanced reliability

## Technical Details

- **Script Version**: 5.1.0
- **Test Number**: 17
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `lifecycle.sh` - Server lifecycle management
- `lib/mock_victoria_logs/server.js` - Mock VictoriaLogs insert endpoint (Node)

### Configuration Files

- **Minimal Config**: `configs/hydrogen_test_17_startup_min.json`
- **Maximal Config**: `configs/hydrogen_test_17_startup_max.json`

### Timeout Configuration

- **Startup Timeout**: 10 seconds
- **Shutdown Timeout**: 90 seconds (hard limit)
- **Shutdown Activity Timeout**: 5 seconds (no new log activity)

## Test Process

1. **Binary Validation**: Locates and validates the Hydrogen binary
2. **Mock VictoriaLogs**: Starts sink on ephemeral port; exports `VICTORIALOGS_URL` and K8s metadata env
3. **Configuration Validation**: Validates both minimal and maximal configuration files
4. **Minimal Configuration Test**: Lifecycle with VL shipping enabled
5. **Maximal Configuration Test**: Lifecycle with VL shipping enabled
6. **Receipt check**: Asserts the mock received at least one JSON-line POST
7. **Results Export**: Exports results for integration with the main test suite

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
./test_17_startup_shutdown.sh
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
