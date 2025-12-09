# Test 26 Terminal Script Documentation

## Overview

The `test_26_terminal.sh` script provides comprehensive testing of the Hydrogen terminal functionality, including HTTP serving, WebSocket connections, and both payload and filesystem configurations. It validates the complete terminal subsystem including xterm.js integration and WebSocket I/O operations.

## Script Information

- **Script**: `test_26_terminal.sh`
- **Purpose**: Test terminal functionality, payload serving, and WebSocket connections
- **Version**: 2.3.0
- **Dependencies**: websocat, curl, standard Unix utilities

## Key Features

- **Dual Configuration Testing**: Tests both payload mode and filesystem mode simultaneously
- **Parallel Execution**: Runs multiple terminal configurations in parallel for efficiency
- **WebSocket Testing**: Comprehensive WebSocket connection, I/O, and resize testing
- **HTTP Endpoint Validation**: Tests terminal page serving and file access
- **Authentication Testing**: Validates WebSocket key authentication
- **Retry Logic**: Robust retry mechanisms for subsystem initialization

## Test Configurations

The script tests two primary terminal configurations:

### Payload Mode

- **Configuration**: `hydrogen_test_26_terminal_payload.json`
- **Content Source**: Terminal interface served from embedded payload
- **Expected Content**: "Hydrogen Terminal"
- **WebSocket Protocol**: "terminal"

### Filesystem Mode

- **Configuration**: `hydrogen_test_26_terminal_filesystem.json`
- **Content Source**: Terminal files served from filesystem
- **Expected Content**: "HYDROGEN_TERMINAL_TEST_MARKER"
- **WebSocket Protocol**: "terminal"

## Test Execution Flow

### Prerequisites Validation

1. **Binary Location**: Finds and validates hydrogen executable
2. **Configuration Files**: Validates both test configuration files
3. **WebSocket Key**: Validates WEBSOCKET_KEY environment variable
4. **Test Artifacts**: Validates terminal test files exist

### Parallel Test Execution

1. **Server Startup**: Starts hydrogen servers for both configurations
2. **Subsystem Readiness**: Waits for HTTP and WebSocket subsystems to initialize
3. **HTTP Testing**: Tests terminal page access and content validation
4. **WebSocket Testing**: Tests connection, ping, I/O, and resize operations
5. **Server Shutdown**: Graceful termination with timeout handling

### Result Analysis

1. **Individual Test Results**: Analyzes each test component
2. **Cross-Configuration Testing**: Validates proper file isolation
3. **Summary Reporting**: Provides comprehensive test results

## WebSocket Test Functions

### Connection Testing

- **Function**: `test_websocket_terminal_connection()`
- **Purpose**: Validates basic WebSocket connectivity with authentication
- **Protocol**: Tests protocol acceptance and connection establishment
- **Authentication**: Uses WEBSOCKET_KEY for secure connections

### Status Testing

- **Function**: `test_websocket_terminal_status()`
- **Purpose**: Tests WebSocket ping/status functionality
- **Message**: Sends `{"type": "ping"}` and validates response
- **Timeout**: 3-second timeout for responsiveness

### I/O Testing

- **Function**: `test_websocket_terminal_input_output()`
- **Purpose**: Tests terminal input/output processing
- **Commands**: Sends multiple shell commands (echo, pwd, date)
- **Coverage**: Exercises `terminal_websocket.c` and `terminal_shell.c`

### Resize Testing

- **Function**: `test_websocket_terminal_resize()`
- **Purpose**: Tests terminal resize functionality
- **Message**: Sends `{"type": "resize", "rows": 30, "cols": 100}`
- **Coverage**: Exercises terminal resize handling

## HTTP Test Functions

### Content Validation

- **Function**: `check_terminal_response_content()`
- **Purpose**: Validates HTTP responses contain expected content
- **Retry Logic**: Robust retry mechanism for subsystem readiness
- **Timeout**: 10-second timeout with 25 retry attempts

### Endpoint Testing

- **Index Page**: Tests `/terminal/` endpoint accessibility
- **Specific Files**: Tests configuration-specific file access
- **Cross-Config**: Validates proper file isolation between modes

## Configuration Requirements

### Environment Variables

- **WEBSOCKET_KEY**: Required for WebSocket authentication
- **PATH**: Must include websocat and curl

### Test Artifacts

- **tests/artifacts/terminal/index.html**: Filesystem mode test file
- **tests/artifacts/terminal/xterm-test.html**: Cross-config test file

### Port Configuration

- **HTTP Port**: Configurable via JSON (default varies)
- **WebSocket Port**: Configurable via JSON (default 5261)

## Error Handling

### Startup Failures

- Validates server startup within timeout period
- Checks for "STARTUP COMPLETE" in server logs
- Handles server startup failures gracefully

### Connection Issues

- Retry logic for HTTP endpoint availability
- WebSocket connection retry with backoff
- Authentication error detection and reporting

### Test Timeouts

- HTTP requests: 10-second timeout
- WebSocket operations: 5-second timeout for connections, 3-second for ping
- Server shutdown: 15-second graceful shutdown period

## Integration with Test Suite

Part of the comprehensive Hydrogen test suite:

- **Test Group**: 20s (Server functionality tests)
- **Dependencies**: Requires successful compilation and basic server tests
- **Parallel Execution**: Can run simultaneously with other server tests
- **Result Tracking**: Integrates with test_00_all.sh result aggregation

## Performance Considerations

### Parallel Execution

- **Job Limiting**: Limits concurrent jobs to available CPU cores
- **Resource Management**: Proper cleanup of background processes
- **SO_REUSEADDR**: Enables immediate port reuse between tests

### Timeout Management

- **Subsystem Readiness**: Waits for services to initialize
- **Graceful Degradation**: Continues testing even with partial failures
- **Resource Cleanup**: Ensures proper server termination

## Troubleshooting

### Common Issues

#### WebSocket Connection Failures

- Verify WEBSOCKET_KEY is set and valid
- Check WebSocket port configuration
- Ensure websocat is installed and accessible

#### HTTP 404 Errors

- Confirm test artifacts exist in correct locations
- Verify server startup completed successfully
- Check HTTP port configuration

#### Timeout Issues

- Increase timeout values for slower systems
- Check system resource availability
- Verify network connectivity

#### Authentication Errors

- Validate WEBSOCKET_KEY format
- Check server WebSocket configuration
- Verify protocol support

## Related Documentation

- [Test 22 Swagger](test_22_swagger.md) - Similar parallel testing pattern
- [Test 23 WebSockets](test_23_websockets.md) - General WebSocket testing
- [Framework Library](framework.md) - Test framework utilities
- [Terminal Architecture](/docs/H/core/reference/terminal_architecture.md) - Terminal subsystem architecture and implementation