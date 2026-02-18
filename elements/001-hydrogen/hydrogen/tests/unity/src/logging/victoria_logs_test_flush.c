/*
 * Unity Test File: victoria_logs_flush Tests
 * This file contains unit tests for the victoria_logs_flush() function
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
void test_victoria_logs_flush_not_initialized(void);
void test_victoria_logs_flush_after_init(void);
void test_victoria_logs_flush_with_messages(void);

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

// Test flush when not initialized (should not crash)
void test_victoria_logs_flush_not_initialized(void) {
    // Should not crash even if never initialized
    victoria_logs_flush();
    TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
}

// Test flush when initialized
void test_victoria_logs_flush_after_init(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    TEST_ASSERT_TRUE(result);
    
    victoria_logs_flush();
    TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test flush with messages
void test_victoria_logs_flush_with_messages(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    TEST_ASSERT_TRUE(result);
    
    // Send some messages
    victoria_logs_send("Test", "Message 1", LOG_LEVEL_DEBUG);
    victoria_logs_send("Test", "Message 2", LOG_LEVEL_DEBUG);
    
    // Flush
    victoria_logs_flush();
    TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_flush_not_initialized);
    RUN_TEST(test_victoria_logs_flush_after_init);
    RUN_TEST(test_victoria_logs_flush_with_messages);

    return UNITY_END();
}
