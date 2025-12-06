# Network Utilities Library Documentation

## Overview

The Network Utilities Library (`network_utils.sh`) provides network-related functions for test scripts, with a focus on TIME_WAIT socket management and port availability checking. This library was created as part of the test suite migration to replace functionality from `support_timewait.sh` and provide robust network testing capabilities.

## Library Information

- **Script**: `lib/network_utils.sh`
- **Version**: 2.0.0
- **Created**: 2025-07-02
- **Purpose**: Network operations and TIME_WAIT socket management for test scripts

## Core Functions

### Port Management Functions

#### `check_port_in_use(port)`

Checks if a specific port is currently in use by any process.

**Parameters:**

- `port` - The port number to check

**Returns:**

- `0` if port is in use
- `1` if port is available or on error

**Example:**

```bash
if check_port_in_use "8080"; then
    echo "Port 8080 is in use"
else
    echo "Port 8080 is available"
fi
```

#### `kill_processes_on_port(port)`

Attempts to kill processes using a specific port.

**Parameters:**

- `port` - The port number to free up

**Returns:**

- `0` on success (whether processes were found or not)

**Example:**

```bash
kill_processes_on_port "8080"
```

### TIME_WAIT Socket Functions

#### `count_time_wait_sockets(port)`

Counts the number of TIME_WAIT sockets for a specific port.

**Parameters:**

- `port` - The port number to check

**Returns:**

- Outputs the count to stdout
- Returns `0` on success, `1` on error

**Example:**

```bash
count=$(count_time_wait_sockets "8080")
echo "Found $count TIME_WAIT sockets"
```

#### `check_time_wait_sockets(port)`

Displays information about TIME_WAIT sockets for a specific port.

**Parameters:**

- `port` - The port number to check

**Returns:**

- `0` on success, `1` on error

**Example:**

```bash
check_time_wait_sockets "8080"
```

#### `wait_for_time_wait_clear(port, max_wait_seconds, check_interval)`

Waits for TIME_WAIT sockets to clear for a specific port.

**Parameters:**

- `port` - The port number to monitor
- `max_wait_seconds` - Maximum time to wait (default: 60)
- `check_interval` - How often to check in seconds (default: 2)

**Returns:**

- `0` if TIME_WAIT sockets cleared
- `1` if timeout reached

**Example:**

```bash
if wait_for_time_wait_clear "8080" 30 1; then
    echo "TIME_WAIT sockets cleared"
else
    echo "Timeout waiting for TIME_WAIT to clear"
fi
```

### Port Availability Functions

#### `wait_for_port_available(port, max_wait_seconds, check_interval)`

Waits for a port to become completely available (no active connections or TIME_WAIT).

**Parameters:**

- `port` - The port number to monitor
- `max_wait_seconds` - Maximum time to wait (default: 60)
- `check_interval` - How often to check in seconds (default: 2)

**Returns:**

- `0` if port becomes available
- `1` if timeout reached

**Example:**

```bash
if wait_for_port_available "8080" 45 2; then
    echo "Port is now available"
else
    echo "Port did not become available in time"
fi
```

### Environment Preparation Functions

#### `prepare_test_environment_with_time_wait(port, max_wait_seconds)`

Comprehensive function to prepare a test environment with TIME_WAIT handling.

**Parameters:**

- `port` - The port number to prepare
- `max_wait_seconds` - Maximum time to wait for TIME_WAIT to clear (default: 90)

**Returns:**

- `0` on success
- `1` on failure

**Features:**

- Kills active processes on the port
- Waits for TIME_WAIT sockets to clear
- Provides detailed status information
- Handles edge cases gracefully

**Example:**

```bash
if prepare_test_environment_with_time_wait "8080" 60; then
    echo "Environment ready for testing"
else
    echo "Failed to prepare environment"
fi
```

### HTTP Testing Functions

#### `make_http_requests(base_url, results_dir, timestamp)`

Makes HTTP requests to create active connections for testing TIME_WAIT scenarios.

**Parameters:**

- `base_url` - Base URL for requests (e.g., "<http://localhost:8080>")
- `results_dir` - Directory to store response files
- `timestamp` - Timestamp string for unique filenames

**Returns:**

- `0` on success

**Features:**

- Makes requests to common web files (index.html, favicon.ico, etc.)
- Creates multiple connections to simulate real usage
- Stores responses for debugging
- Handles timeouts gracefully

**Example:**

```bash
make_http_requests "http://localhost:8080" "/tmp/results" "20250702_143000"
```

## Tool Dependencies

The library automatically detects and uses available network tools:

### Primary Tools (preferred)

- **`ss`** - Modern socket statistics tool (part of iproute2)
- **`lsof`** - List open files and processes

### Fallback Tools

- **`netstat`** - Legacy network statistics tool
- **`fuser`** - Identify processes using files/sockets

## Integration with Test Framework

The Network Utilities Library integrates seamlessly with other test framework libraries:

### Log Output Integration

All functions use the log output library functions when available:

- `print_message()` for informational output
- `print_warning()` for warnings
- `print_error()` for errors
- `print_output()` for data display

### Error Handling

Functions gracefully handle missing tools and provide appropriate fallbacks or error messages.

## Usage Patterns

### Basic Port Checking

```bash
# Simple port availability check
if check_port_in_use "$PORT"; then
    echo "Port is busy"
    kill_processes_on_port "$PORT"
fi
```

### TIME_WAIT Management

```bash
# Check and display TIME_WAIT information
time_wait_count=$(count_time_wait_sockets "$PORT")
if [ "$time_wait_count" -gt 0 ]; then
    echo "Found $time_wait_count TIME_WAIT sockets"
    check_time_wait_sockets "$PORT"  # Display details
fi
```

### Comprehensive Environment Setup

```bash
# Full environment preparation
if prepare_test_environment_with_time_wait "$PORT" 90; then
    echo "Ready to start test server"
else
    echo "Environment preparation failed"
    exit 1
fi
```

### HTTP Connection Testing

```bash
# Create connections for TIME_WAIT testing
BASE_URL="http://localhost:$PORT"
RESULTS_DIR="/tmp/test_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

make_http_requests "$BASE_URL" "$RESULTS_DIR" "$TIMESTAMP"
```

## Best Practices

### SO_REUSEADDR Testing

When testing SO_REUSEADDR functionality:

1. Use `make_http_requests()` to create realistic connections
2. Check TIME_WAIT status with `check_time_wait_sockets()`
3. Don't wait for TIME_WAIT to clear - SO_REUSEADDR should handle it
4. Use informational checks rather than blocking waits

### Port Cleanup

For test cleanup:

1. Always use `kill_processes_on_port()` for active processes
2. Use `check_port_in_use()` to verify cleanup success
3. Consider TIME_WAIT sockets as informational only

### Network Error Handling

1. Check return codes from all functions
2. Provide fallback behavior when tools are unavailable
3. Use appropriate log levels for different scenarios

## Common Use Cases

### Test 55 - Socket Rebinding

The library is extensively used in `test_55_socket_rebind.sh` for:

- Environment preparation without TIME_WAIT waiting
- Creating realistic HTTP connections
- Monitoring TIME_WAIT socket status
- Testing immediate socket rebinding with SO_REUSEADDR

### General Network Testing

- Port availability verification before starting services
- Cleanup of test environments between runs
- Monitoring network state during tests
- Creating realistic network load scenarios

## Version History

- **2.0.0** (2025-07-02) - Initial creation from support_timewait.sh migration for test_55_socket_rebind.sh
  - Modular design with focused functionality
  - Integration with test framework libraries
  - Comprehensive TIME_WAIT socket management
  - HTTP request generation for testing
  - Robust error handling and tool detection

## Related Documentation

- [Test Framework Library](framework.md) - Test lifecycle management
- [Log Output Library](log_output.md) - Logging and output formatting
- [Lifecycle Management Library](lifecycle.md) - Application lifecycle management
