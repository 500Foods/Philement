# Unity Unit Tests

Unity tests provide comprehensive unit testing for individual functions and modules within the Hydrogen codebase. The Unity testing framework enables thorough validation of core logic, edge cases, and boundary conditions at the function level. Of particular note, the testing framework is constructed in such a way as to create individual executable tests that link directly to object files of the project. We're not adding
any code at all to our release build that has any kind of intsrumentation in it, really. But by linking to our object code directly, we can get coverage information that can be compared between our blackbox and unity tests to give an overall coverage report in great detail.

IMPORTANT: The main purpose of the Unity Framework Testing in this project is to improve overall code quality. This means sometimes that when adding new tests in particular, bugs or shortcomings in the codebase will be uncovered and need to be addressed. This is an expected and normal outcome of building tests. Sometimes the tests themselves make incorrect assumptions or aggressive assumptions that don't match the code and will need to be adjusted. In other instances they make perfectly valid assumptions and the code is just not dealing with it the way it should so the code will neeed to be adjusted.

## IMPROVING COVERAGE TASK

A frequent task we preform involves ipmroving coverage for a single source file. When performing this specific task, we've got some things we can do to improve coverage. Normally, we'll start by reviewing existing coverage across both Unity unit tests and our blackbox tests by using the add_coverage.sh script to show us what intrumented lines are reporting no coverage. Sometimes this is the entire source file. From here, we have some things we can do.

1) Write tests to cover larger functions that have no coverage currently
2) Refactor larger functions to use helpers that are more easily tested
3) Refactor larger source code files into smaller files that can be reused or made small enough to fall under a lower threshold if fewer than 100 lines
4) Check that all functions are not static as we can't test static functions.

When performing this task, we're primarily concerned with improving coverage for instrumented lines that have no coverage currently, regardless of whether existing coverage is from unit or blackbox tests. New tests can be added to existing test sources, or new source tests created. If adding a new source test, we have to run `mkt` first to ensure that the new code compiles properly and without any errors or warnings. If the source test file exists already, and we're just making corrections, we can run `mku <testname>` to onfirm that all the tests pass. Run this from a zsh that references .zshrc to get these commands.

Once the tests are passing, stop the task and ask for confirmation of the new coverage result as well as an updated list of source lines if the coverage target is not yet achieved.

## MOCKS

A separate document specifically about the Mocks available and how to use them can be found in [unity/mocks/README.md](/elements/001-hydrogen/hydrogen/tests/unity/mocks/README.md).

## GENERAL FILE/FOLDER CONVENTIONS

Original source files are in src/

Other folders use the same file and folder conventions:

- Unity tests are in tests/unity/src/
- Unity gcov results are in build/unity/src/
- Blackbox gcov results are in build/coverage/src/

gcov files are named `<source.c.gcov>`

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
│   ├── payload_test_data_structure.c           # Tests PayloadData structure
│   └── payload.c
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

**Forward Declaration Rule**: Declare each test function prototype exactly **once**, immediately after the module's forward declarations and before `setUp()`. Some older test files repeat prototypes two or three times — this is harmless but adds noise and should not be copied when writing new tests.

## Test File Structure

Each Unity test file should follow this structure:

```c
/*
 * Unity Test File: <Description>
 * This file contains unit tests for <module> functionality
 */

/ Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/path/to/module.h>

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

## Coverage-Driven Workflow

The most efficient way to check Unity coverage is to use the `add_coverage.sh` tool in `extras/` before writing any tests. It cross-references the Unity gcov data against the blackbox gcov data and reports only the lines that are uncovered in **both** suites — the lines where a new Unity test will actually move the combined coverage needle.

### Step-by-Step Coverage Improvement Process

```bash
# 1. Run the full Unity test suite to ensure gcov data is fresh ONLY if NEW source files are created
mkt

# 2. Identify lines missing from both coverage sets for a specific source file
cd $HYDROGEN_ROOT
extras/add_coverage.sh utils/utils_crypto.c

# Output: Lines not covered in both files:
# --------------------------------------------------
#   260:        log_this("CRYPTO", "JWK parse failed: invalid JSON", LOG_LEVEL_ALERT, 0);
#   ...
```

The tool requires that both gcov files already exist (`build/unity/src/...` and `build/coverage/src/...`). If the Unity gcov file is missing it means no Unity test has been built for that source file yet — all lines are uncovered and you should look at the source file directly.

### Reading the Output

- Lines flagged by `add_coverage.sh` are the highest-priority targets — they are not covered by either the blackbox integration tests or the existing Unity tests.
- Lines covered by the blackbox tests but not by Unity are lower priority; adding Unity tests for them still improves Unity-only metrics but has no effect on combined coverage.
- Lines covered by neither suite that live inside third-party library failure paths (e.g., deep OpenSSL allocation failures, `RAND_bytes` returning -1) represent the **irreducible floor**. These require malloc fault injection to reach and are impractical to cover; accept them and move on.

### Recognising the Irreducible Floor

When the remaining uncovered lines all look like:

```c
log_this("SUBSYSTEM", "some internal library failed", LOG_LEVEL_ALERT, 0);
return false;   // or return NULL;
```

...and the failing call is to a third-party function (`HMAC`, `RAND_bytes`, `EVP_MD_CTX_new`, `OSSL_PARAM_BLD_new`, etc.), stop. These are OpenSSL/system internal allocation failure paths that cannot be triggered without patching `malloc` at a precise call depth. They are acceptable uncovered lines; document them in a comment and move on to the next source file.

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

### Core Testing Principles

1. **Focus on Function Logic**: Test the specific function behavior, not entire system integration
2. **Use Real Dependencies**: When possible, use actual system functions (like logging) rather than mocks
3. **Test Edge Cases**: Pay special attention to boundary conditions and error scenarios
4. **Keep Tests Independent**: Each test should be able to run in isolation
5. **Document Test Intent**: Use clear, descriptive test names and comments
6. **Maintain Test Quality**: Keep tests simple, focused, and maintainable

### Advanced Testing Strategies

#### JSON Processing Best Practices

**Critical**: `json_object_get()` does NOT support dotted paths. Always traverse nested objects step by step:

```c
// ❌ WRONG - This doesn't work
json_t* services = json_object_get(root, "mDNSServer.Services");

// ✅ CORRECT - Traverse nested objects properly
json_t* mdns_section = json_object_get(root, "mDNSServer");
json_t* services = mdns_section ? json_object_get(mdns_section, "Services") : NULL;
```

**Always validate JSON structure** before processing:

```c
if (services && json_is_array(services)) {
    // Safe to process array
}
```

#### Array Processing with Invalid Entries

When processing arrays that may contain invalid entries, use a separate counter for valid items:

```c
// ✅ CORRECT - Use separate counter for valid services
size_t valid_services = 0;
for (size_t i = 0; i < mdns_config->num_services; i++) {
    json_t* service = json_array_get(services, i);
    if (!json_is_object(service)) continue;  // Skip invalid entries

    // Process valid service...
    valid_services++;
}

// Update count to reflect only valid services
mdns_config->num_services = valid_services;
```

#### Memory Management Testing

**Always test memory allocation failure scenarios**:

```c
void test_function_memory_allocation_failure(void) {
    // Test how function handles malloc failures
    // This may require simulating allocation failures or testing cleanup paths
}
```

**Test cleanup functions with various states**:

```c
void test_cleanup_null_pointer(void) {
    cleanup_function(NULL);  // Should handle gracefully
}

void test_cleanup_empty_structure(void) {
    MyStruct config = {0};
    cleanup_function(&config);  // Should handle zeroed structures
}

void test_cleanup_allocated_resources(void) {
    MyStruct config = {0};
    config.allocated_field = strdup("test");
    cleanup_function(&config);  // Should free resources and zero structure
}
```

#### Configuration Testing Patterns

**Test all configuration scenarios**:

- Default values
- Custom values for each field
- Invalid/malformed configurations
- Complex nested structures (services, arrays, objects)

**Example comprehensive configuration test**:

```c
void test_config_full_customization(void) {
    json_t* root = json_object();
    json_t* config_section = json_object();

    // Set all possible configuration fields
    json_object_set(config_section, "Enabled", json_true());
    json_object_set(config_section, "CustomField1", json_string("value1"));
    json_object_set(config_section, "CustomField2", json_integer(42));
    // ... set all fields

    json_object_set(root, "ConfigSection", config_section);

    AppConfig config = {0};
    bool result = load_config_function(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.enabled);
    TEST_ASSERT_EQUAL_STRING("value1", config.custom_field1);
    TEST_ASSERT_EQUAL(42, config.custom_field2);
    // ... verify all fields

    json_decref(root);
    cleanup_config(&config);
}
```

#### Error Condition Testing

**Test all error paths systematically**:

```c
void test_function_null_parameters(void) {
    TEST_ASSERT_FALSE(function_under_test(NULL, valid_param));
    TEST_ASSERT_FALSE(function_under_test(valid_param, NULL));
    TEST_ASSERT_FALSE(function_under_test(NULL, NULL));
}

void test_function_malformed_data(void) {
    // Test with invalid JSON structures
    json_t* malformed = json_string("not an object");
    TEST_ASSERT_FALSE(function_under_test(malformed));
}

void test_function_edge_cases(void) {
    // Test empty arrays, oversized inputs, boundary values
    json_t* empty_array = json_array();
    TEST_ASSERT_TRUE(function_under_test(empty_array));  // Should handle empty gracefully
}
```

#### Complex Data Structure Testing

**Test nested structures comprehensively**:

```c
void test_services_array_processing(void) {
    // Test empty services array
    // Test single service with minimal fields
    // Test single service with all fields
    // Test multiple services
    // Test services with TXT records
    // Test malformed service entries
    // Test memory allocation failures during service processing
}
```

#### Testing Cryptographic and Signing Functions

Functions that verify cryptographic signatures need a real key pair. Generate one inside the test file using the library's own API rather than embedding hardcoded test vectors (which can become stale or incorrect).

Use a module-level static key pointer that is generated once and freed in `main()` after `UNITY_END()`:

```c
static EVP_PKEY* g_test_private_key = NULL;

// Helper: generate a 2048-bit RSA key pair (called lazily by tests that need it)
static void generate_test_keypair(void) {
    if (g_test_private_key != NULL) return;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!ctx) return;
    if (EVP_PKEY_keygen_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0 ||
        EVP_PKEY_keygen(ctx, &g_test_private_key) <= 0) {
        g_test_private_key = NULL;
    }
    EVP_PKEY_CTX_free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rs256_verify_valid_signature);
    // ... other tests ...
    int result = UNITY_END();

    // Free shared resources after all tests have run
    if (g_test_private_key) {
        EVP_PKEY_free(g_test_private_key);
        g_test_private_key = NULL;
    }
    return result;
}
```

An RSA key using `EVP_PKEY` serves as both the private key (for signing in tests) and public key (for verification by the function under test) since it contains both components.

#### Skipping Tests When Prerequisites Are Unavailable

Use `TEST_IGNORE_MESSAGE()` when a test requires a resource (like a generated key pair) that could not be set up. This marks the test as ignored rather than failing, which is honest about why the test did not run:

```c
void test_rs256_verify_valid_signature(void) {
    generate_test_keypair();
    if (!g_test_private_key) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    // ... test body ...
}
```

This is preferable to `TEST_FAIL_MESSAGE` when the skip reason is an environment constraint rather than a code defect.

#### Coverage Maximization Strategies

1. Ensure that there are NO static functions as those cannot be tested
2. Split larger source files (particuarly > 300 instrumented lines) into smaller files
3. Consider using helper functions to make large complex functions easier to test
4. Target high-value functions that provide maximum coverage gain
5. Test all public functions in the module
6. Exercise all code paths with different input combinations
7. Validate coverage reports regularly to identify gaps
8. Test helper functions indirectly through public interfaces
9. Focus on complex logic branches that are hard to test through integration tests

#### Test Organization Best Practices

**Structure tests by functionality**:

```c
// Parameter validation tests
void test_function_null_parameters(void);
void test_function_empty_inputs(void);
void test_function_invalid_parameters(void);

// Normal operation tests
void test_function_basic_functionality(void);
void test_function_custom_values(void);
void test_function_default_values(void);

// Edge case tests
void test_function_boundary_conditions(void);
void test_function_extreme_values(void);

// Complex scenario tests
void test_function_complex_configurations(void);
void test_function_nested_structures(void);

// Error handling tests
void test_function_error_recovery(void);
void test_function_cleanup_on_failure(void);

// Resource management tests
void test_function_memory_management(void);
void test_function_resource_cleanup(void);
```

#### Performance Considerations

**Keep unit tests fast**:

- Avoid sleep() calls or long delays
- Use minimal timing delays when needed (microseconds, not seconds)
- Test timeout scenarios appropriately
- Ensure test suite completes in seconds, not minutes

**Handle global state carefully**:

```c
void setUp(void) {
    // Reset global state before each test
    reset_global_variables();
    initialize_test_fixtures();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_resources();
    reset_global_state();
}
```

### Decision Framework for Unity Testing

**Before investing in Unity tests for a module**:

1. **Assess Function Safety**: Can functions be called safely in isolation?
2. **Evaluate Dependencies**: Do functions require system resources or external contexts?
3. **Check Existing Coverage**: Do integration tests already provide adequate coverage?
4. **Estimate Effort vs. Benefit**: Will the Unity tests provide proportional value?
5. **Consider Alternatives**: Would integration or component tests be more appropriate?

**Skip Unity tests when**:

- Functions require external library contexts that can't be easily mocked
- Code depends heavily on global state or system initialization
- Functions are primarily "glue code" between system components
- Existing integration tests already provide adequate coverage
- The effort to create reliable unit tests exceeds the practical benefit

**Key Lesson**: Not all code needs or benefits from unit tests. System-dependent code is often better validated through integration testing that exercises the complete system in realistic conditions.

## GCOV Regeneration Issues

### The Problem: Test 80 Causes Source File GCOV Regeneration

**Symptom**: When Test 80 (Code Size Analysis) runs after Test 10 (Unity), it caused `rc/launch/launch_database.c.gcov` to be regenerated with a different (smaller) coverage result, even though the underlying `.gcda` and `.gcno` files hadn't changed.

**Symptom**: Test 00 reports a count discrepancy between coverage_table.sh and Test 99 (ultimately Test 10) when the instrumented unity lines in coverage_table.sh don't match the extended CLOC coverage lines reported in the extended table, even when the total blackbox and unity instrumented lines match the total instrumented lines in coverage_table.sh, which is normally the cause of this meessage.

**Root Cause**: The `calculate_test_instrumented_lines` function processes test files (like `launch_database_test_enhanced_coverage.gcno`) to count test instrumentation. When gcov processes these test files, it analyzes **all linked source files** (including `launch_database.c`), causing their `.gcov` files to be regenerated.

**Why Only launch_database.c?**: This file was unique because it had multiple dedicated test files that directly link against it. Other source files don't have corresponding test files, so they aren't affected. This meant that when gcov processed the test files, they triggered a regen of the source C file based on whatever gcov coverage came from the tests, invalidating the original Test 10 coverage, so ironically *lowering* the actual coverage reported.

### The Fix: Proper Header-Only Includes

**Solution Applied**: Removed improper `#include "launch_database.c"` includes from test files and added missing declarations to `src/launch/launch.h`.

**Before (Problematic)**:

```c
#include "launch_database.c"  // ❌ Direct .c include creates gcov dependency
```

**After (Correct)**:

```c
#include "launch.h"  // ✅ Header-only include, no gcov dependency
```

### Prevention: Dependency Check in make-trial.sh

**Added Dependency Check**: The trial build now includes a "Dependency Check" section that validates no `.c` files are included in Unity tests or mocks:

```bash
# Check for improper .c includes in Unity tests
UNITY_C_INCLUDES=$(grep -r "\.c" tests/unity/src 2>/dev/null | grep -i include || true)
MOCKS_C_INCLUDES=$(grep -r "\.c" tests/unity/mocks 2>/dev/null | grep -i include || true)

if [[ -n "${UNITY_C_INCLUDES}" ]]; then
    echo "❌ Found improper .c includes in Unity tests:"
    echo "${UNITY_C_INCLUDES}"
    exit 1
fi
```

**Benefits**:

- **Early Detection**: Catches improper includes during trial builds (`mkt`)
- **Prevents GCOV Issues**: Stops the root cause before it affects coverage calculations
- **Maintains Coverage Integrity**: Ensures test coverage data remains consistent between Test 10 and Test 80

### Why This Matters

**Coverage Data Integrity**: GCOV regeneration "resets" coverage data, causing inconsistent results between test runs. This makes coverage analysis unreliable and debugging difficult.

**Test Isolation**: Unity tests should only depend on header files, not implementation files. Direct `.c` includes violate this principle and create unintended side effects.

**Build Consistency**: Trial builds should catch architectural issues before they affect the full test suite.

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
    double startup_time = calculate_start_time();
    TEST_ASSERT_GREATER_THAN(0.0, startup_time);  // May fail if ready time not set
}
```

**Timing Test Patterns**:

- Set prerequisite state first (e.g., call setup functions)
- Add minimal delay only if needed for timing precision
- Test both the "times not set" and "times properly set" scenarios
- Use conditional assertions when functions have legitimate zero return values

## Mocking Framework for Unity Tests

The Hydrogen project includes a comprehensive mocking framework that enables testing of system-dependent code and error conditions. This framework allows you to achieve high test coverage (75%+) by simulating failures and edge cases that are difficult or impossible to trigger in real system environments.

### Available Mocks

The project currently provides the following mock libraries:

#### 1. **mock_libwebsockets** (`tests/unity/mocks/mock_libwebsockets.c/h`)

- **Purpose**: Mock WebSocket library functions for testing WebSocket-related code
- **Functions**: `lws_create_context`, `lws_write`, `lws_service`, `lws_get_context`, etc.
- **Usage**: Enable with `#define USE_MOCK_LIBWEBSOCKETS` in test files

#### 2. **mock_libmicrohttpd** (`tests/unity/mocks/mock_libmicrohttpd.c/h`)

- **Purpose**: Mock HTTP server functions for testing HTTP-related code
- **Functions**: MHD connection and response handling functions
- **Usage**: Enable with `#define USE_MOCK_LIBMICROHTTPD` in test files

#### 3. **mock_launch** (`tests/unity/mocks/mock_launch.c/h`)

- **Purpose**: Mock launch system functions for testing subsystem initialization
- **Functions**: Launch phase management and subsystem coordination
- **Usage**: Enable with `#define USE_MOCK_LAUNCH` in test files

#### 4. **mock_landing** (`tests/unity/mocks/mock_landing.c/h`)

- **Purpose**: Mock landing system functions for testing subsystem shutdown
- **Functions**: Landing phase management and graceful shutdown
- **Usage**: Enable with `#define USE_MOCK_LANDING` in test files

#### 5. **mock_status** (`tests/unity/mocks/mock_status.c/h`)

- **Purpose**: Mock status reporting functions for testing system monitoring
- **Functions**: Status collection and reporting functions
- **Usage**: Enable with `#define USE_MOCK_STATUS` in test files

#### 6. **mock_info** (`tests/unity/mocks/mock_info.c/h`)

- **Purpose**: Mock info extraction functions for testing system information
- **Functions**: WebSocket metrics and system info extraction
- **Usage**: Enable with `#define USE_MOCK_INFO` in test files

#### 7. **mock_network** (`tests/unity/mocks/mock_network.c/h`) - *New*

- **Purpose**: Mock network functions for testing network-dependent code
- **Functions**: `get_network_info`, `filter_enabled_interfaces`, `create_multicast_socket`
- **Usage**: Enable with `#define USE_MOCK_NETWORK` in test files

#### 8. **mock_system** (`tests/unity/mocks/mock_system.c/h`) - *New*

- **Purpose**: Mock system functions for testing error conditions
- **Functions**: `malloc`, `free`, `strdup`, `gethostname`
- **Usage**: Enable with `#define USE_MOCK_SYSTEM` in test files

### Globally Pre-Defined Mocks

The CMake build system passes several `USE_MOCK_*` defines to **every** Unity test compilation unit. Do **not** re-define these in your test files — the compiler will emit a `-Werror` redefinition error.

Mocks that are always active (defined globally in CMakeLists.txt):

- `USE_MOCK_LOGGING` — `log_this` is remapped to `mock_log_this` for all Unity tests
- `USE_MOCK_LIBMICROHTTPD` — MHD functions mocked globally
- `USE_MOCK_SYSTEM` — `malloc`/`free`/`strdup`/`gethostname` mocked globally

Because `USE_MOCK_LOGGING` is always active, you never need to `#include <unity/mocks/mock_logging.h>` or call `mock_logging_reset_all()` just to silence log output — logging is already redirected. Include the mock header only if you need to **inspect** what was logged (e.g., asserting the log message content via `mock_logging_get_last_message()`).

```c
// ❌ WRONG - causes -Werror redefinition compile failure
#define USE_MOCK_LOGGING
#include <unity/mocks/mock_logging.h>

// ✅ CORRECT - already active globally, no action needed
// log_this() calls in the code under test are silently captured
```

For mocks that are **not** globally pre-defined, the normal `#define USE_MOCK_X` / `#include` pattern applies.

### How to Use Mocks in Test Files

#### Basic Setup Pattern

```c
// Include necessary headers
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
// NOTE: USE_MOCK_LOGGING, USE_MOCK_LIBMICROHTTPD, USE_MOCK_SYSTEM are
// already defined globally by CMake - do not redefine them here.
#define USE_MOCK_NETWORK
#include <unity/mocks/mock_network.h>
#include <unity/mocks/mock_system.h>

// Include source headers (functions will be mocked)
#include <src/network/network.h>
#include <src/mdns/mdns_server.h>

// Test setup
void setUp(void) {
    // Reset all mocks to default state
    mock_network_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_network_reset_all();
    mock_system_reset_all();
}
```

#### Mock Control Functions

Each mock provides control functions to simulate different scenarios:

```c
// Network mock controls
mock_network_set_get_network_info_result(NULL);           // Simulate network failure
mock_network_set_create_multicast_socket_result(-1);      // Simulate socket failure
mock_network_reset_all();                                 // Reset to defaults

// System mock controls
mock_system_set_malloc_failure(1);                        // Make malloc fail
mock_system_set_gethostname_failure(1);                   // Make gethostname fail
mock_system_set_gethostname_result("custom-host");        // Set custom hostname
mock_system_reset_all();                                  // Reset to defaults
```

#### Example: Testing Error Conditions

```c
void test_mdns_server_init_network_failure(void) {
    // Setup: Mock network failure
    mock_network_set_get_network_info_result(NULL);

    // Test: Call function that should handle failure
    mdns_server_t *server = mdns_server_init(...);

    // Assert: Should return NULL on network failure
    TEST_ASSERT_NULL(server);
}

void test_mdns_server_init_malloc_failure(void) {
    // Setup: Mock memory allocation failure
    mock_system_set_malloc_failure(1);

    // Test: Call function that allocates memory
    mdns_server_t *server = mdns_server_init(...);

    // Assert: Should return NULL on allocation failure
    TEST_ASSERT_NULL(server);
}
```

### Adding New Mocks

To add a new mock for additional system functions:

#### 1. Create Mock Header (`mock_newfeature.h`)

```c
#ifndef MOCK_NEWFEATURE_H
#define MOCK_NEWFEATURE_H

// Enable mock override
#ifdef USE_MOCK_NEWFEATURE
#define some_system_function mock_some_system_function
#endif

// Mock function prototypes
void *mock_some_system_function(int param);

// Mock control functions
void mock_newfeature_set_result(void *result);
void mock_newfeature_reset_all(void);

#endif // MOCK_NEWFEATURE_H
```

#### 2. Create Mock Implementation (`mock_newfeature.c`)

```c
#include "mock_newfeature.h"

// Static state
static void *mock_result = NULL;

// Mock implementation
void *mock_some_system_function(int param) {
    (void)param; // Suppress unused parameter
    return mock_result;
}

// Control functions
void mock_newfeature_set_result(void *result) {
    mock_result = result;
}

void mock_newfeature_reset_all(void) {
    mock_result = NULL;
}
```

#### 3. Update CMakeLists.txt

```cmake
set(UNITY_MOCK_SOURCES
    # ... existing mocks ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_newfeature.c
)
```

#### 4. Update Test Files

```c
#define USE_MOCK_NEWFEATURE
#include <unity/mocks/mock_newfeature.h>
```

### Mock Integration in Build System

Mocks are automatically integrated into the Unity test build process:

1. **CMake Integration**: Mocks are compiled as separate object files and linked into test executables
2. **Preprocessor Control**: `#define USE_MOCK_*` enables function overrides
3. **Clean Separation**: Mocks don't affect production code, only test builds
4. **Parallel Building**: Mock compilation is included in the parallel build process

### Best Practices for Mock Usage

#### 1. **Reset State Between Tests**

```c
void setUp(void) {
    mock_network_reset_all();
    mock_system_reset_all();
    // Reset any other mocks used in the test file
}
```

#### 2. **Test One Failure Mode Per Test**

```c
// Good: Single failure mode
void test_function_network_failure(void) {
    mock_network_set_get_network_info_result(NULL);
    // Test network failure handling
}

// Avoid: Multiple failure modes
void test_function_multiple_failures(void) {
    mock_network_set_get_network_info_result(NULL);
    mock_system_set_malloc_failure(1);
    // Hard to debug which failure caused the issue
}
```

#### 3. **Document Mock Dependencies**

```c
/*
 * Test: mdns_server_init_network_failure
 * Mocks: mock_network (get_network_info returns NULL)
 * Purpose: Verify graceful handling of network unavailability
 */
void test_mdns_server_init_network_failure(void) {
    // Implementation
}
```

#### 4. **Use Realistic Mock Data**

```c
// Good: Realistic network interface data
network_info_t *realistic_info = create_realistic_network_info();
mock_network_set_get_network_info_result(realistic_info);

// Avoid: Minimal mock data that doesn't exercise code paths
```

#### 5. **Test Cleanup Functions**

```c
// Test that mocks are properly reset
void test_mock_cleanup(void) {
    mock_system_set_malloc_failure(1);
    // Run test...

    // Verify cleanup
    mock_system_reset_all();
    // Verify function works normally again
}