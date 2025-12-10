# Test 20 Signals Script Documentation

## Overview

The `test_20_signals.sh` script is a comprehensive signal handling test tool within the Hydrogen test suite that validates the server's response to various system signals. This test ensures robust signal handling capabilities including shutdown, restart, configuration dump, and multiple signal scenarios.

## Purpose

This script ensures that the Hydrogen server handles system signals appropriately, performing actions such as clean shutdown, graceful restart, configuration dumping, and proper handling of multiple simultaneous signals. It is essential for verifying the server's robustness and proper behavior under different signal conditions in production environments.

## Key Features

- **Comprehensive Signal Testing**: Tests response to multiple system signals with detailed validation
- **SIGINT Handling**: Tests interrupt signal (Ctrl+C) for clean shutdown
- **SIGTERM Handling**: Tests termination signal for clean shutdown
- **SIGHUP Handling**: Tests hangup signal for graceful restart with configuration reload
- **SIGUSR2 Handling**: Tests user-defined signal for configuration dump
- **Multiple Signal Testing**: Ensures proper handling when multiple signals are received simultaneously
- **Restart Validation**: Tests multiple consecutive restarts to verify restart reliability
- **Performance Optimized**: Removed artificial delays for dramatically faster execution while maintaining reliability

### Dependencies

The script uses several modular libraries:

- `framework.sh` - Core testing framework
- `log_output.sh` - Logging and output formatting
- `lifecycle.sh` - Server lifecycle management
- `file_utils.sh` - File operations and validation

### Signal Test Configuration

- **Restart Count**: 5 (number of SIGHUP restarts to test)
- **Startup Timeout**: 5 seconds
- **Shutdown Timeout**: 10 seconds
- **Signal Timeout**: 15 seconds (for signal response validation)

## Test Cases

### 1. SIGINT Signal Handling

Tests interrupt signal (Ctrl+C) handling:

- Starts server with minimal configuration
- Sends SIGINT signal
- Validates clean shutdown with proper log messages

### 2. SIGTERM Signal Handling

Tests termination signal handling:

- Starts server with minimal configuration
- Sends SIGTERM signal
- Validates clean shutdown with proper log messages

### 3. SIGHUP Signal Handling (Single Restart)

Tests hangup signal for graceful restart:

- Starts server with minimal configuration
- Sends SIGHUP signal
- Validates restart completion with configuration reload

### 4. Multiple SIGHUP Restarts

Tests multiple consecutive restarts:

- Performs 5 consecutive SIGHUP restarts
- Validates each restart completion
- Ensures restart counter increments properly

### 5. SIGUSR2 Signal Handling (Config Dump)

Tests user-defined signal for configuration dump:

- Starts server with minimal configuration
- Sends SIGUSR2 signal
- Validates configuration dump output in logs

### 6. Multiple Signal Handling

Tests simultaneous signal handling:

- Sends multiple signals (SIGTERM and SIGINT) simultaneously
- Validates single shutdown sequence (no duplicate shutdowns)
- Ensures proper signal prioritization

### 7. Signal Handling Verification

Comprehensive verification of all signal tests:

- Analyzes log files from all signal tests
- Validates completion messages in each test
- Reports overall signal handling success rate

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_24_signals.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Individual Signal Logs**:
  - `tests/logs/hydrogen_signal_test_SIGINT_[timestamp].log`
  - `tests/logs/hydrogen_signal_test_SIGTERM_[timestamp].log`
  - `tests/logs/hydrogen_signal_test_SIGHUP_[timestamp].log`
  - `tests/logs/hydrogen_signal_test_SIGUSR2_[timestamp].log`
  - `tests/logs/hydrogen_signal_test_MULTI_[timestamp].log`
- **Archived Logs**: All signal test logs are copied to results directory for analysis
- **Result Log**: `tests/results/test_24_[timestamp].log`

## Expected Signal Behaviors

- **SIGINT/SIGTERM**: Clean shutdown with "Shutdown complete" message
- **SIGHUP**: Graceful restart with "Restart completed successfully" message
- **SIGUSR2**: Configuration dump with "APPCONFIG Dump Complete" message
- **Multiple Signals**: Single shutdown sequence, no duplicate processing

## Version History

- **3.0.1** (2025-07-02): Performance optimization - removed all artificial delays for dramatically faster execution while maintaining reliability
- **3.0.0** (2025-07-02): Complete rewrite to use new modular test libraries
- **2.0.0** (2025-06-17): Major refactoring with shellcheck fixes, improved modularity, enhanced comments
- **1.0.0**: Original version with basic signal handling test

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test orchestration system
- [test_16_shutdown.md](/docs/H/tests/test_16_shutdown.md) - Related shutdown testing
- [test_17_startup_shutdown.md](/docs/H/tests/test_17_startup_shutdown.md) - Related lifecycle testing
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
