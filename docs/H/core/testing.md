# Hydrogen Testing System

This document outlines the testing approach used in the Hydrogen 3D printer control server project. While the testing system is still in early development, it provides a foundation for validating core functionality.

## Testing Philosophy

The Hydrogen testing system is designed around these key principles:

1. **Configuration-Driven Testing** - Test different system configurations through standardized configuration files
2. **Validate Core Operations** - Focus on critical startup and shutdown sequences
3. **Detailed Logging** - Capture comprehensive logs to analyze system behavior
4. **Minimal Dependencies** - Reduce external dependencies to simplify test execution
5. **Progressive Enhancement** - Build the testing system incrementally, with a focus on high-value tests first

## Current Testing Capabilities

The current testing system provides validation at two key levels:

1. **Compilation Testing** - Verifies that all components build cleanly without errors or warnings
2. **Startup/Shutdown Testing** - Validates successful initialization and termination of the system

The compilation testing is a critical first step because it:

- Ensures code quality through strict compiler warnings
- Verifies all build variants work correctly (standard, debug, valgrind)
- Confirms OIDC client examples build successfully
- Prevents downstream issues from broken compilation

The startup/shutdown testing is essential because:

- It verifies that all components initialize and terminate properly
- It ensures resources are correctly allocated and released
- It confirms that interdependent subsystems interact correctly
- It helps identify initialization order issues
- It verifies graceful handling of configuration changes

### Automated Test Scripts

The testing system includes several specialized scripts:

1. **test_compilation.sh** - Verifies clean compilation of all components with strict warning flags
2. **run_tests.sh** - Test orchestration script that executes tests with different configurations
3. **test_startup_shutdown.sh** - Core script that validates startup and shutdown sequences
4. **analyze_stuck_threads.sh** - Diagnostic tool that identifies problematic thread states
5. **monitor_resources.sh** - Monitoring tool for tracking resource usage patterns

These scripts work together to provide comprehensive testing and diagnostic capabilities while requiring minimal changes to the core Hydrogen codebase.

## Running Tests

### Using the Test Runner

The test runner automatically executes tests in logical sequence:

1. First, compilation tests (all build variants and OIDC examples)
2. Then, startup/shutdown tests with configured variants

If compilation fails, startup/shutdown tests will be skipped since they would not be meaningful.

The simplest way to execute tests is using the test runner script:

```bash
./tests/test_00_all.sh
```

The test runner automates the process of:

- Cleaning up previous test results and diagnostics
- Making all test scripts executable
- Running tests with specified configurations
- Collecting and analyzing results
- Providing a formatted summary of test outcomes

### Test Artifacts Management

Test execution generates two types of artifacts:

1. **Results** - Test output and summary reports in `./build/tests/results/`
1. **Logss** - Hydrogen logs for each run in `./build/tests/logs/`
1. **Diagnostics** - Detailed diagnostic data in `./build/tests/diagnostics/`

These artifacts are managed as follows:

- **Automatic Cleanup** - Previous test artifacts are automatically removed before each test run
- **Git Exclusion** - Test artifacts directories are excluded from GitHub synchronization
- **Local Availability** - Artifacts remain available for analysis until the next test run
- **External Testing** - This approach allows tests to be run on production systems without sharing data

## Diagnostic Tools

### Analyzing Test Results

After running a test, examine the generated logs for these key indicators:

1. **Successful Initialization**:
   - Check for "Application started" message
   - Verify "initialized" messages for all enabled subsystems
   - Confirm no initialization errors are reported

2. **Clean Shutdown**:
   - Look for "Shutdown complete" message
   - Verify all subsystems report "shutdown complete"
   - Check that no resource leaks or remaining threads are reported

3. **System Behavior**:
   - Review timing information between startup and shutdown
   - Check for any unexpected warnings or errors
   - Verify subsystem dependencies are respected

### Example Log Analysis

Key log patterns to look for:

```log
[Initialization] Application started         # Successful startup
[Initialization] WebServer initialized       # Component startup success
[Shutdown] Initiating WebServer shutdown     # Orderly shutdown begins
[Shutdown] WebServer shutdown complete       # Component shutdown success
[Shutdown] Shutdown complete                 # Full system shutdown
```

Warning signs to investigate:

```log
[Initialization] Failed to initialize...     # Startup failure
[Shutdown] Some threads did not exit cleanly # Shutdown issue
[Shutdown] ... thread(s) still active        # Resource leak
```

## Future Testing Enhancements

The testing system will be expanded with these planned improvements:

1. **Unit Tests** - Develop targeted tests for individual components
2. **Mock Dependencies** - Create mock implementations of external dependencies
3. **Integration Tests** - Test interactions between multiple components
4. **Performance Tests** - Validate system behavior under load
5. **Security Tests** - Verify proper authentication and authorization

## Adding New Tests

The Hydrogen project provides a standardized template (test_template.sh) to help create consistent and comprehensive tests. This template encapsulates best practices and common test patterns derived from existing tests.

### Using the Test Template

1. Start by copying an existing test

   ```bash
   cp tests/test_02_secrets.sh tests/test_your_feature.sh
   chmod +x tests/test_your_feature.sh
   ```

2. Choose an appropriate test number based on functionality:
   - 10-19: Basic functionality (compilation, startup)
   - 20-29: System state management
   - 30-39: Dynamic behavior
   - 40-49: Configuration and error handling
   - 50-59: Crash and recovery handling
   - 60-69: API functionality
   - 70-79: UI and interface tests
   - 90-99: Code quality and final checks

3. The template provides built-in support for:
   - Basic startup/shutdown validation
   - API endpoint testing
   - Signal handling
   - Resource monitoring
   - Configuration validation

4. Common test patterns available in the template:

   ```bash
   # Server lifecycle testing
   SERVER_PID=$(start_hydrogen_server "$HYDROGEN_BIN" "$CONFIG_FILE" "$LOG_FILE")
   verify_clean_shutdown $SERVER_PID
   
   # API testing
   test_api_endpoint "/api/system/health" "alive" "GET"
   test_api_endpoint "/api/system/test" "success" "POST" '{"test":"data"}'
   
   # Resource monitoring
   test_resource_usage 30  # Monitor for 30 seconds
   
   # Signal handling
   test_signal_handling SIGTERM 10
   ```

### Test Configuration

1. Create configuration files that focus on specific test scenarios:
   - Use hydrogen_test_min.json for basic functionality
   - Use hydrogen_test_max.json for full system testing
   - Create custom configs for specific test requirements

2. Follow the naming pattern: `hydrogen_test_[scenario].json`

3. When using custom configurations:
   - Use unique port numbers (see Port Configuration in tests/TESTING.md)
   - Document special requirements
   - Maintain relative paths for portability
   - Set appropriate logging levels

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

See [tests/TESTING.md](../tests/TESTING.md) for detailed examples and the complete test template documentation.

## Testing Guidelines

1. **Test One Thing at a Time** - Each test configuration should focus on validating a specific aspect
2. **Make Tests Repeatable** - Tests should produce consistent results when run multiple times
3. **Keep Tests Independent** - Avoid dependencies between test configurations
4. **Document Expected Results** - Clearly specify what constitutes success for each test
5. **Local Paths** - Use relative paths to ensure portability across development environments

## Integration with Development Workflow

Testing should be integrated into the development process:

1. **Before Implementation** - Create test configurations that validate new requirements
2. **During Development** - Use tests to verify correct implementation
3. **Before Submission** - Run all tests to ensure no regressions
4. **After Deployment** - Validate system behavior in the target environment
