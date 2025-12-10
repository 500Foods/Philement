# Test 14 Secrets

## Overview

The `test_14_secrets.sh` script is a validation tool within the Hydrogen test suite, focused on ensuring the proper configuration of payload encryption environment variables.

## Purpose

This script validates the presence and correctness of environment variables required for payload encryption, ensuring that the Hydrogen server can securely handle encrypted data. It is essential for confirming the setup before running other payload-related tests.

## Key Features

- **Library-Based Architecture**: Uses modular library scripts for logging, framework functions, and environment utilities
- **Environment Variable Validation**: Checks for the presence of required environment variables (`PAYLOAD_KEY` and `PAYLOAD_LOCK`)
- **RSA Key Verification**: Verifies the format and validity of RSA keys:
  - Ensures `PAYLOAD_KEY` is a valid RSA private key
  - Ensures `PAYLOAD_LOCK` is a valid RSA public key
- **OpenSSL Integration**: Uses OpenSSL commands to validate key formats
- **Comprehensive Testing**: Performs 2 subtests covering all aspects of payload environment setup
- **Detailed Error Reporting**: Provides detailed feedback for missing variables, invalid key formats, or malformed RSA keys

## Script Architecture

### Version Information

- **Current Version**: 3.0.0
- **Major Rewrite**: Version 3.0.0 introduced complete library-based architecture
- **Previous Version**: 2.0.0 added improved modularity and version tracking

### Library Dependencies

The script sources several library modules:

- `log_output.sh` - Log output formatting and printing functions
- `framework.sh` - Test framework functions for subtest management
- `env_utils.sh` - Environment utility functions for variable validation

### Test Execution Flow

1. **Environment Variable Presence Check**: Verify required variables are set
2. **RSA Key Format Validation**: Validate key formats using OpenSSL

## Subtests Performed

1. **Environment Variables Present** - Checks that `PAYLOAD_KEY` and `PAYLOAD_LOCK` are defined
2. **RSA Key Format Validation** - Validates that the keys are properly formatted RSA keys

## Environment Variables Required

| Variable | Type | Purpose |
|----------|------|---------|
| `PAYLOAD_KEY` | RSA Private Key (Base64) | Used for payload decryption |
| `PAYLOAD_LOCK` | RSA Public Key (Base64) | Used for payload encryption |

## Validation Process

### Environment Variable Check

- Verifies each required variable is set and non-empty
- Reports missing variables with clear error messages

### RSA Key Validation

- **Private Key (`PAYLOAD_KEY`)**:
  - Decodes from Base64
  - Validates using `openssl rsa -check -noout`
- **Public Key (`PAYLOAD_LOCK`)**:
  - Decodes from Base64  
  - Validates using `openssl rsa -pubin -noout`

## Usage

### Run as Part of Full Test Suite

```bash
./test_00_all.sh
```

### Run Individually

```bash
./test_14_secrets.sh
```

### Run Specific Test

```bash
./test_00_all.sh 14_secrets
```

## Output Format

The script provides detailed output including:

- Environment variable presence status
- RSA key validation results
- OpenSSL command execution details
- Comprehensive test completion summary

## Integration

- **Test Suite Integration**: Automatically discovered and run by `test_00_all.sh`
- **Result Tracking**: Exports subtest results for summary reporting
- **Prerequisite Role**: Required for payload-related functionality testing

## Error Scenarios

The test will fail if:

- Required environment variables are missing or empty
- RSA keys are not properly Base64 encoded
- Private key fails OpenSSL validation
- Public key fails OpenSSL validation
- OpenSSL is not available on the system

## Related Documentation

- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
- [framework.md](/docs/H/tests/framework.md) - Testing framework overview and architecture
- [env_utils.md](/docs/H/tests/env_utils.md) - Environment utility functions documentation
- [log_output.md](/docs/H/tests/log_output.md) - Log output formatting and analysis
- [SECRETS.md](/docs/H/SECRETS.md) - Information about managing secrets throughout the project