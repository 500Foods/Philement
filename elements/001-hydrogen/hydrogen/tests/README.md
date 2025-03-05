# Hydrogen Testing

This directory contains configuration files and test scripts for validating the Hydrogen 3D printer control server.

## Support Utilities

### support_utils.sh

A shared library of utilities that standardizes the formatting and output of all test scripts:

```bash
source support_utils.sh
```

Key features:

- Provides consistent colored output for all tests
- Standardizes formatting of test headers and results with checkmarks/X marks
- Implements common test flow with start_test and end_test functions
- Supports consistent summary generation across all tests

This library is sourced by all test scripts to ensure consistent visual presentation and reporting.

### support_cleanup.sh

Provides functions for environment cleanup and preparation before running tests:

- Cleans up temporary files and directories
- Ensures no previous test instances are running
- Prepares the test environment for consistent execution

### support_analyze_stuck_threads.sh

A diagnostic tool that analyzes thread states to help diagnose shutdown stalls:

```bash
./support_analyze_stuck_threads.sh <hydrogen_pid>
```

Key features:

- Examines all threads in a running process
- Identifies problematic thread states (especially uninterruptible sleep)
- Captures kernel stacks, wait channel info, and syscall information
- Outputs detailed diagnostics to the `./diagnostics` directory

### support_monitor_resources.sh

A resource monitoring tool for tracking process metrics:

```bash
./support_monitor_resources.sh <hydrogen_pid> [duration_seconds]
```

Key features:

- Tracks memory, CPU, thread count, and file descriptor usage
- Takes periodic snapshots of detailed process state
- Runs until the process exits or specified duration expires
- Generates statistics to help identify resource issues

## Test Scripts

### test_all.sh

A test orchestration script that executes tests with different configurations and provides a comprehensive summary:

```bash
# Run all tests
./test_all.sh all

# Run with minimal configuration only
./test_all.sh min

# Run with maximal configuration only
./test_all.sh max
```

The script:

- Provides formatted output with test results
- Automatically makes other test scripts executable
- Dynamically discovers and runs all test_*.sh scripts
- Generates a comprehensive summary of all test results with visual pass/fail indicators

### test_compilation.sh

A compilation verification script that ensures all components build without errors or warnings:

```bash
./test_compilation.sh
```

### test_env_payload.sh

A validation script that ensures proper configuration of payload encryption environment variables:

```bash
./test_env_payload.sh
```

Key features:
- Validates presence of required environment variables (PAYLOAD_KEY and PAYLOAD_LOCK)
- Verifies RSA key format and validity:
  - Checks PAYLOAD_KEY is a valid 2048-bit RSA private key
  - Checks PAYLOAD_LOCK is a valid RSA public key
- Provides detailed error reporting for:
  - Missing environment variables
  - Invalid key formats
  - Malformed RSA keys
- Uses standardized formatting from support_utils.sh
- Creates detailed test logs in the `./results` directory
- Integrates with test_all.sh for comprehensive test reporting

This test is essential for validating the payload encryption system's configuration before running other payload-related tests.

### test_compilation.sh

- Tests compilation of the main Hydrogen project in all build variants (standard, debug, valgrind)
- Tests compilation of the OIDC client examples
- Treats warnings as errors using strict compiler flags (-Wall -Wextra -Werror -pedantic)
- Creates detailed build logs in the `./results` directory
- Fails fast if any component fails to compile

This test runs as the first test in the test sequence since other tests won't be meaningful if components don't compile correctly.

### test_startup_shutdown.sh

The core test script that validates Hydrogen's startup and shutdown sequence:

```bash
./test_startup_shutdown.sh <config_file.json>
```

Key features:

- Launches Hydrogen with a specified configuration
- Waits for successful startup
- Initiates a controlled shutdown
- Monitors for successful completion or timeout
- Collects detailed diagnostics if shutdown stalls
- Creates logs in the `./results` directory
- Uses standardized formatting from support_utils.sh

### test_system_endpoints.sh

Tests the system API endpoints to ensure they respond correctly:

```bash
./test_system_endpoints.sh
```

Key features:

- Tests all system API endpoints (health, info, test)
- Validates response content and format
- Verifies proper JSON formatting
- Tests various request methods:
  - Basic GET requests
  - GET requests with query parameters
  - POST requests with form data (with proper field extraction)
  - POST requests with both query parameters and form data
- Ensures robust handling of form data in POST requests
- Monitors server stability during tests
- Implements error handling and shell script validation
- Uses standardized formatting from support_utils.sh

The test performs individual validation for each endpoint and request type, ensuring that the system correctly processes different types of HTTP requests and properly extracts form data from POST requests. It also monitors for system crashes or instability during the testing process.

## Test Configuration Files

### hydrogen_test_min.json

This configuration file provides a **minimal** setup for testing the core functionality of the Hydrogen server:

- Disables all optional subsystems (WebServer, WebSocket, PrintQueue, mDNSServer)
- Enables maximum logging (level 0/"ALL") for all subsystems across all logging destinations
- Uses relative paths for logs and database files to simplify testing
- Maintains the core SystemResources and NetworkMonitoring configurations

Purpose: Test the basic startup and shutdown sequence with only core systems running, verifying that the essential components initialize and terminate properly without optional subsystems.

### hydrogen_test_max.json

This configuration file provides a **maximal** setup to test the full feature set of the Hydrogen server:

- Enables all subsystems (WebServer, WebSocket, PrintQueue, mDNSServer)
- Enables maximum logging (level 0/"ALL") for all subsystems across all logging destinations
- Includes test services for mDNS service advertisement
- Uses relative paths for logs and database files to simplify testing

Purpose: Validate that all subsystems can start and stop correctly, testing the complete initialization and shutdown process with all features enabled.

## Port Configuration

To avoid conflicts between tests that need to bind to network ports, dedicated port ranges are used for different test configurations:

## Environment Variables for Testing

The following environment variables are used by various tests:

| Variable | Purpose | Used By |
|----------|---------|----------|
| PAYLOAD_KEY | RSA private key for payload decryption | test_env_payload.sh |
| PAYLOAD_LOCK | RSA public key for payload encryption | test_env_payload.sh |

| Test                       | Configuration File                   | Web Server Port | WebSocket Port |
|----------------------------|-------------------------------------|-----------------|----------------|
| Default Min/Max            | hydrogen_test_min.json              | 5000            | 5001           |
|                            | hydrogen_test_max.json              | 5000            | 5001           |
| API Prefix Test            | hydrogen_test_api_test_1.json       | 5030            | 5031           |
|                            | hydrogen_test_api_test_2.json       | 5050            | 5051           |
| Swagger UI Test (Default)  | hydrogen_test_swagger_test_1.json   | 5040            | 5041           |
| Swagger UI Test (Custom)   | hydrogen_test_swagger_test_2.json   | 5060            | 5061           |
| System Endpoints Test      | hydrogen_test_system_endpoints.json | 5070            | 5071           |

Using different ports for these tests ensures they can run independently without socket binding conflicts, especially in scenarios where a socket might not be released immediately after a test completes (e.g., due to TIME_WAIT state). This improves test reliability and avoids false failures due to port conflicts.

## Test Output

All tests now provide standardized output with:

- Consistent colored headers and section breaks
- Checkmarks (✅) for passed tests
- X marks (❌) for failed tests
- Warning symbols (⚠️) for tests that pass with warnings
- Info symbols (🛈) for informational messages
- Detailed test summaries with pass/fail counts

When running the full test suite with `test_all.sh all`, a comprehensive summary is generated showing:

- Individual test results for each component
- Overall pass/fail statistics
- Final pass/fail determination

## Usage Examples

### Basic Testing

To run tests with both configurations using the orchestration script:

```bash
./test_all.sh all
```

This will run all tests and provide a comprehensive summary of results with standardized formatting.

### Manual Testing

You can manually run Hydrogen with the test configurations:

```bash
# Run directly with minimal configuration 
../hydrogen ./hydrogen_test_min.json

# Run directly with maximal configuration
../hydrogen ./hydrogen_test_max.json
```

### Diagnosing Shutdown Stalls

If Hydrogen stalls during shutdown:

1. Note the process ID (PID) from the test output
2. Run the thread analyzer to identify stuck threads:

   ```bash
   ./support_analyze_stuck_threads.sh <hydrogen_pid>
   ```

3. Check for threads in uninterruptible sleep (D state)
4. Examine the wait channels to identify what resources threads are waiting on

### Monitoring Resource Usage

To track resource usage during a test:

```bash
./support_monitor_resources.sh <hydrogen_pid> 60  # Monitor for 60 seconds
```

This helps identify memory leaks, resource exhaustion, or usage patterns that might contribute to shutdown issues.

## Extending Testing

The testing system follows a logical sequence:

1. **Compilation Testing**: First verify all components build successfully
2. **Startup/Shutdown Testing**: Then test the application's lifecycle management
3. **API Testing**: Test system endpoints to verify API functionality
4. **Specialized Testing**: Finally perform any feature-specific tests

### Creating New Tests

The recommended approach to create new tests is to use the provided template:

1. Copy the template file to create a new test:

   ```bash
   cp test_template.sh test_your_feature.sh
   chmod +x test_your_feature.sh
   ```

2. Modify the new test script:
   - Set an appropriate test name
   - Choose or create a suitable configuration file
   - Implement your specific test cases by replacing the placeholders
   - Add appropriate validation and result checking

The template standardizes:

- Test environment setup and cleanup
- Server startup and shutdown
- Configuration file handling
- Result reporting and formatting

### Test Template Structure

The `test_template.sh` script provides a standard structure for all tests:

1. **Environment Setup**: Sets up directories, logging, and timestamps
2. **Test Configuration**: Selects the appropriate configuration file
3. **Binary Selection**: Automatically finds the appropriate Hydrogen binary
4. **Test Implementation**: Contains the specific test cases
5. **Result Collection**: Tracks and reports test results
6. **Cleanup**: Ensures resources are properly released

### General Guidelines

When adding new tests:

1. Create descriptively named test scripts that start with "test_" (e.g., test_feature.sh)
2. Create any needed support functions in files that start with "support_" (e.g., support_feature_utils.sh)
3. Use the standardized utility functions from `support_utils.sh` to minimize boilerplate
4. Document the purpose and expected outcomes in this README
5. Ensure all test configurations use relative paths for portability
6. Set appropriate log levels for the components being tested

The `test_all.sh` script will automatically discover and run any script named with the "test_" prefix, making it easy to add new tests without modifying the main test runner.

See the [Testing Documentation](../docs/testing.md) for more information about the Hydrogen testing approach.
