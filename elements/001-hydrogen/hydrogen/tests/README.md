# Hydrogen Testing Framework

This directory contains the comprehensive testing framework for the Hydrogen 3D printer control server. It includes test scripts, configuration files, support utilities, and detailed documentation for validating the functionality, performance, and reliability of the Hydrogen system.

## Overview

The Hydrogen testing framework is designed to ensure the robustness and correctness of the server through a structured suite of tests. These tests cover various aspects including compilation, startup/shutdown sequences, API functionality, system endpoints, and code quality checks.

## Core Components

### Test Suite Runner (test_00_all.sh)

**test_00_all.sh** is the central orchestration script for executing the entire test suite. It manages the execution of all test scripts, either in parallel batches or sequentially, and generates a comprehensive summary of results.

- **Version**: 4.0.0 (Last updated: 2025-07-04)
- **Key Features**:
  - Executes tests in parallel batches grouped by tens digit (e.g., 0x, 1x, 2x) for improved performance, or sequentially if specified.
  - Supports skipping test execution for quick summary generation or README updates.
  - Automatically updates the project README.md with test results and repository statistics using the `cloc` tool.
  - Provides detailed output with test status, subtest counts, pass/fail statistics, and elapsed time.
- **Usage**:

  ```bash
  # Run all tests in parallel batches
  ./test_00_all.sh

  # Run all tests sequentially
  ./test_00_all.sh --sequential

  # Run specific tests
  ./test_00_all.sh 01_compilation 22_startup_shutdown

  # Skip test execution and update README
  ./test_00_all.sh --skip-tests
  ```

## Test Scripts

Below is a comprehensive list of all test scripts currently available in the suite, organized by category.
With the exception of Test 01 (Compilation) and Test 11 (Unity) these are all considered "blackbox" tests in that they simply run the server and test it from an external point of view.
Test 01 is just the script that bulids everything, so not really a test in itself, though it does do a number of checks for important things.  
And Test 11 is all about Unity unit tests, where we have custom code written to call our app's functions directly, which is described in more detail below.

### Test Suite Management

- **[test_00_all.sh](docs/test_00_all.md)**: Test suite runner for orchestrating all tests (described above).

### Core Functional Tests

- **[test_01_compilation.sh](docs/test_01_compilation.md)**: Verifies successful compilation and build processes.
- **[test_10_leaks_like_a_sieve.sh](docs/test_10_leaks_like_a_sieve.md)**: Detects memory leaks and resource issues using Valgrind.
- **[test_11_unity.sh](docs/test_11_unity.md)**: Integrates Unity testing framework for unit tests.
- **[test_12_env_variables.sh](docs/test_12_env_variables.md)**: Tests that configuration values can be provided via environment variables.
- **[test_14_env_payload.sh](docs/test_14_env_payload.md)**: Tests environment variable handling for the payload system.
- **[test_16_library_dependencies.sh](docs/test_16_library_dependencies.md)**: Checks for required library dependencies.
- **[test_18_json_error_handling.sh](docs/test_18_json_error_handling.md)**: Tests JSON configuration error handling.
- **[test_20_crash_handler.sh](docs/test_20_crash_handler.md)**: Tests that the crash handler correctly generates and formats core dumps.
- **[test_22_startup_shutdown.sh](docs/test_22_startup_shutdown.md)**: Validates complete startup and shutdown lifecycles.
- **[test_24_signals.sh](docs/test_24_signals.md)**: Tests signal handling (e.g., SIGINT, SIGTERM, SIGHUP).
- **[test_26_shutdown.sh](docs/test_26_shutdown.md)**: Tests the shutdown functionality of the application with a minimal configuration.
- **[test_28_socket_rebind.sh](docs/test_28_socket_rebind.md)**: Tests socket rebinding behavior.
- **[test_30_api_prefixes.sh](docs/test_30_api_prefixes.md)**: Validates API prefix configurations.
- **[test_32_system_endpoints.sh](docs/test_32_system_endpoints.md)**: Tests system endpoint functionality.
- **[test_34_swagger.sh](docs/test_34_swagger.md)**: Verifies Swagger documentation and UI integration.
- **[test_36_websockets.sh](docs/test_36_websockets.md)**: Tests WebSocket server functionality and integration.

### Static Analysis & Code Quality Tests

- **[test_03_code_size.sh](docs/test_03_code_size.md)**: Analyzes code size metrics and file distribution.
- **[test_06_check_links.sh](docs/test_06_check_links.md)**: Validates links in documentation files.
- **[test_91_cppcheck.sh](docs/test_91_cppcheck.md)**: Performs C/C++ static analysis using cppcheck.
- **[test_92_shellcheck.sh](docs/test_92_shellcheck.md)**: Validates shell scripts using shellcheck with exception justification checks.
- **[test_94_eslint.sh](docs/test_94_eslint.md)**: Lints JavaScript files using eslint.
- **[test_95_stylelint.sh](docs/test_95_stylelint.md)**: Validates CSS files using stylelint.
- **[test_96_htmlhint.sh](docs/test_96_htmlhint.md)**: Validates HTML files using htmlhint.
- **[test_97_jsonlint.sh](docs/test_97_jsonlint.md)**: Validates JSON file syntax and structure.
- **[test_98_markdownlint.sh](docs/test_98_markdownlint.md)**: Lints Markdown documentation using markdownlint.
- **[test_99_coverage.sh](docs/test_99_coverage.md)**: Performs comprehensive coverge analysis.

## Writing Unity Tests

Unity tests provide comprehensive unit testing for individual functions and modules within the Hydrogen codebase. The Unity testing framework enables thorough validation of core logic, edge cases, and boundary conditions at the function level. Of particular note, the testing framework is constructed in such a way as to create individual executable tests that link directly to object files of the project. We're not adding
any code at all to our release build that has any kind of intsrumentation in it, really. But by linking to our object code directly, we can get coverage information that can be compared between our blackbox and unity tests to give an overall coverage report in great detail. See Test 99 for the amazing results of that effort.

### Intent and Purpose

Unity tests aim to accomplish several key objectives:

1. **Function-Level Validation**: Test individual functions in isolation to ensure they behave correctly under various conditions
2. **Edge Case Coverage**: Validate behavior with boundary conditions, invalid inputs, and error scenarios
3. **Regression Prevention**: Catch breaking changes early in the development cycle
4. **Code Quality Assurance**: Ensure functions handle all expected input combinations and return appropriate results
5. **Documentation**: Serve as living documentation of expected function behavior
6. **Development Confidence**: Enable safe refactoring and modifications with immediate feedback

### Source Code Organization

**Location**: Unity test files are organized to **mirror the source directory structure** in `tests/unity/src/`

**Directory Structure**: Tests are organized in subdirectories that match the `src/` directory structure:

```directory
tests/unity/src/
├── test_hydrogen.c              # Tests core hydrogen functionality
├── launch/
│   └── test_launch_plan.c       # Tests src/launch/launch_plan.c
├── api/
│   └── test_api_service.c       # Tests src/api/api_service.c (example)
├── utils/
│   └── test_utils.c             # Tests src/utils/utils.c (example)
└── [other directories matching src/ structure]
```

**Naming Convention**: Test files follow the pattern `test_<module_name>.c` where `<module_name>` corresponds to the source file or module being tested.

**Examples**:

- `tests/unity/src/launch/test_launch_plan.c` - Tests functions in `src/launch/launch_plan.c`
- `tests/unity/src/test_hydrogen.c` - Tests core hydrogen functionality
- `tests/unity/src/utils/test_utils.c` - Tests utility functions in `src/utils/utils.c`

**Benefits of Mirrored Structure**:

- **Intuitive Organization**: Easy to locate tests for any source file
- **Maintainability**: Clear relationship between source code and tests
- **Scalability**: Structure naturally grows with the codebase
- **Navigation**: Developers can quickly find relevant tests

### Test File Structure

Each Unity test file should follow this structure:

```c
/*
 * Unity Test File: <Description>
 * This file contains unit tests for <module> functionality
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

// Include necessary headers for the module being tested
#include "path/to/module.h"

// Forward declarations for functions being tested
bool function_to_test(const SomeType* input);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_function_name_basic_functionality(void) {
    // Test basic functionality
    TEST_ASSERT_TRUE(function_to_test(&valid_input));
}

void test_function_name_null_parameter(void) {
    // Test null parameter handling
    TEST_ASSERT_FALSE(function_to_test(NULL));
}

void test_function_name_edge_cases(void) {
    // Test edge cases and boundary conditions
    // ...
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_function_name_basic_functionality);
    RUN_TEST(test_function_name_null_parameter);
    RUN_TEST(test_function_name_edge_cases);
    
    return UNITY_END();
}
```

### CMake Build Integration

Unity tests are **automatically discovered and integrated** with the CMake build system. The system uses dynamic discovery to find all Unity test files and build them with coverage instrumentation.

**Automatic Discovery Process**:

1. **File Detection**: CMake automatically finds all `test_*.c` files in `tests/unity/src/`
2. **Target Creation**: Each test file generates a corresponding build target and executable
3. **Coverage Instrumentation**: All tests are built with gcov coverage flags enabled
4. **Directory Structure**: Coverage data is generated in the `build/unity/src/` directory tree

**Build Commands**:

```bash
# Build all Unity tests (automatically discovers and builds ALL test_*.c files)
cd cmake
cmake --build . --target unity_tests

# Build specific test (individual targets are auto-created)
cmake --build . --target test_launch_plan
cmake --build . --target test_hydrogen

# Run specific test executable
cd ../build/unity/src
./test_launch_plan
./test_hydrogen
```

**CMake Target Convention**: For each `test_<module>.c` file, CMake **automatically creates**:

- A build target named `test_<module>`
- An executable at `build/unity/src/test_<module>`
- Object files with coverage instrumentation in `build/unity/src/`
- Coverage data files (`.gcov`, `.gcda`, `.gcno`) in the corresponding subdirectories

**Coverage File Structure**: After building and running Unity tests, coverage data is organized to **mirror the source structure**:

```directory
build/unity/src/
├── test_hydrogen              # Executable
├── test_hydrogen.c.gcov       # Test file coverage
├── unity.c.gcov               # Unity framework coverage
├── launch/
│   ├── test_launch_plan       # Executable (mirrored structure)
│   ├── test_launch_plan.c.gcov # Test file coverage
│   ├── launch_plan.c.gcov     # Source coverage for launch_plan.c
│   ├── launch_plan.gcda       # Coverage data
│   └── launch_plan.gcno       # Coverage notes
├── api/
│   └── [API module coverage files]
├── utils/
│   └── [Utils module coverage files]
└── [other source directories with coverage files]
```

**Key Integration Features**:

- **No Manual Configuration**: New Unity tests are automatically discovered and built
- **Parallel Building**: Multiple Unity tests can be built simultaneously
- **Coverage Instrumentation**: All tests include gcov coverage by default
- **Dependency Management**: Tests are properly linked with required libraries and source objects

### Test Framework Integration (test_11_unity.sh)

The Unity tests are automatically discovered and executed by `test_11_unity.sh`. This script:

1. **Auto-Discovery**: Automatically finds all Unity test executables in `build/unity/src/`
2. **Individual Execution**: Runs each test as a separate subtest with detailed reporting
3. **Results Parsing**: Extracts test counts and pass/fail statistics from Unity output
4. **Coverage Analysis**: Calculates code coverage for all Unity tests combined
5. **Failure Handling**: Captures and reports detailed failure information

**Test Output Example**:

```log
11-004   000.109   ▇▇ TEST   Run Unity Test: test_launch_plan
11-004   000.131   ▇▇ PASS   Unity test test_launch_plan passed: 14 tests (14/14 passed)
11-005   000.138   ▇▇ TEST   Run Unity Test: test_hydrogen
11-005   000.159   ▇▇ PASS   Unity test test_hydrogen passed: 5 tests (5/5 passed)
```

### Writing Comprehensive Tests

When writing Unity tests, follow these guidelines based on lessons learned from implementing payload and swagger module tests:

**Function Safety Assessment**:

Before writing tests, categorize functions by safety:

- **Safe Functions**: Pure functions, validation functions, cleanup functions, structure accessors
  - Examples: `validate_payload_key()`, `is_swagger_request()`, `free_payload()`, `cleanup_swagger_support()`
  - These can be tested extensively with all input combinations
- **Potentially Problematic Functions**: Initialization functions, functions that interact with global state, networking functions
  - Examples: `init_swagger_support()`, `handle_swagger_request()` (requires MHD connection)
  - These may require mock objects or should be avoided in unit tests
- **High-Risk Functions**: Functions that may hang, crash, or have unpredictable behavior
  - Examples: Functions with infinite loops, signal handlers, thread management
  - Generally should not be tested in Unity tests

**Test Coverage Guidelines**:

- **Focus on Safe Functions**: Prioritize testing functions that can be reliably tested in isolation
- **Test All Public Functions**: In the target module, but assess safety first
- **Comprehensive Parameter Validation**: Test null parameters, empty inputs, invalid configurations
- **Boundary Conditions**: Test minimum/maximum values, edge cases, buffer limits
- **State Validation**: Test different configuration states (enabled/disabled, available/unavailable)
- **Structure Testing**: Validate data structure initialization, assignment, and cleanup
- **Error Scenarios**: Test all error paths and recovery mechanisms

**Systematic Test Organization Patterns**:

Organize tests into logical groups:

1. **Parameter Validation Tests** (null, empty, invalid inputs)
2. **Normal Operation Tests** (valid inputs, expected behavior)
3. **Edge Case Tests** (boundary conditions, special values)
4. **Configuration State Tests** (different system states)
5. **Structure Validation Tests** (data structure behavior)
6. **Cleanup and Resource Management Tests**

**Test Naming Convention**:

- Use descriptive names: `test_<function_name>_<scenario>`
- Examples:
  - `test_validate_payload_key_null_key`
  - `test_is_swagger_request_custom_prefix`
  - `test_cleanup_swagger_support_multiple_calls`
- Group related tests with consistent prefixes for easy identification

**Preventing Test Hangs and Crashes**:

Based on experience with payload environment variable testing:

- **Use Timeout Mechanisms**: Implement alarm-based timeouts for functions that might hang
- **Test Global State Carefully**: Some functions depend on global variables; set these appropriately
- **Avoid System State Functions**: Functions that modify system state may be unreliable in tests
- **Test Cleanup Functions Multiple Times**: Ensure cleanup functions handle multiple calls gracefully

**Example Timeout Pattern**:

```c
// Simple timeout mechanism using alarm
static volatile sig_atomic_t test_timeout = 0;

void timeout_handler(int sig) {
    (void)sig;
    test_timeout = 1;
}

void test_function_with_potential_hang(void) {
    signal(SIGALRM, timeout_handler);
    alarm(2);  // 2-second timeout
    
    bool result = potentially_slow_function();
    
    alarm(0);  // Cancel alarm
    signal(SIGALRM, SIG_DFL);
    
    TEST_ASSERT_FALSE(test_timeout);  // Ensure we didn't timeout
    TEST_ASSERT_TRUE(result);
}
```

**Test Data Management**:

- **Use Realistic Test Data**: Reflect actual system usage patterns
- **Create Comprehensive Fixtures**: Set up complex data structures in setUp()
- **Test Structure Lifecycle**: Initialization, assignment, modification, cleanup
- **Memory Management**: Always clean up allocated resources in tearDown()
- **Configuration Objects**: Test all configuration combinations and states

**Configuration and Structure Testing Patterns**:

```c
// Example pattern for testing configuration structures
void test_config_structure_validation(void) {
    SwaggerConfig config = {0};
    
    // Test initialization state
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.prefix);
    
    // Test assignment
    config.enabled = true;
    config.prefix = strdup("/swagger");
    
    // Test validation
    TEST_ASSERT_TRUE(config.enabled);
    TEST_ASSERT_EQUAL_STRING("/swagger", config.prefix);
    
    // Cleanup
    free(config.prefix);
}
```

**Parameter Validation Testing Strategy**:

Systematically test all parameter combinations:

1. **Null Parameters**: Test each parameter as NULL individually
2. **Empty/Invalid Values**: Test empty strings, zero values, invalid enums
3. **Boundary Values**: Test at limits (max length, max size, etc.)
4. **Combined Invalid States**: Test multiple invalid parameters together
5. **Valid Combinations**: Ensure valid inputs work correctly

**Coverage Maximization**:

- **Target High-Value Functions**: Focus on functions that provide maximum coverage gain
- **Test Wrapper Functions**: Test both wrapper functions and underlying implementations
- **Exercise All Code Paths**: Use different input combinations to trigger different branches
- **Validate Coverage Reports**: Use gcov output to identify missed lines and functions

### Adding New Unity Tests

To add a new Unity test:

1. **Create Test File**: Add `test_<module>.c` in `tests/unity/src/`
2. **Implement Tests**: Write comprehensive test functions covering all scenarios
3. **Build**: Run `cmake --build . --target test_<module>` to verify compilation
4. **Validate**: Execute the test directly to ensure all tests pass
5. **Integration**: The test will automatically be discovered by `test_11_unity.sh`

**No additional configuration is required** - the CMake build system and test framework automatically handle new Unity tests.

### Unity Framework Features

The Unity framework provides rich assertion capabilities:

**Basic Assertions**:

- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`
- `TEST_ASSERT_NULL(pointer)`
- `TEST_ASSERT_NOT_NULL(pointer)`

**Numeric Assertions**:

- `TEST_ASSERT_EQUAL_INT(expected, actual)`
- `TEST_ASSERT_GREATER_THAN(threshold, actual)`
- `TEST_ASSERT_LESS_THAN(threshold, actual)`

**String Assertions**:

- `TEST_ASSERT_EQUAL_STRING(expected, actual)`
- `TEST_ASSERT_EQUAL_MEMORY(expected, actual, length)`

**Array Assertions**:

- `TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, elements)`

### Coverage Analysis

Unity tests automatically generate code coverage data when built with coverage instrumentation. The coverage analysis:

- Tracks line coverage for all tested functions
- Generates detailed coverage reports
- Integrates with the overall test suite coverage metrics
- Helps identify untested code paths

### Best Practices

1. **Focus on Function Logic**: Test the specific function behavior, not entire system integration
2. **Use Real Dependencies**: When possible, use actual system functions (like logging) rather than mocks
3. **Test Edge Cases**: Pay special attention to boundary conditions and error scenarios
4. **Keep Tests Independent**: Each test should be able to run in isolation
5. **Document Test Intent**: Use clear, descriptive test names and comments
6. **Maintain Test Quality**: Keep tests simple, focused, and maintainable

### Example: test_launch_plan.c

The `test_launch_plan.c` file demonstrates comprehensive Unity testing:

- **14 test cases** covering all scenarios for `handle_launch_plan()` function
- Tests null parameters, empty results, mixed readiness states, and boundary conditions
- Uses real logging integration for authentic system behavior
- Validates launch plan decision logic across various subsystem configurations

This example serves as a template for writing effective Unity tests that provide thorough coverage and meaningful validation.

## Configuration Files

The `configs/` directory contains JSON files for various test scenarios:

- **hydrogen_test_min.json**: Minimal configuration for testing core functionality with optional subsystems disabled.
- **hydrogen_test_max.json**: Maximal configuration with all subsystems enabled for full feature testing.
- **hydrogen_test_api_*.json**: Configurations for API-related tests with different prefixes and settings.
- **hydrogen_test_swagger_*.json**: Configurations for testing Swagger UI and documentation endpoints.
- **hydrogen_test_system_endpoints.json**: Configuration for testing system endpoints.

### Port Configuration

To prevent conflicts, tests use dedicated port ranges for different configurations:

| Test Configuration          | Config File                              | Web Server Port | WebSocket Port |
|-----------------------------|------------------------------------------|-----------------|----------------|
| Default Min/Max             | hydrogen_test_min.json                   | 5000            | 5001           |
|                             | hydrogen_test_max.json                   | 5000            | 5001           |
| API Prefix Test             | hydrogen_test_api_test_1.json            | 5030            | 5031           |
|                             | hydrogen_test_api_test_2.json            | 5050            | 5051           |
| Swagger UI Test (Default)   | hydrogen_test_swagger_test_1.json        | 5040            | 5041           |
| Swagger UI Test (Custom)    | hydrogen_test_swagger_test_2.json        | 5060            | 5061           |
| System Endpoints Test       | hydrogen_test_system_endpoints.json      | 5070            | 5071           |

## Linting Configurations

Linting behavior is controlled by configuration files to ensure code quality:

- **.lintignore**: Global exclusion patterns for all linting tools.
- **.lintignore-c**: C/C++ specific configurations for `cppcheck`, defining enabled checks and suppressions.
- **.lintignore-markdown**: Markdown linting rules for documentation consistency.

## Test Output Directories

Test execution generates output in the following directories, which are cleaned at the start of each run:

- **./logs/**: Detailed execution logs for each test run.
- **./results/**: Summaries, build outputs, and result data.
- **./diagnostics/**: Diagnostic data and analysis for debugging issues.

## Documentation

Detailed documentation for each test script and library is available in the `docs/` directory:

- **test_00_all.md**: Documentation for the test suite runner.
- **test_01_compilation.md** to **test_99_cleanup.md**: Individual test script documentation.
- **LIBRARIES.md**: Overview of modular library scripts in the `lib/` directory.
- Additional guides and reference materials for test framework components.

## Developing New Tests

When creating new test scripts, adhere to the following standards:

1. **Naming Convention**: Use `test_NN_descriptive_name.sh` following the numbering system.
2. **Headers**: Include version, title, and change history at the top of the script.
3. **Shellcheck Compliance**: Ensure scripts pass `shellcheck` validation for quality and consistency.
4. **Integration**: Ensure compatibility with `test_00_all.sh` for automatic discovery and execution.
5. **Documentation**: Add corresponding documentation in the `docs/` directory and update this README.md.

### Workflow for New Tests

1. Copy an existing test script as a template.
2. Modify the script with necessary test logic and headers.
3. Validate with `shellcheck` to ensure compliance.
4. Test the script independently before integrating into the suite.
5. Document the test purpose and usage in the `docs/` directory.

## Usage Examples

### Running the Full Test Suite

```bash
# Run all tests in parallel batches (default mode)
./test_00_all.sh all

# Run all tests sequentially
./test_00_all.sh --sequential
```

### Running Specific Tests

```bash
# Run specific tests sequentially
./test_00_all.sh 01_compilation 22_startup_shutdown
```

### Skipping Test Execution

```bash
# Skip test execution but update README with what would run
./test_00_all.sh --skip-tests
```

### Manual Testing with Configurations

```bash
# Run Hydrogen with minimal configuration
../hydrogen ./configs/hydrogen_test_min.json

# Run Hydrogen with maximal configuration
../hydrogen ./configs/hydrogen_test_max.json
```

For more detailed information on the Hydrogen testing approach, refer to the [Testing Documentation](../docs/testing.md).
