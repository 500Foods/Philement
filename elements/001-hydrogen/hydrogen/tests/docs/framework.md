# Test Framework Library Documentation

## Overview

The Test Framework Library (`lib/test_framework.sh`) provides test lifecycle management and result tracking functions for test scripts in the Hydrogen test suite. This library was created as part of the migration from `support_utils.sh` to modular, focused scripts.

## Purpose

- Standardize test lifecycle management across all test scripts
- Provide consistent test result tracking and reporting
- Enable integration with the main test runner (test_all.sh)
- Support HTML report generation for test results
- Facilitate test environment setup and teardown

## Key Features

- **Standardized test lifecycle** with consistent start/end patterns
- **Result tracking** and export for aggregation
- **Environment setup** with automatic directory and log file creation
- **HTML report generation** for detailed test documentation
- **Integration support** for test_all.sh coordination

## Functions

### Test Lifecycle Management

#### `start_test(test_name)`

Starts a test run with a properly formatted header and timestamp.

**Parameters:**

- `test_name` - The name of the test being started

**Features:**

- Uses print_header() from log_output.sh for consistent formatting
- Records start timestamp
- Provides fallback formatting if log_output.sh not available

**Example:**

```bash
start_test "Hydrogen Compilation Test"
```

#### `end_test(test_result, test_name)`

Ends a test run with a summary and final result display.

**Parameters:**

- `test_result` - Exit code (0 for success, non-zero for failure)
- `test_name` - The name of the test being ended

**Returns:**

- The same exit code that was passed in

**Features:**

- Displays completion timestamp
- Shows overall pass/fail result with appropriate formatting
- Uses colors and icons from log_output.sh when available

**Example:**

```bash
end_test $EXIT_CODE "Compilation Test"
```

### Environment Setup

#### `setup_test_environment(test_name)`

Sets up the standard test environment with directories and log files.

**Parameters:**

- `test_name` - The name of the test for log file naming

**Returns:**

- Path to the created log file

**Features:**

- Creates results directory if it doesn't exist
- Generates timestamp-based log file names
- Automatically calls start_test() and logs to file
- Sets up global variables (RESULTS_DIR, TIMESTAMP, RESULT_LOG)

**Example:**

```bash
log_file=$(setup_test_environment "My Test")
echo "Test log: $log_file"
```

### Result Tracking and Export

#### `export_subtest_results(test_name, total_subtests, passed_subtests)`

Exports subtest statistics for use by test_all.sh.

**Parameters:**

- `test_name` - Name of the test (used for file naming)
- `total_subtests` - Total number of subtests executed
- `passed_subtests` - Number of subtests that passed

**Features:**

- Creates a results file that test_all.sh can read
- Enables accurate aggregation of test results
- Provides informational output about export

**Example:**

```bash
export_subtest_results "compilation" 7 6
```

#### `export_test_results(test_name, result, details, output_file)`

Exports test results to a standardized JSON format.

**Parameters:**

- `test_name` - Name of the test
- `result` - Test result (0 for pass, non-zero for fail)
- `details` - Additional details about the test
- `output_file` - Path to the JSON output file

**Features:**

- Creates structured JSON output
- Includes timestamp information
- Suitable for automated processing

**Example:**

```bash
export_test_results "Compilation Test" 0 "All variants built successfully" "results.json"
```

### Report Generation

#### `generate_html_report(result_file)`

Generates an HTML summary report from a test log file.

**Parameters:**

- `result_file` - Path to the test log file

**Features:**

- Creates HTML file with same base name as log file
- Includes CSS styling for professional appearance
- Provides structured layout for test results
- Includes timestamp and metadata

**Example:**

```bash
generate_html_report "$RESULT_LOG"
```

### Utility Functions

#### `print_test_framework_version()`

Displays the library version information.

**Example:**

```bash
print_test_framework_version
```

## Usage

### Basic Test Structure

```bash
#!/bin/bash
# Source required libraries
source "lib/log_output.sh"
source "lib/test_framework.sh"

# Initialize test
EXIT_CODE=0
start_test "My Test Suite"

# Run test logic
if run_my_tests; then
    print_result 0 "All tests passed"
else
    print_result 1 "Some tests failed"
    EXIT_CODE=1
fi

# End test
end_test $EXIT_CODE "My Test Suite"
exit $EXIT_CODE
```

### Complete Test with Environment Setup

```bash
#!/bin/bash
source "lib/log_output.sh"
source "lib/test_framework.sh"

# Set up test environment
RESULT_LOG=$(setup_test_environment "Comprehensive Test")

# Initialize tracking
EXIT_CODE=0
TOTAL_SUBTESTS=3
PASS_COUNT=0

# Run subtests with logging
print_header "Running Subtest 1" | tee -a "$RESULT_LOG"
if run_subtest_1; then
    print_result 0 "Subtest 1 passed" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
else
    print_result 1 "Subtest 1 failed" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
fi

# Export results for test_all.sh
export_subtest_results "comprehensive" $TOTAL_SUBTESTS $PASS_COUNT

# Generate HTML report
generate_html_report "$RESULT_LOG"

# End test
end_test $EXIT_CODE "Comprehensive Test" | tee -a "$RESULT_LOG"
exit $EXIT_CODE
```

### Integration with test_all.sh

```bash
#!/bin/bash
# In your test script
source "lib/test_framework.sh"

# At the end of your test
export_subtest_results "mytest" $TOTAL_SUBTESTS $PASS_COUNT

# test_all.sh can then read the results file:
# results/subtest_mytest.txt contains "7,6" (total,passed)
```

## Dependencies

- **log_output.sh** - For consistent formatting and colors (recommended)
- **Bash 4.0+** - Uses modern bash features
- **Standard Unix tools** - date, mkdir, etc.

## File Structure

The library creates and manages these files:

```directory
tests/
├── results/
│   ├── test_YYYYMMDD_HHMMSS_testname.log    # Test log files
│   ├── test_YYYYMMDD_HHMMSS_testname.html   # HTML reports
│   └── subtest_testname.txt                 # Subtest results for test_all.sh
```

## Integration Points

### With log_output.sh

- Uses print_header(), print_info(), and color variables when available
- Provides fallback formatting when log_output.sh not sourced
- Maintains consistent visual style across test suite

### With test_all.sh

- export_subtest_results() creates files that test_all.sh reads
- Enables accurate aggregation of pass/fail counts
- Supports parallel test execution coordination

### With Individual Tests

- Provides standardized structure for all test scripts
- Handles common setup and teardown operations
- Manages log file creation and organization

## Migration Notes

This library replaces the following functions from `support_utils.sh`:

- `start_test()`, `end_test()` - Test lifecycle management
- `setup_test_environment()` - Environment setup
- `export_subtest_results()` - Result export for test_all.sh
- `export_test_results()` - JSON result export
- `generate_html_report()` - HTML report generation

## Version History

- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [Log Output Library](log_output.md) - Output formatting and logging
- [File Utils Library](file_utils.md) - File and path utilities
- [Migration Plan](Migration_Plan.md) - Overall migration strategy
- [LIBRARIES.md](LIBRARIES.md) - Library index
