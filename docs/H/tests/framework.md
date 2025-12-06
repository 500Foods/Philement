# Test Framework Library Documentation

## Overview

The Test Framework Library (`framework.sh`) provides test lifecycle management and result tracking functions for test scripts in the Hydrogen test suite. This library was created as part of the migration from `support_utils.sh` to modular, focused scripts and integrates with the numbered output system from log_output.sh.

## Library Information

- **Script**: `../lib/framework.sh`
- **Version**: 2.0.0
- **Created**: 2025-07-02
- **Updated**: 2025-07-02 - Integrated with numbered output system
- **Purpose**: Extracted from support_utils.sh migration to provide modular test framework functionality

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

### Core Functions

#### `print_test_framework_version()`

Displays the library name and version information.

**Usage:**

```bash
print_test_framework_version
```

### Test Lifecycle Management

#### `start_test(test_name, script_version, script_path)`

Starts a test run with proper header and numbering integration.

**Parameters:**

- `test_name`: Name of the test being started
- `script_version`: Version of the test script
- `script_path`: Path to the test script (for auto-extracting test number)

**Features:**

- Auto-extracts test number from script filename
- Integrates with numbered output system from log_output.sh
- Sets test number and resets subtest counter
- Provides fallback formatting if log_output.sh not available

**Usage:**

```bash
start_test "Hydrogen Compilation Test" "1.0.0" "$0"
```

#### `start_subtest(subtest_name, subtest_number)`

Starts a subtest with proper header and numbering.

**Parameters:**

- `subtest_name`: Name of the subtest
- `subtest_number`: Number of the subtest

**Features:**

- Sets subtest number in log_output.sh
- Provides consistent subtest formatting
- Integrates with numbered output system

**Usage:**

```bash
start_subtest "Build Debug Variant" "1"
```

#### `end_test(test_result, total_subtests, passed_subtests)`

Ends a test run with proper summary and statistics.

**Parameters:**

- `test_result`: Exit code (0 for success, non-zero for failure)
- `total_subtests`: Total number of subtests executed
- `passed_subtests`: Number of subtests that passed

**Returns:**

- The same exit code that was passed in

**Features:**

- Calculates failed subtests automatically
- Uses print_test_summary() from log_output.sh
- Provides fallback formatting if log_output.sh not available

**Usage:**

```bash
end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT
```

### Environment Setup

#### `setup_test_environment(test_name, test_number)`

Sets up the standard test environment with directories and log files.

**Parameters:**

- `test_name`: Name of the test for log file naming
- `test_number`: Test number for file naming

**Returns:**

- Path to the created log file (echoed to stdout)

**Features:**

- Creates results directory if it doesn't exist
- Generates timestamp-based log file names with test ID
- Automatically calls start_test() and logs to file
- Sets up global variables (RESULTS_DIR, TIMESTAMP, RESULT_LOG)

**Usage:**

```bash
log_file=$(setup_test_environment "Compilation Test" "10")
echo "Test log: $log_file"
```

#### `navigate_to_project_root(script_dir)`

Navigates to the project root directory from the test script location.

**Parameters:**

- `script_dir`: Directory where the test script is located

**Returns:**

- `0`: Success
- `1`: Failure

**Features:**

- Uses safe_cd() from file_utils.sh if available
- Provides error messaging through print_error() if available
- Handles navigation failures gracefully

**Usage:**

```bash
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if navigate_to_project_root "$script_dir"; then
    echo "Successfully navigated to project root"
fi
```

### Result Tracking and Export

#### `export_subtest_results(test_name, total_subtests, passed_subtests)`

Exports subtest statistics for use by test_all.sh.

**Parameters:**

- `test_name`: Name of the test (used for file naming)
- `total_subtests`: Total number of subtests executed
- `passed_subtests`: Number of subtests that passed

**Features:**

- Creates results file in format: `subtest_${test_name}.txt`
- Stores data as comma-separated values: `total,passed`
- Operates silently (no output announcements)
- Creates results directory if it doesn't exist

**Usage:**

```bash
export_subtest_results "compilation" 7 6
```

#### `export_test_results(test_name, result, details, output_file)`

Exports test results to a standardized JSON format.

**Parameters:**

- `test_name`: Name of the test
- `result`: Test result (0 for pass, non-zero for fail)
- `details`: Additional details about the test
- `output_file`: Path to the JSON output file

**Features:**

- Creates structured JSON output with timestamp
- Suitable for automated processing
- Includes test status and details

**Usage:**

```bash
export_test_results "Compilation Test" 0 "All variants built successfully" "results.json"
```

### Check and Evaluation Functions

#### `run_check(check_name, check_command, passed_checks_var, failed_checks_var)`

Runs a check and tracks pass/fail counts with output.

**Parameters:**

- `check_name`: Descriptive name of the check
- `check_command`: Command to execute for the check
- `passed_checks_var`: Variable name to increment on pass
- `failed_checks_var`: Variable name to increment on fail

**Returns:**

- `0`: Check passed
- `1`: Check failed

**Features:**

- Prints the command being executed
- Provides visual feedback (✓ or ✗) with appropriate messaging
- Updates pass/fail counters automatically
- Uses print_message/print_warning from log_output.sh when available

**Usage:**

```bash
PASSED=0
FAILED=0
run_check "File exists" "[ -f 'myfile.txt' ]" "PASSED" "FAILED"
```

#### `evaluate_test_result(test_name, failed_checks, pass_count_var, exit_code_var)`

Evaluates test results and updates pass count with output.

**Parameters:**

- `test_name`: Name of the test being evaluated
- `failed_checks`: Number of failed checks
- `pass_count_var`: Variable name to increment on overall pass
- `exit_code_var`: Variable name to set to 1 on overall fail

**Features:**

- Prints PASSED/FAILED result with appropriate formatting
- Updates pass count only on success
- Sets exit code on failure
- Uses print_result() from log_output.sh when available

**Usage:**

```bash
PASS_COUNT=0
EXIT_CODE=0
evaluate_test_result "Compilation" $FAILED_CHECKS "PASS_COUNT" "EXIT_CODE"
```

#### `evaluate_test_result_silent(test_name, failed_checks, pass_count_var, exit_code_var)`

Evaluates test results silently (no output, just update counts).

**Parameters:**

- `test_name`: Name of the test being evaluated
- `failed_checks`: Number of failed checks
- `pass_count_var`: Variable name to increment on overall pass
- `exit_code_var`: Variable name to set to 1 on overall fail

**Features:**

- Updates counters without producing output
- Avoids duplicate PASS messages
- Useful for batch processing

**Usage:**

```bash
PASS_COUNT=0
EXIT_CODE=0
evaluate_test_result_silent "Compilation" $FAILED_CHECKS "PASS_COUNT" "EXIT_CODE"
```

### Report Generation

#### `generate_html_report(result_file)`

Generates an HTML summary report from a test log file.

**Parameters:**

- `result_file`: Path to the test log file

**Features:**

- Creates HTML file with same base name as log file
- Includes comprehensive CSS styling for professional appearance
- Provides structured layout with test numbers and color coding
- Includes timestamp and metadata
- Handles missing log files gracefully

**Usage:**

```bash
generate_html_report "$RESULT_LOG"
```

### Test Suite Management

#### `start_test_suite(test_name)`

Initializes a test suite with tracking variables.

**Parameters:**

- `test_name`: Name of the test suite

**Features:**

- Initializes PASSED_TESTS, FAILED_TESTS, CURRENT_SUBTEST variables
- Provides informational output about suite start
- Uses print_info() from log_output.sh when available

**Usage:**

```bash
start_test_suite "Hydrogen Complete Test Suite"
```

#### `end_test_suite()`

Completes a test suite and displays summary statistics.

**Features:**

- Calculates total tests from PASSED_TESTS + FAILED_TESTS
- Displays comprehensive summary
- Uses print_info() from log_output.sh when available

**Usage:**

```bash
end_test_suite
```

### Test Counting Functions

#### `increment_passed_tests()`

Increments the PASSED_TESTS counter.

#### `increment_failed_tests()`

Increments the FAILED_TESTS counter.

#### `get_passed_tests()`

Returns the current value of PASSED_TESTS.

#### `get_failed_tests()`

Returns the current value of FAILED_TESTS.

#### `get_total_tests()`

Returns the sum of PASSED_TESTS + FAILED_TESTS.

#### `get_exit_code()`

Returns 1 if FAILED_TESTS > 0, otherwise returns 0.

### Subtest Management

#### `print_subtest_header(subtest_name)`

Prints a subtest header and increments the CURRENT_SUBTEST counter.

**Parameters:**

- `subtest_name`: Name of the subtest

**Features:**

- Auto-increments CURRENT_SUBTEST counter
- Uses print_subtest_header() from log_output.sh when available
- Provides fallback formatting

**Usage:**

```bash
print_subtest_header "Build Debug Variant"
```

#### `skip_remaining_subtests(reason)`

Prints a warning about skipping remaining subtests.

**Parameters:**

- `reason`: Reason for skipping subtests

**Features:**

- Uses print_warning() from log_output.sh when available
- Provides clear indication of test skipping

**Usage:**

```bash
skip_remaining_subtests "Build environment not available"
```

### Legacy Compatibility

#### `print_header()` (Deprecated)

Legacy function that issues a deprecation warning.

**Features:**

- Warns users to migrate to start_test() or start_subtest()
- Maintains backward compatibility temporarily

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

- **2.0.0** (2025-07-02) - Updated to integrate with numbered output system
- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [Log Output Library](log_output.md) - Output formatting and logging
- [File Utils Library](file_utils.md) - File and path utilities
- [LIBRARIES.md](LIBRARIES.md) - Library index
