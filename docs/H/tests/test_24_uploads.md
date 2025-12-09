# Test 24: Uploads Script Documentation

## Overview

The `test_24_uploads.sh` script is a comprehensive file upload testing tool within the Hydrogen test suite, designed to validate both API and web upload functionality with real test artifacts. The script exercises multiple code paths including successful uploads, oversized file rejections, and method validation.

## Purpose

This script ensures that the Hydrogen server correctly handles file uploads through both the REST API endpoint (`/api/system/upload`) and the web interface (`/upload`). It validates upload success scenarios, file size limits, HTTP method restrictions, and integration with analysis tools like Beryllium.

## Script Details

- **Script Name**: `test_24_uploads.sh`
- **Test Name**: Uploads
- **Version**: 1.1.0 (Enhanced with method testing and parallel execution)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Parallel Test Execution**: Runs multiple upload tests simultaneously for efficiency
- **Dual Endpoint Testing**: Validates both API and web upload endpoints
- **File Size Validation**: Tests both successful uploads and oversized file rejection
- **HTTP Method Validation**: Tests non-POST methods to ensure proper 405 responses
- **Beryllium Integration**: Validates that uploaded G-code files trigger Beryllium analysis
- **Real Artifact Testing**: Uses actual 3D model files (3MF, STL, G-code) for realistic testing

### Upload Test Scenarios

- **API Upload Success**: Tests successful file upload via REST API with 3DBenchy.3mf
- **API Upload Rejection**: Tests oversized file rejection (11MB STL file)
- **Web Upload Success**: Tests successful web upload with Beryllium analysis trigger
- **Web Upload Rejection**: Tests oversized file rejection via web interface
- **Method Validation**: Tests GET method rejection with proper 405 response

### Advanced Testing

- **Configuration File Validation**: Verifies all test configuration files exist
- **Artifact File Validation**: Ensures test files exist and reports their sizes
- **Server Lifecycle Management**: Handles server startup, readiness checking, and shutdown
- **Parallel Process Management**: Limits concurrent tests to available CPU cores
- **Result Aggregation**: Collects and analyzes results from parallel test executions

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `framework.sh` - Test framework and result tracking
- `file_utils.sh` - File operations and path management
- `lifecycle.sh` - Process lifecycle management
- `network_utils.sh` - Network utilities and port management

### Configuration Files

- **hydrogen_test_24_uploads_1.json**: Base configuration for API tests (port 5240)
- **hydrogen_test_24_uploads_2.json**: Configuration with size limits for rejection tests (port 5241)
- **hydrogen_test_24_uploads_3.json**: Web server configuration for web endpoint tests (port 5242)
- **hydrogen_test_24_uploads_4.json**: Web server with size limits (port 5243)
- **hydrogen_test_24_uploads_5.json**: Dedicated configuration for method tests (port 5244)

### Test Sequence (5 Subtests + 5 Parallel Tests)

1. **Binary Location**: Locates Hydrogen binary
2. **Configuration Validation**: Verifies all configuration files and artifacts
3. **Parallel Test Execution**: Runs 5 upload tests in parallel
4. **Result Analysis**: Analyzes results from all parallel tests
5. **Final Reporting**: Provides comprehensive test summary

### Test Configuration Details

| Test | Configuration | Endpoint | File | Method | Expected Result |
|------|---------------|----------|------|--------|-----------------|
| T1 | uploads_1.json | `/api/system/upload` | 3DBenchy.3mf | POST | 200 Success |
| T2 | uploads_2.json | `/api/system/upload` | 3DBenchy.stl | POST | 413 Rejected |
| T3 | uploads_3.json | `/upload` | 3DBenchy.gcode | POST | 200 + Beryllium |
| T4 | uploads_4.json | `/upload` | 3DBenchy.stl | POST | 413 Rejected |
| T5 | uploads_5.json | `/api/system/upload` | N/A | GET | 405 Method Not Allowed |

### Response Validation Criteria

- **Successful Uploads**: Must return 200 with proper JSON response and log completion
- **Oversized Files**: Must return 413 with appropriate rejection messages
- **Method Violations**: Must return 405 with "Method not allowed" message
- **Beryllium Integration**: Must trigger analysis for G-code files
- **Server Logs**: Must contain appropriate upload completion or rejection messages

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_24_uploads.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary located successfully
- All configuration files and test artifacts found and validated
- Server starts successfully for each test configuration
- API uploads succeed/fail as expected based on file size
- Web uploads succeed/fail as expected with proper Beryllium integration
- GET method requests return proper 405 Method Not Allowed responses
- All parallel tests complete within timeouts
- Server logs contain appropriate upload completion/rejection messages

### Key Validation Points

- **File Size Limits**: Both API and web endpoints properly enforce size limits
- **HTTP Method Restrictions**: Non-POST methods correctly rejected with 405
- **Beryllium Integration**: G-code uploads trigger analysis workflow
- **Parallel Execution**: Multiple tests can run simultaneously without interference
- **Server Stability**: Each server instance starts, runs tests, and shuts down cleanly
- **Log Analysis**: Server logs provide detailed information about upload processing

## Troubleshooting

### Common Issues

- **Configuration Files Missing**: Ensure all 5 test configuration files exist
- **Test Artifacts Missing**: Verify 3DBenchy.3mf, 3DBenchy.stl, and 3DBenchy.gcode exist
- **Port Conflicts**: Script handles port management automatically via configuration
- **Timeout Issues**: Increase STARTUP_TIMEOUT or SHUTDOWN_TIMEOUT if needed
- **Memory Issues**: Large test files (11MB) may require sufficient system memory

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: Separate logs for each test configuration and parallel instance
- **Response Files**: JSON responses saved for upload validation
- **Parallel Results**: Individual result files for each parallel test
- **Timestamped Output**: All results include timestamp for tracking

## Configuration Requirements

### Base Configuration (`hydrogen_test_24_uploads_1.json`)

- Must enable API endpoints with `/api/system/upload`
- Must specify web server port
- Must configure upload size limits appropriately
- Must enable logging for upload operations

### Size Limit Configuration (`hydrogen_test_24_uploads_2.json`)

- Must configure smaller upload size limits (e.g., 5MB)
- Must enable API endpoints
- Must specify different port to avoid conflicts

### Web Configurations (`hydrogen_test_24_uploads_3.json`, `hydrogen_test_24_uploads_4.json`)

- Must enable web server endpoints with `/upload`
- Must configure Beryllium integration for G-code files
- Must specify different ports for parallel execution

## Test Artifacts

### Required Test Files

- **3DBenchy.3mf**: ~3MB 3D model file for successful upload testing
- **3DBenchy.stl**: ~11MB 3D model file for rejection testing
- **3DBenchy.gcode**: ~4MB G-code file for Beryllium analysis testing

### Artifact Validation

The script automatically validates:

- File existence in `tests/artifacts/uploads/`
- File sizes for reporting
- File accessibility for curl uploads

## Code Coverage Enhancement

This test specifically targets the `src/api/service/upload.c` file to increase code coverage by:

- **POST Method Path**: Exercises normal upload functionality
- **Non-POST Method Path**: Exercises method validation and 405 response
- **File Size Limits**: Tests both acceptance and rejection scenarios
- **Error Handling**: Validates proper error responses and logging

## Version History

- **1.1.0** (2025-08-24): Added method testing, enhanced parallel execution
- **1.0.0** (2025-08-24): Initial implementation following test_22 pattern

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_21_system_endpoints.md](test_21_system_endpoints.md) - System endpoint testing
- [test_22_swagger.md](test_22_swagger.md) - Similar parallel test pattern
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [api.md](/docs/H/core/api.md) - API documentation and endpoint specifications
- [configuration.md](/docs/H/core/configuration.md) - Configuration file documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
