# Log Output Library Documentation

## Overview

The Log Output Library (`lib/log_output.sh`) provides consistent logging, formatting, and display functions for test scripts in the Hydrogen test suite. This library was created as part of the migration from `support_utils.sh` to modular, focused scripts.

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

### Test Numbering Functions

#### `extract_test_number(script_path)`

Extracts the test number from a script filename (e.g., "10" from "test_10_compilation.sh").

**Parameters:**

- `script_path` - Path to the test script

**Returns:** Test number as string

#### `set_test_number(number)`

Sets the current test number for output prefixing.

**Parameters:**

- `number` - Test number (e.g., "10")

#### `next_subtest()`

Increments the subtest counter and sets the current subtest number.

**Returns:** Nothing (sets CURRENT_SUBTEST_NUMBER internally)

#### `reset_subtest_counter()`

Resets the subtest counter to 0.

#### `get_test_prefix()`

Gets the current test prefix for output (e.g., "10-001").

**Returns:** Formatted test prefix

### Test Header Functions

#### `print_test_header(test_name, script_version)`

Prints a beautiful test header using tables.sh with proper columns for Test#, Test Title, Version, and Started timestamp.

**Parameters:**

- `test_name` - Name of the test
- `script_version` - Version of the test script

**Output:** Professional table header with test number (XX-000), name, version, and timestamp

**Example:**

```bash
print_test_header "Hydrogen Compilation Test" "1.0.0"
```

**Output:**

```table
╭────────┬───────────────────────────┬─────────┬─────────────────────────╮
│ Test#  │ Test Title                │ Version │                 Started │
├────────┼───────────────────────────┼─────────┼─────────────────────────┤
│ 10-000 │ Hydrogen Compilation Test │ v1.0.0  │ 2025-07-02 04:52:18.946 │
╰────────┴───────────────────────────┴─────────┴─────────────────────────╯
```

#### `print_subtest(subtest_name)`

Prints a formatted subtest header with the current test-subtest number.

**Parameters:**

- `subtest_name` - Name of the subtest

**Example:**

```bash
next_subtest
print_subtest "Check CMake Availability"
```

### Color and Icon Definitions

The library defines standard colors and icons used throughout the test suite:

```bash
# Colors
GREEN='\033[0;32m'    # Success messages
RED='\033[0;31m'      # Error messages  
YELLOW='\033[0;33m'   # Warning messages
BLUE='\033[0;34m'     # Headers and info
CYAN='\033[0;36m'     # Informational messages
BOLD='\033[1m'        # Bold text
NC='\033[0m'          # No color (reset)

# Icons
PASS_ICON="✅"        # Success indicator
FAIL_ICON="❌"        # Failure indicator
WARN_ICON="⚠️ "       # Warning indicator
INFO_ICON="🛈 "       # Information indicator
```

### Core Output Functions

#### `print_header(message)`

Prints a formatted section header with decorative borders.

**Parameters:**

- `message` - The header text to display

**Example:**

```bash
print_header "Running Compilation Tests"
```

#### `print_result(status, message)`

Prints a test result with appropriate icon and color coding.

**Parameters:**

- `status` - Exit code (0 for success, non-zero for failure)
- `message` - The result message to display

**Example:**

```bash
print_result 0 "All tests passed successfully"
print_result 1 "Test failed with errors"
```

#### `print_info(message)`

Prints an informational message with cyan color and info icon.

**Parameters:**

- `message` - The informational message

**Example:**

```bash
print_info "Starting server initialization..."
```

#### `print_warning(message)`

Prints a warning message with yellow color and warning icon.

**Parameters:**

- `message` - The warning message

**Example:**

```bash
print_warning "Configuration file not found, using defaults"
```

#### `print_error(message)`

Prints an error message with red color and error icon.

**Parameters:**

- `message` - The error message

**Example:**

```bash
print_error "Failed to connect to database"
```

#### `print_command(command)`

Prints a command that will be executed, with path normalization.

**Parameters:**

- `command` - The command string to display

**Example:**

```bash
print_command "cmake --build build --target all"
```

### Test Completion Functions

#### `print_test_completion(test_name)`

Prints a beautiful test completion table using tables.sh showing comprehensive test statistics.

**Parameters:**

- `test_name` - Name of the test

**Output:** Professional table with Test#, Test Name, Subs (total subtests), Pass (passed subtests), Fail (failed subtests), and Elapsed time in SSS.ZZZ format

**Example:**

```bash
print_test_completion "Hydrogen Compilation Test"
```

**Output:**

```table
╭────────┬───────────────────────────┬──────┬──────┬──────┬─────────╮
│ Test#  │ Test Name                 │ Subs │ Pass │ Fail │ Elapsed │
├────────┼───────────────────────────┼──────┼──────┼──────┼─────────┤
│ 10-000 │ Hydrogen Compilation Test │    7 │    4 │    3 │  0.420s │
╰────────┴───────────────────────────┴──────┴──────┴──────┴─────────╯
```

**Features:**

- Automatically tracks timing from `print_test_header()` call
- Automatically counts passed/failed subtests from `print_result()` calls
- Shows elapsed time in seconds with 3 decimal places
- Uses tables.sh for consistent, professional formatting

### Summary Functions

#### `print_test_summary(total, passed, failed)`

Prints a formatted summary of test results (legacy version).

**Parameters:**

- `total` - Total number of tests
- `passed` - Number of passed tests
- `failed` - Number of failed tests

**Example:**

```bash
print_test_summary 10 8 2
```

#### `print_test_item(status, name, details)`

Prints an individual test item in a summary list.

**Parameters:**

- `status` - Test result (0 for pass, non-zero for fail)
- `name` - Test name
- `details` - Additional details about the test

**Example:**

```bash
print_test_item 0 "Compilation Test" "All variants built successfully"
```

### Utility Functions

#### `print_log_output_version()`

Displays the library version information.

**Example:**

```bash
print_log_output_version
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

- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [File Utils Library](file_utils.md) - File and path utilities
- [Test Framework Library](framework.md) - Test lifecycle management
- [Migration Plan](Migration_Plan.md) - Overall migration strategy
- [LIBRARIES.md](LIBRARIES.md) - Library index
