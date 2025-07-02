# Test 00 All Script Documentation

## Overview

The `test_00_all.sh` script serves as the primary test orchestration tool for the Hydrogen test suite. It executes all test scripts in sequence, ensuring comprehensive validation of the Hydrogen 3D printer control server.

## Purpose

This script is designed to automate the execution of the entire test suite, providing a single entry point for running all tests with various configurations. It handles cleanup, compilation verification, test execution, and result summarization, ensuring a consistent testing process.

## Key Features

- **Complete Cleanup**: Performs cleanup of test output directories (`./logs/`, `./results/`, `./diagnostics/`) before starting.
- **Formatted Output**: Provides formatted output with test results for better readability.
- **Automatic Permissions**: Automatically makes test scripts executable.
- **Special Handling for Compilation**: Runs `test_10_compilation.sh` first as a prerequisite, skipping remaining tests if compilation fails.
- **Dynamic Test Discovery**: Dynamically discovers and runs all other tests in numerical order.
- **Comprehensive Summary**: Generates a summary with individual test results, subtest counts, visual pass/fail indicators, and total statistics.
- **Skip Option**: Can skip actual test execution while showing what would run using the `--skip-tests` flag.
- **README Update**: Always updates `README.md` with test results and code statistics.

## Usage

To run all tests with both minimal and maximal configurations:

```bash
./test_00_all.sh all
```

To run with minimal configuration only:

```bash
./test_00_all.sh min
```

To run with maximal configuration only:

```bash
./test_00_all.sh max
```

To skip test execution (for quick README updates):

```bash
./test_00_all.sh --skip-tests
```

## Additional Notes

- This script is the central orchestrator for the test suite and should be used as the primary method to run all tests.
- It integrates with `support_cleanup.sh` for environment preparation and uses standardized formatting from `support_utils.sh`.
- Feedback on the structure or execution flow of this script is welcome during the review process to ensure it meets the needs of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Script](../test_00_all.sh) - Link to the test script for navigation on GitHub.
