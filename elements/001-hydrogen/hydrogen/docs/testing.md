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

The current testing system focuses on validating the successful startup and shutdown of the Hydrogen application with different subsystem configurations. This validation is critical because:

- It verifies that all components initialize and terminate properly
- It ensures resources are correctly allocated and released
- It confirms that interdependent subsystems interact correctly
- It helps identify initialization order issues
- It verifies graceful handling of configuration changes

### Automated Test Scripts

The testing system includes several specialized scripts:

1. **run_tests.sh** - Test orchestration script that executes tests with different configurations
2. **test_startup_shutdown.sh** - Core script that validates startup and shutdown sequences
3. **analyze_stuck_threads.sh** - Diagnostic tool that identifies problematic thread states
4. **monitor_resources.sh** - Monitoring tool for tracking resource usage patterns

These scripts work together to provide comprehensive testing and diagnostic capabilities while requiring minimal changes to the core Hydrogen codebase.

## Test Configurations

Two primary test configurations have been created:

### Minimal Configuration (hydrogen_test_min.json)

- **Purpose**: Test core system startup/shutdown without optional subsystems
- **Enabled Systems**: Core systems only
- **Logging**: Maximum verbosity (ALL) across all subsystems
- **Location**: [tests/hydrogen_test_min.json](../tests/hydrogen_test_min.json)

### Maximal Configuration (hydrogen_test_max.json)

- **Purpose**: Test complete system startup/shutdown with all subsystems
- **Enabled Systems**: All available subsystems (WebServer, WebSocket, PrintQueue, mDNSServer)
- **Logging**: Maximum verbosity (ALL) across all subsystems
- **Location**: [tests/hydrogen_test_max.json](../tests/hydrogen_test_max.json)

## Running Tests

### Using the Test Runner

The simplest way to execute tests is using the test runner script:

```bash
# Run tests with both configurations
./tests/run_tests.sh all

# Run with minimal configuration only
./tests/run_tests.sh min

# Run with maximal configuration only 
./tests/run_tests.sh max
```

The test runner automates the process of:

- Cleaning up previous test results and diagnostics
- Making all test scripts executable
- Running tests with specified configurations
- Collecting and analyzing results
- Providing a formatted summary of test outcomes

### Test Artifacts Management

Test execution generates two types of artifacts:

1. **Results** - Test logs and summary reports in `./tests/results/`
2. **Diagnostics** - Detailed diagnostic data in `./tests/diagnostics/`

These artifacts are managed as follows:

- **Automatic Cleanup** - Previous test artifacts are automatically removed before each test run
- **Git Exclusion** - Test artifacts directories are excluded from GitHub synchronization
- **Local Availability** - Artifacts remain available for analysis until the next test run
- **External Testing** - This approach allows tests to be run on production systems without sharing data

### Manual Test Execution

You can also execute tests directly with the Hydrogen executable:

```bash
# Test with minimal configuration (core systems only)
./hydrogen ./tests/hydrogen_test_min.json

# Test with maximal configuration (all systems)
./hydrogen ./tests/hydrogen_test_max.json
```

## Diagnostic Tools

### Thread Analysis

When Hydrogen stalls during shutdown, the thread analyzer can help identify the cause:

```bash
./tests/analyze_stuck_threads.sh <hydrogen_pid>
```

This tool:

- Examines all threads in a running process
- Identifies threads in problematic states (especially uninterruptible sleep)
- Captures kernel stacks, wait channels info, and syscall information
- Generates detailed reports in the `./diagnostics` directory

Thread analysis is particularly useful for identifying:

- Deadlocks where threads are waiting on each other
- I/O operations that aren't completing
- Resource contention issues
- Improper synchronization

### Resource Monitoring

To track system resource usage over time:

```bash
./tests/monitor_resources.sh <hydrogen_pid> [duration_seconds]
```

This tool captures:

- Memory usage trends
- CPU utilization
- Thread count over time
- File descriptor usage
- Periodic detailed snapshots

Resource monitoring helps identify:

- Memory leaks
- Excessive resource consumption
- Unusual resource patterns during shutdown
- Correlation between resource usage and system behavior

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

```
[Initialization] Application started         # Successful startup
[Initialization] WebServer initialized       # Component startup success
[Shutdown] Initiating WebServer shutdown     # Orderly shutdown begins
[Shutdown] WebServer shutdown complete       # Component shutdown success
[Shutdown] Shutdown complete                 # Full system shutdown
```

Warning signs to investigate:

```
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

When expanding the testing system:

1. Create configuration files that focus on specific test scenarios
2. Follow the naming pattern: `hydrogen_test_[scenario].json`
3. Document the test purpose in the [tests/README.md](../tests/README.md)
4. Ensure configurations use relative paths for portability
5. Set appropriate logging levels for the components being tested

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
