# Hydrogen Testing

This directory contains configuration files and test scripts for validating the Hydrogen 3D printer control server.

## Test Scripts

### run_tests.sh

A test orchestration script that executes tests with different configurations:

```bash
# Run all tests
./run_tests.sh all

# Run with minimal configuration only
./run_tests.sh min

# Run with maximal configuration only
./run_tests.sh max
```

The script provides formatted output with test results and automatically makes other test scripts executable.

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

### analyze_stuck_threads.sh

A diagnostic tool that analyzes thread states to help diagnose shutdown stalls:

```bash
./analyze_stuck_threads.sh <hydrogen_pid>
```

Key features:

- Examines all threads in a running process
- Identifies problematic thread states (especially uninterruptible sleep)
- Captures kernel stacks, wait channels info, and syscall information
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

## Usage Examples

### Basic Testing

To run tests with both configurations using the orchestration script:

```bash
./run_tests.sh all
```

This will run tests with both minimal and maximal configurations and provide a summary of results.

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

When adding new tests:

1. Create descriptively named configuration files that target specific test scenarios
2. Document the purpose and expected outcomes in this README
3. Ensure all test configurations use relative paths for portability
4. Set appropriate log levels for the components being tested

See the [Testing Documentation](../docs/testing.md) for more information about the Hydrogen testing approach.
