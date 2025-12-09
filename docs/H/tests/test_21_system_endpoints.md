# Test 32: System API Endpoints Script Documentation

## Overview

The `test_32_system_endpoints.sh` script is a comprehensive system API testing tool within the Hydrogen test suite, focused on validating all system API endpoints functionality, including health monitoring, configuration access, Prometheus metrics, and various request handling scenarios.

## Purpose

This script ensures that the Hydrogen server correctly exposes and handles all system API endpoints, which are critical for system monitoring, management, and integration. It validates the server's operational status, configuration accessibility, metrics reporting, and request processing capabilities across different HTTP methods and data formats.

## Script Details

- **Script Name**: `test_32_system_endpoints.sh`
- **Test Name**: System API Endpoints
- **Version**: 3.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core System Endpoints Tested

- **Health Endpoint** (`/api/system/health`): Server health status and availability
- **Info Endpoint** (`/api/system/info`): System information and metadata
- **Config Endpoint** (`/api/system/config`): Server configuration details
- **Prometheus Endpoint** (`/api/system/prometheus`): Metrics in Prometheus format
- **Recent Endpoint** (`/api/system/recent`): Recent activity logs
- **Appconfig Endpoint** (`/api/system/appconfig`): Application configuration
- **Test Endpoint** (`/api/system/test`): Request testing with various parameters

### Advanced Testing Capabilities

- **Multiple HTTP Methods**: Tests GET and POST requests
- **Parameter Handling**: Validates URL parameters and form data processing
- **JSON Response Validation**: Verifies JSON format and content
- **Prometheus Format Validation**: Ensures proper metrics format and required fields
- **Content Type Verification**: Validates appropriate response headers
- **Compression Testing**: Monitors Brotli compression functionality
- **Server Log Analysis**: Checks for errors and compression metrics

### Request Scenarios Tested

1. **Basic GET Requests**: Simple endpoint access
2. **GET with Parameters**: URL parameter processing
3. **POST with Form Data**: Form-encoded data handling
4. **POST with Parameters and Form Data**: Combined parameter and form data processing

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `framework.sh` - Test framework and result tracking
- `file_utils.sh` - File operations and path management
- `lifecycle.sh` - Process lifecycle management
- `network_utils.sh` - Network utilities and port management

### Configuration Requirements

- Uses `hydrogen_test_system_endpoints.json` configuration
- Requires all system endpoints to be enabled
- Must specify web server port for testing

### Test Sequence (18 Subtests)

1. **Binary Location**: Locates Hydrogen binary
2. **Configuration Validation**: Verifies configuration file exists
3. **Server Startup**: Starts Hydrogen server with system endpoints configuration
4. **Health Endpoint Test**: Tests `/api/system/health` endpoint
5. **Config Endpoint Test**: Tests `/api/system/config` endpoint
6. **Config JSON Validation**: Validates config response JSON format
7. **Info Endpoint Test**: Tests `/api/system/info` endpoint
8. **Info JSON Validation**: Validates info response JSON format
9. **Prometheus Endpoint Test**: Tests `/api/system/prometheus` endpoint
10. **Prometheus Format Validation**: Validates Prometheus metrics format
11. **System Test Basic GET**: Tests `/api/system/test` with GET request
12. **System Test GET with Parameters**: Tests GET request with URL parameters
13. **System Test POST with Form Data**: Tests POST request with form data
14. **System Test POST with Parameters and Form Data**: Tests combined POST scenario
15. **Recent Endpoint Test**: Tests `/api/system/recent` endpoint
16. **Recent Line Count Validation**: Validates recent logs contain sufficient entries
17. **Appconfig Endpoint Test**: Tests `/api/system/appconfig` endpoint
18. **Brotli Compression Check**: Analyzes server logs for compression metrics

### Response Validation Criteria

- **Health Endpoint**: Must contain "Yes, I'm alive, thanks!" message
- **Config Endpoint**: Must contain "ServerName" field and valid JSON
- **Info Endpoint**: Must contain "system" field and valid JSON
- **Prometheus Endpoint**: Must contain proper Prometheus format with required metrics
- **Test Endpoint**: Must contain "client_ip" field for GET, appropriate fields for POST
- **Recent Endpoint**: Must contain log entries (minimum 100 lines)
- **Appconfig Endpoint**: Must contain "APPCONFIG" identifier

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_32_system_endpoints.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary located successfully
- Configuration file validated
- Server starts and all endpoints respond correctly
- All JSON responses are properly formatted
- Prometheus endpoint returns valid metrics format
- All request scenarios (GET/POST) work correctly
- Server logs show proper compression functionality
- Clean server shutdown

### Key Validation Points

- **Endpoint Availability**: All system endpoints accessible
- **Response Format**: Proper JSON/text formatting
- **Content Validation**: Expected fields present in responses
- **HTTP Method Support**: Both GET and POST requests handled
- **Parameter Processing**: URL parameters and form data processed correctly
- **Metrics Format**: Prometheus metrics follow standard format
- **Compression**: Brotli compression working with detailed metrics

## Troubleshooting

### Common Issues

- **Configuration Missing**: Ensure `hydrogen_test_system_endpoints.json` exists
- **Endpoint Disabled**: Verify all system endpoints are enabled in configuration
- **JSON Validation**: Install `jq` for proper JSON validation (falls back to basic checks)
- **Server Startup**: Check logs for configuration or port binding issues
- **Compression Logs**: Verify Brotli compression is enabled and functioning

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: Detailed server logs with error analysis
- **Response Files**: Individual endpoint responses saved for analysis
- **Compression Logs**: Brotli compression metrics and performance data
- **Error Logs**: Filtered API-related errors and warnings

## Prometheus Metrics Validated

- `system_info`: System information metrics
- `memory_total_bytes`: Memory usage metrics
- `cpu_usage_total`: CPU utilization metrics
- `service_threads`: Thread count metrics

## Version History

- **3.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **2.0.0** (2025-06-17): Major refactoring with shellcheck compliance and improved modularity

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_20_api_prefix.md](test_20_api_prefix.md) - API prefix testing
- [test_22_swagger.md](test_22_swagger.md) - Swagger/OpenAPI documentation testing
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [api.md](/docs/H/core/api.md) - API documentation and endpoint specifications
- [configuration.md](/docs/H/core/configuration.md) - Configuration file documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
- [system_info.md](/docs/H/core/system_info.md) - System information documentation
