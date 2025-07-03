# Test 96: Leaks Like a Sieve Script Documentation

## Overview

The `test_96_leaks_like_a_sieve.sh` script (Test 95: Memory Leak Detection) is a comprehensive memory testing tool within the Hydrogen test suite, focused on detecting memory leaks using AddressSanitizer (ASAN) with the debug build of the Hydrogen server.

## Purpose

This script ensures that the Hydrogen server does not suffer from memory leaks during normal operations, which could lead to performance degradation and resource exhaustion over time. It provides detailed memory leak analysis using ASAN's LeakSanitizer to maintain long-term stability and efficient resource management.

## Script Details

- **Script Name**: `test_96_leaks_like_a_sieve.sh`
- **Test Name**: Memory Leak Detection
- **Test Number**: 95
- **Version**: 3.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **ASAN Integration**: Uses AddressSanitizer's LeakSanitizer for comprehensive memory leak detection
- **Debug Build Validation**: Ensures debug build with ASAN support is available
- **Comprehensive ASAN Configuration**: Uses detailed ASAN options for thorough leak detection
- **API Exercise Testing**: Performs API calls to trigger memory allocation patterns
- **Detailed Leak Analysis**: Categorizes and reports direct and indirect memory leaks

### Advanced Testing Capabilities

- **Build Verification**: Validates debug build exists and has ASAN symbols
- **Configuration Validation**: Ensures proper test configuration is available
- **Startup Monitoring**: Monitors server startup with timeout handling
- **Memory Operation Simulation**: Exercises memory allocation through API calls
- **Graceful Shutdown**: Uses SIGTERM to trigger proper shutdown and leak detection

### Leak Detection Features

- **Direct Leak Detection**: Identifies memory directly allocated but not freed
- **Indirect Leak Detection**: Finds memory reachable only through leaked memory
- **Leak Categorization**: Separates and counts different types of leaks
- **Detailed Reporting**: Provides comprehensive leak analysis with stack traces
- **Summary Generation**: Creates human-readable leak summaries

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking
- `lifecycle.sh` - Process lifecycle management

### ASAN Configuration

- **Leak Detection**: `detect_leaks=1` - Enable leak detection
- **Exit Check**: `leak_check_at_exit=1` - Check for leaks on exit
- **Verbosity**: `verbosity=1` - Detailed output
- **Thread Logging**: `log_threads=1` - Include thread information
- **Statistics**: `print_stats=1` - Print allocation statistics

### Test Sequence (4 Subtests)

1. **Debug Build Validation**: Verifies debug build exists and has ASAN support
2. **Configuration Validation**: Ensures test configuration file is available
3. **Memory Leak Test Execution**: Runs server with ASAN and exercises memory operations
4. **Leak Analysis**: Analyzes ASAN output for memory leaks and generates reports

### Memory Exercise Operations

- **API Health Checks**: Multiple calls to `/api/system/info` endpoint
- **Memory Allocation Patterns**: Triggers various memory allocation scenarios
- **Timing Controls**: Allows memory operations to settle before shutdown
- **Graceful Termination**: Uses SIGTERM for proper shutdown sequence

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_96_leaks_like_a_sieve.sh
```

## Expected Results

### Successful Test Criteria

- Debug build with ASAN support found
- Configuration file validated
- Server starts successfully with ASAN enabled
- Memory operations complete without crashes
- No direct or indirect memory leaks detected
- Comprehensive leak analysis report generated

### Leak Analysis Output

- **Direct Leaks**: Memory allocated but never freed
- **Indirect Leaks**: Memory reachable only through leaked pointers
- **Leak Summary**: Categorized count of different leak types
- **Stack Traces**: Detailed allocation location information (when available)

## Troubleshooting

### Common Issues

- **Missing Debug Build**: Ensure `hydrogen_debug` is built with ASAN support
- **ASAN Not Enabled**: Verify debug build was compiled with `-fsanitize=address`
- **Configuration Missing**: Ensure `hydrogen_test_api_test_1.json` exists
- **Startup Failures**: Check server logs for configuration or dependency issues
- **False Positives**: Some system libraries may show as leaks (expected)

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: Detailed server execution logs with ASAN output
- **Leak Reports**: Complete ASAN leak detection output
- **Leak Summaries**: Human-readable analysis of detected leaks
- **Timestamped Output**: All results include timestamp for tracking

## Build Requirements

### Debug Build with ASAN

- **Compiler Flags**: Must include `-fsanitize=address -fsanitize=leak`
- **Debug Symbols**: Should include `-g` for detailed stack traces
- **ASAN Symbols**: Binary must contain `__asan` symbols
- **Runtime Libraries**: ASAN runtime libraries must be available

### Configuration Requirements

- **Test Configuration**: Uses `hydrogen_test_api_test_1.json`
- **API Endpoints**: Must enable system API endpoints for testing
- **Port Configuration**: Default port 5030 for API testing

## Memory Leak Categories

### Direct Leaks

- Memory allocated by the application but never freed
- Indicates missing `free()`, `delete`, or similar calls
- Most critical type of memory leak

### Indirect Leaks

- Memory reachable only through leaked pointers
- Often caused by not freeing container structures
- May be resolved by fixing direct leaks

## Version History

- **3.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **2.0.0** (2025-06-17): Major refactoring with improved modularity and enhanced comments
- **1.0.0** (2025-06-15): Initial version with basic memory leak detection

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_26_crash_handler.md](test_26_crash_handler.md) - Crash handler testing (also uses debug builds)
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [testing.md](../../docs/testing.md) - Overall testing strategy
- [coding_guidelines.md](../../docs/coding_guidelines.md) - Memory management guidelines
- [developer_onboarding.md](../../docs/developer_onboarding.md) - Development environment setup
