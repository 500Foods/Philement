# Environment Utilities Library Documentation

## Overview

The `env_utils.sh` script is a modular library within the Hydrogen test suite, designed to provide functions for handling and validating environment variables. This library is essential for test scripts that need to ensure specific environment variables are set and valid, particularly for security and configuration purposes.

## Purpose

The primary purpose of `env_utils.sh` is to centralize the logic for environment variable checks and validations, making it reusable across multiple test scripts. This modular approach enhances maintainability and consistency in how environment variables are managed within the test suite.

## Functions

### check_env_var

- **Purpose**: Checks if an environment variable is set and non-empty.
- **Parameters**:
  - `$1` - The name of the environment variable (for display purposes).
  - `$2` - The value of the environment variable to check.
- **Returns**:
  - `0` if the variable is set and non-empty.
  - `1` if the variable is unset or empty.
- **Usage**: Used in test scripts to verify the presence of required environment variables before proceeding with tests.

**Example**:

```bash
if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
    echo "PAYLOAD_KEY is set"
else
    echo "PAYLOAD_KEY is not set"
fi
```

### validate_rsa_key

- **Purpose**: Validates the format of an RSA key, either private or public, provided as a base64-encoded string.
- **Parameters**:
  - `$1` - The name of the key (for display purposes).
  - `$2` - The base64-encoded key data to validate.
  - `$3` - The type of key, either "private" or "public".
- **Returns**:
  - `0` if the key is valid.
  - `1` if the key is invalid or decoding fails.
- **Usage**: Used in test scripts to ensure that cryptographic keys provided via environment variables are correctly formatted and usable.

**Example**:

```bash
if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
    echo "PAYLOAD_KEY is a valid private key"
else
    echo "PAYLOAD_KEY is not a valid private key"
fi
```

## Integration with Test Suite

The `env_utils.sh` library integrates with other modular scripts like `log_output.sh` for consistent logging and output formatting. It uses functions like `print_message` and `print_warning` if available, ensuring that output aligns with the test suite's standardized format.

## Usage in Test Scripts

To use this library in a test script, source it at the beginning of the script:

```bash
source "$SCRIPT_DIR/lib/env_utils.sh"
```

Then, call the functions as needed to validate environment variables or RSA keys. This library supports parallel test execution by avoiding shared resources or stateful operations, ensuring independent operation across test runs.

## Version History

- **1.0.0** - 2025-07-02 - Initial creation for the migration of `test_12_env_payload.sh` to the modular test suite structure.
