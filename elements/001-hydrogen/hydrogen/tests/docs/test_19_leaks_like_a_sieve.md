# Test 10: Memory Leak Detection (test_10_leaks_like_a_sieve.sh)

## Overview

**test_10_leaks_like_a_sieve.sh** is a comprehensive memory leak detection test that uses AddressSanitizer (ASAN) to analyze the Hydrogen server for memory leaks, memory errors, and resource management issues.

## Purpose

This test validates that the Hydrogen server:

- Does not leak memory during normal operation
- Properly manages allocated resources
- Handles memory operations safely
- Releases all allocated memory on shutdown

## Test Configuration

- **Test Number**: 10
- **Category**: Memory Analysis
- **Dependencies**: ASAN-enabled build, minimal test configuration
- **Timeout**: Extended (due to ASAN overhead)

## Test Process

### ASAN Analysis

1. **Startup Testing**: Launches Hydrogen with ASAN supervision
2. **Memory Monitoring**: Tracks all memory allocations and deallocations
3. **Leak Detection**: Identifies any memory that is not properly freed
4. **Error Detection**: Catches memory access violations and buffer overflows

### Memory Categories Checked

- **Direct Leaks**: Memory that is leaked with no references
- **Indirect Leaks**: Memory that is lost due to pointer chain breaks

## Expected Results

- **Zero Memory Leaks**: No direct or indirect leaks
- **Clean Shutdown**: All allocated memory properly released
- **No Memory Errors**: No invalid memory access or corruption

## Usage

```bash
# Run memory leak detection test
./test_10_leaks_like_a_sieve.sh

# Run as part of test suite
./test_00_all.sh 10_leaks_like_a_sieve
```

## Troubleshooting

### Common Issues

- **False Positives**: Some system libraries may report minor leaks
- **Performance Impact**: ASAN slows execution
- **Large Output**: ASAN generates verbose debugging information

### Analysis Tips

- Focus on "Direct leak" and "Indirect leak" reports
- Check for patterns in leaked memory (repeated allocations)
- Review the call stack for leak sources

## Integration

This test integrates with:

- **test_00_all.sh**: Automatic execution in test suite
- **test_22_startup_shutdown.sh**: Complementary lifecycle testing
- **test_26_shutdown.sh**: Additional shutdown validation

## Performance Notes

- ASAN adds runtime overhead
- May require increased timeout values for complex operations
- Memory usage is also increased during analysis

For more information on memory debugging techniques, see the [Testing Documentation](../../docs/testing.md).