# Log Output Library Documentation

## Overview

The Log Output Library (`log_output.sh`) provides consistent logging, formatting, and display functions for test scripts in the Hydrogen test suite. This library features a comprehensive logging system with color-coded output, test numbering, timing, and beautiful table-based headers.

## Library Information

- **Script**: `../lib/log_output.sh`
- **Version**: 3.0.3
- **Created**: 2025-07-02
- **Updated**: 2025-07-03 - Applied color consistency to all output types, removed legacy functions
- **Purpose**: Provides modular logging functionality with modern numbered output system

## Purpose

- Standardize output formatting across all test scripts
- Provide consistent color coding and iconography
- Enable clear visual distinction between different types of messages
- Support test numbering, timing, and statistics tracking
- Generate beautiful table-based headers and completion summaries

## Key Features

- **Color-coded output** with consistent theming
- **Rounded rectangle icons** for different message types
- **Test/subtest numbering** with automatic incrementing
- **Precise timing** with millisecond accuracy
- **Statistics tracking** for pass/fail counts
- **Beautiful table headers** using tables.sh integration
- **Path normalization** for cleaner output
- **Command truncation** for readability

## Global Variables and Constants

### Test Numbering

- `CURRENT_TEST_NUMBER` - Current test number (e.g., "10")
- `CURRENT_SUBTEST_NUMBER` - Current subtest number (e.g., "001")
- `SUBTEST_COUNTER` - Auto-incrementing subtest counter

### Timing and Statistics

- `TEST_START_TIME` - Test start time with millisecond precision
- `TEST_PASSED_COUNT` - Number of passed tests
- `TEST_FAILED_COUNT` - Number of failed tests

### Colors and Icons

- **PASS_COLOR**: Green (`\033[0;32m`)
- **FAIL_COLOR**: Red (`\033[0;31m`)
- **WARN_COLOR**: Yellow (`\033[0;33m`)
- **INFO_COLOR**: Cyan (`\033[0;36m`)
- **DATA_COLOR**: Pale yellow (`\033[38;5;229m`) - 256-color palette
- **EXEC_COLOR**: Yellow (`\033[0;33m`)
- **TEST_COLOR**: Blue (`\033[0;34m`)

All icons are rounded rectangles (██) with matching colors:

- **PASS_ICON**: Green rectangles
- **FAIL_ICON**: Red rectangles
- **WARN_ICON**: Yellow rectangles
- **INFO_ICON**: Cyan rectangles
- **DATA_ICON**: Pale yellow rectangles
- **EXEC_ICON**: Yellow rectangles

## Functions

### Test Numbering and Timing Functions

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
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
```

#### `next_subtest()`

Increments the subtest counter and sets the current subtest number in 3-digit format.

**Features:**

- Auto-increments SUBTEST_COUNTER
- Sets CURRENT_SUBTEST_NUMBER in format "001", "002", etc.

**Usage:**

```bash
next_subtest
print_subtest "Check CMake Availability"
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

### Header and Completion Functions

#### `print_test_header(test_name, script_version)`

Prints a beautiful test header using tables.sh with proper columns.

**Parameters:**

- `test_name`: Name of the test
- `script_version`: Version of the test script

**Features:**

- Uses tables.sh for professional table formatting
- Automatically starts test timer
- Creates timestamp with milliseconds
- Includes test ID in format "XX-000"
- Shows Test#, Test Title, Version, and Started columns

**Usage:**

```bash
print_test_header "Hydrogen Compilation Test" "1.0.0"
```

#### `print_subtest(subtest_name)`

Prints a formatted subtest header with timing and numbering.

**Parameters:**

- `subtest_name`: Name of the subtest

**Features:**

- Shows current test-subtest number (e.g., "10-001")
- Displays elapsed time since test start
- Uses blue color formatting with bold text

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

#### `print_test_completion(test_name)`

Prints a beautiful test completion table using tables.sh with comprehensive statistics.

**Parameters:**

- `test_name`: Name of the test

**Features:**

- Uses tables.sh with blue theme for professional formatting
- Shows Test#, Test Name, Tests (total), Pass, Fail, and Elapsed columns
- Automatically calculates statistics from recorded results
- Displays elapsed time in SSS.ZZZ format

**Usage:**

```bash
print_test_completion "Hydrogen Compilation Test"
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
- Uses yellow color with EXEC icon (██ EXEC)
- **NEW**: Command content is now colored to match icon and label

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
- Uses pale yellow color with DATA icon (██ DATA)
- **NEW**: Message content is now colored to match icon and label

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
- Uses green/red color with pass/fail icons (██ PASS/██ FAIL)
- **NEW**: Result message content is now colored to match icon and label
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
- Uses yellow color with warning icon (██ WARN)

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
- Uses red color with error icon (██ ERROR)

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
- Uses cyan color with info icon (██ INFO)

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

## Usage

### Standard Test Script Pattern

```bash
#!/bin/bash

# Source the log output library
source "$(dirname "${BASH_SOURCE[0]}")/lib/log_output.sh"

# Initialize test numbering
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print test header
print_test_header "My Test Suite" "1.0.0"

# Run subtests
next_subtest
print_subtest "First Test"
print_message "Starting first test..."
print_result 0 "First test passed"

next_subtest
print_subtest "Second Test"
print_command "some-command --option"
print_output "Command output here"
print_result 0 "Second test passed"

# Print completion summary
print_test_completion "My Test Suite"
```

### Integration with Other Libraries

The Log Output Library should be sourced first, as other libraries may depend on its functions:

```bash
#!/bin/bash
# Source libraries in order
source "lib/log_output.sh"      # First - provides output functions
source "lib/file_utils.sh"      # Second - may use print_* functions
source "lib/framework.sh"       # Third - uses output functions
```

## Output Format

All output follows a consistent format:

```log
  XX-YYY   SSS.ZZZ   ██ TYPE   message content
```

Where:

- `XX-YYY`: Test and subtest number (e.g., "10-001")
- `SSS.ZZZ`: Elapsed time in seconds with milliseconds (e.g., "001.234")
- `██ TYPE`: Colored icon and type label (PASS, FAIL, WARN, ERROR, INFO, DATA, EXEC)
- `message content`: The actual message

## Dependencies

- **tables.sh** - For beautiful table formatting (optional, provides fallback)
- **bc** - For precise floating-point time calculations
- **Bash 4.0+** - Uses modern bash features
- **Terminal support** - Colors and icons work best in modern terminals

## Migration from Legacy Code

This library completely replaces legacy functions. The following functions have been **REMOVED**:

- ~~`print_header()`~~ - Use `print_test_header()` or `print_subtest()`
- ~~`print_info()`~~ - Use `print_message()`

All legacy functions have been completely removed as of version 3.0.2.

## Version History

- **3.0.3** (2025-07-03) - Applied color consistency to all output types (DATA, EXEC, PASS, FAIL)
- **3.0.2** (2025-07-03) - Completely removed legacy functions (print_header, print_info)
- **3.0.1** (2025-07-03) - Enhanced documentation, removed unused functions, improved comments
- **3.0.0** (2025-07-03) - General overhaul of colors and icons, removed legacy functions
- **2.1.0** (2025-07-02) - Added DATA_ICON and updated DATA_COLOR to pale yellow (256-color palette)
- **2.0.0** (2025-07-02) - Complete rewrite with numbered output system
- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [Framework Library](framework.md) - Test lifecycle management
- [File Utils Library](file_utils.md) - File and path utilities
- [LIBRARIES.md](LIBRARIES.md) - Library index
