# Test 34: Swagger Script Documentation

## Overview

The `test_34_swagger.sh` script is a comprehensive Swagger/OpenAPI documentation testing tool within the Hydrogen test suite, focused on validating Swagger UI functionality with different prefixes, trailing slash handling, and immediate restart capabilities using SO_REUSEADDR.

## Purpose

This script ensures that the Hydrogen server correctly serves Swagger/OpenAPI documentation with proper URL handling, redirect functionality, and JavaScript file loading. It validates the server's API documentation accessibility, accuracy, and configuration flexibility across different Swagger prefix configurations.

## Script Details

- **Script Name**: `test_34_swagger.sh`
- **Test Name**: Swagger
- **Version**: 3.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Dual Configuration Testing**: Tests both default `/swagger` prefix and custom `/apidocs` prefix configurations
- **Trailing Slash Handling**: Validates proper URL handling with and without trailing slashes
- **Redirect Testing**: Ensures proper 301 redirects from non-trailing slash URLs
- **JavaScript File Validation**: Tests loading and content of Swagger UI JavaScript files
- **Immediate Restart Testing**: Validates SO_REUSEADDR functionality with immediate server restart

### Swagger UI Components Tested

- **Main Swagger UI**: Tests the primary Swagger interface loading
- **Swagger Initializer**: Validates `swagger-initializer.js` file content
- **Content Validation**: Ensures Swagger UI components load correctly
- **Redirect Behavior**: Tests automatic redirection to trailing slash URLs

### Advanced Testing

- **Configuration File Validation**: Verifies both test configuration files exist
- **Port Management**: Extracts and manages different ports for each configuration
- **Response Content Analysis**: Validates specific content in HTML and JavaScript responses
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

- **Default Configuration**: `hydrogen_test_swagger_test_1.json` (uses `/swagger` prefix)
- **Custom Configuration**: `hydrogen_test_swagger_test_2.json` (uses `/apidocs` prefix)

### Test Sequence (13 Subtests)

1. **Binary Location**: Locates Hydrogen binary
2. **Configuration 1 Validation**: Verifies first configuration file exists
3. **Configuration 2 Validation**: Verifies second configuration file exists
4. **Default Server Startup**: Starts server with default `/swagger` prefix
5. **Default Trailing Slash Test**: Tests `/swagger/` endpoint access
6. **Default Redirect Test**: Tests redirect from `/swagger` to `/swagger/`
7. **Default Content Verification**: Validates Swagger UI content loads
8. **Default JavaScript Test**: Tests `swagger-initializer.js` file loading
9. **Custom Server Startup**: Starts server with custom `/apidocs` prefix (immediate restart)
10. **Custom Trailing Slash Test**: Tests `/apidocs/` endpoint access
11. **Custom Redirect Test**: Tests redirect from `/apidocs` to `/apidocs/`
12. **Custom Content Verification**: Validates Swagger UI content loads
13. **Custom JavaScript Test**: Tests `swagger-initializer.js` file loading

### Response Validation Criteria

- **Swagger UI Pages**: Must contain "swagger-ui" content
- **JavaScript Files**: Must contain "window.ui = SwaggerUIBundle" for initializer
- **Redirects**: Must return 301 status with proper Location header
- **Content Loading**: Must successfully load HTML and JavaScript resources

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_34_swagger.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary located successfully
- Both configuration files found and validated
- Default server starts and serves Swagger UI at `/swagger/`
- Proper redirect from `/swagger` to `/swagger/`
- Swagger UI content and JavaScript files load correctly
- Custom server starts immediately after default server shutdown
- Custom server serves Swagger UI at `/apidocs/`
- Proper redirect from `/apidocs` to `/apidocs/`
- All Swagger components function with custom prefix

### Key Validation Points

- **Prefix Flexibility**: Both `/swagger` and `/apidocs` prefixes work correctly
- **URL Handling**: Proper trailing slash handling and redirects
- **Content Loading**: Swagger UI interface loads completely
- **JavaScript Functionality**: All required JavaScript files accessible
- **Immediate Restart**: SO_REUSEADDR allows immediate server restart
- **Configuration Adaptability**: Server adapts to different Swagger prefix configurations

## Troubleshooting

### Common Issues

- **Configuration Files Missing**: Ensure both test configuration files exist
- **Swagger UI Not Loading**: Verify Swagger is enabled in configuration
- **JavaScript Errors**: Check that all Swagger UI assets are properly served
- **Redirect Issues**: Ensure web server is configured for proper URL handling
- **Port Conflicts**: Script handles port management automatically

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: Separate logs for default and custom configurations
- **Response Files**: HTML and JavaScript responses saved for analysis
- **Redirect Headers**: HTTP redirect responses captured for validation
- **Timestamped Output**: All results include timestamp for tracking

## Configuration Requirements

### Default Configuration (`hydrogen_test_swagger_test_1.json`)

- Must configure Swagger prefix as `/swagger`
- Must enable Swagger UI functionality
- Must specify web server port

### Custom Configuration (`hydrogen_test_swagger_test_2.json`)

- Must configure Swagger prefix as `/apidocs`
- Must enable Swagger UI functionality
- Must specify web server port (can be same as default due to SO_REUSEADDR)

## Swagger UI Components Validated

- **Main Interface**: Primary Swagger UI HTML page
- **Initializer Script**: `swagger-initializer.js` with proper SwaggerUIBundle configuration
- **URL Redirects**: Automatic redirection to trailing slash URLs
- **Content Integrity**: Proper loading of all Swagger UI components

## Version History

- **3.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **2.0.0** (2025-06-17): Major refactoring with shellcheck compliance and improved modularity

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test suite documentation
- [test_21_system_endpoints.md](/docs/H/tests/test_21_system_endpoints.md) - System endpoint testing
- [test_20_api_prefix.md](/docs/H/tests/test_20_api_prefix.md) - API prefix testing
- [test_19_socket_rebind.md](/docs/H/tests/test_19_socket_rebind.md) - Socket rebinding tests (SO_REUSEADDR)
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Modular library documentation
- [api.md](/docs/H/core/api.md) - API documentation and endpoint specifications
- [configuration.md](/docs/H/core/configuration.md) - Configuration file documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
