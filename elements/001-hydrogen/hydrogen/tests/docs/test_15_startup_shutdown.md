# Test 15 Startup Shutdown Script Documentation

## Overview

The `test_15_startup_shutdown.sh` script is a comprehensive test tool within the Hydrogen test suite, focused on validating the startup and shutdown sequence of the Hydrogen server with both minimal and maximal configurations.

## Purpose

This script ensures that the core lifecycle management of the Hydrogen server functions correctly, testing the initialization and termination processes under different configuration scenarios. It is crucial for verifying the stability and reliability of the server's basic operations.

## Key Features

- **Dual Configuration Testing**: Tests both minimal and maximal configurations in sequence.
- **Subtest Reporting**: Reports on 6 subtests (3 per configuration):
  1. Startup success.
  2. Shutdown timing.
  3. Clean shutdown verification.
- **Diagnostics Collection**: Provides detailed diagnostics including thread states, stack traces, open file descriptors, resource usage, and shutdown sequence analysis.
- **Organized Logging**: Creates organized logs in the `./results` directory for review.
- **Standardized Formatting**: Uses formatting from `support_utils.sh` for consistent output.

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_15_startup_shutdown.sh
```

## Additional Notes

- This test is essential for validating the fundamental lifecycle of the Hydrogen server and should be run early in the test sequence to ensure basic functionality.
- Feedback on the structure, additional subtests, or diagnostic needs is welcome during the review process to ensure it meets the requirements of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Script](../test_15_startup_shutdown.sh) - Link to the test script for navigation on GitHub.
