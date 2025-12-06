# Hydrogen Test Suite - Environment Utilities Library Documentation

## Overview

**File**: `lib/env_utils.sh`  
**Purpose**: Provides functions for environment variable handling and validation, designed for use in test scripts to ensure required environment variables are set and valid.  
**Version**: 1.1.0  
**Last Updated**: 2025-07-02

## Version History

- **1.1.0** - 2025-07-02 - Updated with additional functions for `test_35_env_variables.sh` migration
- **1.0.0** - 2025-07-02 - Initial creation for `test_12_env_payload.sh` migration

## Functions

### print_env_utils_version()

- **Purpose**: Displays the script name and version information.
- **Parameters**: None
- **Returns**: None (outputs version string to console)
- **Usage**: Used to log the version of the environment utilities library being used in a test script.
- **Example**:

  ```bash
  print_env_utils_version
  # Output: === Hydrogen Environment Utilities Library v1.1.0 ===
  ```

### check_env_var()

- **Purpose**: Checks if an environment variable is set and non-empty.
- **Parameters**:
  - `$1` - Variable name (string)
- **Returns**:
  - `0` if the variable is set and non-empty
  - `1` if the variable is not set or empty
- **Usage**: Used to validate that required environment variables are set before proceeding with test execution. For sensitive variables (names containing PASS, LOCK, KEY, JWT, TOKEN, SECRET) with values longer than 20 characters, only the first 20 characters are shown in output followed by an ellipsis to protect sensitive data.
- **Example**:

  ```bash
  export MY_VAR="test"
  check_env_var "MY_VAR"
  # Output: ✓ MY_VAR is set to: test
  
  export PAYLOAD_KEY="very_long_sensitive_key_data_here"
  check_env_var "PAYLOAD_KEY"
  # Output: ✓ PAYLOAD_KEY is set to: very_long_sensitive_k...
  ```

### validate_rsa_key()

- **Purpose**: Validates the format of an RSA key (private or public) by decoding a base64-encoded key and checking its structure with OpenSSL.
- **Parameters**:
  - `$1` - Key name (string, for logging purposes)
  - `$2` - Base64-encoded key data (string)
  - `$3` - Key type (`private` or `public`)
- **Returns**:
  - `0` if the key is valid
  - `1` if the key is invalid or of an unknown type
- **Usage**: Used to ensure that RSA keys provided via environment variables or configuration are correctly formatted before use in tests.
- **Example**:

  ```bash
  validate_rsa_key "TestKey" "base64_encoded_key_data" "public"
  # Output: ✓ TestKey is a valid RSA public key (if valid)
  ```

### reset_environment_variables()

- **Purpose**: Unsets all Hydrogen-related environment variables used in testing to ensure a clean state.
- **Parameters**: None
- **Returns**: None (outputs confirmation message)
- **Usage**: Used to reset the environment before or after test cases to prevent variable leakage between tests.
- **Example**:

  ```bash
  reset_environment_variables
  # Output: All Hydrogen environment variables have been unset
  ```

### set_basic_test_environment()

- **Purpose**: Sets a predefined set of Hydrogen environment variables for basic test scenarios.
- **Parameters**: None
- **Returns**: None (outputs confirmation message)
- **Usage**: Used to quickly configure a standard set of environment variables for testing Hydrogen functionality.
- **Example**:

  ```bash
  set_basic_test_environment
  # Output: Basic environment variables for Hydrogen test have been set
  ```

- **Variables Set**:
  - `H_SERVER_NAME="hydrogen-env-test"`
  - `H_LOG_FILE="$TEST_LOG_FILE"`
  - `H_WEB_ENABLED="true"`
  - `H_WEB_PORT="9000"`
  - `H_UPLOAD_DIR="/tmp/hydrogen_env_test_uploads"`
  - `H_MAX_UPLOAD_SIZE="1048576"`
  - `H_SHUTDOWN_WAIT="3"`
  - `H_MAX_QUEUE_BLOCKS="64"`
  - `H_DEFAULT_QUEUE_CAPACITY="512"`
  - `H_MEMORY_WARNING="95"`
  - `H_LOAD_WARNING="7.5"`
  - `H_PRINT_QUEUE_ENABLED="false"`
  - `H_CONSOLE_LOG_LEVEL="1"`
  - `H_DEVICE_ID="hydrogen-env-test"`
  - `H_FRIENDLY_NAME="Hydrogen Environment Test"`

### validate_config_files()

- **Purpose**: Validates that required configuration files exist for environment variable testing.
- **Parameters**: None
- **Returns**:
  - `0` if the configuration file exists
  - `1` if the configuration file is not found
- **Usage**: Used to ensure the necessary configuration file (`hydrogen_test_env.json`) is available before running tests.
- **Example**:

  ```bash
  validate_config_files
  # Output: Configuration file validated: /path/to/configs/hydrogen_test_env.json (if exists)
  ```

- **Side Effect**: Sets the global `CONFIG_FILE` variable to the path of the validated configuration file.

### get_config_path()

- **Purpose**: Gets the full path to a configuration file relative to the script's location.
- **Parameters**:
  - `$1` - Configuration file name (string)
- **Returns**: Outputs the full path to the configuration file as a string.
- **Usage**: Used internally by other functions to locate configuration files in the `configs/` directory.
- **Example**:

  ```bash
  config_path=$(get_config_path "hydrogen_test_env.json")
  echo $config_path
  # Output: /path/to/tests/configs/hydrogen_test_env.json
  ```

### convert_to_relative_path()

- **Purpose**: Converts an absolute path to a relative path starting from the `hydrogen` directory for cleaner logging.
- **Parameters**:
  - `$1` - Absolute path (string)
- **Returns**: Outputs the relative path if it matches the expected pattern, otherwise returns the original path.
- **Usage**: Used to simplify log output by converting long absolute paths to shorter relative paths starting from the project root.
- **Example**:

  ```bash
  convert_to_relative_path "/home/user/projects/elements/001-hydrogen/hydrogen/tests/results/log.txt"
  # Output: hydrogen/tests/results/log.txt
  ```

### set_type_conversion_environment()

- **Purpose**: Sets environment variables specifically for testing type conversion functionality.
- **Parameters**: None
- **Returns**: None (outputs confirmation message)
- **Usage**: Used to test how Hydrogen handles type conversion from string environment variables to appropriate data types (boolean, number, float).
- **Example**:

  ```bash
  set_type_conversion_environment
  # Output: Type conversion environment variables for Hydrogen test have been set
  ```

- **Variables Set**:
  - `H_SERVER_NAME="hydrogen-type-test"`
  - `H_WEB_ENABLED="TRUE"` (uppercase, should convert to boolean true)
  - `H_WEB_PORT="8080"` (string that should convert to number)
  - `H_MEMORY_WARNING="75"` (string that should convert to number)
  - `H_LOAD_WARNING="2.5"` (string that should convert to float)
  - `H_PRINT_QUEUE_ENABLED="FALSE"` (uppercase, should convert to boolean false)
  - `H_CONSOLE_LOG_LEVEL="0"`
  - `H_SHUTDOWN_WAIT="5"`
  - `H_MAX_QUEUE_BLOCKS="128"`
  - `H_DEFAULT_QUEUE_CAPACITY="1024"`

### set_validation_test_environment()

- **Purpose**: Sets environment variables with intentionally invalid values for testing validation functionality.
- **Parameters**: None
- **Returns**: None (outputs confirmation message)
- **Usage**: Used to test how Hydrogen handles invalid or edge-case environment variable values and validates input properly.
- **Example**:

  ```bash
  set_validation_test_environment
  # Output: Validation test environment variables for Hydrogen test have been set
  ```

- **Variables Set**:
  - `H_SERVER_NAME="hydrogen-validation-test"`
  - `H_WEB_ENABLED="yes"` (non-standard boolean value)
  - `H_WEB_PORT="invalid"` (invalid port number)
  - `H_MEMORY_WARNING="150"` (invalid percentage >100)
  - `H_LOAD_WARNING="-1.0"` (invalid negative load)
  - `H_PRINT_QUEUE_ENABLED="maybe"` (invalid boolean)
  - `H_CONSOLE_LOG_LEVEL="debug"` (string instead of number)
  - `H_SHUTDOWN_WAIT="0"` (edge case: zero timeout)
  - `H_MAX_QUEUE_BLOCKS="0"` (edge case: zero blocks)
