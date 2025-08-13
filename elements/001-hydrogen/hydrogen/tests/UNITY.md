# Unity Unity Tests

Unity tests provide comprehensive unit testing for individual functions and modules within the Hydrogen codebase. The Unity testing framework enables thorough validation of core logic, edge cases, and boundary conditions at the function level. Of particular note, the testing framework is constructed in such a way as to create individual executable tests that link directly to object files of the project. We're not adding
any code at all to our release build that has any kind of intsrumentation in it, really. But by linking to our object code directly, we can get coverage information that can be compared between our blackbox and unity tests to give an overall coverage report in great detail. See Test 99 for the amazing results of that effort.

## Intent and Purpose

Unity tests aim to accomplish several key objectives:

1. **Function-Level Validation**: Test individual functions in isolation to ensure they behave correctly under various conditions
2. **Edge Case Coverage**: Validate behavior with boundary conditions, invalid inputs, and error scenarios
3. **Regression Prevention**: Catch breaking changes early in the development cycle
4. **Code Quality Assurance**: Ensure functions handle all expected input combinations and return appropriate results
5. **Documentation**: Serve as living documentation of expected function behavior
6. **Development Confidence**: Enable safe refactoring and modifications with immediate feedback

## Source Code Organization

**Location**: Unity test files are organized to **mirror the source directory structure** in `tests/unity/src/`

**Directory Structure**: Tests are organized in subdirectories that match the `src/` directory structure:

```directory
tests/unity/src/
├── test_hydrogen.c              # Tests core hydrogen functionality
├── launch/
│   └── test_launch_plan.c       # Tests src/launch/launch_plan.c
├── payload/
│   ├── test_validate_payload_key.c      # Tests validate_payload_key() function
│   ├── test_free_payload.c              # Tests free_payload() function
│   ├── test_cleanup_openssl.c           # Tests cleanup_openssl() function
│   └── test_payload_data_structure.c    # Tests PayloadData structure
├── swagger/
│   ├── test_is_swagger_request.c        # Tests is_swagger_request() function
│   ├── test_init_swagger_support.c      # Tests init_swagger_support() function
│   ├── test_handle_swagger_request.c    # Tests handle_swagger_request() function
│   ├── test_swagger_request_handler.c   # Tests swagger_request_handler() function
│   ├── test_cleanup_swagger_support.c   # Tests cleanup_swagger_support() function
│   ├── test_swagger_url_validator.c     # Tests swagger_url_validator() function
│   └── test_swagger.c                   # Integration tests for swagger workflow
├── api/
│   └── test_api_service.c       # Tests src/api/api_service.c (example)
├── utils/
│   └── test_utils.c             # Tests src/utils/utils.c (example)
└── [other directories matching src/ structure]
```

**Naming Convention**: Test files follow the pattern `test_<function_name>.c` where `<function_name>` corresponds to the specific function being tested. For structures and constants, use `test_<structure_name>.c`.

**One Test File Per Function Rule**: Each Unity test file focuses on testing **one specific function** from the source code:

- `test_validate_payload_key.c` - Tests only the `validate_payload_key()` function
- `test_is_swagger_request.c` - Tests only the `is_swagger_request()` function
- `test_init_swagger_support.c` - Tests only the `init_swagger_support()` function

**Examples**:

- `tests/unity/src/launch/test_launch_plan.c` - Tests functions in `src/launch/launch_plan.c`
- `tests/unity/src/payload/test_validate_payload_key.c` - Tests `validate_payload_key()` function from `src/payload/payload.c`
- `tests/unity/src/swagger/test_is_swagger_request.c` - Tests `is_swagger_request()` function from `src/swagger/swagger.c`
- `tests/unity/src/test_hydrogen.c` - Tests core hydrogen functionality

**Benefits of One-Function-Per-File Structure**:

- **Focused Testing**: Each test file has a single responsibility
- **Easy Navigation**: Developers can quickly locate tests for specific functions
- **Maintainability**: Changes to one function only affect its dedicated test file
- **Scalability**: Structure naturally grows with new functions
- **Clear Coverage**: Easy to see which functions have dedicated tests
- **Parallel Development**: Multiple developers can work on tests for different functions simultaneously

## Test File Structure

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

## CMake Build Integration

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

## Test Framework Integration (test_11_unity.sh)

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

## Writing Comprehensive Tests

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

## Adding New Unity Tests

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

## Coverage Analysis

Unity tests automatically generate code coverage data when built with coverage instrumentation. The coverage analysis:

- Tracks line coverage for all tested functions
- Generates detailed coverage reports
- Integrates with the overall test suite coverage metrics
- Helps identify untested code paths

## Best Practices

1. **Focus on Function Logic**: Test the specific function behavior, not entire system integration
2. **Use Real Dependencies**: When possible, use actual system functions (like logging) rather than mocks
3. **Test Edge Cases**: Pay special attention to boundary conditions and error scenarios
4. **Keep Tests Independent**: Each test should be able to run in isolation
5. **Document Test Intent**: Use clear, descriptive test names and comments
6. **Maintain Test Quality**: Keep tests simple, focused, and maintainable

## Performance and Timing Best Practices

Based on lessons learned from implementing `utils_time.c` tests:

**Avoid Long Delays in Unit Tests**:

- Never use `sleep(1)` or longer delays - unit tests should complete in milliseconds, not seconds
- A test suite that takes 5+ seconds due to sleep calls will slow down development workflow significantly

**Use Precise, Minimal Timing When Needed**:

```c
// Good: Use nanosleep for minimal, precise delays when testing timing functions
struct timespec delay = {0, 5000000}; // 5 milliseconds
nanosleep(&delay, NULL);

// Bad: Using sleep() for timing tests
sleep(1);  // Too slow for unit tests

// Bad: Using busy-wait loops
for(int i = 0; i < 10000; i++) { /* busy wait */ }  // Unreliable timing
```

**Handle Static/Global State Carefully**:

- Functions that depend on global or static variables may return default values (like 0.0) when prerequisites aren't met
- Test for the actual expected behavior, not just positive outcomes
- Use `TEST_ASSERT_GREATER_OR_EQUAL(0.0, result)` for functions that legitimately return 0

**Consider Implementation Details**:

- Functions using `CLOCK_MONOTONIC` don't return wall clock time
- Timing functions may return 0.0 when internal state isn't properly initialized
- Static variables may retain state between test runs or be reset

**Test Realistic Scenarios**:

```c
// Good: Test the actual behavior
void test_calculate_startup_time_before_ready(void) {
    set_server_start_time();
    double startup_time = calculate_startup_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, startup_time);  // May be 0 if ready time not set
}

// Bad: Assuming always positive without considering implementation
void test_calculate_startup_time_before_ready(void) {
    set_server_start_time();
    double startup_time = calculate_startup_time();
    TEST_ASSERT_GREATER_THAN(0.0, startup_time);  // May fail if ready time not set
}
```

**Timing Test Patterns**:

- Set prerequisite state first (e.g., call setup functions)
- Add minimal delay only if needed for timing precision
- Test both the "times not set" and "times properly set" scenarios
- Use conditional assertions when functions have legitimate zero return values

## When Unity Tests Are NOT Appropriate

Based on lessons learned from attempting comprehensive Unity test coverage for the WebSocket module, it's important to recognize when Unity tests are **not the right approach**:

**System-Dependent Code**:

Unity tests are **unsuitable** for functions that require:

- System resources (network sockets, threads, mutexes)
- External library contexts (libwebsockets `struct lws *wsi`)
- Global application state (e.g., `ws_context`)
- Hardware or OS-level interactions
- Functions that may hang or crash without proper system initialization

**Example - WebSocket Functions**:

The WebSocket module provides a clear example of code that is **not suitable** for Unity testing:

```c
// These functions require system resources and are NOT suitable for Unity tests
int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
bool ws_init_context(const HydrogenConfig *config);
int ws_start_server(const HydrogenConfig *config);
void ws_shutdown_server(void);
```

**Key indicators** that functions are unsuitable for Unity tests:

- Require `struct lws *wsi` or other system library contexts
- Depend on global state variables (e.g., `ws_context`)
- Perform network operations or thread management
- Have initialization/cleanup dependencies that require system resources
- May cause segmentation faults when called without proper system setup

**Effort vs. Benefit Analysis**:

The WebSocket Unity test effort demonstrated that **significant effort** (creating 18 test files, 1000+ lines of test code) can yield **minimal practical benefit**:

- Only ~82 lines of new Unity coverage across 9 WebSocket files
- Existing integration tests already provided 30-53 lines coverage per file
- Segmentation faults and system dependencies made tests unreliable
- Time investment was disproportionate to coverage gains

**Better Alternatives for System-Dependent Code**:

For code that requires system resources:

1. **Integration Tests**: Test the complete system functionality (like `test_27_websockets.sh`)
2. **Blackbox Testing**: Test from external perspective using real system interactions
3. **Component Testing**: Test larger components that include necessary system setup
4. **Mock-Heavy Testing**: Only if mocking system dependencies is practical and valuable

**When to Skip Unity Tests**:

Consider **skipping Unity tests** and focusing on integration tests when:

- Functions require external library contexts that can't be easily mocked
- Code depends heavily on global state or system initialization
- Functions are primarily "glue code" between system components
- Existing integration tests already provide adequate coverage
- The effort to create reliable unit tests exceeds the practical benefit

**Decision Framework**:

Before investing in Unity tests for a module:

1. **Assess Function Safety**: Can functions be called safely in isolation?
2. **Evaluate Dependencies**: Do functions require system resources or external contexts?
3. **Check Existing Coverage**: Do integration tests already provide adequate coverage?
4. **Estimate Effort vs. Benefit**: Will the Unity tests provide proportional value?
5. **Consider Alternatives**: Would integration or component tests be more appropriate?

**Key Lesson**: Not all code needs or benefits from unit tests. System-dependent code is often better validated through integration testing that exercises the complete system in realistic conditions.

## Example: test_launch_plan.c

The `test_launch_plan.c` file demonstrates comprehensive Unity testing:

- **14 test cases** covering all scenarios for `handle_launch_plan()` function
- Tests null parameters, empty results, mixed readiness states, and boundary conditions
- Uses real logging integration for authentic system behavior
- Validates launch plan decision logic across various subsystem configurations

This example serves as a template for writing effective Unity tests that provide thorough coverage and meaningful validation.
