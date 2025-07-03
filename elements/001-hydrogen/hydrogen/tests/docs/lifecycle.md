# Lifecycle Management Library (lifecycle.sh)

This document provides detailed information about the functions available in `lifecycle.sh`, a library script used for managing the lifecycle of the Hydrogen application during testing. These functions facilitate starting, stopping, and monitoring the Hydrogen application, as well as handling related diagnostics and configurations.

## Overview

`lifecycle.sh` is designed to modularize common lifecycle management tasks for the Hydrogen application, ensuring consistency across test scripts and supporting parallel test execution. It is sourced by test scripts to access functions for finding binaries, starting and stopping the application, and capturing diagnostics.

## Version History

- **1.2.1** - 2025-07-02 - Updated `find_hydrogen_binary` to use relative paths in log messages to avoid exposing user information.
- **1.2.0** - 2025-07-02 - Added `validate_config_file` function for single configuration validation.
- **1.1.0** - 2025-07-02 - Added `validate_config_files`, `setup_output_directories`, and `run_lifecycle_test` functions for enhanced modularity.
- **1.0.0** - 2025-07-02 - Initial version with start and stop functions.

## Functions

### find_hydrogen_binary

- **Purpose**: Locates and validates the Hydrogen binary in the project directory.
- **Parameters**:
  - `hydrogen_dir`: The directory to search for the Hydrogen binary.
- **Returns**: The path to the Hydrogen binary if found and executable; otherwise, returns an error.
- **Usage**: Used by test scripts to ensure the Hydrogen binary is available before attempting to start the application.
- **Example**:

  ```bash
  if HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
      print_message "Using Hydrogen binary: $HYDROGEN_BIN"
  else
      print_result 1 "Failed to find Hydrogen binary"
  fi
  ```

### start_hydrogen

- **Purpose**: Starts the Hydrogen application with a specified configuration.
- **Parameters**:
  - `config_file`: Path to the configuration file.
  - `log_file`: Path to the log file for output.
  - `timeout`: Maximum time to wait for startup in seconds.
  - `hydrogen_bin`: Path to the Hydrogen binary.
- **Returns**: The PID of the started process if successful; otherwise, returns an error.
- **Usage**: Initiates the Hydrogen application and waits for it to start, logging output to the specified file.
- **Example**:

  ```bash
  if hydrogen_pid=$(start_hydrogen "$config_file" "$log_file" "$startup_timeout" "$hydrogen_bin"); then
      print_result 0 "Hydrogen started"
  else
      print_result 1 "Failed to start Hydrogen"
  fi
  ```

### wait_for_startup

- **Purpose**: Waits for the Hydrogen application to complete startup by monitoring the log file.
- **Parameters**:
  - `log_file`: Path to the log file to monitor.
  - `timeout`: Maximum time to wait for startup in seconds.
  - `launch_time_ms`: The launch time in milliseconds for calculating elapsed time.
- **Returns**: Success if startup is detected within the timeout; otherwise, failure.
- **Usage**: Internal function used by `start_hydrogen` to confirm application startup.
- **Example**: Called internally by `start_hydrogen`.

### stop_hydrogen

- **Purpose**: Stops the Hydrogen application by sending a shutdown signal and monitoring the process.
- **Parameters**:
  - `pid`: Process ID of the running Hydrogen application.
  - `log_file`: Path to the log file for output.
  - `timeout`: Maximum time to wait for shutdown in seconds.
  - `activity_timeout`: Timeout for inactivity in log output.
  - `diag_dir`: Directory for diagnostic output if shutdown fails.
- **Returns**: Success if shutdown completes cleanly; otherwise, failure.
- **Usage**: Terminates the Hydrogen application and verifies a clean shutdown.
- **Example**:

  ```bash
  if stop_hydrogen "$hydrogen_pid" "$log_file" "$shutdown_timeout" "$shutdown_activity_timeout" "$diag_config_dir"; then
      print_result 0 "Hydrogen stopped"
  else
      print_result 1 "Failed to stop Hydrogen"
  fi
  ```

### monitor_shutdown

- **Purpose**: Monitors the shutdown process of the Hydrogen application.
- **Parameters**:
  - `pid`: Process ID of the running Hydrogen application.
  - `log_file`: Path to the log file to monitor.
  - `timeout`: Maximum time to wait for shutdown in seconds.
  - `activity_timeout`: Timeout for inactivity in log output.
  - `diag_dir`: Directory for diagnostic output if shutdown fails.
- **Returns**: Success if the process terminates within the timeout; otherwise, failure.
- **Usage**: Internal function used by `stop_hydrogen` to ensure the application shuts down properly.
- **Example**: Called internally by `stop_hydrogen`.

### get_process_threads

- **Purpose**: Retrieves thread information for a given process using `pgrep`.
- **Parameters**:
  - `pid`: Process ID to get threads for.
  - `output_file`: File to write thread information to.
- **Returns**: Writes thread IDs to the specified output file.
- **Usage**: Used for diagnostics to capture thread information during process monitoring.
- **Example**: Called internally by diagnostic functions.

### capture_process_diagnostics

- **Purpose**: Captures diagnostic information about a running process, including threads, status, and file descriptors.
- **Parameters**:
  - `pid`: Process ID to capture diagnostics for.
  - `diag_dir`: Directory to store diagnostic files.
  - `prefix`: Prefix for diagnostic file names.
- **Returns**: Writes diagnostic information to files in the specified directory.
- **Usage**: Collects detailed information about the Hydrogen process for debugging purposes.
- **Example**:

  ```bash
  capture_process_diagnostics "$hydrogen_pid" "$diag_config_dir" "pre_shutdown"
  ```

### validate_config_files

- **Purpose**: Validates the existence of configuration files required for testing.
- **Parameters**:
  - `min_config`: Path to the minimal configuration file.
  - `max_config`: Path to the maximal configuration file.
- **Returns**: Success if both configuration files exist; otherwise, failure.
- **Usage**: Ensures configuration files are present before running tests.
- **Example**:

  ```bash
  if validate_config_files "$MIN_CONFIG" "$MAX_CONFIG"; then
      ((PASS_COUNT++))
  else
      EXIT_CODE=1
  fi
  ```

### validate_config_file

- **Purpose**: Validates the existence of a single configuration file required for testing.
- **Parameters**:
  - `config_file`: Path to the configuration file.
- **Returns**: Success if the configuration file exists; otherwise, failure.
- **Usage**: Ensures a single configuration file is present before running tests, useful for tests that only require one configuration.
- **Example**:

  ```bash
  if validate_config_file "$MIN_CONFIG"; then
      ((PASS_COUNT++))
  else
      EXIT_CODE=1
  fi
  ```

### setup_output_directories

- **Purpose**: Creates necessary output directories for test results and diagnostics.
- **Parameters**:
  - `results_dir`: Directory for test results.
  - `diag_dir`: Directory for diagnostics.
  - `log_file`: Path to the log file (used to create parent directory).
  - `diag_test_dir`: Specific test diagnostics directory.
- **Returns**: Success if directories are created or already exist; otherwise, failure.
- **Usage**: Sets up the directory structure needed for test output.
- **Example**:

  ```bash
  if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
      ((PASS_COUNT++))
  else
      EXIT_CODE=1
  fi
  ```

### run_lifecycle_test

- **Purpose**: Runs a complete lifecycle test for a specific configuration, including starting, stopping, and verifying shutdown of the Hydrogen application.
- **Parameters**:
  - `config_file`: Path to the configuration file.
  - `config_name`: Name of the configuration (for display and directory naming).
  - `diag_test_dir`: Base directory for test diagnostics.
  - `startup_timeout`: Timeout for startup in seconds.
  - `shutdown_timeout`: Timeout for shutdown in seconds.
  - `shutdown_activity_timeout`: Timeout for inactivity during shutdown.
  - `hydrogen_bin`: Path to the Hydrogen binary.
  - `log_file`: Path to the log file for output.
  - `pass_count_var`: Variable name to update pass count.
  - `exit_code_var`: Variable name to update exit code.
- **Returns**: Updates pass count and exit code based on test results.
- **Usage**: Orchestrates a full lifecycle test for a given configuration, used in test scripts like startup/shutdown tests.
- **Example**:

  ```bash
  config_name=$(basename "$MIN_CONFIG" .json)
  run_lifecycle_test "$MIN_CONFIG" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"
  ```

### configure_hydrogen_binary

- **Purpose**: Configures the Hydrogen binary path, preferring release build if available.
- **Parameters**:
  - `hydrogen_dir`: Optional directory to search for the binary (defaults to script's parent directory).
- **Returns**: Sets the global `HYDROGEN_BIN` variable and returns success if binary is found and executable.
- **Usage**: Sets up the binary path for use in tests, automatically choosing the best available build.
- **Example**:

  ```bash
  if configure_hydrogen_binary "$HYDROGEN_DIR"; then
      print_message "Hydrogen binary configured: $HYDROGEN_BIN"
  else
      print_error "Failed to configure Hydrogen binary"
      exit 1
  fi
  ```

### initialize_test_environment

- **Purpose**: Initializes the test environment and logging setup.
- **Parameters**:
  - `results_dir`: Directory for test results (optional, defaults to ../results).
  - `test_log_file`: Path to the test log file (optional, defaults to ./hydrogen_env_test.log).
- **Returns**: Sets global variables `RESULT_LOG` and `TEST_LOG_FILE` and creates necessary directories.
- **Usage**: Sets up the test environment with proper logging and result directories.
- **Example**:

  ```bash
  initialize_test_environment "$RESULTS_DIR" "$TEST_LOG_FILE"
  ```

### ensure_no_hydrogen_running

- **Purpose**: Ensures no Hydrogen instances are running before starting tests.
- **Parameters**: None.
- **Returns**: Always returns success after attempting to kill any running Hydrogen processes.
- **Usage**: Cleans up any existing Hydrogen processes to ensure a clean test environment.
- **Example**:

  ```bash
  ensure_no_hydrogen_running
  ```

### start_hydrogen_with_env

- **Purpose**: Starts Hydrogen with environment variables and waits for initialization.
- **Parameters**:
  - `output_file`: File to capture Hydrogen output.
  - `test_name`: Name of the test for logging purposes.
  - `config_file`: Configuration file to use.
- **Returns**: Sets global `HYDROGEN_PID` variable and returns success if Hydrogen starts successfully.
- **Usage**: Alternative startup function that focuses on environment variable testing.
- **Example**:

  ```bash
  if start_hydrogen_with_env "$OUTPUT_FILE" "Environment Test" "$CONFIG_FILE"; then
      print_result 0 "Hydrogen started with environment variables"
  else
      print_result 1 "Failed to start Hydrogen with environment variables"
  fi
  ```

### kill_hydrogen_process

- **Purpose**: Kills the Hydrogen process if it's running.
- **Parameters**: None (uses global `HYDROGEN_PID` variable).
- **Returns**: Always returns success after attempting to kill the process.
- **Usage**: Cleanup function to ensure Hydrogen is stopped, typically used in test cleanup.
- **Example**:

  ```bash
  kill_hydrogen_process
  ```

### verify_log_file_exists

- **Purpose**: Verifies that a log file was created successfully.
- **Parameters**:
  - `log_file`: Path to the log file to check (optional, defaults to `TEST_LOG_FILE`).
- **Returns**: Success if the log file exists; otherwise, failure.
- **Usage**: Validates that Hydrogen created the expected log file during execution.
- **Example**:

  ```bash
  if verify_log_file_exists "$TEST_LOG_FILE"; then
      print_result 0 "Log file created successfully"
  else
      print_result 1 "Log file was not created"
  fi
  ```

## Dependencies

This library depends on the following:

- **log_output.sh**: For consistent output formatting and error reporting
- **Standard Unix utilities**: `ps`, `kill`, `pgrep`, `stat`, `mkdir`, `sleep`
- **Optional utilities**: `lsof` (for enhanced diagnostics)

## Global Variables

The library uses and sets several global variables:

- `HYDROGEN_BIN`: Path to the Hydrogen binary
- `HYDROGEN_PID`: Process ID of the running Hydrogen instance
- `RESULT_LOG`: Path to the test result log file
- `TEST_LOG_FILE`: Path to the Hydrogen application log file

## Error Handling

The library provides comprehensive error handling:

- **Process validation**: Checks that processes start and stop correctly
- **Timeout handling**: Implements timeouts for startup and shutdown operations
- **Diagnostic capture**: Collects process information when operations fail
- **Graceful cleanup**: Ensures processes are properly terminated even on failure

## Usage Patterns

### Basic Lifecycle Test

```bash
#!/bin/bash
source "lib/log_output.sh"
source "lib/lifecycle.sh"

# Configure binary
configure_hydrogen_binary

# Start Hydrogen
if hydrogen_pid=$(start_hydrogen "$CONFIG_FILE" "$LOG_FILE" 30 "$HYDROGEN_BIN"); then
    # Run tests...
    
    # Stop Hydrogen
    stop_hydrogen "$hydrogen_pid" "$LOG_FILE" 10 5 "$DIAG_DIR"
fi
```

### Environment Variable Testing

```bash
#!/bin/bash
source "lib/log_output.sh"
source "lib/lifecycle.sh"

# Initialize environment
initialize_test_environment
ensure_no_hydrogen_running

# Start with environment variables
if start_hydrogen_with_env "$OUTPUT_FILE" "Env Test" "$CONFIG_FILE"; then
    # Verify log file
    verify_log_file_exists
    
    # Cleanup
    kill_hydrogen_process
fi
```
