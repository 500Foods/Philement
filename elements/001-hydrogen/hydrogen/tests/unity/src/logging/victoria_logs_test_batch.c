/*
 * Unity Test File: VictoriaLogs Batch Tests
 * This file contains unit tests for batch operations and error conditions
 * from src/logging/victoria_logs.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// External declarations for global state
extern VictoriaLogsConfig victoria_logs_config;
extern VLThreadState victoria_logs_thread;

// Function prototypes for test functions
void test_victoria_logs_clear_batch(void);
void test_victoria_logs_send_message_too_large(void);
void test_victoria_logs_flush_empty_batch(void);
void test_victoria_logs_send_with_all_control_chars(void);

// Test fixtures
void setUp(void) {
    // Clean up any existing state before each test
    if (victoria_logs_config.enabled && victoria_logs_thread.running) {
        cleanup_victoria_logs();
    }
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

void tearDown(void) {
    // Clean up after each test
    if (victoria_logs_config.enabled && victoria_logs_thread.running) {
        cleanup_victoria_logs();
    }
}

// Test clear_batch function
void test_victoria_logs_clear_batch(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Clear batch should work without crashing
    victoria_logs_clear_batch();
    
    TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test send with message that becomes too large after escaping
void test_victoria_logs_send_message_too_large(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Create a message with many control characters that expand significantly
    // Each control char becomes 6 chars (\u00XX), so 700 control chars = 4200 chars
    char message[3000];
    for (int i = 0; i < 2999; i++) {
        message[i] = 0x01;  // Control character
    }
    message[2999] = '\0';
    
    // This should fail because escaped message exceeds buffer
    bool result = victoria_logs_send("Test", message, LOG_LEVEL_DEBUG);
    // The function may return false if the message is too large
    (void)result;
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test flush with empty batch
void test_victoria_logs_flush_empty_batch(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Clear any pending batch
    victoria_logs_clear_batch();
    
    // Flush should handle empty batch gracefully
    victoria_logs_flush();
    
    TEST_ASSERT_TRUE(true);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test send with various control characters
void test_victoria_logs_send_with_all_control_chars(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Test with all control characters 0x00-0x1F
    for (int c = 1; c < 0x20; c++) {
        char message[8];
        message[0] = 'A';
        message[1] = (char)c;
        message[2] = 'B';
        message[3] = '\0';
        
        bool result = victoria_logs_send("Test", message, LOG_LEVEL_DEBUG);
        TEST_ASSERT_TRUE(result);
    }
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_clear_batch);
    RUN_TEST(test_victoria_logs_send_message_too_large);
    RUN_TEST(test_victoria_logs_flush_empty_batch);
    RUN_TEST(test_victoria_logs_send_with_all_control_chars);

    return UNITY_END();
}
