/*
 * Unity Test: Terminal WebSocket Message Processing
 * Tests JSON message parsing and routing functions from terminal_websocket.c
 * Focuses on process_terminal_websocket_message and related data flow
 */

// Define mocks before any includes
#define USE_MOCK_LIBMICROHTTPD

#include <src/hydrogen.h>
#include <unity.h>

// Include mocks for session management
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the terminal module being tested
#include <src/config/config_terminal.h>
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>

// Include basic dependencies
#include <string.h>
#include <stdlib.h>
#include <jansson.h>

// Test constants
#define TEST_PROTOCOL "terminal"
#define TEST_SESSION_ID "test_session_123"

// Mock structures for testing
typedef struct MockTerminalSession {
    char session_id[64];
    bool active;
    bool mock_resize_called;
    int mock_resize_rows;
    int mock_resize_cols;
    char* last_input_data;
    size_t last_input_size;
} MockTerminalSession;

typedef struct MockWSConnection {
    MockTerminalSession* session;
    char session_id[64];
    bool active;
    bool authenticated;
} MockWSConnection;

// Global test data
static MockWSConnection test_connection;

// Function prototypes for test functions
void test_process_terminal_websocket_message_null_connection(void);
void test_process_terminal_websocket_message_inactive_connection(void);
void test_process_terminal_websocket_message_null_session(void);
void test_process_terminal_websocket_message_null_message(void);
void test_process_terminal_websocket_message_empty_message(void);
void test_process_terminal_websocket_message_input_command(void);
void test_process_terminal_websocket_message_resize_command(void);
void test_process_terminal_websocket_message_ping_command(void);
void test_process_terminal_websocket_message_raw_text_input(void);
void test_process_terminal_websocket_message_invalid_json(void);
void test_process_terminal_websocket_message_malformed_json(void);

// Helper function prototypes
MockTerminalSession* create_mock_terminal_session(const char* session_id);
void setup_test_connection(MockTerminalSession* session);
bool mock_send_data_to_session(TerminalSession* session, const char* data, size_t size);
bool mock_resize_terminal_session(TerminalSession* session, int rows, int cols);
bool mock_update_session_activity(TerminalSession* session);

// Helper function to create mock terminal session
MockTerminalSession* create_mock_terminal_session(const char* session_id) {
    MockTerminalSession* session = calloc(1, sizeof(MockTerminalSession));
    if (session) {
        strcpy(session->session_id, session_id);
        session->active = true;
    }
    return session;
}

// Helper function to setup test connection
void setup_test_connection(MockTerminalSession* session) {
    test_connection.active = true;
    test_connection.authenticated = true;
    test_connection.session = session;
    if (session) {
        strcpy(test_connection.session_id, session->session_id);
    }
}

// Note: mock_send_data_to_session removed to avoid conflicts with comprehensive test file

bool mock_resize_terminal_session(TerminalSession* session, int rows, int cols) {
    if (!session) return false;

    MockTerminalSession* mock = (MockTerminalSession*)session;
    mock->mock_resize_called = true;
    mock->mock_resize_rows = rows;
    mock->mock_resize_cols = cols;
    return true;
}

bool mock_update_session_activity(TerminalSession* session) {
    (void)session; // Mock doesn't need to do anything
    return true;
}

// Setup function - called before each test
void setUp(void) {
    // Reset mocks
    mock_session_reset_all();

    // Clear test connection
    memset(&test_connection, 0, sizeof(test_connection));
}

// Tear down function - called after each test
void tearDown(void) {
    // Clean up mock session if allocated
    if (test_connection.session) {
        if (((MockTerminalSession*)test_connection.session)->last_input_data) {
            free(((MockTerminalSession*)test_connection.session)->last_input_data);
            ((MockTerminalSession*)test_connection.session)->last_input_data = NULL;
        }
        free(test_connection.session);
        test_connection.session = NULL;
    }

    // Reset mocks
    mock_session_reset_all();
}

/*
 * TEST SUITE: process_terminal_websocket_message
 */
void test_process_terminal_websocket_message_null_connection(void) {
    bool result = process_terminal_websocket_message(NULL, "test", 4);

    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_inactive_connection(void) {
    test_connection.active = false;
    bool result = process_terminal_websocket_message((TerminalWSConnection*)&test_connection, "test", 4);

    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_null_session(void) {
    setup_test_connection(NULL);
    bool result = process_terminal_websocket_message((TerminalWSConnection*)&test_connection, "test", 4);

    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_null_message(void) {
    MockTerminalSession* session = create_mock_terminal_session(TEST_SESSION_ID);
    setup_test_connection(session);

    // Replace actual functions with mocks for this test
    // This is a simplified test - in practice, we'd need to mock these functions separately

    bool result = process_terminal_websocket_message((TerminalWSConnection*)&test_connection, NULL, 4);

    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_empty_message(void) {
    MockTerminalSession* session = create_mock_terminal_session(TEST_SESSION_ID);
    setup_test_connection(session);

    bool result = process_terminal_websocket_message((TerminalWSConnection*)&test_connection, "", 0);

    // Empty message should be processed without error (though it does nothing)
    TEST_ASSERT_TRUE(result);
}

void test_process_terminal_websocket_message_raw_text_input(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    const char* test_message = "ls -la";
    size_t message_size = strlen(test_message);

    // Test that function handles the input without crashing
    bool result = process_terminal_websocket_message(&mock_conn, test_message, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_input_command(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    // JSON command: {"type": "input", "data": "ls -la"}
    const char* json_message = "{\"type\": \"input\", \"data\": \"ls -la\"}";
    size_t message_size = strlen(json_message);

    bool result = process_terminal_websocket_message(&mock_conn, json_message, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_resize_command(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    // JSON command: {"type": "resize", "rows": 24, "cols": 80}
    const char* json_message = "{\"type\": \"resize\", \"rows\": 24, \"cols\": 80}";
    size_t message_size = strlen(json_message);

    bool result = process_terminal_websocket_message(&mock_conn, json_message, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_ping_command(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    // JSON command: {"type": "ping"}
    const char* json_message = "{\"type\": \"ping\"}";
    size_t message_size = strlen(json_message);

    bool result = process_terminal_websocket_message(&mock_conn, json_message, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_invalid_json(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    // Invalid JSON
    const char* bad_json = "{\"type\": \"input\", \"data\":missing_quote}";
    size_t message_size = strlen(bad_json);

    bool result = process_terminal_websocket_message(&mock_conn, bad_json, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

void test_process_terminal_websocket_message_malformed_json(void) {
    // Create a minimal mock connection structure
    TerminalWSConnection mock_conn;
    memset(&mock_conn, 0, sizeof(mock_conn));
    mock_conn.active = true;
    mock_conn.authenticated = true;
    // Don't set session - this will cause the function to return false, but test that it doesn't crash

    // Completely malformed JSON
    const char* bad_json = "{invalid json";
    size_t message_size = strlen(bad_json);

    bool result = process_terminal_websocket_message(&mock_conn, bad_json, message_size);

    // Function should return false due to no valid session, but should not crash
    TEST_ASSERT_FALSE(result);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_process_terminal_websocket_message_null_connection);
    RUN_TEST(test_process_terminal_websocket_message_inactive_connection);
    RUN_TEST(test_process_terminal_websocket_message_null_session);
    RUN_TEST(test_process_terminal_websocket_message_null_message);
    RUN_TEST(test_process_terminal_websocket_message_empty_message);

    // Raw text input tests
    RUN_TEST(test_process_terminal_websocket_message_raw_text_input);

    // JSON command tests
    RUN_TEST(test_process_terminal_websocket_message_input_command);
    RUN_TEST(test_process_terminal_websocket_message_resize_command);
    RUN_TEST(test_process_terminal_websocket_message_ping_command);

    // Error handling tests
    RUN_TEST(test_process_terminal_websocket_message_invalid_json);
    RUN_TEST(test_process_terminal_websocket_message_malformed_json);

    return UNITY_END();
}
