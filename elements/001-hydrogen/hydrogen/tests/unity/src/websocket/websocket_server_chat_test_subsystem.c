/*
 * Unity Test File: WebSocket Chat Subsystem Tests
 * Tests chat_subsystem_init and chat_subsystem_cleanup functions
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat.h>

// Test function prototypes
void test_chat_subsystem_init_success(void);
void test_chat_subsystem_cleanup_not_initialized(void);
void test_chat_subsystem_init_then_cleanup(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_chat_subsystem_init_success(void) {
    int result = chat_subsystem_init();

    TEST_ASSERT_EQUAL_INT(0, result);

    // Cleanup to avoid leaking worker thread
    chat_subsystem_cleanup();
}

void test_chat_subsystem_cleanup_not_initialized(void) {
    // Cleanup should be safe even if init was never called
    chat_subsystem_cleanup();

    // If we get here without crashing, cleanup handled uninitialized state
    TEST_ASSERT_TRUE(1);
}

void test_chat_subsystem_init_then_cleanup(void) {
    int result = chat_subsystem_init();

    TEST_ASSERT_EQUAL_INT(0, result);

    chat_subsystem_cleanup();

    // If we get here without crashing, lifecycle completed successfully
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_chat_subsystem_init_success);
    RUN_TEST(test_chat_subsystem_cleanup_not_initialized);
    RUN_TEST(test_chat_subsystem_init_then_cleanup);

    return UNITY_END();
}
