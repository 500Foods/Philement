/*
 * Unity Test File: victoria_logs_send Tests
 * This file contains unit tests for the victoria_logs_send() function
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
void test_victoria_logs_send_not_initialized(void);
void test_victoria_logs_send_valid(void);
void test_victoria_logs_send_below_min_level(void);
void test_victoria_logs_send_special_chars(void);
void test_victoria_logs_send_many_quotes(void);
void test_victoria_logs_send_all_priorities(void);

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

// Test send when not initialized
void test_victoria_logs_send_not_initialized(void) {
    bool result = victoria_logs_send("TestSubsystem", "Test message", LOG_LEVEL_DEBUG);
    TEST_ASSERT_FALSE(result);
}

// Test send with valid initialization
void test_victoria_logs_send_valid(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    bool result = victoria_logs_send("TestSubsystem", "Test message", LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(result);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test send with priority below minimum level
void test_victoria_logs_send_below_min_level(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("VICTORIALOGS_LVL", "ERROR", 1);  // Only ERROR and above
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, victoria_logs_config.min_level);
    
    // DEBUG is below ERROR - should return true (silently skipped)
    bool result = victoria_logs_send("TestSubsystem", "Debug message", LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(result);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("VICTORIALOGS_LVL");
}

// Test send with special characters in message (needs JSON escaping)
void test_victoria_logs_send_special_chars(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Test with quotes
    bool result = victoria_logs_send("TestSubsystem", "Message with \"quotes\"", LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(result);
    
    // Test with newlines
    result = victoria_logs_send("TestSubsystem", "Line1\nLine2", LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(result);
    
    // Test with backslashes
    result = victoria_logs_send("TestSubsystem", "Path: C:\\Users\\test", LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(result);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test send with message that needs lots of escaping (may exceed buffer)
void test_victoria_logs_send_many_quotes(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Create a message with many quotes that will expand significantly when escaped
    char message[2000];
    memset(message, '"', sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    
    // This should handle the large message gracefully
    bool result = victoria_logs_send("TestSubsystem", message, LOG_LEVEL_DEBUG);
    // May return false if message too large after escaping, but should not crash
    (void)result;
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test all priority levels
void test_victoria_logs_send_all_priorities(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("VICTORIALOGS_LVL", "TRACE", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    // Test all priority levels
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "TRACE message", LOG_LEVEL_TRACE));
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "DEBUG message", LOG_LEVEL_DEBUG));
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "STATE message", LOG_LEVEL_STATE));
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "ALERT message", LOG_LEVEL_ALERT));
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "ERROR message", LOG_LEVEL_ERROR));
    TEST_ASSERT_TRUE(victoria_logs_send("Test", "FATAL message", LOG_LEVEL_FATAL));
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("VICTORIALOGS_LVL");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_send_not_initialized);
    RUN_TEST(test_victoria_logs_send_valid);
    RUN_TEST(test_victoria_logs_send_below_min_level);
    RUN_TEST(test_victoria_logs_send_special_chars);
    RUN_TEST(test_victoria_logs_send_many_quotes);
    RUN_TEST(test_victoria_logs_send_all_priorities);

    return UNITY_END();
}
