# Hydrogen Testing

This directory contains configuration files and test scripts for validating the Hydrogen 3D printer control server.

## Test Utilities

### test_utils.sh

A shared library of utilities that standardizes the formatting and output of all test scripts:

```bash
source test_utils.sh
```

Key features:

- Provides consistent colored output for all tests
- Standardizes formatting of test headers and results with checkmarks/X marks
- Implements common test flow with start_test and end_test functions
- Supports consistent summary generation across all tests

This library is sourced by all test scripts to ensure consistent visual presentation and reporting.

## Test Scripts

### test_compilation.sh

A compilation verification script that ensures all components build without errors or warnings:

```bash
./test_compilation.sh
```

Key features:

- Tests compilation of the main Hydrogen project in all build variants (standard, debug, valgrind)
- Tests compilation of the OIDC client examples
- Treats warnings as errors using strict compiler flags (-Wall -Wextra -Werror -pedantic)
- Creates detailed build logs in the `./results` directory
- Fails fast if any component fails to compile

This test runs as the first test in the test sequence since other tests won't be meaningful if components don't compile correctly.

### run_tests.sh

A test orchestration script that executes tests with different configurations and provides a comprehensive summary:

```bash
# Run all tests
./run_tests.sh all

# Run with minimal configuration only
./run_tests.sh min

# Run with maximal configuration only
./run_tests.sh max
```

The script provides formatted output with test results, automatically makes other test scripts executable, and generates a comprehensive summary of all test results with visual pass/fail indicators.

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
- Uses standardized formatting from test_utils.sh

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
- Uses standardized formatting from test_utils.sh

The test performs individual validation for each endpoint and request type, ensuring that the system correctly processes different types of HTTP requests and properly extracts form data from POST requests. It also monitors for system crashes or instability during the testing process.

### analyze_stuck_threads.sh

A diagnostic tool that analyzes thread states to help diagnose shutdown stalls:

```bash
./analyze_stuck_threads.sh <hydrogen_pid>
```

Key features:

- Examines all threads in a running process
- Identifies problematic thread states (especially uninterruptible sleep)
- Captures kernel stacks, wait channel info, and syscall information
- Outputs detailed diagnostics to the `./diagnostics` directory

### monitor_resources.sh

A resource monitoring tool for tracking process metrics:

```bash
./monitor_resources.sh <hydrogen_pid> [duration_seconds]
```

Key features:

- Tracks memory, CPU, thread count, and file descriptor usage
- Takes periodic snapshots of detailed process state
- Runs until the process exits or specified duration expires
- Generates statistics to help identify resource issues

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

## Test Output

All tests now provide standardized output with:

- Consistent colored headers and section breaks
- Checkmarks (✅) for passed tests
- X marks (❌) for failed tests
- Warning symbols (⚠️) for tests that pass with warnings
- Info symbols (ℹ️) for informational messages
- Detailed test summaries with pass/fail counts

When running the full test suite with `run_tests.sh all`, a comprehensive summary is generated showing:
- Individual test results for each component
- Overall pass/fail statistics
- Final pass/fail determination

## Usage Examples

### Basic Testing

To run tests with both configurations using the orchestration script:

```bash
./run_tests.sh all
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
   ./analyze_stuck_threads.sh <hydrogen_pid>
   ```

3. Check for threads in uninterruptible sleep (D state)
4. Examine the wait channels to identify what resources threads are waiting on

### Monitoring Resource Usage

To track resource usage during a test:

```bash
./monitor_resources.sh <hydrogen_pid> 60  # Monitor for 60 seconds
```

This helps identify memory leaks, resource exhaustion, or usage patterns that might contribute to shutdown issues.

## Extending Testing

The testing system follows a logical sequence:

1. **Compilation Testing**: First verify all components build successfully
2. **Startup/Shutdown Testing**: Then test the application's lifecycle management
3. **API Testing**: Test system endpoints to verify API functionality
4. **Specialized Testing**: Finally perform any feature-specific tests

When adding new tests:

1. Create descriptively named configuration files that target specific test scenarios
2. Document the purpose and expected outcomes in this README
3. Ensure all test configurations use relative paths for portability
4. Set appropriate log levels for the components being tested
5. Source the test_utils.sh file for standardized formatting

See the [Testing Documentation](../docs/testing.md) for more information about the Hydrogen testing approach.
