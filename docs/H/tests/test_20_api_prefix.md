# Test 29: API Prefix Script Documentation

## Overview

The `test_29_api_prefixes.sh` script is a comprehensive API testing tool within the Hydrogen test suite, focused on validating API routing with different URL prefixes and configurations using immediate restart approach with SO_REUSEADDR functionality.

## Purpose

This script ensures that the Hydrogen server correctly handles API requests with various URL prefixes, maintaining proper routing and response behavior across different configurations. It validates the server's API accessibility, configuration flexibility, and immediate restart capabilities.

## Script Details

- **Script Name**: `test_29_api_prefixes.sh`
- **Test Name**: API Prefix
- **Version**: 5.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Dual Configuration Testing**: Tests both default `/api` prefix and custom `/myapi` prefix configurations
- **Immediate Restart Testing**: Validates SO_REUSEADDR functionality with immediate server restart
- **Comprehensive API Endpoint Testing**: Tests multiple system endpoints for each configuration
- **Response Content Validation**: Verifies API responses contain expected fields and data
- **Server Lifecycle Management**: Proper startup, testing, and shutdown procedures

### API Endpoints Tested

For each prefix configuration:

- **Health Endpoint**: `/api/system/health` or `/myapi/system/health`
- **Info Endpoint**: `/api/system/info` or `/myapi/system/info`
- **Test Endpoint**: `/api/system/test` or `/myapi/system/test`

### Advanced Testing

- **Configuration File Validation**: Verifies both test configuration files exist
- **Port Management**: Extracts and manages different ports for each configuration
- **Response Content Analysis**: Validates specific fields in JSON responses
- **Server Readiness Checking**: Waits for server to be ready before testing
- **Cleanup and Preservation**: Preserves logs on failure, cleans up on success

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `framework.sh` - Test framework and result tracking
- `file_utils.sh` - File operations and path management
- `lifecycle.sh` - Process lifecycle management
- `network_utils.sh` - Network utilities and port management

### Configuration Files

- **Default Configuration**: `hydrogen_test_api_test_1.json` (uses `/api` prefix)
- **Custom Configuration**: `hydrogen_test_api_test_2.json` (uses `/myapi` prefix)

### Test Sequence (10 Subtests)

1. **Binary Location**: Locates Hydrogen binary
2. **Configuration Validation**: Verifies both configuration files exist
3. **Default Server Startup**: Starts server with default `/api` prefix
4. **Default Health Test**: Tests `/api/system/health` endpoint
5. **Default Info Test**: Tests `/api/system/info` endpoint
6. **Default Test Endpoint**: Tests `/api/system/test` endpoint
7. **Custom Server Startup**: Starts server with custom `/myapi` prefix (immediate restart)
8. **Custom Health Test**: Tests `/myapi/system/health` endpoint
9. **Custom Info Test**: Tests `/myapi/system/info` endpoint
10. **Custom Test Endpoint**: Tests `/myapi/system/test` endpoint

### Response Validation

- **Health Endpoint**: Expects "Yes, I'm alive, thanks!" message
- **Info Endpoint**: Expects "system" field in response
- **Test Endpoint**: Expects "client_ip" field in response

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_29_api_prefixes.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary located successfully
- Both configuration files found and validated
- Default server starts and responds to all three API endpoints
- Custom server starts immediately after default server shutdown
- Custom server responds to all three API endpoints with custom prefix
- All API responses contain expected content fields

### Key Validation Points

- **Prefix Routing**: Both `/api` and `/myapi` prefixes work correctly
- **Immediate Restart**: SO_REUSEADDR allows immediate server restart
- **Response Content**: API endpoints return expected data structures
- **Configuration Flexibility**: Server adapts to different prefix configurations

## Troubleshooting

### Common Issues

- **Configuration Files Missing**: Ensure both test configuration files exist
- **Port Conflicts**: Script handles port management automatically
- **Server Startup Failures**: Check logs preserved in results directory
- **API Response Validation**: Verify server is properly configured for API endpoints

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: Separate logs for default and custom configurations
- **Response Files**: API responses saved for analysis (cleaned up on success)
- **Timestamped Output**: All results include timestamp for tracking

## Configuration Requirements

### Default Configuration (`hydrogen_test_api_test_1.json`)

- Must configure API prefix as `/api`
- Must enable system endpoints (health, info, test)
- Must specify web server port

### Custom Configuration (`hydrogen_test_api_test_2.json`)

- Must configure API prefix as `/myapi`
- Must enable system endpoints (health, info, test)
- Must specify web server port (can be same as default due to SO_REUSEADDR)

## Version History

- **5.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **4.0.0** (2025-06-30): Updated to use immediate restart approach without TIME_WAIT waiting
- **3.0.0** (2025-06-30): Refactored to use robust server management functions
- **2.0.0** (2025-06-17): Major refactoring with shellcheck compliance and improved modularity

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test suite documentation
- [test_19_socket_rebind.md](/docs/H/tests/test_19_socket_rebind.md) - Socket rebinding tests (SO_REUSEADDR)
- [test_21_system_endpoints.md](/docs/H/tests/test_21_system_endpoints.md) - System endpoint testing
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Modular library documentation
- [api.md](/docs/H/core/subsystems/api/api.md) - API documentation and endpoint specifications
- [configuration.md](/docs/H/core/configuration.md) - Configuration file documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
