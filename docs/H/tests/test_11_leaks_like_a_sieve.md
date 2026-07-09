# Test 11: Memory Leak Detection

## Overview

The [`test_11_leaks_like_a_sieve.sh`](/elements/001-hydrogen/hydrogen/tests/test_11_leaks_like_a_sieve.sh) script performs memory leak detection for the Hydrogen server using AddressSanitizer (ASAN) in a debug build.

## Purpose

This test validates that the Hydrogen server:

- Does not leak memory during normal operation
- Properly manages allocated resources
- Handles memory operations safely
- Releases all allocated memory on graceful shutdown

The test uses ASAN's LeakSanitizer to detect direct and indirect memory leaks.

## Test Configuration

- **Test Name**: Memory Leak Detection
- **Test Abbreviation**: SIV
- **Test Number**: 11
- **Version**: 4.1.0

## Key Features

### ASAN Integration

The test requires a debug build (`hydrogen_debug`) with AddressSanitizer enabled:

```bash
# Verify ASAN is compiled in
readelf -s hydrogen_debug | grep __asan
```

ASAN_OPTIONS configured:

 ```text
 detect_leaks=1      # Enable leak detection
 leak_check_at_exit=1  # Check for leaks at shutdown
 verbosity=1          # Show detailed output
 log_threads=1        # Track thread-related issues
 print_stats=1        # Print memory statistics
 ```

### Minimal Configuration

Uses `hydrogen_test_11_leaks.json` with:

- Port 5030
- Minimal subsystems to reduce noise
- No databases (pure infrastructure test)

This differs from `test_41_exercise_asan.sh` / `test_44_exercise_native.sh` which test database-heavy paths.

## Test Flow

1. **Locate Debug Build**: Find `hydrogen_debug` binary
2. **Validate ASAN**: Confirm `__asan` symbols present
3. **Validate Config**: Check configuration file exists
4. **Start Server**: Launch with ASAN_OPTIONS
5. **Wait for Ready**: Poll for `STARTUP COMPLETE` (5s timeout)
6. **Exercise Operations**: Make basic API calls to `/api/system/health`
7. **Shutdown**: Send SIGTERM to trigger graceful shutdown
8. **Analyze Output**: Grep log for LeakSanitizer report

## Leak Detection

### Leak Categories

| Type | Description |
|------|-------------|
| **Direct Leak** | Memory allocated and never freed, no pointers to it |
| **Indirect Leak** | Memory reachable only through leaked pointers |

### Analysis

The test scans the server log for:

- `Direct leak of X bytes in Y allocations`
- `Indirect leak of X bytes in Y allocations`

If both counts are zero, the test passes.

## Expected Results

### Success Criteria

- Debug build with ASAN found
- Server starts within 5 seconds
- Server logs startup time
- Server shuts down gracefully after SIGTERM
- LeakSanitizer reports 0 direct and 0 indirect leaks

### Output Artifacts

- **`test_11_{timestamp}_leaks.log`**: Full server output including ASAN report
- **`test_11_{timestamp}_leak_report.log`**: Copy of leak analysis
- **`test_11_{timestamp}_leak_summary.log`**: Summary of leak counts

### Sample Output

```text
Memory Leak Analysis Summary
============================
Direct Leaks Found: 0
Indirect Leaks Found: 0

No memory leaks detected
```

## Troubleshooting

### Common Issues

- **Debug build not found**: Build with `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- **No ASAN symbols**: Ensure `-fsanitize=address` is in debug build flags
- **Startup timeout**: Check for startup errors in log
- **False positives**: Some system libraries may report minor leaks; focus on Hydrogen code in stack traces

### Manual ASAN Testing

Run Hydrogen directly under ASAN:

```bash
ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1" \
    valgrind --suppressions=.valgrind.supp \
    ./hydrogen_debug tests/configs/hydrogen_test_11_leaks.json
```

## Integration

This test runs as part of the full test suite:

```bash
# Run this specific test
./tests/test_11_leaks_like_a_sieve.sh

# Run as part of suite
./tests/test_00_all.sh 11_leaks_like_a_sieve
```

## Related Documentation

- [test_41_exercise.md](/docs/H/tests/test_41_exercise.md) - Database-heavy ASAN/LSAN exercise
- [test_44_exercise_native.md](/docs/H/tests/test_44_exercise_native.md) - Database-heavy native RSS exercise
- [lib/framework.md](/docs/H/tests/framework.md) - Test framework utilities