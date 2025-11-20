/*
 * Unity Test File: Terminal WebSocket Helper Functions Tests
 * Tests the newly extracted helper functions from terminal_websocket.c
 * for I/O bridge operations
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <time.h>
#include <pthread.h>
#include <src/config/config_terminal.h>
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>

// Forward declarations for functions being tested
bool should_continue_io_bridge(TerminalWSConnection *connection);
int read_pty_with_select(TerminalWSConnection *connection, char *buffer, size_t buffer_size);
bool process_pty_read_result(TerminalWSConnection *connection, const char *buffer, int bytes_read);

// Function prototypes for test functions
void test_should_continue_io_bridge_null_connection(void);
void test_should_continue_io_bridge_inactive_connection(void);
void test_should_continue_io_bridge_null_session(void);
void test_should_continue_io_bridge_inactive_session(void);
void test_should_continue_io_bridge_null_pty_shell(void);
void test_should_continue_io_bridge_disconnected_websocket(void);
void test_should_continue_io_bridge_empty_session_id(void);
void test_should_continue_io_bridge_valid_state(void);
void test_read_pty_with_select_null_connection(void);
void test_read_pty_with_select_null_buffer(void);
void test_read_pty_with_select_null_session(void);
void test_read_pty_with_select_null_pty_shell(void);
void test_process_pty_read_result_null_connection(void);
void test_process_pty_read_result_positive_bytes(void);
void test_process_pty_read_result_zero_bytes(void);
void test_process_pty_read_result_interrupted(void);
void test_process_pty_read_result_error(void);
void test_process_pty_read_result_large_error(void);
void test_helper_functions_integration_inactive_connection(void);
void test_helper_functions_integration_valid_flow(void);

// Mock structures for testing - MUST match real structure layout exactly
typedef struct MockTerminalSession {
    char session_id[64];              // First field
    time_t created_time;              // Second field
    time_t last_activity;             // Third field
    struct PTYShell *pty_shell;       // Fourth field - matches real struct
    void *websocket_connection;
    void *pty_bridge_context;
    int terminal_rows;
    int terminal_cols;
    pthread_mutex_t session_mutex;
    bool active;                      // Matches real struct
    bool connected;                   // Matches real struct
    struct TerminalSession *next;
    struct TerminalSession *prev;
} MockTerminalSession;

typedef struct MockTerminalWSConnection {
    struct lws *wsi;                  // First field - matches real structure
    MockTerminalSession *session;     // Second field - matches real structure
    char session_id[64];              // Third field
    char *incoming_buffer;
    size_t incoming_size;
    size_t incoming_capacity;
    bool active;
    bool authenticated;
} MockTerminalWSConnection;

// Test fixtures
static MockTerminalWSConnection test_connection;
static MockTerminalSession test_session;

void setUp(void) {
    // Initialize test connection - must match real structure layout
    memset(&test_connection, 0, sizeof(test_connection));
    memset(&test_session, 0, sizeof(test_session));
    
    // Initialize connection fields
    test_connection.wsi = NULL;
    test_connection.session = &test_session;
    strncpy(test_connection.session_id, "test_session_123", sizeof(test_connection.session_id) - 1);
    test_connection.incoming_buffer = NULL;
    test_connection.incoming_size = 0;
    test_connection.incoming_capacity = 0;
    test_connection.active = true;
    test_connection.authenticated = false;
    
    // Initialize session fields matching real struct layout
    strncpy(test_session.session_id, "test_session_123", sizeof(test_session.session_id) - 1);
    test_session.created_time = time(NULL);
    test_session.last_activity = time(NULL);
    test_session.pty_shell = NULL;  // This is 4th field after session_id, created_time, last_activity
    test_session.websocket_connection = NULL;
    test_session.pty_bridge_context = NULL;
    test_session.terminal_rows = 24;
    test_session.terminal_cols = 80;
    pthread_mutex_init(&test_session.session_mutex, NULL);
    test_session.active = true;
    test_session.connected = true;
    test_session.next = NULL;
    test_session.prev = NULL;
}

void tearDown(void) {
    // Clean up any allocated resources
    if (test_connection.incoming_buffer) {
        free(test_connection.incoming_buffer);
        test_connection.incoming_buffer = NULL;
    }
    // Clean up mutex
    pthread_mutex_destroy(&test_session.session_mutex);
}

/*
 * TEST SUITE: should_continue_io_bridge
 * Tests the loop continuation validation logic
 */

void test_should_continue_io_bridge_null_connection(void) {
    bool result = should_continue_io_bridge(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_inactive_connection(void) {
    test_connection.active = false;
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_null_session(void) {
    test_connection.session = NULL;
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_inactive_session(void) {
    test_session.active = false;
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_null_pty_shell(void) {
    // PTY shell being NULL should still allow continuation
    test_session.pty_shell = NULL;
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    // Function logs warning but returns true (will skip reading in loop)
    TEST_ASSERT_TRUE(result);
}

void test_should_continue_io_bridge_disconnected_websocket(void) {
    test_session.connected = false;
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_empty_session_id(void) {
    test_session.session_id[0] = '\0';
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_FALSE(result);
}

void test_should_continue_io_bridge_valid_state(void) {
    // Test with valid state - all conditions met
    test_session.pty_shell = (struct PTYShell *)0x1; // Non-NULL pointer
    
    bool result = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_TRUE(result);
    
    // Reset for cleanup
    test_session.pty_shell = NULL;
}

/*
 * TEST SUITE: read_pty_with_select
 * Tests the select and read logic
 */

void test_read_pty_with_select_null_connection(void) {
    char buffer[256] = {0};
    int result = read_pty_with_select(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_read_pty_with_select_null_buffer(void) {
    // Test NULL buffer parameter with valid connection
    const char buffer[256] = {0};
    int result = read_pty_with_select((TerminalWSConnection*)&test_connection, NULL, 256);
    TEST_ASSERT_EQUAL_INT(-1, result);
    (void)buffer; // Suppress unused warning
}

void test_read_pty_with_select_null_session(void) {
    char buffer[256] = {0};
    test_connection.session = NULL;
    int result = read_pty_with_select((TerminalWSConnection*)&test_connection, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(-1, result);
    // Restore session for other tests
    test_connection.session = &test_session;
}

void test_read_pty_with_select_null_pty_shell(void) {
    char buffer[256] = {0};
    test_session.pty_shell = NULL;
    int result = read_pty_with_select((TerminalWSConnection*)&test_connection, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * TEST SUITE: process_pty_read_result
 * Tests the read result processing logic
 */

void test_process_pty_read_result_null_connection(void) {
    const char buffer[] = "test data";
    bool result = process_pty_read_result(NULL, buffer, 9);
    TEST_ASSERT_FALSE(result);
}

void test_process_pty_read_result_positive_bytes(void) {
    // Positive bytes_read should attempt to send data and return true
    const char buffer[] = "test data";
    bool result = process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, 9);
    // Will return true even if send fails (logs error but continues)
    TEST_ASSERT_TRUE(result);
}

void test_process_pty_read_result_zero_bytes(void) {
    // Zero bytes (timeout/no data) should continue
    const char buffer[256] = {0};
    bool result = process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, 0);
    TEST_ASSERT_TRUE(result);
}

void test_process_pty_read_result_interrupted(void) {
    // -2 means interrupted by signal, should continue
    const char buffer[256] = {0};
    bool result = process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, -2);
    TEST_ASSERT_TRUE(result);
}

void test_process_pty_read_result_error(void) {
    // Negative value (not -2) is an error, should exit
    const char buffer[256] = {0};
    bool result = process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, -1);
    TEST_ASSERT_FALSE(result);
}

void test_process_pty_read_result_large_error(void) {
    // Any negative value except -2 should exit
    const char buffer[256] = {0};
    bool result = process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, -100);
    TEST_ASSERT_FALSE(result);
}

/*
 * TEST SUITE: Integration tests
 * Tests multiple functions working together
 */

void test_helper_functions_integration_inactive_connection(void) {
    // Test: inactive connection should not continue
    test_connection.active = false;
    
    TEST_ASSERT_FALSE(should_continue_io_bridge((TerminalWSConnection*)&test_connection));
}

void test_helper_functions_integration_valid_flow(void) {
    // Test: valid connection state
    test_session.pty_shell = (struct PTYShell *)0x1; // Non-NULL pointer
    
    bool cont = should_continue_io_bridge((TerminalWSConnection*)&test_connection);
    TEST_ASSERT_TRUE(cont);
    
    // Test: successful read result should continue
    const char buffer[] = "test";
    TEST_ASSERT_TRUE(process_pty_read_result((TerminalWSConnection*)&test_connection, buffer, 4));
    
    // Reset for cleanup
    test_session.pty_shell = NULL;
}

int main(void) {
    UNITY_BEGIN();
    
    // should_continue_io_bridge tests
    RUN_TEST(test_should_continue_io_bridge_null_connection);
    RUN_TEST(test_should_continue_io_bridge_inactive_connection);
    RUN_TEST(test_should_continue_io_bridge_null_session);
    RUN_TEST(test_should_continue_io_bridge_inactive_session);
    RUN_TEST(test_should_continue_io_bridge_null_pty_shell);
    RUN_TEST(test_should_continue_io_bridge_disconnected_websocket);
    RUN_TEST(test_should_continue_io_bridge_empty_session_id);
    RUN_TEST(test_should_continue_io_bridge_valid_state);
    
    // read_pty_with_select tests
    RUN_TEST(test_read_pty_with_select_null_connection);
    RUN_TEST(test_read_pty_with_select_null_buffer);
    RUN_TEST(test_read_pty_with_select_null_session);
    RUN_TEST(test_read_pty_with_select_null_pty_shell);
    
    // process_pty_read_result tests
    RUN_TEST(test_process_pty_read_result_null_connection);
    RUN_TEST(test_process_pty_read_result_positive_bytes);
    RUN_TEST(test_process_pty_read_result_zero_bytes);
    RUN_TEST(test_process_pty_read_result_interrupted);
    RUN_TEST(test_process_pty_read_result_error);
    RUN_TEST(test_process_pty_read_result_large_error);
    
    // Integration tests
    RUN_TEST(test_helper_functions_integration_inactive_connection);
    RUN_TEST(test_helper_functions_integration_valid_flow);
    
    return UNITY_END();
}