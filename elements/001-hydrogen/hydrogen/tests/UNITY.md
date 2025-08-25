# Unity Unity Tests

Unity tests provide comprehensive unit testing for individual functions and modules within the Hydrogen codebase. The Unity testing framework enables thorough validation of core logic, edge cases, and boundary conditions at the function level. Of particular note, the testing framework is constructed in such a way as to create individual executable tests that link directly to object files of the project. We're not adding
any code at all to our release build that has any kind of intsrumentation in it, really. But by linking to our object code directly, we can get coverage information that can be compared between our blackbox and unity tests to give an overall coverage report in great detail.

IMPORTANT: The main purpose of the Unity Framework Testing in this project is to improve overall code quality. This means sometimes that when adding new tests in particular, bugs or shortcomings in the codebase will be uncovered and need to be addressed. This is an expected and normal outcome of building tests. Sometimes the tests themselves make incorrect assumptions or aggressive assumptions that don't match the code and will need to be adjusted. In other instances they make perfectly valid assumptions and the code is just not dealing with it the way it should so the code will neeed to be adjusted.

## Source Code Organization and Naming Convention

**Location**: Unity test files are organized to **mirror the source directory structure** in `tests/unity/src/`

**🔔 IMPORTANT - File Naming Convention**: Unity test files **MUST** follow the pattern:

- **`<source>_test_<function>.c`** - For testing a specific function (e.g., `payload_test_validate_payload_key.c` tests `validate_payload_key()`)

**Directory Structure**: Tests are organized in subdirectories that match the `src/` directory structure:

```directory
tests/unity/src/
├── hydrogen_test.c                             # Tests core hydrogen functionality
├── launch/
│   └── launch_plan_test.c                      # Tests src/launch/launch_plan.c
├── payload/
│   ├── payload_test_validate_payload_key.c     # Tests validate_payload_key() function
│   ├── payload_test_free_payload.c             # Tests free_payload() function
│   ├── payload_test_cleanup_openssl.c          # Tests cleanup_openssl() function
│   └── payload_test_data_structure.c           # Tests PayloadData structure
├── swagger/
│   ├── swagger_test_is_swagger_request.c       # Tests is_swagger_request() function
│   ├── swagger_test_init_swagger_support.c     # Tests init_swagger_support() function
│   ├── swagger_test_handle_swagger_request.c   # Tests handle_swagger_request() function
│   ├── swagger_test_request_handler.c          # Tests swagger_request_handler() function
│   ├── swagger_test_cleanup_swagger_support.c  # Tests cleanup_swagger_support() function
│   ├── swagger_test_url_validator.c            # Tests swagger_url_validator() function
│   └── swagger_test.c                          # Integration tests for swagger workflow
├── api/
│   └── api_service_test.c                      # Tests src/api/api_service.c (example)
├── utils/
│   └── utils_time_test.c                       # Tests src/utils/utils_time.c (example)
└── [etc]
```

**One Test File Per Function Rule**: Each Unity test file focuses on testing **one specific function** from the source code:

- `payload_test_validate_payload_key.c` - Tests only the `validate_payload_key()` function
- `swagger_test_is_swagger_request.c` - Tests only the `is_swagger_request()` function
- `swagger_test_init_swagger_support.c` - Tests only the `init_swagger_support()` function

## Test File Structure

Each Unity test file should follow this structure:

```c
/*
 * Unity Test File: <Description>
 * This file contains unit tests for <module> functionality
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

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

**Build Commands**:

To build and test a Unity test function, use the 'mku' alias. It can be run from any folder and will find the test, compile it, and then run it. Just provide the base name of the test, without the foloder or extension.

```bash
mku payload_test_cleanup
```

**Key Integration Features**:

- **No Manual Configuration**: New Unity tests are automatically discovered and built
- **Parallel Building**: Multiple Unity tests can be built simultaneously
- **Coverage Instrumentation**: All tests include gcov coverage by default
- **Dependency Management**: Tests are properly linked with required libraries and source objects

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

### 📋 **Step-by-Step Instructions**

To add a new Unity test, follow these steps:

1. **📁 Choose Location**: Place test files in `tests/unity/src/` mirroring the source directory structure

2. **📝 Create Test File**: **CRITICAL** - Follow the naming convention exactly:
   - **`<source>_test_<function>.c`** for function-specific testing

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

## Best Practices

1. **Focus on Function Logic**: Test the specific function behavior, not entire system integration
2. **Use Real Dependencies**: When possible, use actual system functions (like logging) rather than mocks
3. **Test Edge Cases**: Pay special attention to boundary conditions and error scenarios
4. **Keep Tests Independent**: Each test should be able to run in isolation
5. **Document Test Intent**: Use clear, descriptive test names and comments
6. **Maintain Test Quality**: Keep tests simple, focused, and maintainable

## Performance and Timing Best Practices

Based on lessons learned from implementing previous tests.

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

## Example: launch_plan_test.c

The `launch_plan_test.c` file demonstrates comprehensive Unity testing:

- **14 test cases** covering all scenarios for `handle_launch_plan()` function
- Tests null parameters, empty results, mixed readiness states, and boundary conditions
- Uses real logging integration for authentic system behavior
- Validates launch plan decision logic across various subsystem configurations

This example serves as a template for writing effective Unity tests that provide thorough coverage and meaningful validation.

**Coverage Strategy:**

- Test real functions in source files for actual coverage
- Use mock functions to document expected behavior
- Combine unit test coverage + integration test coverage for complete picture

### Memory Management Testing

When testing functions that allocate memory:

**Common Issues:**

- cppcheck warnings about potential null pointer dereferences
- Memory leaks in test code
- Platform-specific allocation behavior

**Best Practices:**

```c
// Always check malloc results
char **line_array = malloc(2 * sizeof(char*));
TEST_ASSERT_NOT_NULL(line_array);  // Verify allocation succeeded

// Clean up properly in tearDown
void tearDown(void) {
    server_executable_size = 0;  // Reset global state
}
```
