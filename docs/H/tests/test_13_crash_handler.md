# Test 13: Crash Handler Script Documentation

## Overview

The `test_20_crash_handler.sh` script (Test 20: Crash Handler) is a comprehensive crash recovery testing tool within the Hydrogen test suite, focused on validating the functionality of the crash handler across multiple build variants.

## Purpose

This script ensures that the Hydrogen server can generate core dumps and handle crashes appropriately, providing valuable debugging information for developers. It is essential for verifying the server's ability to recover from or log unexpected failures across all build configurations.

## Script Details

- **Script Name**: `test_13_crash_handler.sh`
- **Test Number**: 20
- **Version**: 3.0.0 (Complete rewrite using modular test libraries)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Multi-Build Testing**: Tests crash handler for all available build variants (hydrogen, hydrogen_debug, hydrogen_valgrind, hydrogen_perf, hydrogen_release)
- **Core Dump Generation**: Validates core dump creation with proper naming format `binary.core.pid`
- **Debug Symbol Validation**: Verifies presence/absence of debug symbols based on build type
- **Crash Simulation**: Uses `SIGUSR1` signal to trigger controlled crash via `test_crash_handler`
- **GDB Analysis**: Performs comprehensive GDB backtrace analysis with build-specific expectations

### Advanced Testing

- **Core Dump Configuration**: Verifies system core dump settings and ulimit configuration
- **Core File Content Validation**: Ensures core files are valid ELF core dumps with adequate size
- **Log Message Verification**: Checks for specific crash handler messages:
  - "Received SIGUSR1, simulating crash via null dereference"
  - "Signal 11 received...generating core dump"
- **Timeout Management**: Configurable startup (10s) and crash (5s) timeouts
- **Memory Leak Detection**: Disables ASAN leak detection during crash testing

### Build-Specific Behavior

- **Debug Builds**: Expects full debug symbols and detailed GDB backtraces
- **Release Builds**: Accepts absence of debug symbols and basic crash information
- **Valgrind/Perf Builds**: Specialized handling for analysis builds

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking
- `lifecycle.sh` - Process lifecycle management

### Test Sequence

1. **Configuration Validation**: Verifies test configuration file exists
2. **Core Dump Setup**: Checks system core dump configuration
3. **Build Discovery**: Automatically discovers available Hydrogen builds
4. **Per-Build Testing**: For each build variant:
   - Debug symbol verification
   - Core file generation testing
   - Crash handler logging validation
   - GDB analysis execution

### Results and Preservation

- **Results Directory**: `tests/results/`
- **GDB Analysis**: Saved to `results/gdb_analysis/`
- **Log Preservation**: Failed tests preserve core files and logs for debugging
- **Timestamped Output**: All results include timestamp for tracking

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_13_crash_handler.sh
```

## Configuration

The test uses minimal configuration (`hydrogen_test_min.json`) to reduce startup complexity and focus on crash handling functionality.

## Expected Results

### Successful Test Criteria

- Core dump configuration properly set
- All discovered builds pass 4 subtests each:
  1. Debug symbol verification (present/absent as appropriate)
  2. Core file generation
  3. Crash handler logging
  4. GDB backtrace analysis

### Build-Specific Expectations

- **Development Builds**: Full debug symbols and detailed backtraces
- **Release Builds**: No debug symbols, basic crash information
- **Analysis Builds**: Specialized handling for valgrind/perf variants

## Troubleshooting

### Common Issues

- **Core Dump Limits**: Script attempts to set `ulimit -c unlimited`
- **Missing Builds**: Test adapts to available builds automatically
- **GDB Analysis**: Different expectations for debug vs release builds
- **File Preservation**: Failed tests preserve artifacts for debugging

## Version History

- **3.0.0** (2025-07-02): Complete rewrite using modular test libraries
- **2.0.1** (2025-07-01): Updated to use predefined CMake build variants
- **2.0.0** (2025-06-17): Major refactoring with shellcheck compliance

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_18_signals.md](test_18_signals.md) - Signal handling tests
- [test_16_shutdown.md](test_16_shutdown.md) - Shutdown testing
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy