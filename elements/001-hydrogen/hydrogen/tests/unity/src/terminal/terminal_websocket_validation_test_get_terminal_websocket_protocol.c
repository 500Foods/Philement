/*
 * Unity Test File: Terminal WebSocket get_terminal_websocket_protocol Function Tests
 * Tests the get_terminal_websocket_protocol function for WebSocket protocol handling
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/terminal/terminal_websocket.h>

// Forward declarations for functions being tested
const char *get_terminal_websocket_protocol(void);

// Function prototypes for test functions
void test_get_terminal_websocket_protocol_returns_expected_value(void);
void test_get_terminal_websocket_protocol_non_empty(void);
void test_get_terminal_websocket_protocol_null_terminated(void);

void setUp(void) {
    // No setup needed for this simple function
}

void tearDown(void) {
    // No cleanup needed for this simple function
}

// Test basic functionality
void test_get_terminal_websocket_protocol_returns_expected_value(void) {
    const char *protocol = get_terminal_websocket_protocol();
    TEST_ASSERT_NOT_NULL(protocol);
    TEST_ASSERT_EQUAL_STRING("terminal", protocol);
}

// Test that the function returns a non-empty string
void test_get_terminal_websocket_protocol_non_empty(void) {
    const char *protocol = get_terminal_websocket_protocol();
    TEST_ASSERT_NOT_NULL(protocol);
    TEST_ASSERT_GREATER_THAN(0, strlen(protocol));
}

// Test that the function returns a valid C string (null-terminated)
void test_get_terminal_websocket_protocol_null_terminated(void) {
    const char *protocol = get_terminal_websocket_protocol();
    TEST_ASSERT_NOT_NULL(protocol);

    // Find the null terminator with bounds checking
    size_t len = 0;
    while (len < 1000 && protocol[len] != '\0') { // Safety limit first
        len++;
    }
    TEST_ASSERT_LESS_THAN(1000, len); // Should have found null terminator within limit
    TEST_ASSERT_EQUAL('\0', protocol[len]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_terminal_websocket_protocol_returns_expected_value);
    RUN_TEST(test_get_terminal_websocket_protocol_non_empty);
    RUN_TEST(test_get_terminal_websocket_protocol_null_terminated);

    return UNITY_END();
}