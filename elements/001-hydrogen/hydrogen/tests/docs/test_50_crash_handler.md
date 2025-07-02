# Test 50 Crash Handler Script Documentation

## Overview

The `test_50_crash_handler.sh` script is a crash recovery testing tool within the Hydrogen test suite, focused on validating the functionality of the crash handler.

## Purpose

This script ensures that the Hydrogen server can generate core dumps and handle crashes appropriately, providing valuable debugging information for developers. It is essential for verifying the server's ability to recover from or log unexpected failures.

## Key Features

- **Core Dump Generation**: Tests core dump generation for all build variants (standard, debug, valgrind, perf, release).
- **Core File Naming**: Verifies core files are created in the executable's directory with the format `binary.core.pid`.
- **Debug Symbols**: Validates the presence of debug symbols in non-release builds.
- **Crash Simulation**: Tests the crash handler using the `SIGUSR1` signal to trigger `test_crash_handler`.
- **GDB Analysis**: Performs GDB analysis of core dumps, verifying `test_crash_handler` in backtrace for debug builds and accepting basic stack trace for release builds.
- **Log Message Verification**: Checks for specific crash handler log messages like "Received SIGUSR1, simulating crash via null dereference" and "Signal 11 received...generating core dump".
- **Preservation on Failure**: Preserves core files and logs for failed tests.
- **Build Type Support**: Supports different build types with appropriate expectations (debug builds with full debug info, release builds with basic crash info, etc.).

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_50_crash_handler.sh
```

## Additional Notes

- This test is critical for ensuring that crashes are handled in a way that facilitates debugging and recovery, which is vital for maintaining server reliability.
- Feedback on the structure or additional crash scenarios to test is welcome during the review process to ensure it meets the needs of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Script](../test_50_crash_handler.sh) - Link to the test script for navigation on GitHub.
