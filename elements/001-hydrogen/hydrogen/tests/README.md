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

- Cleans up test output directories:
  - `./logs/` - Test execution logs
  - `./results/` - Test results and summaries
  - `./diagnostics/` - Diagnostic information and analysis
- Removes temporary files from the test directory
- Ensures no previous test instances are running
- Prepares the test environment for consistent execution

This cleanup is automatically performed at the start of test execution via `test_00_all.sh`.

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

### support_cppcheck.sh

A wrapper script that provides standardized C/C++ code analysis using cppcheck:

```bash
./support_cppcheck.sh [directory_to_check]
```

Key features:

- Integrates with project-specific linting configurations:
  - Uses .lintignore for file exclusions
  - Uses .lintignore-c for cppcheck-specific settings
- Supports multiple configuration options through .lintignore-c:
  - enable=<checks> - Enable specific check categories
  - include=<path> - Add include paths
  - check-level=<level> - Set checking level
  - template=<format> - Customize error output format
  - suppress=<id> - Suppress specific warning types
  - option=<value> - Pass additional cppcheck options
- Processes files individually for accurate error reporting
- Handles path resolution and exclusions robustly

## Linting Configuration Files

The project uses several configuration files to control linting behavior:

### .lintignore

Global file exclusion patterns for all linting tools:

```text
# Example .lintignore
build/*              # Exclude build directories
tests/logs/*         # Exclude test logs
src/config/*.inc     # Exclude specific file types
```

- Uses glob patterns similar to .gitignore
- Supports comments and empty lines
- Can exclude specific files or entire directories
- Applied to all linting operations

### .lintignore-c

C/C++ specific linting configuration for cppcheck:

```text
# Enable specific checks
enable=style,warning,performance,portability

# Include paths
include=/usr/include
include=/usr/local/include

# Checking level
check-level=normal

# Output template
template={file}:{line}:{column}: {severity}: {message} ({id})

# Suppress specific warnings
suppress=missingIncludeSystem
suppress=variableScope
```

- Controls cppcheck behavior and reporting
- Configures enabled checks and suppressions
- Sets include paths and output formatting
- Used by support_cppcheck.sh and test_99_codebase.sh

### .lintignore-markdown

Markdown linting configuration:

```text
# Example .lintignore-markdown
{
  "default": true,
  "MD013": false,     # Disable line length checks
  "MD033": false      # Allow inline HTML
}
```

- Controls markdownlint behavior
- Enables/disables specific rules
- Configures rule-specific settings
- Used by test_99_codebase.sh for documentation checks

## Test Numbering System

Tests are numbered to ensure they run in a specific order:

- 00-09: Core test infrastructure (e.g., test_00_all.sh)
- 10-19: Basic functionality (compilation, startup)
- 20-29: System state management
- 30-39: Dynamic behavior
- 40-49: Configuration and error handling
- 50-59: Crash and recovery handling
- 60-69: API functionality
- 70-79: UI and interface tests
- 90-99: Code quality and final checks

## Test Scripts

The following tests are listed in numerical order:

### test_00_all.sh (Test Orchestration)

A test orchestration script that executes tests in sequence with compilation verification:

```bash
# Run all tests
./test_00_all.sh all

# Run with minimal configuration only
./test_00_all.sh min

# Run with maximal configuration only
./test_00_all.sh max

# Skip actual test execution (for quick README updates)
./test_00_all.sh --skip-tests
```

Key features:

- Performs complete cleanup of test output directories before starting:
  - Cleans `./logs/` directory for test execution logs
  - Cleans `./results/` directory for test results and summaries
  - Cleans `./diagnostics/` directory for analysis data
- Provides formatted output with test results
- Automatically makes test scripts executable
- Special handling for test_10_compilation.sh:
  - Runs first as a prerequisite
  - Skips remaining tests if compilation fails
  - Only test with special handling
- Dynamically discovers and runs all other tests in order
- Generates comprehensive summary with:
  - Individual test results and subtest counts
  - Visual pass/fail indicators
  - Total test and subtest statistics
- Can skip execution while showing what would run (--skip-tests)
- Always updates README.md with test results and code statistics

Test results and repository statistics are automatically added to the project README.md after running tests.

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

Test execution produces output in three main directories:

- `./logs/` - Contains detailed execution logs from test runs
- `./results/` - Stores test results, summaries, and build outputs
- `./diagnostics/` - Contains analysis data and debugging information

These directories are automatically cleaned at the start of each test run via support_cleanup.sh.

All tests provide standardized output with:

- Consistent colored headers and section breaks
- Checkmarks (✅) for passed tests
- X marks (❌) for failed tests
- Warning symbols (⚠️) for tests that pass with warnings
- Info symbols (🛈) for informational messages
- Detailed test summaries with pass/fail counts

When running the full test suite with `test_00_all.sh all`, a comprehensive summary is generated showing:

- Individual test results for each component
- Overall pass/fail statistics
- Final pass/fail determination

## Usage Examples

### Basic Testing

To run tests with both configurations using the orchestration script:

```bash
./test_00_all.sh all
```

This will run all tests and provide a comprehensive summary of results with standardized formatting.

If you want to quickly update the project README with test results without running the actual tests:

```bash
./test_00_all.sh --skip-tests --update-readme
```

This will:

- Register all tests as "skipped" without executing them
- Generate a test summary showing what tests would run
- Add a "Latest Test Results" section to the project README.md
- Add a "Repository Information" section with code statistics from cloc

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

## Developing New Tests

All new test scripts must follow standardized conventions to ensure consistency, maintainability, and quality across the test suite.

### Required Standards for New Tests

#### 1. Version and Title Headers

Every test script **MUST** include version and title variables at the top:

```bash
#!/bin/bash
#
# VERSION_TEST_XX="1.0.0"
# TITLE_TEST_XX="Descriptive Test Name"
#
# VERSION HISTORY:
# v1.0.0 - YYYY-MM-DD - Initial version with [brief description]
#
# [Rest of script header comments]

VERSION_TEST_XX="1.0.0"
TITLE_TEST_XX="Descriptive Test Name"
```

Replace `XX` with your test number (e.g., `VERSION_TEST_65` for test_65_system_endpoints.sh).

#### 2. Change History Section

Include a CHANGE HISTORY section near the top that tracks:

- Version numbers (semantic versioning: MAJOR.MINOR.PATCH)
- Dates of changes (YYYY-MM-DD format)
- Brief description of what changed

Example:

```bash
# CHANGE HISTORY:
# v1.2.1 - 2025-06-17 - Fixed timeout handling in API tests
# v1.2.0 - 2025-06-15 - Added support for custom headers
# v1.1.0 - 2025-06-10 - Enhanced error reporting
# v1.0.0 - 2025-06-01 - Initial version with basic functionality
```

#### 3. Shellcheck Compliance

All test scripts **MUST** pass shellcheck validation without errors or warnings:

```bash
# Validate your script before committing
shellcheck test_your_script.sh
```

Common shellcheck requirements:

- Quote all variable expansions: `"$variable"` not `$variable`
- Use `[[ ]]` for conditionals instead of `[ ]`
- Declare arrays properly: `declare -a array_name`
- Use `$(command)` instead of backticks
- Handle exit codes explicitly
- Use proper error handling with `set -e` or explicit checks

#### 4. Title Display

Display the test title and version at the beginning of execution:

```bash
# Print header with version
print_header "$TITLE_TEST_XX v$VERSION_TEST_XX" | tee "$SUMMARY_LOG"
```

### Development Workflow

1. **Start with Template**: Copy another scirpt sas your starting point
2. **Add Headers**: Include version, title, and change history sections
3. **Implement Logic**: Write your test functionality
4. **Validate**: Run shellcheck to ensure compliance
5. **Test**: Verify the script works correctly
6. **Document**: Update this README with your test description

### Quality Assurance

Before submitting new tests:

- [ ] Version and title variables are defined
- [ ] Change history section is present and up-to-date
- [ ] Script passes `shellcheck` without errors/warnings
- [ ] Test displays title and version on execution
- [ ] Test follows existing naming conventions (test_NN_description.sh)
- [ ] Test integrates properly with `test_00_all.sh`
- [ ] Documentation is added to this README

## Extending Testing

The testing system follows a logical sequence:

1. **Compilation Testing**: First verify all components build successfully
2. **Startup/Shutdown Testing**: Then test the application's lifecycle management
3. **API Testing**: Test system endpoints to verify API functionality
4. **Specialized Testing**: Finally perform any feature-specific tests

### Getting Started

1. Copy an existing test file to create a new test:

   ```bash
   cp test_15_startup_shutdown.sh test_your_feature.sh
   chmod +x test_your_feature.sh
   ```

2. Choose a descriptive name that follows the numbering convention:
   - 10-19: Basic functionality (compilation, startup)
   - 20-29: System state management
   - 30-39: Dynamic behavior
   - 40-49: Configuration and error handling
   - 50-59: Crash and recovery handling
   - 60-69: API functionality
   - 70-79: UI and interface tests
   - 90-99: Code quality and final checks

### Best Practices

1. **Test Organization**
   - Use clear, descriptive test names
   - Group related tests into functions
   - Include setup and cleanup for each test case
   - Document expected outcomes

2. **Error Handling**
   - Check server startup success
   - Include timeouts for operations
   - Validate all responses
   - Clean up resources on failure

3. **Resource Management**
   - Use the --skip-cleanup option during development
   - Monitor resource usage for memory leaks
   - Verify clean shutdown
   - Check for leftover processes

4. **Documentation**
   - Add detailed comments explaining test purpose
   - Document any special requirements
   - Include example usage
   - Explain configuration requirements

### Example Usage

```bash
# Run with minimal configuration
./test_your_feature.sh --config min

# Run with maximal configuration
./test_your_feature.sh --config max

# Skip cleanup for debugging
./test_your_feature.sh --skip-cleanup
```

### Common Testing Scenarios

1. **API Testing**

   ```bash
   # Test endpoint with authentication
   test_api_endpoint "/api/protected" "success" "GET" "" "Bearer $TOKEN"
   
   # Test with query parameters
   test_api_endpoint "/api/search?query=test" "results"
   ```

2. **Configuration Validation**

   ```bash
   # Verify configuration loading
   check_config_value "server.name" "expected_value"
   
   # Test invalid configurations
   test_invalid_config "malformed.json"
   ```

3. **Resource Monitoring**

   ```bash
   # Monitor during high load
   generate_load &
   test_resource_usage 30
   cleanup_load
   ```

4. **Shutdown Testing**

   ```bash
   # Test clean shutdown
   verify_clean_shutdown $SERVER_PID
   
   # Test forced shutdown
   test_signal_handling SIGKILL 5
   ```

### General Guidelines

When adding new tests:

1. Create descriptively named test scripts that start with "test_" (e.g., test_feature.sh)
2. Create any needed support functions in files that start with "support_" (e.g., support_feature_utils.sh)
3. Use the standardized utility functions from `support_utils.sh` to minimize boilerplate
4. Document the purpose and expected outcomes in this README
5. Ensure all test configurations use relative paths for portability
6. Set appropriate log levels for the components being tested

The `test_00_all.sh` script will automatically discover and run any script named with the "test_" prefix, making it easy to add new tests without modifying the main test runner.

See the [Testing Documentation](../docs/testing.md) for more information about the Hydrogen testing approach.

## Test Documentation

The following test documentation files are available in the `docs/` directory:

### Test Script Documentation

- [test_00_all.md](docs/test_00_all.md) - Test orchestration and execution framework
- [test_10_compilation.md](docs/test_10_compilation.md) - Compilation and build verification tests
- [test_12_env_payload.md](docs/test_12_env_payload.md) - Environment payload testing
- [test_14_env_variables.md](docs/test_14_env_variables.md) - Environment variable validation
- [test_16_library_dependencies.md](docs/test_16_library_dependencies.md) - Library dependency verification
- [test_18_json_error_handling.md](docs/test_18_json_error_handling.md) - JSON error handling tests
- [test_20_shutdown.md](docs/test_20_shutdown.md) - Shutdown sequence testing
- [test_22_startup_shutdown.md](docs/test_22_startup_shutdown.md) - Startup and shutdown lifecycle tests
- [test_24_signals.md](docs/test_24_signals.md) - Signal handling tests
- [test_26_crash_handler.md](docs/test_26_crash_handler.md) - Crash handling and recovery tests
- [test_28_socket_rebind.md](docs/test_28_socket_rebind.md) - Socket rebinding tests
- [test_30_api_prefixes.md](docs/test_30_api_prefixes.md) - API prefix configuration tests
- [test_32_system_endpoints.md](docs/test_32_system_endpoints.md) - System endpoint functionality tests
- [test_34_swagger.md](docs/test_34_swagger.md) - Swagger documentation tests
- [test_90_unity.md](docs/test_90_unity.md) - Unity testing framework integration
- [test_96_leaks_like_a_sieve.md](docs/test_96_leaks_like_a_sieve.md) - Memory leak detection tests
- [test_98_check_links.md](docs/test_98_check_links.md) - Link validation tests
- [test_99_codebase.md](docs/test_99_codebase.md) - Codebase quality and analysis tests

### Framework and Utility Documentation

- [framework.md](docs/framework.md) - Testing framework overview and architecture
- [lifecycle.md](docs/lifecycle.md) - Test lifecycle management
- [log_output.md](docs/log_output.md) - Log output formatting and analysis
- [env_utils.md](docs/env_utils.md) - Environment utility functions
- [file_utils.md](docs/file_utils.md) - File utility functions
- [network_utils.md](docs/network_utils.md) - Network utility functions

### Table System Documentation

- [tables.md](docs/tables.md) - Table system overview
- [tables_config.md](docs/tables_config.md) - Table configuration
- [tables_data.md](docs/tables_data.md) - Table data management
- [tables_datatypes.md](docs/tables_datatypes.md) - Table data types
- [tables_render.md](docs/tables_render.md) - Table rendering
- [tables_themes.md](docs/tables_themes.md) - Table themes and styling

### Project Documentation

- [LIBRARIES.md](docs/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
- [github-sitemap.md](docs/github-sitemap.md) - GitHub repository sitemap
