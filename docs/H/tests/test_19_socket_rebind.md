# Test 28: Socket Rebinding Script Documentation

## Overview

The `test_28_socket_rebind.sh` script is a comprehensive network testing tool within the Hydrogen test suite, focused on validating the `SO_REUSEADDR` socket option functionality and immediate port rebinding capabilities after shutdown with active HTTP connections.

## Purpose

This script ensures that the Hydrogen server can properly manage socket binding and immediately reuse ports after shutdown, even when TIME_WAIT sockets exist. It validates the server's network stability and recovery capabilities by testing realistic scenarios with active HTTP connections that create TIME_WAIT conditions.

## Script Details

- **Script Name**: `test_28_socket_rebind.sh`
- **Test Name**: Socket Rebinding
- **Version**: 2.0.0 (Major refactoring with shellcheck compliance)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **SO_REUSEADDR Testing**: Validates that the socket option allows immediate rebinding after shutdown
- **TIME_WAIT Handling**: Tests proper handling of TIME_WAIT sockets from previous connections
- **Active Connection Simulation**: Creates realistic HTTP connections before shutdown to generate TIME_WAIT states
- **Immediate Rebinding**: Tests that a second instance can start immediately after the first shuts down
- **Port Conflict Resolution**: Handles existing processes on the test port

### Advanced Testing

- **HTTP Request Generation**: Makes actual HTTP requests to create proper TIME_WAIT conditions
- **Process Lifecycle Management**: Uses lifecycle.sh for proper startup/shutdown procedures
- **Network State Monitoring**: Monitors port usage and TIME_WAIT socket counts
- **Cleanup and Recovery**: Comprehensive cleanup of test processes and resources
- **Diagnostic Logging**: Detailed logging of network states and process activities

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `framework.sh` - Test framework and result tracking
- `file_utils.sh` - File operations and path management
- `lifecycle.sh` - Process lifecycle management
- `network_utils.sh` - Network utilities and port management

### Test Sequence (7 Subtests)

1. **Binary and Configuration Discovery**: Locates Hydrogen binary and appropriate configuration
2. **Environment Preparation**: Checks current port status and cleans up if necessary
3. **First Instance Startup**: Starts initial Hydrogen server instance
4. **HTTP Connection Creation**: Makes HTTP requests to establish active connections
5. **First Instance Shutdown**: Gracefully shuts down the first instance
6. **TIME_WAIT Socket Check**: Monitors TIME_WAIT sockets created by active connections
7. **Immediate Rebinding Test**: Starts second instance immediately to test SO_REUSEADDR

### Configuration Requirements

- Uses `hydrogen_test_api.json` (preferred) or `hydrogen_test_max.json` as fallback
- Extracts web server port from configuration for testing
- Requires HTTP API endpoints to be available for connection testing

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_28_socket_rebind.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary and configuration found
- Test environment prepared successfully
- First instance starts and binds to port
- HTTP requests create active connections
- First instance shuts down gracefully
- TIME_WAIT sockets detected (if connections were active)
- Second instance starts immediately without port binding errors

### Key Validation Points

- **SO_REUSEADDR Effectiveness**: Second instance can bind immediately despite TIME_WAIT sockets
- **Network State Handling**: Proper detection and handling of existing port usage
- **Process Management**: Clean startup and shutdown of test instances
- **Connection Simulation**: Realistic HTTP traffic generation

## Troubleshooting

### Common Issues

- **Port Already in Use**: Script automatically detects and cleans up existing processes
- **TIME_WAIT Persistence**: SO_REUSEADDR should handle TIME_WAIT sockets automatically
- **Configuration Missing**: Falls back to alternative configuration files
- **HTTP Request Failures**: May indicate server startup issues or configuration problems

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Instance Logs**: Separate logs for first and second instances
- **Network Diagnostics**: TIME_WAIT socket counts and port status
- **Process Cleanup**: Automatic cleanup on script termination

## Version History

- **4.0.0** (2025-07-02): Migrated to use lib/ scripts following established patterns
- **3.0.0** (2025-06-30): Enhanced with actual HTTP requests for realistic TIME_WAIT testing
- **2.0.0** (2025-06-17): Major refactoring with shellcheck compliance and improved modularity
- **1.0.0**: Original basic socket rebinding test functionality

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_13_crash_handler.md](test_13_crash_handler.md) - Crash handler testing
- [test_18_signals.md](test_18_signals.md) - Signal handling tests
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [testing.md](../../docs/testing.md) - Overall testing strategy
- [api.md](../../docs/api.md) - API documentation for HTTP endpoints
