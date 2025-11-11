# Unity Test Mocks

This directory contains comprehensive mock implementations for the Hydrogen Unity testing framework. These mocks enable isolated unit testing by simulating external dependencies, system calls, and library functions that would otherwise make unit tests unreliable or impossible.

## Quick Reference for AI Agents

**Available Mocks:**

- `mock_libmicrohttpd` - HTTP server functions (MHD_*)
- `mock_libwebsockets` - WebSocket library functions (lws_*)
- `mock_system` - System calls (malloc, fork, gethostname, etc.)
- `mock_network` - Network interface functions
- `mock_launch` - Launch system functions
- `mock_landing` - Shutdown system functions
- `mock_status` - Status reporting functions
- `mock_info` - Info extraction functions
- `mock_libpq` - PostgreSQL client library
- `mock_libmysqlclient` - MySQL client library
- `mock_libdb2` - IBM DB2 client library
- `mock_libsqlite3` - SQLite library
- `mock_threads` - Threading functions
- `mock_pthread` - POSIX thread functions
- `mock_terminal_websocket` - Terminal WebSocket functions
- `mock_logging` - Logging functions
- `mock_database_engine` - Database abstraction layer
- `mock_database_migrations` - Database migration functions
- `mock_db2_transaction` - DB2 transaction functions
- `mock_dbqueue` - Database queue functions

**Key Usage Patterns:**

1. Define `USE_MOCK_*` macros BEFORE any includes
2. Include mock headers immediately after defines
3. Include Unity framework next
4. Include source headers (functions now mocked)
5. Reset mocks in `setUp()` and `tearDown()`

**Example Test Structure:**

```c
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>
#include <unity.h>
#include <src/api/conduit/query/query.h>

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
}
```

**Common Examples:**

- Memory allocation testing: `mock_system_set_malloc_failure(1)`
- HTTP request mocking: `mock_mhd_add_lookup("Content-Type", "application/json")`
- WebSocket testing: `mock_lws_set_write_result(LWS_WRITE_TEXT)`

**Functional Examples by Mock:**

- `mock_libmicrohttpd`: `tests/unity/src/api/conduit/query/query_test_parse_request_data.c`
- `mock_libwebsockets`: `tests/unity/src/websocket/websocket_server_message_test_handle_message_type.c`
- `mock_system`: `tests/unity/src/queue/queue_test_error_paths.c`
- `mock_network`: `tests/unity/src/mdns/mdns_server_init_test.c`

## Overview

The mocking framework provides controlled environments for testing code that interacts with:

- External libraries (libmicrohttpd, libwebsockets, database libraries)
- System functions (memory allocation, file I/O, process management)
- Network operations
- Threading and synchronization primitives

## How Mocks Work

Mocks use preprocessor macros to override function calls at compile time. When `USE_MOCK_*` is defined, the real function calls are redirected to mock implementations that can be controlled during testing.

```c
// In test file
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Now MHD_lookup_connection_value() calls mock implementation
```

## Mocking Individual Functions

### Include Order Challenges

A common challenge in the Hydrogen project is that `<src/hydrogen.h>` is included in nearly every source file, which brings in function declarations before test files can define mock overrides. This creates a "chicken and egg" problem where the real function declarations are seen before the mock macros can take effect.

### Proper Setup Pattern

To successfully mock individual functions, follow this exact pattern:

```c
// 1. Define mock macros BEFORE including ANY source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#define USE_MOCK_NETWORK

// 2. Include mock headers immediately after defines
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_network.h>

// 3. Include Unity framework
#include <unity.h>

// 4. Include source headers (functions will now be mocked)
#include <src/api/conduit/query/query.h>

// 5. Include any additional headers needed for the test
#include <src/some_other_header.h>
```

**Critical**: The `#define USE_MOCK_*` directives must appear before ANY `#include` statements that bring in source code. This ensures the preprocessor macros override the function names before the real declarations are encountered.

### When to Modify Source Code for Mocking

While the general principle is that tests remain external to source code, there are exceptional cases where minimal source code modifications are acceptable to enable critical testing:

#### Acceptable Modifications

1. **Adding `extern` Declarations**: When a function is declared `static` but needs to be tested, adding an `extern` declaration in the test file is acceptable.

2. **Weak Function Attributes**: Using GCC's `__attribute__((weak))` to allow function overriding in test builds.

3. **Conditional Compilation**: Adding `#ifdef UNIT_TEST` blocks for test-specific code paths.

#### Guidelines for Source Modifications

- **Minimal Impact**: Changes should only affect test builds, never production code
- **Documented**: All test-specific modifications must be clearly documented
- **Reviewed**: Source changes for testing purposes require code review approval
- **Isolated**: Changes should be contained within the specific module being tested

#### Example: Weak Function Declaration

```c
// In source file (production code unchanged)
bool validate_request_data(const char *data) {
    // Implementation
}

// In test file
__attribute__((weak)) bool validate_request_data(const char *data);

// Test can now call the function directly
void test_validate_request_data(void) {
    bool result = validate_request_data("test_data");
    TEST_ASSERT_TRUE(result);
}
```

#### When NOT to Modify Source Code

- Adding test-specific function parameters or return values
- Changing function signatures to accommodate testing
- Adding conditional compilation that affects production behavior
- Creating test-only code paths that could introduce bugs

The decision to modify source code for testing should be made carefully, with the understanding that maintaining test isolation is generally preferable to source code changes.

## Mock Categories

### Library Mocks

#### mock_libmicrohttpd

**Purpose**: Mock HTTP server functions for testing HTTP-related code without requiring a real MHD daemon.

**Key Functions Mocked**:

- `MHD_lookup_connection_value()` - HTTP header/query parameter lookup
- `MHD_create_response_from_buffer()` - Response creation
- `MHD_add_response_header()` - Header addition
- `MHD_queue_response()` - Response queuing
- `MHD_destroy_response()` - Response cleanup

**Special Considerations**:

- **Troubleshooting Hotspot**: This mock is frequently problematic due to complex MHD connection structures and session management.
- **Key Issue**: Tests often fail because mock doesn't properly simulate MHD's connection info structures.
- **Common Pitfalls**:
  - Forgetting to call `mock_mhd_reset_all()` in `setUp()` and `tearDown()`
  - Not setting up connection info with `mock_mhd_set_connection_info()`
  - Using NULL connections without proper mock setup
  - Session management functions require careful state management

**Usage Example**:

```c
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

void setUp(void) {
    mock_mhd_reset_all();
    // Set up connection info if needed
    const union MHD_ConnectionInfo info = {.version = MHD_HTTP_VERSION_1_1};
    mock_mhd_set_connection_info(&info);
}

void test_http_request_parsing(void) {
    mock_mhd_add_lookup("Content-Type", "application/json");
    // Test code that calls MHD_lookup_connection_value()
}
```

#### mock_libwebsockets

**Purpose**: Mock WebSocket library functions for testing WebSocket server functionality.

**Key Functions Mocked**:

- `lws_create_context()` - WebSocket context creation
- `lws_write()` - WebSocket message writing
- `lws_service()` - WebSocket event servicing
- `lws_get_context()` - Context retrieval

**Usage Example**:

```c
#define USE_MOCK_LIBWEBSOCKETS
#include <unity/mocks/mock_libwebsockets.h>

void test_websocket_message_handling(void) {
    mock_lws_reset_all();
    mock_lws_set_write_result(LWS_WRITE_TEXT);
    // Test WebSocket message handling code
}
```

#### mock_launch

**Purpose**: Mock launch system functions for testing subsystem initialization phases.

**Key Functions Mocked**:

- Launch phase management
- Subsystem coordination functions

#### mock_landing

**Purpose**: Mock landing system functions for testing graceful shutdown procedures.

**Key Functions Mocked**:

- Landing phase management
- Shutdown coordination functions

#### mock_status

**Purpose**: Mock status reporting functions for testing system monitoring and status endpoints.

#### mock_info

**Purpose**: Mock info extraction functions for testing WebSocket metrics and system information retrieval.

#### Database Library Mocks

##### mock_libpq (PostgreSQL)

**Purpose**: Mock PostgreSQL client library functions for testing database operations without requiring a live PostgreSQL server.

##### mock_libmysqlclient (MySQL)

**Purpose**: Mock MySQL client library functions for testing MySQL database operations.

##### mock_libdb2 (DB2)

**Purpose**: Mock IBM DB2 client library functions for testing DB2 database operations.

##### mock_libsqlite3 (SQLite)

**Purpose**: Mock SQLite library functions for testing SQLite database operations.

### System Function Mocks

#### mock_system

**Purpose**: Mock low-level system functions to enable testing of error conditions and resource failures.

**Key Functions Mocked**:

- Memory management: `malloc()`, `free()`, `calloc()`, `realloc()`, `strdup()`
- Network functions: `gethostname()`, `recvfrom()`
- File I/O: `open()`, `read()`, `write()`, `close()`
- Process management: `fork()`, `waitpid()`, `kill()`
- Time functions: `nanosleep()`, `clock_gettime()`
- Dynamic loading: `dlopen()`, `dlclose()`, `dlerror()`
- Terminal/PTY: `openpty()`, `ioctl()`
- Synchronization: `sem_init()`

**Critical Architecture Notes**:

⚠️ **Global State Variables**: Mock state variables in `mock_system.c` are declared **without `static`** to ensure they are **shared across all object files**. This is essential for malloc mocking to work correctly when testing functions in separately compiled source files.

⚠️ **CMake Integration**: The build system uses `-include mock_system.h` to force inclusion before source processing. CMake flags must be properly separated (list format, not single string) to ensure the `-include` directive works correctly.

⚠️ **Memory Allocation Counter**: `malloc()`, `calloc()`, and `strdup()` **share the same call counter** since strdup internally uses malloc. When setting `mock_system_set_malloc_failure(N)`, it affects all three functions together.

**Special Considerations**:

- **Memory Allocation Testing**: Critical for testing out-of-memory conditions across source file boundaries
- **File Descriptor Management**: Important for testing I/O error scenarios
- **Process Control**: Essential for testing child process management
- **Shared Mock State**: All mock state variables are global to enable cross-compilation-unit testing

**Usage Example**:

```c
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

void setUp(void) {
    mock_system_reset_all();  // Critical: reset shared state
}

void test_memory_allocation_failure(void) {
    // Test first malloc/calloc failure
    mock_system_set_malloc_failure(1);
    void *ptr1 = calloc(1, 64);  // Call #1 - will fail
    void *ptr2 = malloc(100);    // Call #2 - will succeed
    TEST_ASSERT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    free(ptr2);
}

void test_third_allocation_failure(void) {
    // malloc, calloc, and strdup share the same counter
    mock_system_set_malloc_failure(3);
    void *p1 = malloc(100);      // Call #1 - succeed
    void *p2 = calloc(1, 64);    // Call #2 - succeed
    char *p3 = strdup("test");   // Call #3 - fail (shares counter!)
    void *p4 = malloc(100);      // Call #4 - succeed
    
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NULL(p3);        // strdup uses malloc internally
    TEST_ASSERT_NOT_NULL(p4);
    
    free(p1);
    free(p2);
    free(p4);
}
```

**Common Pitfalls**:

1. **Forgetting strdup shares the malloc counter**: When testing malloc failures, account for strdup calls
2. **Not resetting in setUp()**: Shared state from previous tests can cause failures
3. **Assuming static variables**: The variables are global - changes persist across all object files
4. **CMake configuration errors**: Improperly formatted `-include` flags prevent mock from working

#### mock_threads

**Purpose**: Mock threading functions for testing concurrent code paths.

#### mock_pthread

**Purpose**: Mock POSIX thread functions for testing thread synchronization.

### Specialized Mocks

#### mock_network

**Purpose**: Mock network interface functions for testing mDNS and network discovery without physical network dependencies.

**Key Functions Mocked**:

- `get_network_info()` - Network interface enumeration
- `filter_enabled_interfaces()` - Interface filtering
- `create_multicast_socket()` - Multicast socket creation

**Usage Example**:

```c
#define USE_MOCK_NETWORK
#include <unity/mocks/mock_network.h>

void test_mdns_initialization(void) {
    network_info_t *mock_info = create_mock_network_info();
    mock_network_set_get_network_info_result(mock_info);

    // Test mDNS initialization code
    network_info_t *result = get_network_info();
    TEST_ASSERT_NOT_NULL(result);
}
```

#### mock_terminal_websocket

**Purpose**: Mock terminal WebSocket specific functions for testing terminal session management.

#### mock_logging

**Purpose**: Mock logging functions to prevent test output pollution and enable log verification.

#### mock_database_engine

**Purpose**: Mock database engine abstraction layer for testing database-agnostic code.

#### mock_database_migrations

**Purpose**: Mock database migration functions for testing schema management.

#### mock_db2_transaction

**Purpose**: Mock DB2-specific transaction functions.

#### mock_dbqueue

**Purpose**: Mock database queue functions for testing asynchronous database operations.

## Best Practices

### Setup and Teardown

Always reset mocks in `setUp()` and `tearDown()`:

```c
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_network_reset_all();
    // Reset other mocks as needed
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_network_reset_all();
    // Reset other mocks as needed
}
```

### Test Isolation

Each test should be independent. Use mock control functions to set up specific scenarios:

```c
void test_specific_failure_scenario(void) {
    // Arrange
    mock_system_set_malloc_failure(1);
    mock_mhd_set_queue_response_result(MHD_NO);

    // Act
    bool result = function_under_test();

    // Assert
    TEST_ASSERT_FALSE(result);
}
```

### Mock_libmicrohttpd Troubleshooting

**Common Issues**:

1. **Connection Info Not Set**: MHD functions expect connection info to be properly initialized
2. **Session State Management**: Terminal session mocks require careful state tracking
3. **Response Creation Failures**: Mock response creation can fail unexpectedly

**Debugging Steps**:

1. Verify `mock_mhd_reset_all()` is called in setup
2. Check that connection info is set with `mock_mhd_set_connection_info()`
3. Ensure lookup values are added with `mock_mhd_add_lookup()`
4. Verify session management state is properly initialized

### Memory Management

Mocks that allocate memory should be properly cleaned up to prevent test interference:

```c
void tearDown(void) {
    // Clean up any mock-allocated resources
    mock_cleanup_allocated_resources();
}
```

## Integration with Build System

Mocks are automatically integrated into the Unity test build process via `CMakeLists-unity.cmake`. The build system:

1. Compiles mock implementations as separate object files
2. Links mocks into test executables based on preprocessor definitions
3. Ensures mocks don't affect production builds
4. Provides parallel compilation for improved build performance

## Finding and Using Mock Libraries

### Locating Mock Files

All mock libraries are located in `tests/unity/mocks/` with consistent naming:

- Headers: `mock_<library>.h`
- Implementations: `mock_<library>.c`

### Checking Mock Availability

To see which mocks are available and their current state:

```bash
# List all mock files
ls tests/unity/mocks/

# Check mock integration in build system
grep -n "UNITY_MOCK_SOURCES" cmake/CMakeLists-unity.cmake
```

### Updating Mocks for New Features

When additional library functions need mocking:

1. **Identify Missing Functions**: Run tests and note which functions are not mocked
2. **Add Function Declarations**: Update the mock header with new function prototypes
3. **Implement Mock Logic**: Add mock implementations in the .c file
4. **Add Control Functions**: Create setter functions for controlling mock behavior
5. **Update Build System**: Ensure new mock is included in `UNITY_MOCK_SOURCES`
6. **Test Integration**: Verify mock works with existing test patterns

### Example: Adding New Function to mock_system

```c
// In mock_system.h - Add function prototype and control functions
#ifdef USE_MOCK_SYSTEM
#define chdir mock_chdir
#endif

int mock_chdir(const char *path);
void mock_system_set_chdir_failure(int should_fail);

// Add extern declaration for the state variable
extern int mock_chdir_should_fail;

// In mock_system.c - Add implementation
// CRITICAL: Variables must be global (NOT static) to be shared across object files
int mock_chdir_should_fail = 0;
const char *mock_chdir_result = NULL;

int mock_chdir(const char *path) {
    (void)path;
    if (mock_chdir_should_fail) return -1;
    return 0;
}

// Add control function
void mock_system_set_chdir_failure(int should_fail) {
    mock_chdir_should_fail = should_fail;
}

// Update reset function
void mock_system_reset_all(void) {
    // ... existing resets ...
    mock_chdir_should_fail = 0;
    mock_chdir_result = NULL;
}
```

**Important**: State variables MUST be declared without `static` so they are visible and shared across all object files. This ensures that when a test sets a failure condition, the source code being tested sees the same state.

### Example: Creating a New Mock Library

When creating a new mock library for a function that needs to be mocked across multiple tests:

1. **Create Header** (`mock_newfunction.h`):

```c
#ifndef MOCK_NEWFUNCTION_H
#define MOCK_NEWFUNCTION_H

#ifdef USE_MOCK_NEWFUNCTION
#define newfunction mock_newfunction
#endif

char* mock_newfunction(void);
void mock_newfunction_set_result(const char* result);
void mock_newfunction_reset(void);

#endif
```

1. **Create Implementation** (`mock_newfunction.c`):

```c
#include <stdlib.h>
#include <string.h>
#include "mock_newfunction.h"

static const char* mock_result = NULL;

char* mock_newfunction(void) {
    if (mock_result) {
        return strdup(mock_result);
    }
    return NULL; // Simulate failure
}

void mock_newfunction_set_result(const char* result) {
    mock_result = result;
}

void mock_newfunction_reset(void) {
    mock_result = NULL;
}
```

1. **Update CMakeLists-unity.cmake**:
   - Add the new mock source to `UNITY_MOCK_SOURCES`
   - Add conditions for source and test compilation to include `-DUSE_MOCK_NEWFUNCTION` when appropriate

1. **Update Test Files**:
   - Include the mock header
   - Use the control functions in `setUp()` and `tearDown()`
   - Define `USE_MOCK_NEWFUNCTION` if needed (or let build system handle it)

**Real Example**: `mock_generate_query_id` - Created to mock `generate_query_id()` function for testing query ID generation failure scenarios.

### Mock State Management

**Reset Patterns**: Always call `mock_<name>_reset_all()` in both `setUp()` and `tearDown()`

**State Isolation**: Each test should configure mock state independently to avoid interference

**Memory Management**: Mocks that return allocated memory should be properly freed in `tearDown()`

## Adding New Mocks

To add a new mock library:

1. **Create Header** (`mock_newfeature.h`):

   ```c
   #ifndef MOCK_NEWFEATURE_H
   #define MOCK_NEWFEATURE_H

   #ifdef USE_MOCK_NEWFEATURE
   #define real_function mock_function
   #endif

   // Function declarations and control functions
   #endif
   ```

2. **Create Implementation** (`mock_newfeature.c`):

   ```c
   #include "mock_newfeature.h"

   // Mock state variables
   // Mock implementations
   // Control functions
   ```

3. **Update CMakeLists-unity.cmake**:
   Add the new mock source to `UNITY_MOCK_SOURCES`

   **CRITICAL CMAKE CONFIGURATION**: If your mock needs to override functions in specific source files,
   you MUST add detection logic in TWO places in CMakeLists-unity.cmake:

   a. **Source file compilation** (around line 78-122): Add string detection and mock defines

   ```cmake
   string(FIND "${SOURCE_FILE}" "yourmodule" IS_YOURMODULE_SOURCE)
   # Then in the conditional chain add:
   elseif(IS_YOURMODULE_SOURCE GREATER -1)
       set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
       set(MOCK_DEFINES "-DUSE_MOCK_YOURMODULE")
   ```

   b. **Test file compilation** (around line 260-304): Add string detection and mock defines

   ```cmake
   string(FIND "${TEST_SOURCE}" "yourmodule" IS_YOURMODULE_TEST)
   # Then in the conditional chain add:
   elseif(IS_YOURMODULE_TEST GREATER -1)
       set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
       set(MOCK_DEFINES "-DUSE_MOCK_YOURMODULE -DUSE_MOCK_LOGGING -Dlog_this=mock_log_this")
   ```

   **IMPORTANT**: When CMake defines `USE_MOCK_*`, test files should NOT manually define it.
   Remove any `#define USE_MOCK_*` lines and use a comment instead: `// USE_MOCK_* defined by CMake`

   **Example**: See the `launch` test configuration added in CMakeLists-unity.cmake as a reference.

4. **Update Test Files**:

   ```c
   // USE_MOCK_NEWFEATURE defined by CMake (don't define it manually)
   #include <unity/mocks/mock_newfeature.h>
   ```

5. **Add Documentation**: Update this README with the new mock details and examples

## Coverage and Testing

Mocks enable high test coverage (75%+) by allowing testing of:

- Error conditions that are difficult to trigger in real environments
- Resource exhaustion scenarios
- Network failure conditions
- System call failures
- Library unavailability

The mocking framework is essential for achieving comprehensive unit test coverage in the Hydrogen project.