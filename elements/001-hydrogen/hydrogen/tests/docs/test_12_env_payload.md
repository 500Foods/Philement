# Test 05 Environment Payload Script Documentation

## Overview

The `test_05_env_payload.sh` script is a validation tool within the Hydrogen test suite, focused on ensuring the proper configuration of payload encryption environment variables.

## Purpose

This script validates the presence and correctness of environment variables required for payload encryption, ensuring that the Hydrogen server can securely handle encrypted data. It is essential for confirming the setup before running other payload-related tests.

## Key Features

- **Validation of Variables**: Checks for the presence of required environment variables (`PAYLOAD_KEY` and `PAYLOAD_LOCK`).
- **RSA Key Verification**: Verifies the format and validity of RSA keys:
  - Ensures `PAYLOAD_KEY` is a valid 2048-bit RSA private key.
  - Ensures `PAYLOAD_LOCK` is a valid RSA public key.
- **Detailed Error Reporting**: Provides detailed feedback for missing variables, invalid key formats, or malformed RSA keys.
- **Standardized Formatting**: Uses formatting from `support_utils.sh` for consistent output.
- **Log Creation**: Creates detailed test logs in the `./results` directory.
- **Integration**: Integrates with `test_00_all.sh` for comprehensive test reporting.

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_12_env_payload.sh
```

## Additional Notes

- This test is crucial for validating the payload encryption system's configuration and should pass before other payload-related tests are executed.
- Feedback on the structure or additional validation requirements is welcome during the review process to ensure it meets the needs of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Script](../test_12_env_payload.sh) - Link to the test script for navigation on GitHub.
