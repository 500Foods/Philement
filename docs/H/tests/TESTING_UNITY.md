# Unity Unit Tests

Unity tests provide comprehensive unit testing for individual functions and modules within the Hydrogen codebase. The Unity testing framework enables thorough validation of core logic, edge cases, and boundary conditions at the function level. Of particular note, the testing framework is constructed in such a way as to create individual executable tests that link directly to object files of the project. We're not adding
any code at all to our release build that has any kind of instrumentation in it. Linking the object code directly produces coverage that can be compared with the blackbox run. The combined report is what the coverage gate uses.

IMPORTANT: The main purpose of the Unity Framework Testing in this project is to improve overall code quality. Adding tests often uncovers bugs or shortcomings in the codebase, and those get fixed. Sometimes the test's assumptions do not match the code, and the test gets adjusted. Sometimes the assumptions are valid and the code gets adjusted.

## IMPROVING COVERAGE TASK

A frequent task is improving coverage for a single source file. Start with `extras/add_coverage.sh`, which lists instrumented lines covered by neither Unity nor the blackbox run. Sometimes that is the whole file. From there:

1) Write tests to cover larger functions that have no coverage currently
2) Refactor larger functions to use helpers that are more easily tested
3) Refactor larger source code files into smaller files that can be reused or made small enough to fall under a lower threshold if fewer than 100 lines
4) Check that callable functions in `src/` are not `static`. Unity links the object files and cannot call a `static` function. `static` inside a test file is allowed. The trial build rejects a new `static` function in `src/` that is not already named in `tests/.static-baseline.txt`.

The lines that matter are the ones neither suite has covered. A line already hit by a blackbox run counts toward the combined gate. New tests can go in an existing test file, or in a new file. A new or renamed file under `tests/unity/src/` needs `mkt` first, because CMake globs `*_test*.c` at configure time. `mkq` will not see it. Once the file is in the build, `mku <basename without .c>` rebuilds and runs that one test. Run the aliases from a zsh that loads `~/.zshrc` (`zsh -ic 'mku mutex_test_stats'`).

The combined gates are in [`/docs/H/CURIOSITIES.md`](/docs/H/CURIOSITIES.md). A file with fewer than 100 instrumented lines needs more than 50 percent combined coverage. A file with 100 or more needs more than 75 percent. Once the tests pass, stop and ask for confirmation of the new coverage and of the lines that are still uncovered when the gate is not met.

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

// Standard project header plus Unity Framework header.
// hydrogen.h is included first. -Wmissing-prototypes is -Werror, so every
// test function has one prototype before setUp().
#include <src/hydrogen.h>
#include <unity.h>

#include <src/path/to/module.h>

// Prototype of the function under test, when the header does not already
// declare it. Do not #include the .c file.
bool function_to_test(const SomeType* input);

static void test_function_name_basic_functionality(void);
static void test_function_name_null_parameter(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_function_name_basic_functionality(void) {
    SomeType valid_input = {0};
    TEST_ASSERT_TRUE(function_to_test(&valid_input));
}

static void test_function_name_null_parameter(void) {
    TEST_ASSERT_FALSE(function_to_test(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_function_name_basic_functionality);
    RUN_TEST(test_function_name_null_parameter);
    return UNITY_END();
}
```

`static` on the test functions themselves is optional. The ban on `static` applies to callable functions in `src/`, not to helpers in the test file. Do not `freopen` stdout. `mku` and Test 89 read Unity's summary from stdout. See Performance and Timing below.

### 🔠 **Critical: Test File Naming Conventions**

Unity test file naming has two independent constraints that must both be satisfied. Failing either one triggers Test 89 (coverage validation) or a CMake/CTest build error.

#### Constraint 1: Coverage Orphan Detection (Test 89)

Test 89's orphan-detection logic strips everything from `_test` onwards from the test filename to derive the expected source file basename. The test file **must** map to an existing source file in `src/`:

```bash
# Test script logic (test_89_coverage.sh, line 348):
#   src_f="${f/tests\/unity\/src/src}"       # swap tests/unity/src → src
#   src_f="${src_f%%_test*.c}.c"             # strip from _test onward, add .c
#   [[ ! -f "${src_f}" ]] && echo "ORPHAN"   # file must exist in src/

# ✅ CORRECT — maps to src/database/mssql/query.c
tests/unity/src/database/mssql/query_test_format_helpers.c

# ❌ WRONG — maps to src/database/mssql/mssql_query.c (does not exist!)
tests/unity/src/database/mssql/mssql_query_test_format_helpers.c
```

**Rule**: The portion of the test filename *before* `_test` must exactly match the source file basename (minus `.c`). Since the source file is `query.c`, the test file must start with `query_test_`, **not** `mssql_query_test_`.

#### Constraint 2: CMake/CTest Basename Uniqueness

CTest registers tests by basename (path and `.c` suffix stripped). If two test files in the same CMake `add_test` scope produce the same basename, CMake errors out:

```cmake
# These collide because both produce basename "connection_test_check"
tests/unity/src/database/postgresql/connection_test_check_timeout_expired.c → connection_test_check_timeout_expired
tests/unity/src/database/mssql/connection_test_check_timeout_expired.c     → connection_test_check_timeout_expired  # COLLISION
```

**Rule for engine-specific source files with identically-named functions**: When two different modules share the same function name (e.g., `connection_test_check_timeout_expired.c` exists for both PostgreSQL and MSSQL), use a **per-engine suffix** to disambiguate the *entire basename*:

```bash
# ✅ CORRECT — unique basenames across both modules
tests/unity/src/database/postgresql/connection_test_check_timeout_expired.c
tests/unity/src/database/mssql/connection_test_mssql_check_timeout_expired.c

# The engine suffix goes right after "connection_test_", not as a prefix
# This keeps the orphan-detection mapping intact (maps to connection.c)
# since "mssql_" is part of the function-specific portion, not the source basename.
```

**Combined checklist for new test files**:

1. Verify the target source file exists in `src/` (e.g., `src/database/mssql/connection.c`)
2. Start the test filename with the exact source basename + `_test_` (e.g., `connection_test_`)
3. Check whether any existing test file in `tests/unity/src/` already uses the same basename after CMake strips the path — if so, add a disambiguating suffix
4. Run `mkt` to confirm CMake accepts the naming (basename collisions surface as CMake configuration errors)
5. Run `test_89` (or the coverage orphan-check section) to confirm no orphan warnings

#### Example: Correct naming for MSSQL module

Source files: `src/database/mssql/query.c`, `src/database/mssql/connection.c`

| Test file | Maps to (orphan check) | CMake basename |
| --- | --- | --- |
| `query_test_format_helpers.c` | `src/database/mssql/query.c` ✓ | `query_test_format_helpers` |
| `connection_test_mssql_connect.c` | `src/database/mssql/connection.c` ✓ | `connection_test_mssql_connect` |

### File Naming Issues Encountered

During MSSQL coverage improvement work, two naming issues were hit and resolved:

1. **Orphan false-positives**: Test files were initially named `mssql_query_test_*.c`, which the orphan detector mapped to the non-existent `src/database/mssql/mssql_query.c`. Renamed to `query_test_*.c` to correctly map to `query.c`.

2. **CMake basename collisions**: MSSQL connection test files (`connection_test_*.c`) collided with PostgreSQL connection test files (`connection_test_*.c`) sharing identical basenames. Resolved by renaming MSSQL connection tests to `connection_test_mssql_*.c`, inserting the engine suffix after the `connection_test_` prefix so the orphan mapping still resolves to `connection.c`.

3. **Misleading test names for split implementations**: The `api/conduit/query/` module splits implementation across multiple files (query.c, helpers/request_parsing.c, helpers/database_operations.c, helpers/query_execution.c, etc.), but test files under `tests/unity/src/api/conduit/query/` are named `query_test_<function>.c` regardless of which source file the function actually lives in. For example, `query_test_validate_http_method.c` tests `validate_http_method()` which is defined in `src/api/conduit/helpers/request_parsing.c`, not `query.c`. The orphan-detection constraint (Constraint 1) is still satisfied because the filename prefix `query_test_` correctly maps to `query.c` — the test *file* belongs to the query test suite. However, the test name is misleading: it implies the function is in `query.c` when it lives in a helper. When coverage drops unexpectedly on a source file, verify that the test file actually exercises that file's functions, not just that the filename prefix matches. The file `query_test_handle_conduit_query_request.c` is especially problematic: it is named for `handle_conduit_query_request()` but does not actually test that function (the main handler requires an MHD connection context that cannot be easily mocked); it tests smaller helpers instead. To avoid confusion, rename test files to reflect the actual function under test, or include a clear comment indicating which function is being tested and which source file it is defined in.

## CMake Build Integration

Unity tests are discovered by a CMake glob and built with coverage instrumentation. Discovery happens at configure time.

**Build Commands**:

`mku` builds and runs one Unity test from any directory. Pass the basename with no directory and no `.c`. A new or renamed test file needs `mkt` first. `mkq` does not reconfigure, so it keeps the old glob.

```bash
zsh -ic 'mku payload_test_cleanup_openssl'
```

**Key Integration Features**:

- **Configure-time discovery**: `cmake/CMakeLists-unity.cmake` globs `tests/unity/src/*_test*.c`. No hand-written source list. The glob runs at configure time, which is what `mkt` does.
- **Parallel Building**: Multiple Unity tests can be built simultaneously
- **Coverage Instrumentation**: Unity objects are compiled with gcov
- **Dependency Management**: Tests link the required libraries and the project objects. Link the objects. Do not `#include` a `.c` file.

## Coverage-Driven Workflow

The most efficient way to check Unity coverage is to use the `add_coverage.sh` tool in `extras/` before writing any tests. It cross-references the Unity gcov data against the blackbox gcov data and reports only the lines that are uncovered in **both** suites — the lines where a new Unity test will actually move the combined coverage needle.

### Step-by-Step Coverage Improvement Process

```bash
# 1. Reconfigure when a src/ or tests/unity/src/ file was added or renamed.
#    mkq is enough when every file involved is already in the build.
zsh -ic 'mkt'

# 2. Lines missing from both coverage sets. The path is relative to src/.
cd "$HYDROGEN_ROOT"
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

To add a new Unity test:

1. Put the file in `tests/unity/src/` so the directory mirrors `src/`.
2. Name it `<source>_test_<function>.c`. The text before `_test` is the source basename. The basename must be unique across `tests/unity/src/`. Search before creating the file.
3. Include `<src/hydrogen.h>` first, then `<unity.h>`, then the module header. Give every test function one prototype before `setUp()`. Do not `#include` a `.c` file.
4. Do not `#define` a `USE_MOCK_*` macro that CMake already passes for that path. See Mocking Framework below.
5. Run `zsh -ic 'mkt'` for a new or renamed file, then `zsh -ic 'mku <basename>'`.

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

1. Ensure callable functions in `src/` are not `static`. `static` in the test file is allowed
2. Split larger source files (particularly those over 300 instrumented lines) into smaller files
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

## Do Not Include a .c File

A Unity test includes headers and links the project objects. `#include` of a `.c` file makes gcov regenerate that source's `.gcov` from the test link, which replaces the coverage Test 10 already recorded. Code size is Test 99. There is no Test 80.

```c
// Wrong: pulls the implementation into the test and refreshes its .gcov
#include "launch_database.c"

// Right: the prototype lives in the header, and the test links the object
#include <src/launch/launch.h>
```

`extras/make-trial.sh` rejects an include of a `.c` under `tests/unity/src/` and `tests/unity/mocks/`. `mkt` runs that check. The trial build fails before the suite if one is present.

## Performance and Timing Best Practices

Based on lessons learned from implementing previous tests.

**Avoid Long Delays in Unit Tests**:

- Never use `sleep(1)` or longer delays - unit tests should complete in milliseconds, not seconds
- A test suite that takes 5+ seconds due to sleep calls will slow down development workflow significantly

**Do NOT Rebind `stdout` with `freopen`**:

Unity prints its result summary (`N Tests M Failures K Ignored` plus the
`[RUN]`/`[FAIL]`/`[IGNORED]` lines and final `OK`/`FAIL`) to `stdout` from
`UNITY_END()`. The `mku` runner and coverage reporting (test_89) parse that
summary out of the test's standard output, so if it is missing the test is
reported as passing but its sub-test count is never recorded.

`freopen("/dev/null", "w", stdout)` rebinds the **process-wide** `stdout`
`FILE*` for the rest of the process. Any code under test that writes to
`stdout` (e.g. a console/log sink) is silenced, but so is Unity's own summary
— it is thrown away and the test "silently" passes with no `N Tests ...` line.
This is exactly the failure mode where the test runs and exits 0, yet coverage
reporting complains the sub-tests were not recorded.

- **Bad**: `freopen("/dev/null", "w", stdout);` to hide noisy console output —
  it also hides Unity's summary.
- **Good**: redirect only the underlying file descriptor and restore it in
  `tearDown()` (before `UNITY_END()` runs), so the summary still reaches the
  real `stdout`:

  ```c
  static int g_saved_stdout = -1;

  static void silence_stdout(void) {
      if (g_saved_stdout != -1) return;
      g_saved_stdout = dup(1);
      int devnull = open("/dev/null", O_WRONLY);
      if (devnull != -1) {
          dup2(devnull, 1);
          close(devnull);
      }
  }

  static void restore_stdout(void) {
      if (g_saved_stdout == -1) return;
      dup2(g_saved_stdout, 1);
      close(g_saved_stdout);
      g_saved_stdout = -1;
  }
  ```

  Call `silence_stdout()` in `setUp()` and `restore_stdout()` in `tearDown()`.
  The console noise is suppressed during each test, but `stdout` is restored
  before `UNITY_END()` prints the summary. (`<fcntl.h>`, `<unistd.h>` required.)

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

### Where the mocks are

The mock sources are [`/elements/001-hydrogen/hydrogen/tests/unity/mocks/`](/elements/001-hydrogen/hydrogen/tests/unity/mocks/). The files CMake compiles are `UNITY_MOCK_SOURCES` in [`/elements/001-hydrogen/hydrogen/cmake/CMakeLists-unity.cmake`](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-unity.cmake). That list is longer than the eight mocks this page used to name. It includes the database client mocks (PostgreSQL, MySQL, MariaDB, DB2, ODBC for MSSQL, Firebird, SQLite), pthread, crypto, threads, terminal WebSocket, database engine, migrations, dbqueue, API utils, auth, chat, conduit, and the webserver core. Read the CMake list or [`mocks/README.md`](/elements/001-hydrogen/hydrogen/tests/unity/mocks/README.md) before assuming a mock does not exist. The README lags the CMake list.

### Which USE_MOCK macros CMake already passes

Do not `#define` a macro CMake already passes. `-Werror` treats the redefinition as a failure.

Source objects are always compiled with `-DUSE_MOCK_LOGGING` and `-Dlog_this=mock_log_this`. Include `mock_logging.h` only when the test inspects captured lines (`mock_logging_reset_all()`, `mock_logging_message_contains()`). Logging is already redirected, so a test does not include that header just to silence `log_this`.

The per-test switch in `CMakeLists-unity.cmake` is path-based, and the first matching branch wins. The default `else` branch passes `USE_MOCK_LOGGING`, `USE_MOCK_LIBMICROHTTPD`, and `USE_MOCK_SYSTEM`. These paths replace that set. The engine-name checks sit above the generic `database` check, so `tests/unity/src/database/postgresql/` takes the PostgreSQL branch:

- A path containing `postgresql`, `mysql`, `mariadb`, `db2`, `mssql`, or `sqlite` gets that engine's client mock (`USE_MOCK_LIBPQ`, `USE_MOCK_LIBMYSQLCLIENT`, `USE_MOCK_LIBMARIADB`, `USE_MOCK_LIBDB2`, `USE_MOCK_LIBODBC`, `USE_MOCK_LIBSQLITE3`). It does not get the three default macros.
- A path containing `websocket` gets `USE_MOCK_LIBWEBSOCKETS`, `USE_MOCK_STATUS`, and `USE_MOCK_TERMINAL_WEBSOCKET`.
- `database`, `conduit`, `launch`, `webserver`, `auth`, and `oidc` paths add their own sets. Read the `elseif` for that directory before adding a `#define`.

```c
// Wrong when CMake already passed -DUSE_MOCK_LOGGING. -Werror stops the build.
#define USE_MOCK_LOGGING
#include <unity/mocks/mock_logging.h>

// Right when the test only needs the redirected log_this.
// Include the header when the test asserts on a captured line.
```

A mock that CMake did not pass for this test still uses `#define USE_MOCK_<NAME>` before the mock header.

### How to Use Mocks in Test Files

#### Basic Setup Pattern

```c
// Include necessary headers
#include <src/hydrogen.h>
#include <unity.h>

// Default-path tests already receive USE_MOCK_LOGGING, USE_MOCK_LIBMICROHTTPD,
// and USE_MOCK_SYSTEM. Do not #define those three there.
// USE_MOCK_NETWORK is not in the default set. The mdns branch already passes
// it, so this #define belongs in a test that is not under tests/unity/src/mdns/.
#define USE_MOCK_NETWORK
#include <unity/mocks/mock_network.h>
#include <unity/mocks/mock_system.h>

#include <src/network/network.h>

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

#### 3. Add the mock to UNITY_MOCK_SOURCES

The list is in `cmake/CMakeLists-unity.cmake`, not a hand-maintained entry in `cmake/CMakeLists.txt`.

```cmake
set(UNITY_MOCK_SOURCES
    # ... existing mocks ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_newfeature.c
)
```

A new file is picked up on the next `mkt`. If the test path needs the macro, add it to that path's branch in the same file. The default `else` branch does not see a new macro until that branch passes it.

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