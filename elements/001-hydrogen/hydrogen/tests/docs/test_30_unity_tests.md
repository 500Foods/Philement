# Test 30 Unity Tests Script Documentation

## Overview

The `test_30_unity_tests.sh` script is a unit testing tool within the Hydrogen test suite, focused on running unit tests for the Hydrogen project using the Unity testing framework.

## Purpose

This script validates the functionality of core components of the Hydrogen server through unit tests written in C, ensuring that individual units of code work as expected before integration testing. It is essential for maintaining code quality and reliability at a granular level.

## Key Features

- **Unity Framework Integration**: Automatically downloads the Unity framework if not present in `tests/unity/framework/Unity/`.
- **Unit Test Execution**: Compiles and executes unit tests for core components.
- **Subtest Reporting**: Treats each test file as a subtest for detailed reporting.
- **Comprehensive Results**: Provides detailed test results and diagnostics in `./results/` and `./logs/` directories.
- **Automation Integration**: Integrates with `test_00_all.sh` for automated test execution.

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_30_unity_tests.sh
```

## Additional Notes

- This test is crucial for validating individual components of the Hydrogen codebase, ensuring that issues are caught early in the development cycle.
- For more information on the Unity framework and manual installation instructions, refer to the [Unity Test Framework README](../unity/README.md).
- Feedback on the structure or additional unit tests to include is welcome during the review process to ensure it meets the needs of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Unity Framework README](../unity/README.md) - Information on the Unity testing framework.
- [Script](../test_30_unity_tests.sh) - Link to the test script for navigation on GitHub.
