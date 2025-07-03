# Log Output Library Documentation

## Overview

The Log Output Library (`log_output.sh`) provides consistent logging, formatting, and display functions for test scripts in the Hydrogen test suite. This library was created as part of the migration from `support_utils.sh` to modular, focused scripts and features a complete rewrite with numbered output system.

## Library Information

- **Script**: `../lib/log_output.sh`
- **Version**: 2.0.0 (with 2.1.0 features)
- **Created**: 2025-07-02
- **Updated**: 2025-07-02 - Complete rewrite with numbered output system
- **Purpose**: Extracted from support_utils.sh migration to provide modular logging functionality

## Purpose

- Standardize output formatting across all test scripts
- Provide consistent color coding and iconography
- Enable clear visual distinction between different types of messages
- Support both terminal and log file output

## Key Features

- **Color-coded output** with fallback for non-color terminals
- **Consistent iconography** for different message types
- **Formatted headers** for test sections
- **Summary reporting** functions
- **Command display** with path normalization

## Functions

### Core Functions

#### `print_log_output_version()`

Displays the library name and version information.

**Usage:**

```bash
print_log_output_version
```

### Test Numbering and Tracking

#### `set_test_number(number)`

Sets the current test number for output prefixing.

**Parameters:**

- `number`: Test number (e.g., "10")

**Usage:**

```bash
set_test_number "10"
```

#### `set_subtest_number(number)`

Sets the current subtest number for output prefixing.

**Parameters:**

- `number`: Subtest number (e.g., "001")

**Usage:**

```bash
set_subtest_number "001"
```

#### `extract_test_number(script_path)`

Extracts the test number from a script filename using regex pattern matching.

**Parameters:**

- `script_path`: Path to the test script

**Returns:**

- Test number as string (e.g., "10" from "test_10_compilation.sh")
- "XX" if no number found

**Usage:**

```bash
test_num=$(extract_test_number "$0")
```

#### `next_subtest()`

Increments the subtest counter and sets the current subtest number in 3-digit format.

**Features:**

- Auto-increments SUBTEST_COUNTER
- Sets CURRENT_SUBTEST_NUMBER in format "001", "002", etc.

**Usage:**

```bash
next_subtest
echo "Current subtest: $CURRENT_SUBTEST_NUMBER"
```

#### `reset_subtest_counter()`

Resets the subtest counter to 0 and clears the current subtest number.

**Usage:**

```bash
reset_subtest_counter
```

#### `get_test_prefix()`

Gets the current test prefix for output formatting.

**Returns:**

- "XX-YYY" format if both test and subtest numbers are set
- "XX" format if only test number is set
- "XX" if no numbers are set

**Usage:**

```bash
prefix=$(get_test_prefix)
echo "Current prefix: $prefix"
```

### Timing and Statistics

#### `start_test_timer()`

Starts the test timer and resets pass/fail counters.

**Features:**

- Records start time with millisecond precision
- Resets TEST_PASSED_COUNT and TEST_FAILED_COUNT to 0

**Usage:**

```bash
start_test_timer
```

#### `record_test_result(status)`

Records a test result for statistics tracking.

**Parameters:**

- `status`: Exit code (0 for pass, non-zero for fail)

**Features:**

- Increments TEST_PASSED_COUNT on success
- Increments TEST_FAILED_COUNT on failure

**Usage:**

```bash
record_test_result 0  # Record a pass
record_test_result 1  # Record a fail
```

#### `get_elapsed_time()`

Calculates elapsed time since test timer started.

**Returns:**

- Time in format "SSS.ZZZ" (e.g., "001.234")
- "000.000" if timer not started

**Features:**

- Uses bc for precise floating-point calculation
- Formats with at least 3 leading zeros for seconds

**Usage:**

```bash
elapsed=$(get_elapsed_time)
echo "Elapsed: ${elapsed}s"
```

#### `format_file_size(file_size)`

Formats file size with thousands separators.

**Parameters:**

- `file_size`: File size in bytes

**Returns:**

- Formatted file size with commas (e.g., "1,234,567")

**Usage:**

```bash
size=$(format_file_size "1234567")
echo "File size: $size bytes"
```

### Color and Icon Definitions

The library defines standard colors and icons used throughout the test suite:

```bash
# Colors
GREEN='\033[0;32m'           # Success messages
RED='\033[0;31m'             # Error messages  
YELLOW='\033[0;33m'          # Warning messages
BLUE='\033[0;34m'            # Headers and info
CYAN='\033[0;36m'            # Informational messages
BOLD='\033[1m'               # Bold text
DATA_COLOR='\033[38;5;229m'  # Pale yellow for DATA lines (256-color)
NC='\033[0m'                 # No color (reset)

# Icons
PASS_ICON="✅"               # Success indicator
FAIL_ICON="✖️"               # Failure indicator
WARN_ICON="⚠️"               # Warning indicator
INFO_ICON="ℹ️"               # Information indicator
DATA_ICON="▶▶"               # Data output indicator
```

### Test Header Functions

#### `print_test_header(test_name, script_version)`

Prints a beautiful test header using tables.sh with proper columns for Test#, Test Title, Version, and Started timestamp.

**Parameters:**

- `test_name`: Name of the test
- `script_version`: Version of the test script

**Features:**

- Uses tables.sh for professional table formatting
- Automatically starts test timer
- Creates timestamp with milliseconds
- Includes test ID in format "XX-000"
- Provides fallback formatting if tables.sh not available

**Usage:**

```bash
print_test_header "Hydrogen Compilation Test" "1.0.0"
```

#### `print_test(test_name)` (Legacy)

Prints a test header in legacy format with decorative borders.

**Parameters:**

- `test_name`: Name of the test

**Features:**

- Legacy function for backward compatibility
- Uses simple border formatting
- Includes timestamp

**Usage:**

```bash
print_test "My Test Suite"
```

#### `print_subtest(subtest_name)`

Prints a formatted subtest header with timing and numbering.

**Parameters:**

- `subtest_name`: Name of the subtest

**Features:**

- Shows current test-subtest number (e.g., "10-001")
- Displays elapsed time since test start
- Uses blue color formatting

**Usage:**

```bash
print_subtest "Check CMake Availability"
```

#### `print_test_suite_header(test_name, script_version)`

Prints a test suite header using tables.sh with blue theme.

**Parameters:**

- `test_name`: Name of the test suite
- `script_version`: Version of the test script

**Features:**

- Uses blue theme for suite-level headers
- Similar to print_test_header but with different styling
- Starts test timer and creates timestamp

**Usage:**

```bash
print_test_suite_header "Complete Test Suite" "2.0.0"
```

### Core Output Functions

#### `print_command(command)`

Prints a command that will be executed with path normalization and truncation.

**Parameters:**

- `command`: The command string to display

**Features:**

- Shows test prefix and elapsed time
- Converts absolute paths to relative paths
- Truncates commands longer than 200 characters
- Uses yellow color with arrow icon (➡️ EXEC)

**Usage:**

```bash
print_command "cmake --build build --target all"
```

#### `print_output(message)`

Prints command output or data with special formatting.

**Parameters:**

- `message`: The output message to display

**Features:**

- Shows test prefix and elapsed time
- Converts absolute paths to relative paths
- Skips empty or whitespace-only messages
- Uses pale yellow color with data icon (▶▶ DATA)

**Usage:**

```bash
print_output "Build completed successfully"
```

#### `print_result(status, message)`

Prints a test result with appropriate icon and color coding.

**Parameters:**

- `status`: Exit code (0 for success, non-zero for failure)
- `message`: The result message to display

**Features:**

- Shows test prefix and elapsed time
- Records result for statistics tracking
- Uses green/red color with pass/fail icons
- Automatically updates TEST_PASSED_COUNT/TEST_FAILED_COUNT

**Usage:**

```bash
print_result 0 "All tests passed successfully"
print_result 1 "Test failed with errors"
```

#### `print_warning(message)`

Prints a warning message with yellow color and warning icon.

**Parameters:**

- `message`: The warning message

**Features:**

- Shows test prefix and elapsed time
- Uses yellow color with warning icon (⚠️ WARN)

**Usage:**

```bash
print_warning "Configuration file not found, using defaults"
```

#### `print_error(message)`

Prints an error message with red color and error icon.

**Parameters:**

- `message`: The error message

**Features:**

- Shows test prefix and elapsed time
- Uses red color with error icon (✖️ ERROR)

**Usage:**

```bash
print_error "Failed to connect to database"
```

#### `print_message(message)`

Prints an informational message with cyan color and info icon.

**Parameters:**

- `message`: The informational message

**Features:**

- Shows test prefix and elapsed time
- Converts absolute paths to relative paths when available
- Uses cyan color with info icon (ℹ️ INFO)

**Usage:**

```bash
print_message "Starting server initialization..."
```

#### `print_newline()`

Prints a controlled newline for spacing.

**Usage:**

```bash
print_newline
```

### Test Completion Functions

#### `print_test_completion(test_name)`

Prints a beautiful test completion table using tables.sh with comprehensive statistics.

**Parameters:**

- `test_name`: Name of the test

**Features:**

- Uses tables.sh with blue theme for professional formatting
- Shows Test#, Test Name, Tests (total), Pass, Fail, and Elapsed columns
- Automatically calculates statistics from recorded results
- Displays elapsed time in SSS.ZZZ format
- Provides fallback formatting if tables.sh not available

**Usage:**

```bash
print_test_completion "Hydrogen Compilation Test"
```

#### `print_test_summary(total, passed, failed)` (Legacy)

Prints a formatted summary of test results in legacy format.

**Parameters:**

- `total`: Total number of tests
- `passed`: Number of passed tests  
- `failed`: Number of failed tests

**Features:**

- Legacy function for backward compatibility
- Uses decorative borders
- Shows overall pass/fail result
- Includes completion timestamp

**Usage:**

```bash
print_test_summary 10 8 2
```

#### `print_test_item(status, name, details)`

Prints an individual test item in a summary list.

**Parameters:**

- `status`: Test result (0 for pass, non-zero for fail)
- `name`: Test name
- `details`: Additional details about the test

**Features:**

- Shows test prefix
- Uses pass/fail icons and colors
- Formats name in bold

**Usage:**

```bash
print_test_item 0 "Compilation Test" "All variants built successfully"
```

### Legacy Compatibility Functions

#### `print_header(message)` (Deprecated)

Legacy function that issues a deprecation warning and calls print_test().

**Parameters:**

- `message`: The header text to display

**Features:**

- Issues deprecation warning
- Calls print_test() for backward compatibility

**Usage:**

```bash
print_header "Running Tests"  # Will show deprecation warning
```

#### `print_info(message)` (Deprecated)

Legacy function that issues a deprecation warning and calls print_message().

**Parameters:**

- `message`: The informational message

**Features:**

- Issues deprecation warning
- Calls print_message() for backward compatibility

**Usage:**

```bash
print_info "Starting process"  # Will show deprecation warning
```

## Usage

### Basic Usage

```bash
#!/bin/bash
# Source the log output library
source "lib/log_output.sh"

# Use the functions
print_header "My Test Suite"
print_info "Starting tests..."
print_result 0 "Test completed successfully"
```

### Integration with Other Libraries

The Log Output Library is designed to be sourced first, as other libraries may depend on its functions:

```bash
#!/bin/bash
# Source libraries in order
source "lib/log_output.sh"      # First - provides output functions
source "lib/file_utils.sh"      # Second - may use print_* functions
source "lib/test_framework.sh"  # Third - uses output functions
```

## Dependencies

- **None** - This library has no external dependencies
- **Terminal support** - Colors and icons work best in modern terminals
- **Bash 4.0+** - Uses modern bash features

## Migration Notes

This library replaces the following functions from `support_utils.sh`:

- All color definitions (GREEN, RED, YELLOW, etc.)
- All icon definitions (PASS_ICON, FAIL_ICON, etc.)
- `print_header()`, `print_result()`, `print_info()`, `print_warning()`, `print_error()`
- `print_command()`, `print_test_summary()`, `print_test_item()`

## Version History

- **2.1.0** (2025-07-02) - Added DATA_ICON and updated DATA_COLOR to pale yellow (256-color palette) for test_30_unity_tests.sh
- **2.0.0** (2025-07-02) - Complete rewrite with numbered output system
- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [File Utils Library](file_utils.md) - File and path utilities
- [Test Framework Library](framework.md) - Test lifecycle management
- [LIBRARIES.md](LIBRARIES.md) - Library index
